/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 William Jon McCann <jmccann@redhat.com>
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


#ifndef __SCDM_LAUNCH_ENVIRONMENT_H
#define __SCDM_LAUNCH_ENVIRONMENT_H

#include <glib-object.h>
#include "scdm-session.h"

G_BEGIN_DECLS

#define SCDM_TYPE_LAUNCH_ENVIRONMENT         (scdm_launch_environment_get_type ())
#define SCDM_LAUNCH_ENVIRONMENT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), SCDM_TYPE_LAUNCH_ENVIRONMENT, ScdmLaunchEnvironment))
#define SCDM_LAUNCH_ENVIRONMENT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), SCDM_TYPE_LAUNCH_ENVIRONMENT, ScdmLaunchEnvironmentClass))
#define SCDM_IS_LAUNCH_ENVIRONMENT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), SCDM_TYPE_LAUNCH_ENVIRONMENT))
#define SCDM_IS_LAUNCH_ENVIRONMENT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), SCDM_TYPE_LAUNCH_ENVIRONMENT))
#define SCDM_LAUNCH_ENVIRONMENT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), SCDM_TYPE_LAUNCH_ENVIRONMENT, ScdmLaunchEnvironmentClass))

typedef struct ScdmLaunchEnvironmentPrivate ScdmLaunchEnvironmentPrivate;

typedef struct
{
        GObject                   parent;
        ScdmLaunchEnvironmentPrivate *priv;
} ScdmLaunchEnvironment;

typedef struct
{
        GObjectClass   parent_class;

        /* methods */
        gboolean (*start)          (ScdmLaunchEnvironment  *launch_environment);
        gboolean (*stop)           (ScdmLaunchEnvironment  *launch_environment);


        /* signals */
        void (* opened)            (ScdmLaunchEnvironment  *launch_environment);
        void (* started)           (ScdmLaunchEnvironment  *launch_environment);
        void (* stopped)           (ScdmLaunchEnvironment  *launch_environment);
        void (* exited)            (ScdmLaunchEnvironment  *launch_environment,
                                    int                    exit_code);
        void (* died)              (ScdmLaunchEnvironment  *launch_environment,
                                    int                    signal_number);
        void (* hostname_selected) (ScdmLaunchEnvironment  *launch_environment,
                                    const char            *hostname);
} ScdmLaunchEnvironmentClass;

GType                 scdm_launch_environment_get_type           (void);

gboolean              scdm_launch_environment_start              (ScdmLaunchEnvironment *launch_environment);
gboolean              scdm_launch_environment_stop               (ScdmLaunchEnvironment *launch_environment);
ScdmSession *          scdm_launch_environment_get_session        (ScdmLaunchEnvironment *launch_environment);
char *                scdm_launch_environment_get_session_id     (ScdmLaunchEnvironment *launch_environment);

ScdmLaunchEnvironment *scdm_create_greeter_launch_environment (const char *display_name,
                                                             const char *seat_id,
                                                             const char *session_type,
                                                             const char *display_hostname,
                                                             gboolean    display_is_local);
ScdmLaunchEnvironment *scdm_create_initial_setup_launch_environment (const char *display_name,
                                                                   const char *seat_id,
                                                                   const char *session_type,
                                                                   const char *display_hostname,
                                                                   gboolean    display_is_local);
ScdmLaunchEnvironment *scdm_create_chooser_launch_environment (const char *display_name,
                                                             const char *seat_id,
                                                             const char *display_hostname);

G_END_DECLS

#endif /* __SCDM_LAUNCH_ENVIRONMENT_H */
