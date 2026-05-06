/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Red Hat, Inc.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __GDM_CHOOSER_HOST__
#define __GDM_CHOOSER_HOST__

#include <glib-object.h>
#include "scdm-address.h"

G_BEGIN_DECLS

#define GDM_TYPE_CHOOSER_HOST (scdm_chooser_host_get_type ())
G_DECLARE_FINAL_TYPE (ScdmChooserHost, scdm_chooser_host, GDM, CHOOSER_HOST, GObject)

typedef enum {
        GDM_CHOOSER_HOST_KIND_XDMCP = 1 << 0,
} ScdmChooserHostKind;

#define GDM_CHOOSER_HOST_KIND_MASK_ALL (GDM_CHOOSER_HOST_KIND_XDMCP)

G_CONST_RETURN char  *scdm_chooser_host_get_description     (ScdmChooserHost   *chooser_host);
ScdmAddress *          scdm_chooser_host_get_address         (ScdmChooserHost   *chooser_host);
gboolean              scdm_chooser_host_get_willing         (ScdmChooserHost   *chooser_host);
ScdmChooserHostKind    scdm_chooser_host_get_kind            (ScdmChooserHost   *chooser_host);

G_END_DECLS

#endif
