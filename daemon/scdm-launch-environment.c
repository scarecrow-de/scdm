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

#include "config.h"

#include <gio/gio.h>

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#include "scdm-common.h"

#include "scdm-session-enum-types.h"
#include "scdm-launch-environment.h"
#include "scdm-settings-direct.h"
#include "scdm-settings-keys.h"

#define INITIAL_SETUP_USERNAME "gnome-initial-setup"
#define GDM_SESSION_MODE "scdm"
#define INITIAL_SETUP_SESSION_MODE "initial-setup"
#define GNOME_SESSION_SESSIONS_PATH DATADIR "/gnome-session/sessions"

extern char **environ;

struct ScdmLaunchEnvironmentPrivate
{
        ScdmSession     *session;
        char           *command;
        GPid            pid;

        ScdmSessionVerificationMode verification_mode;

        char           *user_name;
        char           *runtime_dir;

        char           *session_id;
        char           *session_type;
        char           *session_mode;
        char           *x11_display_name;
        char           *x11_display_seat_id;
        char           *x11_display_device;
        char           *x11_display_hostname;
        char           *x11_authority_file;
        gboolean        x11_display_is_local;
};

enum {
        PROP_0,
        PROP_VERIFICATION_MODE,
        PROP_SESSION_TYPE,
        PROP_SESSION_MODE,
        PROP_X11_DISPLAY_NAME,
        PROP_X11_DISPLAY_SEAT_ID,
        PROP_X11_DISPLAY_DEVICE,
        PROP_X11_DISPLAY_HOSTNAME,
        PROP_X11_AUTHORITY_FILE,
        PROP_X11_DISPLAY_IS_LOCAL,
        PROP_USER_NAME,
        PROP_RUNTIME_DIR,
        PROP_COMMAND,
};

