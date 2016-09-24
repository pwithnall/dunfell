/* vim:set et sw=2 cin cino=t0,f0,(0,{s,>2s,n-s,^-s,e2s: */
/*
 * Copyright © Collabora Ltd. 2016
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

#ifndef DWL_STATISTICS_PANE_H
#define DWL_STATISTICS_PANE_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include <libdunfell/model.h>

G_BEGIN_DECLS

/**
 * DwlStatisticsPane:
 *
 * All the fields in this structure are private.
 *
 * Since: UNRELEASED
 */
#define DWL_TYPE_STATISTICS_PANE dwl_statistics_pane_get_type ()
G_DECLARE_FINAL_TYPE (DwlStatisticsPane, dwl_statistics_pane,
                      DWL, STATISTICS_PANE, GtkBin)

DwlStatisticsPane *dwl_statistics_pane_new                 (DflModel          *model);
void               dwl_statistics_pane_set_selected_object (DwlStatisticsPane *self,
                                                            GObject           *obj);

G_END_DECLS

#endif /* !DWL_STATISTICS_PANE_H */
