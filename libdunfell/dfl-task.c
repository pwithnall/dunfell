/* vim:set et sw=2 cin cino=t0,f0,(0,{s,>2s,n-s,^-s,e2s: */
/*
 * Copyright © Philip Withnall 2016 <philip@tecnocode.co.uk>
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

/**
 * SECTION:dfl-task
 * @short_description: model of a #GTask from a log
 * @stability: Unstable
 * @include: libdunfell/dfl-task.h
 *
 * TODO
 *
 * Since: UNRELEASED
 */

#include "config.h"

#include <errno.h>
#include <glib.h>
#include <gio/gio.h>
#include <string.h>

#include "dfl-event.h"
#include "dfl-task.h"
#include "dfl-time-sequence.h"


static void dfl_task_dispose (GObject *object);

struct _DflTask
{
  GObject parent;

  DflId id;

  DflTimestamp new_timestamp;
  DflThreadId new_thread_id;
  DflId source_object;
  DflId cancellable;
  gchar *callback_name;  /* owned */
  DflId callback_data;
  gchar *source_tag_name;  /* owned */
  DflTimestamp return_timestamp;
  DflThreadId return_thread_id;
  DflTimestamp propagate_timestamp;
  DflThreadId propagate_thread_id;
  gboolean returned_error;
  DflThreadId run_in_thread_id;
  DflTimestamp before_run_in_thread_timestamp;
  DflTimestamp after_run_in_thread_timestamp;
  gchar *run_in_thread_name;  /* owned */
  gboolean run_in_thread_cancelled;

  /* NOTE: Task data may be set zero or more times, so needs to be implemented
   * as an iteration when it's implemented. */
};

G_DEFINE_TYPE (DflTask, dfl_task, G_TYPE_OBJECT)

static void
dfl_task_class_init (DflTaskClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = dfl_task_dispose;
}

static void
dfl_task_init (DflTask *self)
{
  /* Nothing here. */
}

static void
dfl_task_dispose (GObject *object)
{
  DflTask *self = DFL_TASK (object);

  g_clear_pointer (&self->callback_name, g_free);
  g_clear_pointer (&self->source_tag_name, g_free);
  g_clear_pointer (&self->run_in_thread_name, g_free);

  /* Chain up to the parent class */
  G_OBJECT_CLASS (dfl_task_parent_class)->dispose (object);
}

/**
 * dfl_task_new:
 * @id: TODO
 * @new_timestamp: TODO
 * @new_thread_id: TODO
 *
 * TODO
 *
 * Returns: (transfer full): a new #DflTask
 * Since: UNRELEASED
 */
DflTask *
dfl_task_new (DflId        id,
              DflTimestamp new_timestamp,
              DflThreadId  new_thread_id)
{
  DflTask *task = NULL;

  /* TODO: Properties */
  task = g_object_new (DFL_TYPE_TASK, NULL);

  task->id = id;
  task->new_timestamp = new_timestamp;
  task->new_thread_id = new_thread_id;

  return task;
}


#include "dfl-event-sequence.h"

static void
task_set_source_tag_cb (DflEventSequence *sequence,
                        DflEvent         *event,
                        gpointer          user_data)
{
  DflTask *task = user_data;

  /* Does this event correspond to the right task? */
  g_assert (dfl_event_get_parameter_id (event, 0) == task->id);

  if (task->source_tag_name != NULL)
    {
      /* TODO: Some better error reporting framework than g_warning(). */
      g_warning ("Saw two g_task_set_source_tag() calls for the same task.");
      g_free (task->source_tag_name);
    }

  task->source_tag_name = g_strdup (dfl_event_get_parameter_utf8 (event, 1));
}

static void
task_before_return_cb (DflEventSequence *sequence,
                       DflEvent         *event,
                       gpointer          user_data)
{
  DflTask *task = user_data;

  /* Does this event correspond to the right task? */
  g_assert (dfl_event_get_parameter_id (event, 0) == task->id);

  if (task->return_timestamp != 0)
    {
      /* TODO: Some better error reporting framework than g_warning(). */
      g_warning ("Saw two g_task_return() calls for the same task.");
    }

  task->return_timestamp = dfl_event_get_timestamp (event);
  task->return_thread_id = dfl_event_get_thread_id (event);
}

