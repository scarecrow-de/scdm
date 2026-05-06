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

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include "scdm-common.h"
#include "scdm-display.h"
#include "scdm-launch-environment.h"
#include "scdm-local-display.h"
#include "scdm-local-display-glue.h"
#include "scdm-settings-direct.h"
#include "scdm-settings-keys.h"

struct _ScdmLocalDisplay
{
        ScdmDisplay           parent;
        ScdmDBusLocalDisplay *skeleton;
};

static void     scdm_local_display_class_init   (ScdmLocalDisplayClass *klass);
static void     scdm_local_display_init         (ScdmLocalDisplay      *local_display);

G_DEFINE_TYPE (ScdmLocalDisplay, scdm_local_display, SCDM_TYPE_DISPLAY)

static GObject *
scdm_local_display_constructor (GType                  type,
                               guint                  n_construct_properties,
                               GObjectConstructParam *construct_properties)
{
        ScdmLocalDisplay      *display;

        display = SCDM_LOCAL_DISPLAY (G_OBJECT_CLASS (scdm_local_display_parent_class)->constructor (type,
                                                                                                   n_construct_properties,
                                                                                                   construct_properties));

        display->skeleton = SCDM_DBUS_LOCAL_DISPLAY (scdm_dbus_local_display_skeleton_new ());

        g_dbus_object_skeleton_add_interface (scdm_display_get_object_skeleton (GDM_DISPLAY (display)),
                                              G_DBUS_INTERFACE_SKELETON (display->skeleton));

        return G_OBJECT (display);
}

static void
scdm_local_display_finalize (GObject *object)
{
        ScdmLocalDisplay *display = SCDM_LOCAL_DISPLAY (object);

        g_clear_object (&display->skeleton);

        G_OBJECT_CLASS (scdm_local_display_parent_class)->finalize (object);
}

static gboolean
scdm_local_display_prepare (ScdmDisplay *display)
{
        ScdmLocalDisplay *self = SCDM_LOCAL_DISPLAY (display);
        ScdmLaunchEnvironment *launch_environment;
        char          *seat_id;
        char          *session_class;
        char          *session_type;
        gboolean       doing_initial_setup = FALSE;
        gboolean       failed = FALSE;

        seat_id = NULL;

        g_object_get (self,
                      "seat-id", &seat_id,
                      "doing-initial-setup", &doing_initial_setup,
                      "session-class", &session_class,
                      "session-type", &session_type,
                      NULL);

        if (g_strcmp0 (session_class, "greeter") != 0) {
                goto out;
        }

        g_debug ("doing initial setup? %s", doing_initial_setup? "yes" : "no");

        if (!doing_initial_setup) {
                launch_environment = scdm_create_greeter_launch_environment (NULL,
                                                                            seat_id,
                                                                            session_type,
                                                                            NULL,
                                                                            TRUE);
        } else {
                launch_environment = scdm_create_initial_setup_launch_environment (NULL,
                                                                                  seat_id,
                                                                                  session_type,
                                                                                  NULL,
                                                                                  TRUE);
        }

        g_object_set (self, "launch-environment", launch_environment, NULL);
        g_object_unref (launch_environment);

out:
        g_free (seat_id);
        g_free (session_class);
        g_free (session_type);

        if (failed) {
                return FALSE;
        }
        return GDM_DISPLAY_CLASS (scdm_local_display_parent_class)->prepare (display);
}

static void
scdm_local_display_class_init (ScdmLocalDisplayClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ScdmDisplayClass *display_class = GDM_DISPLAY_CLASS (klass);

        object_class->constructor = scdm_local_display_constructor;
        object_class->finalize = scdm_local_display_finalize;

        display_class->prepare = scdm_local_display_prepare;
}

static void
scdm_local_display_init (ScdmLocalDisplay *local_display)
{
}

ScdmDisplay *
scdm_local_display_new (void)
{
        GObject *object;

        object = g_object_new (SCDM_TYPE_LOCAL_DISPLAY, NULL);

        return GDM_DISPLAY (object);
}
