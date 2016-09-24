/* vim:set et sw=2 cin cino=t0,f0,(0,{s,>2s,n-s,^-s,e2s: */
/*
 * Copyright © Philip Withnall 2016 <philip@tecnocode.co.uk>
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
 * SECTION:task
 * @short_description: model of a #GTask from a log
 * @stability: Unstable
 * @include: libdunfell/task.h
 *
 * TODO
 *
 * Since: UNRELEASED
 */

#include "config.h"

#include <errno.h>
#include <glib.h>
#include <gio/gio.h>
#include <string.h>

#include "event.h"
#include "task.h"
#include "time-sequence.h"


static void dfl_task_get_property (GObject           *object,
                                   guint              property_id,
                                   GValue            *value,
                                   GParamSpec        *pspec);
static void dfl_task_set_property (GObject           *object,
                                   guint              property_id,
                                   const GValue      *value,
                                   GParamSpec        *pspec);
static void dfl_task_dispose (GObject *object);

struct _DflTask
{
  GObject parent;

  DflId id;

  DflTimestamp new_timestamp;
  DflThreadId new_thread_id;
  DflId source_object;
  DflId cancellable;
  gchar *callback_name;  /* owned */
  DflId callback_data;
  gchar *source_tag_name;  /* owned */
  DflTimestamp return_timestamp;
  DflThreadId return_thread_id;
  DflTimestamp propagate_timestamp;
  DflThreadId propagate_thread_id;
  gboolean returned_error;
  DflThreadId run_in_thread_id;
  DflTimestamp before_run_in_thread_timestamp;
  DflTimestamp after_run_in_thread_timestamp;
  gchar *run_in_thread_name;  /* owned */
  gboolean run_in_thread_cancelled;

  /* NOTE: Task data may be set zero or more times, so needs to be implemented
   * as an iteration when it's implemented. */
};

G_DEFINE_TYPE (DflTask, dfl_task, G_TYPE_OBJECT)

typedef enum
{
  PROP_ID = 1,
  PROP_NEW_TIMESTAMP,
  PROP_NEW_THREAD_ID,
  PROP_SOURCE_OBJECT,
  PROP_CANCELLABLE,
  PROP_CALLBACK_NAME,
  PROP_CALLBACK_DATA,
  PROP_SOURCE_TAG_NAME,
  PROP_RETURN_TIMESTAMP,
  PROP_RETURN_THREAD_ID,
  PROP_PROPAGATE_TIMESTAMP,
  PROP_PROPAGATE_THREAD_ID,
  PROP_RETURNED_ERROR,
  PROP_RUN_IN_THREAD_ID,
  PROP_BEFORE_RUN_IN_THREAD_TIMESTAMP,
  PROP_AFTER_RUN_IN_THREAD_TIMESTAMP,
  PROP_RUN_IN_THREAD_NAME,
  PROP_RUN_IN_THREAD_CANCELLED,
  PROP_RUN_DURATION,
  PROP_RUN_IN_THREAD_DURATION,
} DflTaskProperty;

