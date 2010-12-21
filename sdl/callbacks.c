// Copyright (c) 2010 Bryan Cain
// Date: August 21, 2010
// GTK+ event callbacks.

#include <dirent.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include "mast.h"
#include "logwav.h"
#include "sdl.h"
#include "gtk.h"
#include "gtksdl.h"
#include "callbacks.h"

#define PauseAudio(X) if(SDL_WasInit(SDL_INIT_AUDIO)) SDL_PauseAudio(X);

unsigned short vgm_gd3_default[] = {0};
int mainWindowHasFocus = 1;
int paused = 0;

void close_app(GtkWidget* widget, gpointer user_data)
{
	appExit(0);
}

void on_mainwindow_gain_focus(GtkWidget* widget, gpointer user_data)
{
	mainWindowHasFocus = 1;
}

void on_mainwindow_lose_focus(GtkWidget* widget, gpointer user_data)
{
	mainWindowHasFocus = 0;
	PauseAudio(1);
	while(!mainWindowHasFocus) // stop gameplay by looping until the window regains focus.
	{
		// Keep running GTK+ so that it can report when the window regains 
		// focus.
		while(gtk_events_pending())
			gtk_main_iteration_do(FALSE);
	}
	// Window is focused again; resume gameplay.
	PauseAudio(0);
}

void on_loadrom_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	gtk_widget_show(loadRomFileChooser);
}

void on_ok_loadrom_clicked(GtkButton *button, gpointer user_data)
{
	char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(loadRomFileChooser));
	DIR* dir = opendir(filename);
	
	if(dir != NULL) // selected file is a directory; change the directory
	{
		closedir(dir);
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(loadRomFileChooser), filename);
	}
	else // normal file selected; open the file and start the emulator
	{
		gtk_widget_hide(loadRomFileChooser);

		/*
		 * We need to flush any gtk events waiting to happen (like the widget hide
	     * from above) to avoid a deadlock when starting a game fullscreen (at least
	     * in linux).
	     */
		while (gtk_events_pending())
		  gtk_main_iteration();
		
		OpenROM(filename);
	}
}

void on_cancel_loadrom_clicked(GtkButton* button, gpointer user_data)
{
	gtk_widget_hide(loadRomFileChooser);
}

void on_loadstate_activate(GtkButton* button, gpointer user_data)
{
	gtk_widget_show(loadStateFileChooser);
}

void on_ok_loadstate_clicked(GtkButton *button, gpointer user_data)
{
	char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(loadStateFileChooser));
	DIR* dir = opendir(filename);
	
	if(dir != NULL) // selected file is a directory; change the directory
	{
		closedir(dir);
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(loadStateFileChooser), filename);
	}
	else // normal file selected; open the file and start the emulator
	{
		gtk_widget_hide(loadStateFileChooser);
		int success = StateLoad(filename);
		if(success != 0) ; // TODO notify user of success or failure
	}
}

void on_cancel_loadstate_clicked(GtkButton* button, gpointer user_data)
{
	gtk_widget_hide(loadStateFileChooser);
}

void on_savestateas_activate(GtkButton* button, gpointer user_data)
{
	gtk_widget_show(saveStateFileChooser);
}

void on_ok_savestateas_clicked(GtkButton* button, gpointer user_data)
{
	char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(saveStateFileChooser));
	gtk_widget_hide(saveStateFileChooser);
	if(filename)
	{
		int success = StateSave(filename);
		// TODO notify user of success or failure
	}
}

void on_cancel_savestateas_clicked(GtkButton* button, gpointer user_data)
{
	gtk_widget_hide(saveStateFileChooser);
}

void on_closerom_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	CloseROM();
}

void on_pause_activate(GtkMenuItem* menuitem, gpointer user_data)
{
	paused = !paused;
	
	if(paused)
	{
		PauseAudio(1);
		while(paused) // stop gameplay by looping until unpaused
		{
			// Keep running GTK+ so that the application doesn't lock up.
			while(gtk_events_pending())
				gtk_main_iteration_do(FALSE);
		}
		// Unpaused; resume gameplay.
		PauseAudio(0);
	}
}

void on_framestep_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	static int framestepping = 0;
	if(!framestepping)
	{
		framestepping = 1;
		MainLoopIteration();
		framestepping = 0;
	}
}

void on_recordmovie_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	if(MvidInVideo())
	{
		MvidStop();
		gtk_menu_item_set_label(menuitem, "Record Movie...");
	}
	else gtk_widget_show(recordMovieFileChooser);
}

