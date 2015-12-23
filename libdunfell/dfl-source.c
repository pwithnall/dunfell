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
 * SECTION:dfl-source
 * @short_description: model of a #GMainContext from a log
 * @stability: Unstable
 * @include: libdunfell/dfl-source.h
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
#include "dfl-source.h"
#include "dfl-time-sequence.h"


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
};

G_DEFINE_TYPE (DflSource, dfl_source, G_TYPE_OBJECT)

static void
dfl_source_class_init (DflSourceClass *klass)
{
  /* Nothing to see here. */
}

static void
dfl_source_init (DflSource *self)
{
  /* Nothing to see here. */
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
 * Since: UNRELEASED
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
source_before_free_cb (DflEventSequence *sequence,
                       DflEvent         *event,
                       gpointer          user_data)
{
  DflSource *source = user_data;
  DflTimestamp timestamp;

  /* Does this event correspond to the right source? */
  if (dfl_event_get_parameter_id (event, 0) != source->id ||
      source->free_timestamp != 0)
    return;

  timestamp = dfl_event_get_timestamp (event);
  source->free_timestamp = timestamp;
}

static void
source_new_cb (DflEventSequence *sequence,
               DflEvent         *event,
               gpointer          user_data)
{
  GPtrArray/*<owned DflSource>*/ *sources = user_data;
  DflSource *source = NULL;

//printdln (",", "g_source_new", gettimeofday_us (), tid (), source, usymname (source_funcs->prepare), usymname (source_funcs->check), usymname (source_funcs->dispatch), usymname (source_funcs->finalize), struct_size);

  source = dfl_source_new (dfl_event_get_parameter_id (event, 0),
                           dfl_event_get_timestamp (event),
                           dfl_event_get_thread_id (event));

  dfl_event_sequence_add_walker (sequence, "g_source_before_free",
                                 source_before_free_cb,
                                 g_object_ref (source),
                                 (GDestroyNotify) g_object_unref);

  /* TODO: When do these get removed? */

  /* TODO: Other walkers for different members. */

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
 * Since: UNRELEASED
 */
GPtrArray *
dfl_source_factory_from_event_sequence (DflEventSequence *sequence)
{
  GPtrArray/*<owned DflSource>*/ *sources = NULL;

  sources = g_ptr_array_new_with_free_func (g_object_unref);
  dfl_event_sequence_add_walker (sequence, "g_source_new", source_new_cb,
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
 * Since: UNRELEASED
 */
DflId
dfl_source_get_id (DflSource *self)
{
  g_return_val_if_fail (DFL_IS_SOURCE (self), DFL_ID_INVALID);

  return self->id;
}

/**
 * dfl_source_get_new_timestamp:
 * @self: a #DflSource
 *
 * TODO
 *
 * Returns: TODO
 * Since: UNRELEASED
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
 * Since: UNRELEASED
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
 * Since: UNRELEASED
 */
DflThreadId
dfl_source_get_new_thread_id (DflSource *self)
{
  g_return_val_if_fail (DFL_IS_SOURCE (self), DFL_ID_INVALID);

  return self->new_thread_id;
}
