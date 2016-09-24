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
 * SECTION:source-model
 * @short_description: #GtkTreeModel for accessing #DflSources
 * @stability: Unstable
 * @include: libdunfell-ui/source-model.h
 *
 * #DwlSourceModel is a #GtkTreeModel which exposes a tree of #DflSources,
 * representing #GSources and their children (as added using
 * g_source_add_child_source()).
 *
 * Since: UNRELEASED
 */

#include "config.h"

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libdunfell/source.h>
#include <libdunfell/types.h>

#include "libdunfell-ui/source-model.h"


static void dwl_source_model_tree_model_init (GtkTreeModelIface *iface);
static void dwl_source_model_get_property (GObject      *object,
                                           guint         property_id,
                                           GValue       *value,
                                           GParamSpec   *pspec);
static void dwl_source_model_set_property (GObject      *object,
                                           guint         property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec);
static void dwl_source_model_dispose      (GObject      *object);

static GtkTreeModelFlags dwl_source_model_get_flags        (GtkTreeModel *tree_model);
static gint              dwl_source_model_get_n_columns    (GtkTreeModel *tree_model);
static GType             dwl_source_model_get_column_type  (GtkTreeModel *tree_model,
                                                            gint          index_);
static gboolean          dwl_source_model_get_iter         (GtkTreeModel *tree_model,
                                                            GtkTreeIter  *iter,
                                                            GtkTreePath  *path);
static GtkTreePath      *dwl_source_model_get_path         (GtkTreeModel *tree_model,
                                                            GtkTreeIter  *iter);
static void              dwl_source_model_get_value        (GtkTreeModel *tree_model,
                                                            GtkTreeIter  *iter,
                                                            gint          column,
                                                            GValue       *value);
static gboolean          dwl_source_model_iter_nth_sibling (GtkTreeModel *tree_model,
                                                            GtkTreeIter  *iter,
                                                            gint          n);
static gboolean          dwl_source_model_iter_next        (GtkTreeModel *tree_model,
                                                            GtkTreeIter  *iter);
static gboolean          dwl_source_model_iter_previous    (GtkTreeModel *tree_model,
                                                            GtkTreeIter  *iter);
static gboolean          dwl_source_model_iter_children    (GtkTreeModel *tree_model,
                                                            GtkTreeIter  *iter,
                                                            GtkTreeIter  *parent);
static gboolean          dwl_source_model_iter_has_child   (GtkTreeModel *tree_model,
                                                            GtkTreeIter  *iter);
static gint              dwl_source_model_iter_n_children  (GtkTreeModel *tree_model,
                                                            GtkTreeIter  *iter);
static gboolean          dwl_source_model_iter_nth_child   (GtkTreeModel *tree_model,
                                                            GtkTreeIter  *iter,
                                                            GtkTreeIter  *parent,
                                                            gint          n);
static gboolean          dwl_source_model_iter_parent      (GtkTreeModel *tree_model,
                                                            GtkTreeIter  *iter,
                                                            GtkTreeIter  *child);
static void              dwl_source_model_ref_node         (GtkTreeModel *tree_model,
                                                            GtkTreeIter  *iter);
static void              dwl_source_model_unref_node       (GtkTreeModel *tree_model,
                                                            GtkTreeIter  *iter);

struct _DwlSourceModel
{
  GObject parent;

  GPtrArray *sources;  /* (element-type DflSource) (owned) */
};

static const struct {
  GType type;
  const gchar *property_name;
} dwl_source_model_columns[] = {
  { DFL_TYPE_ID, "id" },
  { DFL_TYPE_TIMESTAMP, "new-timestamp" },
  { DFL_TYPE_TIMESTAMP, "free-timestamp" },
  { DFL_TYPE_THREAD_ID, "new-thread-id" },
  { G_TYPE_STRING, "name" },
  { DFL_TYPE_ID, "attach-context" },
  { DFL_TYPE_TIMESTAMP, "attach-timestamp" },
  { DFL_TYPE_THREAD_ID, "attach-thread-id" },
  { DFL_TYPE_TIMESTAMP, "destroy-timestamp" },
  { DFL_TYPE_THREAD_ID, "destroy-thread-id" },
  { G_TYPE_INT, "min-priority" },
  { G_TYPE_INT, "max-priority" },
  { G_TYPE_ULONG, "n-dispatches" },
  { G_TYPE_INT64, "min-dispatch-duration" },
  { G_TYPE_INT64, "median-dispatch-duration" },
  { G_TYPE_INT64, "max-dispatch-duration" },
};