static void
dfl_task_class_init (DflTaskClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = dfl_task_get_property;
  object_class->set_property = dfl_task_set_property;
  object_class->dispose = dfl_task_dispose;

  /**
   * DflTask:id:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_ID,
                                   g_param_spec_ulong ("id",
                                                       "ID",
                                                       "TODO",
                                                       0, G_MAXULONG, 0,
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_STATIC_STRINGS));

  /**
   * DflTask:new-timestamp:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_NEW_TIMESTAMP,
                                   g_param_spec_uint64 ("new-timestamp",
                                                        "New Timestamp",
                                                        "TODO",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflTask:new-thread-id:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_NEW_THREAD_ID,
                                   g_param_spec_uint64 ("new-thread-id",
                                                        "New Thread ID",
                                                        "TODO",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflTask:source-object:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_SOURCE_OBJECT,
                                   g_param_spec_ulong ("source-object",
                                                       "Source Object",
                                                       "TODO",
                                                       0, G_MAXULONG, 0,
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_STATIC_STRINGS));

  /**
   * DflTask:cancellable:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_CANCELLABLE,
                                   g_param_spec_ulong ("cancellable",
                                                       "Cancellable",
                                                       "TODO",
                                                       0, G_MAXULONG, 0,
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_STATIC_STRINGS));

  /**
   * DflTask:callback-name:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_CALLBACK_NAME,
                                   g_param_spec_string ("callback-name",
                                                        "Callback Name",
                                                        "TODO",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflTask:callback-data:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_CALLBACK_DATA,
                                   g_param_spec_ulong ("callback-data",
                                                       "New Timestamp",
                                                       "TODO",
                                                       0, G_MAXULONG, 0,
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_STATIC_STRINGS));

  /**
   * DflTask:source-tag-name:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_SOURCE_TAG_NAME,
                                   g_param_spec_string ("source-tag-name",
                                                        "Source Tag Name",
                                                        "TODO",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflTask:return-timestamp:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_RETURN_TIMESTAMP,
                                   g_param_spec_uint64 ("return-timestamp",
                                                        "Return Timestamp",
                                                        "TODO",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflTask:return-thread-id:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_RETURN_THREAD_ID,
                                   g_param_spec_uint64 ("return-thread-id",
                                                        "Return Thread ID",
                                                        "TODO",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflTask:propagate-timestamp:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_PROPAGATE_TIMESTAMP,
                                   g_param_spec_uint64 ("propagate-timestamp",
                                                        "Propagate Timestamp",
                                                        "TODO",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflTask:propagate-thread-id:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_PROPAGATE_THREAD_ID,
                                   g_param_spec_uint64 ("propagate-thread-id",
                                                        "Propagate Thread ID",
                                                        "TODO",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflTask:returned-error:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_RETURNED_ERROR,
                                   g_param_spec_boolean ("returned-error",
                                                         "Returned Error",
                                                         "TODO",
                                                         FALSE,
                                                         G_PARAM_CONSTRUCT_ONLY |
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_STRINGS));

  /**
   * DflTask:run-in-thread-id:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_RUN_IN_THREAD_ID,
                                   g_param_spec_uint64 ("run-in-thread-id",
                                                        "Run in Thread ID",
                                                        "TODO",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflTask:before-run-in-thread-timestamp:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_BEFORE_RUN_IN_THREAD_TIMESTAMP,
                                   g_param_spec_uint64 ("before-run-in-thread-timestamp",
                                                        "Before Run in Thread Timestamp",
                                                        "TODO",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflTask:after-run-in-thread-timestamp:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_AFTER_RUN_IN_THREAD_TIMESTAMP,
                                   g_param_spec_uint64 ("after-run-in-thread-timestamp",
                                                        "After Run in Thread Timestamp",
                                                        "TODO",
                                                        0, G_MAXUINT64, 0,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflTask:run-in-thread-name:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_RUN_IN_THREAD_NAME,
                                   g_param_spec_string ("run-in-thread-name",
                                                        "Run in Thread Name",
                                                        "TODO",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * DflTask:run-in-thread-cancelled:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_RUN_IN_THREAD_CANCELLED,
                                   g_param_spec_boolean ("run-in-thread-cancelled",
                                                         "Run in Thread Cancelled",
                                                         "TODO",
                                                         FALSE,
                                                         G_PARAM_CONSTRUCT_ONLY |
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_STRINGS));

  /**
   * DflTask:run-duration:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_RUN_DURATION,
                                   g_param_spec_int64 ("run-duration",
                                                       "Run Duration",
                                                       "TODO",
                                                       0, G_MAXINT64, 0,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_STATIC_STRINGS));

  /**
   * DflTask:run-in-thread-duration:
   *
   * TODO
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_RUN_IN_THREAD_DURATION,
                                   g_param_spec_int64 ("run-in-thread-duration",
                                                       "Run in Thread Duration",
                                                       "TODO",
                                                       0, G_MAXINT64, 0,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_STATIC_STRINGS));
}

static void
dfl_task_init (DflTask *self)
{
  /* Nothing here. */
}

