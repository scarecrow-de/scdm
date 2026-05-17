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
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include <xcb/xcb.h>
#include <X11/Xlib.h>

#include "scdm-common.h"
#include "scdm-display.h"
#include "scdm-display-glue.h"
#include "scdm-display-access-file.h"
#include "scdm-launch-environment.h"

#include "scdm-settings-direct.h"
#include "scdm-settings-keys.h"

#include "scdm-launch-environment.h"
#include "scdm-dbus-util.h"

#define GNOME_SESSION_SESSIONS_PATH DATADIR "/scarecrow-session/sessions"

typedef struct _ScdmDisplayPrivate
{
        GObject               parent;

        char                 *id;
        char                 *seat_id;
        char                 *session_id;
        char                 *session_class;
        char                 *session_type;

        char                 *remote_hostname;
        int                   x11_display_number;
        char                 *x11_display_name;
        int                   status;
        time_t                creation_time;

        char                 *x11_cookie;
        gsize                 x11_cookie_size;
        ScdmDisplayAccessFile *access_file;

        guint                 finish_idle_id;

        xcb_connection_t     *xcb_connection;
        int                   xcb_screen_number;

        GDBusConnection      *connection;
        ScdmDisplayAccessFile *user_access_file;

        ScdmDBusDisplay       *display_skeleton;
        GDBusObjectSkeleton  *object_skeleton;

        GDBusProxy           *accountsservice_proxy;

        /* this spawns and controls the greeter session */
        ScdmLaunchEnvironment *launch_environment;

        guint                 is_local : 1;
        guint                 is_initial : 1;
        guint                 allow_timed_login : 1;
        guint                 have_existing_user_accounts : 1;
        guint                 doing_initial_setup : 1;
        guint                 session_registered : 1;
} ScdmDisplayPrivate;

enum {
        PROP_0,
        PROP_ID,
        PROP_STATUS,
        PROP_SEAT_ID,
        PROP_SESSION_ID,
        PROP_SESSION_CLASS,
        PROP_SESSION_TYPE,
        PROP_REMOTE_HOSTNAME,
        PROP_X11_DISPLAY_NUMBER,
        PROP_X11_DISPLAY_NAME,
        PROP_X11_COOKIE,
        PROP_X11_AUTHORITY_FILE,
        PROP_IS_CONNECTED,
        PROP_IS_LOCAL,
        PROP_LAUNCH_ENVIRONMENT,
        PROP_IS_INITIAL,
        PROP_ALLOW_TIMED_LOGIN,
        PROP_HAVE_EXISTING_USER_ACCOUNTS,
        PROP_DOING_INITIAL_SETUP,
        PROP_SESSION_REGISTERED,
};

static void     scdm_display_class_init  (ScdmDisplayClass *klass);
static void     scdm_display_init        (ScdmDisplay      *self);
static void     scdm_display_finalize    (GObject         *object);
static void     queue_finish            (ScdmDisplay      *self);
static void     _scdm_display_set_status (ScdmDisplay *self,
                                         int         status);
static gboolean wants_initial_setup (ScdmDisplay *self);
G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (ScdmDisplay, scdm_display, G_TYPE_OBJECT)

GQuark
scdm_display_error_quark (void)
{
        static GQuark ret = 0;
        if (ret == 0) {
                ret = g_quark_from_static_string ("scdm_display_error");
        }

        return ret;
}

time_t
scdm_display_get_creation_time (ScdmDisplay *self)
{
        ScdmDisplayPrivate *priv;

        g_return_val_if_fail (GDM_IS_DISPLAY (self), 0);

        priv = scdm_display_get_instance_private (self);
        return priv->creation_time;
}

int
scdm_display_get_status (ScdmDisplay *self)
{
        ScdmDisplayPrivate *priv;

        g_return_val_if_fail (GDM_IS_DISPLAY (self), 0);

        priv = scdm_display_get_instance_private (self);
        return priv->status;
}

const char *
scdm_display_get_session_id (ScdmDisplay *self)
{
        ScdmDisplayPrivate *priv;

        priv = scdm_display_get_instance_private (self);
        return priv->session_id;
}

static ScdmDisplayAccessFile *
_create_access_file_for_user (ScdmDisplay  *self,
                              const char  *username,
                              GError     **error)
{
        ScdmDisplayAccessFile *access_file;
        GError *file_error;

        access_file = scdm_display_access_file_new (username);

        file_error = NULL;
        if (!scdm_display_access_file_open (access_file, &file_error)) {
                g_propagate_error (error, file_error);
                return NULL;
        }

        return access_file;
}

gboolean
scdm_display_create_authority (ScdmDisplay *self)
{
        ScdmDisplayPrivate    *priv;
        ScdmDisplayAccessFile *access_file;
        GError               *error;
        gboolean              res;

        g_return_val_if_fail (GDM_IS_DISPLAY (self), FALSE);

        priv = scdm_display_get_instance_private (self);
        g_return_val_if_fail (priv->access_file == NULL, FALSE);

        error = NULL;
        access_file = _create_access_file_for_user (self, SCDM_USERNAME, &error);

        if (access_file == NULL) {
                g_critical ("could not create display access file: %s", error->message);
                g_error_free (error);
                return FALSE;
        }

        g_free (priv->x11_cookie);
        priv->x11_cookie = NULL;
        res = scdm_display_access_file_add_display (access_file,
                                                   self,
                                                   &priv->x11_cookie,
                                                   &priv->x11_cookie_size,
                                                   &error);

        if (! res) {

                g_critical ("could not add display to access file: %s", error->message);
                g_error_free (error);
                scdm_display_access_file_close (access_file);
                g_object_unref (access_file);
                return FALSE;
        }

        priv->access_file = access_file;

        return TRUE;
}

static void
setup_xhost_auth (XHostAddress              *host_entries)
{
        host_entries[0].family    = FamilyServerInterpreted;
        host_entries[0].address   = "localuser\0root";
        host_entries[0].length    = sizeof ("localuser\0root");
        host_entries[1].family    = FamilyServerInterpreted;
        host_entries[1].address   = "localuser\0" SCDM_USERNAME;
        host_entries[1].length    = sizeof ("localuser\0" SCDM_USERNAME);
        host_entries[2].family    = FamilyServerInterpreted;
        host_entries[2].address   = "localuser\0gnome-initial-setup";
        host_entries[2].length    = sizeof ("localuser\0gnome-initial-setup");
}

