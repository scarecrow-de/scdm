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

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include "scdm-display.h"
#include "scdm-launch-environment.h"
#include "scdm-xdmcp-display.h"

#include "scdm-common.h"
#include "scdm-address.h"

#include "scdm-settings.h"
#include "scdm-settings-direct.h"
#include "scdm-settings-keys.h"

typedef struct _ScdmXdmcpDisplayPrivate
{
        ScdmAddress             *remote_address;
        gint32                  session_number;
        guint                   connection_attempts;
} ScdmXdmcpDisplayPrivate;

enum {
        PROP_0,
        PROP_REMOTE_ADDRESS,
        PROP_SESSION_NUMBER,
};

#define MAX_CONNECT_ATTEMPTS  10

static void     scdm_xdmcp_display_class_init    (ScdmXdmcpDisplayClass *klass);
static void     scdm_xdmcp_display_init          (ScdmXdmcpDisplay      *xdmcp_display);

G_DEFINE_TYPE_WITH_PRIVATE (ScdmXdmcpDisplay, scdm_xdmcp_display, SCDM_TYPE_DISPLAY)

gint32
scdm_xdmcp_display_get_session_number (ScdmXdmcpDisplay *display)
{
        ScdmXdmcpDisplayPrivate *priv;

        g_return_val_if_fail (GDM_IS_XDMCP_DISPLAY (display), 0);

        priv = scdm_xdmcp_display_get_instance_private (display);
        return priv->session_number;
}

ScdmAddress *
scdm_xdmcp_display_get_remote_address (ScdmXdmcpDisplay *display)
{
        ScdmXdmcpDisplayPrivate *priv;

        g_return_val_if_fail (GDM_IS_XDMCP_DISPLAY (display), NULL);

        priv = scdm_xdmcp_display_get_instance_private (display);
        return priv->remote_address;
}

static void
_scdm_xdmcp_display_set_remote_address (ScdmXdmcpDisplay *display,
                                       ScdmAddress      *address)
{
        ScdmXdmcpDisplayPrivate *priv;

        priv = scdm_xdmcp_display_get_instance_private (display);
        if (priv->remote_address != NULL) {
                scdm_address_free (priv->remote_address);
        }

        g_assert (address != NULL);

        scdm_address_debug (address);
        priv->remote_address = scdm_address_copy (address);
}

