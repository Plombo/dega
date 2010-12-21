// Copyright (c) 2010 Bryan Cain
// Date: August 21, 2010
// GTK+ user interface stuff.

#ifndef DEGA_GTK_H
#define DEGA_GTK_H

#include <gtk/gtk.h>

extern GtkBuilder* guiBuilder;
extern GtkWidget* mainWindow;
extern GtkWidget* loadRomFileChooser;
extern GtkWidget* loadStateFileChooser;
extern GtkWidget* saveStateFileChooser;
extern GtkWidget* recordMovieFileChooser;
extern GtkWidget* playMovieFileChooser;
extern GtkWidget* saveVgmFileChooser;
extern GtkWidget* saveWavFileChooser;

#endif

