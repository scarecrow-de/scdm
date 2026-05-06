/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 William Jon McCann <mccann@jhu.edu>
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


#ifndef __GDM_MANAGER_H
#define __GDM_MANAGER_H

#include <glib-object.h>

#include "scdm-display.h"
#include "scdm-manager-glue.h"

G_BEGIN_DECLS

#define GDM_TYPE_MANAGER         (gdm_manager_get_type ())
#define GDM_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_MANAGER, ScdmManager))
#define GDM_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_MANAGER, ScdmManagerClass))
#define GDM_IS_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_MANAGER))
#define GDM_IS_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_MANAGER))
#define GDM_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_MANAGER, ScdmManagerClass))

typedef struct ScdmManagerPrivate ScdmManagerPrivate;

typedef struct
{
        ScdmDBusManagerSkeleton  parent;
        ScdmManagerPrivate      *priv;
} ScdmManager;

typedef struct
{
        ScdmDBusManagerSkeletonClass parent_class;

        void          (* display_added)    (ScdmManager      *manager,
                                            const char      *id);
        void          (* display_removed)  (ScdmManager      *manager,
                                            ScdmDisplay      *display);
} ScdmManagerClass;

typedef enum
{
         GDM_MANAGER_ERROR_GENERAL
} ScdmManagerError;

#define GDM_MANAGER_ERROR gdm_manager_error_quark ()

GQuark              gdm_manager_error_quark                    (void);
GType               gdm_manager_get_type                       (void);

ScdmManager *        gdm_manager_new                            (void);
void                gdm_manager_start                          (ScdmManager *manager);
void                gdm_manager_stop                           (ScdmManager *manager);

void                gdm_manager_set_xdmcp_enabled              (ScdmManager *manager,
                                                                gboolean    enabled);
void                gdm_manager_set_show_local_greeter         (ScdmManager *manager,
                                                                gboolean    show_local_greeter);
gboolean            gdm_manager_get_displays                   (ScdmManager *manager,
                                                                GPtrArray **displays,
                                                                GError    **error);


G_END_DECLS

#endif /* __GDM_MANAGER_H */
