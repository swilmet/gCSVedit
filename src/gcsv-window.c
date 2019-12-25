/*
 * This file is part of gCSVedit.
 *
 * Copyright 2015, 2016 - Université Catholique de Louvain
 * Copyright 2017 - Sébastien Wilmet
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
 * Author: Sébastien Wilmet <swilmet@gnome.org>
 */

#include "config.h"
#include "gcsv-window.h"
#include <gtksourceview/gtksource.h>
#include <tepl/tepl.h>
#include <glib/gi18n.h>
#include "gcsv-alignment.h"
#include "gcsv-application.h"
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

static TeplFile *
get_file (GcsvWindow *window)
{
	GcsvBuffer *buffer = get_buffer (window);
	return tepl_buffer_get_file (TEPL_BUFFER (buffer));
}

/* Returns whether @window has been closed. */
static gboolean
launch_close_confirmation_dialog (GcsvWindow *window)
{
	GtkWidget *dialog;
	const gchar *document_name;
	gint response_id;

	document_name = tepl_file_get_short_name (TEPL_FILE (get_file (window)));

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
open_file_chooser_response_cb (GtkFileChooserDialog *file_chooser_dialog,
			       gint                  response_id,
			       GcsvWindow           *window)
{
	if (response_id == GTK_RESPONSE_ACCEPT)
	{
		TeplApplication *app;
		GFile *file;

		file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (file_chooser_dialog));

		app = tepl_application_get_default ();
		tepl_application_open_simple (app, file);

		g_object_unref (file);
	}

	gtk_widget_destroy (GTK_WIDGET (file_chooser_dialog));
}

static void
open_activate_cb (GSimpleAction *open_action,
		  GVariant      *parameter,
		  gpointer       user_data)
{
	GcsvWindow *window = GCSV_WINDOW (user_data);
	GtkWidget *file_chooser_dialog;
	GtkFileChooser *file_chooser;
	GtkFileFilter *dsv_filter;
	GtkFileFilter *all_filter;

	/* Create a GtkFileChooserDialog, not a GtkFileChooserNative, because
	 * with GtkFileChooserNative the GFile that we obtain (in flatpak)
	 * doesn't have the real path to the file, so it would screw up some
	 * features in gCSVedit:
	 * - showing the directory in parentheses in the window title;
	 * - opening a recent file.
	 */
	file_chooser_dialog = gtk_file_chooser_dialog_new (_("Open File"),
							   GTK_WINDOW (window),
							   GTK_FILE_CHOOSER_ACTION_OPEN,
							   _("_Cancel"), GTK_RESPONSE_CANCEL,
							   _("_Open"), GTK_RESPONSE_ACCEPT,
							   NULL);

	gtk_dialog_set_default_response (GTK_DIALOG (file_chooser_dialog), GTK_RESPONSE_ACCEPT);

	file_chooser = GTK_FILE_CHOOSER (file_chooser_dialog);
	gtk_file_chooser_set_local_only (file_chooser, FALSE);

	dsv_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (dsv_filter, _("CSV and TSV Files"));
#ifdef G_OS_WIN32
	/* The mime types don't work on Windows... */
	gtk_file_filter_add_pattern (dsv_filter, "*.csv");
	gtk_file_filter_add_pattern (dsv_filter, "*.tsv");
#else
	/* ...but mimetypes are better.*/
	gtk_file_filter_add_mime_type (dsv_filter, "text/csv");
	gtk_file_filter_add_mime_type (dsv_filter, "text/tab-separated-values");
#endif
	gtk_file_chooser_add_filter (file_chooser, dsv_filter);

	all_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (all_filter, _("All Files"));
	gtk_file_filter_add_pattern (all_filter, "*");
	gtk_file_chooser_add_filter (file_chooser, all_filter);

	gtk_file_chooser_set_filter (file_chooser, dsv_filter);

	g_signal_connect_object (file_chooser_dialog,
				 "response",
				 G_CALLBACK (open_file_chooser_response_cb),
				 window,
				 0);

	gtk_widget_show (file_chooser_dialog);
}

