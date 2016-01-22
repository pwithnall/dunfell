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
 * SECTION:dfl-main-context
 * @short_description: model of a #GMainContext from a log
 * @stability: Unstable
 * @include: libdunfell/dfl-main-context.h
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
#include "dfl-main-context.h"
#include "dfl-time-sequence.h"


static void dfl_main_context_dispose (GObject *object);

struct _DflMainContext
{
  GObject parent;

  DflId id;

  DflTimestamp new_timestamp;
  DflTimestamp free_timestamp;

  /* Sequence of thread IDs and the duration between the thread acquiring and
   * releasing this main context. A duration of ≥ 0 is valid; < 0 is not. */
  DflTimeSequence/*<DflThreadOwnershipData>*/ thread_ownership_events;

  /* Sequence of thread IDs which tried, and failed, to acquire ownership of
   * this main context. */
  DflTimeSequence/*<DflThreadId>*/ thread_acquisition_failure_events;

  /* Sequence of thread IDs and the duration between the start and end of the
   * dispatch. A duration of ≥ 0 is valid; < 0 is not. */
  DflTimeSequence/*<DflMainContextDispatchData>*/ dispatch_events;

  /* TODO */
  DflTimeSequence source_events;
  DflTimeSequence thread_default_events;
};

G_DEFINE_TYPE (DflMainContext, dfl_main_context, G_TYPE_OBJECT)

static void
dfl_main_context_class_init (DflMainContextClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = dfl_main_context_dispose;
}

static void
dfl_main_context_init (DflMainContext *self)
{
  dfl_time_sequence_init (&self->thread_ownership_events,
                          sizeof (DflThreadOwnershipData), NULL, 0);
  dfl_time_sequence_init (&self->thread_acquisition_failure_events,
                          sizeof (DflThreadId), NULL, 0);
  dfl_time_sequence_init (&self->dispatch_events,
                          sizeof (DflMainContextDispatchData), NULL, 0);

#if 0
TODO
  dfl_time_sequence_init (&self->source_events, 0, 0);
  dfl_time_sequence_init (&self->thread_default_events, 0, 0);
#endif
}

static void
dfl_main_context_dispose (GObject *object)
{
  DflMainContext *self = DFL_MAIN_CONTEXT (object);

  dfl_time_sequence_clear (&self->dispatch_events);
  dfl_time_sequence_clear (&self->thread_default_events);
  dfl_time_sequence_clear (&self->source_events);
  dfl_time_sequence_clear (&self->thread_acquisition_failure_events);
  dfl_time_sequence_clear (&self->thread_ownership_events);

  /* Chain up to the parent class */
  G_OBJECT_CLASS (dfl_main_context_parent_class)->dispose (object);
}

/**
 * dfl_main_context_new:
 * @id: TODO
 * @new_timestamp: TODO
 *
 * TODO
 *
 * Returns: (transfer full): a new #DflMainContext
 * Since: UNRELEASED
 */
DflMainContext *
dfl_main_context_new (DflId        id,
                      DflTimestamp new_timestamp)
{
  DflMainContext *main_context = NULL;

  /* TODO: Properties */
  main_context = g_object_new (DFL_TYPE_MAIN_CONTEXT, NULL);

  main_context->id = id;
  main_context->new_timestamp = new_timestamp;

  return main_context;
}


#include "dfl-event-sequence.h"

