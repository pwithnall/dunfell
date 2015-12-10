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
 * SECTION:dfl-event
 * @short_description: main context or source event
 * @stability: Unstable
 * @include: libdunfell/dfl-event.h
 *
 * TODO
 *
 * Since: UNRELEASED
 */

#include "config.h"

#include <glib.h>
#include <glib-object.h>
#include <string.h>

#include "dfl-event.h"


static void dfl_event_get_property (GObject     *object,
                                    guint        property_id,
                                    GValue      *value,
                                    GParamSpec  *pspec);
static void dfl_event_set_property (GObject           *object,
                                    guint              property_id,
                                    const GValue      *value,
                                    GParamSpec        *pspec);

typedef struct
{
  guint64 timestamp;
  guint64 thread_id;
} DflEventPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (DflEvent, dfl_event, G_TYPE_OBJECT)

typedef enum
{
  PROP_TIMESTAMP = 1,
  PROP_THREAD_ID,
} DflEventProperty;

static void
dfl_event_class_init (DflEventClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = dfl_event_get_property;
  object_class->set_property = dfl_event_set_property;

  /**
   * DflEvent:timestamp:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_TIMESTAMP,
                                   g_param_spec_uint64 ("timestamp",
                                                        "Timestamp",
                                                        "Event timestamp.",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  /**
   * DflEvent:thread-id:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_THREAD_ID,
                                   g_param_spec_uint64 ("thread-id",
                                                        "Thread ID",
                                                        "Event thread ID.",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
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
  DflEventPrivate *priv = dfl_event_get_instance_private (DFL_EVENT (object));

  switch ((DflEventProperty) property_id)
    {
    case PROP_TIMESTAMP:
      g_value_set_uint64 (value, priv->timestamp);
      break;
    case PROP_THREAD_ID:
      g_value_set_uint64 (value, priv->thread_id);
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
  DflEventPrivate *priv = dfl_event_get_instance_private (DFL_EVENT (object));

  switch ((DflEventProperty) property_id)
    {
    case PROP_TIMESTAMP:
      /* Construct only. */
      priv->timestamp = g_value_get_uint64 (value);
      break;
    case PROP_THREAD_ID:
      /* Construct only. */
      priv->thread_id = g_value_get_uint64 (value);
      break;
    default:
      g_assert_not_reached ();
    }
}
