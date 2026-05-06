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

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gio/gio.h>

#include <systemd/sd-login.h>

#include "scdm-common.h"
#include "scdm-manager.h"
#include "scdm-display-factory.h"
#include "scdm-local-display-factory.h"
#include "scdm-local-display-factory-glue.h"

#include "scdm-settings-keys.h"
#include "scdm-settings-direct.h"
#include "scdm-display-store.h"
#include "scdm-local-display.h"
#include "scdm-legacy-display.h"

#define SCDM_DBUS_PATH                       "/io/github/scarecrow_de/DisplayManager"
#define SCDM_LOCAL_DISPLAY_FACTORY_DBUS_PATH SCDM_DBUS_PATH "/LocalDisplayFactory"
#define SCDM_MANAGER_DBUS_NAME               "io.github.scarecrow_de.DisplayManager.LocalDisplayFactory"

#define MAX_DISPLAY_FAILURES 5
#define WAIT_TO_FINISH_TIMEOUT 10 /* seconds */

struct _ScdmLocalDisplayFactory
{
        ScdmDisplayFactory              parent;

        ScdmDBusLocalDisplayFactory *skeleton;
        GDBusConnection *connection;
        GHashTable      *used_display_numbers;

        /* FIXME: this needs to be per seat? */
        guint            num_failures;

        guint            seat_new_id;
        guint            seat_removed_id;

#if defined(ENABLE_USER_DISPLAY_SERVER)
        unsigned int     active_vt;
        guint            active_vt_watch_id;
        guint            wait_to_finish_timeout_id;
#endif
};

enum {
        PROP_0,
};

static void     scdm_local_display_factory_class_init    (ScdmLocalDisplayFactoryClass *klass);
static void     scdm_local_display_factory_init          (ScdmLocalDisplayFactory      *factory);
static void     scdm_local_display_factory_finalize      (GObject                     *object);

static ScdmDisplay *create_display                       (ScdmLocalDisplayFactory      *factory,
                                                         const char                  *seat_id,
                                                         const char                  *session_type,
                                                         gboolean                    initial_display);

static void     on_display_status_changed               (ScdmDisplay                  *display,
                                                         GParamSpec                  *arg1,
                                                         ScdmLocalDisplayFactory      *factory);

static gboolean scdm_local_display_factory_sync_seats    (ScdmLocalDisplayFactory *factory);
static gpointer local_display_factory_object = NULL;
static gboolean lookup_by_session_id (const char *id,
                                      ScdmDisplay *display,
                                      gpointer    user_data);

G_DEFINE_TYPE (ScdmLocalDisplayFactory, scdm_local_display_factory, SCDM_TYPE_DISPLAY_FACTORY)

GQuark
scdm_local_display_factory_error_quark (void)
{
        static GQuark ret = 0;
        if (ret == 0) {
                ret = g_quark_from_static_string ("scdm_local_display_factory_error");
        }

        return ret;
}

static void
listify_hash (gpointer    key,
              ScdmDisplay *display,
              GList     **list)
{
        *list = g_list_prepend (*list, key);
}

static int
sort_nums (gpointer a,
           gpointer b)
{
        guint32 num_a;
        guint32 num_b;

        num_a = GPOINTER_TO_UINT (a);
        num_b = GPOINTER_TO_UINT (b);

        if (num_a > num_b) {
                return 1;
        } else if (num_a < num_b) {
                return -1;
        } else {
                return 0;
        }
}

static guint32
take_next_display_number (ScdmLocalDisplayFactory *factory)
{
        GList  *list;
        GList  *l;
        guint32 ret;

        ret = 0;
        list = NULL;

        g_hash_table_foreach (factory->used_display_numbers, (GHFunc)listify_hash, &list);
        if (list == NULL) {
                goto out;
        }

        /* sort low to high */
        list = g_list_sort (list, (GCompareFunc)sort_nums);

        g_debug ("ScdmLocalDisplayFactory: Found the following X displays:");
        for (l = list; l != NULL; l = l->next) {
                g_debug ("ScdmLocalDisplayFactory: %u", GPOINTER_TO_UINT (l->data));
        }

        for (l = list; l != NULL; l = l->next) {
                guint32 num;
                num = GPOINTER_TO_UINT (l->data);

                /* always fill zero */
                if (l->prev == NULL && num != 0) {
                        ret = 0;
                        break;
                }
                /* now find the first hole */
                if (l->next == NULL || GPOINTER_TO_UINT (l->next->data) != (num + 1)) {
                        ret = num + 1;
                        break;
                }
        }
 out:

        /* now reserve this number */
        g_debug ("ScdmLocalDisplayFactory: Reserving X display: %u", ret);
        g_hash_table_insert (factory->used_display_numbers, GUINT_TO_POINTER (ret), NULL);

        return ret;
}

