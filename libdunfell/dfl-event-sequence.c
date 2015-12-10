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
 * SECTION:dfl-event-sequence
 * @short_description: sequence of #DflEvents
 * @stability: Unstable
 * @include: libdunfell/dfl-event-sequence.h
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
#include "dfl-event-sequence.h"


static void dfl_event_sequence_list_model_init (GListModelInterface *iface);
static void dfl_event_sequence_dispose (GObject *object);
static GType dfl_event_sequence_get_item_type (GListModel *list);
static guint dfl_event_sequence_get_n_items (GListModel *list);
static gpointer dfl_event_sequence_get_item (GListModel  *list,
                                             guint        position);

struct _DflEventSequence
{
  GObject parent;

  DflEvent **events;  /* owned */
  guint n_events;
  guint64 initial_timestamp;
};

G_DEFINE_TYPE_WITH_CODE (DflEventSequence, dfl_event_sequence, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL,
                                                dfl_event_sequence_list_model_init))

static void
dfl_event_sequence_class_init (DflEventSequenceClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = dfl_event_sequence_dispose;
}

static void
dfl_event_sequence_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = dfl_event_sequence_get_item_type;
  iface->get_n_items = dfl_event_sequence_get_n_items;
  iface->get_item = dfl_event_sequence_get_item;
}

static void
dfl_event_sequence_init (DflEventSequence *self)
{
  /* Nothing to see here. */
}

static void
dfl_event_sequence_dispose (GObject *object)
{
  DflEventSequence *self = DFL_EVENT_SEQUENCE (object);

  g_clear_pointer (&self->events, g_ptr_array_unref);

  /* Chain up to the parent class */
  G_OBJECT_CLASS (dfl_event_sequence_parent_class)->dispose (object);
}

static GType
dfl_event_sequence_get_item_type (GListModel *list)
{
  return DFL_TYPE_EVENT;
}

static guint
dfl_event_sequence_get_n_items (GListModel *list)
{
  DflEventSequence *self = DFL_EVENT_SEQUENCE (list);

  return self->n_events;
}

static gpointer
dfl_event_sequence_get_item (GListModel  *list,
                             guint        position)
{
  DflEventSequence *self = DFL_EVENT_SEQUENCE (list);

  if (position >= self->n_events)
    return NULL;

  return self->events[position];
}

/**
 * dfl_event_sequence_new:
 * @events: (array length=n_events): array of #DflEvents to put in the sequence
 * @n_events: number of items in @events
 * @initial_timestamp: the timestamp of the start of the sequence (before the
 *    first event)
 *
 * TODO
 *
 * Returns: (transfer full): a new #DflEventSequence
 * Since: UNRELEASED
 */
DflEventSequence *
dfl_event_sequence_new (const DflEvent **events,
                        guint            n_events,
                        guint64          initial_timestamp)
{
  DflEventSequence *obj = NULL;
  guint i;

  g_return_val_if_fail (n_events == 0 || events != NULL, NULL);
  g_return_val_if_fail (n_events < G_MAXUINT / sizeof (DflEvent *), NULL);

  obj = g_object_new (DFL_TYPE_EVENT_SEQUENCE, NULL);
  obj->events = g_memdup (events, sizeof (DflEvent *) * n_events);
  obj->n_events = n_events;
  obj->initial_timestamp = initial_timestamp;

  /* Reference all the events. */
  for (i = 0; i < n_events; i++)
    {
      g_return_val_if_fail (DFL_IS_EVENT (obj->events[i]), NULL);
      g_object_ref (obj->events[i]);
    }

  return obj;
}
