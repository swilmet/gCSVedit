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
#include "gcsv-text-region.h"

struct _GcsvAlignment
{
	GObject parent;

	GcsvBuffer *buffer;

	/* The alignment is done by inserting spaces to the buffer. The spaces
	 * are surrounded by a tag, so we know where the alignment is. It
	 * permits to remove it or to not take it into account for certain
	 * features like file saving.
	 */
	GtkTextTag *tag;

	/* Contains the column lengths as gint's. A column length of -1 means no
	 * alignment.
	 */
	GArray *column_lengths;

	/* The remaining region in the GtkTextBuffer to scan, to compute column
	 * lengths. scan_region is always fully handled before align_region.
	 */
	GcsvTextRegion *scan_region;

	/* The region to align, i.e. adjusting the spacing. */
	GcsvTextRegion *align_region;

	/* When adding a subregion to the scan_region or align_region, the
	 * subregion is sometimes not handled directly/synchronously, instead a
	 * timeout or idle function is used, to not block the user interface. An
	 * idle iteration handles just a chunk of the region.
	 * When possible, the next chunk is scanned/aligned synchronously, so
	 * the columns don't shift, like in a spreadsheet. But it is possible
	 * only when the subregion to scan/align is small (e.g. one line).
	 * Using threads would not be convenient because this class uses lots of
	 * GTK+ functions, and the GTK+ API can be accessed only by the main
	 * thread.
	 */
	guint timeout_id;
	guint idle_id;

	gulong delimiter_notify_handler_id;
	gulong insert_text_handler_id;
	gulong delete_range_handler_id;
	gulong delete_range_after_handler_id;

	/* Whether the alignment is enabled. It is different than setting the
	 * delimiter to '\0'. Setting the delimiter to '\0' removes the
	 * alignment. Disabling the GcsvAlignment keep the buffer as-is, and
	 * when it is enabled again the buffer is re-scanned and re-aligned
	 * entirely. It is useful to not loose the delimiter setting. Disabling
	 * the GcsvAlignment can be useful during a file loading, for example.
	 */
	guint enabled : 1;

	/* In unit test mode, timeouts aren't used, because it slows down the
	 * unit tests and it is more difficult to know when an operation is
	 * finished.
	 */
	guint unit_test_mode : 1;

	/* Whether the scan/align regions need to be handled synchronously (just
	 * the next chunk) when a delete-range in the buffer is done.
	 */
	guint sync_after_delete_range : 1;
};

enum
{
	PROP_0,
	PROP_BUFFER,
	PROP_ENABLED,
};

typedef enum
{
	HANDLE_MODE_SYNC, /* Handle directly the next chunk */
	HANDLE_MODE_IDLE,
	HANDLE_MODE_TIMEOUT,
} HandleMode;

typedef struct _BufferEditData BufferEditData;
struct _BufferEditData
{
	gulong *handler_ids;
	guint modified : 1;
};

/* Function to handle a subregion of a GcsvTextRegion.
 * Returns TRUE if the subregion was handled normally, and if the next subregion
 * can be handled. Returns FALSE otherwise, for example if the GcsvTextRegion has
 * been altered so the iteration of the GcsvTextRegion must stop.
 */
typedef gboolean (* HandleSubregionFunc) (GcsvAlignment     *align,
					  const GtkTextIter *start,
					  const GtkTextIter *end);

/* Max number of lines to scan or align at once. Aligning takes normally more
 * time since it needs to delete and insert text, while scanning is just
 * reading.
 */
#define SCANNING_BATCH_SIZE 100
#define ALIGNING_BATCH_SIZE 50

/* Timeout duration in milliseconds.
 * By default in GNOME, the key repeat-interval is 30ms.
 * It would be better to get the value of the
 * org.gnome.desktop.peripherals.keyboard.repeat-interval gsetting.
 */
#define TIMEOUT_DURATION 40

#define ENABLE_DEBUG 0

G_DEFINE_TYPE (GcsvAlignment, gcsv_alignment, G_TYPE_OBJECT)

/* Prototypes */
static void add_subregion_to_align (GcsvAlignment *align,
				    GtkTextIter   *start,
				    GtkTextIter   *end);

static void update_all (GcsvAlignment *align,
			HandleMode     mode);

