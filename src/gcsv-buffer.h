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

#ifndef __GCSV_BUFFER_H__
#define __GCSV_BUFFER_H__

#include <gtksourceview/gtksource.h>

G_BEGIN_DECLS

#define GCSV_TYPE_BUFFER (gcsv_buffer_get_type ())
G_DECLARE_FINAL_TYPE (GcsvBuffer, gcsv_buffer,
		      GCSV, BUFFER,
		      GtkSourceBuffer)

GcsvBuffer *		gcsv_buffer_new				(void);

GtkSourceFile *		gcsv_buffer_get_file			(GcsvBuffer *buffer);

gboolean		gcsv_buffer_is_untouched		(GcsvBuffer *buffer);

const gchar *		gcsv_buffer_get_short_name		(GcsvBuffer *buffer);

gchar *			gcsv_buffer_get_document_title		(GcsvBuffer *buffer);

void			gcsv_buffer_add_uri_to_recent_manager	(GcsvBuffer *buffer);

gunichar		gcsv_buffer_get_delimiter		(GcsvBuffer *buffer);

void			gcsv_buffer_set_delimiter		(GcsvBuffer *buffer,
								 gunichar    delimiter);

void			gcsv_buffer_get_column_titles_location	(GcsvBuffer  *buffer,
								 GtkTextIter *iter);

void			gcsv_buffer_set_column_titles_line	(GcsvBuffer *buffer,
								 guint       line);

guint			gcsv_buffer_get_column_num		(GcsvBuffer        *buffer,
								 const GtkTextIter *iter);

guint			gcsv_buffer_count_columns_at_line	(GcsvBuffer *buffer,
								 guint       at_line);

void			gcsv_buffer_get_field_bounds		(GcsvBuffer  *buffer,
								 guint        line_num,
								 guint        column_num,
								 GtkTextIter *start,
								 GtkTextIter *end);

void			gcsv_buffer_guess_delimiter		(GcsvBuffer *buffer);

G_END_DECLS

#endif /* __GCSV_BUFFER_H__ */
