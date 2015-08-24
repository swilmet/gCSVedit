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

#include "gcsv-alignment.h"
#include "gcsv-utils.h"

struct _GcsvAlignment
{
	GObject parent;

	GtkTextBuffer *buffer;
	gunichar delimiter;

	/* The alignment is surrounded by a tag, so we know where the alignment
	 * is. It permits to remove it or to not take it into account.
	 */
	GtkTextTag *tag;

	/* Contains the columns lengths as guint's. */
	GArray *columns_lengths;
};

enum
{
	PROP_0,
	PROP_BUFFER,
	PROP_DELIMITER,
};

G_DEFINE_TYPE (GcsvAlignment, gcsv_alignment, G_TYPE_OBJECT)

static guint
get_column_length (GcsvAlignment *align,
		   guint          column_num)
{
	g_return_val_if_fail (column_num < align->columns_lengths->len, 0);

	return g_array_index (align->columns_lengths, guint, column_num);
}

static void
set_buffer (GcsvAlignment *align,
	    GtkTextBuffer *buffer)
{
	g_assert (align->buffer == NULL);
	g_assert (align->tag == NULL);

	align->buffer = g_object_ref (buffer);
	align->tag = gtk_text_buffer_create_tag (buffer, NULL, NULL);

	g_object_notify (G_OBJECT (align), "buffer");

	gcsv_alignment_update (align);
}