enum {
        OPENED,
        STARTED,
        STOPPED,
        EXITED,
        DIED,
        HOSTNAME_SELECTED,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

static void     scdm_launch_environment_class_init    (ScdmLaunchEnvironmentClass *klass);
static void     scdm_launch_environment_init          (ScdmLaunchEnvironment      *launch_environment);
static void     scdm_launch_environment_finalize      (GObject                   *object);

G_DEFINE_TYPE_WITH_PRIVATE (ScdmLaunchEnvironment, scdm_launch_environment, G_TYPE_OBJECT)

static GHashTable *
build_launch_environment (ScdmLaunchEnvironment *launch_environment,
                          gboolean              start_session)
{
        GHashTable    *hash;
        struct passwd *pwent;
        static const char *const optional_environment[] = {
                "GI_TYPELIB_PATH",
                "LANG",
                "LANGUAGE",
                "LC_ADDRESS",
                "LC_ALL",
                "LC_COLLATE",
                "LC_CTYPE",
                "LC_IDENTIFICATION",
                "LC_MEASUREMENT",
                "LC_MESSAGES",
                "LC_MONETARY",
                "LC_NAME",
                "LC_NUMERIC",
                "LC_PAPER",
                "LC_TELEPHONE",
                "LC_TIME",
                "LD_LIBRARY_PATH",
                "PATH",
                "WINDOWPATH",
                "XCURSOR_PATH",
                "XDG_CONFIG_DIRS",
                NULL
        };
        char *system_data_dirs;
        int i;

        /* create a hash table of current environment, then update keys has necessary */
        hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

        for (i = 0; optional_environment[i] != NULL; i++) {
                if (g_getenv (optional_environment[i]) == NULL) {
                        continue;
                }

                g_hash_table_insert (hash,
                                     g_strdup (optional_environment[i]),
                                     g_strdup (g_getenv (optional_environment[i])));
        }

        system_data_dirs = g_strjoinv (":", (char **) g_get_system_data_dirs ());

        g_hash_table_insert (hash,
                             g_strdup ("XDG_DATA_DIRS"),
                             g_strdup_printf ("%s:%s",
                                              DATADIR "/scdm/greeter",
                                              system_data_dirs));
        g_free (system_data_dirs);

        if (launch_environment->priv->x11_authority_file != NULL)
                g_hash_table_insert (hash, g_strdup ("XAUTHORITY"), g_strdup (launch_environment->priv->x11_authority_file));

        if (launch_environment->priv->session_mode != NULL) {
                g_hash_table_insert (hash, g_strdup ("GNOME_SHELL_SESSION_MODE"), g_strdup (launch_environment->priv->session_mode));

		if (strcmp (launch_environment->priv->session_mode, INITIAL_SETUP_SESSION_MODE) != 0) {
			/* gvfs is needed for fetching remote avatars in the initial setup. Disable it otherwise. */
			g_hash_table_insert (hash, g_strdup ("GVFS_DISABLE_FUSE"), g_strdup ("1"));
			g_hash_table_insert (hash, g_strdup ("GIO_USE_VFS"), g_strdup ("local"));
			g_hash_table_insert (hash, g_strdup ("GVFS_REMOTE_VOLUME_MONITOR_IGNORE"), g_strdup ("1"));

			/* The locked down dconf profile should not be used for the initial setup session.
			 * This allows overridden values from the user profile to take effect.
			 */
			g_hash_table_insert (hash, g_strdup ("DCONF_PROFILE"), g_strdup ("scdm"));
		}
        }

        g_hash_table_insert (hash, g_strdup ("LOGNAME"), g_strdup (launch_environment->priv->user_name));
        g_hash_table_insert (hash, g_strdup ("USER"), g_strdup (launch_environment->priv->user_name));
        g_hash_table_insert (hash, g_strdup ("USERNAME"), g_strdup (launch_environment->priv->user_name));

        g_hash_table_insert (hash, g_strdup ("GDM_VERSION"), g_strdup (VERSION));
        g_hash_table_remove (hash, "MAIL");

        g_hash_table_insert (hash, g_strdup ("HOME"), g_strdup ("/"));
        g_hash_table_insert (hash, g_strdup ("PWD"), g_strdup ("/"));
        g_hash_table_insert (hash, g_strdup ("SHELL"), g_strdup ("/bin/sh"));

        scdm_get_pwent_for_name (launch_environment->priv->user_name, &pwent);
        if (pwent != NULL) {
                if (pwent->pw_dir != NULL && pwent->pw_dir[0] != '\0') {
                        g_hash_table_insert (hash, g_strdup ("HOME"), g_strdup (pwent->pw_dir));
                        g_hash_table_insert (hash, g_strdup ("PWD"), g_strdup (pwent->pw_dir));
                }

                g_hash_table_insert (hash, g_strdup ("SHELL"), g_strdup (pwent->pw_shell));
        }

        if (start_session && launch_environment->priv->x11_display_seat_id != NULL) {
                char *seat_id;

                seat_id = launch_environment->priv->x11_display_seat_id;

                g_hash_table_insert (hash, g_strdup ("GDM_SEAT_ID"), g_strdup (seat_id));
        }

        g_hash_table_insert (hash, g_strdup ("RUNNING_UNDER_GDM"), g_strdup ("true"));

        return hash;
}

static void
on_session_setup_complete (ScdmSession        *session,
                           const char        *service_name,
                           ScdmLaunchEnvironment *launch_environment)
{
        GHashTable       *hash;
        GHashTableIter    iter;
        gpointer          key, value;

        hash = build_launch_environment (launch_environment, TRUE);

        g_hash_table_iter_init (&iter, hash);
        while (g_hash_table_iter_next (&iter, &key, &value)) {
                scdm_session_set_environment_variable (launch_environment->priv->session, key, value);
        }
        g_hash_table_destroy (hash);
}

static void
on_session_opened (ScdmSession           *session,
                   const char           *service_name,
                   const char           *session_id,
                   ScdmLaunchEnvironment *launch_environment)
{
        launch_environment->priv->session_id = g_strdup (session_id);

        g_signal_emit (G_OBJECT (launch_environment), signals [OPENED], 0);
        scdm_session_start_session (launch_environment->priv->session, service_name);
}

static void
on_session_started (ScdmSession           *session,
                    const char           *service_name,
                    int                   pid,
                    ScdmLaunchEnvironment *launch_environment)
{
        launch_environment->priv->pid = pid;
        g_signal_emit (G_OBJECT (launch_environment), signals [STARTED], 0);
}

static void
on_session_exited (ScdmSession           *session,
                   int                   exit_code,
                   ScdmLaunchEnvironment *launch_environment)
{
        scdm_session_stop_conversation (launch_environment->priv->session, "scdm-launch-environment");

        g_signal_emit (G_OBJECT (launch_environment), signals [EXITED], 0, exit_code);
}

static void
on_session_died (ScdmSession           *session,
                 int                   signal_number,
                 ScdmLaunchEnvironment *launch_environment)
{
        scdm_session_stop_conversation (launch_environment->priv->session, "scdm-launch-environment");

        g_signal_emit (G_OBJECT (launch_environment), signals [DIED], 0, signal_number);
}

static void
on_hostname_selected (ScdmSession               *session,
                      const char               *hostname,
		      ScdmLaunchEnvironment     *launch_environment)
{
        g_debug ("ScdmSession: hostname selected: %s", hostname);
        g_signal_emit (launch_environment, signals [HOSTNAME_SELECTED], 0, hostname);
}

static void
on_conversation_started (ScdmSession           *session,
                         const char           *service_name,
                         ScdmLaunchEnvironment *launch_environment)
{
        char             *log_path;
        char             *log_file;

        if (launch_environment->priv->x11_display_name != NULL)
                log_file = g_strdup_printf ("%s-greeter.log", launch_environment->priv->x11_display_name);
        else
                log_file = g_strdup ("greeter.log");

        log_path = g_build_filename (LOGDIR, log_file, NULL);
        g_free (log_file);

        scdm_session_setup_for_program (launch_environment->priv->session,
                                       "scdm-launch-environment",
                                       launch_environment->priv->user_name,
                                       log_path);
        g_free (log_path);
}

static void
on_conversation_stopped (ScdmSession           *session,
                         const char           *service_name,
                         ScdmLaunchEnvironment *launch_environment)
{
        ScdmSession *conversation_session;

        conversation_session = launch_environment->priv->session;
        launch_environment->priv->session = NULL;

        g_debug ("ScdmLaunchEnvironment: conversation stopped");

        if (launch_environment->priv->pid > 1) {
                scdm_signal_pid (-launch_environment->priv->pid, SIGTERM);
                g_signal_emit (G_OBJECT (launch_environment), signals [STOPPED], 0);
        }

        if (conversation_session != NULL) {
                scdm_session_close (conversation_session);
                g_object_unref (conversation_session);
        }
}

static gboolean
ensure_directory_with_uid_gid (const char  *path,
                               uid_t        uid,
                               gid_t        gid,
                               GError     **error)
{
        if (mkdir (path, 0700) == -1 && errno != EEXIST) {
                g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                             "Failed to create directory %s: %s", path,
                             g_strerror (errno));
                return FALSE;
        }
        if (chown (path, uid, gid) == -1) {
                g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                             "Failed to set owner of %s: %s", path,
                             g_strerror (errno));
                return FALSE;
        }
        return TRUE;
}