gboolean
scdm_display_add_user_authorization (ScdmDisplay *self,
                                    const char *username,
                                    char      **filename,
                                    GError    **error)
{
        ScdmDisplayPrivate    *priv;
        ScdmDisplayAccessFile *access_file;
        GError               *access_file_error;
        gboolean              res;

        int                       i;
        XHostAddress              host_entries[3];
        xcb_void_cookie_t         cookies[3];

        g_return_val_if_fail (GDM_IS_DISPLAY (self), FALSE);

        priv = scdm_display_get_instance_private (self);

        g_debug ("ScdmDisplay: Adding authorization for user:%s on display %s", username, priv->x11_display_name);

        if (priv->user_access_file != NULL) {
                g_set_error (error,
                             G_DBUS_ERROR,
                             G_DBUS_ERROR_ACCESS_DENIED,
                             "user access already assigned");
                return FALSE;
        }

        g_debug ("ScdmDisplay: Adding user authorization for %s", username);

        access_file_error = NULL;
        access_file = _create_access_file_for_user (self,
                                                    username,
                                                    &access_file_error);

        if (access_file == NULL) {
                g_propagate_error (error, access_file_error);
                return FALSE;
        }

        res = scdm_display_access_file_add_display_with_cookie (access_file,
                                                               self,
                                                               priv->x11_cookie,
                                                               priv->x11_cookie_size,
                                                               &access_file_error);
        if (! res) {
                g_debug ("ScdmDisplay: Unable to add user authorization for %s: %s",
                         username,
                         access_file_error->message);
                g_propagate_error (error, access_file_error);
                scdm_display_access_file_close (access_file);
                g_object_unref (access_file);
                return FALSE;
        }

        *filename = scdm_display_access_file_get_path (access_file);
        priv->user_access_file = access_file;

        g_debug ("ScdmDisplay: Added user authorization for %s: %s", username, *filename);
        /* Remove access for the programs run by greeter now that the
         * user session is starting.
         */
        setup_xhost_auth (host_entries);

        for (i = 0; i < G_N_ELEMENTS (host_entries); i++) {
                cookies[i] = xcb_change_hosts_checked (priv->xcb_connection,
                                                       XCB_HOST_MODE_DELETE,
                                                       host_entries[i].family,
                                                       host_entries[i].length,
                                                       (uint8_t *) host_entries[i].address);
        }

        for (i = 0; i < G_N_ELEMENTS (cookies); i++) {
                xcb_generic_error_t *xcb_error;

                xcb_error = xcb_request_check (priv->xcb_connection, cookies[i]);

                if (xcb_error != NULL) {
                        g_warning ("Failed to remove greeter program access to the display. Trying to proceed.");
                        free (xcb_error);
                }
        }

        return TRUE;
}

gboolean
scdm_display_remove_user_authorization (ScdmDisplay *self,
                                       const char *username,
                                       GError    **error)
{
        ScdmDisplayPrivate *priv;

        g_return_val_if_fail (GDM_IS_DISPLAY (self), FALSE);

        priv = scdm_display_get_instance_private (self);

        g_debug ("ScdmDisplay: Removing authorization for user:%s on display %s", username, priv->x11_display_name);

        scdm_display_access_file_close (priv->user_access_file);

        return TRUE;
}

gboolean
scdm_display_get_x11_cookie (ScdmDisplay  *self,
                            const char **x11_cookie,
                            gsize       *x11_cookie_size,
                            GError     **error)
{
        ScdmDisplayPrivate *priv;

        g_return_val_if_fail (GDM_IS_DISPLAY (self), FALSE);

        priv = scdm_display_get_instance_private (self);

        if (x11_cookie != NULL) {
                *x11_cookie = priv->x11_cookie;
        }

        if (x11_cookie_size != NULL) {
                *x11_cookie_size = priv->x11_cookie_size;
        }

        return TRUE;
}

gboolean
scdm_display_get_x11_authority_file (ScdmDisplay *self,
                                    char      **filename,
                                    GError    **error)
{
        ScdmDisplayPrivate *priv;

        g_return_val_if_fail (GDM_IS_DISPLAY (self), FALSE);
        g_return_val_if_fail (filename != NULL, FALSE);

        priv = scdm_display_get_instance_private (self);
        if (priv->access_file != NULL) {
                *filename = scdm_display_access_file_get_path (priv->access_file);
        } else {
                *filename = NULL;
        }

        return TRUE;
}

gboolean
scdm_display_get_remote_hostname (ScdmDisplay *self,
                                 char      **hostname,
                                 GError    **error)
{
        ScdmDisplayPrivate *priv;

        g_return_val_if_fail (GDM_IS_DISPLAY (self), FALSE);

        priv = scdm_display_get_instance_private (self);
        if (hostname != NULL) {
                *hostname = g_strdup (priv->remote_hostname);
        }

        return TRUE;
}

gboolean
scdm_display_get_x11_display_number (ScdmDisplay *self,
                                    int        *number,
                                    GError    **error)
{
        ScdmDisplayPrivate *priv;

        g_return_val_if_fail (GDM_IS_DISPLAY (self), FALSE);

        priv = scdm_display_get_instance_private (self);
        if (number != NULL) {
                *number = priv->x11_display_number;
        }

        return TRUE;
}

gboolean
scdm_display_get_seat_id (ScdmDisplay *self,
                         char      **seat_id,
                         GError    **error)
{
        ScdmDisplayPrivate *priv;

        g_return_val_if_fail (GDM_IS_DISPLAY (self), FALSE);

        priv = scdm_display_get_instance_private (self);
        if (seat_id != NULL) {
                *seat_id = g_strdup (priv->seat_id);
        }

        return TRUE;
}

gboolean
scdm_display_is_initial (ScdmDisplay  *self,
                        gboolean    *is_initial,
                        GError     **error)
{
        ScdmDisplayPrivate *priv;

        g_return_val_if_fail (GDM_IS_DISPLAY (self), FALSE);

        priv = scdm_display_get_instance_private (self);
        if (is_initial != NULL) {
                *is_initial = priv->is_initial;
        }

        return TRUE;
}

static gboolean
finish_idle (ScdmDisplay *self)
{
        ScdmDisplayPrivate *priv;

        priv = scdm_display_get_instance_private (self);
        priv->finish_idle_id = 0;
        /* finish may end up finalizing object */
        scdm_display_finish (self);
        return FALSE;
}

static void
queue_finish (ScdmDisplay *self)
{
        ScdmDisplayPrivate *priv;

        priv = scdm_display_get_instance_private (self);
        if (priv->finish_idle_id == 0) {
                priv->finish_idle_id = g_idle_add ((GSourceFunc)finish_idle, self);
        }
}

static void
_scdm_display_set_status (ScdmDisplay *self,
                         int         status)
{
        ScdmDisplayPrivate *priv;

        priv = scdm_display_get_instance_private (self);
        if (status != priv->status) {
                priv->status = status;
                g_object_notify (G_OBJECT (self), "status");
        }
}

static gboolean
scdm_display_real_prepare (ScdmDisplay *self)
{
        g_return_val_if_fail (GDM_IS_DISPLAY (self), FALSE);

        g_debug ("ScdmDisplay: prepare display");

        _scdm_display_set_status (self, GDM_DISPLAY_PREPARED);

        return TRUE;
}

