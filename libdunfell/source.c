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


static void dfl_source_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec);
static void dfl_source_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec);
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
  DflTimeSequence/*<DflSourceDispatchData>*/ dispatch_events;

  gchar *name;  /* owned; nullable */

  DflId attach_context;
  DflTimestamp attach_timestamp;
  DflThreadId attach_thread_id;
  DflTimestamp destroy_timestamp;
  DflThreadId destroy_thread_id;

  gboolean priority_set;
  gint min_priority;
  gint max_priority;

  DflSource *parent_source;  /* (ownership none) */
  GPtrArray *children;  /* (element-type DflSource) (ownership container) */
};

G_DEFINE_TYPE (DflSource, dfl_source, G_TYPE_OBJECT)

typedef enum
{
  PROP_ID = 1,
  PROP_NAME,
  PROP_NEW_TIMESTAMP,
  PROP_NEW_THREAD_ID,
  PROP_FREE_TIMESTAMP,
  PROP_ATTACH_TIMESTAMP,
  PROP_ATTACH_THREAD_ID,
  PROP_ATTACH_CONTEXT,
  PROP_DESTROY_TIMESTAMP,
  PROP_DESTROY_THREAD_ID,
  PROP_MIN_PRIORITY,
  PROP_MAX_PRIORITY,
  PROP_N_DISPATCHES,
  PROP_MIN_DISPATCH_DURATION,
  PROP_MEDIAN_DISPATCH_DURATION,
  PROP_MAX_DISPATCH_DURATION,
} DflSourceProperty;

