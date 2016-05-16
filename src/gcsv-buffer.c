/*
 * This file is part of gCSVedit.
 *
 * Copyright 2015, 2016 - Université Catholique de Louvain
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
#include <stdlib.h>

struct _GcsvBuffer
{
	GtefBuffer parent;

	gchar *short_name;

	/* The delimiter is at most one Unicode character. If it is the nul
	 * character ('\0'), there is no alignment.
	 */
	gunichar delimiter;

	/* The column titles location, i.e. the header end boundary. */
	GtkTextMark *title_mark;
};

enum
{
	PROP_0,
	PROP_DOCUMENT_TITLE,
	PROP_DELIMITER,
};

enum
{
	SIGNAL_CURSOR_MOVED,
	SIGNAL_COLUMN_TITLES_SET,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GcsvBuffer, gcsv_buffer, GTEF_TYPE_BUFFER)

#define METADATA_DELIMITER	"gcsvedit-delimiter"
#define METADATA_TITLE_LINE	"gcsvedit-title-line"

static void
save_metadata (GcsvBuffer *buffer)
{
	GtefFile *file;
	gchar *delimiter;
	GError *error = NULL;

	file = gtef_buffer_get_file (GTEF_BUFFER (buffer));

	delimiter = gcsv_buffer_get_delimiter_as_string (buffer);
	gtef_file_set_metadata (file, METADATA_DELIMITER, delimiter);
	g_free (delimiter);

	if (buffer->title_mark != NULL)
	{
		GtkTextIter title_iter;
		gint title_line;
		gchar *title_line_str;

		gcsv_buffer_get_column_titles_location (buffer, &title_iter);
		title_line = gtk_text_iter_get_line (&title_iter);
		title_line_str = g_strdup_printf ("%d", title_line);

		gtef_file_set_metadata (file, METADATA_TITLE_LINE, title_line_str);

		g_free (title_line_str);
	}

	gtef_file_save_metadata (file, NULL, &error);

	if (error != NULL)
	{
		g_warning ("Saving metadata failed: %s", error->message);
		g_clear_error (&error);
	}
}

static void
gcsv_buffer_get_property (GObject    *object,
			  guint       prop_id,
			  GValue     *value,
			  GParamSpec *pspec)
{
	GcsvBuffer *buffer = GCSV_BUFFER (object);

	switch (prop_id)
	{
		case PROP_DOCUMENT_TITLE:
			g_value_take_string (value, gcsv_buffer_get_document_title (buffer));
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
	GtkSourceLanguageManager *language_manager;
	GtkSourceLanguage *csv_lang;
	GtkSourceStyleSchemeManager *scheme_manager;
	GtkSourceStyleScheme *scheme;

	G_OBJECT_CLASS (gcsv_buffer_parent_class)->constructed (object);

	language_manager = gtk_source_language_manager_get_default ();
	csv_lang = gtk_source_language_manager_get_language (language_manager, "csv");
	gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (buffer), csv_lang);

	scheme_manager = gtk_source_style_scheme_manager_get_default ();
	scheme = gtk_source_style_scheme_manager_get_scheme (scheme_manager, "tango");
	gtk_source_buffer_set_style_scheme (GTK_SOURCE_BUFFER (buffer), scheme);
}

static void
gcsv_buffer_dispose (GObject *object)
{
	GcsvBuffer *buffer = GCSV_BUFFER (object);
	GtefFile *file;

	file = gtef_buffer_get_file (GTEF_BUFFER (buffer));
	if (file != NULL)
	{
		save_metadata (buffer);
	}

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

	g_object_notify (G_OBJECT (text_buffer), "document-title");
}

static void
gcsv_buffer_mark_set (GtkTextBuffer     *text_buffer,
		      const GtkTextIter *location,
		      GtkTextMark       *mark)
{
	GcsvBuffer *buffer = GCSV_BUFFER (text_buffer);

	if (GTK_TEXT_BUFFER_CLASS (gcsv_buffer_parent_class)->mark_set != NULL)
	{
		GTK_TEXT_BUFFER_CLASS (gcsv_buffer_parent_class)->mark_set (text_buffer, location, mark);
	}

	if (mark == gtk_text_buffer_get_insert (text_buffer))
	{
		g_signal_emit (buffer, signals[SIGNAL_CURSOR_MOVED], 0);

		/* Sets the title_mark to be at the beginning of the line. Since
		 * the cursor moved (without a buffer change), change the header
		 * boundary to be at the beginning of a line instead of keeping
		 * the location at a random place.
		 *
		 * If later the user places the cursor at that random place and
		 * presses Enter, it would be awkward if the column titles line
		 * changed. On the other hand, at the beginning of the line is
		 * not awkward.
		 */
		if (buffer->title_mark != NULL)
		{
			GtkTextIter iter;
			guint line;

			gcsv_buffer_get_column_titles_location (buffer, &iter);
			line = gtk_text_iter_get_line (&iter);
			gcsv_buffer_set_column_titles_line (buffer, line);
		}
	}
}

