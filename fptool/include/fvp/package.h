#ifndef _FVP_PACKAGE_H_
#define _FVP_PACKAGE_H_

#include <cstdint>
#include <cstdio>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

#define NAME_SIZE 0x100

class FvpPackage {
public:
    typedef struct FileEntry {
        uint32_t nameOffset, offset, storeSize, size;
        char *name;
        size_t nameSize;
        uint8_t *buffer;

        FileEntry() {
            nameOffset = offset = storeSize = size = 0;
            name = nullptr;
            nameSize = 0;
            buffer = nullptr;
        }

        inline void freeBuffer() {
            delete[] buffer;
            buffer = nullptr;
        }

        ~FileEntry() {
            delete[] name;
            delete[] buffer;
        }
    } FileEntry;

    static const uint32_t Z_HEADER_SIZE = 0x20;
    static const uint32_t Z_SIGNATURE = 0x31637a68; // h z c 1
    class FileCompare {
    public:
        bool operator()(const FileEntry *, const FileEntry *) const;
    };
    static void *zAlloc(void *opaque, unsigned count, unsigned size);
    static void zFree(void *opaque, void *address);

    FILE *packageFile;
    uint32_t fileCount, nameHeaderSize;
    char *fileNameBuffer;
    std::set<FileEntry *, FileCompare> fileEntrySet;
    bool writing;

    FvpPackage() : FvpPackage(nullptr) {}

    explicit FvpPackage(FILE *file);

    FileEntry *getFileByName(const char *name);

    int readHeader();

    int readFileInfo(FileEntry *fileEntryPtr) const;

    int readFile(FileEntry *fileEntryPtr, bool doInflate=true) const;

    FileEntry *appendFile(const char *name);

    int writeHeader();

    int writeFile(FileEntry *, bool doDeflate) const;

    int writeEnd();

    ~FvpPackage();
};

class FvpFileSys {
public:
    using PathMap = std::unordered_map<std::string, std::unique_ptr<FvpPackage>>;
    PathMap pathMap;
    std::unordered_set<FILE *> packageFiles;

    int openPath(uint8_t **bufferPtr, size_t *sizePtr, const char *path, bool readInfo);

    ~FvpFileSys();
};

#endif //_FVP_PACKAGE_H_