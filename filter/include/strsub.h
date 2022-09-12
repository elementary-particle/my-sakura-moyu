#ifndef FILTER_STRSUB_H
#define FILTER_STRSUB_H

#include <windows.h>

#include <zlib.h>

#define BUCKET_SIZE 0x10000

class StringMap {
 public:
  struct ENTRY {
    DWORD dwIndex;
    DWORD dwOffset;
    WORD wTextSize;
    PCSTR pText;
    ENTRY *pNext;
  };
  static int (*pInflateInit)(z_streamp, const char *, int);
  static int (*pInflate)(z_streamp, int);
  static int (*pInflateEnd)(z_streamp);
  SYSTEMTIME Time;

 private:
  DWORD dwEntryCount;
  PSTR pPatchData;
  ENTRY *pEntryList;
  ENTRY *pBucket[BUCKET_SIZE];
  static inline SIZE_T Hash(DWORD dwOffset) {
    return (dwOffset ^ ((dwOffset >> 5) | (dwOffset << 27)) ^
            ((dwOffset >> 7) | (dwOffset << 25))) &
           (BUCKET_SIZE - 1);
  }
  inline void AddEntry(ENTRY *pEntry) {
    SIZE_T dwBucketIndex;
    dwBucketIndex = Hash(pEntry->dwOffset);
    pEntry->pNext = pBucket[dwBucketIndex];
    pBucket[dwBucketIndex] = pEntry;
  }
  static void *ZAlloc(void *opaque, unsigned int count, unsigned int size) {
    return HeapAlloc(GetProcessHeap(), 0, count * size);
  }
  static void ZFree(void *opaque, void *address) {
    HeapFree(GetProcessHeap(), 0, address);
  }

 public:
  BOOL Exist() { return pPatchData != NULL; }
  StringMap(PCWSTR pFileName) {
    HANDLE hFile;
    DWORD dwSize, dwIndex, dwRead;
    PSTR pFileBuf;
    PCSTR pRead;
    hFile = CreateFileW(pFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
      pPatchData = NULL;
      pEntryList = NULL;
      return;
    }
    FILETIME FileTime;
    GetFileTime(hFile, NULL, NULL, &FileTime);
    dwSize = GetFileSize(hFile, NULL);
    pFileBuf = (PSTR)VirtualAlloc(NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);
    if (!pFileBuf) {
      pPatchData = NULL;
      pEntryList = NULL;
      return;
    }
    ReadFile(hFile, pFileBuf, dwSize, &dwRead, NULL);
    CloseHandle(hFile);
    if (!*(DWORD *)pFileBuf) {
      // compressed
      DWORD dwCompSize;
      z_stream Stream;
      ULONG64 ul64Time;
      ul64Time = *(DWORD *)(pFileBuf + 4);
      ul64Time = (ul64Time + 11644473600) * 10000000;
      FileTime.dwLowDateTime = (DWORD)ul64Time;
      FileTime.dwHighDateTime = (DWORD)(ul64Time >> 32);
      dwSize = *(DWORD *)(pFileBuf + 8);
      dwCompSize = *(DWORD *)(pFileBuf + 12);
      pPatchData = (PSTR)VirtualAlloc(NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);
      if (!pPatchData) {
        VirtualFree(pFileBuf, 0, MEM_RELEASE);
        pPatchData = NULL;
        pEntryList = NULL;
        return;
      }
      Stream.zalloc = &ZAlloc;
      Stream.zfree = &ZFree;
      Stream.opaque = NULL;
      Stream.avail_in = dwCompSize;
      Stream.next_in = (Bytef *)pFileBuf + 16;
      Stream.avail_out = dwSize;
      Stream.next_out = (Bytef *)pPatchData;
      (*pInflateInit)(&Stream, "1.2.4", 56);
      if ((*pInflate)(&Stream, Z_FINISH) != Z_STREAM_END) {
        VirtualFree(pPatchData, 0, MEM_RELEASE);
        VirtualFree(pFileBuf, 0, MEM_RELEASE);
        pPatchData = NULL;
        pEntryList = NULL;
        return;
      }
      (*pInflateEnd)(&Stream);
      VirtualFree(pFileBuf, 0, MEM_RELEASE);
    } else {
      pPatchData = pFileBuf;
    }
    FileTimeToSystemTime(&FileTime, &Time);

    pRead = pPatchData;
    dwEntryCount = *(DWORD *)pRead;
    pRead += sizeof(DWORD);
    pEntryList = (ENTRY *)VirtualAlloc(NULL, dwEntryCount * sizeof(ENTRY),
                                       MEM_COMMIT, PAGE_READWRITE);
    if (!pEntryList) {
      VirtualFree(pPatchData, 0, MEM_RELEASE);
      VirtualFree(pEntryList, 0, MEM_RELEASE);
      pPatchData = NULL;
      pEntryList = NULL;
      return;
    }
    memset(pBucket, 0, BUCKET_SIZE * sizeof(ENTRY *));
    for (dwIndex = 0; dwIndex < dwEntryCount; dwIndex++) {
      ENTRY *pEntry;
      pEntry = pEntryList + dwIndex;
      pEntry->dwIndex = dwIndex + 1;
      pEntry->dwOffset = *(DWORD *)pRead;
      pRead += sizeof(DWORD);
      pEntry->wTextSize = *(WORD *)pRead;
      pRead += sizeof(WORD);
      if (pEntry->wTextSize) {
        pEntry->pText = pRead;
      } else {
        pEntry->pText = NULL;
      }
      pRead += pEntry->wTextSize;

      AddEntry(pEntry);
    }
  }

  ENTRY *GetEntry(DWORD dwOffset) {
    SIZE_T dwBucketIndex;
    ENTRY *pEntry;
    dwBucketIndex = Hash(dwOffset);
    for (pEntry = pBucket[dwBucketIndex]; pEntry; pEntry = pEntry->pNext) {
      if (pEntry->dwOffset == dwOffset) {
        break;
      }
    }
    return pEntry;
  }

  ~StringMap() {
    VirtualFree(pEntryList, 0, MEM_RELEASE);
    VirtualFree(pPatchData, 0, MEM_RELEASE);
  }
};

#endif  // FILTER_STRSUB_H