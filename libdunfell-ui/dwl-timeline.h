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
 * DwlTimeline:
 *
 * All the fields in this structure are private.
 *
 * Since: UNRELEASED
 */
#define DWL_TYPE_TIMELINE dwl_timeline_get_type ()
G_DECLARE_FINAL_TYPE (DwlTimeline, dwl_timeline, DWL, TIMELINE, GtkWidget)

DwlTimeline *dwl_timeline_new (GPtrArray/*<owned DflThread>*/      *threads,
                               GPtrArray/*<owned DflMainContext>*/ *main_contexts);

G_END_DECLS

#endif /* !DWL_TIMELINE_H */
