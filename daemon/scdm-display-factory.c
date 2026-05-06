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

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include "scdm-display-factory.h"
#include "scdm-display-store.h"

typedef struct _ScdmDisplayFactoryPrivate
{
        ScdmDisplayStore *display_store;
        guint            purge_displays_id;
} ScdmDisplayFactoryPrivate;

enum {
        PROP_0,
        PROP_DISPLAY_STORE,
};

static void     scdm_display_factory_class_init  (ScdmDisplayFactoryClass *klass);
static void     scdm_display_factory_init        (ScdmDisplayFactory      *factory);
static void     scdm_display_factory_finalize    (GObject                *object);

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (ScdmDisplayFactory, scdm_display_factory, G_TYPE_OBJECT)

GQuark
scdm_display_factory_error_quark (void)
{
        static GQuark ret = 0;
        if (ret == 0) {
                ret = g_quark_from_static_string ("scdm_display_factory_error");
        }

        return ret;
}

static gboolean
purge_display (char       *id,
               ScdmDisplay *display,
               gpointer    user_data)
{
        int status;

        status = scdm_display_get_status (display);

        switch (status) {
        case SCDM_DISPLAY_FINISHED:
        case SCDM_DISPLAY_FAILED:
                return TRUE;
        default:
                return FALSE;
        }
}

static void
purge_displays (ScdmDisplayFactory *factory)
{
        ScdmDisplayFactoryPrivate *priv;

        priv = scdm_display_factory_get_instance_private (factory);
        priv->purge_displays_id = 0;
        scdm_display_store_foreach_remove (priv->display_store,
                                          (ScdmDisplayStoreFunc)purge_display,
                                          NULL);
}

void
scdm_display_factory_queue_purge_displays (ScdmDisplayFactory *factory)
{
        ScdmDisplayFactoryPrivate *priv;

        priv = scdm_display_factory_get_instance_private (factory);
        if (priv->purge_displays_id == 0) {
                priv->purge_displays_id = g_idle_add ((GSourceFunc) purge_displays, factory);
        }
}

ScdmDisplayStore *
scdm_display_factory_get_display_store (ScdmDisplayFactory *factory)
{
        ScdmDisplayFactoryPrivate *priv;

        g_return_val_if_fail (SCDM_IS_DISPLAY_FACTORY (factory), NULL);

        priv = scdm_display_factory_get_instance_private (factory);
        return priv->display_store;
}

gboolean
scdm_display_factory_start (ScdmDisplayFactory *factory)
{
        gboolean ret;

        g_return_val_if_fail (SCDM_IS_DISPLAY_FACTORY (factory), FALSE);

        g_object_ref (factory);
        ret = SCDM_DISPLAY_FACTORY_GET_CLASS (factory)->start (factory);
        g_object_unref (factory);

        return ret;
}

gboolean
scdm_display_factory_stop (ScdmDisplayFactory *factory)
{
        gboolean ret;

        g_return_val_if_fail (SCDM_IS_DISPLAY_FACTORY (factory), FALSE);

        g_object_ref (factory);
        ret = SCDM_DISPLAY_FACTORY_GET_CLASS (factory)->stop (factory);
        g_object_unref (factory);

        return ret;
}

static void
scdm_display_factory_set_display_store (ScdmDisplayFactory *factory,
                                       ScdmDisplayStore   *display_store)
{
        ScdmDisplayFactoryPrivate *priv;

        priv = scdm_display_factory_get_instance_private (factory);
        g_clear_object (&priv->display_store);

        if (display_store != NULL) {
                priv->display_store = g_object_ref (display_store);
        }
}

static void
scdm_display_factory_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
        ScdmDisplayFactory *self;

        self = SCDM_DISPLAY_FACTORY (object);

        switch (prop_id) {
        case PROP_DISPLAY_STORE:
                scdm_display_factory_set_display_store (self, g_value_get_object (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
scdm_display_factory_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
        ScdmDisplayFactory *self;
        ScdmDisplayFactoryPrivate *priv;

        self = SCDM_DISPLAY_FACTORY (object);
        priv = scdm_display_factory_get_instance_private (self);

        switch (prop_id) {
        case PROP_DISPLAY_STORE:
                g_value_set_object (value, priv->display_store);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
scdm_display_factory_class_init (ScdmDisplayFactoryClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = scdm_display_factory_get_property;
        object_class->set_property = scdm_display_factory_set_property;
        object_class->finalize = scdm_display_factory_finalize;

        g_object_class_install_property (object_class,
                                         PROP_DISPLAY_STORE,
                                         g_param_spec_object ("display-store",
                                                              "display store",
                                                              "display store",
                                                              SCDM_TYPE_DISPLAY_STORE,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
}

static void
scdm_display_factory_init (ScdmDisplayFactory *factory)
{
}

static void
scdm_display_factory_finalize (GObject *object)
{
        ScdmDisplayFactory *factory;
        ScdmDisplayFactoryPrivate *priv;

        g_return_if_fail (object != NULL);
        g_return_if_fail (SCDM_IS_DISPLAY_FACTORY (object));

        factory = SCDM_DISPLAY_FACTORY (object);
        priv = scdm_display_factory_get_instance_private (factory);

        g_return_if_fail (priv != NULL);

        if (priv->purge_displays_id != 0) {
                g_source_remove (priv->purge_displays_id);
                priv->purge_displays_id = 0;
        }

        G_OBJECT_CLASS (scdm_display_factory_parent_class)->finalize (object);
}