static gboolean
look_for_existing_users_sync (ScdmDisplay *self)
{
        ScdmDisplayPrivate *priv;
        GError *error = NULL;
        GVariant *call_result;
        GVariant *user_list;

        priv = scdm_display_get_instance_private (self);
        priv->accountsservice_proxy = g_dbus_proxy_new_sync (priv->connection,
                                                             0, NULL,
                                                             "org.freedesktop.Accounts",
                                                             "/org/freedesktop/Accounts",
                                                             "org.freedesktop.Accounts",
                                                             NULL,
                                                             &error);

        if (!priv->accountsservice_proxy) {
                g_critical ("Failed to contact accountsservice: %s", error->message);
                goto out;
        }

        call_result = g_dbus_proxy_call_sync (priv->accountsservice_proxy,
                                              "ListCachedUsers",
                                              NULL,
                                              0,
                                              -1,
                                              NULL,
                                              &error);

        if (!call_result) {
                g_critical ("Failed to list cached users: %s", error->message);
                goto out;
        }

        g_variant_get (call_result, "(@ao)", &user_list);
        priv->have_existing_user_accounts = g_variant_n_children (user_list) > 0;
        g_variant_unref (user_list);
        g_variant_unref (call_result);
out:
        g_clear_error (&error);
        return priv->accountsservice_proxy != NULL && call_result != NULL;
}

gboolean
scdm_display_prepare (ScdmDisplay *self)
{
        ScdmDisplayPrivate *priv;
        gboolean ret;

        g_return_val_if_fail (GDM_IS_DISPLAY (self), FALSE);

        priv = scdm_display_get_instance_private (self);

        g_debug ("ScdmDisplay: Preparing display: %s", priv->id);

        /* FIXME: we should probably do this in a more global place,
         * asynchronously
         */
        if (!look_for_existing_users_sync (self)) {
                exit (EXIT_FAILURE);
        }

        priv->doing_initial_setup = wants_initial_setup (self);

        g_object_ref (self);
        ret = GDM_DISPLAY_GET_CLASS (self)->prepare (self);
        g_object_unref (self);

        return ret;
}

gboolean
scdm_display_manage (ScdmDisplay *self)
{
        ScdmDisplayPrivate *priv;
        gboolean res;

        g_return_val_if_fail (GDM_IS_DISPLAY (self), FALSE);

        priv = scdm_display_get_instance_private (self);

        g_debug ("ScdmDisplay: Managing display: %s", priv->id);

        /* If not explicitly prepared, do it now */
        if (priv->status == GDM_DISPLAY_UNMANAGED) {
                res = scdm_display_prepare (self);
                if (! res) {
                        return FALSE;
                }
        }

        if (g_strcmp0 (priv->session_class, "greeter") == 0) {
                if (GDM_DISPLAY_GET_CLASS (self)->manage != NULL) {
                        GDM_DISPLAY_GET_CLASS (self)->manage (self);
                }
        }

        return TRUE;
}

gboolean
scdm_display_finish (ScdmDisplay *self)
{
        ScdmDisplayPrivate *priv;

        g_return_val_if_fail (GDM_IS_DISPLAY (self), FALSE);

        priv = scdm_display_get_instance_private (self);
        if (priv->finish_idle_id != 0) {
                g_source_remove (priv->finish_idle_id);
                priv->finish_idle_id = 0;
        }

        _scdm_display_set_status (self, GDM_DISPLAY_FINISHED);

        g_debug ("ScdmDisplay: finish display");

        return TRUE;
}

static void
scdm_display_disconnect (ScdmDisplay *self)
{
        ScdmDisplayPrivate *priv;
        /* These 3 bits are reserved/unused by the X protocol */
        guint32 unused_bits = 0b11100000000000000000000000000000;
        XID highest_client, client;
        guint32 client_increment;
        const xcb_setup_t *setup;

        priv = scdm_display_get_instance_private (self);

        if (priv->xcb_connection == NULL) {
                return;
        }

        setup = xcb_get_setup (priv->xcb_connection);

        /* resource_id_mask is the bits given to each client for
         * addressing resources */
        highest_client = (XID) ~unused_bits & ~setup->resource_id_mask;
        client_increment = setup->resource_id_mask + 1;

        /* Kill every client but ourselves, then close our own connection
         */
        for (client = 0;
             client <= highest_client;
             client += client_increment) {

                if (client != setup->resource_id_base)
                        xcb_kill_client (priv->xcb_connection, client);
        }

        xcb_flush (priv->xcb_connection);

        g_clear_pointer (&priv->xcb_connection, xcb_disconnect);
}

gboolean
scdm_display_unmanage (ScdmDisplay *self)
{
        ScdmDisplayPrivate *priv;

        g_return_val_if_fail (GDM_IS_DISPLAY (self), FALSE);

        priv = scdm_display_get_instance_private (self);

        g_debug ("ScdmDisplay: unmanage display");

        scdm_display_disconnect (self);

        if (priv->user_access_file != NULL) {
                scdm_display_access_file_close (priv->user_access_file);
                g_object_unref (priv->user_access_file);
                priv->user_access_file = NULL;
        }

        if (priv->access_file != NULL) {
                scdm_display_access_file_close (priv->access_file);
                g_object_unref (priv->access_file);
                priv->access_file = NULL;
        }

        if (!priv->session_registered) {
                g_warning ("ScdmDisplay: Session never registered, failing");
                _scdm_display_set_status (self, GDM_DISPLAY_FAILED);
        } else {
                _scdm_display_set_status (self, GDM_DISPLAY_UNMANAGED);
        }

        return TRUE;
}

gboolean
scdm_display_get_id (ScdmDisplay         *self,
                    char              **id,
                    GError            **error)
{
        ScdmDisplayPrivate *priv;

        g_return_val_if_fail (GDM_IS_DISPLAY (self), FALSE);

        priv = scdm_display_get_instance_private (self);
        if (id != NULL) {
                *id = g_strdup (priv->id);
        }

        return TRUE;
}

gboolean
scdm_display_get_x11_display_name (ScdmDisplay   *self,
                                  char        **x11_display,
                                  GError      **error)
{
        ScdmDisplayPrivate *priv;

        g_return_val_if_fail (GDM_IS_DISPLAY (self), FALSE);

        priv = scdm_display_get_instance_private (self);
        if (x11_display != NULL) {
                *x11_display = g_strdup (priv->x11_display_name);
        }

        return TRUE;
}

gboolean
scdm_display_is_local (ScdmDisplay *self,
                      gboolean   *local,
                      GError    **error)
{
        ScdmDisplayPrivate *priv;

        g_return_val_if_fail (GDM_IS_DISPLAY (self), FALSE);

        priv = scdm_display_get_instance_private (self);
        if (local != NULL) {
                *local = priv->is_local;
        }

        return TRUE;
}

static void
_scdm_display_set_id (ScdmDisplay     *self,
                     const char     *id)
{
        ScdmDisplayPrivate *priv;

        priv = scdm_display_get_instance_private (self);
        g_debug ("ScdmDisplay: id: %s", id);
        g_free (priv->id);
        priv->id = g_strdup (id);
}

