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
 * SECTION:event
 * @short_description: main context or source event
 * @stability: Unstable
 * @include: libdunfell/event.h
 *
 * TODO
 *
 * Since: 0.1.0
 */

#include "config.h"

#include <errno.h>
#include <glib.h>
#include <glib-object.h>
#include <string.h>

#include "event.h"


static void dfl_event_get_property (GObject      *object,
                                    guint         property_id,
                                    GValue       *value,
                                    GParamSpec   *pspec);
static void dfl_event_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec);
static void dfl_event_finalize     (GObject      *object);

struct _DflEvent
{
  GObject parent;

  const gchar *event_type;  /* unowned, interned */
  DflTimestamp timestamp;
  DflThreadId thread_id;
  gchar **parameters;  /* owned, null terminated */
};

G_DEFINE_TYPE (DflEvent, dfl_event, G_TYPE_OBJECT)

typedef enum
{
  PROP_EVENT_TYPE = 1,
  PROP_TIMESTAMP,
  PROP_THREAD_ID,
  PROP_PARAMETERS,
} DflEventProperty;

static void
dfl_event_class_init (DflEventClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = dfl_event_get_property;
  object_class->set_property = dfl_event_set_property;
  object_class->finalize = dfl_event_finalize;

  /**
   * DflEvent:event-type:
   *
   * TODO
   *
   * Since: 0.1.0
   */
  g_object_class_install_property (object_class, PROP_EVENT_TYPE,
                                   g_param_spec_string ("event-type",
                                                        "Event Type",
                                                        "Type of the event.",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflEvent:timestamp:
   *
   * TODO
   *
   * Since: 0.1.0
   */
  g_object_class_install_property (object_class, PROP_TIMESTAMP,
                                   g_param_spec_uint64 ("timestamp",
                                                        "Timestamp",
                                                        "Event timestamp.",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflEvent:thread-id:
   *
   * TODO
   *
   * Since: 0.1.0
   */
  g_object_class_install_property (object_class, PROP_THREAD_ID,
                                   g_param_spec_uint64 ("thread-id",
                                                        "Thread ID",
                                                        "Event thread ID.",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflEvent:parameters:
   *
   * TODO
   *
   * Since: 0.1.0
   */
  g_object_class_install_property (object_class, PROP_PARAMETERS,
                                   g_param_spec_boxed ("parameters",
                                                       "Parameters",
                                                       "Event parameters.",
                                                       G_TYPE_STRV,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_STRINGS));
}

static void
dfl_event_init (DflEvent *self)
{
  /* Nothing to see here. */
}

static void
dfl_event_get_property (GObject     *object,
                        guint        property_id,
                        GValue      *value,
                        GParamSpec  *pspec)
{
  DflEvent *self = DFL_EVENT (object);

  switch ((DflEventProperty) property_id)
    {
    case PROP_EVENT_TYPE:
      g_value_set_string (value, self->event_type);
      break;
    case PROP_TIMESTAMP:
      g_value_set_uint64 (value, self->timestamp);
      break;
    case PROP_THREAD_ID:
      g_value_set_uint64 (value, self->thread_id);
      break;
    case PROP_PARAMETERS:
      g_value_set_boxed (value, self->parameters);
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
dfl_event_set_property (GObject           *object,
                        guint              property_id,
                        const GValue      *value,
                        GParamSpec        *pspec)
{
  DflEvent *self = DFL_EVENT (object);

  switch ((DflEventProperty) property_id)
    {
    case PROP_EVENT_TYPE:
      /* Construct only. */
      g_assert (self->event_type == NULL);
      self->event_type = g_intern_string (g_value_get_string (value));
      break;
    case PROP_TIMESTAMP:
      /* Construct only. */
      self->timestamp = g_value_get_uint64 (value);
      break;
    case PROP_THREAD_ID:
      /* Construct only. */
      self->thread_id = g_value_get_uint64 (value);
      break;
    case PROP_PARAMETERS:
      /* Construct only. */
      g_assert (self->parameters == NULL);
      self->parameters = g_value_dup_boxed (value);
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
dfl_event_finalize (GObject *object)
{
  DflEvent *self = DFL_EVENT (object);

  g_strfreev (self->parameters);

  G_OBJECT_CLASS (dfl_event_parent_class)->finalize (object);
}

/**
 * dfl_event_new:
 * @event_type: event type
 * @timestamp: timestamp when the event happened
 * @thread_id: ID of the thread the event happened in
 * @parameters: (array zero-terminated): zero or more parameters specific
 *    to @event_type, %NULL terminated
 *
 * TODO
 *
 * Returns: (transfer full): a new #DflEvent
 * Since: 0.1.0
 */
DflEvent *
dfl_event_new (const gchar         *event_type,
               DflTimestamp         timestamp,
               DflThreadId          thread_id,
               const gchar * const *parameters)
{
  g_return_val_if_fail (event_type != NULL && *event_type != '\0', NULL);

  return g_object_new (DFL_TYPE_EVENT,
                       "event-type", event_type,
                       "timestamp", timestamp,
                       "thread-id", thread_id,
                       "parameters", parameters,
                       NULL);
}

/**
 * dfl_event_get_event_type:
 * @self: a #DflEvent
 *
 * Get the value of the #DflEvent:event-type property. The returned value is
 * guaranteed to be interned, so can be compared directly to other interned
 * strings to avoid string comparisons.
 *
 * Returns: the event type
 * Since: 0.1.0
 */
const gchar *
dfl_event_get_event_type (DflEvent *self)
{
  g_return_val_if_fail (DFL_IS_EVENT (self), NULL);

  return self->event_type;
}

/**
 * dfl_event_get_timestamp:
 * @self: a #DflEvent
 *
 * TODO
 *
 * Returns: the event timestamp
 * Since: 0.1.0
 */
DflTimestamp
dfl_event_get_timestamp (DflEvent *self)
{
  g_return_val_if_fail (DFL_IS_EVENT (self), 0);

  return self->timestamp;
}

/**
 * dfl_event_get_thread_id:
 * @self: a #DflEvent
 *
 * TODO
 *
 * Returns: the ID of the thread the event happened in
 * Since: 0.1.0
 */
DflThreadId
dfl_event_get_thread_id (DflEvent *self)
{
  g_return_val_if_fail (DFL_IS_EVENT (self), 0);

  return self->thread_id;
}

/**
 * dfl_event_get_parameter_id:
 * @self: a #DflEvent
 * @parameter_index: index of the parameter
 *
 * TODO
 *
 * Returns: TODO
 * Since: 0.1.0
 */
DflId
dfl_event_get_parameter_id (DflEvent *self,
                            guint     parameter_index)
{
  gsize i;

  g_return_val_if_fail (DFL_IS_EVENT (self), DFL_ID_INVALID);

  for (i = 0; self->parameters[i] != NULL; i++)
    {
      if (i == parameter_index)
        {
          guint64 retval;
          const gchar *end;

          retval = g_ascii_strtoull (self->parameters[i], (gchar **) &end, 10);

          if (errno == ERANGE || end == self->parameters[i] || *end != '\0')
            g_warning ("Event parameter ‘%s’ cannot be interpreted as an ID.",
                       self->parameters[i]);

          return retval;
        }
    }

  /* Invalid @parameter_index. This check will always be hit if this code is
   * reached, as the check is actually performed more efficiently in the loop
   * above. */
  g_return_val_if_fail (parameter_index < g_strv_length (self->parameters),
                        DFL_ID_INVALID);
  return DFL_ID_INVALID;
}

/**
 * dfl_event_get_parameter_utf8:
 * @self: a #DflEvent
 * @parameter_index: index of the parameter
 *
 * TODO
 *
 * Returns: TODO
 * Since: UNRELEASED
 */
const gchar *
dfl_event_get_parameter_utf8 (DflEvent *self,
                              guint     parameter_index)
{
  gsize i;

  g_return_val_if_fail (DFL_IS_EVENT (self), NULL);

  for (i = 0; self->parameters[i] != NULL; i++)
    {
      if (i == parameter_index)
        {
          if (!g_utf8_validate (self->parameters[i], -1, NULL))
            {
              g_warning ("Event parameter %" G_GSIZE_FORMAT " cannot be "
                         "interpreted as UTF-8.", i);
              return NULL;
            }

          return self->parameters[i];
        }
    }

  /* Invalid @parameter_index. This check will always be hit if this code is
   * reached, as the check is actually performed more efficiently in the loop
   * above. */
  g_return_val_if_fail (parameter_index < g_strv_length (self->parameters),
                        NULL);
  return NULL;
}

/**
 * dfl_event_get_parameter_int64:
 * @self: a #DflEvent
 * @parameter_index: index of the parameter
 *
 * TODO
 *
 * Returns: TODO
 * Since: UNRELEASED
 */
gint64
dfl_event_get_parameter_int64 (DflEvent *self,
                               guint     parameter_index)
{
  gsize i;

  g_return_val_if_fail (DFL_IS_EVENT (self), 0);

  for (i = 0; self->parameters[i] != NULL; i++)
    {
      if (i == parameter_index)
        {
          gint64 retval;
          const gchar *end;

          retval = g_ascii_strtoll (self->parameters[i], (gchar **) &end, 10);

          if (errno == ERANGE || end == self->parameters[i] || *end != '\0')
            g_warning ("Event parameter ‘%s’ cannot be interpreted as an int64.",
                       self->parameters[i]);

          return retval;
        }
    }

  /* Invalid @parameter_index. This check will always be hit if this code is
   * reached, as the check is actually performed more efficiently in the loop
   * above. */
  g_return_val_if_fail (parameter_index < g_strv_length (self->parameters),
                        DFL_ID_INVALID);
  return DFL_ID_INVALID;
}
