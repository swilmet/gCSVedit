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
startup_cb (GtkApplication *app)
{
	const gchar *menu_str;
	GtkBuilder *builder;
	GMenuModel *menubar;

	menu_str =
		"<interface>"
		"  <menu id='menubar'>"
		"    <submenu>"
		"      <attribute name='label' translatable='yes'>_File</attribute>"
		"      <item>"
		"        <attribute name='label' translatable='yes'>_Quit</attribute>"
		"        <attribute name='icon'>application-exit</attribute>"
		"        <attribute name='action'>win.quit</attribute>"
		"        <attribute name='accel'>&lt;Control&gt;q</attribute>"
		"      </item>"
		"    </submenu>"
		"  </menu>"
		"</interface>";

	builder = gtk_builder_new_from_string (menu_str, -1);
	gtk_builder_set_translation_domain (builder, GETTEXT_PACKAGE);
	menubar = G_MENU_MODEL (gtk_builder_get_object (builder, "menubar"));

	gtk_application_set_menubar (app, menubar);

	g_object_unref (builder);
}

static void
activate_cb (GtkApplication *app)
{
	GcsvWindow *window;

	window = gcsv_window_new ();
	gtk_application_add_window (app, GTK_WINDOW (window));
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

	if (active_window == NULL)
	{
		window = gcsv_window_new ();
		gtk_application_add_window (app, GTK_WINDOW (window));
	}
	else
	{
		g_return_if_fail (GCSV_IS_WINDOW (active_window));
		window = GCSV_WINDOW (active_window);
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

	g_set_application_name ("gCSVedit");

	app = gtk_application_new ("org.ucl.gcsvedit", G_APPLICATION_HANDLES_OPEN);

	g_signal_connect (app,
			  "startup",
			  G_CALLBACK (startup_cb),
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
