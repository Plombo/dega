// Copyright (c) 2010 Bryan Cain
// Date: August 21, 2010
// Redistribution and modification for non-commercial use is permitted.
// Bridge between SDL and GTK.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include "SDL.h"
#include "gtksdl.h"
#include "gtk.h"
#include "sdl.h"
#include "callbacks.h"

GtkWidget* sdlSocket;

void appExit(int retcode)
{
	SDL_Quit();
	// gtk_main_quit();
	exit(retcode);
}

void create_sdlSocket (void)
{
	sdlSocket = GTK_WIDGET(gtk_builder_get_object(guiBuilder, "sdlsocket"));
	
	// Set the SDL socket's background color to black.
	GdkColor background = {0, 0, 0, 0};
	gtk_widget_modify_bg(sdlSocket, GTK_STATE_NORMAL, &background);
	
	// Resize the SDL socket based on the video scale factor.
	int width, height;
	gtk_widget_get_size_request(sdlSocket, &width, &height);
	gtk_widget_set_size_request(sdlSocket, width, height);
	gtk_widget_realize(sdlSocket);
	gtk_widget_show(sdlSocket);
	
	// Enable drag & drop for ROM loading.
	static const GtkTargetEntry target_list[] =
	{
		{"text/plain", 0, 0},
		{"text/uri-list", 0, 1},
	};
	
	// Set up drag-and-drop ROM loading.
	gtk_drag_dest_set
	(
		sdlSocket,
		(GtkDestDefaults)(GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT | GTK_DEST_DEFAULT_DROP),
		target_list,
		G_N_ELEMENTS(target_list),
		(GdkDragAction)(GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK | GDK_ACTION_PRIVATE | GDK_ACTION_ASK)
	);
	
	// Initialize the "key snooper" that creates SDL keyboard events.
	gtk_key_snooper_install(gdk_sdl_keysnoop, NULL);
}

// Drop-in replacement for SDL_SetVideoMode that embeds the SDL window in 
// the GTK+ window and adjusts the GTK+ window size in addition to calling 
// SDL_SetVideoMode.
SDL_Surface* SetVideoMode(int width, int height, int bitsperpixel, int flags)
{
	SDL_Surface* video_surface;
	
	// X11. If windowed, embed the SDL window inside of the GTK+ window.
	if (flags & SDL_FULLSCREEN)
	{
		// Hide the embedded SDL window.
		gtk_widget_hide(sdlSocket);
		SDL_putenv("SDL_WINDOWID=");
		
		// Call SDL_SetVideoMode.
		video_surface = SDL_SetVideoMode(width, height, bitsperpixel, flags);
	}
	else
	{
		if (SDL_WasInit(SDL_INIT_VIDEO))
			SDL_QuitSubSystem(SDL_INIT_VIDEO);
		
		// Show the embedded SDL window.
		gtk_widget_set_size_request(sdlSocket, width, height);
		gtk_widget_realize(sdlSocket);
		gtk_widget_show(sdlSocket);
		
		// Wait for GTK+ to catch up.
		while (gtk_events_pending())
			gtk_main_iteration_do(FALSE);
		
		// Get the window ID of the SDL "socket".
		char SDL_WindowID[128];
		sprintf(SDL_WindowID, "SDL_WINDOWID=%u", (unsigned int)(GDK_WINDOW_XWINDOW(gtk_widget_get_window(sdlSocket))));
		
		SDL_putenv(SDL_WindowID);
		
		// Initialize the SDL video subsystem.void SDL_main(int argc, char **argv);
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
   		{
   			printf("SDL_InitSubSystem(VIDEO) failed at %s:%d - %s\n", __FILE__, __LINE__, SDL_GetError());
   			appExit(1);
		}
		
		// Hide the cursor on the SDL surface area.
		SDL_ShowCursor(SDL_DISABLE);
		
		// Call SDL_SetVideoMode.
		video_surface = SDL_SetVideoMode(width, height, bitsperpixel, flags);
		
		// Resize the main window.
		GtkRequisition req;
		gtk_widget_size_request(GTK_WIDGET(mainWindow), &req);
		gtk_window_resize(GTK_WINDOW(mainWindow), req.width, req.height);
	}
		
	return video_surface;
}

