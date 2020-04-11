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
#include "gcsv-buffer.h"

struct _GcsvTabPrivate
{
	gint something;
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
gcsv_tab_finalize (GObject *object)
{

	G_OBJECT_CLASS (gcsv_tab_parent_class)->finalize (object);
}

static void
gcsv_tab_class_init (GcsvTabClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gcsv_tab_finalize;
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
