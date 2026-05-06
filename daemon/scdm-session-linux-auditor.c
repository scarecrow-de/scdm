/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2004, 2008 Sun Microsystems, Inc.
 * Copyright (C) 2005, 2008 Red Hat, Inc.
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
 *
 * Written by: Brian A. Cameron <Brian.Cameron@sun.com>
 *             Gary Winiger <Gary.Winiger@sun.com>
 *             Ray Strode <rstrode@redhat.com>
 *             Steve Grubb <sgrubb@redhat.com>
 */
#include "config.h"
#include "scdm-session-linux-auditor.h"

#include <fcntl.h>
#include <pwd.h>
#include <syslog.h>
#include <unistd.h>

#include <libaudit.h>

#include <glib.h>

#include "scdm-common.h"

struct _ScdmSessionLinuxAuditor
{
        ScdmSessionAuditor parent;
        int audit_fd;
};

static void scdm_session_linux_auditor_finalize (GObject *object);

G_DEFINE_TYPE (ScdmSessionLinuxAuditor, scdm_session_linux_auditor, GDM_TYPE_SESSION_AUDITOR)

static void
log_user_message (ScdmSessionAuditor *auditor,
                  gint               type,
                  gint               result)
{
        ScdmSessionLinuxAuditor   *linux_auditor;
        char                      buf[512];
        char                     *username;
        char                     *hostname;
        char                     *display_device;
        struct passwd            *pw;

        linux_auditor = GDM_SESSION_LINUX_AUDITOR (auditor);

        g_object_get (G_OBJECT (auditor), "username", &username, NULL);
        g_object_get (G_OBJECT (auditor), "hostname", &hostname, NULL);
        g_object_get (G_OBJECT (auditor), "display-device", &display_device, NULL);

        if (username != NULL) {
                scdm_get_pwent_for_name (username, &pw);
        } else {
                username = g_strdup ("unknown");
                pw = NULL;
        }

        if (pw != NULL) {
                g_snprintf (buf, sizeof (buf), "uid=%d", pw->pw_uid);
                audit_log_user_message (linux_auditor->audit_fd, type,
                                        buf, hostname, NULL, display_device,
                                        result);
        } else {
                g_snprintf (buf, sizeof (buf), "acct=%s", username);
                audit_log_user_message (linux_auditor->audit_fd, type,
                                        buf, hostname, NULL, display_device,
                                        result);
        }

        g_free (username);
        g_free (hostname);
        g_free (display_device);
}

static void
scdm_session_linux_auditor_report_login (ScdmSessionAuditor *auditor)
{
        log_user_message (auditor, AUDIT_USER_LOGIN, 1);
}

static void
scdm_session_linux_auditor_report_login_failure (ScdmSessionAuditor *auditor,
                                                  int                pam_error_code,
                                                  const char        *pam_error_string)
{
        log_user_message (auditor, AUDIT_USER_LOGIN, 0);
}

static void
scdm_session_linux_auditor_report_logout (ScdmSessionAuditor *auditor)
{
        log_user_message (auditor, AUDIT_USER_LOGOUT, 1);
}

static void
scdm_session_linux_auditor_class_init (ScdmSessionLinuxAuditorClass *klass)
{
        GObjectClass           *object_class;
        ScdmSessionAuditorClass *auditor_class;

        object_class = G_OBJECT_CLASS (klass);
        auditor_class = GDM_SESSION_AUDITOR_CLASS (klass);

        object_class->finalize = scdm_session_linux_auditor_finalize;

        auditor_class->report_login = scdm_session_linux_auditor_report_login;
        auditor_class->report_login_failure = scdm_session_linux_auditor_report_login_failure;
        auditor_class->report_logout = scdm_session_linux_auditor_report_logout;
}

static void
scdm_session_linux_auditor_init (ScdmSessionLinuxAuditor *auditor)
{
        auditor->audit_fd = audit_open ();
}

static void
scdm_session_linux_auditor_finalize (GObject *object)
{
        ScdmSessionLinuxAuditor *linux_auditor;
        GObjectClass *parent_class;

        linux_auditor = GDM_SESSION_LINUX_AUDITOR (object);

        close (linux_auditor->audit_fd);

        parent_class = G_OBJECT_CLASS (scdm_session_linux_auditor_parent_class);
        if (parent_class->finalize != NULL) {
                parent_class->finalize (object);
        }
}


ScdmSessionAuditor *
scdm_session_linux_auditor_new (const char *hostname,
                               const char *display_device)
{
        GObject *auditor;

        auditor = g_object_new (GDM_TYPE_SESSION_LINUX_AUDITOR,
                                "hostname", hostname,
                                "display-device", display_device,
                                NULL);

        return GDM_SESSION_AUDITOR (auditor);
}


