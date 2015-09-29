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

#include "gcsv-utils.h"

/* Delete text in @buffer between @start and @end and containing @tag. */
void
gcsv_utils_delete_text_with_tag (GtkTextBuffer     *buffer,
				 const GtkTextIter *start,
				 const GtkTextIter *end,
				 GtkTextTag        *tag)
{
	GtkTextMark *end_mark;
	GtkTextIter iter;

	g_return_if_fail (GTK_IS_TEXT_BUFFER (buffer));
	g_return_if_fail (start != NULL);
	g_return_if_fail (end != NULL);
	g_return_if_fail (gtk_text_iter_compare (start, end) <= 0);
	g_return_if_fail (GTK_IS_TEXT_TAG (tag));

	end_mark = gtk_text_buffer_create_mark (buffer, NULL, end, FALSE);
	iter = *start;

	while (TRUE)
	{
		GtkTextIter chunk_start;
		GtkTextIter chunk_end;
		GtkTextIter limit;

		gtk_text_buffer_get_iter_at_mark (buffer, &limit, end_mark);

		chunk_start = iter;
		if (!gtk_text_iter_begins_tag (&chunk_start, tag))
		{
			if (!gtk_text_iter_forward_to_tag_toggle (&chunk_start, tag))
			{
				break;
			}

			g_assert (gtk_text_iter_begins_tag (&chunk_start, tag));
		}

		if (gtk_text_iter_compare (&limit, &chunk_start) <= 0)
		{
			break;
		}

		chunk_end = chunk_start;
		if (!gtk_text_iter_forward_to_tag_toggle (&chunk_end, tag))
		{
			g_assert_not_reached ();
		}
		g_assert (gtk_text_iter_ends_tag (&chunk_end, tag));

		if (gtk_text_iter_compare (&limit, &chunk_end) < 0)
		{
			gtk_text_buffer_delete (buffer, &chunk_start, &limit);
			break;
		}

		gtk_text_buffer_delete (buffer, &chunk_start, &chunk_end);

		iter = chunk_end;
	}

	gtk_text_buffer_delete_mark (buffer, end_mark);
}

/* Returns: a 0-terminated array of blocked signal handler ids.
 * Free with g_free().
 */
gulong *
gcsv_utils_block_all_signal_handlers (GObject     *instance,
				      const gchar *signal_name)
{
	guint signal_id;
	GArray *handler_ids;

	g_return_val_if_fail (G_IS_OBJECT (instance), NULL);

	signal_id = g_signal_lookup (signal_name, G_OBJECT_TYPE (instance));
	g_return_val_if_fail (signal_id != 0, NULL);

	handler_ids = g_array_new (TRUE, TRUE, sizeof (gulong));

	while (TRUE)
	{
		gulong handler_id;

		handler_id = g_signal_handler_find (instance,
						    G_SIGNAL_MATCH_UNBLOCKED,
						    signal_id,
						    0,
						    NULL,
						    NULL,
						    NULL);

		if (handler_id == 0)
		{
			break;
		}

		g_signal_handler_block (instance, handler_id);
		g_array_append_val (handler_ids, handler_id);
	}

	return (gulong *) g_array_free (handler_ids, FALSE);
}

void
gcsv_utils_unblock_signal_handlers (GObject      *instance,
				    const gulong *handler_ids)
{
	gint i;

	g_return_if_fail (G_IS_OBJECT (instance));

	if (handler_ids == NULL)
	{
		return;
	}

	for (i = 0; handler_ids[i] != 0; i++)
	{
		g_signal_handler_unblock (instance, handler_ids[i]);
	}
}

/* Code taken from gedit-utils.c */
void
gcsv_warning (GtkWindow   *parent,
	      const gchar *format,
	      ...)
{
	va_list args;
	gchar *str;
	GtkWidget *dialog;
	GtkWindowGroup *wg = NULL;

	g_return_if_fail (format != NULL);

	if (parent != NULL)
	{
		wg = gtk_window_get_group (parent);
	}

	va_start (args, format);
	str = g_strdup_vprintf (format, args);
	va_end (args);

	dialog = gtk_message_dialog_new_with_markup (parent,
						     GTK_DIALOG_MODAL |
						     GTK_DIALOG_DESTROY_WITH_PARENT,
						     GTK_MESSAGE_ERROR,
						     GTK_BUTTONS_OK,
						     "%s", str);

	g_free (str);

	if (wg != NULL)
	{
		gtk_window_group_add_window (wg, GTK_WINDOW (dialog));
	}

	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (gtk_widget_destroy),
			  NULL);

	gtk_widget_show (dialog);
}
