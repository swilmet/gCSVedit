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

void
gcsv_utils_delete_text_with_tag (GtkTextBuffer *buffer,
				 GtkTextTag    *tag)
{
	GtkTextIter iter;

	gtk_text_buffer_get_start_iter (buffer, &iter);

	while (TRUE)
	{
		GtkTextIter start;
		GtkTextIter end;

		start = iter;
		if (!gtk_text_iter_begins_tag (&start, tag))
		{
			if (!gtk_text_iter_forward_to_tag_toggle (&start, tag))
			{
				return;
			}

			g_assert (gtk_text_iter_begins_tag (&start, tag));
		}

		end = start;
		if (!gtk_text_iter_forward_to_tag_toggle (&end, tag))
		{
			g_assert_not_reached ();
		}
		g_assert (gtk_text_iter_ends_tag (&end, tag));

		gtk_text_buffer_delete (buffer, &start, &end);
		iter = end;
	}
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
