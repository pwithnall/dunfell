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

/**
 * SECTION:task-model
 * @short_description: #GtkTreeModel for accessing #DflTasks
 * @stability: Unstable
 * @include: libdunfell-ui/task-model.h
 *
 * #DwlTaskModel is a #GtkTreeModel which exposes a list of #DflTasks,
 * representing #GTasks.
 *
 * Since: UNRELEASED
 */

#include "config.h"

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libdunfell/task.h>
#include <libdunfell/types.h>

#include "libdunfell-ui/task-model.h"


static void dwl_task_model_tree_model_init (GtkTreeModelIface *iface);
static void dwl_task_model_get_property (GObject      *object,
                                         guint         property_id,
                                         GValue       *value,
                                         GParamSpec   *pspec);
static void dwl_task_model_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec);
static void dwl_task_model_dispose      (GObject      *object);

static GtkTreeModelFlags dwl_task_model_get_flags        (GtkTreeModel *tree_model);
static gint              dwl_task_model_get_n_columns    (GtkTreeModel *tree_model);
static GType             dwl_task_model_get_column_type  (GtkTreeModel *tree_model,
                                                          gint          index_);
static gboolean          dwl_task_model_get_iter         (GtkTreeModel *tree_model,
                                                          GtkTreeIter  *iter,
                                                          GtkTreePath  *path);
static GtkTreePath      *dwl_task_model_get_path         (GtkTreeModel *tree_model,
                                                          GtkTreeIter  *iter);
static void              dwl_task_model_get_value        (GtkTreeModel *tree_model,
                                                          GtkTreeIter  *iter,
                                                          gint          column,
                                                          GValue       *value);
static gboolean          dwl_task_model_iter_nth_sibling (GtkTreeModel *tree_model,
                                                          GtkTreeIter  *iter,
                                                          gint          n);
static gboolean          dwl_task_model_iter_next        (GtkTreeModel *tree_model,
                                                          GtkTreeIter  *iter);
static gboolean          dwl_task_model_iter_previous    (GtkTreeModel *tree_model,
                                                          GtkTreeIter  *iter);
static gboolean          dwl_task_model_iter_children    (GtkTreeModel *tree_model,
                                                          GtkTreeIter  *iter,
                                                          GtkTreeIter  *parent);
static gboolean          dwl_task_model_iter_has_child   (GtkTreeModel *tree_model,
                                                          GtkTreeIter  *iter);
static gint              dwl_task_model_iter_n_children  (GtkTreeModel *tree_model,
                                                          GtkTreeIter  *iter);
static gboolean          dwl_task_model_iter_nth_child   (GtkTreeModel *tree_model,
                                                          GtkTreeIter  *iter,
                                                          GtkTreeIter  *parent,
                                                          gint          n);
static gboolean          dwl_task_model_iter_parent      (GtkTreeModel *tree_model,
                                                          GtkTreeIter  *iter,
                                                          GtkTreeIter  *child);

struct _DwlTaskModel
{
  GObject parent;

  GPtrArray *tasks;  /* (element-type DflTask) (owned) */
};

static const struct {
  GType type;
  const gchar *property_name;
} dwl_task_model_columns[] = {
  { DFL_TYPE_ID, "id" },
  { DFL_TYPE_TIMESTAMP, "new-timestamp" },
  { DFL_TYPE_THREAD_ID, "new-thread-id" },
  { DFL_TYPE_ID, "source-object" },
  { DFL_TYPE_ID, "cancellable" },
  { G_TYPE_STRING, "callback-name" },
  { DFL_TYPE_ID, "callback-data" },
  { G_TYPE_STRING, "source-tag-name" },
  { DFL_TYPE_TIMESTAMP, "return-timestamp" },
  { DFL_TYPE_THREAD_ID, "return-thread-id" },
  { DFL_TYPE_TIMESTAMP, "propagate-timestamp" },
  { DFL_TYPE_THREAD_ID, "propagate-thread-id" },
  { G_TYPE_BOOLEAN, "returned-error" },
  { DFL_TYPE_THREAD_ID, "run-in-thread-id" },
  { DFL_TYPE_TIMESTAMP, "before-run-in-thread-timestamp" },
  { DFL_TYPE_TIMESTAMP, "after-run-in-thread-timestamp" },
  { G_TYPE_STRING, "run-in-thread-name" },
  { G_TYPE_BOOLEAN, "run-in-thread-cancelled" },
  { DFL_TYPE_DURATION, "run-duration" },
  { DFL_TYPE_DURATION, "run-in-thread-duration" },
};

