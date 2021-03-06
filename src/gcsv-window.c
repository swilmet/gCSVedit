/*
 * This file is part of gCSVedit.
 *
 * Copyright 2015-2016 - Université Catholique de Louvain
 * Copyright 2017-2020 - Sébastien Wilmet <swilmet@gnome.org>
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
#include <tepl/tepl.h>
#include <glib/gi18n.h>
#include "gcsv-alignment.h"
#include "gcsv-buffer.h"
#include "gcsv-tab.h"

struct _GcsvWindow
{
	GtkApplicationWindow parent;

	GtkLabel *statusbar_label;
};

G_DEFINE_TYPE (GcsvWindow, gcsv_window, GTK_TYPE_APPLICATION_WINDOW)

static GcsvTab *
get_tab (GcsvWindow *window)
{
	TeplApplicationWindow *tepl_window;

	tepl_window = tepl_application_window_get_from_gtk_application_window (GTK_APPLICATION_WINDOW (window));
	return GCSV_TAB (tepl_tab_group_get_active_tab (TEPL_TAB_GROUP (tepl_window)));
}

static GcsvBuffer *
get_buffer (GcsvWindow *window)
{
	TeplApplicationWindow *tepl_window;

	tepl_window = tepl_application_window_get_from_gtk_application_window (GTK_APPLICATION_WINDOW (window));
	return GCSV_BUFFER (tepl_tab_group_get_active_buffer (TEPL_TAB_GROUP (tepl_window)));
}

static TeplFile *
get_file (GcsvWindow *window)
{
	GcsvBuffer *buffer = get_buffer (window);
	return tepl_buffer_get_file (TEPL_BUFFER (buffer));
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
	 * doesn't have the real path to the file, so it would ruin some
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
save_activate_cb (GSimpleAction *save_action,
		  GVariant      *parameter,
		  gpointer       user_data)
{
	GcsvWindow *window = GCSV_WINDOW (user_data);
	gcsv_tab_save (get_tab (window));
}

static void
save_file_chooser_response_cb (GtkFileChooserDialog *file_chooser_dialog,
			       gint                  response_id,
			       GcsvWindow           *window)
{
	if (response_id == GTK_RESPONSE_ACCEPT)
	{
		GFile *location;

		location = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (file_chooser_dialog));
		gcsv_tab_save_as (get_tab (window), location);
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
buffer_notify_delimiter_cb (GcsvBuffer *buffer,
			    GParamSpec *pspec,
			    GcsvWindow *window)
{
	update_statusbar_label (window);
}

static void
delete_event__window_close_cb (GObject      *source_object,
			       GAsyncResult *result,
			       gpointer      user_data)
{
	GcsvWindow *window = GCSV_WINDOW (source_object);

	if (gcsv_window_close_finish (window, result))
	{
		gtk_widget_destroy (GTK_WIDGET (window));
	}
}

static gboolean
gcsv_window_delete_event (GtkWidget   *widget,
			  GdkEventAny *event)
{
	GcsvWindow *window = GCSV_WINDOW (widget);

	gcsv_window_close_async (window, delete_event__window_close_cb, NULL);

	return GDK_EVENT_STOP;
}

static void
gcsv_window_class_init (GcsvWindowClass *klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

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
	gtk_menu_shell_append (file_submenu, amtk_factory_create_menu_item (factory, "app.tepl-new-window"));
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
create_edit_submenu (void)
{
	GtkMenuShell *edit_submenu;

	edit_submenu = GTK_MENU_SHELL (gtk_menu_new ());
	tepl_menu_shell_append_edit_actions (edit_submenu);

	return GTK_WIDGET (edit_submenu);
}

static GtkWidget *
create_search_submenu (void)
{
	GtkMenuShell *search_submenu;
	AmtkFactory *factory;

	search_submenu = GTK_MENU_SHELL (gtk_menu_new ());

	factory = amtk_factory_new_with_default_application ();
	gtk_menu_shell_append (search_submenu, amtk_factory_create_menu_item (factory, "win.tepl-goto-line"));
	g_object_unref (factory);

	return GTK_WIDGET (search_submenu);
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
	GtkWidget *edit_menu_item;
	GtkWidget *search_menu_item;
	GtkWidget *help_menu_item;
	GtkMenuBar *menu_bar;
	TeplApplication *app;
	AmtkActionInfoStore *store;

	file_menu_item = gtk_menu_item_new_with_mnemonic (_("_File"));
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (file_menu_item),
				   create_file_submenu (window));

	edit_menu_item = gtk_menu_item_new_with_mnemonic (_("_Edit"));
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (edit_menu_item),
				   create_edit_submenu ());

	search_menu_item = gtk_menu_item_new_with_mnemonic (_("_Search"));
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (search_menu_item),
				   create_search_submenu ());

	help_menu_item = gtk_menu_item_new_with_mnemonic (_("_Help"));
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (help_menu_item),
				   create_help_submenu ());

	menu_bar = GTK_MENU_BAR (gtk_menu_bar_new ());
	gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), file_menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), edit_menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), search_menu_item);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), help_menu_item);

	app = tepl_application_get_default ();
	store = tepl_application_get_app_action_info_store (app);
	amtk_action_info_store_check_all_used (store);

	return menu_bar;
}

static GtkWidget *
create_toolbar (GcsvWindow *window)
{
	GtkToolbar *toolbar;
	AmtkFactory *factory;
	AmtkApplicationWindow *amtk_window;
	GtkMenuToolButton *open_tool_button;

	toolbar = GTK_TOOLBAR (gtk_toolbar_new ());
	factory = amtk_factory_new (NULL);

	open_tool_button = amtk_factory_create_menu_tool_button (factory, "win.open");
	gtk_menu_tool_button_set_arrow_tooltip_text (open_tool_button,
						     _("Open a file recently used with gCSVedit"));
	amtk_window = amtk_application_window_get_from_gtk_application_window (GTK_APPLICATION_WINDOW (window));
	gtk_menu_tool_button_set_menu (open_tool_button,
				       amtk_application_window_create_open_recent_menu (amtk_window));

	gtk_toolbar_insert (toolbar, GTK_TOOL_ITEM (open_tool_button), -1);
	gtk_toolbar_insert (toolbar, amtk_factory_create_tool_button (factory, "win.save"), -1);
	gtk_toolbar_insert (toolbar, amtk_factory_create_tool_button (factory, "win.save-as"), -1);
	gtk_toolbar_insert (toolbar, gtk_separator_tool_item_new (), -1);
	gtk_toolbar_insert (toolbar, amtk_factory_create_tool_button (factory, "win.tepl-cut"), -1);
	gtk_toolbar_insert (toolbar, amtk_factory_create_tool_button (factory, "win.tepl-copy"), -1);
	gtk_toolbar_insert (toolbar, amtk_factory_create_tool_button (factory, "win.tepl-paste"), -1);

	g_object_unref (factory);

	return GTK_WIDGET (toolbar);
}

static void
gcsv_window_init (GcsvWindow *window)
{
	AmtkApplicationWindow *amtk_window;
	TeplApplicationWindow *tepl_window;
	GtkWidget *vgrid;
	GtkMenuBar *menu_bar;
	TeplStatusbar *statusbar;
	GcsvTab *tab;
	TeplView *view;
	GcsvBuffer *buffer;

	amtk_window = amtk_application_window_get_from_gtk_application_window (GTK_APPLICATION_WINDOW (window));
	tepl_window = tepl_application_window_get_from_gtk_application_window (GTK_APPLICATION_WINDOW (window));

	gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);

	vgrid = gtk_grid_new ();
	gtk_orientable_set_orientation (GTK_ORIENTABLE (vgrid), GTK_ORIENTATION_VERTICAL);

	menu_bar = create_menu_bar (window);
	gtk_container_add (GTK_CONTAINER (vgrid), GTK_WIDGET (menu_bar));
	gtk_container_add (GTK_CONTAINER (vgrid), create_toolbar (window));

	/* TeplTab */
	tab = gcsv_tab_new ();
	gtk_container_add (GTK_CONTAINER (vgrid), GTK_WIDGET (tab));
	tepl_application_window_set_tab_group (tepl_window, TEPL_TAB_GROUP (tab));

	tepl_application_window_set_handle_title (tepl_window, TRUE);

	/* Statusbar */
	statusbar = tepl_statusbar_new ();
	window->statusbar_label = GTK_LABEL (gtk_label_new (NULL));
	gtk_box_pack_end (GTK_BOX (statusbar),
			  GTK_WIDGET (window->statusbar_label),
			  FALSE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (vgrid), GTK_WIDGET (statusbar));

	/* Connect menubar to statusbar */
	amtk_application_window_set_statusbar (amtk_window, GTK_STATUSBAR (statusbar));
	amtk_application_window_connect_menu_to_statusbar (amtk_window,
							   GTK_MENU_SHELL (menu_bar));

	gtk_container_add (GTK_CONTAINER (window), vgrid);

	view = tepl_tab_get_view (TEPL_TAB (tab));
	gtk_widget_grab_focus (GTK_WIDGET (view));

	add_actions (window);
	update_statusbar_label (window);

	buffer = get_buffer (window);

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

