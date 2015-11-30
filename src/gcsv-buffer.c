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

#include "gcsv-buffer.h"

struct _GcsvBuffer
{
	GtkSourceBuffer parent;

	/* The delimiter is at most one Unicode character. If it is the nul
	 * character ('\0'), there is no alignment.
	 */
	gunichar delimiter;

	GtkTextMark *title_mark;
};

enum
{
	PROP_0,
	PROP_DELIMITER,
};

G_DEFINE_TYPE (GcsvBuffer, gcsv_buffer, GTK_SOURCE_TYPE_BUFFER)

static void
gcsv_buffer_get_property (GObject    *object,
			  guint       prop_id,
			  GValue     *value,
			  GParamSpec *pspec)
{
	GcsvBuffer *buffer = GCSV_BUFFER (object);

	switch (prop_id)
	{
		case PROP_DELIMITER:
			g_value_set_uint (value, gcsv_buffer_get_delimiter (buffer));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gcsv_buffer_set_property (GObject      *object,
			  guint         prop_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
	GcsvBuffer *buffer = GCSV_BUFFER (object);

	switch (prop_id)
	{
		case PROP_DELIMITER:
			gcsv_buffer_set_delimiter (buffer, g_value_get_uint (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gcsv_buffer_constructed (GObject *object)
{
	GcsvBuffer *buffer = GCSV_BUFFER (object);
	GtkTextIter start;

	G_OBJECT_CLASS (gcsv_buffer_parent_class)->constructed (object);

	gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (buffer), &start);

	buffer->title_mark = gtk_text_buffer_create_mark (GTK_TEXT_BUFFER (buffer),
							  NULL,
							  &start,
							  FALSE);
}

static void
gcsv_buffer_class_init (GcsvBufferClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->get_property = gcsv_buffer_get_property;
	object_class->set_property = gcsv_buffer_set_property;
	object_class->constructed = gcsv_buffer_constructed;

	g_object_class_install_property (object_class,
					 PROP_DELIMITER,
					 g_param_spec_unichar ("delimiter",
							       "Delimiter",
							       "",
							       ',',
							       G_PARAM_READWRITE |
							       G_PARAM_CONSTRUCT |
							       G_PARAM_STATIC_STRINGS));
}

static void
gcsv_buffer_init (GcsvBuffer *buffer)
{
}

GcsvBuffer *
gcsv_buffer_new (void)
{
	return g_object_new (GCSV_TYPE_BUFFER, NULL);
}

gunichar
gcsv_buffer_get_delimiter (GcsvBuffer *buffer)
{
	g_return_val_if_fail (GCSV_IS_BUFFER (buffer), '\0');

	return buffer->delimiter;
}

void
gcsv_buffer_set_delimiter (GcsvBuffer *buffer,
			   gunichar    delimiter)
{
	g_return_if_fail (GCSV_IS_BUFFER (buffer));

	if (buffer->delimiter != delimiter)
	{
		buffer->delimiter = delimiter;
		g_object_notify (G_OBJECT (buffer), "delimiter");
	}
}

/**
 * gcsv_buffer_get_title_mark:
 * @buffer: a #GcsvBuffer.
 *
 * Returns: (transfer none): the #GtkTextMark located at the start of the DSV
 * column titles.
 */
GtkTextMark *
gcsv_buffer_get_title_mark (GcsvBuffer *buffer)
{
	g_return_val_if_fail (GCSV_IS_BUFFER (buffer), NULL);

	return buffer->title_mark;
}

guint
gcsv_buffer_get_column_num (GcsvBuffer        *buffer,
			    const GtkTextIter *iter)
{
	GtkTextIter start_line;
	guint column_num = 0;
	gchar *line;
	gchar *p;

	g_return_val_if_fail (GCSV_IS_BUFFER (buffer), 0);
	g_return_val_if_fail (iter != NULL, 0);
	g_return_val_if_fail (gtk_text_iter_get_buffer (iter) == GTK_TEXT_BUFFER (buffer), 0);

	if (buffer->delimiter == '\0')
	{
		return 0;
	}

	start_line = *iter;
	gtk_text_iter_set_line_offset (&start_line, 0);

	line = gtk_text_buffer_get_text (GTK_TEXT_BUFFER (buffer),
					 &start_line,
					 iter,
					 TRUE);

	p = line;
	while (p != NULL && *p != '\0')
	{
		gunichar ch;

		ch = g_utf8_get_char (p);
		if (ch == buffer->delimiter)
		{
			column_num++;
		}

		p = g_utf8_find_next_char (p, NULL);
	}

	g_free (line);
	return column_num;
}

guint
gcsv_buffer_count_columns_at_line (GcsvBuffer *buffer,
				   guint       at_line)
{
	GtkTextIter iter;

	g_return_val_if_fail (GCSV_IS_BUFFER (buffer), 1);

	gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (buffer), &iter, at_line);

	if (!gtk_text_iter_ends_line (&iter))
	{
		gtk_text_iter_forward_to_line_end (&iter);
	}

	return gcsv_buffer_get_column_num (buffer, &iter) + 1;
}

static void
move_iter_to_nth_column (GcsvBuffer  *buffer,
			 GtkTextIter *iter,
			 guint        nth_column)
{
	guint column_num = 0;

	gtk_text_iter_set_line_offset (iter, 0);

	while (column_num < nth_column &&
	       !gtk_text_iter_is_end (iter))
	{
		gunichar ch;

		ch = gtk_text_iter_get_char (iter);
		if (ch == buffer->delimiter)
		{
			column_num++;
		}
		else if (ch == '\n' || ch == '\r')
		{
			return;
		}

		gtk_text_iter_forward_char (iter);
	}
}

/* Get field bounds, delimiters excluded, virtual spaces included. */
void
gcsv_buffer_get_field_bounds (GcsvBuffer  *buffer,
			      guint        line_num,
			      guint        column_num,
			      GtkTextIter *start,
			      GtkTextIter *end)
{
	g_return_if_fail (GCSV_IS_BUFFER (buffer));
	g_return_if_fail (start != NULL);
	g_return_if_fail (end != NULL);

	gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (buffer), start, line_num);
	if (gtk_text_iter_is_end (start))
	{
		*end = *start;
		return;
	}

	move_iter_to_nth_column (buffer, start, column_num);

	*end = *start;
	while (!gtk_text_iter_is_end (end))
	{
		gunichar ch;

		ch = gtk_text_iter_get_char (end);
		if (ch == buffer->delimiter || ch == '\n' || ch == '\r')
		{
			break;
		}

		gtk_text_iter_forward_char (end);
	}
}

void
gcsv_buffer_guess_delimiter (GcsvBuffer *buffer)
{
	GtkTextIter iter;
	GtkTextIter limit;

	g_return_if_fail (GCSV_IS_BUFFER (buffer));

	gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (buffer), &iter);
	gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (buffer), &limit, 1000);

	/* Really simple guess */
	if (gtk_text_iter_forward_search (&iter,
					  "\t",
					  GTK_TEXT_SEARCH_TEXT_ONLY,
					  NULL,
					  NULL,
					  &limit))
	{
		gcsv_buffer_set_delimiter (buffer, '\t');
	}
	else
	{
		gcsv_buffer_set_delimiter (buffer, ',');
	}
}

/* @line begins at 0. */
void
gcsv_buffer_set_title_line (GcsvBuffer *buffer,
			    guint       line_number)
{
	GtkTextIter iter;

	g_return_if_fail (GCSV_IS_BUFFER (buffer));

	gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (buffer), &iter, line_number);
	gtk_text_buffer_move_mark (GTK_TEXT_BUFFER (buffer), buffer->title_mark, &iter);
}
