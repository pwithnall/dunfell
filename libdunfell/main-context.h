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

#ifndef DFL_MAIN_CONTEXT_H
#define DFL_MAIN_CONTEXT_H

#include <glib.h>
#include <glib-object.h>

#include "event-sequence.h"
#include "time-sequence.h"

G_BEGIN_DECLS

/**
 * DflThreadOwnershipData:
 * @thread_id: TODO
 * @duration: TODO
 *
 * TODO
 *
 * Since: 0.1.0
 */
typedef struct
{
  DflThreadId thread_id;
  DflDuration duration;
} DflThreadOwnershipData;

/**
 * DflMainContextDispatchData:
 * @thread_id: TODO
 * @duration: TODO
 *
 * TODO
 *
 * Since: 0.1.0
 */
typedef struct
{
  DflThreadId thread_id;
  DflDuration duration;
} DflMainContextDispatchData;

/**
 * DflMainContext:
 *
 * All the fields in this structure are private.
 *
 * Since: 0.1.0
 */
#define DFL_TYPE_MAIN_CONTEXT dfl_main_context_get_type ()
G_DECLARE_FINAL_TYPE (DflMainContext, dfl_main_context, DFL, MAIN_CONTEXT, GObject)

DflMainContext *dfl_main_context_new (DflId        id,
                                      DflTimestamp new_timestamp);

GPtrArray *dfl_main_context_factory_from_event_sequence (DflEventSequence *sequence);

DflId dfl_main_context_get_id (DflMainContext *self);
DflTimestamp dfl_main_context_get_new_timestamp (DflMainContext *self);
DflTimestamp dfl_main_context_get_free_timestamp (DflMainContext *self);

void dfl_main_context_thread_ownership_iter (DflMainContext      *self,
                                             DflTimeSequenceIter *iter,
                                             DflTimestamp         start);
void dfl_main_context_dispatch_iter (DflMainContext      *self,
                                     DflTimeSequenceIter *iter,
                                     DflTimestamp         start);

gsize dfl_main_context_get_n_thread_switches (DflMainContext *self);

G_END_DECLS

#endif /* !DFL_MAIN_CONTEXT_H */
