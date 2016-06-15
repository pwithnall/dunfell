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
 * An //event sequence// is a list of #DflEvents, in ascending time order. It
 * uses a moderately compact representation which is optimised for in-order
 * iteration over the sequence (‘walking’ over it from start to finish). It
 * supports matching events to dispatch callbacks ([‘walkers’](#walkers)) for
 * analysing the event sequence.
 *
 * # Walkers # {#walkers}
 *
 * A //walker// is a callback function which is executed for each event out of
 * the #DflEventSequence which matches a set of conditions. Walkers can match on
 * event type or event type and ID. Walking over the event sequence using
 * dfl_event_sequence_walk() will call all installed walkers which match each
 * event, in the order the events are presented in the sequence. There are no
 * guarantees about which order the walkers which match a particular event are
 * called in.
 *
 * Walkers can be installed on the #DflEventSequence using
 * dfl_event_sequence_add_walker(), which may be called at any time before or
 * during a walk over the event sequence (i.e. it may be called from with a
 * walker callback, and will be able to make its first match on the next event
 * in the sequence).
 *
 * Walkers can be removed using dfl_event_sequence_remove_walker(), which may
 * also be called at any time, although walkers must be removed at most once.
 * Any walkers remaining in the #DflEventSequence when it is destroyed are
 * freed.
 *
 * # Walker Groups # {#walker-groups}
 *
 * In order to simplify adding groups of walkers to a #DflEventSequence to match
 * events for a particular object instance (for example), #DflEventSequence
 * supports //walker groups//. These are groups of calls to
 * dfl_event_sequence_add_walker() which are tracked together, and are removed
 * as a group when a //removal event// is matched. Set the removal event by
 * calling dfl_event_sequence_end_walker_group() after the set of
 * dfl_event_sequence_add_walker() calls.
 *
 * Calls to dfl_event_sequence_start_walker_group() and
 * dfl_event_sequence_end_walker_group() must be strictly paired, and no walker
 * groups may be open when the #DflEventSequence is disposed.
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
  DflId id;  /* could be %DFL_ID_INVALID */
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

  GArray/*<guint>*/ *walker_group;  /* owned; nullable */
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
  closure->id = DFL_ID_INVALID;
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

  /* The programmer must have closed any walker groups before disposing the
   * event sequence. */
  g_assert (self->walker_group == NULL);

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
 * dfl_event_sequence_start_walker_group:
 * @self: a #DflEventSequence
 *
 * Start a new walker group. For more information, see
 * [Walker Groups](#walker-groups).
 *
 * It is an error to call this if a walker group has already been started and
 * has not yet been ended with a call to dfl_event_sequence_end_walker_group().
 *
 * Since: UNRELEASED
 */
void
dfl_event_sequence_start_walker_group (DflEventSequence *self)
{
  g_return_if_fail (DFL_IS_EVENT_SEQUENCE (self));
  g_return_if_fail (self->walker_group == NULL);

  self->walker_group = g_array_new (FALSE, FALSE, sizeof (guint));
}

static void
remove_walkers_cb (DflEventSequence *sequence,
                   DflEvent         *event,
                   gpointer          user_data)
{
  GArray/*<guint>*/ *walkers = user_data;  /* owned */
  gsize i, n_walkers;

  for (i = 0, n_walkers = walkers->len; i < n_walkers; i++)
    {
      guint walker_id = g_array_index (walkers, guint, i);
      dfl_event_sequence_remove_walker (sequence, walker_id);
    }
}

/**
 * dfl_event_sequence_end_walker_group:
 * @self: a #DflEventSequence
 * @event_type: event type to trigger removal of the walker group
 * @id: event ID to trigger removal of the walker group, or %DFL_ID_INVALID to
 *    match any event ID
 *
 * End the current walker group, and add a trigger to remove the walkers in the
 * group from the #DflEventSequence when an event matching @event_type and @id
 * is seen. @event_type must be specified; @id is optional. Matching is the same
 * as with dfl_event_sequence_add_walker().
 *
 * It is an error to call this if a walker group is not currently started.
 *
 * Since: UNRELEASED
 */
void
dfl_event_sequence_end_walker_group (DflEventSequence *self,
                                     const gchar      *event_type,
                                     DflId             id)
{
  g_return_if_fail (DFL_IS_EVENT_SEQUENCE (self));
  g_return_if_fail (event_type != NULL);
  g_return_if_fail (self->walker_group != NULL);

  /* Remove the walkers once the next @event_type is seen which matches @id. */
  if (self->walker_group->len > 0)
    {
      dfl_event_sequence_add_walker (self, event_type, id,
                                     remove_walkers_cb,
                                     g_array_ref (self->walker_group),
                                     (GDestroyNotify) g_array_unref);
    }

  g_clear_pointer (&self->walker_group, g_array_unref);
}

/**
 * dfl_event_sequence_add_walker:
 * @self: a #DflEventSequence
 * @event_type: (nullable): event type to match, or %NULL to match all event
 *    types
 * @id: event ID to match, or %DFL_ID_INVALID to match all event IDs
 * @walker: the event walker to add
 * @user_data: user data to pass to @walker
 * @destroy_user_data: (nullable): destroy handler for @user_data
 *
 * Add a walker to the event sequence, to be called in the next (or current)
 * call to dfl_event_sequence_walk(). For more information on walkers, see
 * [the summary](#walkers).
 *
 * The @event_type and @id are optional. If @id is specified, @event_type must
 * be specified. Only events which match both properties (if specified) are
 * returned to the @walker callback.
 *
 * The @user_data is held at least until the walker is removed from the
 * #DflEventSequence, but may be held longer. @destroy_user_data is called when
 * the @user_data is no longer needed.
 *
 * Returned walker IDs are guaranteed to never be zero, which is not a valid ID.
 *
 * Returns: the ID of the walker
 * Since: UNRELEASED
 */
guint
dfl_event_sequence_add_walker (DflEventSequence *self,
                               const gchar      *event_type,
                               DflId             id,
                               DflEventWalker    walker,
                               gpointer          user_data,
                               GDestroyNotify    destroy_user_data)
{
  DflEventSequenceWalkerClosure closure;

  g_return_val_if_fail (DFL_IS_EVENT_SEQUENCE (self), 0);
  g_return_val_if_fail (event_type == NULL || *event_type != '\0', 0);
  g_return_val_if_fail (event_type != NULL || id == DFL_ID_INVALID, 0);
  g_return_val_if_fail (walker != NULL, 0);

  closure.event_type = g_intern_string (event_type);
  closure.id = id;
  closure.walker = walker;
  closure.user_data = user_data;
  closure.destroy_user_data = destroy_user_data;

  g_array_append_val (self->walkers, closure);

  if (self->walker_group != NULL)
    g_array_append_val (self->walker_group, self->walkers->len);

  g_assert (self->walkers->len != 0);
  return self->walkers->len;
}

/**
 * dfl_event_sequence_remove_walker:
 * @self: a #DflEventSequence
 * @walker_id: ID of the walker to remove
 *
 * Remove a walker from the #DflEventSequence which was previously added using
 * dfl_event_sequence_add_walker().
 *
 * Removing the same ID twice, either using this function or as part of a walker
 * group, is an error.
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

          /* FIXME: Having the ID hard-coded in index 0 is a bit icky. */
          if ((closure->event_type == NULL ||
               closure->event_type == dfl_event_get_event_type (event)) &&
              (closure->id == DFL_ID_INVALID ||
               closure->id == dfl_event_get_parameter_id (event, 0)))
            {
              closure->walker (self, event, closure->user_data);
            }
        }
    }
}