static void
on_display_disposed (ScdmLocalDisplayFactory *factory,
                     ScdmDisplay             *display)
{
        g_debug ("ScdmLocalDisplayFactory: Display %p disposed", display);
}

static void
store_display (ScdmLocalDisplayFactory *factory,
               ScdmDisplay             *display)
{
        ScdmDisplayStore *store;

        store = scdm_display_factory_get_display_store (SCDM_DISPLAY_FACTORY (factory));
        scdm_display_store_add (store, display);
}

static gboolean
scdm_local_display_factory_use_wayland (void)
{
#ifdef ENABLE_WAYLAND_SUPPORT
        gboolean wayland_enabled = FALSE;
        if (scdm_settings_direct_get_boolean (SCDM_KEY_WAYLAND_ENABLE, &wayland_enabled)) {
                if (wayland_enabled && g_file_test ("/usr/bin/Xwayland", G_FILE_TEST_IS_EXECUTABLE) )
                        return TRUE;
        }
#endif
        return FALSE;
}

/*
  Example:
  dbus-send --system --dest=io.github.scarecrow_de.DisplayManager \
  --type=method_call --print-reply --reply-timeout=2000 \
  /io/github/scarecrow_de/DisplayManager/Manager \
  io.github.scarecrow_de.DisplayManager.Manager.GetDisplays
*/
gboolean
scdm_local_display_factory_create_transient_display (ScdmLocalDisplayFactory *factory,
                                                    char                  **id,
                                                    GError                **error)
{
        gboolean         ret;
        ScdmDisplay      *display = NULL;
        gboolean         is_initial = FALSE;

        g_return_val_if_fail (SCDM_IS_LOCAL_DISPLAY_FACTORY (factory), FALSE);

        ret = FALSE;

        g_debug ("ScdmLocalDisplayFactory: Creating transient display");

#ifdef ENABLE_USER_DISPLAY_SERVER
        display = scdm_local_display_new ();
        if (scdm_local_display_factory_use_wayland ())
                g_object_set (G_OBJECT (display), "session-type", "wayland", NULL);
        is_initial = TRUE;
#else
        if (display == NULL) {
                guint32 num;

                num = take_next_display_number (factory);

                display = scdm_legacy_display_new (num);
        }
#endif

        g_object_set (display,
                      "seat-id", "seat0",
                      "allow-timed-login", FALSE,
                      "is-initial", is_initial,
                      NULL);

        store_display (factory, display);

        if (! scdm_display_manage (display)) {
                display = NULL;
                goto out;
        }

        if (! scdm_display_get_id (display, id, NULL)) {
                display = NULL;
                goto out;
        }

        ret = TRUE;
 out:
        /* ref either held by store or not at all */
        g_object_unref (display);

        return ret;
}

static void
finish_display_on_seat_if_waiting (ScdmDisplayStore *display_store,
                                   ScdmDisplay      *display,
                                   const char      *seat_id)
{
        if (scdm_display_get_status (display) != SCDM_DISPLAY_WAITING_TO_FINISH)
                return;

        g_debug ("ScdmLocalDisplayFactory: finish background display\n");
        scdm_display_stop_greeter_session (display);
        scdm_display_unmanage (display);
        scdm_display_finish (display);
}

static void
finish_waiting_displays_on_seat (ScdmLocalDisplayFactory *factory,
                                 const char             *seat_id)
{
        ScdmDisplayStore *store;

        store = scdm_display_factory_get_display_store (SCDM_DISPLAY_FACTORY (factory));

        scdm_display_store_foreach (store,
                                   (ScdmDisplayStoreFunc) finish_display_on_seat_if_waiting,
                                   (gpointer)
                                   seat_id);
}

static gboolean
on_finish_waiting_for_seat0_displays_timeout (ScdmLocalDisplayFactory *factory)
{
        g_debug ("ScdmLocalDisplayFactory: timeout following VT switch to registered session complete, looking for any background displays to kill");
        finish_waiting_displays_on_seat (factory, "seat0");
        return G_SOURCE_REMOVE;
}

