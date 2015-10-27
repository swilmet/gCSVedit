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

#ifndef __GCSV_PROPERTIES_CHOOSER_H__
#define __GCSV_PROPERTIES_CHOOSER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GCSV_TYPE_PROPERTIES_CHOOSER (gcsv_properties_chooser_get_type ())
G_DECLARE_FINAL_TYPE (GcsvPropertiesChooser, gcsv_properties_chooser,
		      GCSV, PROPERTIES_CHOOSER,
		      GtkGrid)

GcsvPropertiesChooser *	gcsv_properties_chooser_new		(gunichar delimiter);

gunichar		gcsv_properties_chooser_get_delimiter	(GcsvPropertiesChooser *chooser);

void			gcsv_properties_chooser_set_delimiter	(GcsvPropertiesChooser *chooser,
								 gunichar               delimiter);

guint			gcsv_properties_chooser_get_title_line	(GcsvPropertiesChooser *chooser);

void			gcsv_properties_chooser_set_title_line	(GcsvPropertiesChooser *chooser,
								 guint                  title_line);

G_END_DECLS

#endif /* __GCSV_PROPERTIES_CHOOSER_H__ */