static void handle_mode (GcsvAlignment *align,
			 HandleMode     mode);

#if ENABLE_DEBUG
static void
print_column_lengths (GcsvAlignment *align)
{
	guint i;

	g_print ("column lengths: ");

	for (i = 0; i < align->column_lengths->len; i++)
	{
		gint len;

		len = g_array_index (align->column_lengths, gint, i);
		g_print ("%d", len);

		if (i < align->column_lengths->len - 1)
		{
			g_print (", ");
		}
	}

	g_print ("\n");
}
#endif

static gint
get_column_length (GcsvAlignment *align,
		   guint          column_num)
{
	g_return_val_if_fail (column_num < align->column_lengths->len, 0);

	return g_array_index (align->column_lengths, gint, column_num);
}

static void
set_column_length (GcsvAlignment *align,
		   guint          column_num,
		   gint           column_length)
{
	GtkTextIter start;
	GtkTextIter end;

	if (column_num >= align->column_lengths->len)
	{
		guint i;

		for (i = align->column_lengths->len; i < column_num; i++)
		{
			gint no_alignment = -1;
			g_array_insert_val (align->column_lengths, i, no_alignment);
		}

		g_array_insert_val (align->column_lengths, column_num, column_length);
	}
	else
	{
		/* Update value */
		g_array_remove_index (align->column_lengths, column_num);
		g_array_insert_val (align->column_lengths, column_num, column_length);
	}

	/* Since the column length is updated, we need to re-align the columns. */
	gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (align->buffer), &start, &end);
	add_subregion_to_align (align, &start, &end);
	handle_mode (align, HANDLE_MODE_IDLE);
}

/* A TextRegion can contain empty subregions. So checking the number of
 * subregions is not sufficient.
 * When calling gcsv_text_region_add() with equal iters, the subregion is not
 * added. But when a subregion becomes empty, due to text deletion, the
 * subregion is not removed from the TextRegion.
 */
static gboolean
is_text_region_empty (GcsvTextRegion *region)
{
	GcsvTextRegionIterator region_iter;

	if (region == NULL)
	{
		return TRUE;
	}

	gcsv_text_region_get_iterator (region, &region_iter, 0);

	while (!gcsv_text_region_iterator_is_end (&region_iter))
	{
		GtkTextIter region_start;
		GtkTextIter region_end;

		gcsv_text_region_iterator_get_subregion (&region_iter,
							 &region_start,
							 &region_end);

		if (!gtk_text_iter_equal (&region_start, &region_end))
		{
			return FALSE;
		}

		gcsv_text_region_iterator_next (&region_iter);
	}

	return TRUE;
}

static BufferEditData
begin_buffer_edit (GcsvAlignment *align)
{
	BufferEditData data;

	data.handler_ids = gcsv_utils_block_all_signal_handlers (G_OBJECT (align->buffer),
								 "modified-changed");
	data.modified = gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (align->buffer));

	if (align->insert_text_handler_id != 0)
	{
		g_signal_handler_block (align->buffer, align->insert_text_handler_id);
	}

	if (align->delete_range_handler_id != 0)
	{
		g_signal_handler_block (align->buffer, align->delete_range_handler_id);
	}

	if (align->delete_range_after_handler_id != 0)
	{
		g_signal_handler_block (align->buffer, align->delete_range_after_handler_id);
	}

	return data;
}

static void
end_buffer_edit (GcsvAlignment  *align,
		 BufferEditData *data)
{
	if (align->insert_text_handler_id != 0)
	{
		g_signal_handler_unblock (align->buffer, align->insert_text_handler_id);
	}

	if (align->delete_range_handler_id != 0)
	{
		g_signal_handler_unblock (align->buffer, align->delete_range_handler_id);
	}

	if (align->delete_range_after_handler_id != 0)
	{
		g_signal_handler_unblock (align->buffer, align->delete_range_after_handler_id);
	}

	gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (align->buffer), data->modified);
	gcsv_utils_unblock_signal_handlers (G_OBJECT (align->buffer),
					    data->handler_ids);
	g_free (data->handler_ids);
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

