// Copyright (c) 2010 Bryan Cain
// Date: August 21, 2010
// Functions specific to the SDL backend.

#ifndef DEGA_SDL_H
#define DEGA_SDL_H

int MainLoopIteration(void);
void MainLoop(void);
void OpenROM(const char* filename);
void CloseROM();
void InitSound();
void CloseSound();
void JoystickScan();
void SDL_backend_init();

#endif

