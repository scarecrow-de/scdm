/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _GDM_COMMON_H
#define _GDM_COMMON_H

#include <glib-unix.h>
#include <gio/gio.h>

#include <pwd.h>
#include <errno.h>

#define REGISTER_SESSION_TIMEOUT 10

#define        VE_IGNORE_EINTR(expr) \
        do {                         \
                errno = 0;           \
                expr;                \
        } while G_UNLIKELY (errno == EINTR);

#define GDM_SYSTEMD_SESSION_REQUIRE_ONLINE 0

GQuark scdm_common_error_quark (void);
#define GDM_COMMON_ERROR scdm_common_error_quark()

typedef char * (*GdmExpandVarFunc) (const char *var,
                                    gpointer user_data);

G_BEGIN_DECLS

int            scdm_wait_on_pid           (int pid);
int            scdm_wait_on_and_disown_pid (int pid,
                                           int timeout);
int            scdm_signal_pid            (int pid,
                                          int signal);

gboolean       scdm_find_display_session (GPid        pid,
                                         const uid_t uid,
                                         char      **out_session_id,
                                         GError    **error);

gboolean       scdm_get_pwent_for_name    (const char     *name,
                                          struct passwd **pwentp);

gboolean       scdm_clear_close_on_exec_flag (int fd);

char          *scdm_generate_random_bytes (gsize          size,
                                          GError       **error);
gboolean       scdm_get_login_window_session_id (const char  *seat_id,
                                                char       **session_id);
gboolean       scdm_goto_login_session    (GError **error);

GPtrArray     *scdm_get_script_environment (const char *username,
                                           const char *display_name,
                                           const char *display_hostname,
                                           const char *display_x11_authority_file);
gboolean       scdm_run_script             (const char *dir,
                                           const char *username,
                                           const char *display_name,
                                           const char *display_hostname,
                                           const char *display_x11_authority_file);

gboolean      scdm_shell_var_is_valid_char (char c,
                                           gboolean first);
char *        scdm_shell_expand            (const char *str,
                                           GdmExpandVarFunc expand_func,
                                           gpointer user_data);

gboolean      scdm_activate_session_by_id (GDBusConnection *connection,
                                          const char      *seat_id,
                                          const char      *session_id);

G_END_DECLS

#endif /* _GDM_COMMON_H */