G_DEFINE_TYPE_WITH_CODE (DwlTaskModel, dwl_task_model, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
                                                dwl_task_model_tree_model_init))

typedef enum
{
  PROP_TASKS = 1,
} DwlTaskModelProperty;

static void
dwl_task_model_class_init (DwlTaskModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = dwl_task_model_get_property;
  object_class->set_property = dwl_task_model_set_property;
  object_class->dispose = dwl_task_model_dispose;

  /**
   * DwlTaskModel:tasks:
   *
   * Top-level array containing the #DflTasks.
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_TASKS,
                                   g_param_spec_boxed ("tasks",
                                                       "Tasks",
                                                       "Top-level array "
                                                       "containing the "
                                                       "DflTasks.",
                                                       G_TYPE_PTR_ARRAY,
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_STATIC_STRINGS));
}

static void
dwl_task_model_tree_model_init (GtkTreeModelIface *iface)
{
  iface->get_flags = dwl_task_model_get_flags;
  iface->get_n_columns = dwl_task_model_get_n_columns;
  iface->get_column_type = dwl_task_model_get_column_type;
  iface->get_iter = dwl_task_model_get_iter;
  iface->get_path = dwl_task_model_get_path;
  iface->get_value = dwl_task_model_get_value;
  iface->iter_next = dwl_task_model_iter_next;
  iface->iter_previous = dwl_task_model_iter_previous;
  iface->iter_children = dwl_task_model_iter_children;
  iface->iter_has_child = dwl_task_model_iter_has_child;
  iface->iter_n_children = dwl_task_model_iter_n_children;
  iface->iter_nth_child = dwl_task_model_iter_nth_child;
  iface->iter_parent = dwl_task_model_iter_parent;
}

static void
dwl_task_model_init (DwlTaskModel *self)
{
  /* Nothing to do here. */
}

