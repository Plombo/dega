// Copyright (c) 2010 Bryan Cain
// Date: August 21, 2010
// GTK+ user interface stuff.

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include "gtksdl.h"
#include "callbacks.h"
#include "degaicon.xpm"

GtkBuilder* guiBuilder;
GtkWidget* mainWindow;
GtkWidget* loadRomFileChooser;
GtkWidget* loadStateFileChooser;
GtkWidget* saveStateFileChooser;
GtkWidget* recordMovieFileChooser;
GtkWidget* playMovieFileChooser;
GtkWidget* saveVgmFileChooser;
GtkWidget* saveWavFileChooser;

void createAccelerators()
{
	// Create the accel group for keyboard shortcuts.  For some reason, Glade 
	// doesn't allow specifying an accel group for non-stock menu items.
	GtkAccelGroup* accel_group = gtk_accel_group_new();
	
	// Set up the individual accelerators (keyboard shortcuts).
	gtk_widget_add_accelerator(
			GTK_WIDGET(gtk_builder_get_object(guiBuilder, "loadrom")),
			"activate", accel_group, GDK_O, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	// TODO accelerators for load state (Ctrl+L) and save state as (Ctrl+S)
	gtk_widget_add_accelerator(
			GTK_WIDGET(gtk_builder_get_object(guiBuilder, "quit")),
			"activate", accel_group, GDK_Q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(
			GTK_WIDGET(gtk_builder_get_object(guiBuilder, "pause")),
			"activate", accel_group, GDK_Pause, 0, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(
			GTK_WIDGET(gtk_builder_get_object(guiBuilder, "framestep")),
			"activate", accel_group, GDK_space, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(
			GTK_WIDGET(gtk_builder_get_object(guiBuilder, "softreset")),
			"activate", accel_group, GDK_Tab, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(
			GTK_WIDGET(gtk_builder_get_object(guiBuilder, "hardreset")),
			"activate", accel_group, GDK_Tab, 0, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(
			GTK_WIDGET(gtk_builder_get_object(guiBuilder, "logvgm")),
			"activate", accel_group, GDK_V, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_add_accelerator(
			GTK_WIDGET(gtk_builder_get_object(guiBuilder, "logwav")),
			"activate", accel_group, GDK_W, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	
	// Add the accel group to the main window.
	gtk_window_add_accel_group(GTK_WINDOW(mainWindow), accel_group);
}

int main (int argc, char **argv)
{
	gtk_set_locale();
	gtk_init(&argc, &argv);
	
	// Create the main window and various dialogs
	guiBuilder = gtk_builder_new();
	gtk_builder_add_from_file(guiBuilder, "sdl/interface.ui", NULL);
	gtk_builder_connect_signals(guiBuilder, NULL);
	mainWindow = GTK_WIDGET(gtk_builder_get_object(guiBuilder, "window1"));
	loadRomFileChooser = GTK_WIDGET(gtk_builder_get_object(guiBuilder, "filechooser-loadrom"));
	loadStateFileChooser = GTK_WIDGET(gtk_builder_get_object(guiBuilder, "filechooser-loadstate"));
	saveStateFileChooser = GTK_WIDGET(gtk_builder_get_object(guiBuilder, "filechooser-savestateas"));
	recordMovieFileChooser = GTK_WIDGET(gtk_builder_get_object(guiBuilder, "filechooser-recordmovie"));
	playMovieFileChooser = GTK_WIDGET(gtk_builder_get_object(guiBuilder, "filechooser-playmovie"));
	saveVgmFileChooser = GTK_WIDGET(gtk_builder_get_object(guiBuilder, "filechooser-logvgm"));
	saveWavFileChooser = GTK_WIDGET(gtk_builder_get_object(guiBuilder, "filechooser-logwav"));
	
	GdkPixbuf* icon = gdk_pixbuf_new_from_xpm_data(degaicon_xpm);
	gtk_window_set_icon(GTK_WINDOW(mainWindow), icon);
	
	// Initialize the framework connecting SDL and GTK+.
	create_sdlSocket();
	
	// Initialize the SDL backend.
	SDL_backend_init();
	
	// Set up the keyboard shortcuts for menu items.
	createAccelerators();
	
	// Display the window.
	gtk_widget_show_all(mainWindow);
	
	// Run the emulator's main event loop.
	MainLoop();
	
	// Exit the program after the main event loop is over
	appExit(0);
	return 0;
}

