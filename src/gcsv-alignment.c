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
#include "gtktextregion.h"

struct _GcsvAlignment
{
	GObject parent;

	GtkTextBuffer *buffer;
	gunichar delimiter;

	/* The alignment is surrounded by a tag, so we know where the alignment
	 * is. It permits to remove it or to not take it into account.
	 */
	GtkTextTag *tag;

	/* Contains the column lengths as gint's. A column length of -1 means no
	 * alignment.
	 */
	GArray *column_lengths;

	/* The remaining region in the GtkTextBuffer to align. */
	GtkTextRegion *region;

	guint idle_id;
};

enum
{
	PROP_0,
	PROP_BUFFER,
	PROP_DELIMITER,
};

/* Max number of lines to align at once. */
#define BATCH_SIZE 100

G_DEFINE_TYPE (GcsvAlignment, gcsv_alignment, G_TYPE_OBJECT)

static gint
get_column_length (GcsvAlignment *align,
		   guint          column_num)
{
	g_return_val_if_fail (column_num < align->column_lengths->len, 0);

	return g_array_index (align->column_lengths, gint, column_num);
}

/* A TextRegion can contain empty subregions. So checking the number of
 * subregions is not sufficient.
 * When calling gtk_text_region_add() with equal iters, the subregion is not
 * added. But when a subregion becomes empty, due to text deletion, the
 * subregion is not removed from the TextRegion.
 */