static void
on_session_registered_cb (GObject *gobject,
                          GParamSpec *pspec,
                          gpointer user_data)
{
        ScdmDisplay *display = SCDM_DISPLAY (gobject);
        ScdmLocalDisplayFactory *factory = SCDM_LOCAL_DISPLAY_FACTORY (user_data);
        gboolean registered;

        g_object_get (display, "session-registered", &registered, NULL);

        if (!registered)
                return;

        g_debug ("ScdmLocalDisplayFactory: session registered on display, looking for any background displays to kill");

        finish_waiting_displays_on_seat (factory, "seat0");
}

static void
on_display_status_changed (ScdmDisplay             *display,
                           GParamSpec             *arg1,
                           ScdmLocalDisplayFactory *factory)
{
        int              status;
        int              num;
        char            *seat_id = NULL;
        char            *session_type = NULL;
        char            *session_class = NULL;
        gboolean         is_initial = TRUE;
        gboolean         is_local = TRUE;

        num = -1;
        scdm_display_get_x11_display_number (display, &num, NULL);

        g_object_get (display,
                      "seat-id", &seat_id,
                      "is-initial", &is_initial,
                      "is-local", &is_local,
                      "session-type", &session_type,
                      "session-class", &session_class,
                      NULL);

        status = scdm_display_get_status (display);

        g_debug ("ScdmLocalDisplayFactory: display status changed: %d", status);
        switch (status) {
        case SCDM_DISPLAY_FINISHED:
                /* remove the display number from factory->used_display_numbers
                   so that it may be reused */
                if (num != -1) {
                        g_hash_table_remove (factory->used_display_numbers, GUINT_TO_POINTER (num));
                }
                scdm_display_factory_queue_purge_displays (SCDM_DISPLAY_FACTORY (factory));

                /* if this is a local display, do a full resync.  Only
                 * seats without displays will get created anyway.  This
                 * ensures we get a new login screen when the user logs out,
                 * if there isn't one.
                 */
                if (is_local && g_strcmp0 (session_class, "greeter") != 0) {
                        /* reset num failures */
                        factory->num_failures = 0;

                        scdm_local_display_factory_sync_seats (factory);
                }
                break;
        case SCDM_DISPLAY_FAILED:
                /* leave the display number in factory->used_display_numbers
                   so that it doesn't get reused */
                scdm_display_factory_queue_purge_displays (SCDM_DISPLAY_FACTORY (factory));

                /* Create a new equivalent display if it was static */
                if (is_local) {

                        factory->num_failures++;

                        if (factory->num_failures > MAX_DISPLAY_FAILURES) {
                                /* oh shit */
                                g_warning ("ScdmLocalDisplayFactory: maximum number of X display failures reached: check X server log for errors");
                        } else {
#ifdef ENABLE_WAYLAND_SUPPORT
                                if (g_strcmp0 (session_type, "wayland") == 0) {
                                        g_free (session_type);
                                        session_type = NULL;
                                }

#endif
                                create_display (factory, seat_id, session_type, is_initial);
                        }
                }
                break;
        case SCDM_DISPLAY_UNMANAGED:
                break;
        case SCDM_DISPLAY_PREPARED:
                break;
        case SCDM_DISPLAY_MANAGED:
#if defined(ENABLE_USER_DISPLAY_SERVER)
                g_signal_connect_object (display,
                                         "notify::session-registered",
                                         G_CALLBACK (on_session_registered_cb),
                                         factory,
                                         0);
#endif
                break;
        case SCDM_DISPLAY_WAITING_TO_FINISH:
                break;
        default:
                g_assert_not_reached ();
                break;
        }

        g_free (seat_id);
        g_free (session_type);
        g_free (session_class);
}

static gboolean
lookup_by_seat_id (const char *id,
                   ScdmDisplay *display,
                   gpointer    user_data)
{
        const char *looking_for = user_data;
        char *current;
        gboolean res;

        g_object_get (G_OBJECT (display), "seat-id", &current, NULL);

        res = g_strcmp0 (current, looking_for) == 0;

        g_free(current);

        return res;
}

static gboolean
lookup_prepared_display_by_seat_id (const char *id,
                                    ScdmDisplay *display,
                                    gpointer    user_data)
{
        int status;

        status = scdm_display_get_status (display);

        if (status != SCDM_DISPLAY_PREPARED)
                return FALSE;

        return lookup_by_seat_id (id, display, user_data);
}

