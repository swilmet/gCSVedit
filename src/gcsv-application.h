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

#ifndef GCSV_APPLICATION_H
#define GCSV_APPLICATION_H

#include <gtef/gtef.h>

G_BEGIN_DECLS

#define GCSV_TYPE_APPLICATION             (gcsv_application_get_type ())
#define GCSV_APPLICATION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCSV_TYPE_APPLICATION, GcsvApplication))
#define GCSV_APPLICATION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GCSV_TYPE_APPLICATION, GcsvApplicationClass))
#define GCSV_IS_APPLICATION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCSV_TYPE_APPLICATION))
#define GCSV_IS_APPLICATION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GCSV_TYPE_APPLICATION))
#define GCSV_APPLICATION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GCSV_TYPE_APPLICATION, GcsvApplicationClass))

typedef struct _GcsvApplication         GcsvApplication;
typedef struct _GcsvApplicationClass    GcsvApplicationClass;
typedef struct _GcsvApplicationPrivate  GcsvApplicationPrivate;

struct _GcsvApplication
{
	GtkApplication parent;

	GcsvApplicationPrivate *priv;
};

struct _GcsvApplicationClass
{
	GtkApplicationClass parent_class;
};

GType			gcsv_application_get_type			(void);

GcsvApplication *	gcsv_application_new				(void);

GtefActionInfoStore *	gcsv_application_get_action_info_store		(GcsvApplication *app);

G_END_DECLS

#endif /* GCSV_APPLICATION_H */
