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
 * Author: Sébastien Wilmet <swilmet@gnome.org>
 */

#ifndef GCSV_PROPERTIES_CHOOSER_H
#define GCSV_PROPERTIES_CHOOSER_H

#include <gtk/gtk.h>
#include "gcsv-buffer.h"

G_BEGIN_DECLS

#define GCSV_TYPE_PROPERTIES_CHOOSER (gcsv_properties_chooser_get_type ())
G_DECLARE_FINAL_TYPE (GcsvPropertiesChooser, gcsv_properties_chooser,
		      GCSV, PROPERTIES_CHOOSER,
		      GtkGrid)

GcsvPropertiesChooser *	gcsv_properties_chooser_new		(GcsvBuffer *buffer);

G_END_DECLS

#endif /* GCSV_PROPERTIES_CHOOSER_H */