static ScdmDisplay *
create_display (ScdmLocalDisplayFactory *factory,
                const char             *seat_id,
                const char             *session_type,
                gboolean                initial)
{
        ScdmDisplayStore *store;
        ScdmDisplay      *display = NULL;
        g_autofree char *login_session_id = NULL;

        g_debug ("ScdmLocalDisplayFactory: %s login display for seat %s requested",
                 session_type? : "X11", seat_id);
        store = scdm_display_factory_get_display_store (SCDM_DISPLAY_FACTORY (factory));

        if (sd_seat_can_multi_session (seat_id))
                display = scdm_display_store_find (store, lookup_prepared_display_by_seat_id, (gpointer) seat_id);
        else
                display = scdm_display_store_find (store, lookup_by_seat_id, (gpointer) seat_id);

        /* Ensure we don't create the same display more than once */
        if (display != NULL) {
                g_debug ("ScdmLocalDisplayFactory: display already created");
                return NULL;
        }

        /* If we already have a login window, switch to it */
        if (scdm_get_login_window_session_id (seat_id, &login_session_id)) {
                ScdmDisplay *display;

                display = scdm_display_store_find (store,
                                                  lookup_by_session_id,
                                                  (gpointer) login_session_id);
                if (display != NULL &&
                    (scdm_display_get_status (display) == SCDM_DISPLAY_MANAGED ||
                     scdm_display_get_status (display) == SCDM_DISPLAY_WAITING_TO_FINISH)) {
                        g_object_set (G_OBJECT (display), "status", SCDM_DISPLAY_MANAGED, NULL);
                        g_debug ("ScdmLocalDisplayFactory: session %s found, activating.",
                                 login_session_id);
                        scdm_activate_session_by_id (factory->connection, seat_id, login_session_id);
                        return NULL;
                }
        }

        g_debug ("ScdmLocalDisplayFactory: Adding display on seat %s", seat_id);

#ifdef ENABLE_USER_DISPLAY_SERVER
        if (g_strcmp0 (seat_id, "seat0") == 0) {
                display = scdm_local_display_new ();
                if (session_type != NULL) {
                        g_object_set (G_OBJECT (display), "session-type", session_type, NULL);
                }
        }
#endif

        if (display == NULL) {
                guint32 num;

                num = take_next_display_number (factory);

                display = scdm_legacy_display_new (num);
        }

        g_object_set (display, "seat-id", seat_id, NULL);
        g_object_set (display, "is-initial", initial, NULL);

        store_display (factory, display);

        /* let store own the ref */
        g_object_unref (display);

        if (! scdm_display_manage (display)) {
                scdm_display_unmanage (display);
        }

        return display;
}

static void
delete_display (ScdmLocalDisplayFactory *factory,
                const char             *seat_id) {

        ScdmDisplayStore *store;

        g_debug ("ScdmLocalDisplayFactory: Removing used_display_numbers on seat %s", seat_id);

        store = scdm_display_factory_get_display_store (SCDM_DISPLAY_FACTORY (factory));
        scdm_display_store_foreach_remove (store, lookup_by_seat_id, (gpointer) seat_id);
}

static gboolean
scdm_local_display_factory_sync_seats (ScdmLocalDisplayFactory *factory)
{
        GError *error = NULL;
        GVariant *result;
        GVariant *array;
        GVariantIter iter;
        const char *seat;

        g_debug ("ScdmLocalDisplayFactory: enumerating seats from logind");
        result = g_dbus_connection_call_sync (factory->connection,
                                              "org.freedesktop.login1",
                                              "/org/freedesktop/login1",
                                              "org.freedesktop.login1.Manager",
                                              "ListSeats",
                                              NULL,
                                              G_VARIANT_TYPE ("(a(so))"),
                                              G_DBUS_CALL_FLAGS_NONE,
                                              -1,
                                              NULL, &error);

        if (!result) {
                g_warning ("ScdmLocalDisplayFactory: Failed to issue method call: %s", error->message);
                g_clear_error (&error);
                return FALSE;
        }

        array = g_variant_get_child_value (result, 0);
        g_variant_iter_init (&iter, array);

        while (g_variant_iter_loop (&iter, "(&so)", &seat, NULL)) {
                gboolean is_initial;
                const char *session_type = NULL;

                if (g_strcmp0 (seat, "seat0") == 0) {
                        is_initial = TRUE;
                        if (scdm_local_display_factory_use_wayland ())
                                session_type = "wayland";
                } else {
                        is_initial = FALSE;
                }

                create_display (factory, seat, session_type, is_initial);
        }

        g_variant_unref (result);
        g_variant_unref (array);
        return TRUE;
}