// Adapted from Hu-Go! Plus, which adapted it from Gens/GS.  Converts a GDK key 
// value into an SDL key value.
unsigned short gdk_to_sdl_keyval(int gdk_key)
{
	if (!(gdk_key & 0xFF00))
	{
		// ASCII symbol.
		// SDL and GDK use the same values for these keys.
		
		// Make sure the key value is lowercase.
		gdk_key = tolower(gdk_key);
		
		// Return the key value.
		return gdk_key;
	}
	
	if (gdk_key & 0xFFFF0000)
	{
		// Extended X11 key. Not supported by SDL.
#ifdef GDK_WINDOWING_X11
		fprintf(stderr, "Unhandled extended X11 key: 0x%08X (%s)", gdk_key, XKeysymToString(gdk_key));
#else
		fprintf(stderr, "Unhandled extended key: 0x%08X\n", gdk_key);
#endif
		return 0;
	}
	
	// Non-ASCII symbol.
	static const uint16_t gdk_to_sdl_table[0x100] =
	{
		// 0x00 - 0x0F
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		SDLK_BACKSPACE, SDLK_TAB, SDLK_RETURN, SDLK_CLEAR,
		0x0000, SDLK_RETURN, 0x0000, 0x0000,
		
		// 0x10 - 0x1F
		0x0000, 0x0000, 0x0000, SDLK_PAUSE,
		SDLK_SCROLLOCK, SDLK_SYSREQ, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, SDLK_ESCAPE,
		0x0000, 0x0000, 0x0000, 0x0000,
		
		// 0x20 - 0x2F
		SDLK_COMPOSE, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		
		// 0x30 - 0x3F [Japanese keys]
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		
		// 0x40 - 0x4F [unused]
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		
		// 0x50 - 0x5F
		SDLK_HOME, SDLK_LEFT, SDLK_UP, SDLK_RIGHT,
		SDLK_DOWN, SDLK_PAGEUP, SDLK_PAGEDOWN, SDLK_END,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		
		// 0x60 - 0x6F
		0x0000, SDLK_PRINT, 0x0000, SDLK_INSERT,
		SDLK_UNDO, 0x0000, 0x0000, SDLK_MENU,
		0x0000, SDLK_HELP, SDLK_BREAK, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		
		// 0x70 - 0x7F [mostly unused, except for Alt Gr and Num Lock]
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, SDLK_MODE, SDLK_NUMLOCK,
		
		// 0x80 - 0x8F [mostly unused, except for some numeric keypad keys]
		SDLK_KP5, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, SDLK_KP_ENTER, 0x0000, 0x0000,
		
		// 0x90 - 0x9F
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, SDLK_KP7, SDLK_KP4, SDLK_KP8,
		SDLK_KP6, SDLK_KP2, SDLK_KP9, SDLK_KP3,
		SDLK_KP1, SDLK_KP5, SDLK_KP0, SDLK_KP_PERIOD,
		
		// 0xA0 - 0xAF
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, SDLK_KP_MULTIPLY, SDLK_KP_PLUS,
		0x0000, SDLK_KP_MINUS, SDLK_KP_PERIOD, SDLK_KP_DIVIDE,
		
		// 0xB0 - 0xBF
		SDLK_KP0, SDLK_KP1, SDLK_KP2, SDLK_KP3,
		SDLK_KP4, SDLK_KP5, SDLK_KP6, SDLK_KP7,
		SDLK_KP8, SDLK_KP9, 0x0000, 0x0000,
		0x0000, SDLK_KP_EQUALS, SDLK_F1, SDLK_F2,
		
		// 0xC0 - 0xCF
		SDLK_F3, SDLK_F4, SDLK_F5, SDLK_F6,
		SDLK_F7, SDLK_F8, SDLK_F9, SDLK_F10,
		SDLK_F11, SDLK_F12, SDLK_F13, SDLK_F14,
		SDLK_F15, 0x0000, 0x0000, 0x0000,
		
		// 0xD0 - 0xDF [L* and R* function keys]
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		
		// 0xE0 - 0xEF
		0x0000, SDLK_LSHIFT, SDLK_RSHIFT, SDLK_LCTRL,
		SDLK_RCTRL, SDLK_CAPSLOCK, 0x0000, SDLK_LMETA,
		SDLK_RMETA, SDLK_LALT, SDLK_RALT, SDLK_LSUPER,
		SDLK_RSUPER, 0x0000, 0x0000, 0x0000,
		
		// 0xF0 - 0xFF [mostly unused, except for Delete]
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000,
		0x0000, 0x0000, 0x0000, SDLK_DELETE,		
	};
	
	unsigned short sdl_key = gdk_to_sdl_table[gdk_key & 0xFF];
	if (sdl_key == 0)
	{
		// Unhandled GDK key.
		fprintf(stderr, "Unhandled GDK key: 0x%04X (%s)", gdk_key, gdk_keyval_name(gdk_key));
		return 0;
	}
	
	return sdl_key;
}

