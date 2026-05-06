/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 Ray Strode <rstrode@redhat.com>
 * Copyright (C) 2012 Jasper St. Pierre <jstpierre@mecheye.net>
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

#include "config.h"

#include <gio/gio.h>

#include "scdm-session-worker-common.h"

static const GDBusErrorEntry scdm_session_worker_error_entries[] = {
        { SCDM_SESSION_WORKER_ERROR_GENERIC              , "io.github.scarecrow_de.DisplayManager.SessionWorker.Error.Generic", },
        { SCDM_SESSION_WORKER_ERROR_WITH_SESSION_COMMAND , "io.github.scarecrow_de.DisplayManager.SessionWorker.Error.WithSessionCommand" },
        { SCDM_SESSION_WORKER_ERROR_FORKING              , "io.github.scarecrow_de.DisplayManager.SessionWorker.Error.Forking" },
        { SCDM_SESSION_WORKER_ERROR_OPENING_MESSAGE_PIPE , "io.github.scarecrow_de.DisplayManager.SessionWorker.Error.OpeningMessagePipe" },
        { SCDM_SESSION_WORKER_ERROR_COMMUNICATING        , "io.github.scarecrow_de.DisplayManager.SessionWorker.Error.Communicating" },
        { SCDM_SESSION_WORKER_ERROR_WORKER_DIED          , "io.github.scarecrow_de.DisplayManager.SessionWorker.Error.WorkerDied" },
        { SCDM_SESSION_WORKER_ERROR_SERVICE_UNAVAILABLE  , "io.github.scarecrow_de.DisplayManager.SessionWorker.Error.ServiceUnavailable" },
        { SCDM_SESSION_WORKER_ERROR_AUTHENTICATING       , "io.github.scarecrow_de.DisplayManager.SessionWorker.Error.Authenticating" },
        { SCDM_SESSION_WORKER_ERROR_AUTHORIZING          , "io.github.scarecrow_de.DisplayManager.SessionWorker.Error.Authorizing" },
        { SCDM_SESSION_WORKER_ERROR_OPENING_LOG_FILE     , "io.github.scarecrow_de.DisplayManager.SessionWorker.Error.OpeningLogFile" },
        { SCDM_SESSION_WORKER_ERROR_OPENING_SESSION      , "io.github.scarecrow_de.DisplayManager.SessionWorker.Error.OpeningSession" },
        { SCDM_SESSION_WORKER_ERROR_GIVING_CREDENTIALS   , "io.github.scarecrow_de.DisplayManager.SessionWorker.Error.GivingCredentials" },
        { SCDM_SESSION_WORKER_ERROR_WRONG_STATE          , "io.github.scarecrow_de.DisplayManager.SessionWorker.Error.WrongState" },
        { SCDM_SESSION_WORKER_ERROR_OUTSTANDING_REQUEST  , "io.github.scarecrow_de.DisplayManager.SessionWorker.Error.OutstandingRequest" },
        { SCDM_SESSION_WORKER_ERROR_IN_REAUTH_SESSION    , "io.github.scarecrow_de.DisplayManager.SessionWorker.Error.InReauthSession" }
};

GQuark
scdm_session_worker_error_quark (void)
{
        static volatile gsize error_quark = 0;

        g_dbus_error_register_error_domain ("scdm-session-worker-error-quark",
                                            &error_quark,
                                            scdm_session_worker_error_entries,
                                            G_N_ELEMENTS (scdm_session_worker_error_entries));

        return (GQuark) error_quark;
}
