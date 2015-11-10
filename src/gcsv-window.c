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
#include "gcsv-buffer.h"
#include "gcsv-properties-chooser.h"
#include "gcsv-dsv-utils.h"
#include "gcsv-utils.h"

struct _GcsvWindow
{
	GtkApplicationWindow parent;

	GcsvPropertiesChooser *properties_chooser;
	GtkSourceView *view;
	GtkLabel *statusbar_label;

	GtkSourceFile *file;
	GcsvAlignment *align;

	gchar *document_name;
};

G_DEFINE_TYPE (GcsvWindow, gcsv_window, GTK_TYPE_APPLICATION_WINDOW)

/* Returns whether @window has been closed. */
static gboolean
launch_close_confirmation_dialog (GcsvWindow *window)
{
	GtkWidget *dialog;
	gint response_id;

	dialog = gtk_message_dialog_new (GTK_WINDOW (window),
					 GTK_DIALOG_DESTROY_WITH_PARENT |
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_WARNING,
					 GTK_BUTTONS_NONE,
					 _("The document “%s” has unsaved changes."),
					 window->document_name);

	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
				_("Close _without Saving"), GTK_RESPONSE_CLOSE,
				_("_Don't Close"), GTK_RESPONSE_CANCEL,
				NULL);

	response_id = gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);

	if (response_id == GTK_RESPONSE_CLOSE)
	{
		gtk_widget_destroy (GTK_WIDGET (window));
		return TRUE;
	}

	return FALSE;
}

/* Returns whether the window has been closed. */
static gboolean
gcsv_window_close (GcsvWindow *window)
{
	GtkTextBuffer *buffer;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (window->view));
	if (gtk_text_buffer_get_modified (buffer))
	{
		return launch_close_confirmation_dialog (window);
	}

	gtk_widget_destroy (GTK_WIDGET (window));
	return TRUE;
}

