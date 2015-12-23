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

#include "config.h"

#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <locale.h>
#include <string.h>

#include "dfl-parser.h"
#include "dfl-event-sequence.h"
#include "dfl-main-context.h"
#include "dfl-thread.h"
#include "dwl-timeline.h"


int
main (int argc, char *argv[])
{
  GtkWindow *window;
  GtkWidget *scrolled, *timeline;
  DflParser *parser = NULL;
  DflEventSequence *sequence;
  GPtrArray/*<owned DflMainContext>*/ *main_contexts = NULL;
  GPtrArray/*<owned DflMainContext>*/ *threads = NULL;
  GError *error = NULL;
  gchar *log = NULL;

  setlocale (LC_ALL, "");

  gtk_init (&argc, &argv);

  /* Parse a log. */
  g_file_get_contents ("/tmp/dunfell.log", &log, NULL, &error);
  g_assert_no_error (error);

  /* Parse the log into an event sequence. */
  parser = dfl_parser_new ();

  dfl_parser_load_from_data (parser, (guint8 *) log, strlen (log), &error);
  g_assert_no_error (error);

  g_free (log);

  sequence = dfl_parser_get_event_sequence (parser);
  g_assert_nonnull (sequence);
  g_assert (DFL_IS_EVENT_SEQUENCE (sequence));

  /* Analyse the event sequence. */
  main_contexts = dfl_main_context_factory_from_event_sequence (sequence);
  threads = dfl_thread_factory_from_event_sequence (sequence);
  dfl_event_sequence_walk (sequence);

  g_object_unref (parser);

  /* Build the UI. */
  window = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
  gtk_window_set_resizable (window, TRUE);
  gtk_window_set_default_size (window, 500, 500);

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (window), scrolled);

  timeline = GTK_WIDGET (dwl_timeline_new (threads, main_contexts));
  gtk_container_add (GTK_CONTAINER (scrolled), timeline);

  gtk_widget_show_all (GTK_WIDGET (window));

  gtk_main ();

  return 0;
}
