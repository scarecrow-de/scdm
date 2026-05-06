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

#ifndef _GDM_SETTINGS_KEYS_H
#define _GDM_SETTINGS_KEYS_H

#include <glib.h>

G_BEGIN_DECLS

#define SCDM_KEY_USER "daemon/User"
#define SCDM_KEY_GROUP "daemon/Group"
#define SCDM_KEY_AUTO_LOGIN_ENABLE "daemon/AutomaticLoginEnable"
#define SCDM_KEY_AUTO_LOGIN_USER "daemon/AutomaticLogin"
#define SCDM_KEY_TIMED_LOGIN_ENABLE "daemon/TimedLoginEnable"
#define SCDM_KEY_TIMED_LOGIN_USER "daemon/TimedLogin"
#define SCDM_KEY_TIMED_LOGIN_DELAY "daemon/TimedLoginDelay"
#define SCDM_KEY_INITIAL_SETUP_ENABLE "daemon/InitialSetupEnable"
#define SCDM_KEY_WAYLAND_ENABLE "daemon/WaylandEnable"

#define SCDM_KEY_DEBUG "debug/Enable"

#define SCDM_KEY_INCLUDE "greeter/Include"
#define SCDM_KEY_EXCLUDE "greeter/Exclude"
#define SCDM_KEY_INCLUDE_ALL "greeter/IncludeAll"

#define SCDM_KEY_DISALLOW_TCP "security/DisallowTCP"
#define SCDM_KEY_ALLOW_REMOTE_AUTOLOGIN "security/AllowRemoteAutoLogin"

#define SCDM_KEY_XDMCP_ENABLE "xdmcp/Enable"
#define SCDM_KEY_SHOW_LOCAL_GREETER "xdmcp/ShowLocalGreeter"
#define SCDM_KEY_MAX_PENDING "xdmcp/MaxPending"
#define SCDM_KEY_MAX_SESSIONS "xdmcp/MaxSessions"
#define SCDM_KEY_MAX_WAIT "xdmcp/MaxWait"
#define SCDM_KEY_DISPLAYS_PER_HOST "xdmcp/DisplaysPerHost"
#define SCDM_KEY_UDP_PORT "xdmcp/Port"
#define SCDM_KEY_INDIRECT "xdmcp/HonorIndirect"
#define SCDM_KEY_MAX_WAIT_INDIRECT "xdmcp/MaxWaitIndirect"
#define SCDM_KEY_PING_INTERVAL "xdmcp/PingIntervalSeconds"
#define SCDM_KEY_WILLING "xdmcp/Willing"

#define SCDM_KEY_MULTICAST "chooser/Multicast"
#define SCDM_KEY_MULTICAST_ADDR "chooser/MulticastAddr"

G_END_DECLS

#endif /* _GDM_SETTINGS_KEYS_H */
