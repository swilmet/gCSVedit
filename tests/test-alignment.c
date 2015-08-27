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
	GtkTextBuffer *buffer;
	GtkSourceBuffer *copy_without_align;
	GcsvAlignment *align;
	gchar *buffer_text;

	buffer = gtk_text_buffer_new (NULL);

	/* Test initial alignment */
	gtk_text_buffer_set_text (buffer, before, -1);
	align = gcsv_alignment_new (buffer, delimiter);
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

	/* Test alignment update */
	gtk_text_buffer_set_text (buffer, "", -1);
	align = gcsv_alignment_new (buffer, delimiter);
	gtk_text_buffer_set_text (buffer, before, -1);
	flush_queue ();
	buffer_text = get_buffer_text (buffer);
	/* FIXME column_lengths is not updated when new columns are inserted */
	/*g_assert_cmpstr (buffer_text, ==, after);*/
	g_free (buffer_text);

	/* Test copy without alignment */
	copy_without_align = gcsv_alignment_copy_buffer_without_alignment (align);
	buffer_text = get_buffer_text (GTK_TEXT_BUFFER (copy_without_align));
	g_assert_cmpstr (buffer_text, ==, before);
	g_free (buffer_text);
	g_object_unref (copy_without_align);

	g_object_unref (align);
	g_object_unref (buffer);
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

gint
main (gint    argc,
      gchar **argv)
{
	gtk_test_init (&argc, &argv);

	g_test_add_func ("/align/commas", test_commas);

	return g_test_run ();
}
