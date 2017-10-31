/*
 * This file is part of gCSVedit.
 *
 * Copyright 2017 - Sébastien Wilmet <swilmet@gnome.org>
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

#include "gcsv-factory.h"
#include "gcsv-window.h"

G_DEFINE_TYPE (GcsvFactory, gcsv_factory, TEPL_TYPE_ABSTRACT_FACTORY)

static GtkApplicationWindow *
gcsv_factory_create_main_window (TeplAbstractFactory *factory,
				 GtkApplication      *app)
{
	return GTK_APPLICATION_WINDOW (gcsv_window_new (app));
}

static void
gcsv_factory_class_init (GcsvFactoryClass *klass)
{
	TeplAbstractFactoryClass *factory_class = TEPL_ABSTRACT_FACTORY_CLASS (klass);

	factory_class->create_main_window = gcsv_factory_create_main_window;
}

static void
gcsv_factory_init (GcsvFactory *factory)
{
}

GcsvFactory *
gcsv_factory_new (void)
{
	return g_object_new (GCSV_TYPE_FACTORY, NULL);
}