static void
save_cb (TeplFileSaver *saver,
	 GAsyncResult  *result,
	 GcsvWindow    *window)
{
	GError *error = NULL;
	TeplBuffer *buffer_without_align;
	GApplication *app;

	if (tepl_file_saver_save_finish (saver, result, &error))
	{
		GcsvBuffer *buffer;
		TeplFile *file;

		buffer = get_buffer (window);
		gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (buffer), FALSE);

		file = get_file (window);
		tepl_file_add_uri_to_recent_manager (file);
	}

	if (error != NULL)
	{
		gcsv_warning (GTK_WINDOW (window),
			      _("Error when saving the file: %s"),
			      error->message);

		g_clear_error (&error);
	}

	buffer_without_align = tepl_file_saver_get_buffer (saver);
	g_object_unref (buffer_without_align);
	g_object_unref (saver);

	app = g_application_get_default ();
	g_application_unmark_busy (app);
	g_application_release (app);

	g_object_unref (window);
}

static void
launch_saver (GcsvWindow    *window,
	      TeplFileSaver *saver)
{
	GApplication *app;

	app = g_application_get_default ();
	g_application_hold (app);
	g_application_mark_busy (app);

	tepl_file_saver_save_async (saver,
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
	TeplFile *file;
	GFile *location;
	TeplBuffer *buffer_without_align;
	TeplFileSaver *saver;

	file = get_file (window);
	location = tepl_file_get_location (file);
	g_return_if_fail (location != NULL);

	buffer_without_align = gcsv_alignment_copy_buffer_without_alignment (window->align);

	saver = tepl_file_saver_new (buffer_without_align, file);
	launch_saver (window, saver);
}

static void
save_file_chooser_response_cb (GtkFileChooserDialog *file_chooser_dialog,
			       gint                  response_id,
			       GcsvWindow           *window)
{
	if (response_id == GTK_RESPONSE_ACCEPT)
	{
		GFile *location;
		TeplBuffer *buffer_without_align;
		TeplFileSaver *saver;

		location = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (file_chooser_dialog));

		buffer_without_align = gcsv_alignment_copy_buffer_without_alignment (window->align);

		saver = tepl_file_saver_new_with_target (buffer_without_align,
							 get_file (window),
							 location);
		launch_saver (window, saver);

		g_object_unref (location);
	}

	gtk_widget_destroy (GTK_WIDGET (file_chooser_dialog));
}

static void
save_as_activate_cb (GSimpleAction *save_as_action,
		     GVariant      *parameter,
		     gpointer       user_data)
{
	GcsvWindow *window = GCSV_WINDOW (user_data);
	GtkWidget *file_chooser_dialog;
	GtkFileChooser *file_chooser;

	file_chooser_dialog = gtk_file_chooser_dialog_new (_("Save File"),
							   GTK_WINDOW (window),
							   GTK_FILE_CHOOSER_ACTION_SAVE,
							   _("_Cancel"), GTK_RESPONSE_CANCEL,
							   _("_Save"), GTK_RESPONSE_ACCEPT,
							   NULL);

	gtk_dialog_set_default_response (GTK_DIALOG (file_chooser_dialog), GTK_RESPONSE_ACCEPT);

	file_chooser = GTK_FILE_CHOOSER (file_chooser_dialog);

	gtk_file_chooser_set_do_overwrite_confirmation (file_chooser, TRUE);
	gtk_file_chooser_set_local_only (file_chooser, FALSE);

	g_signal_connect_object (file_chooser_dialog,
				 "response",
				 G_CALLBACK (save_file_chooser_response_cb),
				 window,
				 0);

	gtk_widget_show (file_chooser_dialog);
}

