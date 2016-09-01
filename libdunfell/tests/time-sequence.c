/* vim:set et sw=2 cin cino=t0,f0,(0,{s,>2s,n-s,^-s,e2s: */
/*
 * Copyright © Collabora Ltd. 2016
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

#include <glib.h>
#include <locale.h>

#include "time-sequence.h"


/* Test the properties of an empty time sequence. */
static void
test_time_sequence_empty (void)
{
  g_auto (DflTimeSequence) sequence;
  DflTimestamp timestamp = 123;  /* arbitrary non-zero value */

  dfl_time_sequence_init (&sequence, sizeof (guint), NULL, 0);

  /* Properties. */
  g_assert_null (dfl_time_sequence_get_last_element (&sequence, &timestamp));
  g_assert_cmpuint (timestamp, ==, 0);
}

/* Test iteration over an empty time sequence. */
static void
test_time_sequence_iter_empty (void)
{
  g_auto (DflTimeSequence) sequence;
  DflTimeSequenceIter iter;
  DflTimestamp timestamp;
  gpointer data;
  const DflTimestamp start_timestamps[] = { 0, 100 };
  gsize i;

  dfl_time_sequence_init (&sequence, sizeof (guint), NULL, 0);

  /* Try starting from different timestamps; behaviour should be the same. */
  for (i = 0; i < G_N_ELEMENTS (start_timestamps); i++)
    {
      g_test_message ("i: %" G_GSIZE_FORMAT, i);

      dfl_time_sequence_iter_init (&iter, &sequence, start_timestamps[i]);

      timestamp = 123;  /* arbitrary non-zero value */
      data = &timestamp;  /* arbitrary non-NULL pointer */

      g_assert_false (dfl_time_sequence_iter_next (&iter, &timestamp, &data));
      g_assert_cmpuint (timestamp, ==, 123);
      g_assert (data == &timestamp);

      timestamp = 123;
      data = &timestamp;

      g_assert_false (dfl_time_sequence_iter_previous (&iter, &timestamp,
                                                       &data));
      g_assert_cmpuint (timestamp, ==, 123);
      g_assert (data == &timestamp);

      g_assert_cmpuint (dfl_time_sequence_iter_get_timestamp (&iter), ==, 0);
      g_assert_null (dfl_time_sequence_iter_get_data (&iter));
    }
}

/* Test the properties of a single-element time sequence. */
static void
test_time_sequence_single (void)
{
  g_auto (DflTimeSequence) sequence;
  DflTimestamp timestamp = 123;  /* arbitrary non-zero value */
  guint *element;

  dfl_time_sequence_init (&sequence, sizeof (guint), NULL, 0);
  element = dfl_time_sequence_append (&sequence, 100);
  g_assert_nonnull (element);

  /* Properties. */
  g_assert (dfl_time_sequence_get_last_element (&sequence, &timestamp) ==
            element);
  g_assert_cmpuint (timestamp, ==, 100);
}

/* Test iteration over a single-element time sequence. */
static void
test_time_sequence_iter_single (void)
{
  g_auto (DflTimeSequence) sequence;
  gpointer element;
  DflTimeSequenceIter iter;
  DflTimestamp timestamp;
  gpointer data;
  const DflTimestamp start_timestamps[] = { 0, 50, 100, 500 };
  gsize i;

  dfl_time_sequence_init (&sequence, sizeof (guint), NULL, 0);
  element = dfl_time_sequence_append (&sequence, 100);
  g_assert_nonnull (element);

  /* Try starting from different timestamps; behaviour should be the same
   * regardless of whether the timestamp is ≤ or > 100 because there is only
   * one element. If the start_timestamp is ≤ 100, the iterator should start at
   * the beginning. If the start_timestamp is > 100, the iterator should start
   * at the greatest element ≤ 100, which is the beginning. Try going forwards
   * (success), forwards (failure at the end), backwards (success), backwards
   * (failure at the beginning), forwards (success), forwards (failure at the
   * end). */
  for (i = 0; i < G_N_ELEMENTS (start_timestamps); i++)
    {
      g_test_message ("i: %" G_GSIZE_FORMAT, i);

      dfl_time_sequence_iter_init (&iter, &sequence, start_timestamps[i]);

      timestamp = 123;  /* arbitrary non-zero value */
      data = &timestamp;  /* arbitrary non-NULL pointer */

      g_assert_true (dfl_time_sequence_iter_next (&iter, &timestamp, &data));
      g_assert_cmpuint (timestamp, ==, 100);
      g_assert (data == element);
      g_assert_cmpuint (dfl_time_sequence_iter_get_timestamp (&iter), ==, 100);
      g_assert (dfl_time_sequence_iter_get_data (&iter) == element);

      g_assert_false (dfl_time_sequence_iter_next (&iter, NULL, NULL));
      g_assert_cmpuint (dfl_time_sequence_iter_get_timestamp (&iter), ==, 0);
      g_assert_null (dfl_time_sequence_iter_get_data (&iter));

      timestamp = 123;
      data = &timestamp;

      g_assert_true (dfl_time_sequence_iter_previous (&iter, &timestamp,
                                                      &data));
      g_assert_cmpuint (timestamp, ==, 100);
      g_assert (data == element);
      g_assert_cmpuint (dfl_time_sequence_iter_get_timestamp (&iter), ==, 100);
      g_assert (dfl_time_sequence_iter_get_data (&iter) == element);

      g_assert_false (dfl_time_sequence_iter_previous (&iter, NULL, NULL));
      g_assert_cmpuint (dfl_time_sequence_iter_get_timestamp (&iter), ==, 0);
      g_assert_null (dfl_time_sequence_iter_get_data (&iter));

      timestamp = 123;
      data = &timestamp;

      g_assert_true (dfl_time_sequence_iter_next (&iter, &timestamp, &data));
      g_assert_cmpuint (timestamp, ==, 100);
      g_assert (data == element);
      g_assert_cmpuint (dfl_time_sequence_iter_get_timestamp (&iter), ==, 100);
      g_assert (dfl_time_sequence_iter_get_data (&iter) == element);

      g_assert_false (dfl_time_sequence_iter_next (&iter, NULL, NULL));
      g_assert_cmpuint (dfl_time_sequence_iter_get_timestamp (&iter), ==, 0);
      g_assert_null (dfl_time_sequence_iter_get_data (&iter));
    }
}

