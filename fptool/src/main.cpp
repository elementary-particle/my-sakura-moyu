#include "fvp/image.h"
#include "fvp/package.h"
#include <vector>

#define FPTOOL
#include "../../filelist.h"

int runDiff(std::vector<PatchEntry> &entryArray, char *inputName,
            char *outputName, FvpPackage &output) {
  FvpFileSys fileSys;

  for (PatchEntry &entry : entryArray) {
    char *nameExt, folderPrefix[NAME_SIZE], filePath[NAME_SIZE];
    FILE *patchFile;
    uint8_t *buffer;
    size_t size;
    bool isImage;

    if (strcpy_s(folderPrefix, outputName)) {
      return -4;
    }
    nameExt = strrchr(folderPrefix, '.');
    if (nameExt) {
      *nameExt = 0;
    }
    if (strcat_s(folderPrefix, "/") || strcpy_s(filePath, folderPrefix) ||
        strcat_s(filePath, entry.patchName)) {
      return -4;
    }
    if (!(entry.methodType == METHOD_REPL || entry.methodType == METHOD_DIFF ||
          entry.methodType == METHOD_APPEND)) {
      return -5;
    }
    patchFile = fopen(filePath, "rb");
    if (!patchFile) {
      FvpImage patchImage;
      if (strcat_s(filePath, ".png")) {
        return -4;
      }
      patchFile = fopen(filePath, "rb");
      if (!patchFile) {
        return -6;
      }
      if (patchImage.readPng(patchFile) < 0) {
        return -6;
      }
      patchImage.write(&buffer, &size);
      isImage = true;
    } else {
      fseek(patchFile, 0, SEEK_END);
      size = ftell(patchFile);
      buffer = new uint8_t[size];
      rewind(patchFile);
      fread(buffer, size, 1, patchFile);
      isImage = FvpImage::checkSig(buffer);
    }

    if (isImage) {
      FvpImage patchImage, originImage;
      uint8_t *originBuffer;
      size_t originSize;

      if (entry.methodType != METHOD_APPEND) {
        if (fileSys.openPath(&originBuffer, &originSize, entry.originalName,
                             entry.methodType != METHOD_DIFF) < 0) {
          return -7;
        }
        patchImage.read(buffer, size);
        originImage.read(originBuffer, originSize);
        if (entry.offsetX || entry.offsetY) {
          patchImage.offsetX = entry.offsetX;
          patchImage.offsetY = entry.offsetY;
        } else if (!(patchImage.offsetX || patchImage.offsetY)) {
          patchImage.offsetX = originImage.offsetX;
          patchImage.offsetY = originImage.offsetY;
        }

        if (entry.methodType == METHOD_DIFF) {
          uint32_t offset;
          if (originImage.type == patchImage.type &&
              originImage.size == patchImage.size) {
            for (offset = FvpImage::HEADER_SIZE; offset < size; offset++) {
              buffer[offset] -= originBuffer[offset];
            }
          } else {
            fprintf(stderr,
                    "Warning: %s: mismatch: falling back to replacement\n",
                    entry.originalName);
          }
        }
        delete[] originBuffer;
      } else {
        if (entry.offsetX || entry.offsetY) {
          patchImage.offsetX = entry.offsetX;
          patchImage.offsetY = entry.offsetY;
        }
      }
      patchImage.write(&buffer, &size);
      entry.fileEntryPtr->buffer = buffer;
      entry.fileEntryPtr->size = size;
      output.writeFile(entry.fileEntryPtr, true);
    } else {
      entry.fileEntryPtr->buffer = buffer;
      entry.fileEntryPtr->size = size;
      output.writeFile(entry.fileEntryPtr, false);
    }
    entry.fileEntryPtr->freeBuffer();
    printf("%32s %32s %u\n", entry.originalName, entry.patchName,
           entry.fileEntryPtr->storeSize);
  }
  return 0;
}