static void
gcsv_alignment_get_property (GObject    *object,
			     guint       prop_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
	GcsvAlignment *align = GCSV_ALIGNMENT (object);

	switch (prop_id)
	{
		case PROP_BUFFER:
			g_value_set_object (value, align->buffer);
			break;

		case PROP_DELIMITER:
			g_value_set_uint (value, align->delimiter);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gcsv_alignment_set_property (GObject      *object,
			     guint         prop_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
	GcsvAlignment *align = GCSV_ALIGNMENT (object);

	switch (prop_id)
	{
		case PROP_BUFFER:
			set_buffer (align, g_value_get_object (value));
			break;

		case PROP_DELIMITER:
			gcsv_alignment_set_delimiter (align, g_value_get_uint (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gcsv_alignment_dispose (GObject *object)
{
	GcsvAlignment *align = GCSV_ALIGNMENT (object);

	if (align->buffer != NULL)
	{
		if (align->tag != NULL)
		{
			GtkTextTagTable *table;

			table = gtk_text_buffer_get_tag_table (align->buffer);
			gtk_text_tag_table_remove (table, align->tag);

			align->tag = NULL;
		}

		g_object_unref (align->buffer);
		align->buffer = NULL;
	}

	G_OBJECT_CLASS (gcsv_alignment_parent_class)->dispose (object);
}

static void
gcsv_alignment_class_init (GcsvAlignmentClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->get_property = gcsv_alignment_get_property;
	object_class->set_property = gcsv_alignment_set_property;
	object_class->dispose = gcsv_alignment_dispose;

	g_object_class_install_property (object_class,
					 PROP_BUFFER,
					 g_param_spec_object ("buffer",
							      "Buffer",
							      "",
							      GTK_TYPE_TEXT_BUFFER,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY |
							      G_PARAM_STATIC_STRINGS));

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
gcsv_alignment_init (GcsvAlignment *align)
{
}

GcsvAlignment *
gcsv_alignment_new (GtkTextBuffer *buffer,
		    gunichar       delimiter)
{
	g_return_val_if_fail (GTK_IS_TEXT_BUFFER (buffer), NULL);

	return g_object_new (GCSV_TYPE_ALIGNMENT,
			     "buffer", buffer,
			     "delimiter", delimiter,
			     NULL);
}

gunichar
gcsv_alignment_get_delimiter (GcsvAlignment *align)
{
	g_return_val_if_fail (GCSV_IS_ALIGNMENT (align), '\0');

	return align->delimiter;
}

void
gcsv_alignment_set_delimiter (GcsvAlignment *align,
			      gunichar       delimiter)
{
	g_return_if_fail (GCSV_IS_ALIGNMENT (align));

	if (align->delimiter != delimiter)
	{
		align->delimiter = delimiter;
		g_object_notify (G_OBJECT (align), "delimiter");

		gcsv_alignment_update (align);
	}
}

void
gcsv_alignment_remove_alignment (GcsvAlignment *align)
{
	g_return_if_fail (GCSV_IS_ALIGNMENT (align));

	gcsv_alignment_set_delimiter (align, '\0');
}

static guint
count_columns (GcsvAlignment *align)
{
	guint n_columns = 1;
	gchar *first_line;
	gchar *p;
	GtkTextIter start;
	GtkTextIter end;

	gtk_text_buffer_get_start_iter (align->buffer, &start);
	end = start;
	gtk_text_iter_forward_line (&end);

	first_line = gtk_text_buffer_get_text (align->buffer, &start, &end, TRUE);

	p = first_line;
	while (p != NULL && *p != '\0')
	{
		gunichar ch;

		ch = g_utf8_get_char (p);
		if (ch == align->delimiter)
		{
			n_columns++;
		}

		p = g_utf8_find_next_char (p, NULL);
	}

	g_free (first_line);
	return n_columns;
}

static void
move_iter_to_nth_column (GcsvAlignment *align,
			 GtkTextIter   *iter,
			 guint          nth_column)
{
	guint column_num = 0;

	gtk_text_iter_set_line_offset (iter, 0);

	while (column_num < nth_column &&
	       !gtk_text_iter_is_end (iter))
	{
		gunichar ch;

		ch = gtk_text_iter_get_char (iter);
		if (ch == align->delimiter)
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

static void
get_field_bounds (GcsvAlignment *align,
		  guint          line_num,
		  guint          column_num,
		  GtkTextIter   *start,
		  GtkTextIter   *end)
{
	g_assert (start != NULL);
	g_assert (end != NULL);

	gtk_text_buffer_get_iter_at_line (align->buffer, start, line_num);
	if (gtk_text_iter_is_end (start))
	{
		*end = *start;
		return;
	}

	move_iter_to_nth_column (align, start, column_num);

	*end = *start;
	while (!gtk_text_iter_is_end (end))
	{
		gunichar ch;

		ch = gtk_text_iter_get_char (end);
		if (ch == align->delimiter || ch == '\n' || ch == '\r')
		{
			break;
		}

		gtk_text_iter_forward_char (end);
	}
}

/* Length in characters, not in bytes. */
static guint
get_field_length (const GtkTextIter *field_start,
		  const GtkTextIter *field_end)
{
	g_return_val_if_fail (gtk_text_iter_get_line (field_start) == gtk_text_iter_get_line (field_end), 0);
	g_return_val_if_fail (gtk_text_iter_compare (field_start, field_end) <= 0, 0);

	return gtk_text_iter_get_line_offset (field_end) - gtk_text_iter_get_line_offset (field_start);
}

static guint
compute_max_column_length (GcsvAlignment *align,
			   guint          column_num)
{
	guint n_lines;
	guint line_num;
	guint max_column_length = 0;

	n_lines = gtk_text_buffer_get_line_count (align->buffer);

	for (line_num = 0; line_num < n_lines; line_num++)
	{
		GtkTextIter start;
		GtkTextIter end;
		guint length;

		get_field_bounds (align, line_num, column_num, &start, &end);
		length = get_field_length (&start, &end);

		if (length > max_column_length)
		{
			max_column_length = length;
		}
	}

	return max_column_length;
}

static void
insert_missing_spaces (GcsvAlignment *align,
		       guint          line_num,
		       guint          column_num)
{
	GtkTextIter field_start;
	GtkTextIter field_end;
	guint column_length;
	guint field_length;
	guint n_spaces;
	gchar *alignment;

	column_length = get_column_length (align, column_num);

	get_field_bounds (align, line_num, column_num, &field_start, &field_end);
	field_length = get_field_length (&field_start, &field_end);

	g_return_if_fail (field_length <= column_length);

	if (field_length == column_length)
	{
		return;
	}

	n_spaces = column_length - field_length;
	alignment = g_strnfill (n_spaces, ' ');

	gtk_text_buffer_insert_with_tags (align->buffer,
					  &field_end,
					  alignment, n_spaces,
					  align->tag,
					  NULL);

	g_free (alignment);
}

static void
update_columns_lengths (GcsvAlignment *align)
{
	guint n_columns;
	guint column_num;

	if (align->columns_lengths != NULL)
	{
		g_array_unref (align->columns_lengths);
		align->columns_lengths = NULL;
	}

	n_columns = count_columns (align);
	align->columns_lengths = g_array_sized_new (FALSE, TRUE, sizeof (guint), n_columns);

	for (column_num = 0; column_num < n_columns; column_num++)
	{
		guint max_column_length;

		max_column_length = compute_max_column_length (align, column_num);

		g_array_append_val (align->columns_lengths, max_column_length);
	}
}

void
gcsv_alignment_update (GcsvAlignment *align)
{
	gboolean modified;
	guint n_columns;
	guint column_num;
	guint n_lines;

	g_return_if_fail (GCSV_IS_ALIGNMENT (align));

	modified = gtk_text_buffer_get_modified (align->buffer);

	update_columns_lengths (align);

	gcsv_utils_delete_text_with_tag (align->buffer, align->tag);

	if (align->delimiter == '\0')
	{
		goto end;
	}

	n_columns = align->columns_lengths->len;
	n_lines = gtk_text_buffer_get_line_count (align->buffer);

	for (column_num = 0; column_num < n_columns; column_num++)
	{
		guint line_num;

		for (line_num = 0; line_num < n_lines; line_num++)
		{
			insert_missing_spaces (align, line_num, column_num);
		}
	}

end:
	gtk_text_buffer_set_modified (align->buffer, modified);
}

GtkSourceBuffer *
gcsv_alignment_copy_buffer_without_alignment (GcsvAlignment *align)
{
	GtkTextBuffer *copy;
	GtkTextIter iter;
	GtkTextIter insert;

	copy = GTK_TEXT_BUFFER (gtk_source_buffer_new (NULL));

	gtk_text_buffer_get_start_iter (align->buffer, &iter);
	gtk_text_buffer_get_start_iter (copy, &insert);

	while (!gtk_text_iter_is_end (&iter))
	{
		GtkTextIter start;
		GtkTextIter end;
		gchar *text;

		start = iter;
		if (gtk_text_iter_begins_tag (&start, align->tag))
		{
			gtk_text_iter_forward_to_tag_toggle (&start, align->tag);
			g_assert (gtk_text_iter_ends_tag (&start, align->tag));
		}

		g_assert (!gtk_text_iter_has_tag (&start, align->tag));

		if (gtk_text_iter_is_end (&start))
		{
			break;
		}

		end = start;
		if (gtk_text_iter_forward_to_tag_toggle (&end, align->tag))
		{
			g_assert (gtk_text_iter_begins_tag (&end, align->tag));
		}
		else
		{
			g_assert (gtk_text_iter_is_end (&end));
		}

		text = gtk_text_buffer_get_text (align->buffer, &start, &end, TRUE);
		gtk_text_buffer_insert (copy, &insert, text, -1);
		g_free (text);

		iter = end;
	}

	return GTK_SOURCE_BUFFER (copy);
}
