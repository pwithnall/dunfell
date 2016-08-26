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

#include "event-sequence.h"


typedef struct
{
  const gchar *event_type;
  DflId id;
} EventVector;

/* Utility function to create a GPtrArray of DflEvents from some event
 * vectors. */
static GPtrArray/*<owned DflEvent>*/ *
event_array_from_vectors (const EventVector *events,
                          gsize              n_events)
{
  GPtrArray/*<owned DflEvent>*/ *output = NULL;
  gsize i;

  output = g_ptr_array_new_with_free_func (g_object_unref);

  for (i = 0; i < n_events; i++)
    {
      DflEvent *event = NULL;
      gchar *parameters[2] = { NULL, };

      parameters[0] = g_strdup_printf ("%" G_GUINT64_FORMAT, events[i].id);
      event = dfl_event_new (events[i].event_type, i, 0  /* thread ID */,
                             (const gchar * const *) parameters);
      g_free (parameters[0]);

      g_ptr_array_add (output, event);  /* transfer */
    }

  return output;
}

static DflEventSequence *
event_sequence_from_vectors (const EventVector *vectors,
                             gsize              n_vectors)
{
  DflEventSequence *sequence = NULL;
  GPtrArray/*<owned DflEvent>*/ *events = NULL;

  events = event_array_from_vectors (vectors, n_vectors);
  sequence = dfl_event_sequence_new ((const DflEvent **) events->pdata,
                                     events->len, 0);
  g_ptr_array_unref (events);

  return sequence;
}

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

/* Test that walking over a short sequence with no walkers added is OK. */
static void
test_event_sequence_walk_no_walkers (void)
{
  DflEventSequence *sequence = NULL;
  const EventVector vectors[] = {
    { "type_a", 1 },
    { "type_b", 2 },
  };

  sequence = event_sequence_from_vectors (vectors, G_N_ELEMENTS (vectors));
  dfl_event_sequence_walk (sequence);
  g_object_unref (sequence);
}

static void
walker_not_reached (DflEventSequence *sequence,
                    DflEvent         *event,
                    gpointer          user_data) G_GNUC_NORETURN;

static void
walker_not_reached (DflEventSequence *sequence,
                    DflEvent         *event,
                    gpointer          user_data)
{
  g_assert_not_reached ();
}

static void
walker_match_vector (DflEventSequence *sequence,
                     DflEvent         *event,
                     gpointer          user_data)
{
  const EventVector *vector = user_data;

  g_assert_cmpstr (dfl_event_get_event_type (event), ==, vector->event_type);
  g_assert_cmpuint (dfl_event_get_parameter_id (event, 0), ==, vector->id);
}

static void
walker_match_event_type (DflEventSequence *sequence,
                         DflEvent         *event,
                         gpointer          user_data)
{
  const gchar *event_type = user_data;

  g_assert_cmpstr (dfl_event_get_event_type (event), ==, event_type);
}

static void
walker_count (DflEventSequence *sequence,
              DflEvent         *event,
              gpointer          user_data)
{
  guint *counter = user_data;

  *counter = *counter + 1;
}

/* Test that walking over an empty sequence generates no callbacks. */
static void
test_event_sequence_walk_empty (void)
{
  DflEventSequence *sequence = NULL;

  sequence = dfl_event_sequence_new (NULL, 0, 123456);

  dfl_event_sequence_add_walker (sequence, NULL, DFL_ID_INVALID,
                                 walker_not_reached, NULL, NULL);
  dfl_event_sequence_walk (sequence);

  g_object_unref (sequence);
}

/* Test that walking over a singleton sequence generates one callback. */
static void
test_event_sequence_walk_singleton (void)
{
  DflEventSequence *sequence = NULL;
  const EventVector vectors[] = {
    { "type_a", 1 },
  };

  sequence = event_sequence_from_vectors (vectors, G_N_ELEMENTS (vectors));
  dfl_event_sequence_add_walker (sequence, NULL, DFL_ID_INVALID,
                                 walker_match_vector,
                                 (gpointer) &vectors[0], NULL);
  dfl_event_sequence_walk (sequence);
  g_object_unref (sequence);
}

/* Test that walking over a short sequence generates callbacks which match by
 * event type. */
