#ifndef FILTER_SUBTITLE_H
#define FILTER_SUBTITLE_H

#include "ass/ass.h"

void DebugLog(int Level, const char *pcFormat, ...);

void SubtitleInit();
void SubtitleFini();

class Subtitle {
 private:
  ASS_Track *pTrack;
  ASS_Renderer *pRenderer;

 public:
  static ASS_Image *CopyImage(ASS_Image *pImage);
  static void DeleteImage(ASS_Image *pImage);
  void SetFrameSize(int FrameW, int FrameH);
  Subtitle(char *pAssPath);
  bool Ready() { return pTrack && pRenderer; }
  void Prepare(int FrameW, int FrameH);
  ASS_Image *Render(long long llTick);
  ~Subtitle();
};

#endif  // FILTER_SUBTITLE_H
