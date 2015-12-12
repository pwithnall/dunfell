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
#include <string.h>

#include "dfl-parser.h"


typedef struct
{
  guint n_events_expected;
  const gchar *log;
} LogTestVector;

/* Test the properties of a parser which has been used for nothing. */
static void
test_parser_construction (void)
{
  DflParser *parser = NULL;

  parser = dfl_parser_new ();
  g_assert_null (dfl_parser_get_event_sequence (parser));
  g_object_unref (parser);
}

/* Test that the parser correctly loads and parses various test log files. */
static void
test_parser_log (gconstpointer data)
{
  const LogTestVector *vector = data;
  DflParser *parser = NULL;
  DflEventSequence *sequence;
  GError *error = NULL;

  parser = dfl_parser_new ();

  dfl_parser_load_from_data (parser, (const guint8 *) vector->log,
                             strlen (vector->log), &error);
  g_assert_no_error (error);

  sequence = dfl_parser_get_event_sequence (parser);
  g_assert_nonnull (sequence);
  g_assert (DFL_IS_EVENT_SEQUENCE (sequence));
  g_assert_cmpuint (g_list_model_get_n_items (G_LIST_MODEL (sequence)), ==,
                    vector->n_events_expected);

  g_object_unref (parser);
}

int
main (int argc, char *argv[])
{
  guint i;
  const LogTestVector test_vectors[] = {
    /* Number of events expected, Log file */
    { 0, "Dunfell log,1.0,123" },
    { 0, "Dunfell log,1.0,123\n"},
    { 0, "Dunfell log,1.0,123\n# Comment"},
    { 0, "# Comment\nDunfell log,1.0,123\n"},
    { 0, "   \n\t\n#\nDunfell log,1.0,123\n  # Some more comments\n"},
    { 1,
      "Dunfell log,1.0,123\n"
      "g_main_context_acquire,124,1,0,0\n" },
    { 1,
      "Dunfell log,1.0,123\n"
      "g_main_context_acquire,124,1,0,0" },
    { 2,
      "Dunfell log,1.0,123\n"
      "g_main_context_acquire,123,1,0,0\n"
      "g_main_context_acquire,123,1,0,0\n" },
    { 2,
      "Dunfell log,1.0,123\n"
      "g_main_context_acquire,124,1,0,0\n"
      "g_main_context_acquire,125,1,0,0\n" },
    { 2,
      "Dunfell log,1.0,123\n"
      "\n"
      "# Comment\n"
      "g_main_context_acquire,124,1,0,0\n"
      "# Another comment\n"
      "g_main_context_acquire,125,1,0,0\n" },
    { 1,
      "Dunfell log,1.0,123\n"
      "g_main_context_acquire,124,1,0,0\n"
      "nonexistent_event,125\n" },
  };

  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/parser/construction", test_parser_construction);

  for (i = 0; i < G_N_ELEMENTS (test_vectors); i++)
    {
      gchar *test_name;

      test_name = g_strdup_printf ("/parser/log/%u", i);
      g_test_add_data_func (test_name, &test_vectors[i], test_parser_log);
      g_free (test_name);
    }

  return g_test_run ();
}
