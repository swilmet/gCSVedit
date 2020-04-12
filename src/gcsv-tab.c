/*
 * This file is part of gCSVedit.
 *
 * Copyright 2020 - Sébastien Wilmet <swilmet@gnome.org>
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
 */

#include "gcsv-tab.h"
#include <glib/gi18n.h>
#include "gcsv-buffer.h"
#include "gcsv-properties-chooser.h"

struct _GcsvTabPrivate
{
	GcsvAlignment *align;
};

G_DEFINE_TYPE_WITH_PRIVATE (GcsvTab, gcsv_tab, TEPL_TYPE_TAB)

static TeplView *
create_view (void)
{
	GcsvBuffer *buffer;
	GtkSourceView *view;
	GtkSourceSpaceDrawer *space_drawer;

	buffer = gcsv_buffer_new ();
	view = GTK_SOURCE_VIEW (tepl_view_new_with_buffer (GTK_SOURCE_BUFFER (buffer)));
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

	return TEPL_VIEW (view);
}

static void
gcsv_tab_constructed (GObject *object)
{
	GcsvTab *tab = GCSV_TAB (object);
	GcsvBuffer *buffer;
	GcsvPropertiesChooser *properties_chooser;

	G_OBJECT_CLASS (gcsv_tab_parent_class)->constructed (object);

	buffer = GCSV_BUFFER (tepl_tab_get_buffer (TEPL_TAB (tab)));
	properties_chooser = gcsv_properties_chooser_new (buffer);
	gtk_grid_insert_row (GTK_GRID (tab), 0);
	gtk_grid_attach (GTK_GRID (tab), GTK_WIDGET (properties_chooser), 0, 0, 1, 1);

	tab->priv->align = gcsv_alignment_new (buffer);
}

static void
gcsv_tab_dispose (GObject *object)
{
	GcsvTab *tab = GCSV_TAB (object);

	g_clear_object (&tab->priv->align);

	G_OBJECT_CLASS (gcsv_tab_parent_class)->dispose (object);
}

static void
gcsv_tab_class_init (GcsvTabClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->constructed = gcsv_tab_constructed;
	object_class->dispose = gcsv_tab_dispose;
}

static void
gcsv_tab_init (GcsvTab *tab)
{
	tab->priv = gcsv_tab_get_instance_private (tab);
}

GcsvTab *
gcsv_tab_new (void)
{
	return g_object_new (GCSV_TYPE_TAB,
			     "view", create_view (),
			     NULL);
}

static void
finish_file_loading (GcsvTab *tab)
{
	GcsvBuffer *buffer;

	buffer = GCSV_BUFFER (tepl_tab_get_buffer (TEPL_TAB (tab)));
	gcsv_buffer_setup_state (buffer);

	gcsv_alignment_set_enabled (tab->priv->align, TRUE);
}

static void
load_metadata_cb (GObject      *source_object,
		  GAsyncResult *result,
		  gpointer      user_data)
{
	TeplFileMetadata *metadata = TEPL_FILE_METADATA (source_object);
	GcsvTab *tab = GCSV_TAB (user_data);
	GError *error = NULL;

	tepl_file_metadata_load_finish (metadata, result, &error);

	if (error != NULL)
	{
		g_warning ("Loading metadata failed: %s", error->message);
		g_clear_error (&error);
	}

	finish_file_loading (tab);

	g_object_unref (tab);
}

static void
load_file_content_cb (GObject      *source_object,
		      GAsyncResult *result,
		      gpointer      user_data)
{
	TeplFileLoader *loader = TEPL_FILE_LOADER (source_object);
	GcsvTab *tab = GCSV_TAB (user_data);
	GcsvBuffer *buffer;
	GError *error = NULL;

	buffer = GCSV_BUFFER (tepl_tab_get_buffer (TEPL_TAB (tab)));

	if (tepl_file_loader_load_finish (loader, result, &error))
	{
		TeplFile *file;

		file = tepl_buffer_get_file (TEPL_BUFFER (buffer));
		tepl_file_add_uri_to_recent_manager (file);

		tepl_file_metadata_load_async (gcsv_buffer_get_metadata (buffer),
					       tepl_file_get_location (file),
					       G_PRIORITY_DEFAULT,
					       NULL,
					       load_metadata_cb,
					       g_object_ref (tab));
	}
	else
	{
		finish_file_loading (tab);
	}

	if (error != NULL)
	{
		TeplInfoBar *info_bar;

		info_bar = tepl_info_bar_new_simple (GTK_MESSAGE_ERROR,
						     _("Error when loading file:"),
						     error->message);
		tepl_info_bar_add_close_button (info_bar);

		tepl_tab_add_info_bar (TEPL_TAB (tab), GTK_INFO_BAR (info_bar));
		gtk_widget_show (GTK_WIDGET (info_bar));

		g_clear_error (&error);
	}

	g_object_unref (loader);
	g_object_unref (tab);
}