/* Test the properties of a multiple-element time sequence. */
static void
test_time_sequence_multiple (void)
{
  g_auto (DflTimeSequence) sequence;
  DflTimestamp timestamp = 123;  /* arbitrary non-zero value */
  guint *last_element;

  dfl_time_sequence_init (&sequence, sizeof (guint), NULL, 0);
  dfl_time_sequence_append (&sequence, 100);
  dfl_time_sequence_append (&sequence, 200);
  last_element = dfl_time_sequence_append (&sequence, 300);
  g_assert_nonnull (last_element);

  /* Properties. */
  g_assert (dfl_time_sequence_get_last_element (&sequence, &timestamp) ==
            last_element);
  g_assert_cmpuint (timestamp, ==, 300);
}

/* Test iteration over a multiple-element time sequence. */
static void
test_time_sequence_iter_multiple (void)
{
  g_auto (DflTimeSequence) sequence;
  DflTimeSequenceIter iter;
  const DflTimestamp timestamps[] = { 10, 50, 100, 100, 200, 500 };
  struct
    {
      DflTimestamp start_timestamp;
      DflTimestamp expected_next_timestamp;
      DflTimestamp expected_previous_timestamp;
    } start_timestamps[] = {
      { 0, 10, 0 },
      { 1, 10, 0 },
      { 10, 10, 0 },
      { 50, 50, 10 },
      { 99, 50, 10 },
      { 100, 100, 50 },
      { 101, 100, 50 },
      { 200, 200, 100 },
      { 500, 500, 200 },
      { 600, 500, 200 },
    };
  gsize i;

  /* Set up the sequence. */
  dfl_time_sequence_init (&sequence, sizeof (guint), NULL, 0);

  for (i = 0; i < G_N_ELEMENTS (timestamps); i++)
    {
      guint *data;

      g_test_message ("i: %" G_GSIZE_FORMAT, i);

      data = dfl_time_sequence_append (&sequence, timestamps[i]);
      g_assert_nonnull (data);

      *data = (guint) i;
    }

  /* Try iterating over it in entirety. */
  dfl_time_sequence_iter_init (&iter, &sequence, 0);

  for (i = 0; i < G_N_ELEMENTS (timestamps); i++)
    {
      DflTimestamp timestamp;
      guint *data;

      g_test_message ("i: %" G_GSIZE_FORMAT, i);

      g_assert_true (dfl_time_sequence_iter_next (&iter, &timestamp,
                                                  (gpointer *) &data));
      g_assert_cmpuint (timestamp, ==, timestamps[i]);
      g_assert_cmpuint (*data, ==, i);
    }

  g_assert_false (dfl_time_sequence_iter_next (&iter, NULL, NULL));

  /* And backwards. */
  for (i = 0; i < G_N_ELEMENTS (timestamps); i++)
    {
      DflTimestamp timestamp;
      guint *data;

      g_test_message ("i: %" G_GSIZE_FORMAT, G_N_ELEMENTS (timestamps) - 1 - i);

      g_assert_true (dfl_time_sequence_iter_previous (&iter, &timestamp,
                                                      (gpointer *) &data));
      g_assert_cmpuint (timestamp, ==,
                        timestamps[G_N_ELEMENTS (timestamps) - 1 - i]);
      g_assert_cmpuint (*data, ==, G_N_ELEMENTS (timestamps) - 1 - i);
    }

  g_assert_false (dfl_time_sequence_iter_previous (&iter, NULL, NULL));

  /* Try starting from different timestamps. */
  for (i = 0; i < G_N_ELEMENTS (start_timestamps); i++)
    {
      DflTimestamp timestamp;

      g_test_message ("i: %" G_GSIZE_FORMAT, i);

      dfl_time_sequence_iter_init (&iter, &sequence,
                                   start_timestamps[i].start_timestamp);

      g_assert_true (dfl_time_sequence_iter_next (&iter, &timestamp, NULL));
      g_assert_cmpuint (timestamp, ==,
                        start_timestamps[i].expected_next_timestamp);

      g_assert_true (dfl_time_sequence_iter_previous (&iter, NULL, NULL));

      if (start_timestamps[i].expected_previous_timestamp != 0)
        {
          g_assert_true (dfl_time_sequence_iter_previous (&iter, &timestamp,
                                                          NULL));
          g_assert_cmpuint (timestamp, ==,
                            start_timestamps[i].expected_previous_timestamp);
        }
    }
}

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/time-sequence/empty", test_time_sequence_empty);
  g_test_add_func ("/time-sequence/iter/empty", test_time_sequence_iter_empty);
  g_test_add_func ("/time-sequence/single", test_time_sequence_single);
  g_test_add_func ("/time-sequence/iter/single",
                   test_time_sequence_iter_single);
  g_test_add_func ("/time-sequence/multiple", test_time_sequence_multiple);
  g_test_add_func ("/time-sequence/iter/multiple",
                   test_time_sequence_iter_multiple);

  return g_test_run ();
}
