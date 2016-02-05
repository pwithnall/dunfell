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
 * SECTION:dfl-parser
 * @short_description: Dunfell log file parser
 * @stability: Unstable
 * @include: libdunfell/dfl-parser.h
 *
 * TODO
 *
 * Since: 0.1.0
 */

#include "config.h"

#include <errno.h>
#include <glib.h>
#include <gio/gio.h>
#include <string.h>

#include "dfl-event.h"
#include "dfl-event-sequence.h"
#include "dfl-parser.h"


static void dfl_parser_dispose (GObject *object);

struct _DflParser
{
  GObject parent;

  DflEventSequence *sequence;  /* owned */
};

G_DEFINE_TYPE (DflParser, dfl_parser, G_TYPE_OBJECT)

static void
dfl_parser_class_init (DflParserClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = dfl_parser_dispose;
}

static void
dfl_parser_init (DflParser *self)
{
  /* Nothing to see here. */
}

static void
dfl_parser_dispose (GObject *object)
{
  DflParser *self = DFL_PARSER (object);

  g_clear_object (&self->sequence);

  /* Chain up to the parent class */
  G_OBJECT_CLASS (dfl_parser_parent_class)->dispose (object);
}

typedef struct
{
  const gchar *event_type;
  guint n_parameters;  /* excluding event type, timestamp and thread ID */
} EventData;

const EventData event_type_array[] =
{
  { "g_main_context_new", 1 },
  { "g_main_context_acquire", 2 },
  { "g_main_context_release", 1 },
  { "g_main_context_free", 1 },
  { "g_main_context_before_dispatch", 1 },
  { "g_main_context_after_dispatch", 1 },
  { "g_source_new", 6 },
  { "g_source_before_free", 3 },
  { "g_source_before_dispatch", 4 },
  { "g_source_after_dispatch", 3 },
  { "g_source_set_name", 2 },
  { "g_source_attach", 3 },
  { "g_source_destroy", 2 },
  { "g_thread_spawned", 3 },
};

static const EventData *
event_data_from_event_type (const gchar *event_type)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (event_type_array); i++)
    {
      if (g_strcmp0 (event_type, event_type_array[i].event_type) == 0)
        return &event_type_array[i];
    }

  return NULL;
}

/**
 * dfl_parser_new:
 *
 * TODO
 *
 * Returns: (transfer full): a new #DflParser
 * Since: 0.1.0
 */
DflParser *
dfl_parser_new (void)
{
  return g_object_new (DFL_TYPE_PARSER, NULL);
}

/**
 * dfl_parser_load_from_data:
 * @self: a #DflParser
 * @data: (array length=length): data to load
 * @length: number of bytes in @data
 * @error: return location for a #GError, or %NULL
 *
 * TODO
 *
 * Since: 0.1.0
 */
void
dfl_parser_load_from_data (DflParser     *self,
                           const guint8  *data,
                           gssize         length,
                           GError       **error)
{
  GInputStream *stream = NULL;

  g_return_if_fail (DFL_IS_PARSER (self));
  g_return_if_fail (data != NULL);
  g_return_if_fail (length > 0);
  g_return_if_fail (error == NULL || *error == NULL);

  /* Load by creating a stream for the data. */
  stream = g_memory_input_stream_new_from_data (data, length, NULL);
  dfl_parser_load_from_stream (self, stream, NULL, error);
  g_object_unref (stream);
}

/**
 * dfl_parser_load_from_file:
 * @self: a #DflParser
 * @filename: path to file to load log from
 * @error: return location for a #GError, or %NULL
 *
 * TODO
 *
 * Since: 0.1.0
 */
void
dfl_parser_load_from_file (DflParser    *self,
                           const gchar  *filename,
                           GError      **error)
{
  GFile *file = NULL;
  GFileInputStream *stream = NULL;

  g_return_if_fail (DFL_IS_PARSER (self));
  g_return_if_fail (filename != NULL);
  g_return_if_fail (error == NULL || *error == NULL);

  /* Load by creating a stream for the file. */
  file = g_file_new_for_path (filename);
  stream = g_file_read (file, NULL, error);
  g_object_unref (file);

  if (stream == NULL)
    return;

  dfl_parser_load_from_stream (self, G_INPUT_STREAM (stream), NULL, error);

  g_object_unref (stream);
}

