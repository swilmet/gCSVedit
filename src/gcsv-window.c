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
	GtkSourceFile *file;
};

G_DEFINE_TYPE (GcsvWindow, gcsv_window, GTK_TYPE_APPLICATION_WINDOW)

static void
gcsv_window_dispose (GObject *object)
{
	GcsvWindow *window = GCSV_WINDOW (object);

	g_clear_object (&window->file);

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

static GtkSourceView *
create_view (void)
{
	GtkSourceView *view;
	GtkSourceBuffer *buffer;
	GtkSourceLanguageManager *language_manager;
	GtkSourceLanguage *csv_lang;

	view = GTK_SOURCE_VIEW (gtk_source_view_new ());
	buffer = GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));

	language_manager = gtk_source_language_manager_get_default ();
	csv_lang = gtk_source_language_manager_get_language (language_manager, "csv");
	gtk_source_buffer_set_language (buffer, csv_lang);

	return view;
}

static void
gcsv_window_init (GcsvWindow *window)
{
	GtkWidget *scrolled_window;

	gtk_window_set_title (GTK_WINDOW (window), g_get_application_name ());
	gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);

	window->view = create_view ();

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (window->view));

	gtk_container_add (GTK_CONTAINER (window), scrolled_window);
	gtk_widget_show_all (GTK_WIDGET (window));

	window->file = gtk_source_file_new ();
}

GcsvWindow *
gcsv_window_new (void)
{
	return g_object_new (GCSV_TYPE_WINDOW, NULL);
}

static void
load_cb (GtkSourceFileLoader *loader,
	 GAsyncResult        *result,
	 GcsvWindow          *window)
{
	GError *error = NULL;

	gtk_source_file_loader_load_finish (loader, result, &error);

	if (error != NULL)
	{
		g_warning ("Error when loading file: %s", error->message);
		g_error_free (error);
		error = NULL;
	}

	g_object_unref (loader);
	g_object_unref (window);
}

void
gcsv_window_load_file (GcsvWindow *window,
		       GFile      *location)
{
	GtkTextBuffer *buffer;
	GtkSourceFileLoader *loader;

	g_return_if_fail (GCSV_IS_WINDOW (window));
	g_return_if_fail (G_IS_FILE (location));

	gtk_source_file_set_location (window->file, location);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (window->view));

	loader = gtk_source_file_loader_new (GTK_SOURCE_BUFFER (buffer),
					     window->file);

	gtk_source_file_loader_load_async (loader,
					   G_PRIORITY_DEFAULT,
					   NULL,
					   NULL,
					   NULL,
					   NULL,
					   (GAsyncReadyCallback) load_cb,
					   g_object_ref (window));
}
