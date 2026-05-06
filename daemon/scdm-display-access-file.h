/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * scdm-display-access-file.h - Abstraction around xauth cookies
 *
 * Copyright (C) 2007 Ray Strode <rstrode@redhat.com>
 *
 * Written by Ray Strode <rstrode@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#ifndef __GDM_DISPLAY_ACCESS_FILE_H__
#define __GDM_DISPLAY_ACCESS_FILE_H__

#include <glib.h>
#include <glib-object.h>

#include "scdm-display.h"

G_BEGIN_DECLS

#define GDM_TYPE_DISPLAY_ACCESS_FILE (gdm_display_access_file_get_type ())
G_DECLARE_FINAL_TYPE (ScdmDisplayAccessFile, gdm_display_access_file, GDM, DISPLAY_ACCESS_FILE, GObject)

#define GDM_DISPLAY_ACCESS_FILE_ERROR           (gdm_display_access_file_error_quark ())

typedef enum _ScdmDisplayAccessFileError ScdmDisplayAccessFileError;

enum _ScdmDisplayAccessFileError
{
        GDM_DISPLAY_ACCESS_FILE_ERROR_GENERAL = 0,
        GDM_DISPLAY_ACCESS_FILE_ERROR_FINDING_AUTH_ENTRY
};

GQuark                gdm_display_access_file_error_quark             (void);

ScdmDisplayAccessFile *gdm_display_access_file_new                     (const char            *username);
gboolean              gdm_display_access_file_open                    (ScdmDisplayAccessFile  *file,
                                                                       GError               **error);
gboolean              gdm_display_access_file_add_display             (ScdmDisplayAccessFile  *file,
                                                                       ScdmDisplay            *display,
                                                                       char                 **cookie,
                                                                       gsize                 *cookie_size,
                                                                       GError               **error);
gboolean              gdm_display_access_file_add_display_with_cookie (ScdmDisplayAccessFile  *file,
                                                                       ScdmDisplay            *display,
                                                                       const char            *cookie,
                                                                       gsize                  cookie_size,
                                                                       GError               **error);

void                  gdm_display_access_file_close                   (ScdmDisplayAccessFile  *file);
char                 *gdm_display_access_file_get_path                (ScdmDisplayAccessFile  *file);

G_END_DECLS
#endif /* __GDM_DISPLAY_ACCESS_FILE_H__ */