static void
dfl_source_class_init (DflSourceClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->get_property = dfl_source_get_property;
  object_class->set_property = dfl_source_set_property;
  object_class->dispose = dfl_source_dispose;

  /**
   * DflSource:id:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_ID,
                                   g_param_spec_ulong ("id",
                                                       "ID",
                                                       "TODO.",
                                                       0, G_MAXULONG,
                                                       DFL_ID_INVALID,
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_STATIC_STRINGS));

  /**
   * DflSource:name:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "Name",
                                                        "TODO.",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflSource:new-timestamp:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_NEW_TIMESTAMP,
                                   g_param_spec_uint64 ("new-timestamp",
                                                        "New Timestamp",
                                                        "TODO.",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflSource:new-thread-id:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_NEW_THREAD_ID,
                                   g_param_spec_uint64 ("new-thread-id",
                                                        "New Thread ID",
                                                        "TODO.",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflSource:free-timestamp:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_FREE_TIMESTAMP,
                                   g_param_spec_uint64 ("free-timestamp",
                                                        "Free Timestamp",
                                                        "TODO.",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflSource:attach-timestamp:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_ATTACH_TIMESTAMP,
                                   g_param_spec_uint64 ("attach-timestamp",
                                                        "Attach Timestamp",
                                                        "TODO.",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflSource:attach-thread-id:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_ATTACH_THREAD_ID,
                                   g_param_spec_uint64 ("attach-thread-id",
                                                        "Attach Thread ID",
                                                        "TODO.",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflSource:attach-context:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_ATTACH_CONTEXT,
                                   g_param_spec_ulong ("attach-context",
                                                       "Attach Context",
                                                       "TODO.",
                                                       0, G_MAXULONG, 0,
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_STATIC_STRINGS));

  /**
   * DflSource:destroy-timestamp:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_DESTROY_TIMESTAMP,
                                   g_param_spec_uint64 ("destroy-timestamp",
                                                        "Destroy Timestamp",
                                                        "TODO.",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflSource:destroy-thread-id:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_DESTROY_THREAD_ID,
                                   g_param_spec_uint64 ("destroy-thread-id",
                                                        "Destroy Thread ID",
                                                        "TODO.",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflSource:min-priority:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_MIN_PRIORITY,
                                   g_param_spec_int ("min-priority",
                                                     "Minimum Priority",
                                                     "TODO.",
                                                     G_MININT, G_MAXINT,
                                                     G_PRIORITY_DEFAULT,
                                                     G_PARAM_CONSTRUCT_ONLY |
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_STATIC_STRINGS));

  /**
   * DflSource:max-priority:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_MAX_PRIORITY,
                                   g_param_spec_int ("max-priority",
                                                     "Maximum Priority",
                                                     "TODO.",
                                                     G_MININT, G_MAXINT,
                                                     G_PRIORITY_DEFAULT,
                                                     G_PARAM_CONSTRUCT_ONLY |
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_STATIC_STRINGS));

  /**
   * DflSource:n-dispatches:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_N_DISPATCHES,
                                   g_param_spec_ulong ("n-dispatches",
                                                       "Number of Dispatches",
                                                       "TODO.",
                                                       0, G_MAXULONG, 0,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_STATIC_STRINGS));

  /**
   * DflSource:min-dispatch-duration:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_MIN_DISPATCH_DURATION,
                                   g_param_spec_int64 ("min-dispatch-duration",
                                                       "Minimum Dispatch Duration",
                                                       "TODO.",
                                                       0, G_MAXINT64, 0,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_STATIC_STRINGS));

  /**
   * DflSource:median-dispatch-duration:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_MEDIAN_DISPATCH_DURATION,
                                   g_param_spec_int64 ("median-dispatch-duration",
                                                       "Median Dispatch Duration",
                                                       "TODO.",
                                                       0, G_MAXINT64, 0,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_STATIC_STRINGS));

  /**
   * DflSource:max-dispatch-duration:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_MAX_DISPATCH_DURATION,
                                   g_param_spec_int64 ("max-dispatch-duration",
                                                       "Maximum Dispatch Duration",
                                                       "TODO.",
                                                       0, G_MAXINT64, 0,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_STATIC_STRINGS));
}

static void
dfl_source_init (DflSource *self)
{
  dfl_time_sequence_init (&self->dispatch_events,
                          sizeof (DflSourceDispatchData),
                          (GDestroyNotify) dfl_source_dispatch_data_clear, 0);

  self->children = g_ptr_array_new_with_free_func (g_object_unref);
}

static void
dfl_source_get_property (GObject     *object,
                         guint        property_id,
                         GValue      *value,
                         GParamSpec  *pspec)
{
  DflSource *self = DFL_SOURCE (object);

  switch ((DflSourceProperty) property_id)
    {
    case PROP_ID:
      g_value_set_ulong (value, self->id);
      break;
    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;
    case PROP_NEW_TIMESTAMP:
      g_value_set_uint64 (value, self->new_timestamp);
      break;
    case PROP_NEW_THREAD_ID:
      g_value_set_uint64 (value, self->new_thread_id);
      break;
    case PROP_FREE_TIMESTAMP:
      g_value_set_uint64 (value, self->free_timestamp);
      break;
    case PROP_ATTACH_TIMESTAMP:
      g_value_set_uint64 (value, self->attach_timestamp);
      break;
    case PROP_ATTACH_THREAD_ID:
      g_value_set_uint64 (value, self->attach_thread_id);
      break;
    case PROP_ATTACH_CONTEXT:
      g_value_set_ulong (value, self->attach_context);
      break;
    case PROP_DESTROY_TIMESTAMP:
      g_value_set_uint64 (value, self->destroy_timestamp);
      break;
    case PROP_DESTROY_THREAD_ID:
      g_value_set_uint64 (value, self->destroy_thread_id);
      break;
    case PROP_MIN_PRIORITY:
      g_value_set_int (value, self->min_priority);
      break;
    case PROP_MAX_PRIORITY:
      g_value_set_int (value, self->max_priority);
      break;
    case PROP_N_DISPATCHES:
      {
        gsize n_dispatches;

        dfl_source_get_dispatch_statistics (self, &n_dispatches,
                                            NULL, NULL, NULL);
        g_value_set_ulong (value, n_dispatches);
        break;
      }
    case PROP_MIN_DISPATCH_DURATION:
      {
        DflDuration min_duration;

        dfl_source_get_dispatch_statistics (self, NULL,
                                            &min_duration, NULL, NULL);
        g_value_set_int64 (value, min_duration);
        break;
      }
    case PROP_MEDIAN_DISPATCH_DURATION:
      {
        DflDuration median_duration;

        dfl_source_get_dispatch_statistics (self, NULL, NULL,
                                            &median_duration, NULL);
        g_value_set_int64 (value, median_duration);
        break;
      }
    case PROP_MAX_DISPATCH_DURATION:
      {
        DflDuration max_duration;

        dfl_source_get_dispatch_statistics (self, NULL, NULL, NULL,
                                            &max_duration);
        g_value_set_int64 (value, max_duration);
        break;
      }
    default:
      g_assert_not_reached ();
    }
}

static void
dfl_source_set_property (GObject           *object,
                         guint              property_id,
                         const GValue      *value,
                         GParamSpec        *pspec)
{
  DflSource *self = DFL_SOURCE (object);

  /* All construct only. */
  switch ((DflSourceProperty) property_id)
    {
    case PROP_ID:
      g_assert (self->id == DFL_ID_INVALID);
      self->id = g_value_get_ulong (value);
      break;
    case PROP_NAME:
      g_assert (self->name == NULL);
      self->name = g_value_dup_string (value);
      break;
    case PROP_NEW_TIMESTAMP:
      g_assert (self->new_timestamp == 0);
      self->new_timestamp = g_value_get_uint64 (value);
      break;
    case PROP_NEW_THREAD_ID:
      g_assert (self->new_thread_id == 0);
      self->new_thread_id = g_value_get_uint64 (value);
      break;
    case PROP_FREE_TIMESTAMP:
      g_assert (self->free_timestamp == 0);
      self->free_timestamp = g_value_get_uint64 (value);
      break;
    case PROP_ATTACH_TIMESTAMP:
      g_assert (self->attach_timestamp == 0);
      self->attach_timestamp = g_value_get_uint64 (value);
      break;
    case PROP_ATTACH_THREAD_ID:
      g_assert (self->attach_thread_id == 0);
      self->attach_thread_id = g_value_get_uint64 (value);
      break;
    case PROP_ATTACH_CONTEXT:
      g_assert (self->attach_context == 0);
      self->attach_context = g_value_get_ulong (value);
      break;
    case PROP_DESTROY_TIMESTAMP:
      g_assert (self->destroy_timestamp == 0);
      self->destroy_timestamp = g_value_get_uint64 (value);
      break;
    case PROP_DESTROY_THREAD_ID:
      g_assert (self->destroy_thread_id == 0);
      self->destroy_thread_id = g_value_get_uint64 (value);
      break;
    case PROP_MIN_PRIORITY:
      g_assert (self->min_priority == G_PRIORITY_DEFAULT);
      self->min_priority = g_value_get_int (value);
      break;
    case PROP_MAX_PRIORITY:
      g_assert (self->max_priority == G_PRIORITY_DEFAULT);
      self->max_priority = g_value_get_int (value);
      break;
    case PROP_N_DISPATCHES:
    case PROP_MIN_DISPATCH_DURATION:
    case PROP_MEDIAN_DISPATCH_DURATION:
    case PROP_MAX_DISPATCH_DURATION:
      /* Read only. */
    default:
      g_assert_not_reached ();
    }
}