static void
test_event_sequence_walk_event_type (void)
{
  DflEventSequence *sequence = NULL;
  const EventVector vectors[] = {
    { "type_a", 1 },
    { "type_a", 2 },
    { "type_b", 3 },
  };

  sequence = event_sequence_from_vectors (vectors, G_N_ELEMENTS (vectors));
  dfl_event_sequence_add_walker (sequence, "type_a", DFL_ID_INVALID,
                                 walker_match_event_type,
                                 (gpointer) "type_a", NULL);
  dfl_event_sequence_add_walker (sequence, "type_b", DFL_ID_INVALID,
                                 walker_match_event_type,
                                 (gpointer) "type_b", NULL);
  dfl_event_sequence_add_walker (sequence, "type_unknown", DFL_ID_INVALID,
                                 walker_not_reached, NULL, NULL);
  dfl_event_sequence_walk (sequence);
  g_object_unref (sequence);
}

/* Test that walking over a short sequence generates callbacks which match by
 * event type and ID. */
static void
test_event_sequence_walk_event_type_and_id (void)
{
  DflEventSequence *sequence = NULL;
  const EventVector vectors[] = {
    { "type_a", 1 },
    { "type_a", 2 },
    { "type_b", 3 },
  };

  sequence = event_sequence_from_vectors (vectors, G_N_ELEMENTS (vectors));
  dfl_event_sequence_add_walker (sequence, "type_a", 1,
                                 walker_match_vector,
                                 (gpointer) &vectors[0], NULL);
  dfl_event_sequence_add_walker (sequence, "type_b", 1,
                                 walker_not_reached, NULL, NULL);
  dfl_event_sequence_add_walker (sequence, "type_unknown", 1,
                                 walker_not_reached, NULL, NULL);
  dfl_event_sequence_walk (sequence);
  g_object_unref (sequence);
}

/* Test that walking over a short sequence, followed by an event which removes
 * the group of walkers, followed by one of the events which was originally
 * matched by the group, only matches the event before the group is removed. */
static void
test_event_sequence_walk_remove_group_then_id_reuse (void)
{
  DflEventSequence *sequence = NULL;
  const EventVector vectors[] = {
    { "type_a", 1 },
    { "type_a", 2 },
    { "type_b", 3 },  /* this should remove the group */
    { "type_a", 1 },  /* this should not be fed to the first walker */
  };
  guint counter = 0;

  sequence = event_sequence_from_vectors (vectors, G_N_ELEMENTS (vectors));
  dfl_event_sequence_start_walker_group (sequence);
  dfl_event_sequence_add_walker (sequence, "type_a", 1,
                                 walker_count,
                                 (gpointer) &counter, NULL);
  dfl_event_sequence_add_walker (sequence, "type_b", 1,
                                 walker_not_reached, NULL, NULL);
  dfl_event_sequence_end_walker_group (sequence, "type_b", 3);
  dfl_event_sequence_walk (sequence);
  g_object_unref (sequence);

  g_assert_cmpuint (counter, ==, 1);
}

static void
test_event_sequence_walk_empty_group (void)
{
  DflEventSequence *sequence = NULL;
  const EventVector vectors[] = {
    { "type_a", 1 },
    { "type_a", 2 },
  };

  sequence = event_sequence_from_vectors (vectors, G_N_ELEMENTS (vectors));
  dfl_event_sequence_start_walker_group (sequence);
  dfl_event_sequence_end_walker_group (sequence, "type_a", 2);
  dfl_event_sequence_walk (sequence);
  g_object_unref (sequence);
}

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/event-sequence/empty", test_event_sequence_empty);
  g_test_add_func ("/event-sequence/single", test_event_sequence_single);
  g_test_add_func ("/event-sequence/walk/empty",
                   test_event_sequence_walk_empty);
  g_test_add_func ("/event-sequence/walk/no-walkers",
                   test_event_sequence_walk_no_walkers);
  g_test_add_func ("/event-sequence/walk/singleton",
                   test_event_sequence_walk_singleton);
  g_test_add_func ("/event-sequence/walk/event-type",
                   test_event_sequence_walk_event_type);
  g_test_add_func ("/event-sequence/walk/event-type-and-id",
                   test_event_sequence_walk_event_type_and_id);
  g_test_add_func ("/event-sequence/walk/remove-group-then-id-reuse",
                   test_event_sequence_walk_remove_group_then_id_reuse);
  g_test_add_func ("/event-sequence/walk/empty-group",
                   test_event_sequence_walk_empty_group);

  return g_test_run ();
}
