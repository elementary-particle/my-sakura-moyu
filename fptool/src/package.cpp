#include "fvp/package.h"
#include "fvp/image.h"
#include <cstring>
#include <windows.h>
#include <zlib.h>

bool FvpPackage::FileCompare::operator()(const FileEntry *entry0,
                                         const FileEntry *entry1) const {
  return CompareStringA(0x411, NORM_IGNORECASE, entry0->name, -1, entry1->name,
                        -1) == 1;
}

FvpPackage::FvpPackage(FILE *file) {
  packageFile = file;
  fileCount = nameHeaderSize = 0;
  fileNameBuffer = nullptr;
  writing = false;
}

int FvpPackage::readHeader() {
  int i;

  rewind(packageFile);

  fread(&fileCount, sizeof(uint32_t), 1, packageFile);
  fread(&nameHeaderSize, sizeof(uint32_t), 1, packageFile);

  auto **ptrList = new FileEntry *[fileCount];

  for (i = 0; i < fileCount; i++) {
    FileEntry *fileEntryPtr;

    fileEntryPtr = new FileEntry;

    fread(&fileEntryPtr->nameOffset, sizeof(uint32_t), 1, packageFile);
    fread(&fileEntryPtr->offset, sizeof(uint32_t), 1, packageFile);
    fread(&fileEntryPtr->storeSize, sizeof(uint32_t), 1, packageFile);

    ptrList[i] = fileEntryPtr;
  }

  fileNameBuffer = new char[nameHeaderSize];
  fread(fileNameBuffer, nameHeaderSize, 1, packageFile);

  for (i = 0; i < fileCount; i++) {
    FileEntry *fileEntryPtr;
    char *name;
    uint32_t remainSize;

    fileEntryPtr = ptrList[i];
    if (fileEntryPtr->nameOffset >= nameHeaderSize) {
      delete[] ptrList;
      return -1;
    }
    name = fileNameBuffer + fileEntryPtr->nameOffset;
    remainSize = nameHeaderSize - fileEntryPtr->nameOffset;
    fileEntryPtr->nameSize = strnlen(name, remainSize);
    if (fileEntryPtr->nameSize == remainSize) {
      fileEntryPtr->nameSize = 0;
      delete[] ptrList;
      return -2;
    }
    fileEntryPtr->nameSize++;
    fileEntryPtr->name = new char[fileEntryPtr->nameSize];
    memcpy(fileEntryPtr->name, name, fileEntryPtr->nameSize);
    fileEntrySet.insert(fileEntryPtr);
  }
  delete[] ptrList;
  return 0;
}

FvpPackage::FileEntry *FvpPackage::getFileByName(const char *name) {
  FileEntry target;

  target.name = (char *)name;
  auto iter = fileEntrySet.find(&target);
  target.name = nullptr;
  if (iter == fileEntrySet.end()) {
    return nullptr;
  } else {
    return *iter;
  }
}

int FvpPackage::readFileInfo(FileEntry *fileEntryPtr) const {
  uint8_t *buffer;
  uint32_t signature;
  int infoSize;

  if (fileEntryPtr->storeSize < 4) {
    return -1;
  }
  if (fseek(packageFile, (long)fileEntryPtr->offset, SEEK_SET)) {
    return -2;
  }
  buffer = new uint8_t[Z_HEADER_SIZE];
  fread(buffer, 4, 1, packageFile);

  signature = *((uint32_t *)buffer);
  if (signature == Z_SIGNATURE) {
    if (fileEntryPtr->storeSize < 16) {
      // not enough space for zip header and signature bytes
      delete[] buffer;
      return -3;
    }
    fread(buffer + 4, 8, 1, packageFile);
    infoSize = *((int32_t *)(buffer + 8));
    if (infoSize < 4) {
      // not enough info space for file signature
      delete[] buffer;
      return -4;
    }
    if (infoSize > Z_HEADER_SIZE) {
      uint8_t *newBuffer;

      newBuffer = new uint8_t[infoSize];
      memcpy(newBuffer, buffer, 12);
      delete[] buffer;
      buffer = newBuffer;
    }
    fread(buffer, infoSize, 1, packageFile);
    signature = *((uint32_t *)(buffer + 12));
  } else if (signature == FvpImage::SIGNATURE) {
    infoSize = FvpImage::HEADER_SIZE;
    fread(buffer + 4, FvpImage::HEADER_SIZE - 4, 1, packageFile);
  } else {
    infoSize = 4;
  }
  delete[] fileEntryPtr->buffer;
  fileEntryPtr->buffer = buffer;
  return infoSize;
}

