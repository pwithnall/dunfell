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
 * SECTION:source
 * @short_description: model of a #GMainContext from a log
 * @stability: Unstable
 * @include: libdunfell/source.h
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
#include "source.h"
#include "time-sequence.h"


static void dfl_source_dispose (GObject *object);
static void dfl_source_dispatch_data_clear (DflSourceDispatchData *data);

struct _DflSource
{
  GObject parent;

  /* Note: Since this is derived from the #GSource’s address in memory, it is
   * not necessarily unique over the lifetime of the recorded process. */
  DflId id;

  /* Invariant: @free_timestamp ≥ @new_timestamp. */
  DflTimestamp new_timestamp;
  DflTimestamp free_timestamp;

  DflThreadId new_thread_id;

  /* Sequence of thread IDs and the duration between the start and end of the
   * dispatch. A duration of ≥ 0 is valid; < 0 is not. */
  DflTimeSequence/*<DflMainContextDispatchData>*/ dispatch_events;

  gchar *name;  /* owned; nullable */

  DflId attach_context;
  DflTimestamp attach_timestamp;
  DflThreadId attach_thread_id;
  DflTimestamp destroy_timestamp;
  DflThreadId destroy_thread_id;
};

G_DEFINE_TYPE (DflSource, dfl_source, G_TYPE_OBJECT)

static void
dfl_source_class_init (DflSourceClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->dispose = dfl_source_dispose;
}

static void
dfl_source_init (DflSource *self)
{
  dfl_time_sequence_init (&self->dispatch_events,
                          sizeof (DflSourceDispatchData),
                          (GDestroyNotify) dfl_source_dispatch_data_clear, 0);
}

static void
dfl_source_dispose (GObject *object)
{
  DflSource *self = DFL_SOURCE (object);

  dfl_time_sequence_clear (&self->dispatch_events);
  g_clear_pointer (&self->name, g_free);

  /* Chain up to the parent class */
  G_OBJECT_CLASS (dfl_source_parent_class)->dispose (object);
}

static void
dfl_source_dispatch_data_clear (DflSourceDispatchData *data)
{
  g_free (data->dispatch_name);
  g_free (data->callback_name);
}

/**
 * dfl_source_new:
 * @id: TODO
 * @new_timestamp: TODO
 * @new_thread_id: TODO
 *
 * TODO
 *
 * Returns: (transfer full): a new #DflSource
 * Since: 0.1.0
 */
DflSource *
dfl_source_new (DflId        id,
                DflTimestamp new_timestamp,
                DflThreadId  new_thread_id)
{
  DflSource *source = NULL;

  source = g_object_new (DFL_TYPE_SOURCE, NULL);

  /* TODO: Use properties properly. */
  source->id = id;
  source->new_timestamp = new_timestamp;
  source->new_thread_id = new_thread_id;
  source->free_timestamp = new_timestamp;

  return source;
}