static gboolean
scan_subregion (GcsvAlignment     *align,
		const GtkTextIter *start,
		const GtkTextIter *end)
{
	gunichar delimiter;
	guint start_line;
	guint end_line;
	guint line_num;

	g_assert (gtk_text_iter_starts_line (start));
	g_assert (gtk_text_iter_ends_line (end));

	delimiter = gcsv_buffer_get_delimiter (align->buffer);

	start_line = gtk_text_iter_get_line (start);
	end_line = gtk_text_iter_get_line (end);

	for (line_num = start_line; line_num <= end_line; line_num++)
	{
		guint n_columns;
		guint column_num;

		n_columns = gcsv_buffer_count_columns_at_line (align->buffer, line_num);

		for (column_num = 0; column_num < n_columns; column_num++)
		{
			gint field_length;
			gint column_length;

			if (delimiter == '\0')
			{
				field_length = -1;
			}
			else
			{
				GtkTextIter field_start;
				GtkTextIter field_end;

				gcsv_buffer_get_field_bounds (align->buffer,
							      line_num,
							      column_num,
							      &field_start,
							      &field_end);

				field_length = get_field_length (align, &field_start, &field_end, FALSE);
			}

			if (column_num >= align->column_lengths->len)
			{
				set_column_length (align, column_num, field_length);
				continue;
			}

			column_length = get_column_length (align, column_num);

			if (field_length > column_length)
			{
				set_column_length (align, column_num, field_length);
			}
		}
	}

	return TRUE;
}

static void
insert_virtual_spaces (GcsvAlignment *align,
		       GtkTextIter   *iter,
		       guint          n_spaces)
{
	GtkTextIter selection_start;
	GtkTextIter selection_end;
	gboolean at_insert;
	gchar *alignment;

	gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (align->buffer),
					      &selection_start,
					      &selection_end);

	at_insert = (gtk_text_iter_equal (&selection_start, iter) &&
		     gtk_text_iter_equal (&selection_end, iter));

	alignment = g_strnfill (n_spaces, ' ');
	gtk_text_buffer_insert_with_tags (GTK_TEXT_BUFFER (align->buffer),
					  iter,
					  alignment, n_spaces,
					  align->tag,
					  NULL);
	g_free (alignment);

	if (at_insert)
	{
		GtkTextIter before_alignment = *iter;
		gtk_text_iter_backward_chars (&before_alignment, n_spaces);
		gtk_text_buffer_select_range (GTK_TEXT_BUFFER (align->buffer),
					      &before_alignment,
					      &before_alignment);
	}
}

/* Returns TRUE if field correctly adjusted. Returns FALSE if the column length
 * has been updated.
 */
static gboolean
adjust_field_alignment (GcsvAlignment *align,
			guint          line_num,
			guint          column_num)
{
	GtkTextIter field_start;
	GtkTextIter field_end;
	gint column_length;
	gint field_length;
	guint n_spaces;

	column_length = get_column_length (align, column_num);

	/* Delete previous alignment */
	gcsv_buffer_get_field_bounds (align->buffer,
				      line_num,
				      column_num,
				      &field_start,
				      &field_end);

	gcsv_utils_delete_text_with_tag (GTK_TEXT_BUFFER (align->buffer),
					 &field_start,
					 &field_end,
					 align->tag);

	if (column_length < 0)
	{
		return TRUE;
	}

	/* Compare field length with column length */
	gcsv_buffer_get_field_bounds (align->buffer,
				      line_num,
				      column_num,
				      &field_start,
				      &field_end);

	field_length = get_field_length (align, &field_start, &field_end, TRUE);

	if (field_length == column_length)
	{
		return TRUE;
	}

	if (field_length > column_length)
	{
		update_all (align, HANDLE_MODE_IDLE);
		return FALSE;
	}

	if (gtk_text_iter_ends_line (&field_end))
	{
		/* Do not insert trailing spaces. */
		return TRUE;
	}

	/* Insert missing spaces */
	n_spaces = column_length - field_length;
	insert_virtual_spaces (align, &field_end, n_spaces);

	return TRUE;
}

/* Returns TRUE if the subregion is correctly aligned, FALSE if a column length
 * has been updated.
 */
