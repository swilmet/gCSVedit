/*
 * This file is part of gCSVedit.
 *
 * Copyright 2015, 2016 - Université Catholique de Louvain
 *
 * From gedit for Windows support:
 * Copyright 2010 - Jesse van den Kieboom
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
#include <gtef/gtef.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "gcsv-window.h"

#ifdef G_OS_WIN32
#  include <io.h>
#  include <conio.h>
#  ifndef _WIN32_WINNT
#    define _WIN32_WINNT 0x0501
#  endif
#  include <windows.h>
#endif

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
	return g_build_filename (GCSV_DATADIR, "locale", NULL);
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

static GcsvWindow *
get_active_gcsv_window (GtkApplication *app)
{
	GList *windows;
	GList *l;

	windows = gtk_application_get_windows (app);

	for (l = windows; l != NULL; l = l->next)
	{
		GtkWindow *window = l->data;

		if (GCSV_IS_WINDOW (window))
		{
			return GCSV_WINDOW (window);
		}
	}

	return NULL;
}

static void
quit_activate_cb (GSimpleAction *quit_action,
		  GVariant      *parameter,
		  gpointer       user_data)
{
	GtkApplication *app = GTK_APPLICATION (user_data);

	while (TRUE)
	{
		GcsvWindow *window = get_active_gcsv_window (app);

		if (window != NULL && gcsv_window_close (window))
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
	GcsvWindow *active_window = get_active_gcsv_window (app);

	const gchar *authors[] = {
		"Sébastien Wilmet <sebastien.wilmet@uclouvain.be>",
		NULL
	};

	const gchar *copyright =
		"Copyright 2015-2016 – Université Catholique de Louvain\n"
		"Copyright 2015-2016 – Sébastien Wilmet";

	gtk_show_about_dialog (GTK_WINDOW (active_window),
			       "name", g_get_application_name (),
			       "version", PACKAGE_VERSION,
			       "comments", _("gCSVedit is a small and lightweight CSV text editor"),
			       "authors", authors,
			       "translator-credits", _("translator-credits"),
			       "website", PACKAGE_URL,
			       "website-label", _("gCSVedit website"),
			       "logo-icon-name", "accessories-text-editor",
			       "license-type", GTK_LICENSE_GPL_3_0,
			       "copyright", copyright,
			       NULL);
}

/* Code taken from gedit. */
#ifdef G_OS_WIN32
static void
setup_path (void)
{
	gchar *installdir;
	gchar *bin;
	gchar *path;

	installdir = g_win32_get_package_installation_directory_of_module (NULL);

	bin = g_build_filename (installdir, "bin", NULL);
	g_free (installdir);

	/* Set PATH to include the gedit executable's folder */
	path = g_build_path (";", bin, g_getenv ("PATH"), NULL);
	g_free (bin);

	if (!g_setenv ("PATH", path, TRUE))
	{
		g_warning ("Could not set PATH for gCSVedit.");
	}

	g_free (path);
}

static void
prep_console (void)
{
	/* If we open the application from a console get the stdout printing */
	if (fileno (stdout) != -1 &&
	    _get_osfhandle (fileno (stdout)) != -1)
	{
		/* stdout is fine, presumably redirected to a file or pipe */
	}
	else
	{
		typedef BOOL (* WINAPI AttachConsole_t) (DWORD);

		AttachConsole_t p_AttachConsole =
			(AttachConsole_t) GetProcAddress (GetModuleHandle ("kernel32.dll"),
							  "AttachConsole");

		if (p_AttachConsole != NULL && p_AttachConsole (ATTACH_PARENT_PROCESS))
		{
			freopen ("CONOUT$", "w", stdout);
			dup2 (fileno (stdout), 1);
			freopen ("CONOUT$", "w", stderr);
			dup2 (fileno (stderr), 2);
		}
	}
}
#endif /* G_OS_WIN32 */

static void
startup_cb (GtkApplication *app)
{
	gchar *metadata_filename;

	/* GActions */
	const GActionEntry app_entries[] = {
		{ "quit", quit_activate_cb },
		{ "about", about_activate_cb },
	};

	g_action_map_add_action_entries (G_ACTION_MAP (app),
					 app_entries, G_N_ELEMENTS (app_entries),
					 app);

	/* Init metadata manager */
	metadata_filename = g_build_filename (g_get_user_cache_dir (),
					      "gcsvedit",
					      "gcsvedit-metadata.xml",
					      NULL);

	gtef_metadata_manager_init (metadata_filename);

	g_free (metadata_filename);

#ifdef G_OS_WIN32
	setup_path ();
	prep_console ();
#endif
}

static void
activate_cb (GtkApplication *app)
{
	GcsvWindow *window = gcsv_window_new (app);
	gtk_widget_show (GTK_WIDGET (window));
}

static void
open_cb (GtkApplication  *app,
	 GFile          **files,
	 gint             n_files,
	 gchar           *hint)
{
	GcsvWindow *active_window;
	GcsvWindow *window;

	if (n_files < 1)
	{
		return;
	}

	if (n_files > 1)
	{
		g_warning ("Opening several files at once is not supported.");
	}

	active_window = get_active_gcsv_window (app);

	if (active_window != NULL && gcsv_window_is_untouched (active_window))
	{
		window = active_window;
	}
	else
	{
		window = gcsv_window_new (app);
		gtk_widget_show (GTK_WIDGET (window));
	}

	gcsv_window_load_file (window, files[0]);
}

static void
shutdown_after_cb (GtkApplication *app)
{
	gtef_metadata_manager_shutdown ();
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

	/* Connect after, so we are sure that everything in GTK+ is shutdown and
	 * we can shutdown our stuff. By connecting without the after flag, GTK+
	 * can still hold some references to some objects, and those objects can
	 * still save settings, metadata, etc in their destructors.
	 */
	g_signal_connect_after (app,
				"shutdown",
				G_CALLBACK (shutdown_after_cb),
				NULL);

	status = g_application_run (G_APPLICATION (app), argc, argv);

	g_object_unref (app);

	return status;
}
