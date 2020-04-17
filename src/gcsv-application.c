/*
 * This file is part of gCSVedit.
 *
 * Copyright 2017-2020 - Sébastien Wilmet <swilmet@gnome.org>
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
 */

#include "config.h"
#include "gcsv-application.h"
#include <tepl/tepl.h>
#include <glib/gi18n.h>
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

G_DEFINE_TYPE (GcsvApplication, gcsv_application, GTK_TYPE_APPLICATION)

static void quit_next_window (GcsvApplication *app);

static void
add_action_info_entries (GcsvApplication *gcsv_app)
{
	TeplApplication *tepl_app;
	AmtkActionInfoStore *store;

	const AmtkActionInfoEntry entries[] =
	{
		/* action, icon, label, accel, tooltip */

		{ "app.quit", "application-exit", N_("_Quit"), "<Control>q",
		  N_("Quit the application") },

		{ "app.about", "help-about", N_("_About"), NULL,
		  N_("About gCSVedit") },

		{ "win.open", "document-open", N_("_Open"), "<Control>o",
		  N_("Open a file") },

		{ "win.save", "document-save", N_("_Save"), "<Control>s",
		  N_("Save the current file") },

		{ "win.save-as", "document-save-as", N_("Save _As"), "<Shift><Control>s",
		  N_("Save the current file with a different name") },
	};

	tepl_app = tepl_application_get_from_gtk_application (GTK_APPLICATION (gcsv_app));
	store = tepl_application_get_app_action_info_store (tepl_app);

	amtk_action_info_store_add_entries (store,
					    entries,
					    G_N_ELEMENTS (entries),
					    GETTEXT_PACKAGE);
}

static GcsvWindow *
get_active_gcsv_window (GcsvApplication *app)
{
	TeplApplication *tepl_app = tepl_application_get_from_gtk_application (GTK_APPLICATION (app));
	return GCSV_WINDOW (tepl_application_get_active_main_window (tepl_app));
}

static gint
gcsv_application_handle_local_options (GApplication *app,
				       GVariantDict *options_dict)
{
	if (option_version)
	{
		g_print ("%s %s\n", g_get_application_name (), PACKAGE_VERSION);
		return 0;
	}

	if (G_APPLICATION_CLASS (gcsv_application_parent_class)->handle_local_options != NULL)
	{
		return G_APPLICATION_CLASS (gcsv_application_parent_class)->handle_local_options (app, options_dict);
	}

	return -1;
}

static void
quit_next_window_cb (GObject      *source_object,
		     GAsyncResult *result,
		     gpointer      user_data)
{
	GcsvWindow *window = GCSV_WINDOW (source_object);
	GcsvApplication *app = GCSV_APPLICATION (user_data);

	if (gcsv_window_close_finish (window, result))
	{
		gtk_widget_destroy (GTK_WIDGET (window));
		quit_next_window (app);
	}
	else
	{
		g_application_release (G_APPLICATION (app));
	}
}

static void
quit_next_window (GcsvApplication *app)
{
	GcsvWindow *window = get_active_gcsv_window (app);

	if (window != NULL)
	{
		gcsv_window_close_async (window, quit_next_window_cb, app);
	}
	else
	{
		g_application_release (G_APPLICATION (app));
	}
}

static void
quit_activate_cb (GSimpleAction *quit_action,
		  GVariant      *parameter,
		  gpointer       user_data)
{
	GcsvApplication *app = GCSV_APPLICATION (user_data);

	g_application_hold (G_APPLICATION (app));
	quit_next_window (app);
}

static void
about_activate_cb (GSimpleAction *about_action,
		   GVariant      *parameter,
		   gpointer       user_data)
{
	GcsvApplication *app = GCSV_APPLICATION (user_data);
	GcsvWindow *active_window = get_active_gcsv_window (app);

	const gchar *authors[] = {
		"Sébastien Wilmet <swilmet@gnome.org>",
		NULL
	};

	const gchar *copyright =
		"Copyright 2015-2016 – Université Catholique de Louvain\n"
		"Copyright 2015-2020 – Sébastien Wilmet";

	gtk_show_about_dialog (GTK_WINDOW (active_window),
			       "name", g_get_application_name (),
			       "version", PACKAGE_VERSION,
			       "comments", _("gCSVedit is a simple CSV/TSV text editor"),
			       "authors", authors,
			       "translator-credits", _("translator-credits"),
			       "website", PACKAGE_URL,
			       "website-label", _("gCSVedit website"),
			       "logo-icon-name", "accessories-text-editor",
			       "license-type", GTK_LICENSE_GPL_3_0,
			       "copyright", copyright,
			       NULL);
}