static gboolean
is_text_region_empty (GtkTextRegion *region)
{
	GtkTextRegionIterator region_iter;

	gtk_text_region_get_iterator (region, &region_iter, 0);

	while (!gtk_text_region_iterator_is_end (&region_iter))
	{
		GtkTextIter region_start;
		GtkTextIter region_end;

		gtk_text_region_iterator_get_subregion (&region_iter,
							&region_start,
							&region_end);

		if (!gtk_text_iter_equal (&region_start, &region_end))
		{
			return FALSE;
		}

		gtk_text_region_iterator_next (&region_iter);
	}

	return TRUE;
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

	if (align->idle_id != 0)
	{
		g_source_remove (align->idle_id);
		align->idle_id = 0;
	}

	if (align->region != NULL)
	{
		gtk_text_region_destroy (align->region);
		align->region = NULL;
	}

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

	if (align->delimiter == '\0')
	{
		return 1;
	}

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

/* Get field bounds, delimiters excluded, virtual spaces included. */
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
get_field_length (GcsvAlignment     *align,
		  const GtkTextIter *field_start,
		  const GtkTextIter *field_end,
		  gboolean           include_virtual_spaces)
{
	GtkTextIter iter;
	guint length = 0;

	g_return_val_if_fail (gtk_text_iter_get_line (field_start) == gtk_text_iter_get_line (field_end), 0);
	g_return_val_if_fail (gtk_text_iter_compare (field_start, field_end) <= 0, 0);

	if (include_virtual_spaces)
	{
		return gtk_text_iter_get_line_offset (field_end) - gtk_text_iter_get_line_offset (field_start);
	}

	for (iter = *field_start;
	     !gtk_text_iter_equal (&iter, field_end);
	     gtk_text_iter_forward_char (&iter))
	{
		if (!gtk_text_iter_has_tag (&iter, align->tag))
		{
			length++;
		}
	}

	return length;
}

static gint
compute_column_length (GcsvAlignment *align,
		       guint          column_num)
{
	guint n_lines;
	guint line_num;
	gint max_length = 0;

	if (align->delimiter == '\0')
	{
		return -1;
	}

	n_lines = gtk_text_buffer_get_line_count (align->buffer);

	for (line_num = 0; line_num < n_lines; line_num++)
	{
		GtkTextIter start;
		GtkTextIter end;
		gint length;

		get_field_bounds (align, line_num, column_num, &start, &end);
		length = get_field_length (align, &start, &end, FALSE);

		if (length > max_length)
		{
			max_length = length;
		}
	}

	return max_length;
}

static void
adjust_missing_spaces (GcsvAlignment *align,
		       guint          line_num,
		       guint          column_num)
{
	GtkTextIter field_start;
	GtkTextIter field_end;
	gint column_length;
	gint field_length;
	guint n_spaces;
	gchar *alignment;

	column_length = get_column_length (align, column_num);

	/* Delete alignment */
	get_field_bounds (align, line_num, column_num, &field_start, &field_end);
	gcsv_utils_delete_text_with_tag (align->buffer, &field_start, &field_end, align->tag);

	if (column_length < 0)
	{
		return;
	}

	/* Insert missing spaces */
	get_field_bounds (align, line_num, column_num, &field_start, &field_end);
	field_length = get_field_length (align, &field_start, &field_end, TRUE);

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
update_column_lengths (GcsvAlignment *align)
{
	guint n_columns;
	guint column_num;

	if (align->column_lengths != NULL)
	{
		g_array_unref (align->column_lengths);
		align->column_lengths = NULL;
	}

	n_columns = count_columns (align);
	align->column_lengths = g_array_sized_new (FALSE, TRUE, sizeof (gint), n_columns);

	for (column_num = 0; column_num < n_columns; column_num++)
	{
		gint column_length = compute_column_length (align, column_num);
		g_array_append_val (align->column_lengths, column_length);
	}
}

static void
align_subregion (GcsvAlignment     *align,
		 const GtkTextIter *start,
		 const GtkTextIter *end)
{
	gulong *handler_ids;
	gboolean modified;
	guint start_line;
	guint end_line;
	guint line_num;

	g_assert (gtk_text_iter_starts_line (start));
	g_assert (gtk_text_iter_ends_line (end));

	handler_ids = gcsv_utils_block_all_signal_handlers (G_OBJECT (align->buffer),
							    "modified-changed");
	modified = gtk_text_buffer_get_modified (align->buffer);

	start_line = gtk_text_iter_get_line (start);
	end_line = gtk_text_iter_get_line (end);

	for (line_num = start_line; line_num <= end_line; line_num++)
	{
		guint n_columns = align->column_lengths->len;
		guint column_num;

		for (column_num = 0; column_num < n_columns; column_num++)
		{
			adjust_missing_spaces (align, line_num, column_num);
		}
	}

	gtk_text_buffer_set_modified (align->buffer, modified);
	gcsv_utils_unblock_signal_handlers (G_OBJECT (align->buffer),
					    handler_ids);
	g_free (handler_ids);
}

static void
adjust_subregion (GtkTextIter *start,
		  GtkTextIter *end)
{
	if (!gtk_text_iter_starts_line (start))
	{
		gtk_text_iter_set_line_offset (start, 0);
	}

	if (!gtk_text_iter_ends_line (end))
	{
		gtk_text_iter_forward_to_line_end (end);
	}
}

static gboolean
idle_cb (GcsvAlignment *align)
{
	guint n_remaining_lines = BATCH_SIZE;
	GtkTextRegionIterator region_iter;
	GtkTextIter start;
	GtkTextIter stop;

	if (align->region == NULL)
	{
		align->idle_id = 0;
		return G_SOURCE_REMOVE;
	}

	gtk_text_buffer_get_start_iter (align->buffer, &stop);

	gtk_text_region_get_iterator (align->region, &region_iter, 0);

	while (n_remaining_lines > 0 &&
	       !gtk_text_region_iterator_is_end (&region_iter))
	{
		GtkTextIter subregion_start;
		GtkTextIter subregion_end;
		guint n_lines;
		guint line_end;

		gtk_text_region_iterator_get_subregion (&region_iter,
							&subregion_start,
							&subregion_end);

		n_lines = gtk_text_iter_get_line (&subregion_end) - gtk_text_iter_get_line (&subregion_start) + 1;

		if (n_lines > n_remaining_lines)
		{
			subregion_end = subregion_start;
			gtk_text_iter_forward_lines (&subregion_end, n_remaining_lines - 1);
			n_lines = n_remaining_lines;
		}

		adjust_subregion (&subregion_start, &subregion_end);

		/* Remember the line, since the iter won't be valid after
		 * aligning the columns.
		 */
		line_end = gtk_text_iter_get_line (&subregion_end);

		align_subregion (align, &subregion_start, &subregion_end);

		gtk_text_buffer_get_iter_at_line (align->buffer, &stop, line_end);
		if (!gtk_text_iter_ends_line (&stop))
		{
			gtk_text_iter_forward_to_line_end (&stop);
		}

		n_remaining_lines -= n_lines;
		gtk_text_region_iterator_next (&region_iter);
	}

	gtk_text_buffer_get_start_iter (align->buffer, &start);
	gtk_text_region_subtract (align->region, &start, &stop);

	if (is_text_region_empty (align->region))
	{
		if (align->region != NULL)
		{
			gtk_text_region_destroy (align->region);
			align->region = NULL;
		}

		align->idle_id = 0;
		return G_SOURCE_REMOVE;
	}

	return G_SOURCE_CONTINUE;
}

static void
install_idle (GcsvAlignment *align)
{
	if (align->idle_id == 0)
	{
		align->idle_id = g_idle_add ((GSourceFunc) idle_cb, align);
	}
}

void
gcsv_alignment_update (GcsvAlignment *align)
{
	GtkTextIter start;
	GtkTextIter end;

	g_return_if_fail (GCSV_IS_ALIGNMENT (align));

	update_column_lengths (align);

	if (align->region == NULL)
	{
		align->region = gtk_text_region_new (align->buffer);
	}

	gtk_text_buffer_get_bounds (align->buffer, &start, &end);
	gtk_text_region_add (align->region, &start, &end);

	install_idle (align);
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
