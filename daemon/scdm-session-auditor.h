/* scdm-session-auditor.h - Object for auditing session login/logout
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
#ifndef SCDM_SESSION_AUDITOR_H
#define SCDM_SESSION_AUDITOR_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define SCDM_TYPE_SESSION_AUDITOR (scdm_session_auditor_get_type ())
G_DECLARE_DERIVABLE_TYPE (ScdmSessionAuditor, scdm_session_auditor, GDM, SESSION_AUDITOR, GObject)

struct _ScdmSessionAuditorClass
{
        GObjectClass        parent_class;

        void               (* report_password_changed)        (ScdmSessionAuditor *auditor);
        void               (* report_password_change_failure) (ScdmSessionAuditor *auditor);
        void               (* report_user_accredited)         (ScdmSessionAuditor *auditor);
        void               (* report_login)                   (ScdmSessionAuditor *auditor);
        void               (* report_login_failure)           (ScdmSessionAuditor *auditor,
                                                               int                error_code,
                                                               const char        *error_message);
        void               (* report_logout)                  (ScdmSessionAuditor *auditor);
};

ScdmSessionAuditor        *scdm_session_auditor_new                (const char *hostname,
                                                                  const char *display_device);
void                      scdm_session_auditor_set_username (ScdmSessionAuditor *auditor,
                                                            const char        *username);
void                      scdm_session_auditor_report_password_changed (ScdmSessionAuditor *auditor);
void                      scdm_session_auditor_report_password_change_failure (ScdmSessionAuditor *auditor);
void                      scdm_session_auditor_report_user_accredited (ScdmSessionAuditor *auditor);
void                      scdm_session_auditor_report_login (ScdmSessionAuditor *auditor);
void                      scdm_session_auditor_report_login_failure (ScdmSessionAuditor *auditor,
                                                                    int                error_code,
                                                                    const char        *error_message);
void                      scdm_session_auditor_report_logout (ScdmSessionAuditor *auditor);

G_END_DECLS
#endif /* SCDM_SESSION_AUDITOR_H */
