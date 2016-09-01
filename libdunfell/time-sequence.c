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
 * SECTION:time-sequence
 * @short_description: data structure for storing sequences of events
 * @stability: Unstable
 * @include: libdunfell/time-sequence.h
 *
 * TODO
 *
 * Since: 0.1.0
 */

#include "config.h"

#include <errno.h>
#include <glib.h>

#include "time-sequence.h"


typedef struct
{
  DflTimestamp timestamp;
  guint8 data[];
} DflTimeSequenceElement;

typedef struct
{
  DflTimeSequence *sequence;
  gsize index;
  DflTimeSequenceElement *last_returned_element;  /* (nullable) (ownership none) */
} DflTimeSequenceIterReal;

G_STATIC_ASSERT (sizeof (DflTimeSequenceIterReal) ==
                 sizeof (DflTimeSequenceIter));

G_DEFINE_BOXED_TYPE (DflTimeSequenceIter, dfl_time_sequence_iter,
                     dfl_time_sequence_iter_copy, dfl_time_sequence_iter_free)

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
 * Since: 0.1.0
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

  g_assert (index < self->n_elements_valid);

  element = ((guint8 *) self->elements +
             index * (sizeof (DflTimeSequenceElement) + self->element_size));
  return (DflTimeSequenceElement *) element;
}

/* Find the element with the largest timestamp ≤ @timestamp and return its
 * index in @index. If there are multiple elements with the same timestamp,
 * return the first of them. Return %FALSE if no element with a timestamp ≤
 * @timestamp was found; return %TRUE otherwise. If %FALSE is returned, @index
 * is guaranteed to be set to 0. */
static gboolean
dfl_time_sequence_find_timestamp (DflTimeSequence *sequence,
                                  DflTimestamp     timestamp,
                                  gsize           *index)
{
  DflTimeSequenceReal *self = (DflTimeSequenceReal *) sequence;
  gsize left_index, right_index, middle;
  DflTimeSequenceElement *element;

  /* Trivial cases. */
  if (self->n_elements_valid == 0)
    {
      *index = 0;
      return FALSE;
    }

  if (timestamp == 0)
    {
      *index = 0;
      return TRUE;
    }

  /* Binary search. */
  left_index = 0;
  right_index = self->n_elements_valid - 1;

  do
    {
      middle = (left_index + right_index) / 2;  /* truncate */
      element = dfl_time_sequence_index (sequence, middle);

      if (element->timestamp > timestamp && middle > 0)
        right_index = middle - 1;
      else if (element->timestamp < timestamp && middle < G_MAXSIZE)
        left_index = middle + 1;
      else
        break;
    }
  while (left_index < right_index);

  /* Work backwards until we find the first of the matching elements. (Multiple
   * elements can have the same timestamp.) */
  while (element->timestamp >= timestamp && middle > 0)
    {
      middle--;
      element = dfl_time_sequence_index (sequence, middle);
    }

  if (element->timestamp <= timestamp)
    {
      *index = middle;
      return TRUE;
    }
  else
    {
      *index = 0;
      return FALSE;
    }
}

/**
 * dfl_time_sequence_clear:
 * @sequence: an initialised #DflTimeSequence
 *
 * TODO
 *
 * Since: 0.1.0
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
 * Since: 0.1.0
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
 * Since: 0.1.0
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
 * Since: 0.1.0
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
  self->last_returned_element = NULL;
  dfl_time_sequence_find_timestamp (sequence, start, &self->index);
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
 * Since: 0.1.0
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
    {
      self->last_returned_element = NULL;
      return FALSE;
    }

  /* Return the next element. */
  element = dfl_time_sequence_index (self->sequence, self->index);
  self->last_returned_element = element;

  if (timestamp != NULL)
    *timestamp = element->timestamp;
  if (data != NULL)
    *data = element->data;

  self->index++;

  return TRUE;
}

/**
 * dfl_time_sequence_iter_previous:
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
dfl_time_sequence_iter_previous (DflTimeSequenceIter *iter,
                                 DflTimestamp        *timestamp,
                                 gpointer            *data)
{
  DflTimeSequenceIterReal *self = (DflTimeSequenceIterReal *) iter;
  DflTimeSequenceElement *element;

  g_return_val_if_fail (dfl_time_sequence_iter_is_valid (iter), FALSE);

  /* Reached the end? */
  if (self->index == 0)
    {
      self->last_returned_element = NULL;
      return FALSE;
    }

  /* Return the previous element. */
  self->index--;
  element = dfl_time_sequence_index (self->sequence, self->index);
  self->last_returned_element = element;

  if (timestamp != NULL)
    *timestamp = element->timestamp;
  if (data != NULL)
    *data = element->data;

  return (self->index > 0);
}

