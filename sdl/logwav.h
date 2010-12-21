// Copyright (c) 2010 Bryan Cain
// Date: August 21, 2010
// Redistribution and modification for non-commercial use is permitted.
// Code adapted from the OpenBOR game engine (www.LavaLit.com)

#ifndef LOGWAV_H
#define LOGWAV_H

extern int logwav;

int WavLogStart(char *filename, int samplebits, int frequency, int channels);
int WavLogAppend(void *sampleptr, int soundlen);
int WavLogEnd();

#endif

