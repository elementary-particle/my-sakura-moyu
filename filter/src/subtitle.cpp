#include "subtitle.h"

#include <cstdio>
#include <cstring>

#include <windows.h>

#include "common.h"

ASS_Library *pAssLibrary;

#define LOG_SIZE 0x400

void DebugLogV(int Level, const char *pcFormat, va_list vaList, void *data) {
  char pLog[LOG_SIZE];
  HANDLE hOutput;

  if (Level >= 5) {
    return;
  }

  hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
  vsnprintf(pLog, LOG_SIZE, pcFormat, vaList);

  WriteConsoleA(hOutput, pLog, strlen(pLog), NULL, NULL);
  if (data) {
    WriteConsoleA(hOutput, data, strlen((PCHAR)data), NULL, NULL);
  }
}

void DebugLog(int Level, const char *pcFormat, ...) {
  va_list vaList;
  va_start(vaList, pcFormat);
  DebugLogV(Level, pcFormat, vaList, NULL);
  va_end(vaList);
}

void SubtitleInit() {
  pAssLibrary = ass_library_init();
  // ass_set_message_cb(pAssLibrary, &DebugLogV, (PVOID)"\n");
}

void SubtitleFini() { ass_library_done(pAssLibrary); }

ASS_Image *Subtitle::CopyImage(ASS_Image *pImage) {
  if (pImage) {
    ASS_Image *pNewImage;
    int i;
    pNewImage = new ASS_Image;
    pNewImage->w = pImage->w;
    pNewImage->h = pImage->h;
    pNewImage->stride = pImage->stride;
    pNewImage->bitmap = new unsigned char[pImage->h * pImage->stride];
    for (i = 0; i < pImage->h; i++) {
      memcpy(pNewImage->bitmap + i * pImage->stride,
             pImage->bitmap + i * pImage->stride, pImage->w);
    }
    pNewImage->color = pImage->color;
    pNewImage->dst_x = pImage->dst_x;
    pNewImage->dst_y = pImage->dst_y;
    pNewImage->next = CopyImage(pImage->next);
    pNewImage->type = pImage->type;
    return pNewImage;
  } else {
    return NULL;
  }
}

void Subtitle::DeleteImage(ASS_Image *pImage) {
  if (pImage) {
    DeleteImage(pImage->next);
    delete pImage->bitmap;
    delete pImage;
  }
}

void Subtitle::SetFrameSize(int FrameW, int FrameH) {
  if (pRenderer) {
    ass_set_storage_size(pRenderer, FrameW, FrameH);
    ass_set_frame_size(pRenderer, FrameW, FrameH);
  }
}

Subtitle::Subtitle(char *pAssPath) {
  pTrack = ass_read_file(pAssLibrary, pAssPath, NULL);
  if (!pTrack) {
    return;
  }
  pRenderer = ass_renderer_init(pAssLibrary);
  if (!pRenderer) {
    return;
  }
}

void Subtitle::Prepare(int FrameW, int FrameH) {
  SetFrameSize(FrameW, FrameH);
  ass_set_fonts_dir(pAssLibrary, PATH_FONT);
  ass_set_fonts(pRenderer, NULL, "sans-serif", ASS_FONTPROVIDER_DIRECTWRITE,
                NULL, 1);
}

ASS_Image *Subtitle::Render(long long llTick) {
  int Change;
  ASS_Image *pImage;

  pImage = ass_render_frame(pRenderer, pTrack, llTick, &Change);
  pImage = CopyImage(pImage);
  return pImage;
}

Subtitle::~Subtitle() {
  ass_renderer_done(pRenderer);
  ass_free_track(pTrack);
}