/**
 * scdm_launch_environment_start:
 * @disp: Pointer to a ScdmDisplay structure
 *
 * Starts a local X launch_environment. Handles retries and fatal errors properly.
 */
gboolean
scdm_launch_environment_start (ScdmLaunchEnvironment *launch_environment)
{
        gboolean          res = FALSE;
        GError           *local_error = NULL;
        GError          **error = &local_error;
        struct passwd    *passwd_entry;
        uid_t             uid;
        gid_t             gid;

        g_debug ("ScdmLaunchEnvironment: Starting...");

        if (!scdm_get_pwent_for_name (launch_environment->priv->user_name, &passwd_entry)) {
                g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                             "Unknown user %s",
                             launch_environment->priv->user_name);
                goto out;
        }

        uid = passwd_entry->pw_uid;
        gid = passwd_entry->pw_gid;

        g_debug ("ScdmLaunchEnvironment: Setting up run time dir %s",
                 launch_environment->priv->runtime_dir);
        if (!ensure_directory_with_uid_gid (launch_environment->priv->runtime_dir, uid, gid, error)) {
                goto out;
        }

        /* Create the home directory too */
        if (!ensure_directory_with_uid_gid (passwd_entry->pw_dir, uid, gid, error))
                goto out;

        launch_environment->priv->session = scdm_session_new (launch_environment->priv->verification_mode,
                                                             uid,
                                                             launch_environment->priv->x11_display_name,
                                                             launch_environment->priv->x11_display_hostname,
                                                             launch_environment->priv->x11_display_device,
                                                             launch_environment->priv->x11_display_seat_id,
                                                             launch_environment->priv->x11_authority_file,
                                                             launch_environment->priv->x11_display_is_local,
                                                             NULL);

        g_signal_connect_object (launch_environment->priv->session,
                                 "conversation-started",
                                 G_CALLBACK (on_conversation_started),
                                 launch_environment,
                                 0);
        g_signal_connect_object (launch_environment->priv->session,
                                 "conversation-stopped",
                                 G_CALLBACK (on_conversation_stopped),
                                 launch_environment,
                                 0);
        g_signal_connect_object (launch_environment->priv->session,
                                 "setup-complete",
                                 G_CALLBACK (on_session_setup_complete),
                                 launch_environment,
                                 0);
        g_signal_connect_object (launch_environment->priv->session,
                                 "session-opened",
                                 G_CALLBACK (on_session_opened),
                                 launch_environment,
                                 0);
        g_signal_connect_object (launch_environment->priv->session,
                                 "session-started",
                                 G_CALLBACK (on_session_started),
                                 launch_environment,
                                 0);
        g_signal_connect_object (launch_environment->priv->session,
                                 "session-exited",
                                 G_CALLBACK (on_session_exited),
                                 launch_environment,
                                 0);
        g_signal_connect_object (launch_environment->priv->session,
                                 "session-died",
                                 G_CALLBACK (on_session_died),
                                 launch_environment,
                                 0);
        g_signal_connect_object (launch_environment->priv->session,
                                 "hostname-selected",
                                 G_CALLBACK (on_hostname_selected),
                                 launch_environment,
                                 0);

        scdm_session_start_conversation (launch_environment->priv->session, "scdm-launch-environment");
        scdm_session_select_program (launch_environment->priv->session, launch_environment->priv->command);

        if (launch_environment->priv->session_type != NULL) {
                g_object_set (G_OBJECT (launch_environment->priv->session),
                              "session-type",
                              launch_environment->priv->session_type,
                              NULL);
        }

        res = TRUE;
 out:
        if (local_error) {
                g_critical ("ScdmLaunchEnvironment: %s", local_error->message);
                g_clear_error (&local_error);
        }
        return res;
}

