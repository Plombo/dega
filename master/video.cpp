#include "app.h"

char VideoName[256] = "";

void VideoRecord(int reset)
{
  MvidStart(VideoName, RECORD_MODE, reset);
}

void VideoPlayback()
{
  MvidStart(VideoName, PLAYBACK_MODE, 0);
}