G_DEFINE_TYPE_WITH_CODE (DwlSourceModel, dwl_source_model, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
                                                dwl_source_model_tree_model_init))

typedef enum
{
  PROP_SOURCES = 1,
} DwlSourceModelProperty;

static void
dwl_source_model_class_init (DwlSourceModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = dwl_source_model_get_property;
  object_class->set_property = dwl_source_model_set_property;
  object_class->dispose = dwl_source_model_dispose;

  /**
   * DwlSourceModel:sources:
   *
   * Top-level array containing the unparented #DflSources.
   *
   * Since: UNRELEASED
   */
  g_object_class_install_property (object_class, PROP_SOURCES,
                                   g_param_spec_boxed ("sources",
                                                       "Sources",
                                                       "Top-level array "
                                                       "containing the "
                                                       "unparented DflSources.",
                                                       G_TYPE_PTR_ARRAY,
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_STATIC_STRINGS));
}

static void
dwl_source_model_tree_model_init (GtkTreeModelIface *iface)
{
  iface->get_flags = dwl_source_model_get_flags;
  iface->get_n_columns = dwl_source_model_get_n_columns;
  iface->get_column_type = dwl_source_model_get_column_type;
  iface->get_iter = dwl_source_model_get_iter;
  iface->get_path = dwl_source_model_get_path;
  iface->get_value = dwl_source_model_get_value;
  iface->iter_next = dwl_source_model_iter_next;
  iface->iter_previous = dwl_source_model_iter_previous;
  iface->iter_children = dwl_source_model_iter_children;
  iface->iter_has_child = dwl_source_model_iter_has_child;
  iface->iter_n_children = dwl_source_model_iter_n_children;
  iface->iter_nth_child = dwl_source_model_iter_nth_child;
  iface->iter_parent = dwl_source_model_iter_parent;
  iface->ref_node = dwl_source_model_ref_node;
  iface->unref_node = dwl_source_model_unref_node;
}

static void
dwl_source_model_init (DwlSourceModel *self)
{
  /* Nothing to do here. */
}

