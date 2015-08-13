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
#include <glib/gi18n.h>
#include "gcsv-alignment.h"

struct _GcsvWindow
{
	GtkApplicationWindow parent;

	GtkSourceView *view;
	GtkSourceFile *file;

	GcsvAlignment *align;
};

G_DEFINE_TYPE (GcsvWindow, gcsv_window, GTK_TYPE_APPLICATION_WINDOW)

static void
gcsv_window_dispose (GObject *object)
{
	GcsvWindow *window = GCSV_WINDOW (object);

	g_clear_object (&window->file);
	g_clear_object (&window->align);

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
quit_activate_cb (GSimpleAction *quit_action,
		  GVariant      *parameter,
		  gpointer       user_data)
{
	g_application_quit (g_application_get_default ());
}

static void
open_dialog_response_cb (GtkFileChooserDialog *dialog,
			 gint                  response_id,
			 GcsvWindow           *window)
{
	if (response_id == GTK_RESPONSE_ACCEPT)
	{
		GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
		GApplication *app;
		GFile *file;
		GFile *files[1];

		file = gtk_file_chooser_get_file (chooser);
		files[0] = file;

		app = g_application_get_default ();
		g_application_open (app, files, 1, NULL);

		g_object_unref (file);
	}

	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
open_activate_cb (GSimpleAction *open_action,
		  GVariant      *parameter,
		  gpointer       user_data)
{
	GcsvWindow *window = GCSV_WINDOW (user_data);
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new (_("Open File"),
					      GTK_WINDOW (window),
					      GTK_FILE_CHOOSER_ACTION_OPEN,
					      _("_Cancel"), GTK_RESPONSE_CANCEL,
					      _("_Open"), GTK_RESPONSE_ACCEPT,
					      NULL);

	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (open_dialog_response_cb),
			  window);

	gtk_widget_show (dialog);
}

static void
add_actions (GcsvWindow *window)
{
	const GActionEntry entries[] = {
		{ "quit", quit_activate_cb },
		{ "open", open_activate_cb },
	};

	g_action_map_add_action_entries (G_ACTION_MAP (window),
					 entries,
					 G_N_ELEMENTS (entries),
					 window);
}

static GtkSourceView *
create_view (void)
{
	GtkSourceView *view;
	GtkSourceBuffer *buffer;
	GtkSourceLanguageManager *language_manager;
	GtkSourceLanguage *csv_lang;
	GtkSourceStyleSchemeManager *scheme_manager;
	GtkSourceStyleScheme *scheme;

	view = GTK_SOURCE_VIEW (gtk_source_view_new ());
	buffer = GTK_SOURCE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));

	gtk_text_view_set_monospace (GTK_TEXT_VIEW (view), TRUE);

	language_manager = gtk_source_language_manager_get_default ();
	csv_lang = gtk_source_language_manager_get_language (language_manager, "csv");
	gtk_source_buffer_set_language (buffer, csv_lang);

	scheme_manager = gtk_source_style_scheme_manager_get_default ();
	scheme = gtk_source_style_scheme_manager_get_scheme (scheme_manager, "tango");
	gtk_source_buffer_set_style_scheme (buffer, scheme);

	gtk_widget_set_hexpand (GTK_WIDGET (view), TRUE);
	gtk_widget_set_vexpand (GTK_WIDGET (view), TRUE);

	return view;
}

static GtkWidget *
get_menubar (void)
{
	GtkApplication *app;
	GMenuModel *model;

	app = GTK_APPLICATION (g_application_get_default ());
	model = gtk_application_get_menubar (app);

	return gtk_menu_bar_new_from_model (model);
}

static void
gcsv_window_init (GcsvWindow *window)
{
	GtkWidget *vgrid;
	GtkWidget *scrolled_window;
	GtkTextBuffer *buffer;

	add_actions (window);

	vgrid = gtk_grid_new ();
	gtk_orientable_set_orientation (GTK_ORIENTABLE (vgrid), GTK_ORIENTATION_VERTICAL);

	gtk_container_add (GTK_CONTAINER (vgrid), get_menubar ());

	/*
	gtk_window_set_title (GTK_WINDOW (window), g_get_application_name ());
	*/
	gtk_window_set_title (GTK_WINDOW (window), _("gCSVedit - CSV editor"));
	gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);

	window->view = create_view ();

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (window->view));
	gtk_container_add (GTK_CONTAINER (vgrid), scrolled_window);

	gtk_container_add (GTK_CONTAINER (window), vgrid);
	gtk_widget_show_all (GTK_WIDGET (window));

	window->file = gtk_source_file_new ();

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (window->view));
	window->align = gcsv_alignment_new (buffer, ',');
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

	gcsv_alignment_update (window->align);

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

gboolean
gcsv_window_is_untouched (GcsvWindow *window)
{
	GtkTextBuffer *buffer;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (window->view));

	return (gtk_text_buffer_get_char_count (buffer) == 0 &&
		!gtk_source_buffer_can_undo (GTK_SOURCE_BUFFER (buffer)) &&
		!gtk_source_buffer_can_redo (GTK_SOURCE_BUFFER (buffer)) &&
		gtk_source_file_get_location (window->file) == NULL);
}