static void
source_before_after_dispatch_cb (DflEventSequence *sequence,
                                 DflEvent         *event,
                                 gpointer          user_data)
{
  DflSource *source = user_data;
  gboolean is_before;
  DflTimestamp timestamp;
  DflThreadId thread_id;

  /* Does this event correspond to the right source? */
  g_assert (dfl_event_get_parameter_id (event, 0) == source->id);

  is_before = (dfl_event_get_event_type (event) ==
               g_intern_static_string ("g_source_before_dispatch"));
  timestamp = dfl_event_get_timestamp (event);
  thread_id = dfl_event_get_thread_id (event);

  if (is_before)
    {
      DflSourceDispatchData *last_element;
      DflTimestamp last_timestamp;
      DflSourceDispatchData *next_element;
      const gchar *dispatch_name, *callback_name;

      dispatch_name = dfl_event_get_parameter_utf8 (event, 1);
      callback_name = dfl_event_get_parameter_utf8 (event, 2);

      /* Check that the previous element in the sequence has a valid (non-zero)
       * duration, otherwise no //after// event was logged and something very
       * odd is happening.
       *
       * @last_element will be %NULL if this is the first //before//. */
      last_element = dfl_time_sequence_get_last_element (&source->dispatch_events,
                                                         &last_timestamp);

      if (last_element != NULL && last_element->duration < 0)
        {
          /* TODO: Some better error reporting framework than g_warning(). */
          g_warning ("Saw two g_source_dispatch() calls in a row for the "
                     "same context with no finish in between.");

          /* Fudge it. */
          last_element->duration = timestamp - last_timestamp;
        }

      /* Start the next element. */
      next_element = dfl_time_sequence_append (&source->dispatch_events,
                                               timestamp);
      next_element->thread_id = thread_id;
      next_element->duration = -1;  /* will be set by the paired //after// */
      next_element->dispatch_name = g_strdup (dispatch_name);
      next_element->callback_name = g_strdup (callback_name);
    }
  else
    {
      DflSourceDispatchData *last_element;
      DflTimestamp last_timestamp;

      /* Check that the previous element in the sequence has an invalid (zero)
       * duration, otherwise no //before// event was logged.
       *
       * @last_element should only be %NULL if something odd is happening. */
      last_element = dfl_time_sequence_get_last_element (&source->dispatch_events,
                                                         &last_timestamp);

      if (last_element == NULL)
        {
          /* TODO: Some better error reporting framework than g_warning(). */
          g_warning ("Saw a g_main_context_dispatch() call finish for a "
                     "context with no g_main_context_dispatch() call starting "
                     "beforehand.");

          /* Fudge it. */
          last_element = dfl_time_sequence_append (&source->dispatch_events,
                                                   timestamp);
          last_timestamp = timestamp;
          last_element->thread_id = thread_id;
          last_element->duration = -1;
          last_element->dispatch_name = NULL;
          last_element->callback_name = NULL;
        }
      else if (last_element->duration >= 0)
        {
          /* TODO: Some better error reporting framework than g_warning(). */
          g_warning ("Saw two g_source_dispatch() calls finish in a row "
                     "for the same context with no g_source_dispatch() "
                     "start in between.");
        }
      else if (last_element->thread_id != thread_id)
        {
          /* TODO: Some better error reporting framework than g_warning(). */
          g_warning ("Saw a g_source_dispatch() call finish for one "
                     "thread immediately following a "
                     "g_source_dispatch() call for a different one, "
                     "both on the same context.");

          /* Fudge it. */
          last_element->duration = timestamp - last_timestamp;

          last_element = dfl_time_sequence_append (&source->dispatch_events,
                                                   timestamp);
          last_timestamp = timestamp;
          last_element->thread_id = thread_id;
          last_element->duration = -1;
          last_element->dispatch_name = NULL;
          last_element->callback_name = NULL;
        }

      /* Update the element’s duration. */
      last_element->duration = timestamp - last_timestamp;
    }
}

static void
source_before_free_cb (DflEventSequence *sequence,
                       DflEvent         *event,
                       gpointer          user_data)
{
  DflSource *source = user_data;
  DflTimestamp timestamp;

  /* Does this event correspond to the right source? */
  g_assert (dfl_event_get_parameter_id (event, 0) == source->id);

  if (source->free_timestamp != 0)
    return;

  timestamp = dfl_event_get_timestamp (event);
  source->free_timestamp = timestamp;
}

static void
source_set_name_cb (DflEventSequence *sequence,
                    DflEvent         *event,
                    gpointer          user_data)
{
  DflSource *source = user_data;
  const gchar *name;

  /* Does this event correspond to the right source? */
  g_assert (dfl_event_get_parameter_id (event, 0) == source->id);

  if (source->name != NULL)
    return;

  name = dfl_event_get_parameter_utf8 (event, 1);
  source->name = g_strdup (name);
}