static void
_scdm_display_set_seat_id (ScdmDisplay     *self,
                          const char     *seat_id)
{
        ScdmDisplayPrivate *priv;

        priv = scdm_display_get_instance_private (self);
        g_debug ("ScdmDisplay: seat id: %s", seat_id);
        g_free (priv->seat_id);
        priv->seat_id = g_strdup (seat_id);
}

static void
_scdm_display_set_session_id (ScdmDisplay     *self,
                             const char     *session_id)
{
        ScdmDisplayPrivate *priv;

        priv = scdm_display_get_instance_private (self);
        g_debug ("ScdmDisplay: session id: %s", session_id);
        g_free (priv->session_id);
        priv->session_id = g_strdup (session_id);
}

static void
_scdm_display_set_session_class (ScdmDisplay *self,
                                const char *session_class)
{
        ScdmDisplayPrivate *priv;

        priv = scdm_display_get_instance_private (self);
        g_debug ("ScdmDisplay: session class: %s", session_class);
        g_free (priv->session_class);
        priv->session_class = g_strdup (session_class);
}

static void
_scdm_display_set_session_type (ScdmDisplay *self,
                               const char *session_type)
{
        ScdmDisplayPrivate *priv;

        priv = scdm_display_get_instance_private (self);
        g_debug ("ScdmDisplay: session type: %s", session_type);
        g_free (priv->session_type);
        priv->session_type = g_strdup (session_type);
}

static void
_scdm_display_set_remote_hostname (ScdmDisplay     *self,
                                  const char     *hostname)
{
        ScdmDisplayPrivate *priv;

        priv = scdm_display_get_instance_private (self);
        g_free (priv->remote_hostname);
        priv->remote_hostname = g_strdup (hostname);
}

static void
_scdm_display_set_x11_display_number (ScdmDisplay     *self,
                                     int             num)
{
        ScdmDisplayPrivate *priv;

        priv = scdm_display_get_instance_private (self);
        priv->x11_display_number = num;
}

static void
_scdm_display_set_x11_display_name (ScdmDisplay     *self,
                                   const char     *x11_display)
{
        ScdmDisplayPrivate *priv;

        priv = scdm_display_get_instance_private (self);
        g_free (priv->x11_display_name);
        priv->x11_display_name = g_strdup (x11_display);
}

static void
_scdm_display_set_x11_cookie (ScdmDisplay     *self,
                             const char     *x11_cookie)
{
        ScdmDisplayPrivate *priv;

        priv = scdm_display_get_instance_private (self);
        g_free (priv->x11_cookie);
        priv->x11_cookie = g_strdup (x11_cookie);
}

static void
_scdm_display_set_is_local (ScdmDisplay     *self,
                           gboolean        is_local)
{
        ScdmDisplayPrivate *priv;

        priv = scdm_display_get_instance_private (self);
        g_debug ("ScdmDisplay: local: %s", is_local? "yes" : "no");
        priv->is_local = is_local;
}

static void
_scdm_display_set_session_registered (ScdmDisplay     *self,
                                     gboolean        registered)
{
        ScdmDisplayPrivate *priv;

        priv = scdm_display_get_instance_private (self);
        g_debug ("ScdmDisplay: session registered: %s", registered? "yes" : "no");
        priv->session_registered = registered;
}

static void
_scdm_display_set_launch_environment (ScdmDisplay           *self,
                                     ScdmLaunchEnvironment *launch_environment)
{
        ScdmDisplayPrivate *priv;

        priv = scdm_display_get_instance_private (self);

        g_clear_object (&priv->launch_environment);

        priv->launch_environment = g_object_ref (launch_environment);
}

static void
_scdm_display_set_is_initial (ScdmDisplay     *self,
                             gboolean        initial)
{
        ScdmDisplayPrivate *priv;

        priv = scdm_display_get_instance_private (self);
        g_debug ("ScdmDisplay: initial: %s", initial? "yes" : "no");
        priv->is_initial = initial;
}

static void
_scdm_display_set_allow_timed_login (ScdmDisplay     *self,
                                    gboolean        allow_timed_login)
{
        ScdmDisplayPrivate *priv;

        priv = scdm_display_get_instance_private (self);
        g_debug ("ScdmDisplay: allow timed login: %s", allow_timed_login? "yes" : "no");
        priv->allow_timed_login = allow_timed_login;
}

