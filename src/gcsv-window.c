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

#include "config.h"
#include "gcsv-window.h"
#include <gtksourceview/gtksource.h>
#include <glib/gi18n.h>
#include "gcsv-alignment.h"
#include "gcsv-delimiter-chooser.h"
#include "gcsv-utils.h"

struct _GcsvWindow
{
	GtkApplicationWindow parent;

	GcsvDelimiterChooser *delimiter_chooser;
	GtkSourceView *view;
	GtkSourceFile *file;
	GcsvAlignment *align;
};

G_DEFINE_TYPE (GcsvWindow, gcsv_window, GTK_TYPE_APPLICATION_WINDOW)

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
save_cb (GtkSourceFileSaver *saver,
	 GAsyncResult       *result,
	 GcsvWindow         *window)
{
	GError *error = NULL;
	GtkSourceBuffer *buffer_without_align;
	GApplication *app;

	if (gtk_source_file_saver_save_finish (saver, result, &error))
	{
		GtkTextBuffer *buffer;

		buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (window->view));
		gtk_text_buffer_set_modified (buffer, FALSE);
	}

	if (error != NULL)
	{
		gcsv_warning (GTK_WINDOW (window),
			      _("Error when saving file: %s"),
			      error->message);

		g_error_free (error);
		error = NULL;
	}

	buffer_without_align = gtk_source_file_saver_get_buffer (saver);
	g_object_unref (buffer_without_align);
	g_object_unref (saver);

	app = g_application_get_default ();
	g_application_unmark_busy (app);
	g_application_release (app);

	g_object_unref (window);
}

static void
save_activate_cb (GSimpleAction *save_action,
		  GVariant      *parameter,
		  gpointer       user_data)
{
	GcsvWindow *window = GCSV_WINDOW (user_data);
	GFile *location;
	GtkSourceBuffer *buffer_without_align;
	GtkSourceFileSaver *saver;
	GApplication *app;

	location = gtk_source_file_get_location (window->file);
	g_return_if_fail (location != NULL);

	buffer_without_align = gcsv_alignment_copy_buffer_without_alignment (window->align);

	saver = gtk_source_file_saver_new (buffer_without_align,
					   window->file);

	app = g_application_get_default ();
	g_application_hold (app);
	g_application_mark_busy (app);

	gtk_source_file_saver_save_async (saver,
					  G_PRIORITY_DEFAULT,
					  NULL,
					  NULL,
					  NULL,
					  NULL,
					  (GAsyncReadyCallback) save_cb,
					  g_object_ref (window));
}

static void
about_activate_cb (GSimpleAction *about_action,
		   GVariant      *parameter,
		   gpointer       user_data)
{
	GcsvWindow *window = GCSV_WINDOW (user_data);

	const gchar *authors[] = {
		"Sébastien Wilmet <sebastien.wilmet@uclouvain.be>",
		NULL
	};

	gtk_show_about_dialog (GTK_WINDOW (window),
			       "name", g_get_application_name (),
			       "version", PACKAGE_VERSION,
			       "comments", _("gCSVedit is a small and lightweight CSV text editor"),
			       "authors", authors,
			       "translator-credits", _("translator-credits"),
			       "website", PACKAGE_URL,
			       "website-label", _("gCSVedit website"),
			       "logo-icon-name", "accessories-text-editor",
			       "license-type", GTK_LICENSE_GPL_3_0,
			       "copyright", "Copyright 2015 – Université Catholique de Louvain",
			       NULL);
}

static void
update_save_action_sensitivity (GcsvWindow *window)
{
	GAction *action;
	GtkTextBuffer *buffer;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (window->view));

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "save");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
				     gtk_source_file_get_location (window->file) != NULL &&
				     gtk_text_buffer_get_modified (buffer));
}

static void
update_actions_sensitivity (GcsvWindow *window)
{
	update_save_action_sensitivity (window);
}

static void
add_actions (GcsvWindow *window)
{
	const GActionEntry entries[] = {
		{ "open", open_activate_cb },
		{ "save", save_activate_cb },
		{ "quit", quit_activate_cb },
		{ "about", about_activate_cb },
	};

	g_action_map_add_action_entries (G_ACTION_MAP (window),
					 entries,
					 G_N_ELEMENTS (entries),
					 window);

	update_actions_sensitivity (window);
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
	gtk_source_view_set_show_line_numbers (view, TRUE);
	gtk_source_view_set_highlight_current_line (view, TRUE);

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
set_title (GcsvWindow  *window,
	   const gchar *file_title)
{
	GtkTextBuffer *buffer;
	gboolean modified;
	const gchar *modified_marker;
	gchar *title;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (window->view));
	modified = gtk_text_buffer_get_modified (buffer);
	modified_marker = modified ? "*" : "";

	title = g_strdup_printf ("%s%s - %s",
				 modified_marker,
				 file_title,
				 g_get_application_name ());

	gtk_window_set_title (GTK_WINDOW (window), title);
	g_free (title);
}

