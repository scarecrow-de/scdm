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

#ifndef __GDM_HOST_CHOOSER_WIDGET_H
#define __GDM_HOST_CHOOSER_WIDGET_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include "scdm-chooser-host.h"

G_BEGIN_DECLS

#define SCDM_TYPE_HOST_CHOOSER_WIDGET (scdm_host_chooser_widget_get_type ())
G_DECLARE_FINAL_TYPE (ScdmHostChooserWidget, scdm_host_chooser_widget, GDM, HOST_CHOOSER_WIDGET, GtkBox)

GtkWidget *            scdm_host_chooser_widget_new                (int                   kind_mask);

void                   scdm_host_chooser_widget_set_kind_mask      (ScdmHostChooserWidget *widget,
                                                                   int                   kind_mask);

void                   scdm_host_chooser_widget_refresh            (ScdmHostChooserWidget *widget);

ScdmChooserHost *       scdm_host_chooser_widget_get_host           (ScdmHostChooserWidget *widget);

G_END_DECLS

#endif /* __GDM_HOST_CHOOSER_WIDGET_H */