int FvpPackage::readFile(FileEntry *fileEntryPtr, bool doInflate) const {
  uint8_t *buffer;

  if (fseek(packageFile, (long)fileEntryPtr->offset, SEEK_SET)) {
    return -1;
  }

  buffer = new uint8_t[fileEntryPtr->storeSize];
  if (fread(buffer, fileEntryPtr->storeSize, 1, packageFile) < 1) {
    delete[] buffer;
    return -2;
  }

  if (doInflate && *((uint32_t *)buffer) == Z_SIGNATURE) {
    /* deflated file */
    uint32_t contentSize, infoSize;
    uint8_t *inflateBuffer;
    z_stream stream;

    contentSize = *((uint32_t *)(buffer + 4));
    infoSize = *((uint32_t *)(buffer + 8));
    fileEntryPtr->size = infoSize + contentSize;
    inflateBuffer = new uint8_t[fileEntryPtr->size];
    memcpy(inflateBuffer, buffer + 12, infoSize);
    stream.next_in = buffer + 12 + infoSize;
    stream.avail_in = fileEntryPtr->storeSize - (12 + infoSize);
    stream.next_out = inflateBuffer + infoSize;
    stream.avail_out = contentSize;

    stream.zalloc = zAlloc;
    stream.zfree = zFree;
    stream.opaque = Z_NULL;

    inflateInit(&stream);

    if (inflate(&stream, Z_FINISH) != Z_STREAM_END) {
      inflateEnd(&stream);
      delete[] buffer;
      delete[] inflateBuffer;
      return -3;
    }
    inflateEnd(&stream);

    delete[] buffer;
    delete[] fileEntryPtr->buffer;
    fileEntryPtr->buffer = inflateBuffer;
  } else {
    fileEntryPtr->size = fileEntryPtr->storeSize;
    delete[] fileEntryPtr->buffer;
    fileEntryPtr->buffer = buffer;
  }

  return 0;
}

FvpPackage::FileEntry *FvpPackage::appendFile(const char *name) {
  FileEntry *fileEntryPtr;

  if (writing) {
    return nullptr;
  }

  fileEntryPtr = new FileEntry;
  fileEntryPtr->nameSize = strlen(name) + 1;
  fileEntryPtr->name = new char[fileEntryPtr->nameSize];
  memcpy(fileEntryPtr->name, name, fileEntryPtr->nameSize);
  fileCount++;
  fileEntrySet.insert(fileEntryPtr);

  return fileEntryPtr;
}

int FvpPackage::writeHeader() {
  uint32_t nameOffset;

  rewind(packageFile);

  if (fileCount == 0) {
    return 0;
  }
  nameOffset = 0;
  fwrite(&fileCount, sizeof(uint32_t), 1, packageFile);
  fwrite(&nameOffset, sizeof(uint32_t), 1, packageFile);

  for (FileEntry *const &fileEntryPtr : fileEntrySet) {
    fileEntryPtr->nameOffset = nameOffset;
    fwrite(&fileEntryPtr->nameOffset, sizeof(uint32_t), 1, packageFile);
    fwrite(&fileEntryPtr->offset, sizeof(uint32_t), 1, packageFile);
    fwrite(&fileEntryPtr->storeSize, sizeof(uint32_t), 1, packageFile);

    nameOffset += fileEntryPtr->nameSize;
  }
  nameHeaderSize = nameOffset;

  for (FileEntry *const &fileEntryPtr : fileEntrySet) {
    fwrite(fileEntryPtr->name, fileEntryPtr->nameSize, 1, packageFile);
  }

  writing = true;
  return 0;
}