static gboolean
align_subregion (GcsvAlignment     *align,
		 const GtkTextIter *start,
		 const GtkTextIter *end)
{
	BufferEditData edit_data;
	guint start_line;
	guint end_line;
	guint line_num;
	gboolean finished = TRUE;

	g_assert (gtk_text_iter_starts_line (start));
	g_assert (gtk_text_iter_ends_line (end));

	edit_data = begin_buffer_edit (align);

	/* Adjust all fields alignment */
	start_line = gtk_text_iter_get_line (start);
	end_line = gtk_text_iter_get_line (end);

	for (line_num = start_line; line_num <= end_line; line_num++)
	{
		guint n_columns = align->column_lengths->len;
		guint column_num;

		for (column_num = 0; column_num < n_columns; column_num++)
		{
			if (!adjust_field_alignment (align, line_num, column_num))
			{
				finished = FALSE;
				goto out;
			}
		}
	}

out:
	end_buffer_edit (align, &edit_data);

	return finished;
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

/* Returns whether the handling of the whole buffer is finished.
 * I.e. it returns TRUE if there is no more chunks.
 */
static gboolean
handle_next_chunk (GcsvAlignment       *align,
		   GcsvTextRegion      *region,
		   guint                batch_size,
		   HandleSubregionFunc  handle_subregion_func)
{
	guint n_remaining_lines = batch_size;
	GcsvTextRegionIterator region_iter;
	GtkTextIter start;
	GtkTextIter stop;

	if (region == NULL)
	{
		return TRUE;
	}

	gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (align->buffer), &stop);

	gcsv_text_region_get_iterator (region, &region_iter, 0);

	while (n_remaining_lines > 0 &&
	       !gcsv_text_region_iterator_is_end (&region_iter))
	{
		GtkTextIter subregion_start;
		GtkTextIter subregion_end;
		guint n_lines;
		gint line_end;

		gcsv_text_region_iterator_get_subregion (&region_iter,
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
		 * handling the subregion. In our case, the line number won't
		 * change, but if lines are inserted or deleted, we would need a
		 * GtkTextMark to remember the location.
		 */
		line_end = gtk_text_iter_get_line (&subregion_end);

		if (!handle_subregion_func (align, &subregion_start, &subregion_end))
		{
			return FALSE;
		}

		/* We need to take the start of the next line, because
		 * line_end _included_ has already been handled.
		 */
		gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (align->buffer),
						  &stop,
						  line_end + 1);

		n_remaining_lines -= n_lines;
		gcsv_text_region_iterator_next (&region_iter);
	}

	gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (align->buffer), &start);
	gcsv_text_region_subtract (region, &start, &stop);

	if (is_text_region_empty (region))
	{
		return TRUE;
	}

	return FALSE;
}

static gboolean
scan_next_chunk (GcsvAlignment *align)
{
	return handle_next_chunk (align,
				  align->scan_region,
				  SCANNING_BATCH_SIZE,
				  scan_subregion);
}

static gboolean
align_next_chunk (GcsvAlignment *align)
{
	return handle_next_chunk (align,
				  align->align_region,
				  ALIGNING_BATCH_SIZE,
				  align_subregion);
}

static gboolean
idle_cb (GcsvAlignment *align)
{
	if (align->scan_region != NULL)
	{
		gboolean finished = scan_next_chunk (align);

#if ENABLE_DEBUG
		if (finished)
		{
			print_column_lengths (align);
		}
#endif

		if (finished && align->scan_region != NULL)
		{
			gcsv_text_region_destroy (align->scan_region);
			align->scan_region = NULL;
		}

		return G_SOURCE_CONTINUE;
	}

	if (align->align_region != NULL)
	{
		gboolean finished = align_next_chunk (align);
		if (finished)
		{
			if (align->align_region != NULL)
			{
				gcsv_text_region_destroy (align->align_region);
				align->align_region = NULL;
			}

			align->idle_id = 0;
			return G_SOURCE_REMOVE;
		}

		return G_SOURCE_CONTINUE;
	}

	align->idle_id = 0;
	return G_SOURCE_REMOVE;
}

static void
install_idle (GcsvAlignment *align)
{
	if (!align->enabled)
	{
		return;
	}

	/* If the idle is installed, the timeout is no longer needed. */
	if (align->timeout_id != 0)
	{
		g_source_remove (align->timeout_id);
		align->timeout_id = 0;
	}

	if (align->idle_id == 0)
	{
		align->idle_id = g_idle_add ((GSourceFunc) idle_cb, align);
	}
}

