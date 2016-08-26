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

/**
 * SECTION:thread
 * @short_description: model of a #GMainContext from a log
 * @stability: Unstable
 * @include: libdunfell/thread.h
 *
 * TODO
 *
 * Since: 0.1.0
 */

#include "config.h"

#include <errno.h>
#include <glib.h>
#include <gio/gio.h>
#include <string.h>

#include "event.h"
#include "event-sequence.h"
#include "thread.h"
#include "time-sequence.h"


static void dfl_thread_finalize (GObject *object);

struct _DflThread
{
  GObject parent;

  DflThreadId id;

  /* Invariant: @free_timestamp >= @new_timestamp. */
  DflTimestamp new_timestamp;
  DflTimestamp free_timestamp;

  gchar *name;  /* owned; nullable */
};

G_DEFINE_TYPE (DflThread, dfl_thread, G_TYPE_OBJECT)

static void
dfl_thread_class_init (DflThreadClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->finalize = dfl_thread_finalize;
}

static void
dfl_thread_init (DflThread *self)
{
  /* Nothing to see here. */
}

static void
dfl_thread_finalize (GObject *object)
{
  DflThread *self = DFL_THREAD (object);

  g_free (self->name);

  G_OBJECT_CLASS (dfl_thread_parent_class)->finalize (object);
}

/**
 * dfl_thread_new:
 * @id: TODO
 * @new_timestamp: TODO
 * @name: (nullable): debugging name for the thread, or %NULL if it doesn’t
 *    have one
 *
 * TODO
 *
 * Returns: (transfer full): a new #DflThread
 * Since: UNRELEASED
 */
DflThread *
dfl_thread_new (DflThreadId   id,
                DflTimestamp  new_timestamp,
                const gchar  *name)
{
  DflThread *thread = NULL;

  thread = g_object_new (DFL_TYPE_THREAD, NULL);

  /* TODO: Use properties properly. */
  thread->id = id;
  thread->new_timestamp = new_timestamp;
  thread->free_timestamp = new_timestamp;
  thread->name = g_strdup (name);

  return thread;
}

static void
event_cb (DflEventSequence *sequence,
          DflEvent         *event,
          gpointer          user_data)
{
  GPtrArray/*<owned DflThread>*/ *threads = user_data;
  DflThread *thread = NULL;
  DflThreadId thread_id;
  guint i;
  const gchar *name = NULL;

  thread_id = dfl_event_get_thread_id (event);

  /* Check the ID doesn’t already exist. If it does, update its final
   * timestamp. */
  for (i = 0; i < threads->len; i++)
    {
      DflThread *t = threads->pdata[i];

      if (t->id == thread_id)
        {
          t->free_timestamp = dfl_event_get_timestamp (event);
          return;
        }
    }

  /* We can know the thread’s nickname if it was detected from a
   * g_thread_spawned event. */
  if (dfl_event_get_event_type (event) ==
      g_intern_static_string ("g_thread_spawned"))
    name = dfl_event_get_parameter_utf8 (event, 2);

  thread = dfl_thread_new (thread_id, dfl_event_get_timestamp (event), name);
  g_ptr_array_add (threads, thread);  /* transfer */
}

/**
 * dfl_thread_factory_from_event_sequence:
 * @sequence: an event sequence to analyse
 *
 * TODO
 *
 * Returns: (transfer full) (element-type DflThread): an array of zero or more
 *    #DflThreads
 * Since: 0.1.0
 */
GPtrArray *
dfl_thread_factory_from_event_sequence (DflEventSequence *sequence)
{
  GPtrArray/*<owned DflThread>*/ *threads = NULL;

  threads = g_ptr_array_new_with_free_func (g_object_unref);
  dfl_event_sequence_add_walker (sequence, NULL, DFL_ID_INVALID, event_cb,
                                 g_ptr_array_ref (threads),
                                 (GDestroyNotify) g_ptr_array_unref);

  return threads;
}

/**
 * dfl_thread_get_id:
 * @self: a #DflThread
 *
 * TODO
 *
 * Returns: TODO
 * Since: 0.1.0
 */
DflThreadId
dfl_thread_get_id (DflThread *self)
{
  g_return_val_if_fail (DFL_IS_THREAD (self), DFL_ID_INVALID);

  return self->id;
}

/**
 * dfl_thread_get_name:
 * @self: a #DflThread
 *
 * TODO
 *
 * Returns: TODO
 * Since: UNRELEASED
 */
const gchar *
dfl_thread_get_name (DflThread *self)
{
  g_return_val_if_fail (DFL_IS_THREAD (self), NULL);

  return self->name;
}

/**
 * dfl_thread_get_new_timestamp:
 * @self: a #DflThread
 *
 * TODO
 *
 * Returns: TODO
 * Since: 0.1.0
 */
DflTimestamp
dfl_thread_get_new_timestamp (DflThread *self)
{
  g_return_val_if_fail (DFL_IS_THREAD (self), 0);

  return self->new_timestamp;
}

/**
 * dfl_thread_get_free_timestamp:
 * @self: a #DflThread
 *
 * TODO
 *
 * Returns: TODO
 * Since: 0.1.0
 */
DflTimestamp
dfl_thread_get_free_timestamp (DflThread *self)
{
  g_return_val_if_fail (DFL_IS_THREAD (self), 0);

  return self->free_timestamp;
}
