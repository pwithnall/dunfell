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

#include "dfl-main-context.h"
#include "dfl-parser.h"


/* Test the properties of newly constructed #DflMainContext. */
static void
test_main_context_construction (void)
{
  DflMainContext *main_context = NULL;

  main_context = dfl_main_context_new (1, 123);
  g_object_unref (main_context);
}

static GPtrArray/*<owned DflMainContext>*/ *
parser_helper (const gchar *log)
{
  DflParser *parser = NULL;
  DflEventSequence *sequence;
  GPtrArray/*<owned DflMainContext>*/ *main_contexts = NULL;
  GError *error = NULL;

  /* Parse the log into an event sequence. */
  parser = dfl_parser_new ();

  dfl_parser_load_from_data (parser, (const guint8 *) log, strlen (log),
                             &error);
  g_assert_no_error (error);

  sequence = dfl_parser_get_event_sequence (parser);
  g_assert_nonnull (sequence);
  g_assert (DFL_IS_EVENT_SEQUENCE (sequence));

  /* Analyse the event sequence. */
  main_contexts = dfl_main_context_factory_from_event_sequence (sequence);
  dfl_event_sequence_walk (sequence);

  g_object_unref (parser);

  return main_contexts;  /* transfer */
}

/* Test that a log file with no main context related entries is correctly
 * turned into zero #DflMainContexts. */
static void
test_main_context_parse_log_empty (void)
{
  GPtrArray/*<owned DflMainContext>*/ *main_contexts = NULL;

  main_contexts = parser_helper ("Dunfell log,1.0,1\n");
  g_assert_cmpuint (main_contexts->len, ==, 0);
  g_ptr_array_unref (main_contexts);
}

/* Test that a log file with entries for a single context in a single thread is
 * correctly turned into one #DflMainContext. */
static void
test_main_context_parse_log_single_context_single_thread (void)
{
  GPtrArray/*<owned DflMainContext>*/ *main_contexts = NULL;
  DflMainContext *context;

  /* Timestamps: 1+; thread ID: 1000; context ID: 666 */
  main_contexts = parser_helper (
    "Dunfell log,1.0,1\n"
    "g_main_context_new,1,1000,666\n"
    "g_main_context_acquire,1,1000,666,1\n"
    "g_main_context_release,10,1000,666\n"
    "g_main_context_acquire,11,1000,666,1\n"
    "g_main_context_release,13,1000,666\n"
    "g_main_context_free,17,1000,666\n");

  g_assert_cmpuint (main_contexts->len, ==, 1);
  context = main_contexts->pdata[0];
  g_assert_cmpint (dfl_main_context_get_id (context), ==, 666);
  g_assert_cmpuint (dfl_main_context_get_new_timestamp (context), ==, 1);
  g_assert_cmpuint (dfl_main_context_get_free_timestamp (context), ==, 17);

  g_ptr_array_unref (main_contexts);
}

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/main-context/construction",
                   test_main_context_construction);
  g_test_add_func ("/main-context/parse-log/empty",
                   test_main_context_parse_log_empty);
  g_test_add_func ("/main-context/parse-log/single-context-single-thread",
                   test_main_context_parse_log_single_context_single_thread);

  return g_test_run ();
}