static gboolean
timeout_cb (GcsvAlignment *align)
{
	install_idle (align);

	align->timeout_id = 0;
	return G_SOURCE_REMOVE;
}

static void
install_timeout (GcsvAlignment *align)
{
	if (!align->enabled)
	{
		return;
	}

	if (align->unit_test_mode)
	{
		install_idle (align);
		return;
	}

	/* Remove the idle, because if we install a timeout it's because we want
	 * to be more responsive. If the idle function is running, we loose the
	 * responsiveness.
	 */
	if (align->idle_id != 0)
	{
		g_source_remove (align->idle_id);
		align->idle_id = 0;
	}

	if (align->timeout_id != 0)
	{
		g_source_remove (align->timeout_id);
	}

	align->timeout_id = g_timeout_add (TIMEOUT_DURATION,
					   (GSourceFunc) timeout_cb,
					   align);
}

/* Returns whether the scan/align is finished. */
static gboolean
sync_scan_and_align (GcsvAlignment *align)
{
	if (align->scan_region != NULL)
	{
		gboolean finished = scan_next_chunk (align);

		if (!finished)
		{
			return FALSE;
		}

		if (align->scan_region != NULL)
		{
			gcsv_text_region_destroy (align->scan_region);
			align->scan_region = NULL;
		}
	}

	if (align->align_region != NULL)
	{
		gboolean finished = align_next_chunk (align);

		if (!finished)
		{
			return FALSE;
		}

		if (align->align_region != NULL)
		{
			gcsv_text_region_destroy (align->align_region);
			align->align_region = NULL;
		}
	}

	return TRUE;
}

static void
handle_mode (GcsvAlignment *align,
	     HandleMode     mode)
{
	switch (mode)
	{
		case HANDLE_MODE_SYNC:
			if (!sync_scan_and_align (align))
			{
				install_timeout (align);
			}
			break;

		case HANDLE_MODE_IDLE:
			install_idle (align);
			break;

		case HANDLE_MODE_TIMEOUT:
			install_timeout (align);
			break;

		default:
			g_assert_not_reached ();
	}
}

static void
remove_header (GcsvAlignment  *align,
	       GcsvTextRegion *region)
{
	GtkTextIter start;
	GtkTextIter header_end;

	gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (align->buffer), &start);
	gcsv_buffer_get_column_titles_location (align->buffer, &header_end);

	gcsv_text_region_subtract (region, &start, &header_end);
}

static void
add_subregion_to_scan (GcsvAlignment *align,
		       GtkTextIter   *start,
		       GtkTextIter   *end)
{
	adjust_subregion (start, end);

	if (align->scan_region == NULL)
	{
		align->scan_region = gcsv_text_region_new (GTK_TEXT_BUFFER (align->buffer));
	}

	gcsv_text_region_add (align->scan_region, start, end);

	remove_header (align, align->scan_region);
}

static void
add_subregion_to_align (GcsvAlignment *align,
			GtkTextIter   *start,
			GtkTextIter   *end)
{
	adjust_subregion (start, end);

	if (align->align_region == NULL)
	{
		align->align_region = gcsv_text_region_new (GTK_TEXT_BUFFER (align->buffer));
	}

	gcsv_text_region_add (align->align_region, start, end);

	remove_header (align, align->align_region);
}

static void
add_subregion (GcsvAlignment *align,
	       GtkTextIter   *start,
	       GtkTextIter   *end,
	       HandleMode     mode)
{
	add_subregion_to_scan (align, start, end);
	add_subregion_to_align (align, start, end);
	handle_mode (align, mode);
}

static void
update_all (GcsvAlignment *align,
	    HandleMode     mode)
{
	GtkTextIter start;
	GtkTextIter end;

	if (!align->enabled)
	{
		return;
	}

	if (align->column_lengths != NULL)
	{
		g_array_unref (align->column_lengths);
	}

	align->column_lengths = g_array_new (FALSE, TRUE, sizeof (gint));

	gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (align->buffer), &start, &end);
	add_subregion (align, &start, &end, mode);
}

static void
delimiter_notify_cb (GcsvBuffer    *buffer,
		     GParamSpec    *pspec,
		     GcsvAlignment *align)
{
	update_all (align, HANDLE_MODE_IDLE);
}