static void
on_seat_new (GDBusConnection *connection,
             const gchar     *sender_name,
             const gchar     *object_path,
             const gchar     *interface_name,
             const gchar     *signal_name,
             GVariant        *parameters,
             gpointer         user_data)
{
        const char *seat;

        g_variant_get (parameters, "(&s&o)", &seat, NULL);
        create_display (SCDM_LOCAL_DISPLAY_FACTORY (user_data), seat, NULL, FALSE);
}

static void
on_seat_removed (GDBusConnection *connection,
                 const gchar     *sender_name,
                 const gchar     *object_path,
                 const gchar     *interface_name,
                 const gchar     *signal_name,
                 GVariant        *parameters,
                 gpointer         user_data)
{
        const char *seat;

        g_variant_get (parameters, "(&s&o)", &seat, NULL);
        delete_display (SCDM_LOCAL_DISPLAY_FACTORY (user_data), seat);
}

static gboolean
lookup_by_session_id (const char *id,
                      ScdmDisplay *display,
                      gpointer    user_data)
{
        const char *looking_for = user_data;
        const char *current;

        current = scdm_display_get_session_id (display);
        return g_strcmp0 (current, looking_for) == 0;
}

static gboolean
lookup_by_tty (const char *id,
              ScdmDisplay *display,
              gpointer    user_data)
{
        const char *tty_to_find = user_data;
        g_autofree char *tty_to_check = NULL;
        const char *session_id;
        int ret;

        session_id = scdm_display_get_session_id (display);

        if (!session_id)
                return FALSE;

        ret = sd_session_get_tty (session_id, &tty_to_check);

        if (ret != 0)
                return FALSE;

        return g_strcmp0 (tty_to_check, tty_to_find) == 0;
}

#if defined(ENABLE_USER_DISPLAY_SERVER)
static void
maybe_stop_greeter_in_background (ScdmLocalDisplayFactory *factory,
                                  ScdmDisplay             *display)
{
        gboolean doing_initial_setup = FALSE;

        if (scdm_display_get_status (display) != SCDM_DISPLAY_MANAGED) {
                g_debug ("ScdmLocalDisplayFactory: login window not in managed state, so ignoring");
                return;
        }

        g_object_get (G_OBJECT (display),
                      "doing-initial-setup", &doing_initial_setup,
                      NULL);

        /* we don't ever stop initial-setup implicitly */
        if (doing_initial_setup) {
                g_debug ("ScdmLocalDisplayFactory: login window is performing initial-setup, so ignoring");
                return;
        }

        g_debug ("ScdmLocalDisplayFactory: killing login window once its unused");

        g_object_set (G_OBJECT (display), "status", SCDM_DISPLAY_WAITING_TO_FINISH, NULL);
}

