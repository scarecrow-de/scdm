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


#ifndef __SCDM_DISPLAY_H
#define __SCDM_DISPLAY_H

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define SCDM_TYPE_DISPLAY (scdm_display_get_type ())
G_DECLARE_DERIVABLE_TYPE (ScdmDisplay, scdm_display, GDM, DISPLAY, GObject)

typedef enum {
        SCDM_DISPLAY_UNMANAGED = 0,
        SCDM_DISPLAY_PREPARED,
        SCDM_DISPLAY_MANAGED,
        SCDM_DISPLAY_WAITING_TO_FINISH,
        SCDM_DISPLAY_FINISHED,
        SCDM_DISPLAY_FAILED,
} ScdmDisplayStatus;

struct _ScdmDisplayClass
{
        GObjectClass   parent_class;

        /* methods */
        gboolean (*prepare) (ScdmDisplay *display);
        void     (*manage)  (ScdmDisplay *self);
};

typedef enum
{
         SCDM_DISPLAY_ERROR_GENERAL,
         SCDM_DISPLAY_ERROR_GETTING_USER_INFO,
         SCDM_DISPLAY_ERROR_GETTING_SESSION_INFO,
} ScdmDisplayError;

#define SCDM_DISPLAY_ERROR scdm_display_error_quark ()

GQuark              scdm_display_error_quark                    (void);

int                 scdm_display_get_status                     (ScdmDisplay *display);
time_t              scdm_display_get_creation_time              (ScdmDisplay *display);
char *              scdm_display_open_session_sync              (ScdmDisplay    *display,
                                                                GPid           pid_of_caller,
                                                                uid_t          uid_of_caller,
                                                                GCancellable  *cancellable,
                                                                GError       **error);

char *              scdm_display_open_reauthentication_channel_sync        (ScdmDisplay    *display,
                                                                           const char    *username,
                                                                           GPid           pid_of_caller,
                                                                           uid_t          uid_of_caller,
                                                                           GCancellable  *cancellable,
                                                                           GError       **error);
const char *        scdm_display_get_session_id                 (ScdmDisplay *display);
gboolean            scdm_display_create_authority               (ScdmDisplay *display);
gboolean            scdm_display_prepare                        (ScdmDisplay *display);
gboolean            scdm_display_manage                         (ScdmDisplay *display);
gboolean            scdm_display_finish                         (ScdmDisplay *display);
gboolean            scdm_display_unmanage                       (ScdmDisplay *display);

GDBusObjectSkeleton *scdm_display_get_object_skeleton           (ScdmDisplay *display);

/* exported to bus */
gboolean            scdm_display_get_id                         (ScdmDisplay *display,
                                                                char      **id,
                                                                GError    **error);
gboolean            scdm_display_get_remote_hostname            (ScdmDisplay *display,
                                                                char      **hostname,
                                                                GError    **error);
gboolean            scdm_display_get_x11_display_number         (ScdmDisplay *display,
                                                                int        *number,
                                                                GError    **error);
gboolean            scdm_display_get_x11_display_name           (ScdmDisplay *display,
                                                                char      **x11_display,
                                                                GError    **error);
gboolean            scdm_display_get_seat_id                    (ScdmDisplay *display,
                                                                char      **seat_id,
                                                                GError    **error);
gboolean            scdm_display_is_local                       (ScdmDisplay *display,
                                                                gboolean   *local,
                                                                GError    **error);
gboolean            scdm_display_is_initial                     (ScdmDisplay  *display,
                                                                gboolean    *initial,
                                                                GError     **error);

gboolean            scdm_display_get_x11_cookie                 (ScdmDisplay  *display,
                                                                const char **x11_cookie,
                                                                gsize       *x11_cookie_size,
                                                                GError     **error);
gboolean            scdm_display_get_x11_authority_file         (ScdmDisplay *display,
                                                                char      **filename,
                                                                GError    **error);
gboolean            scdm_display_add_user_authorization         (ScdmDisplay *display,
                                                                const char *username,
                                                                char      **filename,
                                                                GError    **error);
gboolean            scdm_display_remove_user_authorization      (ScdmDisplay *display,
                                                                const char *username,
                                                                GError    **error);
void                scdm_display_start_greeter_session          (ScdmDisplay  *display);
void                scdm_display_stop_greeter_session           (ScdmDisplay  *display);

gboolean            scdm_display_connect                        (ScdmDisplay *self);

G_END_DECLS

#endif /* __SCDM_DISPLAY_H */