static void
gcsv_buffer_changed (GtkTextBuffer *buffer)
{
	if (GTK_TEXT_BUFFER_CLASS (gcsv_buffer_parent_class)->changed != NULL)
	{
		GTK_TEXT_BUFFER_CLASS (gcsv_buffer_parent_class)->changed (buffer);
	}

	g_signal_emit (buffer, signals[SIGNAL_CURSOR_MOVED], 0);
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
	text_buffer_class->mark_set = gcsv_buffer_mark_set;
	text_buffer_class->changed = gcsv_buffer_changed;

	g_object_class_install_property (object_class,
					 PROP_DOCUMENT_TITLE,
					 g_param_spec_string ("document-title",
							      "Document Title",
							      "",
							      NULL,
							      G_PARAM_READABLE |
							      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
					 PROP_DELIMITER,
					 g_param_spec_unichar ("delimiter",
							       "Delimiter",
							       "",
							       '\0',
							       G_PARAM_READWRITE |
							       G_PARAM_CONSTRUCT |
							       G_PARAM_STATIC_STRINGS));

	signals[SIGNAL_CURSOR_MOVED] =
		g_signal_new ("cursor-moved",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_FIRST,
			      0,
			      NULL, NULL, NULL,
			      G_TYPE_NONE, 0);

	/**
	 * GcsvBuffer::column-titles-set:
	 * @buffer: the #GcsvBuffer who emits the signal.
	 *
	 * The ::column-titles-set signal is emitted when the column titles line
	 * is set to a different line. The signal is not emitted when the column
	 * titles line changes due to text insertion or deletion, it is emitted
	 * only when the column titles line is set externally, for example by
	 * the #GcsvPropertiesChooser.
	 */
	signals[SIGNAL_COLUMN_TITLES_SET] =
		g_signal_new ("column-titles-set",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_FIRST,
			      0,
			      NULL, NULL, NULL,
			      G_TYPE_NONE, 0);
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

	g_object_notify (G_OBJECT (buffer), "document-title");

out:
	g_clear_object (&info);

	/* Async operation finished */
	g_object_unref (buffer);
}

static void
update_short_name (GcsvBuffer *buffer)
{
	GtefFile *file;
	GFile *location;

	file = gtef_buffer_get_file (GTEF_BUFFER (buffer));
	location = gtk_source_file_get_location (GTK_SOURCE_FILE (file));

	if (location == NULL)
	{
		g_free (buffer->short_name);
		buffer->short_name = g_strdup (_("Untitled File"));

		g_object_notify (G_OBJECT (buffer), "document-title");
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
	GtefFile *file;

	/* Disable the undo/redo, because it doesn't work well currently with
	 * the virtual spaces.
	 */
	gtk_source_buffer_set_max_undo_levels (GTK_SOURCE_BUFFER (buffer), 0);

	update_short_name (buffer);

	file = gtef_buffer_get_file (GTEF_BUFFER (buffer));
	g_signal_connect_object (file,
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

gboolean
gcsv_buffer_is_untouched (GcsvBuffer *buffer)
{
	GtefFile *file;

	g_return_val_if_fail (GCSV_IS_BUFFER (buffer), FALSE);

	file = gtef_buffer_get_file (GTEF_BUFFER (buffer));

	return (gtk_text_buffer_get_char_count (GTK_TEXT_BUFFER (buffer)) == 0 &&
		!gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (buffer)) &&
		!gtk_source_buffer_can_undo (GTK_SOURCE_BUFFER (buffer)) &&
		!gtk_source_buffer_can_redo (GTK_SOURCE_BUFFER (buffer)) &&
		gtk_source_file_get_location (GTK_SOURCE_FILE (file)) == NULL);
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

/* Returns a title suitable for a GtkWindow title. It contains (in that order):
 * - '*' if the buffer is modified;
 * - the short name;
 * - the directory name in parenthesis if the GFile location isn't NULL.
 *
 * Free the return value with g_free().
 */
gchar *
gcsv_buffer_get_document_title (GcsvBuffer *buffer)
{
	GtefFile *file;
	GFile *location;
	gchar *title;
	gchar *full_title;
	gboolean modified;
	const gchar *modified_marker;

	g_return_val_if_fail (GCSV_IS_BUFFER (buffer), NULL);

	file = gtef_buffer_get_file (GTEF_BUFFER (buffer));
	location = gtk_source_file_get_location (GTK_SOURCE_FILE (file));

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

void
gcsv_buffer_add_uri_to_recent_manager (GcsvBuffer *buffer)
{
	GtkRecentManager *recent_manager;
	GtefFile *file;
	GFile *location;
	gchar *uri;

	g_return_if_fail (GCSV_IS_BUFFER (buffer));

	file = gtef_buffer_get_file (GTEF_BUFFER (buffer));
	location = gtk_source_file_get_location (GTK_SOURCE_FILE (file));
	if (location == NULL)
	{
		return;
	}

	recent_manager = gtk_recent_manager_get_default ();

	uri = g_file_get_uri (location);
	gtk_recent_manager_add_item (recent_manager, uri);
	g_free (uri);
}

gunichar
gcsv_buffer_get_delimiter (GcsvBuffer *buffer)
{
	g_return_val_if_fail (GCSV_IS_BUFFER (buffer), '\0');

	return buffer->delimiter;
}

gchar *
gcsv_buffer_get_delimiter_as_string (GcsvBuffer *buffer)
{
	gchar *delimiter_str;

	g_return_val_if_fail (GCSV_IS_BUFFER (buffer), g_strdup (""));

	delimiter_str = g_malloc0 (7);
	g_unichar_to_utf8 (buffer->delimiter, delimiter_str);
	return delimiter_str;
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

void
gcsv_buffer_get_column_titles_location (GcsvBuffer  *buffer,
					GtkTextIter *iter)
{
	g_return_if_fail (GCSV_IS_BUFFER (buffer));
	g_return_if_fail (iter != NULL);

	if (buffer->title_mark == NULL)
	{
		gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (buffer), iter);
	}
	else
	{
		gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (buffer),
						  iter,
						  buffer->title_mark);

		gtk_text_iter_set_line_offset (iter, 0);
	}
}

/* @line starts at 0. */
void
gcsv_buffer_set_column_titles_line (GcsvBuffer *buffer,
				    guint       line)
{
	GtkTextIter new_location;
	GtkTextIter old_location;

	g_return_if_fail (GCSV_IS_BUFFER (buffer));

	gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (buffer),
					  &new_location,
					  line);

	if (buffer->title_mark == NULL)
	{
		/* Create the mark lazily, so that for a new file, the column
		 * titles are kept at the first line by default. Once this
		 * function is called, it means that the column titles are
		 * known, so we can keep track of them.
		 */
		buffer->title_mark = gtk_text_buffer_create_mark (GTK_TEXT_BUFFER (buffer),
								  NULL,
								  &new_location,
								  FALSE);

		g_signal_emit (buffer, signals[SIGNAL_COLUMN_TITLES_SET], 0);
		return;
	}

	gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (buffer),
					  &old_location,
					  buffer->title_mark);

	gtk_text_buffer_move_mark (GTK_TEXT_BUFFER (buffer),
				   buffer->title_mark,
				   &new_location);

	if (gtk_text_iter_get_line (&old_location) != gtk_text_iter_get_line (&new_location))
	{
		g_signal_emit (buffer, signals[SIGNAL_COLUMN_TITLES_SET], 0);
	}
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

static void
guess_delimiter (GcsvBuffer *buffer)
{
	GtkTextIter iter;
	GtkTextIter limit;

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

/* Setup the state (delimiter and column titles location) from the metadata, or
 * guess the state if the metadata doesn't exist.
 * The metadata must have been loaded before calling this function.
 */
void
gcsv_buffer_setup_state (GcsvBuffer *buffer)
{
	GtefFile *file;
	gchar *delimiter;
	gchar *title_line_str;

	g_return_if_fail (GCSV_IS_BUFFER (buffer));

	file = gtef_buffer_get_file (GTEF_BUFFER (buffer));

	delimiter = gtef_file_get_metadata (file, METADATA_DELIMITER);
	if (delimiter != NULL)
	{
		gcsv_buffer_set_delimiter (buffer, g_utf8_get_char (delimiter));
		g_free (delimiter);
	}
	else
	{
		guess_delimiter (buffer);
	}

	title_line_str = gtef_file_get_metadata (file, METADATA_TITLE_LINE);
	if (title_line_str != NULL)
	{
		gint title_line;

		title_line = strtol (title_line_str, NULL, 10);
		gcsv_buffer_set_column_titles_line (buffer, title_line);

		g_free (title_line_str);
	}
}
