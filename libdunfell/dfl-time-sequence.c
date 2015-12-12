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
 * SECTION:dfl-time-sequence
 * @short_description: data structure for storing sequences of events
 * @stability: Unstable
 * @include: libdunfell/dfl-time-sequence.h
 *
 * TODO
 *
 * Since: UNRELEASED
 */

#include "config.h"

#include <errno.h>
#include <glib.h>

#include "dfl-time-sequence.h"


typedef struct
{
  DflTimestamp timestamp;
  guint8 data[];
} DflTimeSequenceElement;

typedef struct
{
  DflTimeSequence *sequence;
  gsize index;
} DflTimeSequenceIterReal;

G_STATIC_ASSERT (sizeof (DflTimeSequenceIterReal) ==
                 sizeof (DflTimeSequenceIter));

typedef struct
{
  gsize element_size;
  GDestroyNotify element_destroy_notify;
  gsize n_elements_valid;
  gsize n_elements_allocated;
  gpointer *elements;  /* actually DflTimeSequenceElement+element_size */
} DflTimeSequenceReal;

G_STATIC_ASSERT (sizeof (DflTimeSequenceReal) == sizeof (DflTimeSequence));

/**
 * dfl_time_sequence_init:
 * @sequence: an uninitialised #DflTimeSequence
 * @element_size: size of the element data, in bytes
 * @element_destroy_notify: (nullable): function to free an element when it is
 *    no longer needed, or %NULL if unnecessary
 * @n_elements_preallocated: number of elements to preallocate space for
 *
 * TODO
 *
 * Since: UNRELEASED
 */
void
dfl_time_sequence_init (DflTimeSequence *sequence,
                        gsize            element_size,
                        GDestroyNotify   element_destroy_notify,
                        gsize            n_elements_preallocated)
{
  DflTimeSequenceReal *self = (DflTimeSequenceReal *) sequence;

  g_return_if_fail (sequence != NULL);
  g_return_if_fail (element_size <= G_MAXSIZE - sizeof (DflTimeSequenceElement));

  self->element_size = element_size;
  self->element_destroy_notify = element_destroy_notify;
  self->n_elements_valid = 0;
  self->n_elements_allocated = n_elements_preallocated;
  self->elements = g_malloc_n (n_elements_preallocated,
                               sizeof (DflTimeSequenceElement) + element_size);
}

static DflTimeSequenceElement *
dfl_time_sequence_index (DflTimeSequence *sequence,
                         gsize            index)
{
  DflTimeSequenceReal *self = (DflTimeSequenceReal *) sequence;
  gpointer element;

  element = ((guint8 *) self->elements +
             index * (sizeof (DflTimeSequenceElement) + self->element_size));
  return (DflTimeSequenceElement *) element;
}

/**
 * dfl_time_sequence_clear:
 * @sequence: an initialised #DflTimeSequence
 *
 * TODO
 *
 * Since: UNRELEASED
 */
void
dfl_time_sequence_clear (DflTimeSequence *sequence)
{
  DflTimeSequenceReal *self = (DflTimeSequenceReal *) sequence;
  gsize i;

  g_return_if_fail (sequence != NULL);

  if (self->element_destroy_notify != NULL)
    {
      for (i = 0; i < self->n_elements_valid; i++)
        {
          DflTimeSequenceElement *element;

          element = dfl_time_sequence_index (sequence, i);
          self->element_destroy_notify (element->data);
        }
    }

  g_free (self->elements);
  self->elements = NULL;
  self->n_elements_valid = 0;
  self->n_elements_allocated = 0;
}

/**
 * dfl_time_sequence_get_last_element:
 * @sequence: a #DflTimeSequence
 * @timestamp: (out caller-allocates) (optional): TODO
 *
 * TODO
 *
 * Returns: (transfer none) (nullable): TODO
 * Since: UNRELEASED
 */
gpointer
dfl_time_sequence_get_last_element (DflTimeSequence *sequence,
                                    DflTimestamp    *timestamp)
{
  DflTimeSequenceReal *self = (DflTimeSequenceReal *) sequence;
  gpointer element_data;
  DflTimestamp element_timestamp;

  g_return_val_if_fail (sequence != NULL, NULL);

  if (self->n_elements_valid == 0)
    {
      element_data = NULL;
      element_timestamp = 0;
    }
  else
    {
      DflTimeSequenceElement *element;

      element = dfl_time_sequence_index (sequence, self->n_elements_valid - 1);
      element_data = element->data;
      element_timestamp = element->timestamp;
    }

  if (timestamp != NULL)
    *timestamp = element_timestamp;

  return element_data;
}