static void
dfl_task_get_property (GObject     *object,
                       guint        property_id,
                       GValue      *value,
                       GParamSpec  *pspec)
{
  DflTask *self = DFL_TASK (object);

  switch ((DflTaskProperty) property_id)
    {
    case PROP_ID:
      g_value_set_ulong (value, self->id);
      break;
    case PROP_NEW_TIMESTAMP:
      g_value_set_uint64 (value, self->new_timestamp);
      break;
    case PROP_NEW_THREAD_ID:
      g_value_set_uint64 (value, self->new_thread_id);
      break;
    case PROP_SOURCE_OBJECT:
      g_value_set_ulong (value, self->source_object);
      break;
    case PROP_CANCELLABLE:
      g_value_set_ulong (value, self->cancellable);
      break;
    case PROP_CALLBACK_NAME:
      g_value_set_string (value, self->callback_name);
      break;
    case PROP_CALLBACK_DATA:
      g_value_set_ulong (value, self->callback_data);
      break;
    case PROP_SOURCE_TAG_NAME:
      g_value_set_string (value, self->source_tag_name);
      break;
    case PROP_RETURN_TIMESTAMP:
      g_value_set_uint64 (value, self->return_timestamp);
      break;
    case PROP_RETURN_THREAD_ID:
      g_value_set_uint64 (value, self->return_thread_id);
      break;
    case PROP_PROPAGATE_TIMESTAMP:
      g_value_set_uint64 (value, self->propagate_timestamp);
      break;
    case PROP_PROPAGATE_THREAD_ID:
      g_value_set_uint64 (value, self->propagate_thread_id);
      break;
    case PROP_RETURNED_ERROR:
      g_value_set_boolean (value, self->returned_error);
      break;
    case PROP_RUN_IN_THREAD_ID:
      g_value_set_uint64 (value, self->run_in_thread_id);
      break;
    case PROP_BEFORE_RUN_IN_THREAD_TIMESTAMP:
      g_value_set_uint64 (value, self->before_run_in_thread_timestamp);
      break;
    case PROP_AFTER_RUN_IN_THREAD_TIMESTAMP:
      g_value_set_uint64 (value, self->after_run_in_thread_timestamp);
      break;
    case PROP_RUN_IN_THREAD_NAME:
      g_value_set_string (value, self->run_in_thread_name);
      break;
    case PROP_RUN_IN_THREAD_CANCELLED:
      g_value_set_boolean (value, self->run_in_thread_cancelled);
      break;
    case PROP_RUN_DURATION:
      g_value_set_int64 (value, self->return_timestamp - self->new_timestamp);
      break;
    case PROP_RUN_IN_THREAD_DURATION:
      g_value_set_int64 (value,
                         self->after_run_in_thread_timestamp -
                         self->before_run_in_thread_timestamp);
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
dfl_task_set_property (GObject           *object,
                       guint              property_id,
                       const GValue      *value,
                       GParamSpec        *pspec)
{
  DflTask *self = DFL_TASK (object);

  /* All construct only. */
  switch ((DflTaskProperty) property_id)
    {
    case PROP_ID:
      g_assert (self->id == DFL_ID_INVALID);
      self->id = g_value_get_ulong (value);
      break;
    case PROP_NEW_TIMESTAMP:
      g_assert (self->new_timestamp == 0);
      self->new_timestamp = g_value_get_uint64 (value);
      break;
    case PROP_NEW_THREAD_ID:
      g_assert (self->new_thread_id == 0);
      self->new_thread_id = g_value_get_uint64 (value);
      break;
    case PROP_SOURCE_OBJECT:
      g_assert (self->source_object == 0);
      self->source_object = g_value_get_ulong (value);
      break;
    case PROP_CANCELLABLE:
      g_assert (self->cancellable == 0);
      self->cancellable = g_value_get_ulong (value);
      break;
    case PROP_CALLBACK_NAME:
      g_assert (self->callback_name == NULL);
      self->callback_name = g_value_dup_string (value);
      break;
    case PROP_CALLBACK_DATA:
      g_assert (self->callback_data == 0);
      self->callback_data = g_value_get_ulong (value);
      break;
    case PROP_SOURCE_TAG_NAME:
      g_assert (self->source_tag_name == NULL);
      self->source_tag_name = g_value_dup_string (value);
      break;
    case PROP_RETURN_TIMESTAMP:
      g_assert (self->return_timestamp == 0);
      self->return_timestamp = g_value_get_uint64 (value);
      break;
    case PROP_RETURN_THREAD_ID:
      g_assert (self->return_thread_id == 0);
      self->return_thread_id = g_value_get_uint64 (value);
      break;
    case PROP_PROPAGATE_TIMESTAMP:
      g_assert (self->propagate_timestamp == 0);
      self->propagate_timestamp = g_value_get_uint64 (value);
      break;
    case PROP_PROPAGATE_THREAD_ID:
      g_assert (self->propagate_thread_id == 0);
      self->propagate_thread_id = g_value_get_uint64 (value);
      break;
    case PROP_RETURNED_ERROR:
      self->returned_error = g_value_get_boolean (value);
      break;
    case PROP_RUN_IN_THREAD_ID:
      g_assert (self->run_in_thread_id == 0);
      self->run_in_thread_id = g_value_get_uint64 (value);
      break;
    case PROP_BEFORE_RUN_IN_THREAD_TIMESTAMP:
      g_assert (self->before_run_in_thread_timestamp == 0);
      self->before_run_in_thread_timestamp = g_value_get_uint64 (value);
      break;
    case PROP_AFTER_RUN_IN_THREAD_TIMESTAMP:
      g_assert (self->after_run_in_thread_timestamp == 0);
      self->after_run_in_thread_timestamp = g_value_get_uint64 (value);
      break;
    case PROP_RUN_IN_THREAD_NAME:
      g_assert (self->run_in_thread_name == NULL);
      self->run_in_thread_name = g_value_dup_string (value);
      break;
    case PROP_RUN_IN_THREAD_CANCELLED:
      self->run_in_thread_cancelled = g_value_get_boolean (value);
      break;
    case PROP_RUN_DURATION:
    case PROP_RUN_IN_THREAD_DURATION:
      /* Read only. */
    default:
      g_assert_not_reached ();
    }
}

static void
dfl_task_dispose (GObject *object)
{
  DflTask *self = DFL_TASK (object);

  g_clear_pointer (&self->callback_name, g_free);
  g_clear_pointer (&self->source_tag_name, g_free);
  g_clear_pointer (&self->run_in_thread_name, g_free);

  /* Chain up to the parent class */
  G_OBJECT_CLASS (dfl_task_parent_class)->dispose (object);
}

/**
 * dfl_task_new:
 * @id: TODO
 * @new_timestamp: TODO
 * @new_thread_id: TODO
 *
 * TODO
 *
 * Returns: (transfer full): a new #DflTask
 * Since: UNRELEASED
 */
DflTask *
dfl_task_new (DflId        id,
              DflTimestamp new_timestamp,
              DflThreadId  new_thread_id)
{
  return g_object_new (DFL_TYPE_TASK,
                       "id", id,
                       "new-timestamp", new_timestamp,
                       "new-thread-id", new_thread_id,
                       NULL);
}


#include "event-sequence.h"

static void
task_set_source_tag_cb (DflEventSequence *sequence,
                        DflEvent         *event,
                        gpointer          user_data)
{
  DflTask *task = user_data;

  /* Does this event correspond to the right task? */
  g_assert (dfl_event_get_parameter_id (event, 0) == task->id);

  if (task->source_tag_name != NULL)
    {
      /* TODO: Some better error reporting framework than g_warning(). */
      g_warning ("Saw two g_task_set_source_tag() calls for the same task.");
      g_free (task->source_tag_name);
    }

  task->source_tag_name = g_strdup (dfl_event_get_parameter_utf8 (event, 1));
}

static void
task_before_return_cb (DflEventSequence *sequence,
                       DflEvent         *event,
                       gpointer          user_data)
{
  DflTask *task = user_data;

  /* Does this event correspond to the right task? */
  g_assert (dfl_event_get_parameter_id (event, 0) == task->id);

  if (task->return_timestamp != 0)
    {
      /* TODO: Some better error reporting framework than g_warning(). */
      g_warning ("Saw two g_task_return() calls for the same task.");
    }

  task->return_timestamp = dfl_event_get_timestamp (event);
  task->return_thread_id = dfl_event_get_thread_id (event);
}

static void
task_propagate_cb (DflEventSequence *sequence,
                   DflEvent         *event,
                   gpointer          user_data)
{
  DflTask *task = user_data;
  DflTimestamp timestamp;

  /* Does this event correspond to the right task? */
  g_assert (dfl_event_get_parameter_id (event, 0) == task->id);

  timestamp = dfl_event_get_timestamp (event);

  if (task->propagate_timestamp != 0)
    {
      /* TODO: Some better error reporting framework than g_warning(). */
      g_warning ("Saw two g_task_propagate() calls for the same task.");
    }

  task->propagate_timestamp = timestamp;
  task->returned_error = dfl_event_get_parameter_id (event, 1);
  task->propagate_thread_id = dfl_event_get_thread_id (event);
}

static void
task_before_run_in_thread_cb (DflEventSequence *sequence,
                              DflEvent         *event,
                              gpointer          user_data)
{
  DflTask *task = user_data;

  /* Does this event correspond to the right task? */
  g_assert (dfl_event_get_parameter_id (event, 0) == task->id);

  if (task->before_run_in_thread_timestamp != 0)
    {
      /* TODO: Some better error reporting framework than g_warning(). */
      g_warning ("Saw two g_task_run_in_thread() calls for the same task.");
    }
  else if (task->after_run_in_thread_timestamp != 0)
    {
      g_warning ("Events for g_task_run_in_thread() appeared in the wrong "
                 "order.");
    }

  task->before_run_in_thread_timestamp = dfl_event_get_timestamp (event);
  task->run_in_thread_name = g_strdup (dfl_event_get_parameter_utf8 (event, 1));
  task->run_in_thread_id = dfl_event_get_thread_id (event);
}

static void
task_after_run_in_thread_cb (DflEventSequence *sequence,
                             DflEvent         *event,
                             gpointer          user_data)
{
  DflTask *task = user_data;
  DflTimestamp timestamp;

  /* Does this event correspond to the right task? */
  g_assert (dfl_event_get_parameter_id (event, 0) == task->id);

  timestamp = dfl_event_get_timestamp (event);

  if (task->after_run_in_thread_timestamp != 0)
    {
      /* TODO: Some better error reporting framework than g_warning(). */
      g_warning ("Saw two g_task_run_in_thread() calls for the same task.");
    }
  else if (task->before_run_in_thread_timestamp == 0 ||
           task->before_run_in_thread_timestamp < timestamp)
    {
      g_warning ("Events for g_task_run_in_thread() appeared in the wrong "
                 "order.");
    }

  task->after_run_in_thread_timestamp = timestamp;
  task->run_in_thread_cancelled = dfl_event_get_parameter_id (event, 1);
}

static void
task_new_cb (DflEventSequence *sequence,
             DflEvent         *event,
             gpointer          user_data)
{
  GPtrArray/*<owned DflTask>*/ *tasks = user_data;
  DflTask *task = NULL;
  DflId task_id;

  task_id = dfl_event_get_parameter_id (event, 0);
  task = dfl_task_new (task_id, dfl_event_get_timestamp (event),
                       dfl_event_get_thread_id (event));

  task->source_object = dfl_event_get_parameter_id (event, 1);
  task->cancellable = dfl_event_get_parameter_id (event, 2);
  task->callback_name = g_strdup (dfl_event_get_parameter_utf8 (event, 3));
  task->callback_data = dfl_event_get_parameter_id (event, 4);

  dfl_event_sequence_start_walker_group (sequence);

  dfl_event_sequence_add_walker (sequence, "g_task_set_source_tag",
                                 task_id,
                                 task_set_source_tag_cb,
                                 g_object_ref (task),
                                 (GDestroyNotify) g_object_unref);
  dfl_event_sequence_add_walker (sequence, "g_task_before_return",
                                 task_id,
                                 task_before_return_cb,
                                 g_object_ref (task),
                                 (GDestroyNotify) g_object_unref);
  dfl_event_sequence_add_walker (sequence, "g_task_propagate",
                                 task_id,
                                 task_propagate_cb,
                                 g_object_ref (task),
                                 (GDestroyNotify) g_object_unref);
  dfl_event_sequence_add_walker (sequence, "g_task_before_run_in_thread",
                                 task_id,
                                 task_before_run_in_thread_cb,
                                 g_object_ref (task),
                                 (GDestroyNotify) g_object_unref);
  dfl_event_sequence_add_walker (sequence, "g_task_after_run_in_thread",
                                 task_id,
                                 task_after_run_in_thread_cb,
                                 g_object_ref (task),
                                 (GDestroyNotify) g_object_unref);

  dfl_event_sequence_end_walker_group (sequence, "g_task_propagate", task_id);

  g_ptr_array_add (tasks, task);  /* transfer */
}

/**
 * dfl_task_factory_from_event_sequence:
 * @sequence: an event sequence to analyse
 *
 * TODO
 *
 * Returns: (transfer full) (element-type DflTask): an array of zero or
 *    more #DflTasks
 * Since: UNRELEASED
 */
GPtrArray *
dfl_task_factory_from_event_sequence (DflEventSequence *sequence)
{
  GPtrArray/*<owned DflTask>*/ *tasks = NULL;

  tasks = g_ptr_array_new_with_free_func (g_object_unref);
  dfl_event_sequence_add_walker (sequence, "g_task_new", DFL_ID_INVALID,
                                 task_new_cb,
                                 g_ptr_array_ref (tasks),
                                 (GDestroyNotify) g_ptr_array_unref);

  return tasks;
}

/**
 * dfl_task_get_id:
 * @self: a #DflTask
 *
 * TODO
 *
 * Returns: TODO
 * Since: UNRELEASED
 */
DflId
dfl_task_get_id (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), DFL_ID_INVALID);

  return self->id;
}

