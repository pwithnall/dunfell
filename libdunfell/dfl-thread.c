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

/**
 * SECTION:dfl-thread
 * @short_description: model of a #GMainContext from a log
 * @stability: Unstable
 * @include: libdunfell/dfl-thread.h
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
#include "dfl-thread.h"
#include "dfl-time-sequence.h"


struct _DflThread
{
  GObject parent;

  DflId id;

  DflTimestamp new_timestamp;
  DflTimestamp free_timestamp;
};

G_DEFINE_TYPE (DflThread, dfl_thread, G_TYPE_OBJECT)

static void
dfl_thread_class_init (DflThreadClass *klass)
{
  /* Nothing to see here. */
}

static void
dfl_thread_init (DflThread *self)
{
  /* Nothing to see here. */
}

/**
 * dfl_thread_new:
 * @id: TODO
 * @new_timestamp: TODO
 *
 * TODO
 *
 * Returns: (transfer full): a new #DflThread
 * Since: UNRELEASED
 */
DflThread *
dfl_thread_new (DflId        id,
                DflTimestamp new_timestamp)
{
  DflThread *thread = NULL;

  thread = g_object_new (DFL_TYPE_THREAD, NULL);

  /* TODO: Use properties properly. */
  thread->id = id;
  thread->new_timestamp = new_timestamp;

  return thread;
}


#include "dfl-event-sequence.h"

static void
event_cb (DflEventSequence *sequence,
          DflEvent         *event,
          gpointer          user_data)
{
  GPtrArray/*<owned DflThread>*/ *threads = user_data;
  DflThread *thread = NULL;
  DflThreadId thread_id;
  guint i;

  thread_id = dfl_event_get_thread_id (event);

  /* Check the ID doesn’t already exist. */
  for (i = 0; i < threads->len; i++)
    {
      DflThread *t = threads->pdata[i];

      if (t->id == thread_id)
        return;
    }

  thread = dfl_thread_new (thread_id, dfl_event_get_timestamp (event));
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
 * Since: UNRELEASED
 */
GPtrArray *
dfl_thread_factory_from_event_sequence (DflEventSequence *sequence)
{
  GPtrArray/*<owned DflThread>*/ *threads = NULL;

  threads = g_ptr_array_new_with_free_func (g_object_unref);
  dfl_event_sequence_add_walker (sequence, NULL, event_cb,
                                 g_ptr_array_ref (threads),
                                 (GDestroyNotify) g_ptr_array_unref);

  return threads;
}
