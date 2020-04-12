/*
 * This file is part of gCSVedit.
 *
 * Copyright 2020 - Sébastien Wilmet <swilmet@gnome.org>
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

#ifndef GCSV_TAB_H
#define GCSV_TAB_H

#include <tepl/tepl.h>
#include "gcsv-alignment.h"

G_BEGIN_DECLS

#define GCSV_TYPE_TAB             (gcsv_tab_get_type ())
#define GCSV_TAB(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GCSV_TYPE_TAB, GcsvTab))
#define GCSV_TAB_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GCSV_TYPE_TAB, GcsvTabClass))
#define GCSV_IS_TAB(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GCSV_TYPE_TAB))
#define GCSV_IS_TAB_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GCSV_TYPE_TAB))
#define GCSV_TAB_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GCSV_TYPE_TAB, GcsvTabClass))

typedef struct _GcsvTab         GcsvTab;
typedef struct _GcsvTabClass    GcsvTabClass;
typedef struct _GcsvTabPrivate  GcsvTabPrivate;

struct _GcsvTab
{
	TeplTab parent;

	GcsvTabPrivate *priv;
};

struct _GcsvTabClass
{
	TeplTabClass parent_class;
};

GType		gcsv_tab_get_type	(void);

GcsvTab *	gcsv_tab_new		(void);

void		gcsv_tab_load_file	(GcsvTab *tab,
					 GFile   *location);

void		gcsv_tab_save		(GcsvTab *tab);

void		gcsv_tab_save_as	(GcsvTab *tab,
					 GFile   *target_location);

G_END_DECLS

#endif /* GCSV_TAB_H */
