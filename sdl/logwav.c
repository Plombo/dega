// Copyright (c) 2004-2010 OpenBOR Team
// Copyright (c) 2010 Bryan Cain
// Date: August 21, 2010
// Redistribution and modification for non-commercial use is permitted.
// Code adapted from the OpenBOR game engine (www.LavaLit.com)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

// assume little endian architecture, change these definitions for big endian
#define		SwapLSB16(X)	(X)
#define		SwapLSB32(X)	(X)

#define		HEX_RIFF	    0x46464952
#define		HEX_WAVE	    0x45564157
#define		HEX_fmt		    0x20746D66
#define		HEX_data	    0x61746164
#define		FMT_PCM		    0x0001

#define error() { if(fd){fclose(fd);fd=NULL;} logwav=0; printf("WAV logging error: %s\n",strerror(errno)); return 1; }

FILE* fd = NULL;
uint32_t totalsize;
int logwav = 0; // are we logging sound output to WAV currently?

/*
 * Opens a WAV file to log sound output to.
 * 
 * filename - the filename of the file to save the PCM output to.
 * samplebits - bits per sample (8 or 16)
 * frequency - frequency in Hz (usually 11025, 22050, or 44100)
 * channels - 1 for mono, 2 for stereo
 * 
 * Returns: 0 on success, 1 on error
 */
int WavLogStart(char* filename, int samplebits, int frequency, int channels)
{
	struct {
		uint32_t	riff;
		uint32_t	size;
		uint32_t	type;
	} riffheader;
	struct {
		uint32_t	tag;
		uint32_t	size;
	} rifftag;
	struct {
		uint16_t	format;		// 1 = PCM
		uint16_t	channels;	// Mono, stereo
		uint32_t	samplerate;	// 11025, 22050, 44100
		uint32_t	bps;		// Bytes/second
		uint16_t	blockalign;
		uint16_t	samplebits;	// 8, 16
	} fmt;

	fd = fopen(filename, "wb");
	if(fd == NULL) error();

	// RIFF header
	riffheader.riff = SwapLSB32(HEX_RIFF);
	riffheader.size = SwapLSB32(0);           // this is overwritten by WavLogEnd()
	riffheader.type = SwapLSB32(HEX_WAVE);
	if(fwrite(&riffheader, sizeof(riffheader), 1, fd) != 1) error();
	
	// fmt chunk
	rifftag.tag = SwapLSB32(HEX_fmt);
	rifftag.size = SwapLSB32(16);
	fmt.format = SwapLSB16(FMT_PCM);
	fmt.channels = SwapLSB16(channels);
	fmt.samplerate = SwapLSB32(frequency);
	fmt.bps = SwapLSB32(frequency*channels*2); // bps = frequency * blockalign
	fmt.blockalign = SwapLSB16(channels*2); // blockalign = channels * samplebits / 8
	fmt.samplebits = SwapLSB16(samplebits);
	if(fwrite(&rifftag, sizeof(rifftag), 1, fd) != 1) error();
	if(fwrite(&fmt, sizeof(fmt), 1, fd) != 1) error();

	// data chunk - tag
	rifftag.tag = SwapLSB32(HEX_data);
	rifftag.size = SwapLSB32(0);            // this is overwritten by WavLogEnd()
	if(fwrite(&rifftag, sizeof(rifftag), 1, fd) != 1) error();
	
	totalsize = 0;
	logwav = 1;
	
	return 0;
}

/*
 * Writes PCM data to an opened log file.
 * 
 * sampleptr - pointer to audio data
 * soundlen - length of audio data in bytes
 */
int WavLogAppend(void* sampleptr, int soundlen)
{
	if(!fd || !logwav) return 1;
	
	if(fwrite(sampleptr, soundlen, 1, fd) != 1) error();
	totalsize += soundlen;
	
	return 0;
}

/*
 * Finishes logging to WAV.
 * 
 * Returns: 0 on success, 1 on error
 */
int WavLogEnd()
{
	uint32_t wav_totalsize = SwapLSB32(totalsize+44); // size of WAV including 44-byte header
	uint32_t stream_totalsize = SwapLSB32(totalsize);
	
	if(!fd || !logwav) return 1;
	
	fseek(fd, 4, SEEK_SET);
	if(fwrite(&wav_totalsize, 4, 1, fd) != 1) error();
	fseek(fd, 40, SEEK_SET);
	if(fwrite(&stream_totalsize, 4, 1, fd) != 1) error();
	
	fclose(fd);
	fd = NULL;
	logwav = 0;
	return 0;
}