static void
query_display_name_cb (GFile        *location,
		       GAsyncResult *result,
		       GcsvWindow   *window)
{
	GFileInfo *info;
	const gchar *display_name;
	GFile *parent = NULL;
	gchar *directory = NULL;
	gchar *file_title = NULL;
	GError *error = NULL;

	info = g_file_query_info_finish (location, result, &error);

	if (error != NULL)
	{
		gcsv_warning (GTK_WINDOW (window),
			      _("Error when querying file information: %s"),
			      error->message);

		g_error_free (error);
		error = NULL;
		goto out;
	}

	display_name = g_file_info_get_display_name (info);

	parent = g_file_get_parent (location);
	g_return_if_fail (parent != NULL);

	directory = g_file_get_parse_name (parent);

	file_title = g_strdup_printf ("%s (%s)", display_name, directory);
	set_title (window, file_title);

out:
	g_clear_object (&info);
	g_clear_object (&parent);
	g_free (directory);
	g_free (file_title);

	/* Async operation finished */
	g_object_unref (window);
}

static void
update_title (GcsvWindow *window)
{
	GFile *location;

	location = gtk_source_file_get_location (window->file);

	if (location == NULL)
	{
		set_title (window, _("Unsaved File"));
	}
	else
	{
		g_file_query_info_async (location,
					 G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
					 G_FILE_QUERY_INFO_NONE,
					 G_PRIORITY_DEFAULT,
					 NULL,
					 (GAsyncReadyCallback) query_display_name_cb,
					 g_object_ref (window));
	}
}

static void
buffer_modified_changed_cb (GtkTextBuffer *buffer,
			    GcsvWindow    *window)
{
	update_title (window);
	update_save_action_sensitivity (window);
}

static void
location_notify_cb (GtkSourceFile *file,
		    GParamSpec    *pspec,
		    GcsvWindow    *window)
{
	update_title (window);
	update_save_action_sensitivity (window);
}

static void
gcsv_window_dispose (GObject *object)
{
	GcsvWindow *window = GCSV_WINDOW (object);

	g_clear_object (&window->align);
	g_clear_object (&window->file);

	G_OBJECT_CLASS (gcsv_window_parent_class)->dispose (object);
}

static void
gcsv_window_class_init (GcsvWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gcsv_window_dispose;
}

static void
gcsv_window_init (GcsvWindow *window)
{
	GtkWidget *vgrid;
	GtkWidget *scrolled_window;
	GtkTextBuffer *buffer;

	gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);

	vgrid = gtk_grid_new ();
	gtk_orientable_set_orientation (GTK_ORIENTABLE (vgrid), GTK_ORIENTATION_VERTICAL);

	gtk_container_add (GTK_CONTAINER (vgrid), get_menubar ());

	window->delimiter_chooser = gcsv_delimiter_chooser_new (',');
	gtk_container_add (GTK_CONTAINER (vgrid),
			   GTK_WIDGET (window->delimiter_chooser));

	window->view = create_view ();

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (window->view));
	gtk_container_add (GTK_CONTAINER (vgrid), scrolled_window);

	gtk_container_add (GTK_CONTAINER (window), vgrid);
	gtk_widget_grab_focus (GTK_WIDGET (window->view));
	gtk_widget_show_all (GTK_WIDGET (window));

	window->file = gtk_source_file_new ();

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (window->view));
	window->align = gcsv_alignment_new (buffer, ',');

	add_actions (window);
	update_title (window);

	g_signal_connect_object (buffer,
				 "modified-changed",
				 G_CALLBACK (buffer_modified_changed_cb),
				 window,
				 0);

	g_signal_connect_object (window->file,
				 "notify::location",
				 G_CALLBACK (location_notify_cb),
				 window,
				 0);

	g_object_bind_property (window->delimiter_chooser, "delimiter",
				window->align, "delimiter",
				G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);
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
		gcsv_warning (GTK_WINDOW (window),
			      _("Error when loading file: %s"),
			      error->message);

		g_error_free (error);
		error = NULL;
	}

	gcsv_alignment_set_enabled (window->align, TRUE);

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

	gcsv_alignment_set_enabled (window->align, FALSE);

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