void
gcsv_window_load_file (GcsvWindow *window,
		       GFile      *location)
{
	g_return_if_fail (GCSV_IS_WINDOW (window));
	g_return_if_fail (G_IS_FILE (location));

	gcsv_tab_load_file (get_tab (window), location);
}

static void
launch_close_confirmation_dialog (GTask *task)
{
	GcsvWindow *window = g_task_get_source_object (task);
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
		gcsv_buffer_save_metadata (get_buffer (window));

		g_task_return_boolean (task, TRUE);
		g_object_unref (task);
		return;
	}

	g_task_return_boolean (task, FALSE);
	g_object_unref (task);
}

void
gcsv_window_close_async (GcsvWindow          *window,
			 GAsyncReadyCallback  callback,
			 gpointer             user_data)
{
	GTask *task;
	GcsvBuffer *buffer;

	g_return_if_fail (GCSV_IS_WINDOW (window));

	task = g_task_new (window, NULL, callback, user_data);

	buffer = get_buffer (window);
	if (gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (buffer)))
	{
		launch_close_confirmation_dialog (task);
		return;
	}

	gcsv_buffer_save_metadata (buffer);

	g_task_return_boolean (task, TRUE);
	g_object_unref (task);
}

/* Returns whether the window can be destroyed. */
gboolean
gcsv_window_close_finish (GcsvWindow   *window,
			  GAsyncResult *result)
{
	g_return_val_if_fail (GCSV_IS_WINDOW (window), FALSE);
	g_return_val_if_fail (g_task_is_valid (result, window), FALSE);

	return g_task_propagate_boolean (G_TASK (result), NULL);
}