static void
update_save_action_sensitivity (GcsvWindow *window)
{
	GAction *action;
	GcsvBuffer *buffer;
	TeplFile *file;

	buffer = get_buffer (window);
	file = get_file (window);

	action = g_action_map_lookup_action (G_ACTION_MAP (window), "save");
	g_simple_action_set_enabled (G_SIMPLE_ACTION (action),
				     tepl_file_get_location (file) != NULL &&
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
		{ "save-as", save_as_activate_cb },
	};

	amtk_action_map_add_action_entries_check_dups (G_ACTION_MAP (window),
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
	GtkSourceSpaceDrawer *space_drawer;

	buffer = gcsv_buffer_new ();
	view = GTK_SOURCE_VIEW (gtk_source_view_new_with_buffer (GTK_SOURCE_BUFFER (buffer)));
	g_object_unref (buffer);

	gtk_text_view_set_monospace (GTK_TEXT_VIEW (view), TRUE);
	gtk_source_view_set_show_line_numbers (view, TRUE);
	gtk_source_view_set_highlight_current_line (view, TRUE);

	/* Draw all kind of spaces everywhere except CR and LF.
	 * Line numbers are already displayed, so drawing line breaks would be
	 * redundant and is not very useful.
	 */
	space_drawer = gtk_source_view_get_space_drawer (view);
	gtk_source_space_drawer_set_enable_matrix (space_drawer, TRUE);
	gtk_source_space_drawer_set_types_for_locations (space_drawer,
							 GTK_SOURCE_SPACE_LOCATION_ALL,
							 GTK_SOURCE_SPACE_TYPE_ALL &
							 ~GTK_SOURCE_SPACE_TYPE_NEWLINE);

	gtk_widget_set_hexpand (GTK_WIDGET (view), TRUE);
	gtk_widget_set_vexpand (GTK_WIDGET (view), TRUE);

	return view;
}

static void
update_title (GcsvWindow *window)
{
	gchar *buffer_title;
	gchar *window_title;

	buffer_title = tepl_buffer_get_full_title (TEPL_BUFFER (get_buffer (window)));

	window_title = g_strdup_printf ("%s - %s",
					buffer_title,
					g_get_application_name ());

	gtk_window_set_title (GTK_WINDOW (window), window_title);

	g_free (buffer_title);
	g_free (window_title);
}

static void
buffer_full_title_notify_cb (TeplBuffer *buffer,
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
location_notify_cb (TeplFile   *file,
		    GParamSpec *pspec,
		    GcsvWindow *window)
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

static GtkWidget *
create_file_submenu (GcsvWindow *gcsv_window)
{
	GtkMenuShell *file_submenu;
	AmtkApplicationWindow *amtk_window;
	AmtkFactory *factory;

	file_submenu = GTK_MENU_SHELL (gtk_menu_new ());

	amtk_window = amtk_application_window_get_from_gtk_application_window (GTK_APPLICATION_WINDOW (gcsv_window));

	factory = amtk_factory_new_with_default_application ();
	gtk_menu_shell_append (file_submenu, amtk_factory_create_menu_item (factory, "win.open"));
	gtk_menu_shell_append (file_submenu, amtk_application_window_create_open_recent_menu_item (amtk_window));
	gtk_menu_shell_append (file_submenu, gtk_separator_menu_item_new ());
	gtk_menu_shell_append (file_submenu, amtk_factory_create_menu_item (factory, "win.save"));
	gtk_menu_shell_append (file_submenu, amtk_factory_create_menu_item (factory, "win.save-as"));
	gtk_menu_shell_append (file_submenu, gtk_separator_menu_item_new ());
	gtk_menu_shell_append (file_submenu, amtk_factory_create_menu_item (factory, "app.quit"));
	g_object_unref (factory);

	return GTK_WIDGET (file_submenu);
}

static GtkWidget *
create_help_submenu (void)
{
	GtkMenuShell *help_submenu;
	AmtkFactory *factory;

	help_submenu = GTK_MENU_SHELL (gtk_menu_new ());

	factory = amtk_factory_new_with_default_application ();
	gtk_menu_shell_append (help_submenu, amtk_factory_create_menu_item (factory, "app.about"));
	g_object_unref (factory);

	return GTK_WIDGET (help_submenu);
}

static GtkMenuBar *
create_menu_bar (GcsvWindow *window)
{
	GtkWidget *file_menu_item;
	GtkWidget *help_menu_item;
	GtkMenuBar *menu_bar;
	TeplApplication *app;
	AmtkActionInfoStore *store;

	file_menu_item = gtk_menu_item_new_with_mnemonic (_("_File"));
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_menu_item),
				   create_file_submenu (window));

	help_menu_item = gtk_menu_item_new_with_mnemonic (_("_Help"));
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (help_menu_item),
				   create_help_submenu ());

	menu_bar = GTK_MENU_BAR (gtk_menu_bar_new ());
	gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), file_menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), help_menu_item);

	app = tepl_application_get_default ();
	store = tepl_application_get_app_action_info_store (app);
	amtk_action_info_store_check_all_used (store);

	return menu_bar;
}