static void
quit_activate_cb (GSimpleAction *quit_action,
		  GVariant      *parameter,
		  gpointer       user_data)
{
	GtkApplication *app = GTK_APPLICATION (g_application_get_default ());

	while (TRUE)
	{
		GtkWindow *window = gtk_application_get_active_window (app);

		if (GCSV_IS_WINDOW (window) &&
		    gcsv_window_close (GCSV_WINDOW (window)))
		{
			continue;
		}

		break;
	}
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
launch_save (GcsvWindow *window,
	     GFile      *location)
{
	GtkSourceBuffer *buffer_without_align;
	GtkSourceFileSaver *saver;
	GApplication *app;

	buffer_without_align = gcsv_alignment_copy_buffer_without_alignment (window->align);

	saver = gtk_source_file_saver_new_with_target (buffer_without_align,
						       window->file,
						       location);

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
save_activate_cb (GSimpleAction *save_action,
		  GVariant      *parameter,
		  gpointer       user_data)
{
	GcsvWindow *window = GCSV_WINDOW (user_data);
	GFile *location;

	location = gtk_source_file_get_location (window->file);
	g_return_if_fail (location != NULL);

	launch_save (window, location);
}

static void
save_as_dialog_response_cb (GtkFileChooserDialog *dialog,
			    gint                  response_id,
			    GcsvWindow           *window)
{
	GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
	GFile *location;

	if (response_id != GTK_RESPONSE_ACCEPT)
	{
		goto out;
	}

	location = gtk_file_chooser_get_file (chooser);

	launch_save (window, location);

	g_object_unref (location);

out:
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
save_as_activate_cb (GSimpleAction *save_as_action,
		     GVariant      *parameter,
		     gpointer       user_data)
{
	GcsvWindow *window = GCSV_WINDOW (user_data);
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new (_("Save File"),
					      GTK_WINDOW (window),
					      GTK_FILE_CHOOSER_ACTION_SAVE,
					      _("_Cancel"), GTK_RESPONSE_CANCEL,
					      _("_Save"), GTK_RESPONSE_ACCEPT,
					      NULL);

	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (save_as_dialog_response_cb),
			  window);

	gtk_widget_show (dialog);
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
		{ "save_as", save_as_activate_cb },
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
	GcsvBuffer *buffer;
	GtkSourceBuffer *source_buffer;
	GtkSourceView *view;
	GtkSourceLanguageManager *language_manager;
	GtkSourceLanguage *csv_lang;
	GtkSourceStyleSchemeManager *scheme_manager;
	GtkSourceStyleScheme *scheme;

	buffer = gcsv_buffer_new ();
	source_buffer = GTK_SOURCE_BUFFER (buffer);
	view = GTK_SOURCE_VIEW (gtk_source_view_new_with_buffer (source_buffer));

	gtk_text_view_set_monospace (GTK_TEXT_VIEW (view), TRUE);
	gtk_source_view_set_show_line_numbers (view, TRUE);
	gtk_source_view_set_highlight_current_line (view, TRUE);

	/* Disable the undo/redo, because it doesn't work well currently with
	 * the virtual spaces.
	 */
	gtk_source_buffer_set_max_undo_levels (source_buffer, 0);

	/* Draw all kind of spaces everywhere except CR and LF.
	 * Line numbers are already displayed, so drawing line breaks would be
	 * redundant and is not very useful.
	 */
	gtk_source_view_set_draw_spaces (view,
					 GTK_SOURCE_DRAW_SPACES_SPACE |
					 GTK_SOURCE_DRAW_SPACES_TAB |
					 GTK_SOURCE_DRAW_SPACES_NBSP |
					 GTK_SOURCE_DRAW_SPACES_LEADING |
					 GTK_SOURCE_DRAW_SPACES_TEXT |
					 GTK_SOURCE_DRAW_SPACES_TRAILING);

	language_manager = gtk_source_language_manager_get_default ();
	csv_lang = gtk_source_language_manager_get_language (language_manager, "csv");
	gtk_source_buffer_set_language (source_buffer, csv_lang);

	scheme_manager = gtk_source_style_scheme_manager_get_default ();
	scheme = gtk_source_style_scheme_manager_get_scheme (scheme_manager, "tango");
	gtk_source_buffer_set_style_scheme (source_buffer, scheme);

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

static gchar *
get_document_title (GcsvWindow *window)
{
	GFile *location;
	GFile *parent;
	gchar *directory;
	gchar *document_title;

	location = gtk_source_file_get_location (window->file);

	if (location == NULL)
	{
		return g_strdup (window->document_name);
	}

	parent = g_file_get_parent (location);
	g_return_val_if_fail (parent != NULL, NULL);

	directory = g_file_get_parse_name (parent);

	document_title = g_strdup_printf ("%s (%s)",
					  window->document_name,
					  directory);

	g_object_unref (parent);
	g_free (directory);

	return document_title;
}

static void
update_title (GcsvWindow *window)
{
	GtkTextBuffer *buffer;
	gboolean modified;
	const gchar *modified_marker;
	gchar *document_title;
	gchar *title;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (window->view));
	modified = gtk_text_buffer_get_modified (buffer);
	modified_marker = modified ? "*" : "";

	document_title = get_document_title (window);

	title = g_strdup_printf ("%s%s - %s",
				 modified_marker,
				 document_title,
				 g_get_application_name ());

	gtk_window_set_title (GTK_WINDOW (window), title);

	g_free (document_title);
	g_free (title);
}

static void
query_display_name_cb (GFile        *location,
		       GAsyncResult *result,
		       GcsvWindow   *window)
{
	GFileInfo *info;
	GError *error = NULL;

	info = g_file_query_info_finish (location, result, &error);

	if (error != NULL)
	{
		gcsv_warning (GTK_WINDOW (window),
			      _("Error when querying file information: %s"),
			      error->message);

		g_clear_error (&error);
		goto out;
	}

	g_free (window->document_name);
	window->document_name = g_strdup (g_file_info_get_display_name (info));

	update_title (window);

out:
	g_clear_object (&info);

	/* Async operation finished */
	g_object_unref (window);
}

static void
update_document_name (GcsvWindow *window)
{
	GFile *location;

	location = gtk_source_file_get_location (window->file);

	if (location == NULL)
	{
		g_free (window->document_name);
		window->document_name = g_strdup (_("Unsaved File"));
		update_title (window);
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
	update_document_name (window);
	update_save_action_sensitivity (window);
}

static void
update_statusbar_label (GcsvWindow *window)
{
	GtkTextBuffer *buffer;
	GtkTextIter insert;
	gint line_num;
	gint csv_column_num;
	gunichar delimiter;
	gchar *label_text;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (window->view));
	gtk_text_buffer_get_iter_at_mark (buffer, &insert,
					  gtk_text_buffer_get_insert (buffer));
	line_num = gtk_text_iter_get_line (&insert) + 1;

	delimiter = gcsv_buffer_get_delimiter (GCSV_BUFFER (buffer));
	csv_column_num = gcsv_dsv_get_column_num (&insert, delimiter) + 1;

	label_text = g_strdup_printf (_("Line: %d   CSV Column: %d"),
				      line_num,
				      csv_column_num);

	gtk_label_set_text (window->statusbar_label, label_text);
	g_free (label_text);
}

static void
cursor_moved (GcsvWindow *window)
{
	update_statusbar_label (window);
}

static void
buffer_mark_set_after_cb (GtkTextBuffer *buffer,
			  GtkTextIter   *location,
			  GtkTextMark   *mark,
			  GcsvWindow    *window)
{
	if (mark == gtk_text_buffer_get_insert (buffer))
	{
		cursor_moved (window);
	}
}

static void
buffer_changed_cb (GtkTextBuffer *buffer,
		   GcsvWindow    *window)
{
	cursor_moved (window);
}

static void
alignment_notify_delimiter_cb (GcsvAlignment *align,
			       GParamSpec    *pspec,
			       GcsvWindow    *window)
{
	update_statusbar_label (window);
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
gcsv_window_finalize (GObject *object)
{
	GcsvWindow *window = GCSV_WINDOW (object);

	g_free (window->document_name);

	G_OBJECT_CLASS (gcsv_window_parent_class)->finalize (object);
}

static gboolean
gcsv_window_delete_event (GtkWidget   *widget,
			  GdkEventAny *event)
{
	GcsvWindow *window = GCSV_WINDOW (widget);
	GtkTextBuffer *buffer;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (window->view));
	if (gtk_text_buffer_get_modified (buffer))
	{
		launch_close_confirmation_dialog (window);
		return GDK_EVENT_STOP;
	}

	return GDK_EVENT_PROPAGATE;
}

static void
gcsv_window_class_init (GcsvWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	object_class->dispose = gcsv_window_dispose;
	object_class->finalize = gcsv_window_finalize;

	widget_class->delete_event = gcsv_window_delete_event;
}

static void
gcsv_window_init (GcsvWindow *window)
{
	GtkWidget *vgrid;
	GtkWidget *scrolled_window;
	GtkWidget *statusbar;
	GcsvBuffer *buffer;

	gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);

	vgrid = gtk_grid_new ();
	gtk_orientable_set_orientation (GTK_ORIENTABLE (vgrid), GTK_ORIENTATION_VERTICAL);

	/* Menubar */
	gtk_container_add (GTK_CONTAINER (vgrid), get_menubar ());

	/* Delimiter chooser */
	window->properties_chooser = gcsv_properties_chooser_new (',');
	gtk_container_add (GTK_CONTAINER (vgrid),
			   GTK_WIDGET (window->properties_chooser));

	/* GtkSourceView */
	window->view = create_view ();

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (window->view));
	gtk_container_add (GTK_CONTAINER (vgrid), scrolled_window);

	/* Statusbar */
	statusbar = gtk_statusbar_new ();
	gtk_widget_set_margin_top (statusbar, 0);
	gtk_widget_set_margin_bottom (statusbar, 0);
	window->statusbar_label = GTK_LABEL (gtk_label_new (NULL));
	gtk_box_pack_end (GTK_BOX (statusbar),
			  GTK_WIDGET (window->statusbar_label),
			  FALSE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (vgrid), statusbar);

	gtk_container_add (GTK_CONTAINER (window), vgrid);
	gtk_widget_grab_focus (GTK_WIDGET (window->view));
	gtk_widget_show_all (GTK_WIDGET (window));

	window->file = gtk_source_file_new ();

	buffer = GCSV_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (window->view)));
	window->align = gcsv_alignment_new (buffer);

	add_actions (window);
	update_document_name (window);
	update_statusbar_label (window);

	g_signal_connect_object (buffer,
				 "modified-changed",
				 G_CALLBACK (buffer_modified_changed_cb),
				 window,
				 0);

	g_signal_connect_object (buffer,
				 "mark-set",
				 G_CALLBACK (buffer_mark_set_after_cb),
				 window,
				 G_CONNECT_AFTER);

	g_signal_connect_object (buffer,
				 "changed",
				 G_CALLBACK (buffer_changed_cb),
				 window,
				 0);

	g_signal_connect_object (window->file,
				 "notify::location",
				 G_CALLBACK (location_notify_cb),
				 window,
				 0);

	g_object_bind_property (window->properties_chooser, "delimiter",
				buffer, "delimiter",
				G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

	g_object_bind_property (window->properties_chooser, "title-line",
				window->align, "title-line",
				G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

	g_signal_connect_object (window->align,
				 "notify::delimiter",
				 G_CALLBACK (alignment_notify_delimiter_cb),
				 window,
				 0);
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
	GtkTextBuffer *buffer;
	GtkTextIter start;
	gunichar delimiter;

	gtk_source_file_loader_load_finish (loader, result, &error);

	if (error != NULL)
	{
		gcsv_warning (GTK_WINDOW (window),
			      _("Error when loading file: %s"),
			      error->message);

		g_error_free (error);
		error = NULL;
	}

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (window->view));
	delimiter = gcsv_dsv_guess_delimiter (buffer);
	gcsv_properties_chooser_set_delimiter (window->properties_chooser, delimiter);

	gcsv_alignment_set_enabled (window->align, TRUE);

	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_select_range (buffer, &start, &start);

	g_signal_handlers_unblock_by_func (buffer, buffer_mark_set_after_cb, window);
	g_signal_handlers_unblock_by_func (buffer, buffer_changed_cb, window);
	cursor_moved (window);

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
	g_signal_handlers_block_by_func (buffer, buffer_mark_set_after_cb, window);
	g_signal_handlers_block_by_func (buffer, buffer_changed_cb, window);

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
