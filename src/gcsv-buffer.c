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

#include "gcsv-buffer.h"

struct _GcsvBuffer
{
	GtkSourceBuffer parent;

	/* The delimiter is at most one Unicode character. If it is the nul
	 * character ('\0'), there is no alignment.
	 */
	gunichar delimiter;
};

enum
{
	PROP_0,
	PROP_DELIMITER,
};

G_DEFINE_TYPE (GcsvBuffer, gcsv_buffer, GTK_SOURCE_TYPE_BUFFER)

static void
gcsv_buffer_get_property (GObject    *object,
			  guint       prop_id,
			  GValue     *value,
			  GParamSpec *pspec)
{
	GcsvBuffer *buffer = GCSV_BUFFER (object);

	switch (prop_id)
	{
		case PROP_DELIMITER:
			g_value_set_uint (value, buffer->delimiter);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gcsv_buffer_set_property (GObject      *object,
			  guint         prop_id,
			  const GValue *value,
			  GParamSpec   *pspec)
{
	GcsvBuffer *buffer = GCSV_BUFFER (object);

	switch (prop_id)
	{
		case PROP_DELIMITER:
			buffer->delimiter = g_value_get_uint (value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gcsv_buffer_dispose (GObject *object)
{

	G_OBJECT_CLASS (gcsv_buffer_parent_class)->dispose (object);
}

static void
gcsv_buffer_class_init (GcsvBufferClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->get_property = gcsv_buffer_get_property;
	object_class->set_property = gcsv_buffer_set_property;
	object_class->dispose = gcsv_buffer_dispose;

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
gcsv_buffer_init (GcsvBuffer *buffer)
{
}

GcsvBuffer *
gcsv_buffer_new (void)
{
	return g_object_new (GCSV_TYPE_BUFFER, NULL);
}
