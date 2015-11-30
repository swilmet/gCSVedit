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
#include <glib/gi18n.h>

struct _GcsvBuffer
{
	GtkSourceBuffer parent;

	GtkSourceFile *file;
	gchar *short_name;

	/* The delimiter is at most one Unicode character. If it is the nul
	 * character ('\0'), there is no alignment.
	 */
	gunichar delimiter;

	GtkTextMark *title_mark;
};

enum
{
	PROP_0,
	PROP_TITLE,
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
		case PROP_TITLE:
			g_value_take_string (value, gcsv_buffer_get_title (buffer));
			break;

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
gcsv_buffer_dispose (GObject *object)
{
	GcsvBuffer *buffer = GCSV_BUFFER (object);

	g_clear_object (&buffer->file);

	G_OBJECT_CLASS (gcsv_buffer_parent_class)->dispose (object);
}

static void
gcsv_buffer_finalize (GObject *object)
{
	GcsvBuffer *buffer = GCSV_BUFFER (object);

	g_free (buffer->short_name);

	G_OBJECT_CLASS (gcsv_buffer_parent_class)->finalize (object);
}

static void
gcsv_buffer_modified_changed (GtkTextBuffer *text_buffer)
{
	if (GTK_TEXT_BUFFER_CLASS (gcsv_buffer_parent_class)->modified_changed != NULL)
	{
		GTK_TEXT_BUFFER_CLASS (gcsv_buffer_parent_class)->modified_changed (text_buffer);
	}

	g_object_notify (G_OBJECT (text_buffer), "title");
}

static void
gcsv_buffer_class_init (GcsvBufferClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkTextBufferClass *text_buffer_class = GTK_TEXT_BUFFER_CLASS (klass);

	object_class->get_property = gcsv_buffer_get_property;
	object_class->set_property = gcsv_buffer_set_property;
	object_class->constructed = gcsv_buffer_constructed;
	object_class->dispose = gcsv_buffer_dispose;
	object_class->finalize = gcsv_buffer_finalize;

	text_buffer_class->modified_changed = gcsv_buffer_modified_changed;

	g_object_class_install_property (object_class,
					 PROP_TITLE,
					 g_param_spec_string ("title",
							      "Title",
							      "",
							      NULL,
							      G_PARAM_READABLE |
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
query_display_name_cb (GFile        *location,
		       GAsyncResult *result,
		       GcsvBuffer   *buffer)
{
	GFileInfo *info;
	GError *error = NULL;

	info = g_file_query_info_finish (location, result, &error);

	if (error != NULL)
	{
		g_warning (_("Error when querying file information: %s"), error->message);
		g_clear_error (&error);
		goto out;
	}

	g_free (buffer->short_name);
	buffer->short_name = g_strdup (g_file_info_get_display_name (info));

	g_object_notify (G_OBJECT (buffer), "title");

out:
	g_clear_object (&info);

	/* Async operation finished */
	g_object_unref (buffer);
}

static void
update_short_name (GcsvBuffer *buffer)
{
	GFile *location;

	location = gtk_source_file_get_location (buffer->file);

	if (location == NULL)
	{
		g_free (buffer->short_name);
		buffer->short_name = g_strdup (_("Untitled File"));

		g_object_notify (G_OBJECT (buffer), "title");
	}
	else
	{
		g_file_query_info_async (location,
					 G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
					 G_FILE_QUERY_INFO_NONE,
					 G_PRIORITY_DEFAULT,
					 NULL,
					 (GAsyncReadyCallback) query_display_name_cb,
					 g_object_ref (buffer));
	}
}

static void
location_notify_cb (GtkSourceFile *file,
		    GParamSpec    *pspec,
		    GcsvBuffer    *buffer)
{
	update_short_name (buffer);
}

static void
gcsv_buffer_init (GcsvBuffer *buffer)
{
	buffer->file = gtk_source_file_new ();

	update_short_name (buffer);

	g_signal_connect_object (buffer->file,
				 "notify::location",
				 G_CALLBACK (location_notify_cb),
				 buffer,
				 0);
}

GcsvBuffer *
gcsv_buffer_new (void)
{
	return g_object_new (GCSV_TYPE_BUFFER, NULL);
}

GtkSourceFile *
gcsv_buffer_get_file (GcsvBuffer *buffer)
{
	g_return_val_if_fail (GCSV_IS_BUFFER (buffer), NULL);

	return buffer->file;
}

gboolean
gcsv_buffer_is_untouched (GcsvBuffer *buffer)
{
	g_return_val_if_fail (GCSV_IS_BUFFER (buffer), FALSE);

	return (gtk_text_buffer_get_char_count (GTK_TEXT_BUFFER (buffer)) == 0 &&
		!gtk_source_buffer_can_undo (GTK_SOURCE_BUFFER (buffer)) &&
		!gtk_source_buffer_can_redo (GTK_SOURCE_BUFFER (buffer)) &&
		gtk_source_file_get_location (buffer->file) == NULL);
}

/* Gets the document's short name. If the GFile location is non-NULL, returns
 * its display-name.
 */
const gchar *
gcsv_buffer_get_short_name (GcsvBuffer *buffer)
{
	g_return_val_if_fail (GCSV_IS_BUFFER (buffer), NULL);

	return buffer->short_name;
}

/* Returns a title suitable for a GtkWindow title. It contains '*' if the buffer
 * is modified. The short name, plus the directory name in parenthesis if the
 * buffer is saved.
 * Free the return value with g_free().
 */
gchar *
gcsv_buffer_get_title (GcsvBuffer *buffer)
{
	GFile *location;
	gchar *title;
	gchar *full_title;
	gboolean modified;
	const gchar *modified_marker;

	g_return_val_if_fail (GCSV_IS_BUFFER (buffer), NULL);

	location = gtk_source_file_get_location (buffer->file);

	if (location == NULL)
	{
		title = g_strdup (buffer->short_name);
	}
	else
	{
		GFile *parent;
		gchar *directory;

		parent = g_file_get_parent (location);
		g_return_val_if_fail (parent != NULL, NULL);

		directory = g_file_get_parse_name (parent);
		g_object_unref (parent);

		title = g_strdup_printf ("%s (%s)", buffer->short_name, directory);
		g_free (directory);
	}

	modified = gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (buffer));
	modified_marker = modified ? "*" : "";

	full_title = g_strconcat (modified_marker, title, NULL);
	g_free (title);

	return full_title;
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
