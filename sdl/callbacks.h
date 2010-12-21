// Copyright (c) 2010 Bryan Cain
// Date: August 21, 2010
// GTK+ event callbacks.

#include <gtk/gtk.h>

void close_app(GtkWidget* widget, gpointer user_data);
void on_open1_activate(GtkMenuItem *menuitem, gpointer user_data);
gint gdk_sdl_keysnoop(GtkWidget *grab, GdkEventKey *event, gpointer user_data);