static void
insert_text_after_cb (GtkTextBuffer *buffer,
		      GtkTextIter   *location,
		      gchar         *text,
		      gint           length,
		      GcsvAlignment *align)
{
	glong n_chars;
	GtkTextIter start;
	GtkTextIter end;
	gunichar delimiter;

	/* If Enter is pressed in the middle of a line, a column can shrink. So
	 * it's simpler to update everything.
	 */
	if (g_utf8_strchr (text, length, '\n') != NULL ||
	    g_utf8_strchr (text, length, '\r') != NULL)
	{
		update_all (align, HANDLE_MODE_TIMEOUT);
	}

	n_chars = g_utf8_strlen (text, length);

	start = end = *location;
	gtk_text_iter_backward_chars (&start, n_chars);

	delimiter = gcsv_buffer_get_delimiter (align->buffer);

	if (delimiter != '\0' &&
	    n_chars == 1 &&
	    is_text_region_empty (align->scan_region) &&
	    is_text_region_empty (align->align_region) &&
	    g_utf8_strchr (text, length, delimiter) == NULL)
	{
		GtkTextMark *mark;

		mark = gtk_text_buffer_create_mark (buffer, NULL, location, TRUE);

		/* When possible, it's better to re-align the buffer
		 * synchronously, so the fields on the right are not shifted,
		 * like in a spreadsheet.
		 */
		add_subregion (align, &start, &end, HANDLE_MODE_SYNC);

		/* Restore location */
		gtk_text_buffer_get_iter_at_mark (buffer, location, mark);
		gtk_text_buffer_delete_mark (buffer, mark);
	}
	else
	{
		add_subregion (align, &start, &end, HANDLE_MODE_TIMEOUT);
	}
}

static void
delete_range_cb (GtkTextBuffer *buffer,
		 GtkTextIter   *start,
		 GtkTextIter   *end,
		 GcsvAlignment *align)
{
	gunichar delimiter;
	GtkTextIter start_copy = *start;
	GtkTextIter end_copy = *end;
	GtkTextIter start_buffer;
	GtkTextIter end_buffer;
	GtkTextIter field_start;
	GtkTextIter field_end;
	guint column_num_start;
	guint column_num_end;
	gint field_length;
	gint column_length;

	delimiter = gcsv_buffer_get_delimiter (align->buffer);

	if (delimiter == '\0')
	{
		add_subregion (align, &start_copy, &end_copy, HANDLE_MODE_TIMEOUT);
		return;
	}

	gtk_text_buffer_get_bounds (buffer, &start_buffer, &end_buffer);
	if (gtk_text_iter_equal (&start_buffer, start) &&
	    gtk_text_iter_equal (&end_buffer, end))
	{
		add_subregion (align, &start_copy, &end_copy, HANDLE_MODE_TIMEOUT);
		return;
	}

	/* If the deletion spans multiple fields, it's simpler to update
	 * everything, because a column can shrink.
	 */
	column_num_start = gcsv_buffer_get_column_num (align->buffer, start);
	column_num_end = gcsv_buffer_get_column_num (align->buffer, end);
	if (gtk_text_iter_get_line (start) != gtk_text_iter_get_line (end) ||
	    column_num_start != column_num_end)
	{
		update_all (align, HANDLE_MODE_TIMEOUT);
		return;
	}

	/* Still not scanned */
	if (column_num_start >= align->column_lengths->len)
	{
		add_subregion (align, &start_copy, &end_copy, HANDLE_MODE_TIMEOUT);
		return;
	}

	column_length = get_column_length (align, column_num_start);

	gcsv_buffer_get_field_bounds (align->buffer,
				      gtk_text_iter_get_line (start),
				      column_num_start,
				      &field_start,
				      &field_end);

	field_length = get_field_length (align, &field_start, &field_end, FALSE);

	if (field_length >= column_length)
	{
		/* Maybe a column shrinking, we need to re-scan the buffer if it
		 * was the last field with the maximum length.
		 */
		update_all (align, HANDLE_MODE_TIMEOUT);
		return;
	}

	align->sync_after_delete_range = (is_text_region_empty (align->scan_region) &&
					  is_text_region_empty (align->align_region));

	add_subregion (align, &start_copy, &end_copy, HANDLE_MODE_TIMEOUT);
}

