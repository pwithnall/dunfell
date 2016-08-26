/* vim:set et sw=2 cin cino=t0,f0,(0,{s,>2s,n-s,^-s,e2s: */
/*
 * Copyright © Philip Withnall 2015 <philip@tecnocode.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DWL_TIMELINE_H
#define DWL_TIMELINE_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

/**
 * DwlSelectionMovementStep:
 * @DWL_SELECTION_MOVEMENT_SIBLING: Move the selection to a sibling of the
 *    current selection.
 * @DWL_SELECTION_MOVEMENT_ANCESTOR: Move the selection an ancestor or
 *    descendent of the current selection.
 *
 * Since: UNRELEASED
 */
typedef enum
{
  DWL_SELECTION_MOVEMENT_SIBLING,
  DWL_SELECTION_MOVEMENT_ANCESTOR,
} DwlSelectionMovementStep;

/**
 * DwlTimeline:
 *
 * All the fields in this structure are private.
 *
 * Since: 0.1.0
 */
#define DWL_TYPE_TIMELINE dwl_timeline_get_type ()
G_DECLARE_FINAL_TYPE (DwlTimeline, dwl_timeline, DWL, TIMELINE, GtkWidget)

DwlTimeline *dwl_timeline_new (GPtrArray/*<owned DflThread>*/      *threads,
                               GPtrArray/*<owned DflMainContext>*/ *main_contexts,
                               GPtrArray/*<owned DflSource>*/      *sources,
                               GPtrArray/*<owned DflTask>*/        *tasks);

gfloat   dwl_timeline_get_zoom (DwlTimeline *self);
gboolean dwl_timeline_set_zoom (DwlTimeline *self,
                                gfloat       zoom);

G_END_DECLS

#endif /* !DWL_TIMELINE_H */