static void
task_propagate_cb (DflEventSequence *sequence,
                   DflEvent         *event,
                   gpointer          user_data)
{
  DflTask *task = user_data;
  DflTimestamp timestamp;

  /* Does this event correspond to the right task? */
  g_assert (dfl_event_get_parameter_id (event, 0) == task->id);

  timestamp = dfl_event_get_timestamp (event);

  if (task->propagate_timestamp != 0)
    {
      /* TODO: Some better error reporting framework than g_warning(). */
      g_warning ("Saw two g_task_propagate() calls for the same task.");
    }

  task->propagate_timestamp = timestamp;
  task->returned_error = dfl_event_get_parameter_id (event, 1);
  task->propagate_thread_id = dfl_event_get_thread_id (event);
}

static void
task_before_run_in_thread_cb (DflEventSequence *sequence,
                              DflEvent         *event,
                              gpointer          user_data)
{
  DflTask *task = user_data;

  /* Does this event correspond to the right task? */
  g_assert (dfl_event_get_parameter_id (event, 0) == task->id);

  if (task->before_run_in_thread_timestamp != 0)
    {
      /* TODO: Some better error reporting framework than g_warning(). */
      g_warning ("Saw two g_task_run_in_thread() calls for the same task.");
    }
  else if (task->after_run_in_thread_timestamp != 0)
    {
      g_warning ("Events for g_task_run_in_thread() appeared in the wrong "
                 "order.");
    }

  task->before_run_in_thread_timestamp = dfl_event_get_timestamp (event);
  task->run_in_thread_name = g_strdup (dfl_event_get_parameter_utf8 (event, 1));
  task->run_in_thread_id = dfl_event_get_thread_id (event);
}

static void
task_after_run_in_thread_cb (DflEventSequence *sequence,
                             DflEvent         *event,
                             gpointer          user_data)
{
  DflTask *task = user_data;
  DflTimestamp timestamp;

  /* Does this event correspond to the right task? */
  g_assert (dfl_event_get_parameter_id (event, 0) == task->id);

  timestamp = dfl_event_get_timestamp (event);

  if (task->after_run_in_thread_timestamp != 0)
    {
      /* TODO: Some better error reporting framework than g_warning(). */
      g_warning ("Saw two g_task_run_in_thread() calls for the same task.");
    }
  else if (task->before_run_in_thread_timestamp == 0 ||
           task->before_run_in_thread_timestamp < timestamp)
    {
      g_warning ("Events for g_task_run_in_thread() appeared in the wrong "
                 "order.");
    }

  task->after_run_in_thread_timestamp = timestamp;
  task->run_in_thread_cancelled = dfl_event_get_parameter_id (event, 1);
}

