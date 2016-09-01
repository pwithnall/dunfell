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

#ifndef DFL_MODEL_H
#define DFL_MODEL_H

#include <glib.h>
#include <glib-object.h>

#include "event-sequence.h"

G_BEGIN_DECLS

/**
 * DflModel:
 *
 * All the fields in this structure are private.
 *
 * Since: UNRELEASED
 */
#define DFL_TYPE_MODEL dfl_model_get_type ()
G_DECLARE_FINAL_TYPE (DflModel, dfl_model, DFL, MODEL, GObject)

DflModel *dfl_model_new (DflEventSequence *event_sequence);

DflEventSequence *dfl_model_get_event_sequence (DflModel *self);

GPtrArray        *dfl_model_dup_main_contexts  (DflModel *self);
GPtrArray        *dfl_model_dup_threads        (DflModel *self);
GPtrArray        *dfl_model_dup_sources        (DflModel *self);
GPtrArray        *dfl_model_dup_tasks          (DflModel *self);

G_END_DECLS

#endif /* !DFL_MODEL_H */
