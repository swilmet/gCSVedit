#!/bin/sh

# Fetch the GtkTextRegion utility files from GtkSourceView
# but rename the symbols to avoid possible symbol clashes
# if both GtkSourceView and gCSVedit are used in the same application
# with different versons of GtkTextRegion.
# G_GNUC_INTERNAL should protect us, but it could be a no-op with
# some compilers. Besides in theory GtkTextRegion could become
# public API one day.

GSVURL=https://git.gnome.org/browse/gtksourceview/plain/gtksourceview

update_file () {
    _source="${GSVURL}/$1"
    _dest="$2"

    echo "/* Do not edit: this file is generated from ${_source} */" > "${_dest}"
    echo >> "${_dest}"

    # gtksourcetypes-private.h is only needed for INTERNAL that we are removing.
    curl "${_source}" | sed \
        -e '/gtksourcetypes-private.h/d' \
        -e 's/GTK_SOURCE_INTERNAL/G_GNUC_INTERNAL/g' \
        -e 's/gtktextregion/gcsv-text-region/g' \
        -e 's/gtk_text_region/gcsv_text_region/g' \
        -e 's/GtkTextRegion/GcsvTextRegion/g' \
        -e 's/GTK_TEXT_REGION/GCSV_TEXT_REGION/g' >> "${_dest}"
}

update_file "gtktextregion.c" "gcsv-text-region.c"
update_file "gtktextregion.h" "gcsv-text-region.h"
