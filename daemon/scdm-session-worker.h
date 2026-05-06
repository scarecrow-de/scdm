/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 Ray Strode <rstrode@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef __GDM_SESSION_WORKER_H
#define __GDM_SESSION_WORKER_H

#include <glib-object.h>

#include "scdm-session-worker-glue.h"
#include "scdm-session-worker-common.h"
#include "scdm-session-worker-enum-types.h"

G_BEGIN_DECLS

#define SCDM_TYPE_SESSION_WORKER            (scdm_session_worker_get_type ())
#define GDM_SESSION_WORKER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SCDM_TYPE_SESSION_WORKER, ScdmSessionWorker))
#define GDM_SESSION_WORKER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SCDM_TYPE_SESSION_WORKER, ScdmSessionWorkerClass))
#define GDM_IS_SESSION_WORKER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SCDM_TYPE_SESSION_WORKER))
#define GDM_IS_SESSION_WORKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SCDM_TYPE_SESSION_WORKER))
#define GDM_SESSION_WORKER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), SCDM_TYPE_SESSION_WORKER, ScdmSessionWorkerClass))

typedef enum {
        GDM_SESSION_WORKER_STATE_NONE = 0,
        GDM_SESSION_WORKER_STATE_SETUP_COMPLETE,
        GDM_SESSION_WORKER_STATE_AUTHENTICATED,
        GDM_SESSION_WORKER_STATE_AUTHORIZED,
        GDM_SESSION_WORKER_STATE_ACCREDITED,
        GDM_SESSION_WORKER_STATE_ACCOUNT_DETAILS_SAVED,
        GDM_SESSION_WORKER_STATE_SESSION_OPENED,
        GDM_SESSION_WORKER_STATE_SESSION_STARTED
} ScdmSessionWorkerState;

typedef struct ScdmSessionWorkerPrivate ScdmSessionWorkerPrivate;

typedef struct
{
        ScdmDBusWorkerSkeleton parent;
        ScdmSessionWorkerPrivate *priv;
} ScdmSessionWorker;

typedef struct
{
        ScdmDBusWorkerSkeletonClass parent_class;
} ScdmSessionWorkerClass;

GType              scdm_session_worker_get_type                 (void);

ScdmSessionWorker * scdm_session_worker_new                      (const char *server_address,
                                                                gboolean    is_for_reauth) G_GNUC_MALLOC;
G_END_DECLS
#endif /* GDM_SESSION_WORKER_H */
