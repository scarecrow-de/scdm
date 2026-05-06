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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include "scdm-display-store.h"
#include "scdm-display.h"

struct ScdmDisplayStorePrivate
{
        GHashTable *displays;
};

typedef struct
{
        ScdmDisplayStore *store;
        ScdmDisplay      *display;
} StoredDisplay;

enum {
        DISPLAY_ADDED,
        DISPLAY_REMOVED,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

static void     scdm_display_store_class_init    (ScdmDisplayStoreClass *klass);
static void     scdm_display_store_init          (ScdmDisplayStore      *display_store);
static void     scdm_display_store_finalize      (GObject              *object);

G_DEFINE_TYPE_WITH_PRIVATE (ScdmDisplayStore, scdm_display_store, G_TYPE_OBJECT)

static StoredDisplay *
stored_display_new (ScdmDisplayStore *store,
                    ScdmDisplay      *display)
{
        StoredDisplay *stored_display;

        stored_display = g_slice_new (StoredDisplay);
        stored_display->store = store;
        stored_display->display = g_object_ref (display);

        return stored_display;
}

static void
stored_display_free (StoredDisplay *stored_display)
{
        g_signal_emit (G_OBJECT (stored_display->store),
                       signals[DISPLAY_REMOVED],
                       0,
                       stored_display->display);

        g_debug ("ScdmDisplayStore: Unreffing display: %p",
                 stored_display->display);
        g_object_unref (stored_display->display);

        g_slice_free (StoredDisplay, stored_display);
}

GQuark
scdm_display_store_error_quark (void)
{
        static GQuark ret = 0;
        if (ret == 0) {
                ret = g_quark_from_static_string ("scdm_display_store_error");
        }

        return ret;
}

void
scdm_display_store_clear (ScdmDisplayStore    *store)
{
        g_return_if_fail (store != NULL);
        g_debug ("ScdmDisplayStore: Clearing display store");
        g_hash_table_remove_all (store->priv->displays);
}

static gboolean
remove_display (char              *id,
                ScdmDisplay        *display,
                ScdmDisplay        *display_to_remove)
{
        if (display == display_to_remove) {
                return TRUE;
        }
        return FALSE;
}

gboolean
scdm_display_store_remove (ScdmDisplayStore    *store,
                          ScdmDisplay         *display)
{
        g_return_val_if_fail (store != NULL, FALSE);

        scdm_display_store_foreach_remove (store,
                                          (ScdmDisplayStoreFunc)remove_display,
                                          display);
        return FALSE;
}

typedef struct
{
        ScdmDisplayStoreFunc predicate;
        gpointer            user_data;
} FindClosure;

static gboolean
find_func (const char    *id,
           StoredDisplay *stored_display,
           FindClosure   *closure)
{
        return closure->predicate (id,
                                   stored_display->display,
                                   closure->user_data);
}

static void
foreach_func (const char    *id,
              StoredDisplay *stored_display,
              FindClosure   *closure)
{
        (void) closure->predicate (id,
                                   stored_display->display,
                                   closure->user_data);
}

void
scdm_display_store_foreach (ScdmDisplayStore    *store,
                           ScdmDisplayStoreFunc func,
                           gpointer            user_data)
{
        FindClosure  closure;

        g_return_if_fail (store != NULL);
        g_return_if_fail (func != NULL);

        closure.predicate = func;
        closure.user_data = user_data;

        g_hash_table_foreach (store->priv->displays,
                              (GHFunc) foreach_func,
                              &closure);
}

ScdmDisplay *
scdm_display_store_lookup (ScdmDisplayStore *store,
                          const char      *id)
{
        StoredDisplay *stored_display;

        g_return_val_if_fail (store != NULL, NULL);
        g_return_val_if_fail (id != NULL, NULL);

        stored_display = g_hash_table_lookup (store->priv->displays,
                                              id);
        if (stored_display == NULL) {
                return NULL;
        }

        return stored_display->display;
}

ScdmDisplay *
scdm_display_store_find (ScdmDisplayStore    *store,
                        ScdmDisplayStoreFunc predicate,
                        gpointer            user_data)
{
        StoredDisplay *stored_display;
        FindClosure    closure;

        g_return_val_if_fail (store != NULL, NULL);
        g_return_val_if_fail (predicate != NULL, NULL);

        closure.predicate = predicate;
        closure.user_data = user_data;

        stored_display = g_hash_table_find (store->priv->displays,
                                            (GHRFunc) find_func,
                                            &closure);

        if (stored_display == NULL) {
                return NULL;
        }

        return stored_display->display;
}

guint
scdm_display_store_foreach_remove (ScdmDisplayStore    *store,
                                  ScdmDisplayStoreFunc func,
                                  gpointer            user_data)
{
        FindClosure closure;
        guint       ret;

        g_return_val_if_fail (store != NULL, 0);
        g_return_val_if_fail (func != NULL, 0);

        closure.predicate = func;
        closure.user_data = user_data;

        ret = g_hash_table_foreach_remove (store->priv->displays,
                                           (GHRFunc) find_func,
                                           &closure);
        return ret;
}

void
scdm_display_store_add (ScdmDisplayStore *store,
                       ScdmDisplay      *display)
{
        char          *id;
        StoredDisplay *stored_display;

        g_return_if_fail (store != NULL);
        g_return_if_fail (display != NULL);

        scdm_display_get_id (display, &id, NULL);

        g_debug ("ScdmDisplayStore: Adding display %s to store", id);

        stored_display = stored_display_new (store, display);
        g_hash_table_insert (store->priv->displays,
                             id,
                             stored_display);

        g_signal_emit (G_OBJECT (store),
                       signals[DISPLAY_ADDED],
                       0,
                       id);
}

static void
scdm_display_store_class_init (ScdmDisplayStoreClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = scdm_display_store_finalize;

        signals [DISPLAY_ADDED] =
                g_signal_new ("display-added",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ScdmDisplayStoreClass, display_added),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);
        signals [DISPLAY_REMOVED] =
                g_signal_new ("display-removed",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ScdmDisplayStoreClass, display_removed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__OBJECT,
                              G_TYPE_NONE,
                              1, G_TYPE_OBJECT);
}

static void
scdm_display_store_init (ScdmDisplayStore *store)
{

        store->priv = scdm_display_store_get_instance_private (store);

        store->priv->displays = g_hash_table_new_full (g_str_hash,
                                                       g_str_equal,
                                                       g_free,
                                                       (GDestroyNotify)
                                                       stored_display_free);
}

static void
scdm_display_store_finalize (GObject *object)
{
        ScdmDisplayStore *store;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GDM_IS_DISPLAY_STORE (object));

        store = GDM_DISPLAY_STORE (object);

        g_return_if_fail (store->priv != NULL);

        g_hash_table_destroy (store->priv->displays);

        G_OBJECT_CLASS (scdm_display_store_parent_class)->finalize (object);
}

ScdmDisplayStore *
scdm_display_store_new (void)
{
        GObject *object;

        object = g_object_new (SCDM_TYPE_DISPLAY_STORE,
                               NULL);

        return GDM_DISPLAY_STORE (object);
}
