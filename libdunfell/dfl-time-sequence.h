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

#ifndef DFL_TIME_SEQUENCE_H
#define DFL_TIME_SEQUENCE_H

#include <glib.h>

#include "dfl-types.h"

G_BEGIN_DECLS

/**
 * DflTimeSequence:
 *
 * All the fields in this structure are private.
 *
 * Since: UNRELEASED
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

/**
 * DflTimeSequenceIter:
 *
 * TODO
 *
 * Since: UNRELEASED
 */
typedef struct
{
  gpointer dummy[2];
} DflTimeSequenceIter;

void     dfl_time_sequence_iter_init (DflTimeSequenceIter *iter,
                                      DflTimeSequence     *sequence,
                                      DflTimestamp         start);
gboolean dfl_time_sequence_iter_next (DflTimeSequenceIter *iter,
                                      DflTimestamp        *timestamp,
                                      gpointer            *data);

G_END_DECLS

#endif /* !DFL_TIME_SEQUENCE_H */
