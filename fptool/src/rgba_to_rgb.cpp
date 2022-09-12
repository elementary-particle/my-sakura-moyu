#include <png.h>

int main(int argc, char *argv[]) {
  FILE *pngFile;
  png_structp pngPtr, pngOutPtr;
  png_infop pngInfoPtr, pngOutInfoPtr;
  png_bytep buffer;
  png_bytepp image;
  png_uint_32 width, height;
  png_int_32 offX, offY;
  int bitDepth, colorType, unitType;
  bool hasOffs;

  if (argc != 2) {
    return -1;
  }
  pngFile = fopen(argv[1], "rb");
  if (!pngFile) {
    return -2;
  }
  pngPtr =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!pngPtr) {
    return -3;
  }
  pngInfoPtr = png_create_info_struct(pngPtr);
  if (!pngInfoPtr) {
    png_destroy_read_struct(&pngPtr, nullptr, nullptr);
    return -4;
  }
  pngOutPtr =
      png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!pngOutPtr) {
    png_destroy_read_struct(&pngPtr, nullptr, nullptr);
    return -5;
  }
  pngOutInfoPtr = png_create_info_struct(pngOutPtr);
  if (!pngOutInfoPtr) {
    png_destroy_write_struct(&pngOutPtr, nullptr);
    png_destroy_read_struct(&pngPtr, nullptr, nullptr);
    return -6;
  }
  png_init_io(pngPtr, pngFile);
  png_read_info(pngPtr, pngInfoPtr);
  png_get_IHDR(pngPtr, pngInfoPtr, &width, &height, &bitDepth, &colorType,
               nullptr, nullptr, nullptr);
  if (bitDepth != 8 || colorType != PNG_COLOR_TYPE_RGBA) {
    png_read_end(pngPtr, pngInfoPtr);
    png_destroy_write_struct(&pngOutPtr, nullptr);
    png_destroy_read_struct(&pngPtr, &pngInfoPtr, nullptr);
    return -7;
  }
  hasOffs = false;
  if (png_get_valid(pngPtr, pngInfoPtr, PNG_INFO_oFFs)) {
    png_get_oFFs(pngPtr, pngInfoPtr, &offX, &offY, &unitType);
    if (unitType == PNG_OFFSET_PIXEL) {
      hasOffs = true;
    }
  }
  buffer = new png_byte[height * width * 3];
  image = new png_bytep[png_get_image_height(pngPtr, pngInfoPtr)];
  for (png_uint_32 i = 0; i < height; i++) {
    image[i] = buffer + i * width * 3;
  }
  png_set_strip_alpha(pngPtr);
  png_read_image(pngPtr, image);
  png_read_end(pngPtr, pngInfoPtr);
  fclose(pngFile);

  pngFile = fopen(argv[1], "wb");
  png_init_io(pngOutPtr, pngFile);
  png_set_alpha_mode(pngOutPtr, PNG_ALPHA_PNG, PNG_GAMMA_sRGB);
  png_set_IHDR(pngOutPtr, pngOutInfoPtr, width, height, 8, PNG_COLOR_TYPE_RGB,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
               PNG_FILTER_TYPE_BASE);
  if (hasOffs) {
    png_set_oFFs(pngOutPtr, pngOutInfoPtr, offX, offY, unitType);
  }
  png_write_info(pngOutPtr, pngOutInfoPtr);
  png_write_image(pngOutPtr, image);
  png_write_end(pngOutPtr, pngOutInfoPtr);
  fclose(pngFile);
  delete[] image;
  delete[] buffer;
  png_destroy_read_struct(&pngPtr, &pngInfoPtr, nullptr);
  png_destroy_write_struct(&pngOutPtr, &pngInfoPtr);

  return 0;
}
