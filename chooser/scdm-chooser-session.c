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
#include <unistd.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include "scdm-chooser-session.h"
#include "scdm-client.h"

#include "scdm-host-chooser-dialog.h"

struct _ScdmChooserSession
{
        GObject                parent;

        ScdmClient             *client;
        ScdmRemoteGreeter      *remote_greeter;
        ScdmChooser            *chooser;
        GtkWidget             *chooser_dialog;
};

enum {
        PROP_0,
};

static void     scdm_chooser_session_class_init  (ScdmChooserSessionClass *klass);
static void     scdm_chooser_session_init        (ScdmChooserSession      *chooser_session);
static void     scdm_chooser_session_finalize    (GObject                *object);

G_DEFINE_TYPE (ScdmChooserSession, scdm_chooser_session, G_TYPE_OBJECT)

static gpointer session_object = NULL;

static gboolean
launch_compiz (ScdmChooserSession *session)
{
        GError  *error;
        gboolean ret;

        g_debug ("ScdmChooserSession: Launching compiz");

        ret = FALSE;

        error = NULL;
        g_spawn_command_line_async ("gtk-window-decorator --replace", &error);
        if (error != NULL) {
                g_warning ("Error starting WM: %s", error->message);
                g_error_free (error);
                goto out;
        }

        error = NULL;
        g_spawn_command_line_async ("compiz --replace", &error);
        if (error != NULL) {
                g_warning ("Error starting WM: %s", error->message);
                g_error_free (error);
                goto out;
        }

        ret = TRUE;

        /* FIXME: should try to detect if it actually works */

 out:
        return ret;
}

static gboolean
launch_metacity (ScdmChooserSession *session)
{
        GError  *error;
        gboolean ret;

        g_debug ("ScdmChooserSession: Launching metacity");

        ret = FALSE;

        error = NULL;
        g_spawn_command_line_async ("metacity --replace", &error);
        if (error != NULL) {
                g_warning ("Error starting WM: %s", error->message);
                g_error_free (error);
                goto out;
        }

        ret = TRUE;

 out:
        return ret;
}

static void
start_window_manager (ScdmChooserSession *session)
{
        if (! launch_metacity (session)) {
                launch_compiz (session);
        }
}

static gboolean
start_settings_daemon (ScdmChooserSession *session)
{
        GError  *error;
        gboolean ret;

        g_debug ("ScdmChooserSession: Launching settings daemon");

        ret = FALSE;

        error = NULL;
        g_spawn_command_line_async (GNOME_SETTINGS_DAEMON_DIR "/scarecrow-settings-daemon", &error);
        if (error != NULL) {
                g_warning ("Error starting settings daemon: %s", error->message);
                g_error_free (error);
                goto out;
        }

        ret = TRUE;

 out:
        return ret;
}

static void
on_dialog_response (GtkDialog         *dialog,
                    int                response_id,
                    ScdmChooserSession *session)
{
        ScdmChooserHost *host;
        GError *error = NULL;

        host = NULL;
        switch (response_id) {
        case GTK_RESPONSE_OK:
                host = scdm_host_chooser_dialog_get_host (SCDM_HOST_CHOOSER_DIALOG (dialog));
        case GTK_RESPONSE_NONE:
                /* delete event */
        default:
                break;
        }

        if (host != NULL) {
                char *hostname;

                /* only support XDMCP hosts in remote chooser */
                g_assert (scdm_chooser_host_get_kind (host) == SCDM_CHOOSER_HOST_KIND_XDMCP);

                hostname = NULL;
                scdm_address_get_hostname (scdm_chooser_host_get_address (host), &hostname);
                /* FIXME: fall back to numerical address? */
                if (hostname != NULL) {
                        g_debug ("ScdmChooserSession: Selected hostname '%s'", hostname);
                        scdm_chooser_call_select_hostname_sync (session->chooser,
                                                               hostname,
                                                               NULL,
                                                               &error);

                        if (error != NULL) {
                                g_debug ("ScdmChooserSession: %s", error->message);
                                g_clear_error (&error);
                        }
                        g_free (hostname);
                }
        }

        scdm_remote_greeter_call_disconnect_sync (session->remote_greeter,
                                                 NULL,
                                                 &error);
        if (error != NULL) {
                g_debug ("ScdmChooserSession: disconnect failed: %s", error->message);
        }
}