/**
 * dfl_time_sequence_iter_copy:
 * @iter: a #DflTimeSequenceIter
 *
 * Copy a #DflTimeSequenceIter, resulting in a second iterator which can be
 * iterated independently of the original.
 *
 * Returns: (transfer full): copy of @iter
 * Since: UNRELEASED
 */
DflTimeSequenceIter *
dfl_time_sequence_iter_copy (DflTimeSequenceIter *iter)
{
  g_autoptr (DflTimeSequenceIter) new_iter = NULL;
  DflTimeSequenceIterReal *new_iter_real, *iter_real;

  g_return_val_if_fail (dfl_time_sequence_iter_is_valid (iter), NULL);

  new_iter = g_new0 (DflTimeSequenceIter, 1);
  new_iter_real = (DflTimeSequenceIterReal *) new_iter;
  iter_real = (DflTimeSequenceIterReal *) iter;

  new_iter_real->sequence = iter_real->sequence;
  new_iter_real->index = iter_real->index;
  new_iter_real->last_returned_element = iter_real->last_returned_element;

  return g_steal_pointer (&new_iter);
}

/**
 * dfl_time_sequence_iter_free:
 * @iter: (transfer full): a #DflTimeSequenceIter
 *
 * Free a heap-allocated #DflTimeSequenceIter, such as allocated by
 * dfl_time_sequence_iter_copy().
 *
 * Since: UNRELEASED
 */
void
dfl_time_sequence_iter_free (DflTimeSequenceIter *iter)
{
  g_return_if_fail (dfl_time_sequence_iter_is_valid (iter));

  g_free (iter);
}

/**
 * dfl_time_sequence_iter_equal:
 * @iter1: a #DflTimeSequenceIter
 * @iter2: another #DflTimeSequenceIter
 *
 * Check whether @iter1 and @iter2 point to the same position in the same time
 * sequence. It is an error to call this function on two iterators which come
 * from different #DflTimeSequences.
 *
 * Returns: %TRUE if the iterators point to the same position; %FALSE otherwise
 * Since: UNRELEASED
 */
gboolean
dfl_time_sequence_iter_equal (DflTimeSequenceIter *iter1,
                              DflTimeSequenceIter *iter2)
{
  DflTimeSequenceIterReal *iter1_real, *iter2_real;

  g_return_val_if_fail (dfl_time_sequence_iter_is_valid (iter1), FALSE);
  g_return_val_if_fail (dfl_time_sequence_iter_is_valid (iter2), FALSE);

  iter1_real = (DflTimeSequenceIterReal *) iter1;
  iter2_real = (DflTimeSequenceIterReal *) iter2;

  /* Note: Equality is not defined for iters from different sequences. */
  g_return_val_if_fail (iter1_real->sequence == iter2_real->sequence, FALSE);

  return (iter1_real->index == iter2_real->index);
}

/**
 * dfl_time_sequence_iter_get_timestamp:
 * @iter: a #DflTimeSequenceIter
 *
 * Get the timestamp returned by the most recent call to
 * dfl_time_sequence_iter_next() or dfl_time_sequence_iter_previous(). If at
 * the beginning or end of the time sequence, 0 will be returned.
 *
 * Returns: timestamp of @iter’s position in the time sequence
 * Since: UNRELEASED
 */
DflTimestamp
dfl_time_sequence_iter_get_timestamp (DflTimeSequenceIter *iter)
{
  DflTimeSequenceIterReal *self = (DflTimeSequenceIterReal *) iter;

  g_return_val_if_fail (dfl_time_sequence_iter_is_valid (iter), 0);

  if (self->last_returned_element == NULL)
    return 0;

  return self->last_returned_element->timestamp;
}

/**
 * dfl_time_sequence_iter_get_data:
 * @iter: a #DflTimeSequenceIter
 *
 * Get the data returned by the most recent call to
 * dfl_time_sequence_iter_next() or dfl_time_sequence_previous(). If at the
 * beginning or end of the time sequence, %NULL will be returned.
 *
 * Returns: data from @iter’s position in the time sequence
 * Since: UNRELEASED
 */
gpointer
dfl_time_sequence_iter_get_data (DflTimeSequenceIter *iter)
{
  DflTimeSequenceIterReal *self = (DflTimeSequenceIterReal *) iter;

  g_return_val_if_fail (dfl_time_sequence_iter_is_valid (iter), NULL);

  if (self->last_returned_element == NULL)
    return NULL;

  return self->last_returned_element->data;
}