int FvpPackage::writeFile(FileEntry *fileEntryPtr, bool doDeflate) const {
  if (!writing) {
    return -1;
  }

  fflush(packageFile);
  fileEntryPtr->offset = ftell(packageFile);
  if (doDeflate) {
    uint32_t totalSize, deflateSize;
    uint8_t *deflateBuffer;
    z_stream stream;

    stream.next_in = fileEntryPtr->buffer + Z_HEADER_SIZE;
    stream.avail_in = fileEntryPtr->size - Z_HEADER_SIZE;
    stream.next_out = nullptr;
    stream.avail_out = 0;
    stream.opaque = Z_NULL;
    stream.zalloc = zAlloc;
    stream.zfree = zFree;
    deflateInit2(&stream, Z_BEST_COMPRESSION, Z_DEFLATED, 15, 8, Z_FILTERED);
    deflateSize = deflateBound(&stream, stream.avail_in);
    totalSize = 12 + Z_HEADER_SIZE + deflateSize;
    deflateBuffer = new uint8_t[totalSize];
    stream.next_out = deflateBuffer + 12 + Z_HEADER_SIZE;
    stream.avail_out = deflateSize;

    *((uint32_t *)deflateBuffer) = Z_SIGNATURE;
    *((uint32_t *)(deflateBuffer + 4)) = fileEntryPtr->size - Z_HEADER_SIZE;
    *((uint32_t *)(deflateBuffer + 8)) = Z_HEADER_SIZE;
    memcpy(deflateBuffer + 12, fileEntryPtr->buffer, Z_HEADER_SIZE);

    if (deflate(&stream, Z_FINISH) != Z_STREAM_END) {
      delete[] deflateBuffer;
      deflateEnd(&stream);
      return -2;
    }
    totalSize = 12 + Z_HEADER_SIZE + stream.total_out;
    deflateEnd(&stream);

    fileEntryPtr->storeSize = totalSize;
    fwrite(deflateBuffer, totalSize, 1, packageFile);
    delete[] deflateBuffer;
  } else {
    fileEntryPtr->storeSize = fileEntryPtr->size;
    fwrite(fileEntryPtr->buffer, fileEntryPtr->size, 1, packageFile);
  }

  return 0;
}

int FvpPackage::writeEnd() {
  long offset;

  if (!writing) {
    return -1;
  }
  fflush(packageFile);
  fseek(packageFile, sizeof(uint32_t), SEEK_SET);
  fwrite(&nameHeaderSize, sizeof(uint32_t), 1, packageFile);
  for (FileEntry *const &fileEntryPtr : fileEntrySet) {
    fseek(packageFile, sizeof(uint32_t), SEEK_CUR);
    fwrite(&fileEntryPtr->offset, sizeof(uint32_t), 1, packageFile);
    fwrite(&fileEntryPtr->storeSize, sizeof(uint32_t), 1, packageFile);
  }
  return 0;
}

FvpPackage::~FvpPackage() {
  for (FileEntry *const &fileEntryPtr : fileEntrySet) {
    delete fileEntryPtr;
  }
}

void *FvpPackage::zAlloc(void *opaque, unsigned int count, unsigned int size) {
  return HeapAlloc(GetProcessHeap(), 0, count * size);
}

void FvpPackage::zFree(void *opaque, void *address) {
  HeapFree(GetProcessHeap(), 0, address);
}

int FvpFileSys::openPath(uint8_t **bufferPtr, size_t *sizePtr,
                         const char *path, bool readInfo) {

  const char *pathSep;
  char packageName[NAME_SIZE];
  FvpPackage::FileEntry *fileEntryPtr;
  size_t keySize;
  int result;
  FvpPackage *package;

  pathSep = strchr(path, '/');

  if (!pathSep) {
    return -6;
  }

  keySize = pathSep - path;
  memcpy(packageName, path, keySize);
  packageName[keySize] = 0;

  std::string key(packageName, keySize);
  PathMap::const_iterator iter = pathMap.find(key);
  if (iter == pathMap.end()) {
    FILE *packageFile;

    if (strcat_s(packageName, ".bin")) {
      return -1;
    }
    packageFile = fopen(packageName, "rb");
    if (!packageFile) {
      return -2;
    }
    packageFiles.insert(packageFile);
    std::unique_ptr<FvpPackage> packagePtr(new FvpPackage(packageFile));
    if (packagePtr->readHeader() < 0) {
      fclose(packageFile);
      return -3;
    }
    package = packagePtr.get();
    pathMap.insert(std::make_pair(key, std::move(packagePtr)));
  } else {
    package = iter->second.get();
  }
  pathSep++;
  fileEntryPtr = package->getFileByName(pathSep);
  if (!fileEntryPtr) {
    return -4;
  }
  if (readInfo) {
    result = package->readFileInfo(fileEntryPtr);
  } else {
    result = package->readFile(fileEntryPtr);
  }
  if (result < 0) {
    return -5;
  }
  *bufferPtr = fileEntryPtr->buffer;
  fileEntryPtr->buffer = nullptr;
  if (readInfo) {
    *sizePtr = result;
  } else {
    *sizePtr = fileEntryPtr->size;
  }
  return 0;
}

FvpFileSys::~FvpFileSys() {
  for (FILE *const &file : packageFiles) {
    fclose(file);
  }
}
