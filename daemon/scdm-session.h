/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 Ray Strode <rstrode@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef __GDM_SESSION_H
#define __GDM_SESSION_H

#include <glib-object.h>
#include <sys/types.h>

G_BEGIN_DECLS

#define SCDM_TYPE_SESSION (scdm_session_get_type ())
G_DECLARE_FINAL_TYPE (ScdmSession, scdm_session, GDM, SESSION, GObject)

typedef enum
{
        GDM_SESSION_VERIFICATION_MODE_LOGIN,
        GDM_SESSION_VERIFICATION_MODE_CHOOSER,
        GDM_SESSION_VERIFICATION_MODE_REAUTHENTICATE
} ScdmSessionVerificationMode;

typedef enum {
        /* We reuse the existing display server, e.g. X server
         * in "classic" mode from the greeter for the first seat. */
        GDM_SESSION_DISPLAY_MODE_REUSE_VT,

        /* Doesn't know anything about VTs. Tries to set DRM
         * master and will throw a tantrum if something bad
         * happens. e.g. weston-launch or vater-launch. */
        GDM_SESSION_DISPLAY_MODE_NEW_VT,

        /* Uses logind sessions to manage itself. We need to set an
         * XDG_VTNR and it will switch to the correct VT on startup.
         * e.g. vater-wayland with logind integration, X server with
         * logind integration. */
        GDM_SESSION_DISPLAY_MODE_LOGIND_MANAGED,
} ScdmSessionDisplayMode;

ScdmSessionDisplayMode scdm_session_display_mode_from_string (const char *str);
const char * scdm_session_display_mode_to_string (ScdmSessionDisplayMode mode);

ScdmSession      *scdm_session_new                      (ScdmSessionVerificationMode verification_mode,
                                                       uid_t         allowed_user,
                                                       const char   *display_name,
                                                       const char   *display_hostname,
                                                       const char   *display_device,
                                                       const char   *display_seat_id,
                                                       const char   *display_x11_authority_file,
                                                       gboolean      display_is_local,
                                                       const char * const *environment);
uid_t             scdm_session_get_allowed_user       (ScdmSession     *session);
void              scdm_session_start_reauthentication (ScdmSession *session,
                                                      GPid        pid_of_caller,
                                                      uid_t       uid_of_caller);

const char       *scdm_session_get_server_address          (ScdmSession     *session);
const char       *scdm_session_get_username                (ScdmSession     *session);
const char       *scdm_session_get_display_device          (ScdmSession     *session);
const char       *scdm_session_get_display_seat_id         (ScdmSession     *session);
const char       *scdm_session_get_session_id              (ScdmSession     *session);
gboolean          scdm_session_bypasses_xsession           (ScdmSession     *session);
gboolean          scdm_session_session_registers           (ScdmSession     *session);
ScdmSessionDisplayMode scdm_session_get_display_mode  (ScdmSession     *session);

#ifdef ENABLE_WAYLAND_SUPPORT
void              scdm_session_set_ignore_wayland          (ScdmSession *session,
                                                           gboolean    ignore_wayland);
#endif
gboolean          scdm_session_start_conversation          (ScdmSession *session,
                                                           const char *service_name);
void              scdm_session_stop_conversation           (ScdmSession *session,
                                                           const char *service_name);
const char       *scdm_session_get_conversation_session_id (ScdmSession *session,
                                                           const char *service_name);
void              scdm_session_setup                       (ScdmSession *session,
                                                           const char *service_name);
void              scdm_session_setup_for_user              (ScdmSession *session,
                                                           const char *service_name,
                                                           const char *username);
void              scdm_session_setup_for_program           (ScdmSession *session,
                                                           const char *service_name,
                                                           const char *username,
                                                           const char *log_file);
void              scdm_session_set_environment_variable    (ScdmSession *session,
                                                           const char *key,
                                                           const char *value);
void              scdm_session_send_environment            (ScdmSession *self,
                                                           const char *service_name);
void              scdm_session_reset                       (ScdmSession *session);
void              scdm_session_cancel                      (ScdmSession *session);
void              scdm_session_authenticate                (ScdmSession *session,
                                                           const char *service_name);
void              scdm_session_authorize                   (ScdmSession *session,
                                                           const char *service_name);
void              scdm_session_accredit                    (ScdmSession *session,
                                                           const char *service_name);
void              scdm_session_open_session                (ScdmSession *session,
                                                           const char *service_name);
void              scdm_session_start_session               (ScdmSession *session,
                                                           const char *service_name);
void              scdm_session_close                       (ScdmSession *session);

void              scdm_session_answer_query                (ScdmSession *session,
                                                           const char *service_name,
                                                           const char *text);
void              scdm_session_select_program              (ScdmSession *session,
                                                           const char *command_line);
void              scdm_session_select_session              (ScdmSession *session,
                                                           const char *session_name);
void              scdm_session_select_user                 (ScdmSession *session,
                                                           const char *username);
void              scdm_session_set_timed_login_details     (ScdmSession *session,
                                                           const char *username,
                                                           int         delay);
gboolean          scdm_session_client_is_connected         (ScdmSession *session);
gboolean          scdm_session_is_running                  (ScdmSession *session);
GPid              scdm_session_get_pid                     (ScdmSession *session);

G_END_DECLS

#endif /* GDM_SESSION_H */
