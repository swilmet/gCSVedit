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
#include <gtksourceview/gtksource.h>

static gchar *
get_buffer_text (GtkTextBuffer *buffer)
{
	GtkTextIter start;
	GtkTextIter end;

	gtk_text_buffer_get_bounds (buffer, &start, &end);
	return gtk_text_buffer_get_text (buffer, &start, &end, TRUE);
}

static void
flush_queue (void)
{
	while (gtk_events_pending ())
	{
		gtk_main_iteration ();
	}
}

static void
check_alignment (const gchar *before,
		 const gchar *after,
		 gunichar     delimiter)
{
	GtkSourceBuffer *source_buffer;
	GtkTextBuffer *buffer;
	GtkSourceBuffer *copy_without_align;
	GcsvAlignment *align;
	gchar *buffer_text;

	source_buffer = gtk_source_buffer_new (NULL);
	buffer = GTK_TEXT_BUFFER (source_buffer);

	/* Test initial alignment */
	gtk_text_buffer_set_text (buffer, before, -1);
	align = gcsv_alignment_new (source_buffer, delimiter);
	gcsv_alignment_set_unit_test_mode (align, TRUE);
	flush_queue ();

	buffer_text = get_buffer_text (buffer);
	g_assert_cmpstr (buffer_text, ==, after);
	g_free (buffer_text);

	/* Test unalignment */
	gcsv_alignment_remove_alignment (align);
	flush_queue ();
	g_object_unref (align);
	buffer_text = get_buffer_text (buffer);
	g_assert_cmpstr (buffer_text, ==, before);
	g_free (buffer_text);

	/* Test alignment update, with column insertion */
	gtk_text_buffer_set_text (buffer, "", -1);
	align = gcsv_alignment_new (source_buffer, delimiter);
	gcsv_alignment_set_unit_test_mode (align, TRUE);
	gtk_text_buffer_set_text (buffer, before, -1);
	flush_queue ();
	buffer_text = get_buffer_text (buffer);
	g_assert_cmpstr (buffer_text, ==, after);
	g_free (buffer_text);

	/* Test copy without alignment */
	copy_without_align = gcsv_alignment_copy_buffer_without_alignment (align);
	buffer_text = get_buffer_text (GTK_TEXT_BUFFER (copy_without_align));
	g_assert_cmpstr (buffer_text, ==, before);
	g_free (buffer_text);
	g_object_unref (copy_without_align);

	g_object_unref (align);
	g_object_unref (source_buffer);
}

static void
test_commas (void)
{
	check_alignment ("aaa,bbb\n"
			 "1,2\n"
			 "10,20",
			 /**/
			 "aaa,bbb\n"
			 "1  ,2  \n"
			 "10 ,20 ",
			 /**/
			 ',');

	check_alignment ("a,b,c,\n"
			 "1,2\n"
			 "xx,yy,Zzz",
			 /**/
			 "a ,b ,c  ,\n"
			 "1 ,2    \n"
			 "xx,yy,Zzz",
			 /**/
			 ',');
}

static void
test_column_growing (void)
{
	GtkSourceBuffer *source_buffer;
	GtkTextBuffer *buffer;
	GcsvAlignment *align;
	const gchar *text_after;
	gchar *buffer_text;
	GtkTextIter iter;

	source_buffer = gtk_source_buffer_new (NULL);
	buffer = GTK_TEXT_BUFFER (source_buffer);
	gtk_text_buffer_set_text (buffer,
				  "aa,bb\n"
				  "1,2",
				  -1);

	align = gcsv_alignment_new (source_buffer, ',');
	gcsv_alignment_set_unit_test_mode (align, TRUE);
	flush_queue ();

	text_after =
		"aa,bb\n"
		"1 ,2 ";

	buffer_text = get_buffer_text (buffer);
	g_assert_cmpstr (buffer_text, ==, text_after);
	g_free (buffer_text);

	gtk_text_buffer_get_start_iter (buffer, &iter);
	gtk_text_buffer_insert (buffer, &iter, "i", -1);
	flush_queue ();

	text_after =
		"iaa,bb\n"
		"1  ,2 ";

	buffer_text = get_buffer_text (buffer);
	g_assert_cmpstr (buffer_text, ==, text_after);
	g_free (buffer_text);

	g_object_unref (source_buffer);
	g_object_unref (align);
}

static void
test_column_shrinking (void)
{
	GtkSourceBuffer *source_buffer;
	GtkTextBuffer *buffer;
	GcsvAlignment *align;
	const gchar *text_after;
	gchar *buffer_text;
	GtkTextIter start;
	GtkTextIter end;

	source_buffer = gtk_source_buffer_new (NULL);
	buffer = GTK_TEXT_BUFFER (source_buffer);
	gtk_text_buffer_set_text (buffer,
				  "daa,bb\n"
				  "1,2",
				  -1);

	align = gcsv_alignment_new (source_buffer, ',');
	gcsv_alignment_set_unit_test_mode (align, TRUE);
	flush_queue ();

	text_after =
		"daa,bb\n"
		"1  ,2 ";

	buffer_text = get_buffer_text (buffer);
	g_assert_cmpstr (buffer_text, ==, text_after);
	g_free (buffer_text);

	/* Delete the 'd' */
	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_get_iter_at_offset (buffer, &end, 1);
	gtk_text_buffer_delete (buffer, &start, &end);
	flush_queue ();

	text_after =
		"aa,bb\n"
		"1 ,2 ";

	buffer_text = get_buffer_text (buffer);
	g_assert_cmpstr (buffer_text, ==, text_after);
	g_free (buffer_text);

	g_object_unref (source_buffer);
	g_object_unref (align);
}

gint
main (gint    argc,
      gchar **argv)
{
	gtk_test_init (&argc, &argv);

	g_test_add_func ("/align/commas", test_commas);
	g_test_add_func ("/align/column_growing", test_column_growing);
	g_test_add_func ("/align/column_shrinking", test_column_shrinking);

	return g_test_run ();
}