static void
dwl_task_model_get_property (GObject     *object,
                             guint        property_id,
                             GValue      *value,
                             GParamSpec  *pspec)
{
  DwlTaskModel *self = DWL_TASK_MODEL (object);

  switch ((DwlTaskModelProperty) property_id)
    {
    case PROP_TASKS:
      g_value_set_boxed (value, self->tasks);
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
dwl_task_model_set_property (GObject           *object,
                             guint              property_id,
                             const GValue      *value,
                             GParamSpec        *pspec)
{
  DwlTaskModel *self = DWL_TASK_MODEL (object);

  switch ((DwlTaskModelProperty) property_id)
    {
    case PROP_TASKS:
      /* Construct-only. */
      g_assert (self->tasks == NULL);
      self->tasks = g_value_dup_boxed (value);
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
dwl_task_model_dispose (GObject *object)
{
  DwlTaskModel *self = DWL_TASK_MODEL (object);

  g_clear_pointer (&self->tasks, g_ptr_array_unref);

  G_OBJECT_CLASS (dwl_task_model_parent_class)->dispose (object);
}

static GtkTreeModelFlags
dwl_task_model_get_flags (GtkTreeModel *tree_model)
{
  /* The model is immutable. */
  return GTK_TREE_MODEL_ITERS_PERSIST;
}

static gint
dwl_task_model_get_n_columns (GtkTreeModel *tree_model)
{
  /* How many properties does #DflTask have? */
  return G_N_ELEMENTS (dwl_task_model_columns);
}

static GType
dwl_task_model_get_column_type (GtkTreeModel *tree_model,
                                gint          index_)
{
  g_assert (index_ >= 0 &&
            (guint) index_ < G_N_ELEMENTS (dwl_task_model_columns));

  return dwl_task_model_columns[index_].type;
}

static gboolean
dwl_task_model_get_iter (GtkTreeModel *tree_model,
                         GtkTreeIter  *iter,
                         GtkTreePath  *path)
{
  DwlTaskModel *self = DWL_TASK_MODEL (tree_model);
  const gint *indices;
  gint depth;

  indices = gtk_tree_path_get_indices_with_depth (path, &depth);

  if (depth != 1)
    {
      iter->stamp = 0;  /* invalid */
      return FALSE;
    }

  /* Store the index and task in the iter. */
  iter->stamp = 1;  /* valid */
  iter->user_data = GINT_TO_POINTER (indices[0]);
  iter->user_data2 = DFL_TASK (self->tasks->pdata[indices[0]]);

  return TRUE;
}

static GtkTreePath *
dwl_task_model_get_path (GtkTreeModel *tree_model,
                         GtkTreeIter  *iter)
{
  /* Is the iter valid? */
  if (iter->stamp < 1)
    return gtk_tree_path_new ();

  return gtk_tree_path_new_from_indices (GPOINTER_TO_INT (iter->user_data), -1);
}

static void
dwl_task_model_get_value (GtkTreeModel *tree_model,
                          GtkTreeIter  *iter,
                          gint          column,
                          GValue       *value)
{
  DflTask *task;

  task = DFL_TASK (iter->user_data2);
  g_assert (DFL_IS_TASK (task));
  g_assert (column >= 0 &&
            (guint) column < G_N_ELEMENTS (dwl_task_model_columns));

  g_value_init (value, dwl_task_model_columns[column].type);
  g_object_get_property (G_OBJECT (task),
                         dwl_task_model_columns[column].property_name, value);
}

static gboolean
dwl_task_model_iter_nth_sibling (GtkTreeModel *tree_model,
                                 GtkTreeIter  *iter,
                                 gint          n)
{
  DwlTaskModel *self = DWL_TASK_MODEL (tree_model);
  gint index;

  /* Invalid? */
  if (iter->stamp < 1)
    return FALSE;

  index = GPOINTER_TO_INT (iter->user_data);

  if ((n >= 0 && index <= G_MAXINT - n &&
       (guint) (index + n) < self->tasks->len) ||
      (n < 0 && index >= -n))
    {
      index += n;
      iter->user_data = GINT_TO_POINTER (index);
      iter->user_data2 = self->tasks->pdata[index];
      return TRUE;
    }
  else
    {
      /* Invalidate @iter. */
      iter->stamp = 0;
      iter->user_data = NULL;
      iter->user_data2 = NULL;

      return FALSE;
    }
}

static gboolean
dwl_task_model_iter_next (GtkTreeModel *tree_model,
                          GtkTreeIter  *iter)
{
  return dwl_task_model_iter_nth_sibling (tree_model, iter, 1);
}

static gboolean
dwl_task_model_iter_previous (GtkTreeModel *tree_model,
                              GtkTreeIter  *iter)
{
  return dwl_task_model_iter_nth_sibling (tree_model, iter, -1);
}

static gboolean
dwl_task_model_iter_children (GtkTreeModel *tree_model,
                              GtkTreeIter  *iter,
                              GtkTreeIter  *parent)
{
  return dwl_task_model_iter_nth_child (tree_model, iter, parent, 0);
}

static gboolean
dwl_task_model_iter_has_child (GtkTreeModel *tree_model,
                               GtkTreeIter  *iter)
{
  return (dwl_task_model_iter_n_children (tree_model, iter) > 0);
}

static gint
dwl_task_model_iter_n_children (GtkTreeModel *tree_model,
                                GtkTreeIter  *iter)
{
  DwlTaskModel *self = DWL_TASK_MODEL (tree_model);

  /* Top-level case. */
  return (iter == NULL) ? self->tasks->len : 0;
}

static gboolean
dwl_task_model_iter_nth_child (GtkTreeModel *tree_model,
                               GtkTreeIter  *iter,
                               GtkTreeIter  *parent,
                               gint          n)
{
  DwlTaskModel *self = DWL_TASK_MODEL (tree_model);

  /* Return the first node in the tree? */
  if (parent == NULL && n >= 0 && self->tasks->len > (guint) n)
    {
      iter->stamp = 1;  /* valid */
      iter->user_data = GINT_TO_POINTER (n);
      iter->user_data2 = self->tasks->pdata[n];

      return TRUE;
    }
  else
    {
      iter->stamp = 0;  /* invalid */
      return FALSE;
    }
}

static gboolean
dwl_task_model_iter_parent (GtkTreeModel *tree_model,
                            GtkTreeIter  *iter,
                            GtkTreeIter  *child)
{
  iter->stamp = 0;  /* invalid */
  return FALSE;
}

/**
 * dwl_task_model_new:
 * @tasks: (transfer none) (element-type DflTask): array of tasks to
 *    expose in the model
 *
 * Create a new #DwlTaskModel to expose the given #DflTasks as a list for
 * use in the UI.
 *
 * Returns: (transfer full): a new #DwlTaskModel
 * Since: UNRELEASED
 */
DwlTaskModel *
dwl_task_model_new (GPtrArray *tasks)
{
  g_return_val_if_fail (tasks != NULL, NULL);

  return g_object_new (DWL_TYPE_TASK_MODEL,
                       "tasks", tasks,
                       NULL);
}
