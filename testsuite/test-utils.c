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
#include <gtk/gtk.h>

static gchar *
get_buffer_text (GtkTextBuffer *buffer)
{
	GtkTextIter start;
	GtkTextIter end;

	gtk_text_buffer_get_bounds (buffer, &start, &end);
	return gtk_text_buffer_get_text (buffer, &start, &end, TRUE);
}

static void
test_delete_text_with_tag (void)
{
	GtkTextBuffer *buffer;
	GtkTextTag *tag;
	GtkTextIter start;
	GtkTextIter end;
	const gchar *text_before = "hello universe";
	gchar *text_after;

	buffer = gtk_text_buffer_new (NULL);
	tag = gtk_text_buffer_create_tag (buffer, NULL, NULL);

	gtk_text_buffer_set_text (buffer, text_before, -1);
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	gcsv_utils_delete_text_with_tag (buffer, &start, &end, tag);
	text_after = get_buffer_text (buffer);
	g_assert_cmpstr (text_after, ==, text_before);
	g_free (text_after);

	gtk_text_buffer_set_text (buffer, text_before, -1);

	/* Add tag to the 'h'. */
	gtk_text_buffer_get_iter_at_offset (buffer, &start, 0);
	gtk_text_buffer_get_iter_at_offset (buffer, &end, 1);
	gtk_text_buffer_apply_tag (buffer, tag, &start, &end);

	/* Add tag to "uni". */
	gtk_text_buffer_get_iter_at_offset (buffer, &start, 6);
	gtk_text_buffer_get_iter_at_offset (buffer, &end, 9);
	gtk_text_buffer_apply_tag (buffer, tag, &start, &end);

	/* Add tag to "se". */
	gtk_text_buffer_get_iter_at_offset (buffer, &start, 12);
	gtk_text_buffer_get_iter_at_offset (buffer, &end, 14);
	gtk_text_buffer_apply_tag (buffer, tag, &start, &end);

	/* Delete text with tag in "hello" */
	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_get_iter_at_offset (buffer, &end, 5);
	gcsv_utils_delete_text_with_tag (buffer, &start, &end, tag);
	text_after = get_buffer_text (buffer);
	g_assert_cmpstr (text_after, ==, "ello universe");
	g_free (text_after);

	/* Delete remaining text with tag */
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	gcsv_utils_delete_text_with_tag (buffer, &start, &end, tag);
	text_after = get_buffer_text (buffer);
	g_assert_cmpstr (text_after, ==, "ello ver");
	g_free (text_after);

	/* Test delete with start and end not at tag toggles. */
	gtk_text_buffer_set_text (buffer, text_before, -1);
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	gtk_text_buffer_apply_tag (buffer, tag, &start, &end);
	gtk_text_buffer_get_iter_at_offset (buffer, &start, 1);
	gtk_text_buffer_get_iter_at_offset (buffer, &end, 6);
	gcsv_utils_delete_text_with_tag (buffer, &start, &end, tag);
	text_after = get_buffer_text (buffer);
	g_assert_cmpstr (text_after, ==, "huniverse");
	g_free (text_after);

	g_object_unref (buffer);
}

gint
main (gint    argc,
      gchar **argv)
{
	gtk_test_init (&argc, &argv);

	g_test_add_func ("/utils/delete-text-with-tag", test_delete_text_with_tag);

	return g_test_run ();
}
