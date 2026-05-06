/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 William Jon McCann <jmccann@redhat.com>
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


#ifndef __GDM_XDMCP_CHOOSER_DISPLAY_H
#define __GDM_XDMCP_CHOOSER_DISPLAY_H

#include <sys/types.h>
#include <sys/socket.h>
#include <glib-object.h>

#include "scdm-xdmcp-display.h"
#include "scdm-address.h"

G_BEGIN_DECLS

#define GDM_TYPE_XDMCP_CHOOSER_DISPLAY         (gdm_xdmcp_chooser_display_get_type ())
#define GDM_XDMCP_CHOOSER_DISPLAY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_XDMCP_CHOOSER_DISPLAY, ScdmXdmcpChooserDisplay))
#define GDM_XDMCP_CHOOSER_DISPLAY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_XDMCP_CHOOSER_DISPLAY, ScdmXdmcpChooserDisplayClass))
#define GDM_IS_XDMCP_CHOOSER_DISPLAY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_XDMCP_CHOOSER_DISPLAY))
#define GDM_IS_XDMCP_CHOOSER_DISPLAY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_XDMCP_CHOOSER_DISPLAY))
#define GDM_XDMCP_CHOOSER_DISPLAY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_XDMCP_CHOOSER_DISPLAY, ScdmXdmcpChooserDisplayClass))

typedef struct ScdmXdmcpChooserDisplayPrivate ScdmXdmcpChooserDisplayPrivate;

typedef struct
{
        ScdmXdmcpDisplay                parent;
} ScdmXdmcpChooserDisplay;

typedef struct
{
        ScdmXdmcpDisplayClass   parent_class;

        void (* hostname_selected)          (ScdmXdmcpChooserDisplay *display,
                                             const char             *hostname);
} ScdmXdmcpChooserDisplayClass;

GType                     gdm_xdmcp_chooser_display_get_type                 (void);


ScdmDisplay *              gdm_xdmcp_chooser_display_new                      (const char              *hostname,
                                                                              int                      number,
                                                                              ScdmAddress              *addr,
                                                                              gint32                   serial_number);

G_END_DECLS

#endif /* __GDM_XDMCP_CHOOSER_DISPLAY_H */
