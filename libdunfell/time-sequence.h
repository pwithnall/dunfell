/* vim:set et sw=2 cin cino=t0,f0,(0,{s,>2s,n-s,^-s,e2s: */
/*
 * Copyright © Philip Withnall 2015 <philip@tecnocode.co.uk>
 * Copyright © Collabora Ltd 2016
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

#ifndef DFL_TIME_SEQUENCE_H
#define DFL_TIME_SEQUENCE_H

#include <glib.h>
#include <glib-object.h>

#include "types.h"

G_BEGIN_DECLS

/**
 * DflTimeSequence:
 *
 * All the fields in this structure are private. Use dfl_time_sequence_init()
 * to initialise an already-allocated sequence.
 *
 * Since: 0.1.0
 */
typedef struct
{
  gpointer dummy[5];
} DflTimeSequence;

void dfl_time_sequence_init (DflTimeSequence *sequence,
                             gsize            element_size,
                             GDestroyNotify   element_destroy_notify,
                             gsize            n_elements_preallocated);
void dfl_time_sequence_clear (DflTimeSequence *sequence);

gpointer dfl_time_sequence_get_last_element (DflTimeSequence *sequence,
                                             DflTimestamp    *timestamp);
gpointer dfl_time_sequence_append           (DflTimeSequence *sequence,
                                             DflTimestamp     timestamp);

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC (DflTimeSequence, dfl_time_sequence_clear)

/**
 * DflTimeSequenceIter:
 *
 * All the fields in this structure are private. Use
 * dfl_time_sequence_iter_init() to initialise an already-allocated iterator.
 *
 * Since: 0.1.0
 */
typedef struct
{
  gpointer dummy[2];
} DflTimeSequenceIter;

GType dfl_time_sequence_iter_get_type (void);

void     dfl_time_sequence_iter_init (DflTimeSequenceIter *iter,
                                      DflTimeSequence     *sequence,
                                      DflTimestamp         start);
gboolean dfl_time_sequence_iter_next (DflTimeSequenceIter *iter,
                                      DflTimestamp        *timestamp,
                                      gpointer            *data);
gboolean dfl_time_sequence_iter_previous (DflTimeSequenceIter *iter,
                                          DflTimestamp        *timestamp,
                                          gpointer            *data);

DflTimeSequenceIter *dfl_time_sequence_iter_copy          (DflTimeSequenceIter *iter);
void                 dfl_time_sequence_iter_free          (DflTimeSequenceIter *iter);

gboolean             dfl_time_sequence_iter_equal         (DflTimeSequenceIter *iter1,
                                                           DflTimeSequenceIter *iter2);

DflTimestamp         dfl_time_sequence_iter_get_timestamp (DflTimeSequenceIter *iter);
gpointer             dfl_time_sequence_iter_get_data      (DflTimeSequenceIter *iter);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (DflTimeSequenceIter, dfl_time_sequence_iter_free)

G_END_DECLS

#endif /* !DFL_TIME_SEQUENCE_H */
