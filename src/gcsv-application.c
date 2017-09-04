/*
 * This file is part of gCSVedit.
 *
 * Copyright 2017 - Sébastien Wilmet <swilmet@gnome.org>
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

struct _GcsvApplicationPrivate
{
	gint something;
};

static gboolean option_version;

static GOptionEntry options[] = {
	{ "version", 'v',
	  G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, &option_version,
	  N_("Display the version and exit"),
	  NULL
	},

        { NULL }
};

G_DEFINE_TYPE_WITH_PRIVATE (GcsvApplication, gcsv_application, GTK_TYPE_APPLICATION)

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
	GList *windows;
	GList *l;

	windows = gtk_application_get_windows (GTK_APPLICATION (app));

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
quit_activate_cb (GSimpleAction *quit_action,
		  GVariant      *parameter,
		  gpointer       user_data)
{
	GcsvApplication *app = GCSV_APPLICATION (user_data);

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
	GcsvApplication *app = GCSV_APPLICATION (user_data);
	GcsvWindow *active_window = get_active_gcsv_window (app);

	const gchar *authors[] = {
		"Sébastien Wilmet <sebastien.wilmet@uclouvain.be>",
		NULL
	};

	const gchar *copyright =
		"Copyright 2015-2016 – Université Catholique de Louvain\n"
		"Copyright 2015-2017 – Sébastien Wilmet";

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

static void
init_metadata_manager (void)
{
	gchar *metadata_filename;

	metadata_filename = g_build_filename (g_get_user_cache_dir (),
					      "gcsvedit",
					      "gcsvedit-metadata.xml",
					      NULL);

	tepl_metadata_manager_init (metadata_filename);

	g_free (metadata_filename);
}

static void
set_app_menu_if_needed (GtkApplication *app)
{
	GMenu *manual_app_menu;

	manual_app_menu = gtk_application_get_menu_by_id (app, "manual-app-menu");
	g_return_if_fail (manual_app_menu != NULL);

	if (gtk_application_prefers_app_menu (app))
	{
		gtk_application_set_app_menu (app, G_MENU_MODEL (manual_app_menu));
	}
}

static void
gcsv_application_startup (GApplication *app)
{
	if (G_APPLICATION_CLASS (gcsv_application_parent_class)->startup != NULL)
	{
		G_APPLICATION_CLASS (gcsv_application_parent_class)->startup (app);
	}

	add_action_info_entries (GCSV_APPLICATION (app));
	add_action_entries (GCSV_APPLICATION (app));
	init_metadata_manager ();
	set_app_menu_if_needed (GTK_APPLICATION (app));
}

static void
gcsv_application_activate (GApplication *app)
{
	GcsvWindow *window;

	if (G_APPLICATION_CLASS (gcsv_application_parent_class)->activate != NULL)
	{
		G_APPLICATION_CLASS (gcsv_application_parent_class)->activate (app);
	}

	window = gcsv_window_new (GTK_APPLICATION (app));
	gtk_widget_show (GTK_WIDGET (window));
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

	if (active_window != NULL && gcsv_window_is_untouched (active_window))
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
	/* Chain-up first, so we are sure that everything in GTK+ is shutdown
	 * and we can shutdown our stuff. If we chain-up after our code, GTK+
	 * can still hold some references to some objects, and those objects can
	 * still save settings, metadata, etc in their destructors.
	 */
	if (G_APPLICATION_CLASS (gcsv_application_parent_class)->shutdown != NULL)
	{
		G_APPLICATION_CLASS (gcsv_application_parent_class)->shutdown (app);
	}

	tepl_metadata_manager_shutdown ();
}

static void
gcsv_application_class_init (GcsvApplicationClass *klass)
{
	GApplicationClass *gapp_class = G_APPLICATION_CLASS (klass);

	gapp_class->handle_local_options = gcsv_application_handle_local_options;
	gapp_class->startup = gcsv_application_startup;
	gapp_class->activate = gcsv_application_activate;
	gapp_class->open = gcsv_application_open;
	gapp_class->shutdown = gcsv_application_shutdown;
}

static void
gcsv_application_init (GcsvApplication *app)
{
	app->priv = gcsv_application_get_instance_private (app);

	g_set_application_name (PACKAGE_NAME);
	gtk_window_set_default_icon_name ("accessories-text-editor");

	g_application_add_main_option_entries (G_APPLICATION (app), options);
}

GcsvApplication *
gcsv_application_new (void)
{
	return g_object_new (GCSV_TYPE_APPLICATION,
			     "application-id", "be.uclouvain.gcsvedit",
			     "flags", G_APPLICATION_HANDLES_OPEN,
			     NULL);
}
