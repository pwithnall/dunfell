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

#ifndef DFV_VIEWER_WINDOW_H
#define DFV_VIEWER_WINDOW_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

/**
 * DfvViewerWindow:
 *
 * All the fields in this structure are private.
 *
 * Since: UNRELEASED
 */
#define DFV_TYPE_VIEWER_WINDOW dfv_viewer_window_get_type ()
G_DECLARE_FINAL_TYPE (DfvViewerWindow, dfv_viewer_window, DFV, VIEWER_WINDOW,
                      GtkApplicationWindow)

DfvViewerWindow *dfv_viewer_window_new (GtkApplication *application);
DfvViewerWindow *dfv_viewer_window_new_for_file (GtkApplication *application,
                                                 GFile          *file);

const gchar *dfv_viewer_window_get_pane_name (DfvViewerWindow *self);

void dfv_viewer_window_open (DfvViewerWindow *self,
                             GFile           *file);
void dfv_viewer_window_record (DfvViewerWindow *self);

G_END_DECLS

#endif /* !DFV_VIEWER_WINDOW_H */