static void
main_context_acquire_release_cb (DflEventSequence *sequence,
                                 DflEvent         *event,
                                 gpointer          user_data)
{
  DflMainContext *main_context = user_data;
  gboolean is_acquire;
  DflTimestamp timestamp;
  DflThreadId thread_id;

  /* Does this event correspond to the right main context? */
  if (dfl_event_get_parameter_id (event, 0) != main_context->id)
    return;

  is_acquire = (dfl_event_get_event_type (event) == g_intern_static_string ("g_main_context_acquire"));
  timestamp = dfl_event_get_timestamp (event);
  thread_id = dfl_event_get_thread_id (event);

  if (is_acquire)
    {
      DflThreadOwnershipData *last_element;
      DflTimestamp last_timestamp;
      DflThreadOwnershipData *next_element;

      /* Check that the previous element in the sequence has a valid (non-zero)
       * duration, otherwise no release event was logged and something very odd
       * is happening.
       *
       * @last_element will be %NULL if this is the first acquire. */
      last_element = dfl_time_sequence_get_last_element (&main_context->thread_ownership_events,
                                                         &last_timestamp);

      if (last_element != NULL && last_element->duration < 0)
        {
          /* TODO: Some better error reporting framework than g_warning(). */
          g_warning ("Saw two g_main_context_acquire() calls in a row for the "
                     "same context with no g_main_context_release() in "
                     "between.");

          /* Fudge it. */
          last_element->duration = timestamp - last_timestamp;
        }

      /* Start the next element. */
      next_element = dfl_time_sequence_append (&main_context->thread_ownership_events,
                                               timestamp);
      next_element->thread_id = thread_id;
      next_element->duration = -1;  /* will be set by the paired release() */
    }
  else
    {
      DflThreadOwnershipData *last_element;
      DflTimestamp last_timestamp;

      /* Check that the previous element in the sequence has an invalid (zero)
       * duration, otherwise no acquire event was logged.
       *
       * @last_element should only be %NULL if something odd is happening. */
      last_element = dfl_time_sequence_get_last_element (&main_context->thread_ownership_events,
                                                         &last_timestamp);

      if (last_element == NULL)
        {
          /* TODO: Some better error reporting framework than g_warning(). */
          g_warning ("Saw a g_main_context_release() call for a context with "
                     "no g_main_context_acquire() beforehand.");

          /* Fudge it. */
          last_element = dfl_time_sequence_append (&main_context->thread_ownership_events,
                                                   timestamp);
          last_timestamp = timestamp;
          last_element->thread_id = thread_id;
          last_element->duration = -1;
        }
      else if (last_element->duration >= 0)
        {
          /* TODO: Some better error reporting framework than g_warning(). */
          g_warning ("Saw two g_main_context_release() calls in a row for the "
                     "same context with no g_main_context_acquire() in "
                     "between.");
        }
      else if (last_element->thread_id != thread_id)
        {
          /* TODO: Some better error reporting framework than g_warning(). */
          g_warning ("Saw a g_main_context_release() call for one thread "
                     "immediately following a g_main_context_acquire() call "
                     "for a different one, both on the same context.");

          /* Fudge it. */
          last_element->duration = timestamp - last_timestamp;

          last_element = dfl_time_sequence_append (&main_context->thread_ownership_events,
                                                   timestamp);
          last_timestamp = timestamp;
          last_element->thread_id = thread_id;
          last_element->duration = -1;
        }

      /* Update the element’s duration. */
      last_element->duration = timestamp - last_timestamp;
    }
}

static void
main_context_free_cb (DflEventSequence *sequence,
                      DflEvent         *event,
                      gpointer          user_data)
{
  DflMainContext *main_context = user_data;
  DflTimestamp timestamp;

  /* Does this event correspond to the right main context? */
  if (dfl_event_get_parameter_id (event, 0) != main_context->id)
    return;

  timestamp = dfl_event_get_timestamp (event);

  if (main_context->free_timestamp != 0)
    {
      /* TODO: Some better error reporting framework than g_warning(). */
      g_warning ("Saw two g_main_context_free() calls for the same main "
                 "context.");
    }

  main_context->free_timestamp = timestamp;
}