void on_ok_recordmovie_clicked(GtkButton* button, gpointer user_data)
{
	char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(recordMovieFileChooser));
	gtk_widget_hide(recordMovieFileChooser);
	if(filename)
	{
		gtk_menu_item_set_label(GTK_MENU_ITEM(gtk_builder_get_object(guiBuilder, "recordmovie")), "Stop Recording");
		MvidStart(filename, RECORD_MODE, 0, 0);
		// TODO notify user at start of recording
	}
}

void on_cancel_recordmovie_clicked(GtkButton* button, gpointer user_data)
{
	gtk_widget_hide(recordMovieFileChooser);
}

void on_playmovie_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	if(MvidInVideo())
	{
		MvidStop();
		gtk_menu_item_set_label(menuitem, "Play Movie...");
	}
	else gtk_widget_show(playMovieFileChooser);
}

// TODO fix movie playback
void on_ok_playmovie_clicked(GtkButton* button, gpointer user_data)
{
	char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(playMovieFileChooser));
	gtk_widget_hide(playMovieFileChooser);
	if(filename)
	{
		gtk_menu_item_set_label(GTK_MENU_ITEM(gtk_builder_get_object(guiBuilder, "playmovie")), "Stop Playing");
		MvidStart(filename, PLAYBACK_MODE, 0, 0);
		// TODO notify user at start of playback
	}
}

void on_cancel_playmovie_clicked(GtkButton* button, gpointer user_data)
{
	gtk_widget_hide(playMovieFileChooser);
}

void on_softreset_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	if(VgmFile) VgmStop(vgm_gd3_default);
	MastReset();
}

void on_hardreset_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	if(VgmFile) VgmStop(vgm_gd3_default);
	MastHardReset();
}

void on_logvgm_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	if(VgmFile)
	{
		VgmStop(vgm_gd3_default);
		gtk_menu_item_set_label(menuitem, "Log to VGM...");
	}
	else gtk_widget_show(saveVgmFileChooser);
}

void on_ok_logvgm_clicked(GtkButton* button, gpointer user_data)
{
	char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(saveVgmFileChooser));
	gtk_widget_hide(saveVgmFileChooser);
	if(filename)
	{
		VgmStart(filename);
		gtk_menu_item_set_label(GTK_MENU_ITEM(gtk_builder_get_object(guiBuilder, "logvgm")), "Stop VGM Logging");
	}
}

void on_cancel_logvgm_clicked(GtkButton* button, gpointer user_data)
{
	gtk_widget_hide(saveVgmFileChooser);
}

void on_logwav_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	if(logwav)
	{
		WavLogEnd();
		gtk_menu_item_set_label(menuitem, "Log to WAV...");
	}
	else gtk_widget_show(saveWavFileChooser);
}

void on_ok_logwav_clicked(GtkButton* button, gpointer user_data)
{
	char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(saveWavFileChooser));
	gtk_widget_hide(saveWavFileChooser);
	if(filename)
	{
		WavLogStart(filename, 16, MsndRate, 2);
		gtk_menu_item_set_label(GTK_MENU_ITEM(gtk_builder_get_object(guiBuilder, "logwav")), "Stop WAV Logging");
	}
}

void on_cancel_logwav_clicked(GtkButton* button, gpointer user_data)
{
	gtk_widget_hide(saveWavFileChooser);
}

// Function adapted from Gens/GS (source/gens/input/input_sdl.c)
gint gdk_sdl_keysnoop(GtkWidget *grab, GdkEventKey *event, gpointer user_data)
{
	SDL_Event sdlev;
	SDLKey sdlkey;
	int keystate;


	// Only grab keys from the main window.
	if (grab != mainWindow)
	{
		// Don't push this key onto the SDL event stack.
		return FALSE;
	}
	
	switch (event->type)
	{
		case GDK_KEY_PRESS:
			sdlev.type = SDL_KEYDOWN;
			sdlev.key.state = SDL_PRESSED;
			keystate = 1;
			break;
		
		case GDK_KEY_RELEASE:
			sdlev.type = SDL_KEYUP;
			sdlev.key.state = SDL_RELEASED;
			keystate = 0;
			break;
		
		default:
			fprintf(stderr, "Unhandled GDK event type: %d", event->type);
			return FALSE;
	}
	
	// Convert this keypress from GDK to SDL.
	sdlkey = (SDLKey)gdk_to_sdl_keyval(event->keyval);
	
	// Create an SDL event from the keypress.
	sdlev.key.keysym.sym = sdlkey;
	if (sdlkey != 0)
		SDL_PushEvent(&sdlev);
	
	// Allow GTK+ to process this key.
	return FALSE;
}

