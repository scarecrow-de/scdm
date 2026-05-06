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


#ifndef __GDM_DISPLAY_STORE_H
#define __GDM_DISPLAY_STORE_H

#include <glib-object.h>
#include "scdm-display.h"

G_BEGIN_DECLS

#define GDM_TYPE_DISPLAY_STORE         (gdm_display_store_get_type ())
#define GDM_DISPLAY_STORE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_DISPLAY_STORE, ScdmDisplayStore))
#define GDM_DISPLAY_STORE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_DISPLAY_STORE, ScdmDisplayStoreClass))
#define GDM_IS_DISPLAY_STORE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_DISPLAY_STORE))
#define GDM_IS_DISPLAY_STORE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_DISPLAY_STORE))
#define GDM_DISPLAY_STORE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_DISPLAY_STORE, ScdmDisplayStoreClass))

typedef struct ScdmDisplayStorePrivate ScdmDisplayStorePrivate;

typedef struct
{
        GObject                 parent;
        ScdmDisplayStorePrivate *priv;
} ScdmDisplayStore;

typedef struct
{
        GObjectClass   parent_class;

        void          (* display_added)    (ScdmDisplayStore *display_store,
                                            const char      *id);
        void          (* display_removed)  (ScdmDisplayStore *display_store,
                                            ScdmDisplay      *display);
} ScdmDisplayStoreClass;

typedef enum
{
         GDM_DISPLAY_STORE_ERROR_GENERAL
} ScdmDisplayStoreError;

#define GDM_DISPLAY_STORE_ERROR gdm_display_store_error_quark ()

typedef gboolean (*ScdmDisplayStoreFunc) (const char *id,
                                         ScdmDisplay *display,
                                         gpointer    user_data);

GQuark              gdm_display_store_error_quark              (void);
GType               gdm_display_store_get_type                 (void);

ScdmDisplayStore *   gdm_display_store_new                      (void);

void                gdm_display_store_add                      (ScdmDisplayStore    *store,
                                                                ScdmDisplay         *display);
void                gdm_display_store_clear                    (ScdmDisplayStore    *store);
gboolean            gdm_display_store_remove                   (ScdmDisplayStore    *store,
                                                                ScdmDisplay         *display);
void                gdm_display_store_foreach                  (ScdmDisplayStore    *store,
                                                                ScdmDisplayStoreFunc func,
                                                                gpointer            user_data);
guint               gdm_display_store_foreach_remove           (ScdmDisplayStore    *store,
                                                                ScdmDisplayStoreFunc func,
                                                                gpointer            user_data);
ScdmDisplay *        gdm_display_store_lookup                   (ScdmDisplayStore    *store,
                                                                const char         *id);

ScdmDisplay *        gdm_display_store_find                     (ScdmDisplayStore    *store,
                                                                ScdmDisplayStoreFunc predicate,
                                                                gpointer            user_data);


G_END_DECLS

#endif /* __GDM_DISPLAY_STORE_H */