static void
main_context_before_after_dispatch_cb (DflEventSequence *sequence,
                                       DflEvent         *event,
                                       gpointer          user_data)
{
  DflMainContext *main_context = user_data;
  gboolean is_before;
  DflTimestamp timestamp;
  DflThreadId thread_id;

  /* Does this event correspond to the right main context? */
  if (dfl_event_get_parameter_id (event, 0) != main_context->id)
    return;

  is_before = (dfl_event_get_event_type (event) ==
               g_intern_static_string ("g_main_context_before_dispatch"));
  timestamp = dfl_event_get_timestamp (event);
  thread_id = dfl_event_get_thread_id (event);

  if (is_before)
    {
      DflMainContextDispatchData *last_element;
      DflTimestamp last_timestamp;
      DflMainContextDispatchData *next_element;

      /* Check that the previous element in the sequence has a valid (non-zero)
       * duration, otherwise no //after// event was logged and something very
       * odd is happening.
       *
       * @last_element will be %NULL if this is the first //before//. */
      last_element = dfl_time_sequence_get_last_element (&main_context->dispatch_events,
                                                         &last_timestamp);

      if (last_element != NULL && last_element->duration < 0)
        {
          /* TODO: Some better error reporting framework than g_warning(). */
          g_warning ("Saw two g_main_context_dispatch() calls in a row for the "
                     "same context with no finish in between.");

          /* Fudge it. */
          last_element->duration = timestamp - last_timestamp;
        }

      /* Start the next element. */
      next_element = dfl_time_sequence_append (&main_context->dispatch_events,
                                               timestamp);
      next_element->thread_id = thread_id;
      next_element->duration = -1;  /* will be set by the paired //after// */
    }
  else
    {
      DflMainContextDispatchData *last_element;
      DflTimestamp last_timestamp;

      /* Check that the previous element in the sequence has an invalid (zero)
       * duration, otherwise no //before// event was logged.
       *
       * @last_element should only be %NULL if something odd is happening. */
      last_element = dfl_time_sequence_get_last_element (&main_context->dispatch_events,
                                                         &last_timestamp);

      if (last_element == NULL)
        {
          /* TODO: Some better error reporting framework than g_warning(). */
          g_warning ("Saw a g_main_context_dispatch() call finish for a "
                     "context with no g_main_context_dispatch() call starting "
                     "beforehand.");

          /* Fudge it. */
          last_element = dfl_time_sequence_append (&main_context->dispatch_events,
                                                   timestamp);
          last_timestamp = timestamp;
          last_element->thread_id = thread_id;
          last_element->duration = -1;
        }
      else if (last_element->duration >= 0)
        {
          /* TODO: Some better error reporting framework than g_warning(). */
          g_warning ("Saw two g_main_context_dispatch() calls finish in a row "
                     "for the same context with no g_main_context_dispatch() "
                     "start in between.");
        }
      else if (last_element->thread_id != thread_id)
        {
          /* TODO: Some better error reporting framework than g_warning(). */
          g_warning ("Saw a g_main_context_dispatch() call finish for one "
                     "thread immediately following a "
                     "g_main_context_dispatch() call for a different one, "
                     "both on the same context.");

          /* Fudge it. */
          last_element->duration = timestamp - last_timestamp;

          last_element = dfl_time_sequence_append (&main_context->dispatch_events,
                                                   timestamp);
          last_timestamp = timestamp;
          last_element->thread_id = thread_id;
          last_element->duration = -1;
        }

      /* Update the element’s duration. */
      last_element->duration = timestamp - last_timestamp;
    }
}

static void
main_context_new_cb (DflEventSequence *sequence,
                     DflEvent         *event,
                     gpointer          user_data)
{
  GPtrArray/*<owned DflMainContext>*/ *main_contexts = user_data;
  DflMainContext *main_context = NULL;

  main_context = dfl_main_context_new (dfl_event_get_parameter_id (event, 0),
                                       dfl_event_get_timestamp (event));

  dfl_event_sequence_add_walker (sequence, "g_main_context_acquire",
                                 main_context_acquire_release_cb,
                                 g_object_ref (main_context),
                                 (GDestroyNotify) g_object_unref);
  dfl_event_sequence_add_walker (sequence, "g_main_context_release",
                                 main_context_acquire_release_cb,
                                 g_object_ref (main_context),
                                 (GDestroyNotify) g_object_unref);
  dfl_event_sequence_add_walker (sequence, "g_main_context_free",
                                 main_context_free_cb,
                                 g_object_ref (main_context),
                                 (GDestroyNotify) g_object_unref);

  dfl_event_sequence_add_walker (sequence, "g_main_context_before_dispatch",
                                 main_context_before_after_dispatch_cb,
                                 g_object_ref (main_context),
                                 (GDestroyNotify) g_object_unref);
  dfl_event_sequence_add_walker (sequence, "g_main_context_after_dispatch",
                                 main_context_before_after_dispatch_cb,
                                 g_object_ref (main_context),
                                 (GDestroyNotify) g_object_unref);

  /* TODO: When do these get removed? */

  /* TODO: Other walkers for different members. */

  g_ptr_array_add (main_contexts, main_context);  /* transfer */
}