static void
dwl_source_model_get_property (GObject     *object,
                               guint        property_id,
                               GValue      *value,
                               GParamSpec  *pspec)
{
  DwlSourceModel *self = DWL_SOURCE_MODEL (object);

  switch ((DwlSourceModelProperty) property_id)
    {
    case PROP_SOURCES:
      g_value_set_boxed (value, self->sources);
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
dwl_source_model_set_property (GObject           *object,
                               guint              property_id,
                               const GValue      *value,
                               GParamSpec        *pspec)
{
  DwlSourceModel *self = DWL_SOURCE_MODEL (object);

  switch ((DwlSourceModelProperty) property_id)
    {
    case PROP_SOURCES:
      /* Construct-only. */
      g_assert (self->sources == NULL);
      self->sources = g_value_dup_boxed (value);
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
dwl_source_model_dispose (GObject *object)
{
  DwlSourceModel *self = DWL_SOURCE_MODEL (object);

  g_clear_pointer (&self->sources, g_ptr_array_unref);

  G_OBJECT_CLASS (dwl_source_model_parent_class)->dispose (object);
}

static GtkTreeModelFlags
dwl_source_model_get_flags (GtkTreeModel *tree_model)
{
  /* The model is immutable. */
  return GTK_TREE_MODEL_ITERS_PERSIST;
}

static gint
dwl_source_model_get_n_columns (GtkTreeModel *tree_model)
{
  /* How many properties does #DflSource have? */
  return G_N_ELEMENTS (dwl_source_model_columns);
}

static GType
dwl_source_model_get_column_type (GtkTreeModel *tree_model,
                                  gint          index_)
{
  g_assert (index_ >= 0 &&
            (guint) index_ < G_N_ELEMENTS (dwl_source_model_columns));

  return dwl_source_model_columns[index_].type;
}

static DflSource *
get_source (GPtrArray  *sources,
            const gint *indices,
            gint        depth)
{
  DflSource *source;
  gsize i;

  g_assert (depth >= 0);

  for (i = 0; i < (guint) depth; i++)
    {
      g_assert (indices[i] >= 0 && (guint) indices[i] < sources->len);

      source = sources->pdata[indices[i]];
      sources = dfl_source_get_children (source);
    }

  return source;
}

static gboolean
dwl_source_model_get_iter (GtkTreeModel *tree_model,
                           GtkTreeIter  *iter,
                           GtkTreePath  *path)
{
  DwlSourceModel *self = DWL_SOURCE_MODEL (tree_model);
  const gint *indices;
  gint depth;

  indices = gtk_tree_path_get_indices_with_depth (path, &depth);

  if (depth < 1)
    {
      iter->stamp = 0;  /* invalid */
      return FALSE;
    }

  /* Store the indices in the iter. Store a refcount on the iter as the
   * stamp. Cache a pointer to the source as user_data3. */
  iter->stamp = 1;
  iter->user_data = g_new0 (gint, depth);
  iter->user_data2 = GINT_TO_POINTER (depth);
  iter->user_data3 = get_source (self->sources, indices, depth);

  memcpy (iter->user_data, indices, sizeof (gint) * depth);

  return TRUE;
}

static GtkTreePath *
dwl_source_model_get_path (GtkTreeModel *tree_model,
                           GtkTreeIter  *iter)
{
  /* Is the iter valid? */
  if (iter->stamp < 1)
    return gtk_tree_path_new ();

  return gtk_tree_path_new_from_indicesv (iter->user_data,
                                          GPOINTER_TO_INT (iter->user_data2));
}

static void
dwl_source_model_get_value (GtkTreeModel *tree_model,
                            GtkTreeIter  *iter,
                            gint          column,
                            GValue       *value)
{
  DflSource *source;

  source = iter->user_data3;
  g_assert (DFL_IS_SOURCE (source));
  g_assert (column >= 0 &&
            (guint) column < G_N_ELEMENTS (dwl_source_model_columns));

  g_value_init (value, dwl_source_model_columns[column].type);
  g_object_get_property (G_OBJECT (source),
                         dwl_source_model_columns[column].property_name, value);
}

static gboolean
dwl_source_model_iter_nth_sibling (GtkTreeModel *tree_model,
                                   GtkTreeIter  *iter,
                                   gint          n)
{
  DwlSourceModel *self = DWL_SOURCE_MODEL (tree_model);
  GPtrArray *sources;  /* (element-type DflSource) */
  gint *indices;
  gint depth;
  gsize i;

  /* Invalid? */
  if (iter->stamp < 1)
    return FALSE;

  indices = iter->user_data;
  depth = GPOINTER_TO_INT (iter->user_data2);

  g_assert (depth > 0);

  /* Find the array for this @depth (minus one as we’re finding a sibling). */
  sources = self->sources;

  for (i = 0; i < (guint) depth - 1; i++)
    sources = dfl_source_get_children (sources->pdata[indices[i]]);

  if ((n >= 0 && indices[depth - 1] <= G_MAXINT - n &&
       (guint) (indices[depth - 1] + n) < sources->len) ||
      (n < 0 && indices[depth - 1] >= -n))
    {
      indices[depth - 1] += n;
      iter->user_data3 = sources->pdata[indices[depth - 1]];
      return TRUE;
    }
  else
    {
      /* Invalidate @iter. */
      iter->stamp = 0;
      g_clear_pointer (&iter->user_data, g_free);
      iter->user_data2 = NULL;
      iter->user_data3 = NULL;

      return FALSE;
    }
}

static gboolean
dwl_source_model_iter_next (GtkTreeModel *tree_model,
                            GtkTreeIter  *iter)
{
  return dwl_source_model_iter_nth_sibling (tree_model, iter, 1);
}

static gboolean
dwl_source_model_iter_previous (GtkTreeModel *tree_model,
                                GtkTreeIter  *iter)
{
  return dwl_source_model_iter_nth_sibling (tree_model, iter, -1);
}

static gboolean
dwl_source_model_iter_children (GtkTreeModel *tree_model,
                                GtkTreeIter  *iter,
                                GtkTreeIter  *parent)
{
  return dwl_source_model_iter_nth_child (tree_model, iter, parent, 0);
}

static gboolean
dwl_source_model_iter_has_child (GtkTreeModel *tree_model,
                                 GtkTreeIter  *iter)
{
  return (dwl_source_model_iter_n_children (tree_model, iter) > 0);
}

static gint
dwl_source_model_iter_n_children (GtkTreeModel *tree_model,
                                  GtkTreeIter  *iter)
{
  DwlSourceModel *self = DWL_SOURCE_MODEL (tree_model);
  DflSource *source;
  GPtrArray *children;  /* (element-type DflSource) */

  /* Top-level case. */
  if (iter == NULL)
    return self->sources->len;

  /* Invalid? */
  if (iter->stamp < 1)
    return 0;

  source = DFL_SOURCE (iter->user_data3);
  children = dfl_source_get_children (source);

  return children->len;
}

static gboolean
dwl_source_model_iter_nth_child (GtkTreeModel *tree_model,
                                 GtkTreeIter  *iter,
                                 GtkTreeIter  *parent,
                                 gint          n)
{
  DwlSourceModel *self = DWL_SOURCE_MODEL (tree_model);
  DflSource *parent_source;
  GPtrArray *children;  /* (element-type DflSource) */
  gint parent_depth;
  gint *parent_indices;
  gint *indices;

  /* Return the first node in the tree? Or the first child node of @parent? */
  if (parent == NULL)
    {
      children = self->sources;
      parent_depth = 0;
      parent_indices = NULL;
    }
  else
    {
      parent_source = parent->user_data3;
      g_assert (DFL_IS_SOURCE (parent_source));
      children = dfl_source_get_children (parent_source);
      parent_depth = GPOINTER_TO_INT (parent->user_data2);
      parent_indices = parent->user_data;
    }

  /* Grab the first element in the array. */
  if (children->len == 0)
    {
      iter->stamp = 0;  /* invalid */
      return FALSE;
    }

  iter->stamp = 1;  /* valid */
  indices = g_new0 (gint, parent_depth + 1);
  iter->user_data = indices;
  iter->user_data2 = GINT_TO_POINTER (parent_depth + 1);
  iter->user_data3 = children->pdata[n];

  if (parent_indices != NULL)
    memcpy (indices, parent_indices, sizeof (gint) * parent_depth);
  indices[parent_depth] = n;

  return TRUE;
}

static gboolean
dwl_source_model_iter_parent (GtkTreeModel *tree_model,
                              GtkTreeIter  *iter,
                              GtkTreeIter  *child)
{
  gint child_depth;
  DflSource *child_source;
  gint *indices;
  const gint *child_indices;

  child_source = DFL_SOURCE (child->user_data3);
  child_indices = child->user_data;
  child_depth = GPOINTER_TO_INT (child->user_data2);

  if (child_depth <= 1)
    {
      iter->stamp = 0;  /* invalid */
      return FALSE;
    }

  iter->stamp = 1;  /* valid */
  indices = g_new0 (gint, child_depth - 1);
  iter->user_data = indices;
  iter->user_data2 = GINT_TO_POINTER (child_depth - 1);
  iter->user_data3 = dfl_source_get_parent (child_source);
  g_assert (iter->user_data3 != NULL);

  memcpy (indices, child_indices, sizeof (gint) * (child_depth - 1));

  return TRUE;
}

static void
dwl_source_model_ref_node (GtkTreeModel *tree_model,
                           GtkTreeIter  *iter)
{
  /* The stamp is the reference count. */
  g_assert (iter->stamp > 0);
  iter->stamp++;
}

static void
dwl_source_model_unref_node (GtkTreeModel *tree_model,
                             GtkTreeIter  *iter)
{
  /* The stamp is the reference count. */
  g_assert (iter->stamp > 0);
  iter->stamp--;

  if (iter->stamp == 0)
    {
      g_free (iter->user_data);
      iter->user_data = NULL;
      iter->user_data2 = GINT_TO_POINTER (0);
      iter->user_data3 = NULL;
    }
}

/**
 * dwl_source_model_new:
 * @sources: (transfer none) (element-type DflSource): array of sources to
 *    expose in the model
 *
 * Create a new #DwlSourceModel to expose the given #DflSources as a tree for
 * use in the UI.
 *
 * Returns: (transfer full): a new #DwlSourceModel
 * Since: UNRELEASED
 */
DwlSourceModel *
dwl_source_model_new (GPtrArray *sources)
{
  g_return_val_if_fail (sources != NULL, NULL);

  return g_object_new (DWL_TYPE_SOURCE_MODEL,
                       "sources", sources,
                       NULL);
}
