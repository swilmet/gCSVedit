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
#include "gcsv-utils.h"

struct _GcsvWindow
{
	GtkApplicationWindow parent;

	GcsvPropertiesChooser *properties_chooser;
	GtkSourceView *view;
	GtkLabel *statusbar_label;

	GcsvAlignment *align;
};

G_DEFINE_TYPE (GcsvWindow, gcsv_window, GTK_TYPE_APPLICATION_WINDOW)

static GcsvBuffer *
get_buffer (GcsvWindow *window)
{
	return GCSV_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (window->view)));
}

static GtkSourceFile *
get_file (GcsvWindow *window)
{
	GcsvBuffer *buffer = get_buffer (window);
	return gcsv_buffer_get_file (buffer);
}

/* Returns whether @window has been closed. */
static gboolean
launch_close_confirmation_dialog (GcsvWindow *window)
{
	GtkWidget *dialog;
	const gchar *document_name;
	gint response_id;

	document_name = gcsv_buffer_get_short_name (get_buffer (window));

	dialog = gtk_message_dialog_new (GTK_WINDOW (window),
					 GTK_DIALOG_DESTROY_WITH_PARENT |
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_WARNING,
					 GTK_BUTTONS_NONE,
					 _("The document “%s” has unsaved changes."),
					 document_name);

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
gboolean
gcsv_window_close (GcsvWindow *window)
{
	GcsvBuffer *buffer;

	buffer = get_buffer (window);
	if (gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (buffer)))
	{
		return launch_close_confirmation_dialog (window);
	}

	gtk_widget_destroy (GTK_WIDGET (window));
	return TRUE;
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
	GtkFileFilter *dsv_filter;
	GtkFileFilter *all_filter;

	dialog = gtk_file_chooser_dialog_new (_("Open File"),
					      GTK_WINDOW (window),
					      GTK_FILE_CHOOSER_ACTION_OPEN,
					      _("_Cancel"), GTK_RESPONSE_CANCEL,
					      _("_Open"), GTK_RESPONSE_ACCEPT,
					      NULL);

	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), FALSE);

	dsv_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (dsv_filter, _("CSV and TSV Files"));
	gtk_file_filter_add_mime_type (dsv_filter, "text/csv");
	gtk_file_filter_add_mime_type (dsv_filter, "text/tab-separated-values");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), dsv_filter);

	all_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (all_filter, _("All Files"));
	gtk_file_filter_add_pattern (all_filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), all_filter);

	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), dsv_filter);

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
		GcsvBuffer *buffer;

		buffer = get_buffer (window);
		gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (buffer), FALSE);

		gcsv_buffer_add_uri_to_recent_manager (buffer);
	}

	if (error != NULL)
	{
		gcsv_warning (GTK_WINDOW (window),
			      _("Error when saving the file: %s"),
			      error->message);

		g_clear_error (&error);
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
						       get_file (window),
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

	location = gtk_source_file_get_location (get_file (window));
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
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), FALSE);

	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (save_as_dialog_response_cb),
			  window);

	gtk_widget_show (dialog);
}

static void
update_save_action_sensitivity (GcsvWindow *window)
{
	GAction *action;
	GcsvBuffer *buffer;
	GtkSourceFile *file;

	buffer = get_buffer (window);
	file = get_file (window);

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "save");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
				     gtk_source_file_get_location (file) != NULL &&
				     gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (buffer)));
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
	GtkSourceView *view;

	buffer = gcsv_buffer_new ();
	view = GTK_SOURCE_VIEW (gtk_source_view_new_with_buffer (GTK_SOURCE_BUFFER (buffer)));

	gtk_text_view_set_monospace (GTK_TEXT_VIEW (view), TRUE);
	gtk_source_view_set_show_line_numbers (view, TRUE);
	gtk_source_view_set_highlight_current_line (view, TRUE);

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

	gtk_widget_set_hexpand (GTK_WIDGET (view), TRUE);
	gtk_widget_set_vexpand (GTK_WIDGET (view), TRUE);

	return view;
}

static void
update_title (GcsvWindow *window)
{
	gchar *document_title;
	gchar *title;

	document_title = gcsv_buffer_get_document_title (get_buffer (window));

	title = g_strdup_printf ("%s - %s",
				 document_title,
				 g_get_application_name ());

	gtk_window_set_title (GTK_WINDOW (window), title);

	g_free (document_title);
	g_free (title);
}

static void
document_title_notify_cb (GcsvBuffer *buffer,
			  GParamSpec *pspec,
			  GcsvWindow *window)
{
	update_title (window);
}

static void
buffer_modified_changed_cb (GtkTextBuffer *buffer,
			    GcsvWindow    *window)
{
	update_save_action_sensitivity (window);
}

static void
location_notify_cb (GtkSourceFile *file,
		    GParamSpec    *pspec,
		    GcsvWindow    *window)
{
	update_save_action_sensitivity (window);
}