static void
source_attach_cb (DflEventSequence *sequence,
                  DflEvent         *event,
                  gpointer          user_data)
{
  DflSource *source = user_data;
  DflTimestamp timestamp;

  /* Does this event correspond to the right source? */
  g_assert (dfl_event_get_parameter_id (event, 0) == source->id);

  if (source->attach_timestamp != 0)
    return;

  timestamp = dfl_event_get_timestamp (event);
  source->attach_timestamp = timestamp;
  source->attach_context = dfl_event_get_parameter_id (event, 1);
  source->attach_thread_id = dfl_event_get_thread_id (event);
}

static void
source_destroy_cb (DflEventSequence *sequence,
                   DflEvent         *event,
                   gpointer          user_data)
{
  DflSource *source = user_data;
  DflTimestamp timestamp;

  /* Does this event correspond to the right source? */
  g_assert (dfl_event_get_parameter_id (event, 0) == source->id);

  if (source->destroy_timestamp != 0)
    return;

  timestamp = dfl_event_get_timestamp (event);
  source->destroy_timestamp = timestamp;
  source->destroy_thread_id = dfl_event_get_thread_id (event);
}

static void
source_new_cb (DflEventSequence *sequence,
               DflEvent         *event,
               gpointer          user_data)
{
  GPtrArray/*<owned DflSource>*/ *sources = user_data;
  DflSource *source = NULL;
  DflId source_id;

  source_id = dfl_event_get_parameter_id (event, 0);
  source = dfl_source_new (source_id,
                           dfl_event_get_timestamp (event),
                           dfl_event_get_thread_id (event));

  dfl_event_sequence_start_walker_group (sequence);

  dfl_event_sequence_add_walker (sequence, "g_source_set_name", source_id,
                                 source_set_name_cb,
                                 g_object_ref (source),
                                 (GDestroyNotify) g_object_unref);
  dfl_event_sequence_add_walker (sequence, "g_source_before_free", source_id,
                                 source_before_free_cb,
                                 g_object_ref (source),
                                 (GDestroyNotify) g_object_unref);
  dfl_event_sequence_add_walker (sequence, "g_source_before_dispatch",
                                 source_id,
                                 source_before_after_dispatch_cb,
                                 g_object_ref (source),
                                 (GDestroyNotify) g_object_unref);
  dfl_event_sequence_add_walker (sequence, "g_source_after_dispatch", source_id,
                                 source_before_after_dispatch_cb,
                                 g_object_ref (source),
                                 (GDestroyNotify) g_object_unref);
  dfl_event_sequence_add_walker (sequence, "g_source_attach", source_id,
                                 source_attach_cb,
                                 g_object_ref (source),
                                 (GDestroyNotify) g_object_unref);
  dfl_event_sequence_add_walker (sequence, "g_source_destroy", source_id,
                                 source_destroy_cb,
                                 g_object_ref (source),
                                 (GDestroyNotify) g_object_unref);

  /* Remove the walkers once the source is destroyed. */
  dfl_event_sequence_end_walker_group (sequence, "g_source_before_free",
                                       source_id);

  g_ptr_array_add (sources, source);  /* transfer */
}

/**
 * dfl_source_factory_from_event_sequence:
 * @sequence: an event sequence to analyse
 *
 * TODO
 *
 * Returns: (transfer full) (element-type DflSource): an array of zero or more
 *    #DflSources
 * Since: 0.1.0
 */
GPtrArray *
dfl_source_factory_from_event_sequence (DflEventSequence *sequence)
{
  GPtrArray/*<owned DflSource>*/ *sources = NULL;

  sources = g_ptr_array_new_with_free_func (g_object_unref);
  dfl_event_sequence_add_walker (sequence, "g_source_new", DFL_ID_INVALID,
                                 source_new_cb,
                                 g_ptr_array_ref (sources),
                                 (GDestroyNotify) g_ptr_array_unref);

  return sources;
}

/**
 * dfl_source_get_id:
 * @self: a #DflSource
 *
 * TODO
 *
 * Returns: TODO
 * Since: 0.1.0
 */
DflId
dfl_source_get_id (DflSource *self)
{
  g_return_val_if_fail (DFL_IS_SOURCE (self), DFL_ID_INVALID);

  return self->id;
}