static void
scdm_xdmcp_display_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
        ScdmXdmcpDisplay *self;
        ScdmXdmcpDisplayPrivate *priv;

        self = SCDM_XDMCP_DISPLAY (object);
        priv = scdm_xdmcp_display_get_instance_private (self);

        switch (prop_id) {
        case PROP_REMOTE_ADDRESS:
                _scdm_xdmcp_display_set_remote_address (self, g_value_get_boxed (value));
                break;
        case PROP_SESSION_NUMBER:
                priv->session_number = g_value_get_int (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
scdm_xdmcp_display_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
        ScdmXdmcpDisplay *self;
        ScdmXdmcpDisplayPrivate *priv;

        self = SCDM_XDMCP_DISPLAY (object);
        priv = scdm_xdmcp_display_get_instance_private (self);

        switch (prop_id) {
        case PROP_REMOTE_ADDRESS:
                g_value_set_boxed (value, priv->remote_address);
                break;
        case PROP_SESSION_NUMBER:
                g_value_set_int (value, priv->session_number);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static gboolean
scdm_xdmcp_display_prepare (ScdmDisplay *display)
{
        ScdmXdmcpDisplay *self = SCDM_XDMCP_DISPLAY (display);
        ScdmLaunchEnvironment *launch_environment;
        char          *display_name;
        char          *seat_id;
        char          *hostname;

        launch_environment = NULL;
        display_name = NULL;
        seat_id = NULL;
        hostname = NULL;

        g_object_get (self,
                      "x11-display-name", &display_name,
                      "seat-id", &seat_id,
                      "remote-hostname", &hostname,
                      "launch-environment", &launch_environment,
                      NULL);

        if (launch_environment == NULL) {
                launch_environment = scdm_create_greeter_launch_environment (display_name,
                                                                            seat_id,
                                                                            NULL,
                                                                            hostname,
                                                                            FALSE);
                g_object_set (self, "launch-environment", launch_environment, NULL);
                g_object_unref (launch_environment);
        }

        if (!scdm_display_create_authority (display)) {
                g_warning ("Unable to set up access control for display %s",
                           display_name);
                return FALSE;
        }

        return GDM_DISPLAY_CLASS (scdm_xdmcp_display_parent_class)->prepare (display);
}

static gboolean
idle_connect_to_display (ScdmXdmcpDisplay *self)
{
        ScdmXdmcpDisplayPrivate *priv;
        gboolean res;

        priv = scdm_xdmcp_display_get_instance_private (self);
        priv->connection_attempts++;

        res = scdm_display_connect (GDM_DISPLAY (self));
        if (res) {
                g_object_set (G_OBJECT (self), "status", GDM_DISPLAY_MANAGED, NULL);
        } else {
                if (priv->connection_attempts >= MAX_CONNECT_ATTEMPTS) {
                        g_warning ("Unable to connect to display after %d tries - bailing out", priv->connection_attempts);
                        scdm_display_unmanage (GDM_DISPLAY (self));
                        return FALSE;
                }
                return TRUE;
        }

        return FALSE;
}

static void
scdm_xdmcp_display_manage (ScdmDisplay *display)
{
        ScdmXdmcpDisplay *self = SCDM_XDMCP_DISPLAY (display);

        g_timeout_add (500, (GSourceFunc)idle_connect_to_display, self);
}

static void
scdm_xdmcp_display_class_init (ScdmXdmcpDisplayClass *klass)
{
        GObjectClass    *object_class = G_OBJECT_CLASS (klass);
        ScdmDisplayClass *display_class = GDM_DISPLAY_CLASS (klass);

        object_class->get_property = scdm_xdmcp_display_get_property;
        object_class->set_property = scdm_xdmcp_display_set_property;

        display_class->prepare = scdm_xdmcp_display_prepare;
        display_class->manage = scdm_xdmcp_display_manage;

        g_object_class_install_property (object_class,
                                         PROP_REMOTE_ADDRESS,
                                         g_param_spec_boxed ("remote-address",
                                                             "Remote address",
                                                             "Remote address",
                                                             SCDM_TYPE_ADDRESS,
                                                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

        g_object_class_install_property (object_class,
                                         PROP_SESSION_NUMBER,
                                         g_param_spec_int ("session-number",
                                                           "session-number",
                                                           "session-number",
                                                           G_MININT,
                                                           G_MAXINT,
                                                           0,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

}

static void
scdm_xdmcp_display_init (ScdmXdmcpDisplay *xdmcp_display)
{

        gboolean allow_remote_autologin;

        allow_remote_autologin = FALSE;
        scdm_settings_direct_get_boolean (SCDM_KEY_ALLOW_REMOTE_AUTOLOGIN, &allow_remote_autologin);

        g_object_set (G_OBJECT (xdmcp_display), "allow-timed-login", allow_remote_autologin, NULL);
}

ScdmDisplay *
scdm_xdmcp_display_new (const char *hostname,
                       int         number,
                       ScdmAddress *address,
                       gint32      session_number)
{
        GObject *object;
        char    *x11_display;

        x11_display = g_strdup_printf ("%s:%d", hostname, number);
        object = g_object_new (SCDM_TYPE_XDMCP_DISPLAY,
                               "remote-hostname", hostname,
                               "x11-display-number", number,
                               "x11-display-name", x11_display,
                               "is-local", FALSE,
                               "remote-address", address,
                               "session-number", session_number,
                               NULL);
        g_free (x11_display);

        return GDM_DISPLAY (object);
}
