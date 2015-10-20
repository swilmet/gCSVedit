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

#include "gcsv-dsv-utils.h"

guint
gcsv_dsv_get_column_num (const GtkTextIter *iter,
			 gunichar           delimiter)
{
	GtkTextBuffer *buffer;
	GtkTextIter start_line;
	guint column_num = 0;
	gchar *line;
	gchar *p;

	g_return_val_if_fail (iter != NULL, 0);

	if (delimiter == '\0')
	{
		return 0;
	}

	start_line = *iter;
	gtk_text_iter_set_line_offset (&start_line, 0);

	buffer = gtk_text_iter_get_buffer (iter);
	line = gtk_text_buffer_get_text (buffer, &start_line, iter, TRUE);

	p = line;
	while (p != NULL && *p != '\0')
	{
		gunichar ch;

		ch = g_utf8_get_char (p);
		if (ch == delimiter)
		{
			column_num++;
		}

		p = g_utf8_find_next_char (p, NULL);
	}

	g_free (line);
	return column_num;
}

guint
gcsv_dsv_count_columns (GtkTextBuffer *buffer,
			guint          at_line,
			gunichar       delimiter)
{
	GtkTextIter iter;

	g_return_val_if_fail (GTK_IS_TEXT_BUFFER (buffer), 1);

	gtk_text_buffer_get_iter_at_line (buffer, &iter, at_line);

	if (!gtk_text_iter_ends_line (&iter))
	{
		gtk_text_iter_forward_to_line_end (&iter);
	}

	return gcsv_dsv_get_column_num (&iter, delimiter) + 1;
}