/**
 * dfl_task_get_new_timestamp:
 * @self: a #DflTask
 *
 * TODO
 *
 * Returns: TODO
 * Since: UNRELEASED
 */
DflTimestamp
dfl_task_get_new_timestamp (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), 0);

  return self->new_timestamp;
}

/* TODO */
DflThreadId
dfl_task_get_new_thread_id (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), DFL_ID_INVALID);

  return self->new_thread_id;
}

/* TODO */
DflTimestamp
dfl_task_get_return_timestamp (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), 0);

  return self->return_timestamp;
}

/* TODO */
DflThreadId
dfl_task_get_return_thread_id (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), DFL_ID_INVALID);

  return self->return_thread_id;
}

/* TODO */
DflTimestamp
dfl_task_get_propagate_timestamp (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), 0);

  return self->propagate_timestamp;
}

/* TODO */
DflThreadId
dfl_task_get_propagate_thread_id (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), DFL_ID_INVALID);

  return self->propagate_thread_id;
}

/* TODO */
gboolean
dfl_task_get_is_thread_cancelled (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), FALSE);

  return self->run_in_thread_cancelled;
}

/* TODO */
DflTimestamp
dfl_task_get_thread_before_timestamp (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), 0);

  return self->before_run_in_thread_timestamp;
}

/* TODO */
DflTimestamp
dfl_task_get_thread_after_timestamp (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), 0);

  return self->after_run_in_thread_timestamp;
}

/* TODO */
DflThreadId
dfl_task_get_thread_id (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), DFL_ID_INVALID);

  return self->run_in_thread_id;
}

/* TODO */
const gchar *
dfl_task_get_callback_name (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), NULL);

  return self->callback_name;
}

/* TODO */
const gchar *
dfl_task_get_source_tag_name (DflTask *self)
{
  g_return_val_if_fail (DFL_IS_TASK (self), NULL);

  return self->source_tag_name;
}