gboolean
scdm_chooser_session_start (ScdmChooserSession *session,
                           GError           **error)
{
        g_return_val_if_fail (SCDM_IS_CHOOSER_SESSION (session), FALSE);

        session->remote_greeter = scdm_client_get_remote_greeter_sync (session->client,
                                                                            NULL,
                                                                            error);
        if (session->remote_greeter == NULL) {
                return FALSE;
        }

        session->chooser = scdm_client_get_chooser_sync (session->client,
                                                              NULL,
                                                              error);
        if (session->chooser == NULL) {
                return FALSE;
        }

        start_settings_daemon (session);
        start_window_manager (session);

        /* Only support XDMCP on remote choosers */
        session->chooser_dialog = scdm_host_chooser_dialog_new (SCDM_CHOOSER_HOST_KIND_XDMCP);
        g_signal_connect (session->chooser_dialog,
                          "response",
                          G_CALLBACK (on_dialog_response),
                          session);
        gtk_widget_show (session->chooser_dialog);

        return TRUE;
}

void
scdm_chooser_session_stop (ScdmChooserSession *session)
{
        g_return_if_fail (SCDM_IS_CHOOSER_SESSION (session));

}

static void
scdm_chooser_session_set_property (GObject        *object,
                                  guint           prop_id,
                                  const GValue   *value,
                                  GParamSpec     *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
scdm_chooser_session_get_property (GObject        *object,
                                  guint           prop_id,
                                  GValue         *value,
                                  GParamSpec     *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
scdm_chooser_session_constructor (GType                  type,
                                 guint                  n_construct_properties,
                                 GObjectConstructParam *construct_properties)
{
        ScdmChooserSession      *chooser_session;

        chooser_session = SCDM_CHOOSER_SESSION (G_OBJECT_CLASS (scdm_chooser_session_parent_class)->constructor (type,
                                                                                                               n_construct_properties,
                                                                                                               construct_properties));

        return G_OBJECT (chooser_session);
}

static void
scdm_chooser_session_dispose (GObject *object)
{
        g_debug ("ScdmChooserSession: Disposing chooser_session");

        G_OBJECT_CLASS (scdm_chooser_session_parent_class)->dispose (object);
}

static void
scdm_chooser_session_class_init (ScdmChooserSessionClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = scdm_chooser_session_get_property;
        object_class->set_property = scdm_chooser_session_set_property;
        object_class->constructor = scdm_chooser_session_constructor;
        object_class->dispose = scdm_chooser_session_dispose;
        object_class->finalize = scdm_chooser_session_finalize;
}

static void
scdm_chooser_session_init (ScdmChooserSession *session)
{
        session->client = scdm_client_new ();
}

static void
scdm_chooser_session_finalize (GObject *object)
{
        ScdmChooserSession *chooser_session;

        g_return_if_fail (object != NULL);
        g_return_if_fail (SCDM_IS_CHOOSER_SESSION (object));

        chooser_session = SCDM_CHOOSER_SESSION (object);

        g_return_if_fail (chooser_session != NULL);

        g_clear_object (&chooser_session->chooser);
        g_clear_object (&chooser_session->remote_greeter);
        g_clear_object (&chooser_session->client);

        G_OBJECT_CLASS (scdm_chooser_session_parent_class)->finalize (object);
}

ScdmChooserSession *
scdm_chooser_session_new (void)
{
        if (session_object != NULL) {
                g_object_ref (session_object);
        } else {
                session_object = g_object_new (SCDM_TYPE_CHOOSER_SESSION, NULL);
                g_object_add_weak_pointer (session_object,
                                           (gpointer *) &session_object);
        }

        return SCDM_CHOOSER_SESSION (session_object);
}
