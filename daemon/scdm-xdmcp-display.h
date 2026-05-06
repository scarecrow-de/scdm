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


#ifndef __SCDM_XDMCP_DISPLAY_H
#define __SCDM_XDMCP_DISPLAY_H

#include <sys/types.h>
#include <sys/socket.h>
#include <glib-object.h>

#include "scdm-display.h"
#include "scdm-address.h"

G_BEGIN_DECLS

#define SCDM_TYPE_XDMCP_DISPLAY (scdm_xdmcp_display_get_type ())
G_DECLARE_DERIVABLE_TYPE (ScdmXdmcpDisplay, scdm_xdmcp_display, GDM, XDMCP_DISPLAY, ScdmDisplay)

struct _ScdmXdmcpDisplayClass
{
        ScdmDisplayClass   parent_class;
};

gint32                    scdm_xdmcp_display_get_session_number       (ScdmXdmcpDisplay         *display);
ScdmAddress              * scdm_xdmcp_display_get_remote_address       (ScdmXdmcpDisplay         *display);

ScdmDisplay              * scdm_xdmcp_display_new                      (const char              *hostname,
                                                                      int                      number,
                                                                      ScdmAddress              *address,
                                                                      gint32                   session_number);

G_END_DECLS

#endif /* __SCDM_XDMCP_DISPLAY_H */