static void
delete_range_after_cb (GtkTextBuffer *buffer,
		       GtkTextIter   *start,
		       GtkTextIter   *end,
		       GcsvAlignment *align)
{
	if (align->sync_after_delete_range)
	{
		GtkTextMark *mark;

		mark = gtk_text_buffer_create_mark (buffer, NULL, start, TRUE);

		/* When possible, it's better to re-align the buffer
		 * synchronously, so the fields on the right are not shifted,
		 * like in a spreadsheet.
		 */
		handle_mode (align, HANDLE_MODE_SYNC);
		align->sync_after_delete_range = FALSE;

		/* Restore location */
		gtk_text_buffer_get_iter_at_mark (buffer, start, mark);
		*end = *start;
		gtk_text_buffer_delete_mark (buffer, mark);
	}
}

static void
connect_signals (GcsvAlignment *align)
{
	if (align->delimiter_notify_handler_id == 0)
	{
		align->delimiter_notify_handler_id =
			g_signal_connect (align->buffer,
					  "notify::delimiter",
					  G_CALLBACK (delimiter_notify_cb),
					  align);
	}

	if (align->insert_text_handler_id == 0)
	{
		align->insert_text_handler_id =
			g_signal_connect_after (align->buffer,
						"insert-text",
						G_CALLBACK (insert_text_after_cb),
						align);
	}

	if (align->delete_range_handler_id == 0)
	{
		align->delete_range_handler_id =
			g_signal_connect (align->buffer,
					  "delete-range",
					  G_CALLBACK (delete_range_cb),
					  align);
	}

	if (align->delete_range_after_handler_id == 0)
	{
		align->delete_range_after_handler_id =
			g_signal_connect_after (align->buffer,
						"delete-range",
						G_CALLBACK (delete_range_after_cb),
						align);
	}
}

static void
disconnect_signals (GcsvAlignment *align)
{
	if (align->delimiter_notify_handler_id != 0)
	{
		g_signal_handler_disconnect (align->buffer, align->delimiter_notify_handler_id);
		align->delimiter_notify_handler_id = 0;
	}

	if (align->insert_text_handler_id != 0)
	{
		g_signal_handler_disconnect (align->buffer, align->insert_text_handler_id);
		align->insert_text_handler_id = 0;
	}

	if (align->delete_range_handler_id != 0)
	{
		g_signal_handler_disconnect (align->buffer, align->delete_range_handler_id);
		align->delete_range_handler_id = 0;
	}

	if (align->delete_range_after_handler_id != 0)
	{
		g_signal_handler_disconnect (align->buffer, align->delete_range_after_handler_id);
		align->delete_range_after_handler_id = 0;
	}
}

static void
column_titles_set_cb (GcsvBuffer    *buffer,
		      GcsvAlignment *align)
{
	BufferEditData edit_data;
	GtkTextIter start;
	GtkTextIter header_end;

	/* Remove alignment in the header. It is done synchronously, but it
	 * should not be a problem because a header is usually small.
	 */
	edit_data = begin_buffer_edit (align);
	gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (buffer), &start);
	gcsv_buffer_get_column_titles_location (buffer, &header_end);
	gcsv_utils_delete_text_with_tag (GTK_TEXT_BUFFER (buffer),
					 &start,
					 &header_end,
					 align->tag);
	end_buffer_edit (align, &edit_data);

	update_all (align, HANDLE_MODE_TIMEOUT);
}

