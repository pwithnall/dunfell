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

#ifndef DFL_SOURCE_H
#define DFL_SOURCE_H

#include <glib.h>
#include <glib-object.h>

#include "event-sequence.h"
#include "time-sequence.h"

G_BEGIN_DECLS

/**
 * DflSourceDispatchData:
 * @thread_id: TODO
 * @duration: TODO
 * @dispatch_name: (nullable): name of the dispatch function for the #GSource
 *    from #GSourceFuncs
 * @callback_name: (nullable): name of the user callback function set with
 *    g_source_set_callback()
 *
 * TODO
 *
 * Since: UNRELEASED
 */
typedef struct
{
  DflThreadId thread_id;
  DflDuration duration;
  gchar *dispatch_name;  /* owned */
  gchar *callback_name;  /* owned */
} DflSourceDispatchData;

/**
 * DflSource:
 *
 * All the fields in this structure are private.
 *
 * Since: 0.1.0
 */
#define DFL_TYPE_SOURCE dfl_source_get_type ()
G_DECLARE_FINAL_TYPE (DflSource, dfl_source, DFL, SOURCE, GObject)

DflSource *dfl_source_new (DflId        id,
                           DflTimestamp new_timestamp,
                           DflThreadId  new_thread_id);

GPtrArray *dfl_source_factory_from_event_sequence (DflEventSequence *sequence);

DflId dfl_source_get_id (DflSource *self);
const gchar *dfl_source_get_name (DflSource *self);

DflTimestamp dfl_source_get_new_timestamp (DflSource *self);
DflTimestamp dfl_source_get_free_timestamp (DflSource *self);

DflTimestamp dfl_source_get_attach_timestamp (DflSource *self);
DflThreadId  dfl_source_get_attach_thread_id (DflSource *self);
DflTimestamp dfl_source_get_destroy_timestamp (DflSource *self);
DflThreadId  dfl_source_get_destroy_thread_id (DflSource *self);

DflId dfl_source_get_attach_main_context_id (DflSource *self);

DflThreadId dfl_source_get_new_thread_id (DflSource *self);

void dfl_source_dispatch_iter (DflSource           *self,
                               DflTimeSequenceIter *iter,
                               DflTimestamp         start);

G_END_DECLS

#endif /* !DFL_SOURCE_H */