/**
 * dfl_source_get_name:
 * @self: a #DflSource
 *
 * TODO
 *
 * Returns: TODO
 * Since: UNRELEASED
 */
const gchar *
dfl_source_get_name (DflSource *self)
{
  g_return_val_if_fail (DFL_IS_SOURCE (self), NULL);

  return self->name;
}

/**
 * dfl_source_get_new_timestamp:
 * @self: a #DflSource
 *
 * TODO
 *
 * Returns: TODO
 * Since: 0.1.0
 */
DflTimestamp
dfl_source_get_new_timestamp (DflSource *self)
{
  g_return_val_if_fail (DFL_IS_SOURCE (self), 0);

  return self->new_timestamp;
}

/**
 * dfl_source_get_free_timestamp:
 * @self: a #DflSource
 *
 * TODO
 *
 * Returns: TODO
 * Since: 0.1.0
 */
DflTimestamp
dfl_source_get_free_timestamp (DflSource *self)
{
  g_return_val_if_fail (DFL_IS_SOURCE (self), 0);

  return self->free_timestamp;
}

/**
 * dfl_source_get_new_thread_id:
 * @self: a #DflSource
 *
 * TODO
 *
 * Returns: TODO
 * Since: 0.1.0
 */
DflThreadId
dfl_source_get_new_thread_id (DflSource *self)
{
  g_return_val_if_fail (DFL_IS_SOURCE (self), DFL_ID_INVALID);

  return self->new_thread_id;
}

/**
 * dfl_source_get_attach_timestamp:
 * @self: a #DflSource
 *
 * TODO
 *
 * Returns: TODO
 * Since: UNRELEASED
 */
DflTimestamp
dfl_source_get_attach_timestamp (DflSource *self)
{
  g_return_val_if_fail (DFL_IS_SOURCE (self), 0);

  return self->attach_timestamp;
}

/**
 * dfl_source_get_attach_thread_id:
 * @self: a #DflSource
 *
 * TODO
 *
 * Returns: TODO
 * Since: UNRELEASED
 */
DflThreadId
dfl_source_get_attach_thread_id (DflSource *self)
{
  g_return_val_if_fail (DFL_IS_SOURCE (self), DFL_ID_INVALID);

  return self->attach_thread_id;
}

/**
 * dfl_source_get_attach_main_context_id:
 * @self: a #DflSource
 *
 * TODO
 *
 * Returns: TODO
 * Since: UNRELEASED
 */
DflId
dfl_source_get_attach_main_context_id (DflSource *self)
{
  g_return_val_if_fail (DFL_IS_SOURCE (self), DFL_ID_INVALID);

  return self->attach_context;
}

/**
 * dfl_source_get_destroy_timestamp:
 * @self: a #DflSource
 *
 * TODO
 *
 * Returns: TODO
 * Since: UNRELEASED
 */
DflTimestamp
dfl_source_get_destroy_timestamp (DflSource *self)
{
  g_return_val_if_fail (DFL_IS_SOURCE (self), 0);

  return self->destroy_timestamp;
}

/**
 * dfl_source_get_destroy_thread_id:
 * @self: a #DflSource
 *
 * TODO
 *
 * Returns: TODO
 * Since: UNRELEASED
 */
DflThreadId
dfl_source_get_destroy_thread_id (DflSource *self)
{
  g_return_val_if_fail (DFL_IS_SOURCE (self), DFL_ID_INVALID);

  return self->destroy_thread_id;
}

/**
 * dfl_source_dispatch_iter:
 * @self: a #DflSource
 * @iter: an uninitialised #DflTimeSequenceIter to use
 * @start: optional timestamp to start iterating from, or 0
 *
 * TODO
 *
 * Since: 0.1.0
 */
void
dfl_source_dispatch_iter (DflSource           *self,
                          DflTimeSequenceIter *iter,
                          DflTimestamp         start)
{
  g_return_if_fail (DFL_IS_SOURCE (self));
  g_return_if_fail (iter != NULL);

  dfl_time_sequence_iter_init (iter, &self->dispatch_events, start);
}
