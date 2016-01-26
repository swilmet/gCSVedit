/* Do not edit: this file is generated from https://git.gnome.org/browse/gtksourceview/plain/gtksourceview/gtktextregion.h */

/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 * gcsv-text-region.h - GtkTextMark based region utility functions
 * This file is part of GtkSourceView
 *
 * Copyright (C) 2002 Gustavo Giráldez <gustavo.giraldez@gmx.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __GCSV_TEXT_REGION_H__
#define __GCSV_TEXT_REGION_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _GcsvTextRegion		GcsvTextRegion;
typedef struct _GcsvTextRegionIterator	GcsvTextRegionIterator;

struct _GcsvTextRegionIterator {
	/* GcsvTextRegionIterator is an opaque datatype; ignore all these fields.
	 * Initialize the iter with gcsv_text_region_get_iterator
	 * function
	 */
	/*< private >*/
	gpointer dummy1;
	guint32  dummy2;
	gpointer dummy3;
};

G_GNUC_INTERNAL
GcsvTextRegion *gcsv_text_region_new                          (GtkTextBuffer *buffer);

G_GNUC_INTERNAL
void           gcsv_text_region_destroy                      (GcsvTextRegion *region);

G_GNUC_INTERNAL
GtkTextBuffer *gcsv_text_region_get_buffer                   (GcsvTextRegion *region);

G_GNUC_INTERNAL
void           gcsv_text_region_add                          (GcsvTextRegion     *region,
							     const GtkTextIter *_start,
							     const GtkTextIter *_end);

G_GNUC_INTERNAL
void           gcsv_text_region_subtract                     (GcsvTextRegion     *region,
							     const GtkTextIter *_start,
							     const GtkTextIter *_end);

G_GNUC_INTERNAL
gint           gcsv_text_region_subregions                   (GcsvTextRegion *region);

G_GNUC_INTERNAL
gboolean       gcsv_text_region_nth_subregion                (GcsvTextRegion *region,
							     guint          subregion,
							     GtkTextIter   *start,
							     GtkTextIter   *end);

G_GNUC_INTERNAL
GcsvTextRegion *gcsv_text_region_intersect                    (GcsvTextRegion     *region,
							     const GtkTextIter *_start,
							     const GtkTextIter *_end);

G_GNUC_INTERNAL
void           gcsv_text_region_get_iterator                 (GcsvTextRegion         *region,
                                                             GcsvTextRegionIterator *iter,
                                                             guint                  start);

G_GNUC_INTERNAL
gboolean       gcsv_text_region_iterator_is_end              (GcsvTextRegionIterator *iter);

/* Returns FALSE if iterator is the end iterator */
G_GNUC_INTERNAL
gboolean       gcsv_text_region_iterator_next	            (GcsvTextRegionIterator *iter);

G_GNUC_INTERNAL
gboolean       gcsv_text_region_iterator_get_subregion       (GcsvTextRegionIterator *iter,
							     GtkTextIter           *start,
							     GtkTextIter           *end);

G_GNUC_INTERNAL
void           gcsv_text_region_debug_print                  (GcsvTextRegion *region);

G_END_DECLS

#endif /* __GCSV_TEXT_REGION_H__ */
