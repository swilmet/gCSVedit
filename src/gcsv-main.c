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

#include "config.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "gcsv-window.h"

static gboolean option_version;

static GOptionEntry options[] = {
	{ "version", 'v',
	  G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &option_version,
	  N_("Display the version and exit"),
	  NULL
	},

        { NULL }
};

static gint
handle_local_options_cb (GApplication *app,
			 GVariantDict *options_dict)
{
	if (option_version)
	{
		g_print ("%s %s\n", g_get_application_name (), PACKAGE_VERSION);
		return 0;
	}

	return -1;
}

static gchar *
get_locale_directory (void)
{
	return g_build_filename (DATADIR, "locale", NULL);
}

static void
setup_i18n (void)
{
	gchar *locale_dir;

	setlocale (LC_ALL, "");

	locale_dir = get_locale_directory ();
	bindtextdomain (GETTEXT_PACKAGE, locale_dir);
	g_free (locale_dir);

	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
}

static void
quit_activate_cb (GSimpleAction *quit_action,
		  GVariant      *parameter,
		  gpointer       user_data)
{
	GtkApplication *app = GTK_APPLICATION (user_data);

	while (TRUE)
	{
		GtkWindow *window = gtk_application_get_active_window (app);

		if (GCSV_IS_WINDOW (window) &&
		    gcsv_window_close (GCSV_WINDOW (window)))
		{
			continue;
		}

		break;
	}
}

static void
about_activate_cb (GSimpleAction *about_action,
		   GVariant      *parameter,
		   gpointer       user_data)
{
	GtkApplication *app = GTK_APPLICATION (user_data);
	GtkWindow *active_window = gtk_application_get_active_window (app);

	const gchar *authors[] = {
		"Sébastien Wilmet <sebastien.wilmet@uclouvain.be>",
		NULL
	};

	gtk_show_about_dialog (active_window,
			       "name", g_get_application_name (),
			       "version", PACKAGE_VERSION,
			       "comments", _("gCSVedit is a small and lightweight CSV text editor"),
			       "authors", authors,
			       "translator-credits", _("translator-credits"),
			       "website", PACKAGE_URL,
			       "website-label", _("gCSVedit website"),
			       "logo-icon-name", "accessories-text-editor",
			       "license-type", GTK_LICENSE_GPL_3_0,
			       "copyright", "Copyright 2015 – Université Catholique de Louvain",
			       NULL);
}

static void
startup_cb (GtkApplication *app)
{
	const GActionEntry app_entries[] = {
		{ "quit", quit_activate_cb },
		{ "about", about_activate_cb },
	};

	g_action_map_add_action_entries (G_ACTION_MAP (app),
					 app_entries, G_N_ELEMENTS (app_entries),
					 app);
}

static void
activate_cb (GtkApplication *app)
{
	GcsvWindow *window = gcsv_window_new (app);
	gtk_widget_show_all (GTK_WIDGET (window));
}

static void
open_cb (GtkApplication  *app,
	 GFile          **files,
	 gint             n_files,
	 gchar           *hint)
{
	GtkWindow *active_window;
	GcsvWindow *window;

	if (n_files < 1)
	{
		return;
	}

	if (n_files > 1)
	{
		g_warning ("Opening several files at once is not supported.");
	}

	active_window = gtk_application_get_active_window (app);

	g_return_if_fail (active_window == NULL || GCSV_IS_WINDOW (active_window));

	if (active_window != NULL &&
	    gcsv_window_is_untouched (GCSV_WINDOW (active_window)))
	{
		window = GCSV_WINDOW (active_window);
	}
	else
	{
		window = gcsv_window_new (app);
		gtk_widget_show_all (GTK_WIDGET (window));
	}

	gcsv_window_load_file (window, files[0]);
}

gint
main (gint    argc,
      gchar **argv)
{
	GtkApplication *app;
	gint status;

	setup_i18n ();

	app = gtk_application_new ("be.uclouvain.gcsvedit", G_APPLICATION_HANDLES_OPEN);

	g_set_application_name ("gCSVedit");
	gtk_window_set_default_icon_name ("accessories-text-editor");

	g_application_add_main_option_entries (G_APPLICATION (app), options);

	g_signal_connect (app,
			  "startup",
			  G_CALLBACK (startup_cb),
			  NULL);

	g_signal_connect (app,
			  "handle-local-options",
			  G_CALLBACK (handle_local_options_cb),
			  NULL);

	g_signal_connect (app,
			  "activate",
			  G_CALLBACK (activate_cb),
			  NULL);

	g_signal_connect (app,
			  "open",
			  G_CALLBACK (open_cb),
			  NULL);

	status = g_application_run (G_APPLICATION (app), argc, argv);

	g_object_unref (app);

	return status;
}