static gboolean
on_vt_changed (GIOChannel    *source,
               GIOCondition   condition,
               ScdmLocalDisplayFactory *factory)
{
        ScdmDisplayStore *store;
        GIOStatus status;
        g_autofree char *tty_of_active_vt = NULL;
        g_autofree char *login_session_id = NULL;
        g_autofree char *active_session_id = NULL;
        unsigned int previous_vt, new_vt, login_window_vt = 0;
        const char *session_type = NULL;
        int ret, n_returned;

        g_debug ("ScdmLocalDisplayFactory: received VT change event");
        g_io_channel_seek_position (source, 0, G_SEEK_SET, NULL);

        if (condition & G_IO_PRI) {
                g_autoptr (GError) error = NULL;
                status = g_io_channel_read_line (source, &tty_of_active_vt, NULL, NULL, &error);

                if (error != NULL) {
                        g_warning ("could not read active VT from kernel: %s", error->message);
                }
                switch (status) {
                        case G_IO_STATUS_ERROR:
                            return G_SOURCE_REMOVE;
                        case G_IO_STATUS_EOF:
                            return G_SOURCE_REMOVE;
                        case G_IO_STATUS_AGAIN:
                            return G_SOURCE_CONTINUE;
                        case G_IO_STATUS_NORMAL:
                            break;
                }
        }

        if ((condition & G_IO_ERR) || (condition & G_IO_HUP)) {
                g_debug ("ScdmLocalDisplayFactory: kernel hung up active vt watch");
                return G_SOURCE_REMOVE;
        }

        if (tty_of_active_vt == NULL) {
                g_debug ("ScdmLocalDisplayFactory: unable to read active VT from kernel");
                return G_SOURCE_CONTINUE;
        }

        g_strchomp (tty_of_active_vt);

        errno = 0;
        n_returned = sscanf (tty_of_active_vt, "tty%u", &new_vt);

        if (n_returned != 1 || errno != 0) {
                g_critical ("ScdmLocalDisplayFactory: Couldn't read active VT (got '%s')",
                            tty_of_active_vt);
                return G_SOURCE_CONTINUE;
        }

        /* don't do anything if we're on the same VT we were before */
        if (new_vt == factory->active_vt) {
                g_debug ("ScdmLocalDisplayFactory: VT changed to the same VT, ignoring");
                return G_SOURCE_CONTINUE;
        }

        previous_vt = factory->active_vt;
        factory->active_vt = new_vt;

        /* don't do anything at start up */
        if (previous_vt == 0) {
                g_debug ("ScdmLocalDisplayFactory: VT is %u at startup",
                         factory->active_vt);
                return G_SOURCE_CONTINUE;
        }

        g_debug ("ScdmLocalDisplayFactory: VT changed from %u to %u",
                 previous_vt, factory->active_vt);

        store = scdm_display_factory_get_display_store (SCDM_DISPLAY_FACTORY (factory));

        /* if the old VT was running a wayland login screen kill it
         */
        if (scdm_get_login_window_session_id ("seat0", &login_session_id)) {
                ret = sd_session_get_vt (login_session_id, &login_window_vt);
                if (ret == 0 && login_window_vt != 0) {
                        g_debug ("ScdmLocalDisplayFactory: VT of login window is %u", login_window_vt);
                        if (login_window_vt == previous_vt) {
                                ScdmDisplay *display;

                                g_debug ("ScdmLocalDisplayFactory: VT switched from login window");

                                display = scdm_display_store_find (store,
                                                                  lookup_by_session_id,
                                                                  (gpointer) login_session_id);
                                if (display != NULL)
                                        maybe_stop_greeter_in_background (factory, display);
                        } else {
                                g_debug ("ScdmLocalDisplayFactory: VT not switched from login window");
                        }
                }
        }

        /* If we jumped to a registered user session, we can kill
         * the login screen (after a suitable timeout to avoid flicker)
         */
        if (factory->active_vt != login_window_vt) {
                ScdmDisplay *display;

                display = scdm_display_store_find (store,
                                                  lookup_by_tty,
                                                  (gpointer) tty_of_active_vt);

                if (display != NULL) {
                        gboolean registered;

                        g_object_get (display, "session-registered", &registered, NULL);

                        if (registered) {
                                g_debug ("ScdmLocalDisplayFactory: switched to registered user session, so reaping login screen in %d seconds",
                                         WAIT_TO_FINISH_TIMEOUT);
                                if (factory->wait_to_finish_timeout_id != 0) {
                                         g_debug ("ScdmLocalDisplayFactory: deferring previous login screen clean up operation");
                                         g_source_remove (factory->wait_to_finish_timeout_id);
                                }

                                factory->wait_to_finish_timeout_id = g_timeout_add_seconds (WAIT_TO_FINISH_TIMEOUT,
                                                                                            (GSourceFunc)
                                                                                            on_finish_waiting_for_seat0_displays_timeout,
                                                                                            factory);
                        }
                }
        }

        /* if user jumped back to initial vt and it's empty put a login screen
         * on it (unless a login screen is already running elsewhere, then
         * jump to that login screen)
         */
        if (factory->active_vt != SCDM_INITIAL_VT) {
                g_debug ("ScdmLocalDisplayFactory: active VT is not initial VT, so ignoring");
                return G_SOURCE_CONTINUE;
        }

        if (scdm_local_display_factory_use_wayland ())
                session_type = "wayland";

        g_debug ("ScdmLocalDisplayFactory: creating new display on seat0 because of VT change");

        create_display (factory, "seat0", session_type, TRUE);

        return G_SOURCE_CONTINUE;
}
#endif

