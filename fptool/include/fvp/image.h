#ifndef _FVP_IMAGE_H_
#define _FVP_IMAGE_H_

#include <cstdint>
#include "png.h"

class FvpImage {
public:
    enum Type {
        RGB8 = 0,
        RGBA8 = 1,
        RGBA8_MULTI = 2,
        GRAY8 = 3,
        GRAY1 = 4,
        UNKNOWN = 5,
    };
    static const uint32_t SIGNATURE = 0x4753564e; // N V S G
    static const uint32_t HEADER_SIZE = 0x20;
    static bool checkSig(const uint8_t *graph) {
        return *((uint32_t *)graph) == SIGNATURE;
    }
    uint16_t type;
    uint32_t width, height;
    int32_t offsetX, offsetY;
    uint16_t partCount;
    uint8_t *buffer;
    size_t size;

    FvpImage();

    int readPng(FILE *pngFile);

    int writePng(FILE *pngFile) const;

    int read(uint8_t *graph, size_t graphSize);

    int write(uint8_t **graphPtr, size_t *graphSizePtr) const;

    ~FvpImage();
};

#endif //_FVP_IMAGE_H_
