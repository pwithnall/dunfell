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

#include <glib.h>
#include <locale.h>

#include "dfl-event-sequence.h"


/* Test the properties of an empty event sequence. */
static void
test_event_sequence_empty (void)
{
  DflEventSequence *sequence = NULL;

  sequence = dfl_event_sequence_new (NULL, 0, 123456);

  g_assert_cmpuint (g_list_model_get_n_items (G_LIST_MODEL (sequence)), ==, 0);
  g_assert_null (g_list_model_get_item (G_LIST_MODEL (sequence), 0));
  g_assert_cmpuint (g_list_model_get_item_type (G_LIST_MODEL (sequence)), ==,
                    DFL_TYPE_EVENT);

  g_object_unref (sequence);
}

/* Test the properties of an event sequence containing a single event. */
static void
test_event_sequence_single (void)
{
  DflEventSequence *sequence = NULL;
  DflEvent *event = NULL;

  event = g_object_new (DFL_TYPE_EVENT, NULL);
  sequence = dfl_event_sequence_new ((const DflEvent **) &event, 1, 123456);
  g_object_unref (event);

  g_assert_cmpuint (g_list_model_get_n_items (G_LIST_MODEL (sequence)), ==, 1);
  g_assert (g_list_model_get_item (G_LIST_MODEL (sequence), 0) == event);
  g_assert (DFL_IS_EVENT (g_list_model_get_item (G_LIST_MODEL (sequence), 0)));
  g_assert (g_list_model_get_item (G_LIST_MODEL (sequence), 0) ==
            g_list_model_get_object (G_LIST_MODEL (sequence), 0));
  g_assert_null (g_list_model_get_item (G_LIST_MODEL (sequence), 1));
  g_assert_cmpuint (g_list_model_get_item_type (G_LIST_MODEL (sequence)), ==,
                    DFL_TYPE_EVENT);

  g_object_unref (sequence);
}

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/event-sequence/empty", test_event_sequence_empty);
  g_test_add_func ("/event-sequence/single", test_event_sequence_single);

  return g_test_run ();
}