gboolean
scdm_launch_environment_stop (ScdmLaunchEnvironment *launch_environment)
{
        if (launch_environment->priv->pid > 1) {
                scdm_signal_pid (-launch_environment->priv->pid, SIGTERM);
        }

        if (launch_environment->priv->session != NULL) {
                scdm_session_close (launch_environment->priv->session);

                g_clear_object (&launch_environment->priv->session);
        }

        g_signal_emit (G_OBJECT (launch_environment), signals [STOPPED], 0);

        return TRUE;
}

ScdmSession *
scdm_launch_environment_get_session (ScdmLaunchEnvironment *launch_environment)
{
        return launch_environment->priv->session;
}

char *
scdm_launch_environment_get_session_id (ScdmLaunchEnvironment *launch_environment)
{
        return g_strdup (launch_environment->priv->session_id);
}

static void
_scdm_launch_environment_set_verification_mode (ScdmLaunchEnvironment           *launch_environment,
                                               ScdmSessionVerificationMode      verification_mode)
{
        launch_environment->priv->verification_mode = verification_mode;
}

static void
_scdm_launch_environment_set_session_type (ScdmLaunchEnvironment *launch_environment,
                                          const char           *session_type)
{
        g_free (launch_environment->priv->session_type);
        launch_environment->priv->session_type = g_strdup (session_type);
}

static void
_scdm_launch_environment_set_session_mode (ScdmLaunchEnvironment *launch_environment,
                                          const char           *session_mode)
{
        g_free (launch_environment->priv->session_mode);
        launch_environment->priv->session_mode = g_strdup (session_mode);
}

static void
_scdm_launch_environment_set_x11_display_name (ScdmLaunchEnvironment *launch_environment,
                                              const char           *name)
{
        g_free (launch_environment->priv->x11_display_name);
        launch_environment->priv->x11_display_name = g_strdup (name);
}

static void
_scdm_launch_environment_set_x11_display_seat_id (ScdmLaunchEnvironment *launch_environment,
                                                 const char           *sid)
{
        g_free (launch_environment->priv->x11_display_seat_id);
        launch_environment->priv->x11_display_seat_id = g_strdup (sid);
}

static void
_scdm_launch_environment_set_x11_display_hostname (ScdmLaunchEnvironment *launch_environment,
                                                  const char           *name)
{
        g_free (launch_environment->priv->x11_display_hostname);
        launch_environment->priv->x11_display_hostname = g_strdup (name);
}

