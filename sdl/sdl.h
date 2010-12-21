// Copyright (c) 2010 Bryan Cain
// Date: August 21, 2010
// Functions specific to the SDL backend.

#ifndef DEGA_SDL_H
#define DEGA_SDL_H

int StateLoad(char *StateName);
int StateSave(char *StateName);

int MainLoopIteration(void);
void MainLoop(void);

void OpenROM(char* filename);
void CloseROM();

void InitVideo();
void InitSound();
void CloseSound();
void JoystickScan();

void SDL_backend_init();
void SDL_backend_exit();

void PrintText(int x, int y, int color, char *buf);
void SetMessage(char* format, ...);

#endif

