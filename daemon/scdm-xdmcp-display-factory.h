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


#ifndef __SCDM_XDMCP_DISPLAY_FACTORY_H
#define __SCDM_XDMCP_DISPLAY_FACTORY_H

#include <glib-object.h>

#include "scdm-display-factory.h"
#include "scdm-display-store.h"

G_BEGIN_DECLS

#define SCDM_TYPE_XDMCP_DISPLAY_FACTORY (scdm_xdmcp_display_factory_get_type ())
G_DECLARE_FINAL_TYPE (ScdmXdmcpDisplayFactory, scdm_xdmcp_display_factory, GDM, XDMCP_DISPLAY_FACTORY, ScdmDisplayFactory)

typedef enum
{
         SCDM_XDMCP_DISPLAY_FACTORY_ERROR_GENERAL
} ScdmXdmcpDisplayFactoryError;

#define SCDM_XDMCP_DISPLAY_FACTORY_ERROR scdm_xdmcp_display_factory_error_quark ()

GQuark                     scdm_xdmcp_display_factory_error_quark      (void);

ScdmXdmcpDisplayFactory *   scdm_xdmcp_display_factory_new              (ScdmDisplayStore        *display_store);

void                       scdm_xdmcp_display_factory_set_port         (ScdmXdmcpDisplayFactory *manager,
                                                                       guint                   port);

G_END_DECLS

#endif /* __SCDM_XDMCP_DISPLAY_FACTORY_H */