/**
 * dfl_parser_load_from_stream:
 * @self: a #DflParser
 * @stream: input stream to read log from
 * @cancellable: a #GCancellable, or %NULL
 * @error: return location for a #GError, or %NULL
 *
 * TODO
 *
 * Since: 0.1.0
 */
void
dfl_parser_load_from_stream (DflParser     *self,
                             GInputStream  *stream,
                             GCancellable  *cancellable,
                             GError       **error)
{
  GDataInputStream *data_stream = NULL;
  guint8 *line = NULL;
  gsize length = 0;
  guint line_number;
  guint n_comment_lines;
  guint64 initial_timestamp;
  GHashTable/*<owned guint64, owned guint64>*/ *highest_timestamps = NULL;
  guint file_version;
  GPtrArray/*<owned DflEvent*>*/ *events = NULL;
  GError *child_error = NULL;

  g_return_if_fail (DFL_IS_PARSER (self));
  g_return_if_fail (G_IS_INPUT_STREAM (stream));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (error == NULL || *error == NULL);

  /* Wrap in a data input stream and read line by line. */
  data_stream = g_data_input_stream_new (stream);
  n_comment_lines = 0;
  file_version = 0;
  initial_timestamp = 0;
  highest_timestamps = g_hash_table_new_full (g_int64_hash, g_int64_equal,
                                              g_free, g_free);
  events = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);

  for (line_number = 1,
       line = (guint8 *) g_data_input_stream_read_line (data_stream, &length,
                                                        cancellable,
                                                        &child_error);
       line != NULL;
       line_number++, g_free (line),
       line = (guint8 *) g_data_input_stream_read_line (data_stream, &length,
                                                        cancellable,
                                                        &child_error))
    {
      const gchar *end = NULL;
      gchar **components = NULL;

      /* Note: The line is an arbitrary byte stream. It is not valid UTF-8 and
       * may contain embedded nuls. Validate that first. */
      if (!g_utf8_validate ((gchar *) line, length, &end))
        {
          /* TODO: Use a proper error code here. */
          g_set_error (&child_error, G_IO_ERROR, G_IO_ERROR_UNKNOWN,
                       "Invalid log file line %u — invalid UTF-8 at byte %"
                       G_GOFFSET_FORMAT,
                       line_number, (goffset) (((guint8 *) end) - line));
          break;
        }

      /* Ignore whitespace. */
      line = (guint8 *) g_strstrip ((gchar *) line);

      /* Ignore comment or blank lines. */
      if (line[0] == '\0' || line[0] == '#')
        {
          n_comment_lines++;
          continue;
        }

      g_debug ("%s: Line: %s", G_STRFUNC, line);

      /* Split into components. */
      /* TODO: Formally document log file format. */
      components = g_strsplit ((gchar *) line, ",", -1);

      if (components[0] == NULL)
        {
          /* TODO: Use a proper error code here. */
          g_set_error (&child_error, G_IO_ERROR, G_IO_ERROR_UNKNOWN,
                       "Invalid log file line %u — %s: %s", line_number,
                       "not enough components", line);
          g_strfreev (components);
          break;
        }

      if (g_strcmp0 (components[0], "Dunfell log") == 0)
        {
          const gchar *version, *timestamp;

          /* Header line? Looks like:
           *    Dunfell log,1.0,123456
           * where 1.0 is the log format version, and 123456 is the starting
           * timestamp. */

          /* Is this the first line? */
          if (line_number - n_comment_lines != 1)
            {
              /* TODO: Use a proper error code here. */
              g_set_error (&child_error, G_IO_ERROR, G_IO_ERROR_UNKNOWN,
                           "Invalid log file line %u — %s: %s", line_number,
                           "header must be first non-comment line", line);
              g_strfreev (components);
              break;
            }

          /* Check the number of components. */
          if (components[1] == NULL || components[2] == NULL ||
              components[3] != NULL)
            {
              /* TODO: Use a proper error code here. */
              g_set_error (&child_error, G_IO_ERROR, G_IO_ERROR_UNKNOWN,
                           "Invalid log file line %u — %s: %s", line_number,
                           "header contains the wrong number of components",
                           line);
              g_strfreev (components);
              break;
            }

          /* Extract the components. */
          version = components[1];
          timestamp = components[2];

          /* File version check. */
          if (g_strcmp0 (version, "1.0") != 0)
            {
              /* TODO: Use a proper error code here. */
              g_set_error (&child_error, G_IO_ERROR, G_IO_ERROR_UNKNOWN,
                           "Unsupported log file version ‘%s’ on line %u"
                           "(versions supported: 1.0)", version, line_number);
              g_strfreev (components);
              break;
            }

          file_version = 1;

          /* Parse the timestamp. */
          initial_timestamp = g_ascii_strtoull (timestamp, (gchar **) &end, 10);

          if (errno == ERANGE || end == timestamp || *end != '\0')
            {
              /* TODO: Use a proper error code here. */
              g_set_error (&child_error, G_IO_ERROR, G_IO_ERROR_UNKNOWN,
                           "Invalid timestamp ‘%s’ on line %u", timestamp,
                           line_number);
              g_strfreev (components);
              break;
            }
        }
      else
        {
          const EventData *event_data;
          const gchar *event_type;
          const gchar *timestamp;
          const gchar *tid;
          guint n_components;
          guint64 timestamp_int, tid_int;
          guint64 *highest_timestamp;
          DflEvent *event = NULL;

          /* Non-header line. Looks like:
           *    g_idle_dispatch,1449749875412059,8491,140407983871120,12007776,\
           *    140408421089918,0x7fb36210027e,14614576,0
           */

          /* Has there been a header? */
          if (file_version == 0)
            {
              /* TODO: Use a proper error code here. */
              g_set_error (&child_error, G_IO_ERROR, G_IO_ERROR_UNKNOWN,
                           "Invalid log file line %u — %s: %s", line_number,
                           "header must be first non-comment line", line);
              g_strfreev (components);
              break;
            }

          /* Extract the event type. */
          event_type = g_intern_string (components[0]);

          if (*event_type == '\0')
            {
              /* TODO: Use a proper error code here. */
              g_set_error (&child_error, G_IO_ERROR, G_IO_ERROR_UNKNOWN,
                           "Invalid log file line %u — %s: %s", line_number,
                           "event type not specified", line);
              g_strfreev (components);
              break;
            }

          /* Match it to an event parser. */
          event_data = event_data_from_event_type (event_type);

          if (event_data == NULL)
            {
              /* Ignore unknown event types to allow for more probe points to be
               * added to GLib in future. */
              g_debug ("%s: Ignoring unrecognised event type ‘%s’ on "
                       "line %u: %s", G_STRFUNC, event_type, line_number, line);
              g_strfreev (components);
              continue;
            }

          /* Check the number of components (ignoring the event type, timestamp
           * and thread ID. */
          n_components = g_strv_length (components);

          if (n_components < 3 || n_components - 3 != event_data->n_parameters)
            {
              /* TODO: Use a proper error code here. */
              g_set_error (&child_error, G_IO_ERROR, G_IO_ERROR_UNKNOWN,
                           "Invalid log file line %u — %s: %s", line_number,
                           "event line contains the wrong number of components",
                           line);
              g_strfreev (components);
              break;
            }

          /* Grab the timestamp and thread ID. */
          timestamp = components[1];
          tid = components[2];

          timestamp_int = g_ascii_strtoull (timestamp, (gchar **) &end, 10);

          if (errno == ERANGE || end == timestamp || *end != '\0')
            {
              /* TODO: Use a proper error code here. */
              g_set_error (&child_error, G_IO_ERROR, G_IO_ERROR_UNKNOWN,
                           "Invalid timestamp ‘%s’ on line %u", timestamp,
                           line_number);
              g_strfreev (components);
              break;
            }

          tid_int = g_ascii_strtoull (tid, (gchar **) &end, 10);

          if (errno == ERANGE || end == tid || *end != '\0')
            {
              /* TODO: Use a proper error code here. */
              g_set_error (&child_error, G_IO_ERROR, G_IO_ERROR_UNKNOWN,
                           "Invalid thread ID ‘%s’ on line %u", tid,
                           line_number);
              g_strfreev (components);
              break;
            }

          /* Check that the timestamps in each thread are monotonically
           * increasing. */
          highest_timestamp = g_hash_table_lookup (highest_timestamps,
                                                   (gpointer) &tid_int);

          if ((highest_timestamp == NULL && timestamp_int < initial_timestamp) ||
              (highest_timestamp != NULL && timestamp_int < *highest_timestamp))
            {
              /* TODO: Use a proper error code here. */
              g_set_error (&child_error, G_IO_ERROR, G_IO_ERROR_UNKNOWN,
                           "Invalid timestamp ‘%s’ on line %u: timestamps must "
                           "be monotonically increasing", timestamp,
                           line_number);
              g_strfreev (components);
              break;
            }

          if (highest_timestamp != NULL)
            {
              *highest_timestamp = timestamp_int;
            }
          else
            {
              guint64 *key = NULL;

              highest_timestamp = g_new0 (guint64, 1);
              *highest_timestamp = timestamp_int;
              key = g_new0 (guint64, 1);
              *key = tid_int;

              g_hash_table_insert (highest_timestamps, key, highest_timestamp);
            }

          /* Create the event. */
          event = dfl_event_new (event_type, timestamp_int, tid_int,
                                 (const gchar * const *) components + 3);
          g_ptr_array_add (events, event);  /* transfer ownership */
        }

      g_strfreev (components);
    }

  g_free (line);

  /* Success? */
  if (child_error == NULL)
    {
      g_clear_object (&self->sequence);
      self->sequence = dfl_event_sequence_new ((const DflEvent **) events->pdata,
                                               events->len, initial_timestamp);
    }
  else
    {
      g_propagate_error (error, child_error);
    }

  g_clear_pointer (&events, g_ptr_array_unref);
  g_clear_pointer (&highest_timestamps, g_hash_table_unref);
  g_object_unref (data_stream);
}

