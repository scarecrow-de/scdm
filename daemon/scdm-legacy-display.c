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
#include "scdm-legacy-display.h"
#include "scdm-local-display-glue.h"
#include "scdm-server.h"
#include "scdm-settings-direct.h"
#include "scdm-settings-keys.h"

struct _ScdmLegacyDisplay
{
        ScdmDisplay           parent;

        ScdmDBusLocalDisplay *skeleton;

        ScdmServer           *server;
};

static void     scdm_legacy_display_class_init   (ScdmLegacyDisplayClass *klass);
static void     scdm_legacy_display_init         (ScdmLegacyDisplay      *legacy_display);

G_DEFINE_TYPE (ScdmLegacyDisplay, scdm_legacy_display, SCDM_TYPE_DISPLAY)

static GObject *
scdm_legacy_display_constructor (GType                  type,
                               guint                  n_construct_properties,
                               GObjectConstructParam *construct_properties)
{
        ScdmLegacyDisplay      *display;

        display = SCDM_LEGACY_DISPLAY (G_OBJECT_CLASS (scdm_legacy_display_parent_class)->constructor (type,
                                                                                                     n_construct_properties,
                                                                                                     construct_properties));

        display->skeleton = SCDM_DBUS_LOCAL_DISPLAY (scdm_dbus_local_display_skeleton_new ());

        g_dbus_object_skeleton_add_interface (scdm_display_get_object_skeleton (SCDM_DISPLAY (display)),
                                              G_DBUS_INTERFACE_SKELETON (display->skeleton));

        return G_OBJECT (display);
}

static void
scdm_legacy_display_finalize (GObject *object)
{
        ScdmLegacyDisplay *display = SCDM_LEGACY_DISPLAY (object);

        g_clear_object (&display->skeleton);
        g_clear_object (&display->server);

        G_OBJECT_CLASS (scdm_legacy_display_parent_class)->finalize (object);
}

static gboolean
scdm_legacy_display_prepare (ScdmDisplay *display)
{
        ScdmLegacyDisplay *self = SCDM_LEGACY_DISPLAY (display);
        ScdmLaunchEnvironment *launch_environment;
        char          *display_name;
        char          *seat_id;
        gboolean       doing_initial_setup = FALSE;

        display_name = NULL;
        seat_id = NULL;

        g_object_get (self,
                      "x11-display-name", &display_name,
                      "seat-id", &seat_id,
                      "doing-initial-setup", &doing_initial_setup,
                      NULL);

        if (!doing_initial_setup) {
                launch_environment = scdm_create_greeter_launch_environment (display_name,
                                                                            seat_id,
                                                                            NULL,
                                                                            NULL,
                                                                            TRUE);
        } else {
                launch_environment = scdm_create_initial_setup_launch_environment (display_name,
                                                                                  seat_id,
                                                                                  NULL,
                                                                                  NULL,
                                                                                  TRUE);
        }

        g_object_set (self, "launch-environment", launch_environment, NULL);
        g_object_unref (launch_environment);

        if (!scdm_display_create_authority (display)) {
                g_warning ("Unable to set up access control for display %s",
                           display_name);
                return FALSE;
        }

        return SCDM_DISPLAY_CLASS (scdm_legacy_display_parent_class)->prepare (display);
}

static void
on_server_ready (ScdmServer       *server,
                 ScdmLegacyDisplay *self)
{
        gboolean ret;

        ret = scdm_display_connect (SCDM_DISPLAY (self));

        if (!ret) {
                g_debug ("ScdmDisplay: could not connect to display");
                scdm_display_unmanage (SCDM_DISPLAY (self));
        } else {
                ScdmLaunchEnvironment *launch_environment;
                char *display_device;

                display_device = scdm_server_get_display_device (server);

                g_object_get (G_OBJECT (self),
                              "launch-environment", &launch_environment,
                              NULL);
                g_object_set (G_OBJECT (launch_environment),
                              "x11-display-device",
                              display_device,
                              NULL);
                g_clear_pointer(&display_device, g_free);
                g_clear_object (&launch_environment);

                g_debug ("ScdmDisplay: connected to display");
                g_object_set (G_OBJECT (self), "status", SCDM_DISPLAY_MANAGED, NULL);
        }
}

