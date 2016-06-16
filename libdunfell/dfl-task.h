/* vim:set et sw=2 cin cino=t0,f0,(0,{s,>2s,n-s,^-s,e2s: */
/*
 * Copyright © Philip Withnall 2016<philip@tecnocode.co.uk>
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

#ifndef DFL_TASK_H
#define DFL_TASK_H

#include <glib.h>
#include <glib-object.h>

#include "dfl-event-sequence.h"
#include "dfl-time-sequence.h"

G_BEGIN_DECLS

/**
 * DflTask:
 *
 * All the fields in this structure are private.
 *
 * Since: UNRELEASED
 */
#define DFL_TYPE_TASK dfl_task_get_type ()
G_DECLARE_FINAL_TYPE (DflTask, dfl_task, DFL, TASK, GObject)

DflTask *dfl_task_new (DflId        id,
                       DflTimestamp new_timestamp,
                       DflThreadId  new_thread_id);

GPtrArray *dfl_task_factory_from_event_sequence (DflEventSequence *sequence);

DflId dfl_task_get_id (DflTask *self);
DflTimestamp dfl_task_get_new_timestamp (DflTask *self);
DflThreadId dfl_task_get_new_thread_id (DflTask *self);

DflTimestamp dfl_task_get_return_timestamp (DflTask *self);
DflThreadId dfl_task_get_return_thread_id (DflTask *self);

DflTimestamp dfl_task_get_propagate_timestamp (DflTask *self);
DflThreadId dfl_task_get_propagate_thread_id (DflTask *self);

gboolean dfl_task_get_is_thread_cancelled (DflTask *self);
DflTimestamp dfl_task_get_thread_before_timestamp (DflTask *self);
DflTimestamp dfl_task_get_thread_after_timestamp (DflTask *self);
DflThreadId dfl_task_get_thread_id (DflTask *self);

const gchar *dfl_task_get_callback_name (DflTask *self);
const gchar *dfl_task_get_source_tag_name (DflTask *self);

G_END_DECLS

#endif /* !DFL_TASK_H */
