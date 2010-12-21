// Copyright (c) 2010 Bryan Cain
// Date: August 21, 2010
// Redistribution and modification for non-commercial use is permitted.
// Bridge between SDL and GTK.

#ifndef GTKSDL_H
#define GTKSDL_H

#include <gtk/gtk.h>
#include "SDL.h"

extern GtkWidget* sdlSocket;

void appExit(int retcode);
void create_sdlSocket(void);
unsigned short gdk_to_sdl_keyval(int gdk_key);

SDL_Surface* SetVideoMode(int width, int height, int bitsperpixel, int flags);

#endif