int runPatch(std::vector<PatchEntry> &entryArray, char *inputName,
             FvpPackage &output) {
  FvpFileSys fileSys;
  FILE *diffFile;

  diffFile = fopen(inputName, "rb");
  if (!diffFile) {
    return -4;
  }
  FvpPackage diff(diffFile);
  if (diff.readHeader() < 0) {
    fclose(diffFile);
    return -5;
  }
  for (PatchEntry &entry : entryArray) {
    FvpPackage::FileEntry *diffEntryPtr;
    uint8_t *buffer;
    size_t size;

    diffEntryPtr = diff.getFileByName(entry.patchName);
    if (!diffEntryPtr) {
      fprintf(stderr, "Warning: %s: file not found\n", entry.originalName);
      continue;
    }
    if (entry.methodType == METHOD_REPL || entry.methodType == METHOD_APPEND) {
      diff.readFile(diffEntryPtr, false);
      entry.fileEntryPtr->buffer = diffEntryPtr->buffer;
      entry.fileEntryPtr->size = diffEntryPtr->storeSize;
      output.writeFile(entry.fileEntryPtr, false);
      entry.fileEntryPtr->buffer = nullptr;
      diffEntryPtr->freeBuffer();
    } else if (entry.methodType == METHOD_DIFF) {
      bool isImage;
      uint32_t i;

      diff.readFileInfo(diffEntryPtr);
      isImage = FvpImage::checkSig(diffEntryPtr->buffer);
      if (!isImage) {
        diffEntryPtr->freeBuffer();
        fprintf(
            stderr,
            "Warning: %s: diff method specified, but not a image, skipping\n",
            entry.originalName);
        continue;
      }
      if (fileSys.openPath(&buffer, &size, entry.originalName, false) < 0) {
        diffEntryPtr->freeBuffer();
        fprintf(stderr, "Warning: %s: cannot open original file, skipping\n",
                entry.originalName);
        continue;
      }
      diff.readFile(diffEntryPtr);
      if (size != diffEntryPtr->size) {
        fprintf(stderr, "Warning: %s: diff patch size mismatch, skipping\n",
                entry.originalName);
        diffEntryPtr->freeBuffer();
        delete[] buffer;
        continue;
      }
      memcpy(buffer, diffEntryPtr->buffer, FvpImage::HEADER_SIZE);
      for (i = FvpImage::HEADER_SIZE; i < size; i++) {
        buffer[i] += diffEntryPtr->buffer[i];
      }
      diffEntryPtr->freeBuffer();
      entry.fileEntryPtr->buffer = buffer;
      entry.fileEntryPtr->size = size;
      output.writeFile(entry.fileEntryPtr, true);
      entry.fileEntryPtr->freeBuffer();
    } else {
      fprintf(stderr, "Warning: %s: unrecognized patching method, skipping\n",
              entry.originalName);
    }
  }
  fclose(diffFile);
  return 0;
}

int main(int argc, char *argv[]) {
  /* -[dp] <manifest> -i <input> -o <output> */
  int i;
  bool doDiff, doPatch;
  char *listName, *inputName, *outputName;

  doDiff = doPatch = false;
  listName = inputName = outputName = nullptr;
#define STR_ARG(v)                                                             \
  {                                                                            \
    i++;                                                                       \
    if (i >= argc) {                                                           \
      return 2;                                                                \
    }                                                                          \
    (v) = argv[i];                                                             \
  }
  for (i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      switch (argv[i][1]) {
      case 'd':
        doDiff = true;
        goto argList;
      case 'p':
        doPatch = true;
      argList:
        if (i + 1 < argc && argv[i + 1][0] != '-') {
          STR_ARG(listName)
        }
        break;
      case 'i':
        STR_ARG(inputName)
        break;
      case 'o':
        STR_ARG(outputName)
        break;
      default:
        return 1;
      }
    }
  }

  FILE *listFile, *outputFile;
  FvpPackage output;
  std::vector<PatchEntry> entryArray;
  int result;

  result = -1;
  if ((doDiff || doPatch) && outputName) {
    if (listName) {
      PatchEntry entry{};
      listFile = fopen(listName, "r");
      if (!listFile) {
        return -2;
      }
      while (fscanf(listFile, "%4s%32s%32s%d%d", entry.method,
                    entry.originalName, entry.patchName, &entry.offsetX,
                    &entry.offsetY) != EOF) {

        if (strcmp(entry.method, "R") == 0) {
          entry.methodType = METHOD_REPL;
        } else if (strcmp(entry.method, "P") == 0) {
          entry.methodType = METHOD_DIFF;
        } else if (strcmp(entry.method, "A") == 0) {
          entry.methodType = METHOD_APPEND;
        } else {
          fclose(listFile);
          return -3;
        }
        entry.fileEntryPtr = output.appendFile(entry.patchName);
        entryArray.push_back(entry);
      }
      fclose(listFile);
    } else {
      entryArray = std::move(defaultEntryList);
      for (PatchEntry &entry : entryArray) {
        entry.fileEntryPtr = output.appendFile(entry.patchName);
      }
    }
    outputFile = fopen(outputName, "wb");
    if (!outputFile) {
      return -2;
    }
    output.packageFile = outputFile;
    output.writeHeader();

    result = -1;
    if (doDiff) {
      result = runDiff(entryArray, inputName, outputName, output);
    } else if (inputName) {
      result = runPatch(entryArray, inputName, output);
    }

    output.writeEnd();
    fclose(outputFile);
  }
  return result;
}