static void
scdm_local_display_factory_start_monitor (ScdmLocalDisplayFactory *factory)
{
        g_autoptr (GIOChannel) io_channel = NULL;

        factory->seat_new_id = g_dbus_connection_signal_subscribe (factory->connection,
                                                                         "org.freedesktop.login1",
                                                                         "org.freedesktop.login1.Manager",
                                                                         "SeatNew",
                                                                         "/org/freedesktop/login1",
                                                                         NULL,
                                                                         G_DBUS_SIGNAL_FLAGS_NONE,
                                                                         on_seat_new,
                                                                         g_object_ref (factory),
                                                                         g_object_unref);
        factory->seat_removed_id = g_dbus_connection_signal_subscribe (factory->connection,
                                                                             "org.freedesktop.login1",
                                                                             "org.freedesktop.login1.Manager",
                                                                             "SeatRemoved",
                                                                             "/org/freedesktop/login1",
                                                                             NULL,
                                                                             G_DBUS_SIGNAL_FLAGS_NONE,
                                                                             on_seat_removed,
                                                                             g_object_ref (factory),
                                                                             g_object_unref);

#if defined(ENABLE_USER_DISPLAY_SERVER)
        io_channel = g_io_channel_new_file ("/sys/class/tty/tty0/active", "r", NULL);

        if (io_channel != NULL) {
                factory->active_vt_watch_id =
                        g_io_add_watch (io_channel,
                                        G_IO_PRI,
                                        (GIOFunc)
                                        on_vt_changed,
                                        factory);
        }
#endif
}

static void
scdm_local_display_factory_stop_monitor (ScdmLocalDisplayFactory *factory)
{
        if (factory->seat_new_id) {
                g_dbus_connection_signal_unsubscribe (factory->connection,
                                                      factory->seat_new_id);
                factory->seat_new_id = 0;
        }
        if (factory->seat_removed_id) {
                g_dbus_connection_signal_unsubscribe (factory->connection,
                                                      factory->seat_removed_id);
                factory->seat_removed_id = 0;
        }
#if defined(ENABLE_USER_DISPLAY_SERVER)
        if (factory->active_vt_watch_id) {
                g_source_remove (factory->active_vt_watch_id);
                factory->active_vt_watch_id = 0;
        }
        if (factory->wait_to_finish_timeout_id != 0) {
                g_source_remove (factory->wait_to_finish_timeout_id);
                factory->wait_to_finish_timeout_id = 0;
        }
#endif
}

static void
on_display_added (ScdmDisplayStore        *display_store,
                  const char             *id,
                  ScdmLocalDisplayFactory *factory)
{
        ScdmDisplay *display;

        display = scdm_display_store_lookup (display_store, id);

        if (display != NULL) {
                g_signal_connect_object (display, "notify::status",
                                         G_CALLBACK (on_display_status_changed),
                                         factory,
                                         0);

                g_object_weak_ref (G_OBJECT (display), (GWeakNotify)on_display_disposed, factory);
        }
}

static void
on_display_removed (ScdmDisplayStore        *display_store,
                    ScdmDisplay             *display,
                    ScdmLocalDisplayFactory *factory)
{
        g_signal_handlers_disconnect_by_func (display, G_CALLBACK (on_display_status_changed), factory);
        g_object_weak_unref (G_OBJECT (display), (GWeakNotify)on_display_disposed, factory);
}

static gboolean
scdm_local_display_factory_start (ScdmDisplayFactory *base_factory)
{
        ScdmLocalDisplayFactory *factory = SCDM_LOCAL_DISPLAY_FACTORY (base_factory);
        ScdmDisplayStore *store;

        g_return_val_if_fail (SCDM_IS_LOCAL_DISPLAY_FACTORY (factory), FALSE);

        store = scdm_display_factory_get_display_store (SCDM_DISPLAY_FACTORY (factory));

        g_signal_connect_object (G_OBJECT (store),
                                 "display-added",
                                 G_CALLBACK (on_display_added),
                                 factory,
                                 0);

        g_signal_connect_object (G_OBJECT (store),
                                 "display-removed",
                                 G_CALLBACK (on_display_removed),
                                 factory,
                                 0);

        scdm_local_display_factory_start_monitor (factory);
        return scdm_local_display_factory_sync_seats (factory);
}

static gboolean
scdm_local_display_factory_stop (ScdmDisplayFactory *base_factory)
{
        ScdmLocalDisplayFactory *factory = SCDM_LOCAL_DISPLAY_FACTORY (base_factory);
        ScdmDisplayStore *store;

        g_return_val_if_fail (SCDM_IS_LOCAL_DISPLAY_FACTORY (factory), FALSE);

        scdm_local_display_factory_stop_monitor (factory);

        store = scdm_display_factory_get_display_store (SCDM_DISPLAY_FACTORY (factory));

        g_signal_handlers_disconnect_by_func (G_OBJECT (store),
                                              G_CALLBACK (on_display_added),
                                              factory);
        g_signal_handlers_disconnect_by_func (G_OBJECT (store),
                                              G_CALLBACK (on_display_removed),
                                              factory);

        return TRUE;
}