static void
dfl_source_dispose (GObject *object)
{
  DflSource *self = DFL_SOURCE (object);

  dfl_time_sequence_clear (&self->dispatch_events);
  g_clear_pointer (&self->name, g_free);

  g_clear_pointer (&self->children, g_ptr_array_unref);

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
  return g_object_new (DFL_TYPE_SOURCE,
                       "id", id,
                       "new-timestamp", new_timestamp,
                       "new-thread-id", new_thread_id,
                       "free-timestamp", new_timestamp,
                       NULL);
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
source_set_priority_cb (DflEventSequence *sequence,
                        DflEvent         *event,
                        gpointer          user_data)
{
  DflSource *source = user_data;
  gint64 priority;

  /* Does this event correspond to the right source? */
  g_assert (dfl_event_get_parameter_id (event, 0) == source->id);

  priority = dfl_event_get_parameter_int64 (event, 2);

  if (!source->priority_set)
    {
      source->min_priority = priority;
      source->max_priority = priority;
    }
  else
    {
      source->min_priority = MIN (source->min_priority, priority);
      source->max_priority = MAX (source->max_priority, priority);
    }

  source->priority_set = TRUE;
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
  dfl_event_sequence_add_walker (sequence, "g_source_set_priority", source_id,
                                 source_set_priority_cb,
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

static void
source_add_child_source_cb (DflEventSequence *sequence,
                            DflEvent         *event,
                            gpointer          user_data)
{
  GPtrArray/*<owned DflSource>*/ *sources = user_data;
  DflSource *parent_source = NULL, *child_source = NULL;
  DflId parent_source_id, child_source_id;
  gsize i;

  parent_source_id = dfl_event_get_parameter_id (event, 0);
  child_source_id = dfl_event_get_parameter_id (event, 1);

  /* Find the two sources. */
  for (i = 0;
       i < sources->len && (parent_source == NULL || child_source == NULL);
       i++)
    {
      if (parent_source == NULL &&
          dfl_source_get_id (sources->pdata[i]) == parent_source_id)
        parent_source = DFL_SOURCE (sources->pdata[i]);

      if (child_source == NULL &&
          dfl_source_get_id (sources->pdata[i]) == child_source_id)
        child_source = DFL_SOURCE (sources->pdata[i]);
    }

  if (parent_source == NULL || child_source == NULL)
    {
      g_warning ("g_source_add_child_source() seen for a parent or child "
                 "source which don’t exist.");
      return;
    }

  if (child_source->parent_source != NULL)
    {
      g_warning ("g_source_add_child_source() seen twice for the same source.");
      return;
    }

  g_ptr_array_add (parent_source->children, g_object_ref (child_source));
  child_source->parent_source = parent_source;
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
  dfl_event_sequence_add_walker (sequence, "g_source_add_child_source",
                                 DFL_ID_INVALID, source_add_child_source_cb,
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

/**
 * dfl_source_get_n_long_dispatches:
 * @self: a #DflSource
 * @min_duration: minimum dispatch duration to count (inclusive), in
 *    microseconds
 *
 * TODO
 *
 * Returns: number of dispatches whose duration is equal to or greater than
 *    @min_duration
 * Since: UNRELEASED
 */
gsize
dfl_source_get_n_long_dispatches (DflSource   *self,
                                  DflDuration  min_duration)
{
  DflTimeSequenceIter iter;
  DflSourceDispatchData *dispatch_data;
  gsize count;

  g_return_val_if_fail (DFL_IS_SOURCE (self), 0);

  /* Fast path. */
  if (min_duration == 0)
    return dfl_time_sequence_get_n_elements (&self->dispatch_events);

  /* Iteration path. */
  dfl_time_sequence_iter_init (&iter, &self->dispatch_events, 0);
  count = 0;

  while (dfl_time_sequence_iter_next (&iter, NULL, (gpointer *) &dispatch_data))
    {
      if (dispatch_data->duration >= min_duration)
        count++;
    }

  return count;
}

static gint
compare_durations (gconstpointer a,
                   gconstpointer b)
{
  const DflDuration *duration_a = a, *duration_b = b;

  return (*duration_a - *duration_b);
}

/* TODO: Docs */
void
dfl_source_get_dispatch_statistics (DflSource   *self,
                                    gsize       *n_dispatches,
                                    DflDuration *min_duration,
                                    DflDuration *median_duration,
                                    DflDuration *max_duration)
{
  g_autoptr (GArray) durations = NULL;  /* (element-type DflDuration) */
  DflTimeSequenceIter iter;
  DflSourceDispatchData *dispatch_data;

  g_return_if_fail (DFL_IS_SOURCE (self));

  if (n_dispatches != NULL)
    *n_dispatches = dfl_time_sequence_get_n_elements (&self->dispatch_events);

  /* Fast paths. */
  if (min_duration == NULL && median_duration == NULL && max_duration == NULL)
    return;

  if (dfl_time_sequence_get_n_elements (&self->dispatch_events) == 0)
    {
      if (min_duration != NULL)
        *min_duration = 0;
      if (median_duration != NULL)
        *median_duration = 0;
      if (max_duration != NULL)
        *max_duration = 0;

      return;
    }

  /* In order to calculate the median, we need to order all the dispatches by
   * duration. */
  durations = g_array_sized_new (FALSE, FALSE, sizeof (DflDuration),
                                 dfl_time_sequence_get_n_elements (&self->dispatch_events));

  dfl_time_sequence_iter_init (&iter, &self->dispatch_events, 0);

  while (dfl_time_sequence_iter_next (&iter, NULL, (gpointer *) &dispatch_data))
    g_array_append_val (durations, dispatch_data->duration);

  g_array_sort (durations, compare_durations);

  /* Calculate the aggregates. */
  if (min_duration != NULL)
    *min_duration = g_array_index (durations, DflDuration, 0);

  if (max_duration != NULL)
    *max_duration = g_array_index (durations, DflDuration, durations->len - 1);

  if (median_duration != NULL)
    {
      if ((durations->len % 2) == 0)
        *median_duration = (g_array_index (durations, DflDuration, durations->len / 2) +
                            g_array_index (durations, DflDuration, durations->len / 2 + 1)) / 2;
      else
        *median_duration = g_array_index (durations, DflDuration, durations->len / 2);
    }
}

/**
 * dfl_source_get_priority_statistics:
 * @self: a #DflSource
 * @min_priority: (out caller-allocates) (optional): TODO
 * @max_priority: (out caller-allocates) (optional): TODO
 *
 * TODO
 *
 * Since: UNRELEASED
 */
void
dfl_source_get_priority_statistics (DflSource *self,
                                    gint      *min_priority,
                                    gint      *max_priority)
{
  g_return_if_fail (DFL_IS_SOURCE (self));

  if (min_priority != NULL)
    *min_priority = self->min_priority;
  if (max_priority != NULL)
    *max_priority = self->max_priority;
}

/**
 * dfl_source_get_children:
 * @self: a #DflSource
 *
 * TODO
 *
 * Returns: (element-type DflSource) (transfer none): potentially empty array
 *    containing the children of this #DflSource
 * Since: UNRELEASED
 */
GPtrArray *
dfl_source_get_children (DflSource *self)
{
  g_return_val_if_fail (DFL_IS_SOURCE (self), NULL);

  return self->children;
}

/**
 * dfl_source_get_parent:
 * @self: a #DflSource
 *
 * TODO
 *
 * Returns: (transfer none) (nullable): parent #DflSource, or %NULL if this
 *    #DflSource has no parent
 * Since: UNRELEASED
 */
DflSource *
dfl_source_get_parent (DflSource *self)
{
  g_return_val_if_fail (DFL_IS_SOURCE (self), NULL);

  return self->parent_source;
}