static void
on_server_exited (ScdmServer  *server,
                  int         exit_code,
                  ScdmDisplay *self)
{
        g_debug ("ScdmDisplay: server exited with code %d\n", exit_code);

        scdm_display_unmanage (SCDM_DISPLAY (self));
}

static void
on_server_died (ScdmServer  *server,
                int         signal_number,
                ScdmDisplay *self)
{
        g_debug ("ScdmDisplay: server died with signal %d, (%s)",
                 signal_number,
                 g_strsignal (signal_number));

        scdm_display_unmanage (SCDM_DISPLAY (self));
}

static void
scdm_legacy_display_manage (ScdmDisplay *display)
{
        ScdmLegacyDisplay *self = SCDM_LEGACY_DISPLAY (display);
        char            *display_name;
        char            *auth_file;
        char            *seat_id;
        gboolean         is_initial;
        gboolean         res;
        gboolean         disable_tcp;

        g_object_get (G_OBJECT (self),
                      "x11-display-name", &display_name,
                      "x11-authority-file", &auth_file,
                      "seat-id", &seat_id,
                      "is-initial", &is_initial,
                      NULL);

        self->server = scdm_server_new (display_name, seat_id, auth_file, is_initial);

        g_free (display_name);
        g_free (auth_file);
        g_free (seat_id);

        disable_tcp = TRUE;
        if (scdm_settings_direct_get_boolean (SCDM_KEY_DISALLOW_TCP, &disable_tcp)) {
                g_object_set (self->server,
                              "disable-tcp", disable_tcp,
                              NULL);
        }

        g_signal_connect (self->server,
                          "exited",
                          G_CALLBACK (on_server_exited),
                          self);
        g_signal_connect (self->server,
                          "died",
                          G_CALLBACK (on_server_died),
                          self);
        g_signal_connect (self->server,
                          "ready",
                          G_CALLBACK (on_server_ready),
                          self);

        res = scdm_server_start (self->server);
        if (! res) {
                g_warning (_("Could not start the X "
                             "server (your graphical environment) "
                             "due to an internal error. "
                             "Please contact your system administrator "
                             "or check your syslog to diagnose. "
                             "In the meantime this display will be "
                             "disabled.  Please restart GDM when "
                             "the problem is corrected."));
                scdm_display_unmanage (SCDM_DISPLAY (self));
        }

        g_debug ("ScdmDisplay: Started X server");

}

static void
scdm_legacy_display_class_init (ScdmLegacyDisplayClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ScdmDisplayClass *display_class = SCDM_DISPLAY_CLASS (klass);

        object_class->constructor = scdm_legacy_display_constructor;
        object_class->finalize = scdm_legacy_display_finalize;

        display_class->prepare = scdm_legacy_display_prepare;
        display_class->manage = scdm_legacy_display_manage;
}

static void
on_display_status_changed (ScdmLegacyDisplay *self)
{
        int status;

        status = scdm_display_get_status (SCDM_DISPLAY (self));

        switch (status) {
            case SCDM_DISPLAY_UNMANAGED:
                if (self->server != NULL)
                        scdm_server_stop (self->server);
                break;
            default:
                break;
        }
}

static void
scdm_legacy_display_init (ScdmLegacyDisplay *legacy_display)
{
        g_signal_connect (legacy_display, "notify::status",
                          G_CALLBACK (on_display_status_changed),
                          NULL);
}

ScdmDisplay *
scdm_legacy_display_new (int display_number)
{
        GObject *object;
        char    *x11_display;

        x11_display = g_strdup_printf (":%d", display_number);
        object = g_object_new (SCDM_TYPE_LEGACY_DISPLAY,
                               "x11-display-number", display_number,
                               "x11-display-name", x11_display,
                               NULL);
        g_free (x11_display);

        return SCDM_DISPLAY (object);
}