static void
task_new_cb (DflEventSequence *sequence,
             DflEvent         *event,
             gpointer          user_data)
{
  GPtrArray/*<owned DflTask>*/ *tasks = user_data;
  DflTask *task = NULL;
  DflId task_id;

  task_id = dfl_event_get_parameter_id (event, 0);
  task = dfl_task_new (task_id, dfl_event_get_timestamp (event),
                       dfl_event_get_thread_id (event));

  task->source_object = dfl_event_get_parameter_id (event, 1);
  task->cancellable = dfl_event_get_parameter_id (event, 2);
  task->callback_name = g_strdup (dfl_event_get_parameter_utf8 (event, 3));
  task->callback_data = dfl_event_get_parameter_id (event, 4);

  dfl_event_sequence_start_walker_group (sequence);

  dfl_event_sequence_add_walker (sequence, "g_task_set_source_tag",
                                 task_id,
                                 task_set_source_tag_cb,
                                 g_object_ref (task),
                                 (GDestroyNotify) g_object_unref);
  dfl_event_sequence_add_walker (sequence, "g_task_before_return",
                                 task_id,
                                 task_before_return_cb,
                                 g_object_ref (task),
                                 (GDestroyNotify) g_object_unref);
  dfl_event_sequence_add_walker (sequence, "g_task_propagate",
                                 task_id,
                                 task_propagate_cb,
                                 g_object_ref (task),
                                 (GDestroyNotify) g_object_unref);
  dfl_event_sequence_add_walker (sequence, "g_task_before_run_in_thread",
                                 task_id,
                                 task_before_run_in_thread_cb,
                                 g_object_ref (task),
                                 (GDestroyNotify) g_object_unref);
  dfl_event_sequence_add_walker (sequence, "g_task_after_run_in_thread",
                                 task_id,
                                 task_after_run_in_thread_cb,
                                 g_object_ref (task),
                                 (GDestroyNotify) g_object_unref);

  dfl_event_sequence_end_walker_group (sequence, "g_task_propagate", task_id);

  g_ptr_array_add (tasks, task);  /* transfer */
}

/**
 * dfl_task_factory_from_event_sequence:
 * @sequence: an event sequence to analyse
 *
 * TODO
 *
 * Returns: (transfer full) (element-type DflTask): an array of zero or
 *    more #DflTasks
 * Since: UNRELEASED
 */
GPtrArray *
dfl_task_factory_from_event_sequence (DflEventSequence *sequence)
{
  GPtrArray/*<owned DflTask>*/ *tasks = NULL;

  tasks = g_ptr_array_new_with_free_func (g_object_unref);
  dfl_event_sequence_add_walker (sequence, "g_task_new", DFL_ID_INVALID,
                                 task_new_cb,
                                 g_ptr_array_ref (tasks),
                                 (GDestroyNotify) g_ptr_array_unref);

  return tasks;
}

/**
 * dfl_task_get_id:
 * @self: a #DflTask
 *
 * TODO
 *
 * Returns: TODO
 * Since: UNRELEASED
 */
DflId
dfl_task_get_id (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), DFL_ID_INVALID);

  return self->id;
}

/**
 * dfl_task_get_new_timestamp:
 * @self: a #DflTask
 *
 * TODO
 *
 * Returns: TODO
 * Since: UNRELEASED
 */
DflTimestamp
dfl_task_get_new_timestamp (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), 0);

  return self->new_timestamp;
}

/* TODO */
DflThreadId
dfl_task_get_new_thread_id (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), DFL_ID_INVALID);

  return self->new_thread_id;
}

/* TODO */
DflTimestamp
dfl_task_get_return_timestamp (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), 0);

  return self->return_timestamp;
}

/* TODO */
DflThreadId
dfl_task_get_return_thread_id (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), DFL_ID_INVALID);

  return self->return_thread_id;
}

/* TODO */
DflTimestamp
dfl_task_get_propagate_timestamp (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), 0);

  return self->propagate_timestamp;
}

/* TODO */
DflThreadId
dfl_task_get_propagate_thread_id (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), DFL_ID_INVALID);

  return self->propagate_thread_id;
}

/* TODO */
gboolean
dfl_task_get_is_thread_cancelled (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), FALSE);

  return self->run_in_thread_cancelled;
}

/* TODO */
DflTimestamp
dfl_task_get_thread_before_timestamp (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), 0);

  return self->before_run_in_thread_timestamp;
}

/* TODO */
DflTimestamp
dfl_task_get_thread_after_timestamp (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), 0);

  return self->after_run_in_thread_timestamp;
}

/* TODO */
DflThreadId
dfl_task_get_thread_id (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), DFL_ID_INVALID);

  return self->run_in_thread_id;
}

/* TODO */
const gchar *
dfl_task_get_callback_name (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), NULL);

  return self->callback_name;
}

/* TODO */
const gchar *
dfl_task_get_source_tag_name (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), NULL);

  return self->source_tag_name;
}