static void
add_action_entries (GcsvApplication *app)
{
	const GActionEntry app_entries[] =
	{
		{ "quit", quit_activate_cb },
		{ "about", about_activate_cb },
	};

	amtk_action_map_add_action_entries_check_dups (G_ACTION_MAP (app),
						       app_entries,
						       G_N_ELEMENTS (app_entries),
						       app);
}

static GFile *
get_metadata_store_file (void)
{
	return g_file_new_build_filename (g_get_user_data_dir (),
					  "gcsvedit",
					  "gcsvedit-metadata.xml",
					  NULL);
}

static void
load_metadata_store (void)
{
	TeplMetadataStore *store;
	GFile *store_file;
	GError *error = NULL;

	store = tepl_metadata_store_get_singleton ();
	store_file = get_metadata_store_file ();
	tepl_metadata_store_load (store, store_file, &error);

	if (error != NULL)
	{
		g_warning ("Failed to load metadata: %s", error->message);
		g_clear_error (&error);
	}

	g_object_unref (store_file);
}

static void
save_metadata_store (void)
{
	TeplMetadataStore *store;
	GFile *store_file;
	GError *error = NULL;

	store = tepl_metadata_store_get_singleton ();
	store_file = get_metadata_store_file ();
	tepl_metadata_store_save (store, store_file, TRUE, &error);

	if (error != NULL)
	{
		g_warning ("Failed to save metadata: %s", error->message);
		g_clear_error (&error);
	}

	g_object_unref (store_file);
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
gcsv_application_startup (GApplication *app)
{
	if (G_APPLICATION_CLASS (gcsv_application_parent_class)->startup != NULL)
	{
		G_APPLICATION_CLASS (gcsv_application_parent_class)->startup (app);
	}

	load_metadata_store ();
	add_action_info_entries (GCSV_APPLICATION (app));
	add_action_entries (GCSV_APPLICATION (app));

#ifdef G_OS_WIN32
	setup_path ();
	prep_console ();
#endif
}

static gboolean
window_is_untouched (GcsvWindow *window)
{
	TeplApplicationWindow *tepl_window;
	TeplBuffer *buffer;

	tepl_window = tepl_application_window_get_from_gtk_application_window (GTK_APPLICATION_WINDOW (window));
	buffer = tepl_tab_group_get_active_buffer (TEPL_TAB_GROUP (tepl_window));
	return tepl_buffer_is_untouched (buffer);
}

static void
gcsv_application_open (GApplication  *app,
		       GFile        **files,
		       gint           n_files,
		       const gchar   *hint)
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

	active_window = get_active_gcsv_window (GCSV_APPLICATION (app));

	if (active_window != NULL && window_is_untouched (active_window))
	{
		window = active_window;
	}
	else
	{
		window = gcsv_window_new (GTK_APPLICATION (app));
		gtk_widget_show (GTK_WIDGET (window));
	}

	gcsv_window_load_file (window, files[0]);
}

static void
gcsv_application_shutdown (GApplication *app)
{
	save_metadata_store ();

	if (G_APPLICATION_CLASS (gcsv_application_parent_class)->shutdown != NULL)
	{
		G_APPLICATION_CLASS (gcsv_application_parent_class)->shutdown (app);
	}
}

static void
gcsv_application_class_init (GcsvApplicationClass *klass)
{
	GApplicationClass *gapp_class = G_APPLICATION_CLASS (klass);

	gapp_class->handle_local_options = gcsv_application_handle_local_options;
	gapp_class->startup = gcsv_application_startup;
	gapp_class->open = gcsv_application_open;
	gapp_class->shutdown = gcsv_application_shutdown;
}

static void
gcsv_application_init (GcsvApplication *app)
{
	TeplApplication *tepl_app;

	g_set_application_name (PACKAGE_NAME);
	gtk_window_set_default_icon_name ("accessories-text-editor");

	g_application_add_main_option_entries (G_APPLICATION (app), options);

	tepl_app = tepl_application_get_from_gtk_application (GTK_APPLICATION (app));
	tepl_application_handle_activate (tepl_app);
}

GcsvApplication *
gcsv_application_new (void)
{
	return g_object_new (GCSV_TYPE_APPLICATION,
			     "application-id", "com.github.swilmet.gcsvedit",
			     "flags", G_APPLICATION_HANDLES_OPEN,
			     NULL);
}
