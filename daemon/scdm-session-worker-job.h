/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */


#ifndef __GDM_SESSION_WORKER_JOB_H
#define __GDM_SESSION_WORKER_JOB_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GDM_TYPE_SESSION_WORKER_JOB (scdm_session_worker_job_get_type ())
G_DECLARE_FINAL_TYPE (ScdmSessionWorkerJob, scdm_session_worker_job, GDM, SESSION_WORKER_JOB, GObject)

ScdmSessionWorkerJob *   scdm_session_worker_job_new                (void);
void                    scdm_session_worker_job_set_server_address (ScdmSessionWorkerJob *session_worker_job,
                                                                   const char          *server_address);
void                    scdm_session_worker_job_set_for_reauth (ScdmSessionWorkerJob *session_worker_job,
                                                               gboolean             for_reauth);
void                    scdm_session_worker_job_set_environment    (ScdmSessionWorkerJob *session_worker_job,
                                                                   const char * const  *environment);
gboolean                scdm_session_worker_job_start              (ScdmSessionWorkerJob *session_worker_job,
                                                                   const char          *name);
void                    scdm_session_worker_job_stop               (ScdmSessionWorkerJob *session_worker_job);
void                    scdm_session_worker_job_stop_now           (ScdmSessionWorkerJob *session_worker_job);

GPid                    scdm_session_worker_job_get_pid            (ScdmSessionWorkerJob *session_worker_job);

G_END_DECLS

#endif /* __GDM_SESSION_WORKER_JOB_H */