static void
load_from_stream_thread_cb (GTask         *task,
                            gpointer       source_object,
                            gpointer       task_data,
                            GCancellable  *cancellable)
{
  DflParser *self;
  GInputStream *stream;
  GError *error = NULL;

  self = DFL_PARSER (source_object);
  stream = G_INPUT_STREAM (task_data);

  dfl_parser_load_from_stream (self, stream, cancellable, &error);

  if (error != NULL)
    g_task_return_error (task, error);
  else
    g_task_return_boolean (task, TRUE);
}

/**
 * dfl_parser_load_from_stream_async:
 * @self: a #DflParser
 * @stream: input stream to read log from
 * @cancellable: a #GCancellable, or  %NULL
 * @callback: callback to call once loading is complete
 * @user_data: data to pass to @callback
 *
 * Asynchronous version of dfl_parser_load_from_stream().
 *
 * Since: 0.1.0
 */
void
dfl_parser_load_from_stream_async (DflParser            *self,
                                   GInputStream         *stream,
                                   GCancellable         *cancellable,
                                   GAsyncReadyCallback   callback,
                                   gpointer              user_data)
{
  GTask *task = NULL;

  g_return_if_fail (DFL_IS_PARSER (self));
  g_return_if_fail (G_IS_INPUT_STREAM (stream));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, dfl_parser_load_from_stream_async);
  g_task_set_task_data (task, g_object_ref (stream), g_object_unref);
  g_task_run_in_thread (task, load_from_stream_thread_cb);
  g_object_unref (task);
}

/**
 * dfl_parser_load_from_stream_finish:
 * @self: a #DflParser
 * @result: result of the asynchronous operation
 * @error: return location for a #GError, or %NULL
 *
 * Finish function for dfl_parser_load_from_stream_async().
 *
 * Since: 0.1.0
 */
void
dfl_parser_load_from_stream_finish (DflParser     *self,
                                    GAsyncResult  *result,
                                    GError       **error)
{
  g_return_if_fail (DFL_IS_PARSER (self));
  g_return_if_fail (G_IS_ASYNC_RESULT (result));
  g_return_if_fail (g_task_is_valid (result, self));
  g_return_if_fail (error == NULL || *error == NULL);

  g_task_propagate_boolean (G_TASK (result), error);
}

/**
 * dfl_parser_get_event_sequence:
 * @self: a #DflParser
 *
 * TODO
 *
 * Returns: (transfer none) (nullable): TODO
 * Since: 0.1.0
 */
DflEventSequence *
dfl_parser_get_event_sequence (DflParser *self)
{
  g_return_val_if_fail (DFL_IS_PARSER (self), NULL);

  return self->sequence;
}

