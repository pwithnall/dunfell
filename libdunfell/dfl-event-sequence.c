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
 * SECTION:dfl-event-sequence
 * @short_description: sequence of #DflEvents
 * @stability: Unstable
 * @include: libdunfell/dfl-event-sequence.h
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

#include "dfl-event.h"
#include "dfl-event-sequence.h"


static void dfl_event_sequence_list_model_init (GListModelInterface *iface);
static void dfl_event_sequence_dispose (GObject *object);
static GType dfl_event_sequence_get_item_type (GListModel *list);
static guint dfl_event_sequence_get_n_items (GListModel *list);
static gpointer dfl_event_sequence_get_item (GListModel  *list,
                                             guint        position);

typedef struct
{
  const gchar *event_type;  /* nullable, unowned, interned */
  DflEventWalker walker;
  gpointer user_data;  /* nullable */
  GDestroyNotify destroy_user_data;  /* nullable */
} DflEventSequenceWalkerClosure;

struct _DflEventSequence
{
  GObject parent;

  DflEvent **events;  /* owned */
  guint n_events;
  guint64 initial_timestamp;

  GArray/*<DflEventWalkerClosure>*/ *walkers;  /* owned */
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
walkers_clear_cb (gpointer user_data)
{
  DflEventSequenceWalkerClosure *closure = user_data;

  if (closure->user_data != NULL && closure->destroy_user_data != NULL)
    closure->destroy_user_data (closure->user_data);

  closure->event_type = NULL;
  closure->walker = NULL;
  closure->user_data = NULL;
  closure->destroy_user_data = NULL;
}

static void
dfl_event_sequence_init (DflEventSequence *self)
{
  self->walkers = g_array_new (FALSE, FALSE,
                               sizeof (DflEventSequenceWalkerClosure));
  g_array_set_clear_func (self->walkers, walkers_clear_cb);
}

static void
dfl_event_sequence_dispose (GObject *object)
{
  DflEventSequence *self = DFL_EVENT_SEQUENCE (object);
  guint i;

  for (i = 0; i < self->n_events; i++)
    g_object_unref (self->events[i]);

  g_clear_pointer (&self->events, g_free);
  self->n_events = 0;

  g_clear_pointer (&self->walkers, g_array_unref);

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
 * Since: 0.1.0
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

/**
 * dfl_event_sequence_add_walker:
 * @self: a #DflEventSequence
 * @event_type: (nullable): event type to match, or %NULL to match all event
 *    types
 * @walker: the event walker to add
 * @user_data: user data to pass to @walker
 * @destroy_user_data: (nullable): destroy handler for @user_data
 *
 * TODO
 *
 * Returns: the ID of the walker
 * Since: 0.1.0
 */
guint
dfl_event_sequence_add_walker (DflEventSequence *self,
                               const gchar      *event_type,
                               DflEventWalker    walker,
                               gpointer          user_data,
                               GDestroyNotify    destroy_user_data)
{
  DflEventSequenceWalkerClosure closure;

  g_return_val_if_fail (DFL_IS_EVENT_SEQUENCE (self), 0);
  g_return_val_if_fail (event_type == NULL || *event_type != '\0', 0);
  g_return_val_if_fail (walker != NULL, 0);

  closure.event_type = g_intern_string (event_type);
  closure.walker = walker;
  closure.user_data = user_data;
  closure.destroy_user_data = destroy_user_data;

  g_array_append_val (self->walkers, closure);

  return self->walkers->len;
}

/**
 * dfl_event_sequence_remove_walker:
 * @self: a #DflEventSequence
 * @walker_id: ID of the walker to remove
 *
 * TODO
 *
 * Removing the same ID twice is an error.
 *
 * Since: 0.1.0
 */
void
dfl_event_sequence_remove_walker (DflEventSequence *self,
                                  guint             walker_id)
{
  DflEventSequenceWalkerClosure *closure;

  g_return_if_fail (DFL_IS_EVENT_SEQUENCE (self));
  g_return_if_fail (walker_id != 0 && walker_id <= self->walkers->len);

  /* Clear the given element of the walkers array, but don’t remove it so we
   * don’t muck up the IDs of other walkers. */
  closure = &g_array_index (self->walkers, DflEventSequenceWalkerClosure,
                            walker_id - 1);

  g_return_if_fail (closure->walker != NULL);
  walkers_clear_cb (closure);
}

/**
 * dfl_event_sequence_walk:
 * @self: a #DflEventSequence
 *
 * TODO
 *
 * It is allowed to add and remove walkers from callbacks within this function.
 *
 * Since: 0.1.0
 */
void
dfl_event_sequence_walk (DflEventSequence *self)
{
  guint i, j;

  g_return_if_fail (DFL_IS_EVENT_SEQUENCE (self));

  if (self->walkers->len == 0)
    return;

  for (i = 0; i < self->n_events; i++)
    {
      DflEvent *event = self->events[i];

      for (j = 0; j < self->walkers->len; j++)
        {
          const DflEventSequenceWalkerClosure *closure;

          closure = &g_array_index (self->walkers,
                                    DflEventSequenceWalkerClosure, j);

          /* Has the walker been removed? */
          if (closure->walker == NULL)
            continue;

          if (closure->event_type == NULL ||
              closure->event_type == dfl_event_get_event_type (event))
            {
              closure->walker (self, event, closure->user_data);
            }
        }
    }
}