static void
_scdm_launch_environment_set_x11_display_device (ScdmLaunchEnvironment *launch_environment,
                                                const char           *name)
{
        g_free (launch_environment->priv->x11_display_device);
        launch_environment->priv->x11_display_device = g_strdup (name);
}

static void
_scdm_launch_environment_set_x11_display_is_local (ScdmLaunchEnvironment *launch_environment,
                                                  gboolean              is_local)
{
        launch_environment->priv->x11_display_is_local = is_local;
}

static void
_scdm_launch_environment_set_x11_authority_file (ScdmLaunchEnvironment *launch_environment,
                                                const char           *file)
{
        g_free (launch_environment->priv->x11_authority_file);
        launch_environment->priv->x11_authority_file = g_strdup (file);
}

static void
_scdm_launch_environment_set_user_name (ScdmLaunchEnvironment *launch_environment,
                                       const char           *name)
{
        g_free (launch_environment->priv->user_name);
        launch_environment->priv->user_name = g_strdup (name);
}

static void
_scdm_launch_environment_set_runtime_dir (ScdmLaunchEnvironment *launch_environment,
                                         const char           *dir)
{
        g_free (launch_environment->priv->runtime_dir);
        launch_environment->priv->runtime_dir = g_strdup (dir);
}

static void
_scdm_launch_environment_set_command (ScdmLaunchEnvironment *launch_environment,
                                     const char           *name)
{
        g_free (launch_environment->priv->command);
        launch_environment->priv->command = g_strdup (name);
}