/**
 * dfl_main_context_factory_from_event_sequence:
 * @sequence: an event sequence to analyse
 *
 * TODO
 *
 * Returns: (transfer full) (element-type DflMainContext): an array of zero or
 *    more #DflMainContexts
 * Since: UNRELEASED
 */
GPtrArray *
dfl_main_context_factory_from_event_sequence (DflEventSequence *sequence)
{
  GPtrArray/*<owned DflMainContext>*/ *main_contexts = NULL;

  main_contexts = g_ptr_array_new_with_free_func (g_object_unref);
  dfl_event_sequence_add_walker (sequence, "g_main_context_new",
                                 main_context_new_cb,
                                 g_ptr_array_ref (main_contexts),
                                 (GDestroyNotify) g_ptr_array_unref);

  return main_contexts;
}

/**
 * dfl_main_context_get_id:
 * @self: a #DflMainContext
 *
 * TODO
 *
 * Returns: TODO
 * Since: UNRELEASED
 */
DflId
dfl_main_context_get_id (DflMainContext *self)
{
  g_return_val_if_fail (DFL_IS_MAIN_CONTEXT (self), DFL_ID_INVALID);

  return self->id;
}

/**
 * dfl_main_context_get_new_timestamp:
 * @self: a #DflMainContext
 *
 * TODO
 *
 * Returns: TODO
 * Since: UNRELEASED
 */
DflTimestamp
dfl_main_context_get_new_timestamp (DflMainContext *self)
{
  g_return_val_if_fail (DFL_IS_MAIN_CONTEXT (self), 0);

  return self->new_timestamp;
}

/**
 * dfl_main_context_get_free_timestamp:
 * @self: a #DflMainContext
 *
 * TODO
 *
 * Returns: TODO
 * Since: UNRELEASED
 */
DflTimestamp
dfl_main_context_get_free_timestamp (DflMainContext *self)
{
  g_return_val_if_fail (DFL_IS_MAIN_CONTEXT (self), 0);

  return self->free_timestamp;
}

/**
 * dfl_main_context_thread_ownership_iter:
 * @self: a #DflMainContext
 * @iter: an uninitialised #DflTimeSequenceIter to use
 * @start: optional timestamp to start iterating from, or 0
 *
 * TODO
 *
 * Since: UNRELEASED
 */
void
dfl_main_context_thread_ownership_iter (DflMainContext      *self,
                                        DflTimeSequenceIter *iter,
                                        DflTimestamp         start)
{
  g_return_if_fail (DFL_IS_MAIN_CONTEXT (self));
  g_return_if_fail (iter != NULL);

  dfl_time_sequence_iter_init (iter, &self->thread_ownership_events, start);
}

/**
 * dfl_main_context_dispatch_iter:
 * @self: a #DflMainContext
 * @iter: an uninitialised #DflTimeSequenceIter to use
 * @start: optional timestamp to start iterating from, or 0
 *
 * TODO
 *
 * Since: UNRELEASED
 */
void
dfl_main_context_dispatch_iter (DflMainContext      *self,
                                DflTimeSequenceIter *iter,
                                DflTimestamp         start)
{
  g_return_if_fail (DFL_IS_MAIN_CONTEXT (self));
  g_return_if_fail (iter != NULL);

  dfl_time_sequence_iter_init (iter, &self->dispatch_events, start);
}
