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

#include "gcsv-properties-chooser.h"
#include <glib/gi18n.h>

struct _GcsvPropertiesChooser
{
	GtkGrid parent;

	/* For the delimiter. */
	GtkComboBoxText *combo;
	GtkEntry *entry;

	/* For the title line. */
	GtkSpinButton *title_spinbutton;
};

enum
{
	PROP_0,
	PROP_DELIMITER,
	PROP_TITLE_LINE,
};

#define ROW_ID_DISABLE	"disable"
#define ROW_ID_COMMA	"comma"
#define ROW_ID_TAB	"tab"
#define ROW_ID_OTHER	"other"

G_DEFINE_TYPE (GcsvPropertiesChooser, gcsv_properties_chooser, GTK_TYPE_GRID)

static void
gcsv_properties_chooser_get_property (GObject    *object,
				      guint       prop_id,
				      GValue     *value,
				      GParamSpec *pspec)
{
	GcsvPropertiesChooser *chooser = GCSV_PROPERTIES_CHOOSER (object);

	switch (prop_id)
	{
		case PROP_DELIMITER:
			g_value_set_uint (value, gcsv_properties_chooser_get_delimiter (chooser));
			break;

		case PROP_TITLE_LINE:
			g_value_set_uint (value, gcsv_properties_chooser_get_title_line (chooser));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gcsv_properties_chooser_set_property (GObject      *object,
				      guint         prop_id,
				      const GValue *value,
				      GParamSpec   *pspec)
{
	GcsvPropertiesChooser *chooser = GCSV_PROPERTIES_CHOOSER (object);

	switch (prop_id)
	{
		case PROP_DELIMITER:
			gcsv_properties_chooser_set_delimiter (chooser, g_value_get_uint (value));
			break;

		case PROP_TITLE_LINE:
			gcsv_properties_chooser_set_title_line (chooser, g_value_get_uint (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gcsv_properties_chooser_class_init (GcsvPropertiesChooserClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->get_property = gcsv_properties_chooser_get_property;
	object_class->set_property = gcsv_properties_chooser_set_property;

	g_object_class_install_property (object_class,
					 PROP_DELIMITER,
					 g_param_spec_unichar ("delimiter",
							       "Delimiter",
							       "",
							       ',',
							       G_PARAM_READWRITE |
							       G_PARAM_CONSTRUCT |
							       G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
					 PROP_TITLE_LINE,
					 g_param_spec_uint ("title-line",
							    "Title Line",
							    "",
							    0,
							    G_MAXUINT,
							    0,
							    G_PARAM_READWRITE |
							    G_PARAM_CONSTRUCT |
							    G_PARAM_STATIC_STRINGS));
}

static void
combo_changed_cb (GtkComboBox           *combo,
		  GcsvPropertiesChooser *chooser)
{
	gtk_widget_set_visible (GTK_WIDGET (chooser->entry),
				g_strcmp0 (gtk_combo_box_get_active_id (combo), ROW_ID_OTHER) == 0);

	g_object_notify (G_OBJECT (chooser), "delimiter");
}

static void
entry_changed_cb (GtkEntry              *entry,
		  GcsvPropertiesChooser *chooser)
{
	g_object_notify (G_OBJECT (chooser), "delimiter");
}

static void
title_spinbutton_value_changed_cb (GtkSpinButton         *title_spinbutton,
				   GcsvPropertiesChooser *chooser)
{
	g_object_notify (G_OBJECT (chooser), "title-line");
}

static void
gcsv_properties_chooser_init (GcsvPropertiesChooser *chooser)
{
	GtkWidget *label;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (chooser), GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_set_column_spacing (GTK_GRID (chooser), 6);

	/* Delimiter */
	label = gtk_label_new_with_mnemonic (_("_Delimiter:"));
	gtk_container_add (GTK_CONTAINER (chooser), label);

	chooser->combo = GTK_COMBO_BOX_TEXT (gtk_combo_box_text_new ());
	gtk_container_add (GTK_CONTAINER (chooser), GTK_WIDGET (chooser->combo));

	gtk_label_set_mnemonic_widget (GTK_LABEL (label),
				       GTK_WIDGET (chooser->combo));

	gtk_combo_box_text_append (chooser->combo, ROW_ID_DISABLE, _("Disable"));
	gtk_combo_box_text_append (chooser->combo, ROW_ID_COMMA, _("Comma"));
	gtk_combo_box_text_append (chooser->combo, ROW_ID_TAB, _("Tab"));
	gtk_combo_box_text_append (chooser->combo, ROW_ID_OTHER, _("Other:"));

	g_signal_connect (chooser->combo,
			  "changed",
			  G_CALLBACK (combo_changed_cb),
			  chooser);

	chooser->entry = GTK_ENTRY (gtk_entry_new ());
	gtk_entry_set_max_length (chooser->entry, 1);
	gtk_entry_set_width_chars (chooser->entry, 3);
	gtk_entry_set_max_width_chars (chooser->entry, 3);
	gtk_widget_set_no_show_all (GTK_WIDGET (chooser->entry), TRUE);
	gtk_container_add (GTK_CONTAINER (chooser), GTK_WIDGET (chooser->entry));

	g_signal_connect (chooser->entry,
			  "changed",
			  G_CALLBACK (entry_changed_cb),
			  chooser);

	/* Title line */
	label = gtk_label_new_with_mnemonic (_("_Column titles line:"));
	gtk_widget_set_hexpand (label, TRUE);
	gtk_widget_set_halign (label, GTK_ALIGN_END);
	gtk_container_add (GTK_CONTAINER (chooser), label);

	chooser->title_spinbutton = GTK_SPIN_BUTTON (gtk_spin_button_new_with_range (1, 9999, 1));
	gtk_container_add (GTK_CONTAINER (chooser), GTK_WIDGET (chooser->title_spinbutton));

	gtk_label_set_mnemonic_widget (GTK_LABEL (label),
				       GTK_WIDGET (chooser->title_spinbutton));

	g_signal_connect (chooser->title_spinbutton,
			  "value-changed",
			  G_CALLBACK (title_spinbutton_value_changed_cb),
			  chooser);
}

GcsvPropertiesChooser *
gcsv_properties_chooser_new (gunichar delimiter)
{
	return g_object_new (GCSV_TYPE_PROPERTIES_CHOOSER,
			     "delimiter", delimiter,
			     "margin", 6,
			     NULL);
}

gunichar
gcsv_properties_chooser_get_delimiter (GcsvPropertiesChooser *chooser)
{
	const gchar *row_id;

	g_return_val_if_fail (GCSV_IS_PROPERTIES_CHOOSER (chooser), '\0');

	row_id = gtk_combo_box_get_active_id (GTK_COMBO_BOX (chooser->combo));
	g_return_val_if_fail (row_id != NULL, '\0');

	if (g_str_equal (row_id, ROW_ID_DISABLE))
	{
		return '\0';
	}
	if (g_str_equal (row_id, ROW_ID_COMMA))
	{
		return ',';
	}
	if (g_str_equal (row_id, ROW_ID_TAB))
	{
		return '\t';
	}
	if (g_str_equal (row_id, ROW_ID_OTHER))
	{
		const gchar *entry_text;

		entry_text = gtk_entry_get_text (chooser->entry);

		if (entry_text == NULL || entry_text[0] == '\0')
		{
			return '\0';
		}

		return g_utf8_get_char (entry_text);
	}

	g_assert_not_reached ();
}

void
gcsv_properties_chooser_set_delimiter (GcsvPropertiesChooser *chooser,
				       gunichar               delimiter)
{
	g_return_if_fail (GCSV_IS_PROPERTIES_CHOOSER (chooser));

	if (delimiter == '\0')
	{
		gtk_combo_box_set_active_id (GTK_COMBO_BOX (chooser->combo),
					     ROW_ID_DISABLE);
	}
	else if (delimiter == ',')
	{
		gtk_combo_box_set_active_id (GTK_COMBO_BOX (chooser->combo),
					     ROW_ID_COMMA);
	}
	else if (delimiter == '\t')
	{
		gtk_combo_box_set_active_id (GTK_COMBO_BOX (chooser->combo),
					     ROW_ID_TAB);
	}
	else
	{
		gchar text[6];

		g_unichar_to_utf8 (delimiter, text);
		gtk_entry_set_text (chooser->entry, text);

		gtk_combo_box_set_active_id (GTK_COMBO_BOX (chooser->combo),
					     ROW_ID_OTHER);
	}
}

/* Starts at 0. */
guint
gcsv_properties_chooser_get_title_line (GcsvPropertiesChooser *chooser)
{
	g_return_val_if_fail (GCSV_IS_PROPERTIES_CHOOSER (chooser), 0);

	return gtk_spin_button_get_value_as_int (chooser->title_spinbutton) - 1;
}

/* Starts at 0. */
void
gcsv_properties_chooser_set_title_line (GcsvPropertiesChooser *chooser,
					guint                  title_line)
{
	g_return_if_fail (GCSV_IS_PROPERTIES_CHOOSER (chooser));

	if (title_line != gcsv_properties_chooser_get_title_line (chooser))
	{
		gtk_spin_button_set_value (chooser->title_spinbutton,
					   title_line + 1);

		g_object_notify (G_OBJECT (chooser), "title-line");
	}
}