/**
 * dfl_time_sequence_append:
 * @sequence: a #DflTimeSequence
 * @timestamp: TODO
 *
 * TODO
 *
 * Returns: (transfer none): TODO
 * Since: UNRELEASED
 */
gpointer
dfl_time_sequence_append (DflTimeSequence *sequence,
                          DflTimestamp     timestamp)
{
  DflTimeSequenceReal *self = (DflTimeSequenceReal *) sequence;
  DflTimestamp last_timestamp;
  gpointer last_element;
  DflTimeSequenceElement *element;

  g_return_val_if_fail (sequence != NULL, NULL);
  g_return_val_if_fail (self->n_elements_valid < G_MAXSIZE, NULL);

  /* Check @timestamp is monotonically increasing. */
  last_element = dfl_time_sequence_get_last_element (sequence, &last_timestamp);
  g_return_val_if_fail (last_element == NULL || timestamp >= last_timestamp,
                       NULL);

  /* Do we need to expand the array first? */
  if (self->n_elements_valid == self->n_elements_allocated)
    {
      self->n_elements_allocated =
        ((gsize) 1 << (g_bit_nth_msf (self->n_elements_allocated, -1) + 1));
      self->elements = g_realloc_n (self->elements, self->n_elements_allocated,
                                    sizeof (DflTimeSequenceElement) +
                                    self->element_size);
    }

  g_assert (self->n_elements_allocated > self->n_elements_valid);

  /* Append the new element. */
  self->n_elements_valid++;

  element = dfl_time_sequence_index (sequence, self->n_elements_valid - 1);
  element->timestamp = timestamp;

  return element->data;
}

static gboolean
dfl_time_sequence_iter_is_valid (DflTimeSequenceIter *iter)
{
  DflTimeSequenceIterReal *self = (DflTimeSequenceIterReal *) iter;

  return (self != NULL &&
          self->sequence != NULL &&
          self->index <=
          ((DflTimeSequenceReal *) self->sequence)->n_elements_valid);
}

/**
 * dfl_time_sequence_iter_init:
 * @iter: an uninitialised #DflTimeSequenceIter
 * @sequence: the #DflTimeSequence to iterate over
 * @start: TODO
 *
 * TODO
 *
 * Since: UNRELEASED
 */
void
dfl_time_sequence_iter_init (DflTimeSequenceIter *iter,
                             DflTimeSequence     *sequence,
                             DflTimestamp         start)
{
  DflTimeSequenceIterReal *self = (DflTimeSequenceIterReal *) iter;

  g_return_if_fail (iter != NULL);
  g_return_if_fail (sequence != NULL);

  self->sequence = sequence;
  self->index = 0;  /* TODO: handle @start */
}

/**
 * dfl_time_sequence_iter_next:
 * @iter: a #DflTimeSequenceIter
 * @timestamp: (out caller-allocates) (optional): TODO
 * @data: (out caller-allocates) (optional) (nullable): TODO
 *
 * TODO
 *
 * Returns: TODO
 * Since: UNRELEASED
 */
gboolean
dfl_time_sequence_iter_next (DflTimeSequenceIter *iter,
                             DflTimestamp        *timestamp,
                             gpointer            *data)
{
  DflTimeSequenceIterReal *self = (DflTimeSequenceIterReal *) iter;
  DflTimeSequenceReal *sequence;
  DflTimeSequenceElement *element;

  g_return_val_if_fail (dfl_time_sequence_iter_is_valid (iter), FALSE);

  sequence = (DflTimeSequenceReal *) self->sequence;

  /* Reached the end? */
  if (self->index >= sequence->n_elements_valid)
    return FALSE;

  /* Return the next element. */
  element = dfl_time_sequence_index (self->sequence, self->index);

  if (timestamp != NULL)
    *timestamp = element->timestamp;
  if (data != NULL)
    *data = element->data;

  self->index++;

  return TRUE;
}