static void
update_statusbar_label (GcsvWindow *window)
{
	GcsvBuffer *buffer;
	GtkTextMark *insert_mark;
	GtkTextIter insert_iter;
	gint line_num;
	gint csv_column_num;
	gchar *label_text;

	buffer = get_buffer (window);
	insert_mark = gtk_text_buffer_get_insert (GTK_TEXT_BUFFER (buffer));
	gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (buffer),
					  &insert_iter,
					  insert_mark);
	line_num = gtk_text_iter_get_line (&insert_iter) + 1;

	csv_column_num = gcsv_buffer_get_column_num (buffer, &insert_iter) + 1;

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
buffer_notify_delimiter_cb (GcsvAlignment *align,
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

	G_OBJECT_CLASS (gcsv_window_parent_class)->dispose (object);
}

static gboolean
gcsv_window_delete_event (GtkWidget   *widget,
			  GdkEventAny *event)
{
	GcsvWindow *window = GCSV_WINDOW (widget);
	GcsvBuffer *buffer;

	buffer = get_buffer (window);
	if (gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (buffer)))
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

	widget_class->delete_event = gcsv_window_delete_event;
}

static void
gcsv_window_init (GcsvWindow *window)
{
	GtkWidget *vgrid;
	GtkWidget *scrolled_window;
	GtkWidget *statusbar;
	GcsvBuffer *buffer;

	window->view = create_view ();
	buffer = get_buffer (window);

	gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);

	vgrid = gtk_grid_new ();
	gtk_orientable_set_orientation (GTK_ORIENTABLE (vgrid), GTK_ORIENTATION_VERTICAL);

	/* Properties chooser */
	window->properties_chooser = gcsv_properties_chooser_new (buffer);
	gtk_container_add (GTK_CONTAINER (vgrid),
			   GTK_WIDGET (window->properties_chooser));

	/* GtkSourceView */
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (window->view));
	gtk_container_add (GTK_CONTAINER (vgrid), scrolled_window);

	/* Disable overlay scrolling, because it currently doesn't work well
	 * with a GtkTextView. It is for example more difficult to place the
	 * cursor on the last line if there is an horizontal scrollbar getting
	 * in the way. Or when placing the cursor on the last visible character
	 * on a line, the vertical scrollbar also gets in the way.
	 * Ideally margins/padding should be set to handle correctly overlay
	 * scrollbars.
	 */
	gtk_scrolled_window_set_overlay_scrolling (GTK_SCROLLED_WINDOW (scrolled_window), FALSE);

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

	window->align = gcsv_alignment_new (buffer);

	add_actions (window);
	update_title (window);
	update_statusbar_label (window);

	g_signal_connect_object (buffer,
				 "notify::document-title",
				 G_CALLBACK (document_title_notify_cb),
				 window,
				 0);

	g_signal_connect_object (buffer,
				 "modified-changed",
				 G_CALLBACK (buffer_modified_changed_cb),
				 window,
				 0);

	g_signal_connect_object (buffer,
				 "cursor-moved",
				 G_CALLBACK (cursor_moved),
				 window,
				 G_CONNECT_SWAPPED);

	g_signal_connect_object (buffer,
				 "notify::delimiter",
				 G_CALLBACK (buffer_notify_delimiter_cb),
				 window,
				 0);

	g_signal_connect_object (get_file (window),
				 "notify::location",
				 G_CALLBACK (location_notify_cb),
				 window,
				 0);

	gtk_widget_show_all (vgrid);
}

GcsvWindow *
gcsv_window_new (GtkApplication *app)
{
	g_return_val_if_fail (GTK_IS_APPLICATION (app), NULL);

	return g_object_new (GCSV_TYPE_WINDOW,
			     "application", app,
			     NULL);
}

static void
load_cb (GtkSourceFileLoader *loader,
	 GAsyncResult        *result,
	 GcsvWindow          *window)
{
	GcsvBuffer *buffer;
	GtkTextIter start;
	GError *error = NULL;

	buffer = get_buffer (window);

	if (gtk_source_file_loader_load_finish (loader, result, &error))
	{
		gcsv_buffer_add_uri_to_recent_manager (buffer);
	}

	if (error != NULL)
	{
		gcsv_warning (GTK_WINDOW (window),
			      _("Error when loading file: %s"),
			      error->message);

		g_clear_error (&error);
	}

	gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (buffer), &start);
	gtk_text_buffer_select_range (GTK_TEXT_BUFFER (buffer), &start, &start);

	gcsv_buffer_guess_delimiter (buffer);
	gcsv_buffer_set_column_titles_line (buffer, 0);

	gcsv_alignment_set_enabled (window->align, TRUE);

	g_signal_handlers_unblock_by_func (buffer, cursor_moved, window);
	cursor_moved (window);

	g_object_unref (loader);
	g_object_unref (window);
}

void
gcsv_window_load_file (GcsvWindow *window,
		       GFile      *location)
{
	GcsvBuffer *buffer;
	GtkSourceFile *file;
	GtkSourceFileLoader *loader;

	g_return_if_fail (GCSV_IS_WINDOW (window));
	g_return_if_fail (G_IS_FILE (location));

	buffer = get_buffer (window);
	file = gcsv_buffer_get_file (buffer);

	gtk_source_file_set_location (file, location);

	loader = gtk_source_file_loader_new (GTK_SOURCE_BUFFER (buffer), file);

	gcsv_alignment_set_enabled (window->align, FALSE);
	g_signal_handlers_block_by_func (buffer, cursor_moved, window);

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
	g_return_val_if_fail (GCSV_IS_WINDOW (window), FALSE);

	return gcsv_buffer_is_untouched (get_buffer (window));
}