static void
scdm_launch_environment_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
        ScdmLaunchEnvironment *self;

        self = GDM_LAUNCH_ENVIRONMENT (object);

        switch (prop_id) {
        case PROP_VERIFICATION_MODE:
                _scdm_launch_environment_set_verification_mode (self, g_value_get_enum (value));
                break;
        case PROP_SESSION_TYPE:
                _scdm_launch_environment_set_session_type (self, g_value_get_string (value));
                break;
        case PROP_SESSION_MODE:
                _scdm_launch_environment_set_session_mode (self, g_value_get_string (value));
                break;
        case PROP_X11_DISPLAY_NAME:
                _scdm_launch_environment_set_x11_display_name (self, g_value_get_string (value));
                break;
        case PROP_X11_DISPLAY_SEAT_ID:
                _scdm_launch_environment_set_x11_display_seat_id (self, g_value_get_string (value));
                break;
        case PROP_X11_DISPLAY_HOSTNAME:
                _scdm_launch_environment_set_x11_display_hostname (self, g_value_get_string (value));
                break;
        case PROP_X11_DISPLAY_DEVICE:
                _scdm_launch_environment_set_x11_display_device (self, g_value_get_string (value));
                break;
        case PROP_X11_DISPLAY_IS_LOCAL:
                _scdm_launch_environment_set_x11_display_is_local (self, g_value_get_boolean (value));
                break;
        case PROP_X11_AUTHORITY_FILE:
                _scdm_launch_environment_set_x11_authority_file (self, g_value_get_string (value));
                break;
        case PROP_USER_NAME:
                _scdm_launch_environment_set_user_name (self, g_value_get_string (value));
                break;
        case PROP_RUNTIME_DIR:
                _scdm_launch_environment_set_runtime_dir (self, g_value_get_string (value));
                break;
        case PROP_COMMAND:
                _scdm_launch_environment_set_command (self, g_value_get_string (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
scdm_launch_environment_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
        ScdmLaunchEnvironment *self;

        self = GDM_LAUNCH_ENVIRONMENT (object);

        switch (prop_id) {
        case PROP_VERIFICATION_MODE:
                g_value_set_enum (value, self->priv->verification_mode);
                break;
        case PROP_SESSION_TYPE:
                g_value_set_string (value, self->priv->session_type);
                break;
        case PROP_SESSION_MODE:
                g_value_set_string (value, self->priv->session_mode);
                break;
        case PROP_X11_DISPLAY_NAME:
                g_value_set_string (value, self->priv->x11_display_name);
                break;
        case PROP_X11_DISPLAY_SEAT_ID:
                g_value_set_string (value, self->priv->x11_display_seat_id);
                break;
        case PROP_X11_DISPLAY_HOSTNAME:
                g_value_set_string (value, self->priv->x11_display_hostname);
                break;
        case PROP_X11_DISPLAY_DEVICE:
                g_value_set_string (value, self->priv->x11_display_device);
                break;
        case PROP_X11_DISPLAY_IS_LOCAL:
                g_value_set_boolean (value, self->priv->x11_display_is_local);
                break;
        case PROP_X11_AUTHORITY_FILE:
                g_value_set_string (value, self->priv->x11_authority_file);
                break;
        case PROP_USER_NAME:
                g_value_set_string (value, self->priv->user_name);
                break;
        case PROP_RUNTIME_DIR:
                g_value_set_string (value, self->priv->runtime_dir);
                break;
        case PROP_COMMAND:
                g_value_set_string (value, self->priv->command);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
scdm_launch_environment_class_init (ScdmLaunchEnvironmentClass *klass)
{
        GObjectClass    *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = scdm_launch_environment_get_property;
        object_class->set_property = scdm_launch_environment_set_property;
        object_class->finalize = scdm_launch_environment_finalize;

        g_object_class_install_property (object_class,
                                         PROP_VERIFICATION_MODE,
                                         g_param_spec_enum ("verification-mode",
                                                            "verification mode",
                                                            "verification mode",
                                                            GDM_TYPE_SESSION_VERIFICATION_MODE,
                                                            GDM_SESSION_VERIFICATION_MODE_LOGIN,
                                                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_SESSION_TYPE,
                                         g_param_spec_string ("session-type",
                                                              NULL,
                                                              NULL,
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_SESSION_MODE,
                                         g_param_spec_string ("session-mode",
                                                              NULL,
                                                              NULL,
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_X11_DISPLAY_NAME,
                                         g_param_spec_string ("x11-display-name",
                                                              "name",
                                                              "name",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_X11_DISPLAY_SEAT_ID,
                                         g_param_spec_string ("x11-display-seat-id",
                                                              "seat id",
                                                              "seat id",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_X11_DISPLAY_HOSTNAME,
                                         g_param_spec_string ("x11-display-hostname",
                                                              "hostname",
                                                              "hostname",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_X11_DISPLAY_DEVICE,
                                         g_param_spec_string ("x11-display-device",
                                                              "device",
                                                              "device",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_X11_DISPLAY_IS_LOCAL,
                                         g_param_spec_boolean ("x11-display-is-local",
                                                               "is local",
                                                               "is local",
                                                               FALSE,
                                                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_X11_AUTHORITY_FILE,
                                         g_param_spec_string ("x11-authority-file",
                                                              "authority file",
                                                              "authority file",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_USER_NAME,
                                         g_param_spec_string ("user-name",
                                                              "user name",
                                                              "user name",
                                                              GDM_USERNAME,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_RUNTIME_DIR,
                                         g_param_spec_string ("runtime-dir",
                                                              "runtime dir",
                                                              "runtime dir",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (object_class,
                                         PROP_COMMAND,
                                         g_param_spec_string ("command",
                                                              "command",
                                                              "command",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
        signals [OPENED] =
                g_signal_new ("opened",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (ScdmLaunchEnvironmentClass, opened),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        signals [STARTED] =
                g_signal_new ("started",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (ScdmLaunchEnvironmentClass, started),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        signals [STOPPED] =
                g_signal_new ("stopped",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (ScdmLaunchEnvironmentClass, stopped),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        signals [EXITED] =
                g_signal_new ("exited",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (ScdmLaunchEnvironmentClass, exited),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__INT,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_INT);
        signals [DIED] =
                g_signal_new ("died",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (ScdmLaunchEnvironmentClass, died),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__INT,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_INT);

        signals [HOSTNAME_SELECTED] =
                g_signal_new ("hostname-selected",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (ScdmLaunchEnvironmentClass, hostname_selected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
}

static void
scdm_launch_environment_init (ScdmLaunchEnvironment *launch_environment)
{

        launch_environment->priv = scdm_launch_environment_get_instance_private (launch_environment);

        launch_environment->priv->command = NULL;
        launch_environment->priv->session = NULL;
}

static void
scdm_launch_environment_finalize (GObject *object)
{
        ScdmLaunchEnvironment *launch_environment;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GDM_IS_LAUNCH_ENVIRONMENT (object));

        launch_environment = GDM_LAUNCH_ENVIRONMENT (object);

        g_return_if_fail (launch_environment->priv != NULL);

        scdm_launch_environment_stop (launch_environment);

        if (launch_environment->priv->session) {
                g_object_unref (launch_environment->priv->session);
        }

        g_free (launch_environment->priv->command);
        g_free (launch_environment->priv->user_name);
        g_free (launch_environment->priv->runtime_dir);
        g_free (launch_environment->priv->x11_display_name);
        g_free (launch_environment->priv->x11_display_seat_id);
        g_free (launch_environment->priv->x11_display_device);
        g_free (launch_environment->priv->x11_display_hostname);
        g_free (launch_environment->priv->x11_authority_file);
        g_free (launch_environment->priv->session_id);
        g_free (launch_environment->priv->session_type);

        G_OBJECT_CLASS (scdm_launch_environment_parent_class)->finalize (object);
}

static ScdmLaunchEnvironment *
create_gnome_session_environment (const char *session_id,
                                  const char *user_name,
                                  const char *display_name,
                                  const char *seat_id,
                                  const char *session_type,
                                  const char *session_mode,
                                  const char *display_hostname,
                                  gboolean    display_is_local)
{
        gboolean debug = FALSE;
        char *command;
        ScdmLaunchEnvironment *launch_environment;
        char **argv;
        GPtrArray *args;

        scdm_settings_direct_get_boolean (GDM_KEY_DEBUG, &debug);

        args = g_ptr_array_new ();
        g_ptr_array_add (args, "gnome-session");

        g_ptr_array_add (args, "--autostart");
        g_ptr_array_add (args, DATADIR "/scdm/greeter/autostart");

        if (debug) {
                g_ptr_array_add (args, "--debug");
        }

        if (session_id != NULL) {
                g_ptr_array_add (args, " --session");
                g_ptr_array_add (args, (char *) session_id);
        }

        g_ptr_array_add (args, NULL);

        argv = (char **) g_ptr_array_free (args, FALSE);
        command = g_strjoinv (" ", argv);
        g_free (argv);

        launch_environment = g_object_new (GDM_TYPE_LAUNCH_ENVIRONMENT,
                                           "command", command,
                                           "user-name", user_name,
                                           "session-type", session_type,
                                           "session-mode", session_mode,
                                           "x11-display-name", display_name,
                                           "x11-display-seat-id", seat_id,
                                           "x11-display-hostname", display_hostname,
                                           "x11-display-is-local", display_is_local,
                                           "runtime-dir", GDM_SCREENSHOT_DIR,
                                           NULL);

        g_free (command);
        return launch_environment;
}

ScdmLaunchEnvironment *
scdm_create_greeter_launch_environment (const char *display_name,
                                       const char *seat_id,
                                       const char *session_type,
                                       const char *display_hostname,
                                       gboolean    display_is_local)
{
        const char *session_name = NULL;

        return create_gnome_session_environment (session_name,
                                                 GDM_USERNAME,
                                                 display_name,
                                                 seat_id,
                                                 session_type,
                                                 GDM_SESSION_MODE,
                                                 display_hostname,
                                                 display_is_local);
}

ScdmLaunchEnvironment *
scdm_create_initial_setup_launch_environment (const char *display_name,
                                             const char *seat_id,
                                             const char *session_type,
                                             const char *display_hostname,
                                             gboolean    display_is_local)
{
        return create_gnome_session_environment ("gnome-initial-setup",
                                                 INITIAL_SETUP_USERNAME,
                                                 display_name,
                                                 seat_id,
                                                 session_type,
                                                 INITIAL_SETUP_SESSION_MODE,
                                                 display_hostname,
                                                 display_is_local);
}

ScdmLaunchEnvironment *
scdm_create_chooser_launch_environment (const char *display_name,
                                       const char *seat_id,
                                       const char *display_hostname)

{
        ScdmLaunchEnvironment *launch_environment;

        launch_environment = g_object_new (GDM_TYPE_LAUNCH_ENVIRONMENT,
                                           "command", LIBEXECDIR "/scdm-simple-chooser",
                                           "verification-mode", GDM_SESSION_VERIFICATION_MODE_CHOOSER,
                                           "user-name", GDM_USERNAME,
                                           "x11-display-name", display_name,
                                           "x11-display-seat-id", seat_id,
                                           "x11-display-hostname", display_hostname,
                                           "x11-display-is-local", FALSE,
                                           "runtime-dir", GDM_SCREENSHOT_DIR,
                                           NULL);

        return launch_environment;
}

