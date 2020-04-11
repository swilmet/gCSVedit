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

struct _GcsvTabPrivate
{
	gint something;
};

G_DEFINE_TYPE_WITH_PRIVATE (GcsvTab, gcsv_tab, TEPL_TYPE_TAB)

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
	return g_object_new (GCSV_TYPE_TAB, NULL);
}
