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
