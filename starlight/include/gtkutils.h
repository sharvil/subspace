#pragma once

#include <gtk/gtk.h>

GtkWidget * gtk_widget_get_by_name(GtkWidget * start, const std::string & name);
GtkWidget * gtk_widget_find_by_name(GtkWidget * root, const std::string & name);