static void
set_buffer (GcsvAlignment *align,
	    GcsvBuffer    *buffer)
{
	g_assert (align->buffer == NULL);
	g_assert (align->tag == NULL);

	g_return_if_fail (GCSV_IS_BUFFER (buffer));
	align->buffer = g_object_ref (buffer);

	align->tag = gtk_source_buffer_create_source_tag (GTK_SOURCE_BUFFER (buffer),
							  NULL,
							  "draw-spaces", FALSE,
							  NULL);
	g_object_ref (align->tag);

	if (align->enabled)
	{
		connect_signals (align);
	}

	/* This signal must always be connected, to remove the alignment in the
	 * header.
	 */
	g_signal_connect_object (buffer,
				 "column-titles-set",
				 G_CALLBACK (column_titles_set_cb),
				 align,
				 0);

	g_object_notify (G_OBJECT (align), "buffer");

	update_all (align, HANDLE_MODE_IDLE);
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

		case PROP_ENABLED:
			g_value_set_boolean (value, align->enabled);
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

		case PROP_ENABLED:
			gcsv_alignment_set_enabled (align, g_value_get_boolean (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
remove_event_sources (GcsvAlignment *align)
{
	if (align->idle_id != 0)
	{
		g_source_remove (align->idle_id);
		align->idle_id = 0;
	}

	if (align->timeout_id != 0)
	{
		g_source_remove (align->timeout_id);
		align->timeout_id = 0;
	}
}

static void
gcsv_alignment_dispose (GObject *object)
{
	GcsvAlignment *align = GCSV_ALIGNMENT (object);

	disconnect_signals (align);
	remove_event_sources (align);

	if (align->scan_region != NULL)
	{
		gcsv_text_region_destroy (align->scan_region);
		align->scan_region = NULL;
	}

	if (align->align_region != NULL)
	{
		gcsv_text_region_destroy (align->align_region);
		align->align_region = NULL;
	}

	if (align->buffer != NULL)
	{
		if (align->tag != NULL)
		{
			GtkTextTagTable *table;

			table = gtk_text_buffer_get_tag_table (GTK_TEXT_BUFFER (align->buffer));
			gtk_text_tag_table_remove (table, align->tag);

			g_object_unref (align->tag);
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
							      GCSV_TYPE_BUFFER,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY |
							      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
					 PROP_ENABLED,
					 g_param_spec_boolean ("enabled",
							       "Enabled",
							       "",
							       TRUE,
							       G_PARAM_READWRITE |
							       G_PARAM_CONSTRUCT |
							       G_PARAM_STATIC_STRINGS));
}

static void
gcsv_alignment_init (GcsvAlignment *align)
{
}

GcsvAlignment *
gcsv_alignment_new (GcsvBuffer *buffer)
{
	g_return_val_if_fail (GCSV_IS_BUFFER (buffer), NULL);

	return g_object_new (GCSV_TYPE_ALIGNMENT,
			     "buffer", buffer,
			     NULL);
}

void
gcsv_alignment_set_enabled (GcsvAlignment *align,
			    gboolean       enabled)
{
	g_return_if_fail (GCSV_IS_ALIGNMENT (align));

	enabled = enabled != FALSE;

	if (align->enabled == enabled)
	{
		return;
	}

	align->enabled = enabled;

	if (enabled)
	{
		connect_signals (align);
		update_all (align, HANDLE_MODE_IDLE);
	}
	else
	{
		disconnect_signals (align);
		remove_event_sources (align);
	}

	g_object_notify (G_OBJECT (align), "enabled");
}

GtkSourceBuffer *
gcsv_alignment_copy_buffer_without_alignment (GcsvAlignment *align)
{
	GtkTextBuffer *copy;
	GtkTextIter iter;
	GtkTextIter insert;

	copy = GTK_TEXT_BUFFER (gtk_source_buffer_new (NULL));

	gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (align->buffer), &iter);
	gtk_text_buffer_get_start_iter (copy, &insert);

	while (!gtk_text_iter_is_end (&iter))
	{
		GtkTextIter start;
		GtkTextIter end;
		gchar *text;

		start = iter;
		if (gtk_text_iter_starts_tag (&start, align->tag))
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
			g_assert (gtk_text_iter_starts_tag (&end, align->tag));
		}
		else
		{
			g_assert (gtk_text_iter_is_end (&end));
		}

		text = gtk_text_buffer_get_text (GTK_TEXT_BUFFER (align->buffer), &start, &end, TRUE);
		gtk_text_buffer_insert (copy, &insert, text, -1);
		g_free (text);

		iter = end;
	}

	return GTK_SOURCE_BUFFER (copy);
}

void
gcsv_alignment_set_unit_test_mode (GcsvAlignment *align,
				   gboolean       unit_test_mode)
{
	g_return_if_fail (GCSV_IS_ALIGNMENT (align));

	align->unit_test_mode = unit_test_mode != FALSE;

	if (align->timeout_id != 0)
	{
		install_idle (align);
	}
}
