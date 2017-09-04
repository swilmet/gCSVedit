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
 * Author: Sébastien Wilmet <swilmet@gnome.org>
 */

#include "gcsv-properties-chooser.h"
#include <glib/gi18n.h>

struct _GcsvPropertiesChooser
{
	GtkGrid parent;

	GcsvBuffer *buffer;

	/* For the delimiter. */
	GtkComboBoxText *combo;
	GtkEntry *entry;

	/* For the title line. */
	GtkSpinButton *title_spinbutton;
};

enum
{
	PROP_0,
	PROP_BUFFER,
};

#define ROW_ID_DISABLE	"disable"
#define ROW_ID_COMMA	"comma"
#define ROW_ID_TAB	"tab"
#define ROW_ID_OTHER	"other"

G_DEFINE_TYPE (GcsvPropertiesChooser, gcsv_properties_chooser, GTK_TYPE_GRID)

static gunichar
get_chooser_delimiter (GcsvPropertiesChooser *chooser)
{
	const gchar *row_id;

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

static void
update_chooser_delimiter (GcsvPropertiesChooser *chooser)
{
	gunichar buffer_delimiter;
	gunichar chooser_delimiter;

	buffer_delimiter = gcsv_buffer_get_delimiter (chooser->buffer);
	chooser_delimiter = get_chooser_delimiter (chooser);

	if (buffer_delimiter == chooser_delimiter)
	{
		/* The same delimiter can be set differently in the properties
		 * chooser, for example the comma can be set as a ComboBox item
		 * or in the GtkEntry. So we should not change that state.
		 */
		return;
	}

	if (buffer_delimiter == '\0')
	{
		gtk_combo_box_set_active_id (GTK_COMBO_BOX (chooser->combo),
					     ROW_ID_DISABLE);
	}
	else if (buffer_delimiter == ',')
	{
		gtk_combo_box_set_active_id (GTK_COMBO_BOX (chooser->combo),
					     ROW_ID_COMMA);
	}
	else if (buffer_delimiter == '\t')
	{
		gtk_combo_box_set_active_id (GTK_COMBO_BOX (chooser->combo),
					     ROW_ID_TAB);
	}
	else
	{
		gchar *buffer_delimiter_str;

		buffer_delimiter_str = gcsv_buffer_get_delimiter_as_string (chooser->buffer);
		gtk_entry_set_text (chooser->entry, buffer_delimiter_str);
		g_free (buffer_delimiter_str);

		gtk_combo_box_set_active_id (GTK_COMBO_BOX (chooser->combo),
					     ROW_ID_OTHER);
	}
}

static void
delimiter_notify_cb (GcsvBuffer            *buffer,
		     GParamSpec            *pspec,
		     GcsvPropertiesChooser *chooser)
{
	update_chooser_delimiter (chooser);
}

/* Starts at 0. */
static guint
get_chooser_title_line (GcsvPropertiesChooser *chooser)
{
	return gtk_spin_button_get_value_as_int (chooser->title_spinbutton) - 1;
}

static void
title_spinbutton_value_changed_cb (GtkSpinButton         *title_spinbutton,
				   GcsvPropertiesChooser *chooser)
{
	guint title_line = get_chooser_title_line (chooser);
	gcsv_buffer_set_column_titles_line (chooser->buffer, title_line);
}

static void
update_chooser_title_line (GcsvPropertiesChooser *chooser)
{
	GtkTextIter title_iter;
	gint title_line;

	gcsv_buffer_get_column_titles_location (chooser->buffer, &title_iter);
	title_line = gtk_text_iter_get_line (&title_iter);

	g_signal_handlers_block_by_func (chooser->title_spinbutton,
					 title_spinbutton_value_changed_cb,
					 chooser);

	gtk_spin_button_set_value (chooser->title_spinbutton,
				   title_line + 1);

	g_signal_handlers_unblock_by_func (chooser->title_spinbutton,
					   title_spinbutton_value_changed_cb,
					   chooser);
}

static void
column_titles_set_cb (GcsvBuffer            *buffer,
		      GcsvPropertiesChooser *chooser)
{
	update_chooser_title_line (chooser);
}

static void
buffer_changed_cb (GcsvBuffer            *buffer,
		   GcsvPropertiesChooser *chooser)
{
	update_chooser_title_line (chooser);
}

static void
set_buffer (GcsvPropertiesChooser *chooser,
	    GcsvBuffer            *buffer)
{
	g_assert (chooser->buffer == NULL);

	g_return_if_fail (GCSV_IS_BUFFER (buffer));
	chooser->buffer = g_object_ref (buffer);

	g_signal_connect_object (buffer,
				 "notify::delimiter",
				 G_CALLBACK (delimiter_notify_cb),
				 chooser,
				 0);

	g_signal_connect_object (buffer,
				 "column-titles-set",
				 G_CALLBACK (column_titles_set_cb),
				 chooser,
				 0);

	g_signal_connect_object (buffer,
				 "changed",
				 G_CALLBACK (buffer_changed_cb),
				 chooser,
				 0);

	update_chooser_delimiter (chooser);
	update_chooser_title_line (chooser);

	g_object_notify (G_OBJECT (chooser), "buffer");
}

static void
gcsv_properties_chooser_get_property (GObject    *object,
				      guint       prop_id,
				      GValue     *value,
				      GParamSpec *pspec)
{
	GcsvPropertiesChooser *chooser = GCSV_PROPERTIES_CHOOSER (object);

	switch (prop_id)
	{
		case PROP_BUFFER:
			g_value_set_object (value, chooser->buffer);
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
		case PROP_BUFFER:
			set_buffer (chooser, g_value_get_object (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gcsv_properties_chooser_dispose (GObject *object)
{
	GcsvPropertiesChooser *chooser = GCSV_PROPERTIES_CHOOSER (object);

	g_clear_object (&chooser->buffer);

	G_OBJECT_CLASS (gcsv_properties_chooser_parent_class)->dispose (object);
}

static void
gcsv_properties_chooser_class_init (GcsvPropertiesChooserClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->get_property = gcsv_properties_chooser_get_property;
	object_class->set_property = gcsv_properties_chooser_set_property;
	object_class->dispose = gcsv_properties_chooser_dispose;

	g_object_class_install_property (object_class,
					 PROP_BUFFER,
					 g_param_spec_object ("buffer",
							      "Buffer",
							      "",
							      GCSV_TYPE_BUFFER,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY |
							      G_PARAM_STATIC_STRINGS));
}

static void
combo_changed_cb (GtkComboBox           *combo,
		  GcsvPropertiesChooser *chooser)
{
	gtk_widget_set_visible (GTK_WIDGET (chooser->entry),
				g_strcmp0 (gtk_combo_box_get_active_id (combo), ROW_ID_OTHER) == 0);

	gcsv_buffer_set_delimiter (chooser->buffer,
				   get_chooser_delimiter (chooser));
}

static void
entry_changed_cb (GtkEntry              *entry,
		  GcsvPropertiesChooser *chooser)
{
	gcsv_buffer_set_delimiter (chooser->buffer,
				   get_chooser_delimiter (chooser));
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

	gtk_combo_box_set_active_id (GTK_COMBO_BOX (chooser->combo),
				     ROW_ID_DISABLE);

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
gcsv_properties_chooser_new (GcsvBuffer *buffer)
{
	g_return_val_if_fail (GCSV_IS_BUFFER (buffer), NULL);

	return g_object_new (GCSV_TYPE_PROPERTIES_CHOOSER,
			     "buffer", buffer,
			     "margin", 6,
			     NULL);
}