static void
gcsv_window_init (GcsvWindow *window)
{
	AmtkApplicationWindow *amtk_window;
	GtkWidget *vgrid;
	GtkMenuBar *menu_bar;
	GtkWidget *scrolled_window;
	GtkWidget *statusbar;
	GcsvBuffer *buffer;

	amtk_window = amtk_application_window_get_from_gtk_application_window (GTK_APPLICATION_WINDOW (window));

	window->view = create_view ();
	buffer = get_buffer (window);

	gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);

	vgrid = gtk_grid_new ();
	gtk_orientable_set_orientation (GTK_ORIENTABLE (vgrid), GTK_ORIENTATION_VERTICAL);

	menu_bar = create_menu_bar (window);
	gtk_container_add (GTK_CONTAINER (vgrid), GTK_WIDGET (menu_bar));

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

	/* Connect menubar to statusbar */
	amtk_application_window_set_statusbar (amtk_window, GTK_STATUSBAR (statusbar));
	amtk_application_window_connect_menu_to_statusbar (amtk_window,
							   GTK_MENU_SHELL (menu_bar));

	gtk_container_add (GTK_CONTAINER (window), vgrid);
	gtk_widget_grab_focus (GTK_WIDGET (window->view));

	window->align = gcsv_alignment_new (buffer);

	add_actions (window);
	update_title (window);
	update_statusbar_label (window);

	g_signal_connect_object (buffer,
				 "notify::tepl-full-title",
				 G_CALLBACK (buffer_full_title_notify_cb),
				 window,
				 0);

	g_signal_connect_object (buffer,
				 "modified-changed",
				 G_CALLBACK (buffer_modified_changed_cb),
				 window,
				 0);

	g_signal_connect_object (buffer,
				 "tepl-cursor-moved",
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
finish_file_loading (GcsvWindow *window)
{
	gcsv_buffer_setup_state (get_buffer (window));
	gcsv_alignment_set_enabled (window->align, TRUE);
}

static void
load_metadata_cb (GObject      *source_object,
		  GAsyncResult *result,
		  gpointer      user_data)
{
	TeplFileMetadata *metadata = TEPL_FILE_METADATA (source_object);
	GcsvWindow *window = GCSV_WINDOW (user_data);
	GError *error = NULL;

	tepl_file_metadata_load_finish (metadata, result, &error);

	if (error != NULL)
	{
		g_warning ("Loading metadata failed: %s", error->message);
		g_clear_error (&error);
	}

	finish_file_loading (window);

	g_object_unref (window);
}

static void
load_file_content_cb (TeplFileLoader *loader,
		      GAsyncResult   *result,
		      GcsvWindow     *window)
{
	GcsvBuffer *buffer;
	GError *error = NULL;

	buffer = get_buffer (window);

	if (tepl_file_loader_load_finish (loader, result, &error))
	{
		TeplFile *file;
		TeplFileMetadata *metadata;

		file = tepl_buffer_get_file (TEPL_BUFFER (buffer));
		tepl_file_add_uri_to_recent_manager (file);

		metadata = tepl_file_get_file_metadata (file);

		tepl_file_metadata_load_async (metadata,
					       G_PRIORITY_DEFAULT,
					       NULL,
					       load_metadata_cb,
					       g_object_ref (window));
	}
	else
	{
		finish_file_loading (window);
	}

	if (error != NULL)
	{
		gcsv_warning (GTK_WINDOW (window),
			      _("Error when loading file: %s"),
			      error->message);

		g_clear_error (&error);
	}

	g_object_unref (loader);
	g_object_unref (window);
}

void
gcsv_window_load_file (GcsvWindow *window,
		       GFile      *location)
{
	GcsvBuffer *buffer;
	TeplFile *file;
	TeplFileLoader *loader;

	g_return_if_fail (GCSV_IS_WINDOW (window));
	g_return_if_fail (G_IS_FILE (location));

	buffer = get_buffer (window);
	file = get_file (window);

	tepl_file_set_location (file, location);

	loader = tepl_file_loader_new (TEPL_BUFFER (buffer), file);

	gcsv_alignment_set_enabled (window->align, FALSE);

	tepl_file_loader_load_async (loader,
				     G_PRIORITY_DEFAULT,
				     NULL, /* cancellable */
				     NULL, NULL, NULL, /* progress */
				     (GAsyncReadyCallback) load_file_content_cb,
				     g_object_ref (window));
}

gboolean
gcsv_window_is_untouched (GcsvWindow *window)
{
	g_return_val_if_fail (GCSV_IS_WINDOW (window), FALSE);

	return tepl_buffer_is_untouched (TEPL_BUFFER (get_buffer (window)));
}
