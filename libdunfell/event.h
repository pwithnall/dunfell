/* vim:set et sw=2 cin cino=t0,f0,(0,{s,>2s,n-s,^-s,e2s: */
/*
 * Copyright © Philip Withnall 2015, 2016 <philip@tecnocode.co.uk>
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

#ifndef DFL_EVENT_H
#define DFL_EVENT_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include "types.h"

G_BEGIN_DECLS

/**
 * DflEvent:
 *
 * All the fields in this structure are private.
 *
 * Since: 0.1.0
 */
#define DFL_TYPE_EVENT dfl_event_get_type ()
G_DECLARE_FINAL_TYPE (DflEvent, dfl_event, DFL, EVENT, GObject)

DflEvent *dfl_event_new (const gchar         *event_type,
                         DflTimestamp         timestamp,
                         DflThreadId          thread_id,
                         const gchar * const *parameters);

const gchar *dfl_event_get_event_type   (DflEvent *self);
DflTimestamp dfl_event_get_timestamp    (DflEvent *self);
DflThreadId  dfl_event_get_thread_id    (DflEvent *self);

DflId        dfl_event_get_parameter_id (DflEvent *self,
                                         guint     parameter_index);
gint64       dfl_event_get_parameter_int64 (DflEvent *self,
                                            guint     parameter_index);
const gchar *dfl_event_get_parameter_utf8 (DflEvent *self,
                                           guint     parameter_index);

G_END_DECLS

#endif /* !DFL_EVENT_H */