static void
scdm_local_display_factory_set_property (GObject       *object,
                                        guint          prop_id,
                                        const GValue  *value,
                                        GParamSpec    *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
scdm_local_display_factory_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static gboolean
handle_create_transient_display (ScdmDBusLocalDisplayFactory *skeleton,
                                 GDBusMethodInvocation      *invocation,
                                 ScdmLocalDisplayFactory     *factory)
{
        GError *error = NULL;
        gboolean created;
        char *id = NULL;

        created = scdm_local_display_factory_create_transient_display (factory,
                                                                      &id,
                                                                      &error);
        if (!created) {
                g_dbus_method_invocation_return_gerror (invocation, error);
        } else {
                scdm_dbus_local_display_factory_complete_create_transient_display (skeleton, invocation, id);
        }

        g_free (id);
        return TRUE;
}

static gboolean
register_factory (ScdmLocalDisplayFactory *factory)
{
        GError *error = NULL;

        error = NULL;
        factory->connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
        if (factory->connection == NULL) {
                g_critical ("error getting system bus: %s", error->message);
                g_error_free (error);
                exit (EXIT_FAILURE);
        }

        factory->skeleton = SCDM_DBUS_LOCAL_DISPLAY_FACTORY (scdm_dbus_local_display_factory_skeleton_new ());

        g_signal_connect (factory->skeleton,
                          "handle-create-transient-display",
                          G_CALLBACK (handle_create_transient_display),
                          factory);

        if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (factory->skeleton),
                                               factory->connection,
                                               SCDM_LOCAL_DISPLAY_FACTORY_DBUS_PATH,
                                               &error)) {
                g_critical ("error exporting LocalDisplayFactory object: %s", error->message);
                g_error_free (error);
                exit (EXIT_FAILURE);
        }

        return TRUE;
}

static GObject *
scdm_local_display_factory_constructor (GType                  type,
                                       guint                  n_construct_properties,
                                       GObjectConstructParam *construct_properties)
{
        ScdmLocalDisplayFactory      *factory;
        gboolean                     res;

        factory = SCDM_LOCAL_DISPLAY_FACTORY (G_OBJECT_CLASS (scdm_local_display_factory_parent_class)->constructor (type,
                                                                                                                   n_construct_properties,
                                                                                                                   construct_properties));

        res = register_factory (factory);
        if (! res) {
                g_warning ("Unable to register local display factory with system bus");
        }

        return G_OBJECT (factory);
}

static void
scdm_local_display_factory_class_init (ScdmLocalDisplayFactoryClass *klass)
{
        GObjectClass           *object_class = G_OBJECT_CLASS (klass);
        ScdmDisplayFactoryClass *factory_class = SCDM_DISPLAY_FACTORY_CLASS (klass);

        object_class->get_property = scdm_local_display_factory_get_property;
        object_class->set_property = scdm_local_display_factory_set_property;
        object_class->finalize = scdm_local_display_factory_finalize;
        object_class->constructor = scdm_local_display_factory_constructor;

        factory_class->start = scdm_local_display_factory_start;
        factory_class->stop = scdm_local_display_factory_stop;
}

static void
scdm_local_display_factory_init (ScdmLocalDisplayFactory *factory)
{
        factory->used_display_numbers = g_hash_table_new (NULL, NULL);
}

static void
scdm_local_display_factory_finalize (GObject *object)
{
        ScdmLocalDisplayFactory *factory;

        g_return_if_fail (object != NULL);
        g_return_if_fail (SCDM_IS_LOCAL_DISPLAY_FACTORY (object));

        factory = SCDM_LOCAL_DISPLAY_FACTORY (object);

        g_return_if_fail (factory != NULL);

        g_clear_object (&factory->connection);
        g_clear_object (&factory->skeleton);

        g_hash_table_destroy (factory->used_display_numbers);

        scdm_local_display_factory_stop_monitor (factory);

        G_OBJECT_CLASS (scdm_local_display_factory_parent_class)->finalize (object);
}

ScdmLocalDisplayFactory *
scdm_local_display_factory_new (ScdmDisplayStore *store)
{
        if (local_display_factory_object != NULL) {
                g_object_ref (local_display_factory_object);
        } else {
                local_display_factory_object = g_object_new (SCDM_TYPE_LOCAL_DISPLAY_FACTORY,
                                                             "display-store", store,
                                                             NULL);
                g_object_add_weak_pointer (local_display_factory_object,
                                           (gpointer *) &local_display_factory_object);
        }

        return SCDM_LOCAL_DISPLAY_FACTORY (local_display_factory_object);
}