static void
scdm_display_set_property (GObject        *object,
                          guint           prop_id,
                          const GValue   *value,
                          GParamSpec     *pspec)
{
        ScdmDisplay *self;

        self = GDM_DISPLAY (object);

        switch (prop_id) {
        case PROP_ID:
                _scdm_display_set_id (self, g_value_get_string (value));
                break;
        case PROP_STATUS:
                _scdm_display_set_status (self, g_value_get_int (value));
                break;
        case PROP_SEAT_ID:
                _scdm_display_set_seat_id (self, g_value_get_string (value));
                break;
        case PROP_SESSION_ID:
                _scdm_display_set_session_id (self, g_value_get_string (value));
                break;
        case PROP_SESSION_CLASS:
                _scdm_display_set_session_class (self, g_value_get_string (value));
                break;
        case PROP_SESSION_TYPE:
                _scdm_display_set_session_type (self, g_value_get_string (value));
                break;
        case PROP_REMOTE_HOSTNAME:
                _scdm_display_set_remote_hostname (self, g_value_get_string (value));
                break;
        case PROP_X11_DISPLAY_NUMBER:
                _scdm_display_set_x11_display_number (self, g_value_get_int (value));
                break;
        case PROP_X11_DISPLAY_NAME:
                _scdm_display_set_x11_display_name (self, g_value_get_string (value));
                break;
        case PROP_X11_COOKIE:
                _scdm_display_set_x11_cookie (self, g_value_get_string (value));
                break;
        case PROP_IS_LOCAL:
                _scdm_display_set_is_local (self, g_value_get_boolean (value));
                break;
        case PROP_ALLOW_TIMED_LOGIN:
                _scdm_display_set_allow_timed_login (self, g_value_get_boolean (value));
                break;
        case PROP_LAUNCH_ENVIRONMENT:
                _scdm_display_set_launch_environment (self, g_value_get_object (value));
                break;
        case PROP_IS_INITIAL:
                _scdm_display_set_is_initial (self, g_value_get_boolean (value));
                break;
        case PROP_SESSION_REGISTERED:
                _scdm_display_set_session_registered (self, g_value_get_boolean (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
scdm_display_get_property (GObject        *object,
                          guint           prop_id,
                          GValue         *value,
                          GParamSpec     *pspec)
{
        ScdmDisplay *self;
        ScdmDisplayPrivate *priv;

        self = GDM_DISPLAY (object);
        priv = scdm_display_get_instance_private (self);

        switch (prop_id) {
        case PROP_ID:
                g_value_set_string (value, priv->id);
                break;
        case PROP_STATUS:
                g_value_set_int (value, priv->status);
                break;
        case PROP_SEAT_ID:
                g_value_set_string (value, priv->seat_id);
                break;
        case PROP_SESSION_ID:
                g_value_set_string (value, priv->session_id);
                break;
        case PROP_SESSION_CLASS:
                g_value_set_string (value, priv->session_class);
                break;
        case PROP_SESSION_TYPE:
                g_value_set_string (value, priv->session_type);
                break;
        case PROP_REMOTE_HOSTNAME:
                g_value_set_string (value, priv->remote_hostname);
                break;
        case PROP_X11_DISPLAY_NUMBER:
                g_value_set_int (value, priv->x11_display_number);
                break;
        case PROP_X11_DISPLAY_NAME:
                g_value_set_string (value, priv->x11_display_name);
                break;
        case PROP_X11_COOKIE:
                g_value_set_string (value, priv->x11_cookie);
                break;
        case PROP_X11_AUTHORITY_FILE:
                g_value_take_string (value,
                                     priv->access_file?
                                     scdm_display_access_file_get_path (priv->access_file) : NULL);
                break;
        case PROP_IS_LOCAL:
                g_value_set_boolean (value, priv->is_local);
                break;
        case PROP_IS_CONNECTED:
                g_value_set_boolean (value, priv->xcb_connection != NULL);
                break;
        case PROP_LAUNCH_ENVIRONMENT:
                g_value_set_object (value, priv->launch_environment);
                break;
        case PROP_IS_INITIAL:
                g_value_set_boolean (value, priv->is_initial);
                break;
        case PROP_HAVE_EXISTING_USER_ACCOUNTS:
                g_value_set_boolean (value, priv->have_existing_user_accounts);
                break;
        case PROP_DOING_INITIAL_SETUP:
                g_value_set_boolean (value, priv->doing_initial_setup);
                break;
        case PROP_SESSION_REGISTERED:
                g_value_set_boolean (value, priv->session_registered);
                break;
        case PROP_ALLOW_TIMED_LOGIN:
                g_value_set_boolean (value, priv->allow_timed_login);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static gboolean
handle_get_id (ScdmDBusDisplay        *skeleton,
               GDBusMethodInvocation *invocation,
               ScdmDisplay            *self)
{
        char *id;

        scdm_display_get_id (self, &id, NULL);

        scdm_dbus_display_complete_get_id (skeleton, invocation, id);

        g_free (id);
        return TRUE;
}

static gboolean
handle_get_remote_hostname (ScdmDBusDisplay        *skeleton,
                            GDBusMethodInvocation *invocation,
                            ScdmDisplay            *self)
{
        char *hostname;

        scdm_display_get_remote_hostname (self, &hostname, NULL);

        scdm_dbus_display_complete_get_remote_hostname (skeleton,
                                                       invocation,
                                                       hostname ? hostname : "");

        g_free (hostname);
        return TRUE;
}

static gboolean
handle_get_seat_id (ScdmDBusDisplay        *skeleton,
                    GDBusMethodInvocation *invocation,
                    ScdmDisplay            *self)
{
        char *seat_id;

        seat_id = NULL;
        scdm_display_get_seat_id (self, &seat_id, NULL);

        if (seat_id == NULL) {
                seat_id = g_strdup ("");
        }
        scdm_dbus_display_complete_get_seat_id (skeleton, invocation, seat_id);

        g_free (seat_id);
        return TRUE;
}

static gboolean
handle_get_x11_display_name (ScdmDBusDisplay        *skeleton,
                             GDBusMethodInvocation *invocation,
                             ScdmDisplay            *self)
{
        char *name;

        scdm_display_get_x11_display_name (self, &name, NULL);

        scdm_dbus_display_complete_get_x11_display_name (skeleton, invocation, name);

        g_free (name);
        return TRUE;
}

static gboolean
handle_is_local (ScdmDBusDisplay        *skeleton,
                 GDBusMethodInvocation *invocation,
                 ScdmDisplay            *self)
{
        gboolean is_local;

        scdm_display_is_local (self, &is_local, NULL);

        scdm_dbus_display_complete_is_local (skeleton, invocation, is_local);

        return TRUE;
}

static gboolean
handle_is_initial (ScdmDBusDisplay        *skeleton,
                   GDBusMethodInvocation *invocation,
                   ScdmDisplay            *self)
{
        gboolean is_initial = FALSE;

        scdm_display_is_initial (self, &is_initial, NULL);

        scdm_dbus_display_complete_is_initial (skeleton, invocation, is_initial);

        return TRUE;
}

static gboolean
register_display (ScdmDisplay *self)
{
        ScdmDisplayPrivate *priv;
        GError *error = NULL;

        priv = scdm_display_get_instance_private (self);

        error = NULL;
        priv->connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
        if (priv->connection == NULL) {
                g_critical ("error getting system bus: %s", error->message);
                g_error_free (error);
                exit (EXIT_FAILURE);
        }

        priv->object_skeleton = g_dbus_object_skeleton_new (priv->id);
        priv->display_skeleton = SCDM_DBUS_DISPLAY (scdm_dbus_display_skeleton_new ());

        g_signal_connect_object (priv->display_skeleton, "handle-get-id",
                                 G_CALLBACK (handle_get_id), self, 0);
        g_signal_connect_object (priv->display_skeleton, "handle-get-remote-hostname",
                                 G_CALLBACK (handle_get_remote_hostname), self, 0);
        g_signal_connect_object (priv->display_skeleton, "handle-get-seat-id",
                                 G_CALLBACK (handle_get_seat_id), self, 0);
        g_signal_connect_object (priv->display_skeleton, "handle-get-x11-display-name",
                                 G_CALLBACK (handle_get_x11_display_name), self, 0);
        g_signal_connect_object (priv->display_skeleton, "handle-is-local",
                                 G_CALLBACK (handle_is_local), self, 0);
        g_signal_connect_object (priv->display_skeleton, "handle-is-initial",
                                 G_CALLBACK (handle_is_initial), self, 0);

        g_dbus_object_skeleton_add_interface (priv->object_skeleton,
                                              G_DBUS_INTERFACE_SKELETON (priv->display_skeleton));

        return TRUE;
}

/*
  dbus-send --system --print-reply --dest=io.github.scarecrow_de.DisplayManager /io/github/scarecrow_de/DisplayManager/Displays/1 org.freedesktop.DBus.Introspectable.Introspect
*/

static GObject *
scdm_display_constructor (GType                  type,
                         guint                  n_construct_properties,
                         GObjectConstructParam *construct_properties)
{
        ScdmDisplay        *self;
        ScdmDisplayPrivate *priv;
        gboolean           res;

        self = GDM_DISPLAY (G_OBJECT_CLASS (scdm_display_parent_class)->constructor (type,
                                                                                    n_construct_properties,
                                                                                    construct_properties));

        priv = scdm_display_get_instance_private (self);

        g_free (priv->id);
        priv->id = g_strdup_printf ("/io/github/scarecrow_de/DisplayManager/Displays/%lu",
                                          (gulong) self);

        res = register_display (self);
        if (! res) {
                g_warning ("Unable to register display with system bus");
        }

        return G_OBJECT (self);
}

static void
scdm_display_dispose (GObject *object)
{
        ScdmDisplay *self;
        ScdmDisplayPrivate *priv;

        self = GDM_DISPLAY (object);
        priv = scdm_display_get_instance_private (self);

        g_debug ("ScdmDisplay: Disposing display");

        if (priv->finish_idle_id != 0) {
                g_source_remove (priv->finish_idle_id);
                priv->finish_idle_id = 0;
        }
        g_clear_object (&priv->launch_environment);

        g_warn_if_fail (priv->status != GDM_DISPLAY_MANAGED);
        g_warn_if_fail (priv->user_access_file == NULL);
        g_warn_if_fail (priv->access_file == NULL);

        G_OBJECT_CLASS (scdm_display_parent_class)->dispose (object);
}

static void
scdm_display_class_init (ScdmDisplayClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = scdm_display_get_property;
        object_class->set_property = scdm_display_set_property;
        object_class->constructor = scdm_display_constructor;
        object_class->dispose = scdm_display_dispose;
        object_class->finalize = scdm_display_finalize;

        klass->prepare = scdm_display_real_prepare;

        g_object_class_install_property (object_class,
                                         PROP_ID,
                                         g_param_spec_string ("id",
                                                              "id",
                                                              "id",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_REMOTE_HOSTNAME,
                                         g_param_spec_string ("remote-hostname",
                                                              "remote-hostname",
                                                              "remote-hostname",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_X11_DISPLAY_NUMBER,
                                         g_param_spec_int ("x11-display-number",
                                                          "x11 display number",
                                                          "x11 display number",
                                                          -1,
                                                          G_MAXINT,
                                                          -1,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_X11_DISPLAY_NAME,
                                         g_param_spec_string ("x11-display-name",
                                                              "x11-display-name",
                                                              "x11-display-name",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_SEAT_ID,
                                         g_param_spec_string ("seat-id",
                                                              "seat id",
                                                              "seat id",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_SESSION_ID,
                                         g_param_spec_string ("session-id",
                                                              "session id",
                                                              "session id",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_SESSION_CLASS,
                                         g_param_spec_string ("session-class",
                                                              NULL,
                                                              NULL,
                                                              "greeter",
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_SESSION_TYPE,
                                         g_param_spec_string ("session-type",
                                                              NULL,
                                                              NULL,
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_IS_INITIAL,
                                         g_param_spec_boolean ("is-initial",
                                                               NULL,
                                                               NULL,
                                                               FALSE,
                                                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_ALLOW_TIMED_LOGIN,
                                         g_param_spec_boolean ("allow-timed-login",
                                                               NULL,
                                                               NULL,
                                                               TRUE,
                                                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_X11_COOKIE,
                                         g_param_spec_string ("x11-cookie",
                                                              "cookie",
                                                              "cookie",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_X11_AUTHORITY_FILE,
                                         g_param_spec_string ("x11-authority-file",
                                                              "authority file",
                                                              "authority file",
                                                              NULL,
                                                              G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

        g_object_class_install_property (object_class,
                                         PROP_IS_LOCAL,
                                         g_param_spec_boolean ("is-local",
                                                               NULL,
                                                               NULL,
                                                               TRUE,
                                                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_IS_CONNECTED,
                                         g_param_spec_boolean ("is-connected",
                                                               NULL,
                                                               NULL,
                                                               TRUE,
                                                               G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_HAVE_EXISTING_USER_ACCOUNTS,
                                         g_param_spec_boolean ("have-existing-user-accounts",
                                                               NULL,
                                                               NULL,
                                                               FALSE,
                                                               G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_DOING_INITIAL_SETUP,
                                         g_param_spec_boolean ("doing-initial-setup",
                                                               NULL,
                                                               NULL,
                                                               FALSE,
                                                               G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_SESSION_REGISTERED,
                                         g_param_spec_boolean ("session-registered",
                                                               NULL,
                                                               NULL,
                                                               FALSE,
                                                               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

        g_object_class_install_property (object_class,
                                         PROP_LAUNCH_ENVIRONMENT,
                                         g_param_spec_object ("launch-environment",
                                                              NULL,
                                                              NULL,
                                                              SCDM_TYPE_LAUNCH_ENVIRONMENT,
                                                              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_STATUS,
                                         g_param_spec_int ("status",
                                                           "status",
                                                           "status",
                                                           -1,
                                                           G_MAXINT,
                                                           GDM_DISPLAY_UNMANAGED,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
}

static void
scdm_display_init (ScdmDisplay *self)
{
        ScdmDisplayPrivate *priv;

        priv = scdm_display_get_instance_private (self);

        priv->creation_time = time (NULL);
}

static void
scdm_display_finalize (GObject *object)
{
        ScdmDisplay *self;
        ScdmDisplayPrivate *priv;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GDM_IS_DISPLAY (object));

        self = GDM_DISPLAY (object);
        priv = scdm_display_get_instance_private (self);

        g_return_if_fail (priv != NULL);

        g_debug ("ScdmDisplay: Finalizing display: %s", priv->id);
        g_free (priv->id);
        g_free (priv->seat_id);
        g_free (priv->session_class);
        g_free (priv->remote_hostname);
        g_free (priv->x11_display_name);
        g_free (priv->x11_cookie);

        g_clear_object (&priv->display_skeleton);
        g_clear_object (&priv->object_skeleton);
        g_clear_object (&priv->connection);
        g_clear_object (&priv->accountsservice_proxy);

        if (priv->access_file != NULL) {
                g_object_unref (priv->access_file);
        }

        if (priv->user_access_file != NULL) {
                g_object_unref (priv->user_access_file);
        }

        G_OBJECT_CLASS (scdm_display_parent_class)->finalize (object);
}

GDBusObjectSkeleton *
scdm_display_get_object_skeleton (ScdmDisplay *self)
{
        ScdmDisplayPrivate *priv;

        priv = scdm_display_get_instance_private (self);
        return priv->object_skeleton;
}

static void
on_launch_environment_session_opened (ScdmLaunchEnvironment *launch_environment,
                                      ScdmDisplay           *self)
{
        char       *session_id;

        g_debug ("ScdmDisplay: Greeter session opened");
        session_id = scdm_launch_environment_get_session_id (launch_environment);
        _scdm_display_set_session_id (self, session_id);
        g_free (session_id);
}

static void
on_launch_environment_session_started (ScdmLaunchEnvironment *launch_environment,
                                       ScdmDisplay           *self)
{
        g_debug ("ScdmDisplay: Greeter started");
}

static void
self_destruct (ScdmDisplay *self)
{
        g_object_ref (self);
        if (scdm_display_get_status (self) == GDM_DISPLAY_MANAGED) {
                scdm_display_unmanage (self);
        }

        if (scdm_display_get_status (self) != GDM_DISPLAY_FINISHED) {
                queue_finish (self);
        }
        g_object_unref (self);
}

static void
on_launch_environment_session_stopped (ScdmLaunchEnvironment *launch_environment,
                                       ScdmDisplay           *self)
{
        g_debug ("ScdmDisplay: Greeter stopped");
        self_destruct (self);
}

static void
on_launch_environment_session_exited (ScdmLaunchEnvironment *launch_environment,
                                      int                   code,
                                      ScdmDisplay           *self)
{
        g_debug ("ScdmDisplay: Greeter exited: %d", code);
        self_destruct (self);
}

static void
on_launch_environment_session_died (ScdmLaunchEnvironment *launch_environment,
                                    int                   signal,
                                    ScdmDisplay           *self)
{
        g_debug ("ScdmDisplay: Greeter died: %d", signal);
        self_destruct (self);
}

static gboolean
can_create_environment (const char *session_id)
{
        char *path;
        gboolean session_exists;

        path = g_strdup_printf (GNOME_SESSION_SESSIONS_PATH "/%s.session", session_id);
        session_exists = g_file_test (path, G_FILE_TEST_EXISTS);

        g_free (path);

        return session_exists;
}

#define ALREADY_RAN_INITIAL_SETUP_ON_THIS_BOOT SCDM_RUN_DIR "/scdm.ran-initial-setup"

static gboolean
already_done_initial_setup_on_this_boot (void)
{
        if (g_file_test (ALREADY_RAN_INITIAL_SETUP_ON_THIS_BOOT, G_FILE_TEST_EXISTS))
                return TRUE;

        return FALSE;
}

static gboolean
kernel_cmdline_initial_setup_argument (const gchar  *contents,
                                       gchar       **initial_setup_argument,
                                       GError      **error)
{
        GRegex *regex = NULL;
        GMatchInfo *match_info = NULL;
        gchar *match_group = NULL;

        g_return_val_if_fail (initial_setup_argument != NULL, FALSE);

        regex = g_regex_new ("\\bgnome.initial-setup=([^\\s]*)\\b", 0, 0, error);

        if (!regex)
            return FALSE;

        if (!g_regex_match (regex, contents, 0, &match_info)) {
                g_free (match_info);
                g_free (regex);

                g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                             "Could not match gnome.initial-setup= in kernel cmdline");

                return FALSE;
        }

        match_group = g_match_info_fetch (match_info, 1);

        if (!match_group) {
                g_free (match_info);
                g_free (regex);

                g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                             "Could not match gnome.initial-setup= in kernel cmdline");

                return FALSE;
        }

        *initial_setup_argument = match_group;

        g_free (match_info);
        g_free (regex);

        return TRUE;
}

/* Function returns true if we had a force state in the kernel
 * cmdline */
static gboolean
kernel_cmdline_initial_setup_force_state (gboolean *force_state)
{
        GError *error = NULL;
        gchar *contents = NULL;
        gchar *setup_argument = NULL;

        g_return_val_if_fail (force_state != NULL, FALSE);

        if (!g_file_get_contents ("/proc/cmdline", &contents, NULL, &error)) {
                g_debug ("ScdmDisplay: Could not check kernel parameters, not forcing initial setup: %s",
                          error->message);
                g_clear_error (&error);
                return FALSE;
        }

        g_debug ("ScdmDisplay: Checking kernel command buffer %s", contents);

        if (!kernel_cmdline_initial_setup_argument (contents, &setup_argument, &error)) {
                g_debug ("ScdmDisplay: Failed to read kernel commandline: %s", error->message);
                g_clear_pointer (&contents, g_free);
                return FALSE;
        }

        g_clear_pointer (&contents, g_free);

        /* Poor-man's check for truthy or falsey values */
        *force_state = setup_argument[0] == '1';

        g_free (setup_argument);
        return TRUE;
}

static gboolean
wants_initial_setup (ScdmDisplay *self)
{
        ScdmDisplayPrivate *priv;
        gboolean enabled = FALSE;
        gboolean forced = FALSE;

        priv = scdm_display_get_instance_private (self);

        if (already_done_initial_setup_on_this_boot ()) {
                return FALSE;
        }

        if (kernel_cmdline_initial_setup_force_state (&forced)) {
                if (forced) {
                        g_debug ("ScdmDisplay: Forcing gnome-initial-setup");
                        return TRUE;
                }

                g_debug ("ScdmDisplay: Forcing no gnome-initial-setup");
                return FALSE;
        }

        /* don't run initial-setup on remote displays
         */
        if (!priv->is_local) {
                return FALSE;
        }

        /* don't run if the system has existing users */
        if (priv->have_existing_user_accounts) {
                return FALSE;
        }

        /* don't run if initial-setup is unavailable */
        if (!can_create_environment ("gnome-initial-setup")) {
                return FALSE;
        }

        if (!scdm_settings_direct_get_boolean (SCDM_KEY_INITIAL_SETUP_ENABLE, &enabled)) {
                return FALSE;
        }

        return enabled;
}

void
scdm_display_start_greeter_session (ScdmDisplay *self)
{
        ScdmDisplayPrivate *priv;
        ScdmSession    *session;
        char          *display_name;
        char          *seat_id;
        char          *hostname;
        char          *auth_file = NULL;

        priv = scdm_display_get_instance_private (self);
        g_return_if_fail (g_strcmp0 (priv->session_class, "greeter") == 0);

        g_debug ("ScdmDisplay: Running greeter");

        display_name = NULL;
        seat_id = NULL;
        hostname = NULL;

        g_object_get (self,
                      "x11-display-name", &display_name,
                      "seat-id", &seat_id,
                      "remote-hostname", &hostname,
                      NULL);
        if (priv->access_file != NULL) {
                auth_file = scdm_display_access_file_get_path (priv->access_file);
        }

        g_debug ("ScdmDisplay: Creating greeter for %s %s", display_name, hostname);

        g_signal_connect_object (priv->launch_environment,
                                 "opened",
                                 G_CALLBACK (on_launch_environment_session_opened),
                                 self, 0);
        g_signal_connect_object (priv->launch_environment,
                                 "started",
                                 G_CALLBACK (on_launch_environment_session_started),
                                 self, 0);
        g_signal_connect_object (priv->launch_environment,
                                 "stopped",
                                 G_CALLBACK (on_launch_environment_session_stopped),
                                 self, 0);
        g_signal_connect_object (priv->launch_environment,
                                 "exited",
                                 G_CALLBACK (on_launch_environment_session_exited),
                                 self, 0);
        g_signal_connect_object (priv->launch_environment,
                                 "died",
                                 G_CALLBACK (on_launch_environment_session_died),
                                 self, 0);

        if (auth_file != NULL) {
                g_object_set (priv->launch_environment,
                              "x11-authority-file", auth_file,
                              NULL);
        }

        scdm_launch_environment_start (priv->launch_environment);

        session = scdm_launch_environment_get_session (priv->launch_environment);
        g_object_set (G_OBJECT (session),
                      "display-is-initial", priv->is_initial,
                      NULL);

        g_free (display_name);
        g_free (seat_id);
        g_free (hostname);
        g_free (auth_file);
}

void
scdm_display_stop_greeter_session (ScdmDisplay *self)
{
        ScdmDisplayPrivate *priv;

        priv = scdm_display_get_instance_private (self);

        if (priv->launch_environment != NULL) {

                g_signal_handlers_disconnect_by_func (priv->launch_environment,
                                                      G_CALLBACK (on_launch_environment_session_opened),
                                                      self);
                g_signal_handlers_disconnect_by_func (priv->launch_environment,
                                                      G_CALLBACK (on_launch_environment_session_started),
                                                      self);
                g_signal_handlers_disconnect_by_func (priv->launch_environment,
                                                      G_CALLBACK (on_launch_environment_session_stopped),
                                                      self);
                g_signal_handlers_disconnect_by_func (priv->launch_environment,
                                                      G_CALLBACK (on_launch_environment_session_exited),
                                                      self);
                g_signal_handlers_disconnect_by_func (priv->launch_environment,
                                                      G_CALLBACK (on_launch_environment_session_died),
                                                      self);
                scdm_launch_environment_stop (priv->launch_environment);
                g_clear_object (&priv->launch_environment);
        }
}

static xcb_window_t
get_root_window (xcb_connection_t *connection,
                 int               screen_number)
{
        xcb_screen_t *screen = NULL;
        xcb_screen_iterator_t iter;

        iter = xcb_setup_roots_iterator (xcb_get_setup (connection));
        while (iter.rem) {
                if (screen_number == 0)
                        screen = iter.data;
                screen_number--;
                xcb_screen_next (&iter);
        }

        if (screen != NULL) {
                return screen->root;
        }

        return XCB_WINDOW_NONE;
}

static void
scdm_display_set_windowpath (ScdmDisplay *self)
{
        ScdmDisplayPrivate *priv;
        /* setting WINDOWPATH for clients */
        xcb_intern_atom_cookie_t atom_cookie;
        xcb_intern_atom_reply_t *atom_reply = NULL;
        xcb_get_property_cookie_t get_property_cookie;
        xcb_get_property_reply_t *get_property_reply = NULL;
        xcb_window_t root_window = XCB_WINDOW_NONE;
        const char *windowpath;
        char *newwindowpath;
        uint32_t num;
        char nums[10];
        int numn;

        priv = scdm_display_get_instance_private (self);

        atom_cookie = xcb_intern_atom (priv->xcb_connection, 0, strlen("XFree86_VT"), "XFree86_VT");
        atom_reply = xcb_intern_atom_reply (priv->xcb_connection, atom_cookie, NULL);

        if (atom_reply == NULL) {
                g_debug ("no XFree86_VT atom\n");
                goto out;
        }

        root_window = get_root_window (priv->xcb_connection,
                                       priv->xcb_screen_number);

        if (root_window == XCB_WINDOW_NONE) {
                g_debug ("couldn't find root window\n");
                goto out;
        }

        get_property_cookie = xcb_get_property (priv->xcb_connection,
                                                FALSE,
                                                root_window,
                                                atom_reply->atom,
                                                XCB_ATOM_INTEGER,
                                                0,
                                                1);

        get_property_reply = xcb_get_property_reply (priv->xcb_connection, get_property_cookie, NULL);

        if (get_property_reply == NULL) {
                g_debug ("no XFree86_VT property\n");
                goto out;
        }

        num = ((uint32_t *) xcb_get_property_value (get_property_reply))[0];

        windowpath = getenv ("WINDOWPATH");
        numn = snprintf (nums, sizeof (nums), "%u", num);
        if (!windowpath) {
                newwindowpath = malloc (numn + 1);
                sprintf (newwindowpath, "%s", nums);
        } else {
                newwindowpath = malloc (strlen (windowpath) + 1 + numn + 1);
                sprintf (newwindowpath, "%s:%s", windowpath, nums);
        }

        g_setenv ("WINDOWPATH", newwindowpath, TRUE);
out:
        g_clear_pointer (&atom_reply, free);
        g_clear_pointer (&get_property_reply, free);
}

gboolean
scdm_display_connect (ScdmDisplay *self)
{
        ScdmDisplayPrivate *priv;
        xcb_auth_info_t *auth_info = NULL;
        gboolean ret;

        priv = scdm_display_get_instance_private (self);
        ret = FALSE;

        g_debug ("ScdmDisplay: Server is ready - opening display %s", priv->x11_display_name);

        /* Get access to the display independent of current hostname */
        if (priv->x11_cookie != NULL) {
                auth_info = g_alloca (sizeof (xcb_auth_info_t));

                auth_info->namelen = strlen ("MIT-MAGIC-COOKIE-1");
                auth_info->name = "MIT-MAGIC-COOKIE-1";
                auth_info->datalen = priv->x11_cookie_size;
                auth_info->data = priv->x11_cookie;

        }

        priv->xcb_connection = xcb_connect_to_display_with_auth_info (priv->x11_display_name,
                                                                      auth_info,
                                                                      &priv->xcb_screen_number);

        if (xcb_connection_has_error (priv->xcb_connection)) {
                g_clear_pointer (&priv->xcb_connection, xcb_disconnect);
                g_warning ("Unable to connect to display %s", priv->x11_display_name);
                ret = FALSE;
        } else if (priv->is_local) {
                XHostAddress              host_entries[3];
                xcb_void_cookie_t         cookies[3];
                int                       i;

                g_debug ("ScdmDisplay: Connected to display %s", priv->x11_display_name);
                ret = TRUE;

                /* Give programs access to the display independent of current hostname
                 */
                setup_xhost_auth (host_entries);

                for (i = 0; i < G_N_ELEMENTS (host_entries); i++) {
                        cookies[i] = xcb_change_hosts_checked (priv->xcb_connection,
                                                               XCB_HOST_MODE_INSERT,
                                                               host_entries[i].family,
                                                               host_entries[i].length,
                                                               (uint8_t *) host_entries[i].address);
                }

                for (i = 0; i < G_N_ELEMENTS (cookies); i++) {
                        xcb_generic_error_t *xcb_error;

                        xcb_error = xcb_request_check (priv->xcb_connection, cookies[i]);

                        if (xcb_error != NULL) {
                                g_debug ("Failed to give system user '%s' access to the display. Trying to proceed.", host_entries[i].address + sizeof ("localuser"));
                                free (xcb_error);
                        } else {
                                g_debug ("Gave system user '%s' access to the display.", host_entries[i].address + sizeof ("localuser"));
                        }
                }

                scdm_display_set_windowpath (self);
        } else {
                g_debug ("ScdmDisplay: Connected to display %s", priv->x11_display_name);
                ret = TRUE;
        }

        if (ret == TRUE) {
                g_object_notify (G_OBJECT (self), "is-connected");
        }

        return ret;
}

