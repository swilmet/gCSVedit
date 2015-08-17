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

#ifndef __GCSV_DELIMITER_CHOOSER_H__
#define __GCSV_DELIMITER_CHOOSER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GCSV_TYPE_DELIMITER_CHOOSER (gcsv_delimiter_chooser_get_type ())
G_DECLARE_FINAL_TYPE (GcsvDelimiterChooser, gcsv_delimiter_chooser,
		      GCSV, DELIMITER_CHOOSER,
		      GtkGrid)

GcsvDelimiterChooser *	gcsv_delimiter_chooser_new		(gunichar delimiter);

gunichar		gcsv_delimiter_chooser_get_delimiter	(GcsvDelimiterChooser *chooser);

void			gcsv_delimiter_chooser_set_delimiter	(GcsvDelimiterChooser *chooser,
								 gunichar              delimiter);

G_END_DECLS

#endif /* __GCSV_DELIMITER_CHOOSER_H__ */