void
gcsv_tab_load_file (GcsvTab *tab,
		    GFile   *location)
{
	TeplBuffer *buffer;
	TeplFile *file;
	TeplFileLoader *loader;

	g_return_if_fail (GCSV_IS_TAB (tab));
	g_return_if_fail (G_IS_FILE (location));

	buffer = tepl_tab_get_buffer (TEPL_TAB (tab));
	file = tepl_buffer_get_file (buffer);

	tepl_file_set_location (file, location);

	loader = tepl_file_loader_new (buffer, file);

	gcsv_alignment_set_enabled (tab->priv->align, FALSE);

	tepl_file_loader_load_async (loader,
				     G_PRIORITY_DEFAULT,
				     NULL, /* cancellable */
				     NULL, NULL, NULL, /* progress */
				     load_file_content_cb,
				     g_object_ref (tab));
}

static void
save_cb (GObject      *source_object,
	 GAsyncResult *result,
	 gpointer      user_data)
{
	TeplFileSaver *saver = TEPL_FILE_SAVER (source_object);
	GcsvTab *tab = GCSV_TAB (user_data);
	TeplBuffer *buffer_without_align;
	GApplication *app = g_application_get_default ();
	GError *error = NULL;

	if (tepl_file_saver_save_finish (saver, result, &error))
	{
		TeplBuffer *buffer;
		TeplFile *file;

		buffer = tepl_tab_get_buffer (TEPL_TAB (tab));
		gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (buffer), FALSE);

		file = tepl_buffer_get_file (buffer);
		tepl_file_add_uri_to_recent_manager (file);

		/* TODO save metadata (async). */
	}

	if (error != NULL)
	{
		TeplInfoBar *info_bar;

		info_bar = tepl_info_bar_new_simple (GTK_MESSAGE_ERROR,
						     _("Error when saving the file:"),
						     error->message);
		tepl_info_bar_add_close_button (info_bar);

		tepl_tab_add_info_bar (TEPL_TAB (tab), GTK_INFO_BAR (info_bar));
		gtk_widget_show (GTK_WIDGET (info_bar));

		g_clear_error (&error);
	}

	buffer_without_align = tepl_file_saver_get_buffer (saver);
	g_object_unref (buffer_without_align);
	g_object_unref (saver);

	g_application_unmark_busy (app);
	g_application_release (app);

	g_object_unref (tab);
}

static void
launch_saver (GcsvTab       *tab,
	      TeplFileSaver *saver)
{
	GApplication *app = g_application_get_default ();

	g_application_hold (app);
	g_application_mark_busy (app);

	tepl_file_saver_save_async (saver,
				    G_PRIORITY_DEFAULT,
				    NULL, /* Cancellable */
				    NULL, NULL, NULL, /* Progress */
				    save_cb,
				    g_object_ref (tab));
}

void
gcsv_tab_save (GcsvTab *tab)
{
	TeplBuffer *buffer;
	TeplFile *file;
	GFile *location;
	TeplBuffer *buffer_without_align;
	TeplFileSaver *saver;

	g_return_if_fail (GCSV_IS_TAB (tab));

	buffer = tepl_tab_get_buffer (TEPL_TAB (tab));
	file = tepl_buffer_get_file (buffer);
	location = tepl_file_get_location (file);
	g_return_if_fail (location != NULL);

	buffer_without_align = gcsv_alignment_copy_buffer_without_alignment (tab->priv->align);

	saver = tepl_file_saver_new (buffer_without_align, file);
	launch_saver (tab, saver);
}

void
gcsv_tab_save_as (GcsvTab *tab,
		  GFile   *target_location)
{
	TeplBuffer *buffer;
	TeplFile *file;
	TeplBuffer *buffer_without_align;
	TeplFileSaver *saver;

	g_return_if_fail (GCSV_IS_TAB (tab));
	g_return_if_fail (G_IS_FILE (target_location));

	buffer = tepl_tab_get_buffer (TEPL_TAB (tab));
	file = tepl_buffer_get_file (buffer);

	buffer_without_align = gcsv_alignment_copy_buffer_without_alignment (tab->priv->align);

	saver = tepl_file_saver_new_with_target (buffer_without_align, file, target_location);
	launch_saver (tab, saver);
}
