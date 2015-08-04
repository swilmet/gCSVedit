/*
 * This file is part of gCSVedit.
 *
 * Copyright 2015 - Université Catholique de Louvain
 *
 * gCSVedit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * gCSVedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gCSVedit.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Sébastien Wilmet <sebastien.wilmet@uclouvain.be>
 */

#include "gcsv-window.h"
#include <gtksourceview/gtksource.h>

struct _GcsvWindow
{
	GtkApplicationWindow parent;

	GtkSourceView *view;
};

G_DEFINE_TYPE (GcsvWindow, gcsv_window, GTK_TYPE_APPLICATION_WINDOW)

static void
gcsv_window_dispose (GObject *object)
{

	G_OBJECT_CLASS (gcsv_window_parent_class)->dispose (object);
}

static void
gcsv_window_finalize (GObject *object)
{

	G_OBJECT_CLASS (gcsv_window_parent_class)->finalize (object);
}

static void
gcsv_window_class_init (GcsvWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gcsv_window_dispose;
	object_class->finalize = gcsv_window_finalize;
}

static void
gcsv_window_init (GcsvWindow *window)
{
	GtkWidget *scrolled_window;

	gtk_window_set_title (GTK_WINDOW (window), g_get_application_name ());
	gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);

	window->view = GTK_SOURCE_VIEW (gtk_source_view_new ());

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (window->view));

	gtk_container_add (GTK_CONTAINER (window), scrolled_window);
	gtk_widget_show_all (GTK_WIDGET (window));
}

GcsvWindow *
gcsv_window_new (void)
{
	return g_object_new (GCSV_TYPE_WINDOW, NULL);
}
