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


#ifndef __SCDM_LOCAL_DISPLAY_FACTORY_H
#define __SCDM_LOCAL_DISPLAY_FACTORY_H

#include <glib-object.h>

#include "scdm-display-factory.h"
#include "scdm-display-store.h"

G_BEGIN_DECLS

#define SCDM_TYPE_LOCAL_DISPLAY_FACTORY (scdm_local_display_factory_get_type ())
G_DECLARE_FINAL_TYPE (ScdmLocalDisplayFactory, scdm_local_display_factory, GDM, LOCAL_DISPLAY_FACTORY, ScdmDisplayFactory)

typedef enum
{
         SCDM_LOCAL_DISPLAY_FACTORY_ERROR_GENERAL
} ScdmLocalDisplayFactoryError;

#define SCDM_LOCAL_DISPLAY_FACTORY_ERROR scdm_local_display_factory_error_quark ()

GQuark                     scdm_local_display_factory_error_quark              (void);

ScdmLocalDisplayFactory *   scdm_local_display_factory_new                      (ScdmDisplayStore        *display_store);

gboolean                   scdm_local_display_factory_create_transient_display (ScdmLocalDisplayFactory *factory,
                                                                               char                  **id,
                                                                               GError                **error);
G_END_DECLS

#endif /* __SCDM_LOCAL_DISPLAY_FACTORY_H */
