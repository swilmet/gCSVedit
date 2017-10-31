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

#ifndef GCSV_FACTORY_H
#define GCSV_FACTORY_H

#include <tepl/tepl.h>

G_BEGIN_DECLS

#define GCSV_TYPE_FACTORY             (gcsv_factory_get_type ())
#define GCSV_FACTORY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCSV_TYPE_FACTORY, GcsvFactory))
#define GCSV_FACTORY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GCSV_TYPE_FACTORY, GcsvFactoryClass))
#define GCSV_IS_FACTORY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCSV_TYPE_FACTORY))
#define GCSV_IS_FACTORY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GCSV_TYPE_FACTORY))
#define GCSV_FACTORY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GCSV_TYPE_FACTORY, GcsvFactoryClass))

typedef struct _GcsvFactory         GcsvFactory;
typedef struct _GcsvFactoryClass    GcsvFactoryClass;

struct _GcsvFactory
{
	TeplAbstractFactory parent;
};

struct _GcsvFactoryClass
{
	TeplAbstractFactoryClass parent_class;
};

GType		gcsv_factory_get_type	(void);

GcsvFactory *	gcsv_factory_new	(void);

G_END_DECLS

#endif /* GCSV_FACTORY_H */
