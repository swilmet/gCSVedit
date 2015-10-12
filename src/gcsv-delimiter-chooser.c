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

#include "gcsv-delimiter-chooser.h"
#include <glib/gi18n.h>

struct _GcsvDelimiterChooser
{
	GtkGrid parent;

	GtkComboBoxText *combo;
};

enum
{
	PROP_0,
	PROP_DELIMITER,
};

#define ROW_ID_COMMA	"comma"
#define ROW_ID_TAB	"tab"

G_DEFINE_TYPE (GcsvDelimiterChooser, gcsv_delimiter_chooser, GTK_TYPE_GRID)

static void
gcsv_delimiter_chooser_get_property (GObject    *object,
				     guint       prop_id,
				     GValue     *value,
				     GParamSpec *pspec)
{
	GcsvDelimiterChooser *chooser = GCSV_DELIMITER_CHOOSER (object);

	switch (prop_id)
	{
		case PROP_DELIMITER:
			g_value_set_uint (value, gcsv_delimiter_chooser_get_delimiter (chooser));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gcsv_delimiter_chooser_set_property (GObject      *object,
				     guint         prop_id,
				     const GValue *value,
				     GParamSpec   *pspec)
{
	GcsvDelimiterChooser *chooser = GCSV_DELIMITER_CHOOSER (object);

	switch (prop_id)
	{
		case PROP_DELIMITER:
			gcsv_delimiter_chooser_set_delimiter (chooser, g_value_get_uint (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gcsv_delimiter_chooser_class_init (GcsvDelimiterChooserClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->get_property = gcsv_delimiter_chooser_get_property;
	object_class->set_property = gcsv_delimiter_chooser_set_property;

	g_object_class_install_property (object_class,
					 PROP_DELIMITER,
					 g_param_spec_unichar ("delimiter",
							       "Delimiter",
							       "",
							       ',',
							       G_PARAM_READWRITE |
							       G_PARAM_CONSTRUCT |
							       G_PARAM_STATIC_STRINGS));
}

static void
changed_cb (GtkComboBox          *combo,
	    GcsvDelimiterChooser *chooser)
{
	g_object_notify (G_OBJECT (chooser), "delimiter");
}

static void
gcsv_delimiter_chooser_init (GcsvDelimiterChooser *chooser)
{
	GtkWidget *label;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (chooser), GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_set_column_spacing (GTK_GRID (chooser), 6);

	label = gtk_label_new_with_mnemonic (_("_Delimiter:"));
	gtk_container_add (GTK_CONTAINER (chooser), label);

	chooser->combo = GTK_COMBO_BOX_TEXT (gtk_combo_box_text_new ());
	gtk_container_add (GTK_CONTAINER (chooser), GTK_WIDGET (chooser->combo));

	gtk_label_set_mnemonic_widget (GTK_LABEL (label),
				       GTK_WIDGET (chooser->combo));

	gtk_combo_box_text_append (chooser->combo, ROW_ID_COMMA, _("Comma"));
	gtk_combo_box_text_append (chooser->combo, ROW_ID_TAB, _("Tab"));

	g_signal_connect (chooser->combo,
			  "changed",
			  G_CALLBACK (changed_cb),
			  chooser);

	gtk_widget_show (GTK_WIDGET (chooser->combo));
}

GcsvDelimiterChooser *
gcsv_delimiter_chooser_new (gunichar delimiter)
{
	return g_object_new (GCSV_TYPE_DELIMITER_CHOOSER,
			     "delimiter", delimiter,
			     "margin", 6,
			     NULL);
}

gunichar
gcsv_delimiter_chooser_get_delimiter (GcsvDelimiterChooser *chooser)
{
	const gchar *row_id;

	g_return_val_if_fail (GCSV_IS_DELIMITER_CHOOSER (chooser), '\0');

	row_id = gtk_combo_box_get_active_id (GTK_COMBO_BOX (chooser->combo));
	g_return_val_if_fail (row_id != NULL, '\0');

#if 0
	if (row_id == NULL)
	{
		gchar *active_text;
		gunichar delimiter;

		active_text = gtk_combo_box_text_get_active_text (chooser->combo);

		if (active_text == NULL)
		{
			return '\0';
		}

		delimiter = g_utf8_get_char (active_text);

		g_free (active_text);
		return delimiter;
	}
#endif

	if (g_str_equal (row_id, ROW_ID_COMMA))
	{
		return ',';
	}
	if (g_str_equal (row_id, ROW_ID_TAB))
	{
		return '\t';
	}

	g_return_val_if_reached ('\0');
}

void
gcsv_delimiter_chooser_set_delimiter (GcsvDelimiterChooser *chooser,
				      gunichar              delimiter)
{
	g_return_if_fail (GCSV_IS_DELIMITER_CHOOSER (chooser));

	if (delimiter == ',')
	{
		gtk_combo_box_set_active_id (GTK_COMBO_BOX (chooser->combo),
					     ROW_ID_COMMA);
	}
	else if (delimiter == '\t')
	{
		gtk_combo_box_set_active_id (GTK_COMBO_BOX (chooser->combo),
					     ROW_ID_TAB);
	}
#if 0
	else
	{
		GtkEntry *entry;
		gchar text[6];

		entry = GTK_ENTRY (gtk_bin_get_child (GTK_BIN (chooser->combo)));

		g_unichar_to_utf8 (delimiter, text);
		gtk_entry_set_text (entry, text);
	}
#endif
}
