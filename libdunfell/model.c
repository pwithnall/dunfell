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
 * SECTION:model
 * @short_description: model containing analysed event information
 * @stability: Unstable
 * @include: libdunfell/model.h
 *
 * A model containing all the structured information extracted and analysed
 * from a #DflEventSequence. This is the main data model for presenting and
 * analysing statistics from a recorded event sequence.
 *
 * The analysis is performed at construction time and not updated afterwards;
 * the event sequence is immutable.
 *
 * Since: UNRELEASED
 */

#include "config.h"

#include <glib.h>
#include <glib-object.h>

#include "event-sequence.h"
#include "main-context.h"
#include "model.h"
#include "source.h"
#include "task.h"
#include "thread.h"


static void dfl_model_get_property (GObject      *object,
                                    guint         property_id,
                                    GValue       *value,
                                    GParamSpec   *pspec);
static void dfl_model_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec);
static void dfl_model_constructed  (GObject      *object);
static void dfl_model_finalize     (GObject      *object);

struct _DflModel
{
  GObject parent;

  /* Input data. */
  DflEventSequence *event_sequence;  /* (owned) */

  /* Results of analysis. */
  GPtrArray *main_contexts;  /* (owned) (element-type DflMainContext) */
  GPtrArray *threads;  /* (owned) (element-type DflThread) */
  GPtrArray *sources;  /* (owned) (element-type DflSource) */
  GPtrArray *tasks;  /* (owned) (element-type DflTask) */
};

G_DEFINE_TYPE (DflModel, dfl_model, G_TYPE_OBJECT)

typedef enum
{
  PROP_EVENT_SEQUENCE = 1,
} DflModelProperty;

static void
dfl_model_class_init (DflModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = dfl_model_get_property;
  object_class->set_property = dfl_model_set_property;
  object_class->constructed = dfl_model_constructed;
  object_class->finalize = dfl_model_finalize;

  /**
   * DflModel:event-sequence:
   *
   * Event sequence to analyse and extract information from.
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_EVENT_SEQUENCE,
                                   g_param_spec_object ("event-sequence",
                                                        "Event Sequence",
                                                        "Event sequence to "
                                                        "analyse and extract "
                                                        "information from.",
                                                        DFL_TYPE_EVENT_SEQUENCE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
}

static void
dfl_model_init (DflModel *self)
{
  /* Nothing to see here. */
}

static void
dfl_model_get_property (GObject     *object,
                        guint        property_id,
                        GValue      *value,
                        GParamSpec  *pspec)
{
  DflModel *self = DFL_MODEL (object);

  switch ((DflModelProperty) property_id)
    {
    case PROP_EVENT_SEQUENCE:
      g_value_set_object (value, self->event_sequence);
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
dfl_model_set_property (GObject           *object,
                        guint              property_id,
                        const GValue      *value,
                        GParamSpec        *pspec)
{
  DflModel *self = DFL_MODEL (object);

  switch ((DflModelProperty) property_id)
    {
    case PROP_EVENT_SEQUENCE:
      /* Construct only. */
      g_assert (self->event_sequence == NULL);
      self->event_sequence = g_value_dup_object (value);
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
dfl_model_constructed (GObject *object)
{
  DflModel *self = DFL_MODEL (object);

  /* Chain up first. */
  G_OBJECT_CLASS (dfl_model_parent_class)->constructed (object);

  /* Analyse the event sequence. */
  g_assert (self->event_sequence != NULL);

  self->main_contexts = dfl_main_context_factory_from_event_sequence (self->event_sequence);
  self->threads = dfl_thread_factory_from_event_sequence (self->event_sequence);
  self->sources = dfl_source_factory_from_event_sequence (self->event_sequence);
  self->tasks = dfl_task_factory_from_event_sequence (self->event_sequence);

  dfl_event_sequence_walk (self->event_sequence);
}

static void
dfl_model_finalize (GObject *object)
{
  DflModel *self = DFL_MODEL (object);

  g_clear_pointer (&self->main_contexts, g_ptr_array_unref);
  g_clear_pointer (&self->threads, g_ptr_array_unref);
  g_clear_pointer (&self->sources, g_ptr_array_unref);
  g_clear_pointer (&self->tasks, g_ptr_array_unref);

  g_clear_object (&self->event_sequence);

  G_OBJECT_CLASS (dfl_model_parent_class)->finalize (object);
}

/**
 * dfl_model_new:
 * @event_sequence: event sequence to analyse
 *
 * Construct a new #DflModel, analysing the events in the given @event_sequence.
 *
 * Returns: (transfer full): a new #DflModel
 * Since: UNRELEASED
 */
DflModel *
dfl_model_new (DflEventSequence *event_sequence)
{
  g_return_val_if_fail (DFL_IS_EVENT_SEQUENCE (event_sequence), NULL);

  return g_object_new (DFL_TYPE_MODEL,
                       "event-sequence", event_sequence,
                       NULL);
}

/**
 * dfl_model_get_event_sequence:
 * @self: a #DflModel
 *
 * Get the value of the #DflModel:event-sequence property.
 *
 * Returns: (transfer none): the event sequence
 * Since: UNRELEASED
 */
DflEventSequence *
dfl_model_get_event_sequence (DflModel *self)
{
  g_return_val_if_fail (DFL_IS_MODEL (self), NULL);

  return self->event_sequence;
}

/**
 * dfl_model_dup_main_contexts:
 * @self: a #DflModel
 *
 * Get the set of #DflMainContexts extracted from the event sequence. They are
 * returned in an undefined order.
 *
 * Returns: (transfer container) (element-type DflMainContext): the main
 *    contexts
 * Since: UNRELEASED
 */
GPtrArray *
dfl_model_dup_main_contexts (DflModel *self)
{
  g_return_val_if_fail (DFL_IS_MODEL (self), NULL);

  return g_ptr_array_ref (self->main_contexts);
}

/**
 * dfl_model_dup_threads:
 * @self: a #DflModel
 *
 * Get the set of #DflThreads extracted from the event sequence. They are
 * returned in an undefined order.
 *
 * Returns: (transfer container) (element-type DflThread): the threads
 * Since: UNRELEASED
 */
GPtrArray *
dfl_model_dup_threads (DflModel *self)
{
  g_return_val_if_fail (DFL_IS_MODEL (self), NULL);

  return g_ptr_array_ref (self->threads);
}

/**
 * dfl_model_dup_sources:
 * @self: a #DflModel
 *
 * Get the set of #DflSources extracted from the event sequence. They are
 * returned in an undefined order.
 *
 * Returns: (transfer container) (element-type DflSource): the sources
 * Since: UNRELEASED
 */
GPtrArray *
dfl_model_dup_sources (DflModel *self)
{
  g_return_val_if_fail (DFL_IS_MODEL (self), NULL);

  return g_ptr_array_ref (self->sources);
}

/**
 * dfl_model_dup_tasks:
 * @self: a #DflModel
 *
 * Get the set of #DflTasks extracted from the event sequence. They are
 * returned in an undefined order.
 *
 * Returns: (transfer container) (element-type DflTask): the tasks
 * Since: UNRELEASED
 */
GPtrArray *
dfl_model_dup_tasks (DflModel *self)
{
  g_return_val_if_fail (DFL_IS_MODEL (self), NULL);

  return g_ptr_array_ref (self->tasks);
}
