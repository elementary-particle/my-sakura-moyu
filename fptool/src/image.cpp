#include "fvp/image.h"
#include <cstring>

FvpImage::FvpImage() {
  type = UNKNOWN;
  width = height = 0;
  offsetX = offsetY = 0;
  partCount = 0;
  size = 0;
  buffer = nullptr;
}

int FvpImage::readPng(FILE *pngFile) {
  png_structp pngPtr;
  png_infop pngInfoPtr;
  int bitDepth, colorType;
  png_bytepp rows;

  pngPtr =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!pngPtr) {
    return -1;
  }
  pngInfoPtr = png_create_info_struct(pngPtr);
  if (!pngInfoPtr) {
    png_destroy_read_struct(&pngPtr, nullptr, nullptr);
    return -2;
  }
  png_init_io(pngPtr, pngFile);
  png_read_info(pngPtr, pngInfoPtr);
  png_get_IHDR(pngPtr, pngInfoPtr, &width, &height, &bitDepth, &colorType,
               nullptr, nullptr, nullptr);

  if (bitDepth == 8 && colorType == PNG_COLOR_TYPE_RGBA) {
    uint32_t i;

    type = RGBA8;
    size = HEADER_SIZE + height * width * 4;
    buffer = (new uint8_t[size]) + HEADER_SIZE;
    rows = new uint8_t *[height];
    for (i = 0; i < height; i++) {
      rows[i] = buffer + i * width * 4;
    }
  } else if (bitDepth == 8 && colorType == PNG_COLOR_TYPE_RGB) {
    uint32_t i;

    type = RGB8;
    size = HEADER_SIZE + height * width * 3;
    buffer = (new uint8_t[size]) + HEADER_SIZE;
    rows = new uint8_t *[height];
    for (i = 0; i < height; i++) {
      rows[i] = buffer + i * width * 3;
    }
  } else {
    png_destroy_read_struct(&pngPtr, &pngInfoPtr, nullptr);
    return -3;
  }

  if (png_get_valid(pngPtr, pngInfoPtr, PNG_INFO_oFFs)) {
    int unitType;
    png_get_oFFs(pngPtr, pngInfoPtr, &offsetX, &offsetY, &unitType);
    if (unitType != PNG_OFFSET_PIXEL) {
      offsetX = offsetY = 0;
    }
  }
  png_set_bgr(pngPtr);
  png_read_image(pngPtr, rows);
  png_read_end(pngPtr, pngInfoPtr);

  delete[] rows;
  png_destroy_read_struct(&pngPtr, &pngInfoPtr, nullptr);
  return 0;
}

int FvpImage::writePng(FILE *pngFile) const {
  png_structp pngPtr;
  png_infop pngInfoPtr;
  png_bytepp rows;

  if (!buffer) {
    return -1;
  }

  pngPtr =
      png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!pngPtr) {
    return -2;
  }
  pngInfoPtr = png_create_info_struct(pngPtr);
  if (!pngInfoPtr) {
    png_destroy_write_struct(&pngPtr, nullptr);
    return -3;
  }

  png_init_io(pngPtr, pngFile);
  png_set_alpha_mode(pngPtr, PNG_ALPHA_PNG, PNG_GAMMA_sRGB);
  if (type == RGBA8) {
    uint32_t i;

    png_set_IHDR(pngPtr, pngInfoPtr, width, height, 8, PNG_COLOR_TYPE_RGBA,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
                 PNG_FILTER_TYPE_BASE);

    rows = new uint8_t *[height];
    for (i = 0; i < height; i++) {
      rows[i] = buffer + i * width * 4;
    }
  } else if (type == RGB8) {
    uint32_t i;

    png_set_IHDR(pngPtr, pngInfoPtr, width, height, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
                 PNG_FILTER_TYPE_BASE);

    rows = new uint8_t *[height];
    for (i = 0; i < height; i++) {
      rows[i] = buffer + i * width * 3;
    }
  } else {
    png_destroy_write_struct(&pngPtr, &pngInfoPtr);
    return -4;
  }
  png_set_bgr(pngPtr);
  png_set_packing(pngPtr);
  png_write_info(pngPtr, pngInfoPtr);
  png_write_image(pngPtr, rows);
  png_write_end(pngPtr, pngInfoPtr);
  png_destroy_write_struct(&pngPtr, &pngInfoPtr);
  return 0;
}

int FvpImage::read(uint8_t *graph, size_t graphSize) {
  if (!checkSig(graph)) {
    return -1;
  }
  type = *((uint16_t *)(graph + 6));
  width = *((uint16_t *)(graph + 8));
  height = *((uint16_t *)(graph + 10));
  offsetX = *((uint16_t *)(graph + 12));
  offsetY = *((uint16_t *)(graph + 14));
  partCount = *((uint16_t *)(graph + 20));

  size = graphSize;
  buffer = graph + HEADER_SIZE;
  return 0;
}

int FvpImage::write(uint8_t **graphPtr, size_t *graphSizePtr) const {
  uint8_t *graph;

  if (!buffer) {
    return -1;
  }

  *graphSizePtr = size;
  graph = *graphPtr = buffer - HEADER_SIZE;

  memset(graph, 0, HEADER_SIZE);
  *((uint32_t *)graph) = SIGNATURE;
  *((uint16_t *)(graph + 4)) = 1; // version?
  *((uint16_t *)(graph + 6)) = type;
  *((uint16_t *)(graph + 8)) = width;
  *((uint16_t *)(graph + 10)) = height;
  *((uint16_t *)(graph + 12)) = offsetX;
  *((uint16_t *)(graph + 14)) = offsetY;
  *((uint16_t *)(graph + 20)) = partCount;

  return 0;
}

FvpImage::~FvpImage() = default;
