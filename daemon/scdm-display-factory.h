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


#ifndef __GDM_DISPLAY_FACTORY_H
#define __GDM_DISPLAY_FACTORY_H

#include <glib-object.h>

#include "scdm-display-store.h"

G_BEGIN_DECLS

#define GDM_TYPE_DISPLAY_FACTORY (scdm_display_factory_get_type ())
G_DECLARE_DERIVABLE_TYPE (ScdmDisplayFactory, scdm_display_factory, GDM, DISPLAY_FACTORY, GObject)

struct _ScdmDisplayFactoryClass
{
        GObjectClass   parent_class;

        gboolean (*start)                  (ScdmDisplayFactory *factory);
        gboolean (*stop)                   (ScdmDisplayFactory *factory);
};

typedef enum
{
         GDM_DISPLAY_FACTORY_ERROR_GENERAL
} ScdmDisplayFactoryError;

#define GDM_DISPLAY_FACTORY_ERROR scdm_display_factory_error_quark ()

GQuark                     scdm_display_factory_error_quark             (void);
GType                      scdm_display_factory_get_type                (void);

gboolean                   scdm_display_factory_start                   (ScdmDisplayFactory *manager);
gboolean                   scdm_display_factory_stop                    (ScdmDisplayFactory *manager);
ScdmDisplayStore *          scdm_display_factory_get_display_store       (ScdmDisplayFactory *manager);
void                       scdm_display_factory_queue_purge_displays    (ScdmDisplayFactory *manager);

G_END_DECLS

#endif /* __GDM_DISPLAY_FACTORY_H */
