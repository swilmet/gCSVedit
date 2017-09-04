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

#ifndef GCSV_UTILS_H
#define GCSV_UTILS_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

void		gcsv_utils_delete_text_with_tag		(GtkTextBuffer     *buffer,
							 const GtkTextIter *start,
							 const GtkTextIter *end,
							 GtkTextTag        *tag);

gulong *	gcsv_utils_block_all_signal_handlers	(GObject     *instance,
							 const gchar *signal_name);

void		gcsv_utils_unblock_signal_handlers	(GObject      *instance,
							 const gulong *handler_ids);

void		gcsv_warning				(GtkWindow   *parent,
							 const gchar *format,
							 ...) G_GNUC_PRINTF(2, 3);

G_END_DECLS

#endif /* GCSV_UTILS_H */
