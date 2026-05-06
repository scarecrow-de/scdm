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


#ifndef __SCDM_SETTINGS_UTILS_H
#define __SCDM_SETTINGS_UTILS_H

#include <glib-object.h>
#include <glib.h>


G_BEGIN_DECLS

typedef struct _ScdmSettingsEntry ScdmSettingsEntry;

ScdmSettingsEntry *        scdm_settings_entry_new               (void);
ScdmSettingsEntry *        scdm_settings_entry_copy              (ScdmSettingsEntry *entry);
void                      scdm_settings_entry_free              (ScdmSettingsEntry *entry);

const char *              scdm_settings_entry_get_key           (ScdmSettingsEntry *entry);
const char *              scdm_settings_entry_get_signature     (ScdmSettingsEntry *entry);
const char *              scdm_settings_entry_get_default_value (ScdmSettingsEntry *entry);
const char *              scdm_settings_entry_get_value         (ScdmSettingsEntry *entry);

void                      scdm_settings_entry_set_value         (ScdmSettingsEntry *entry,
                                                                const char       *value);

gboolean                  scdm_settings_parse_schemas           (const char  *file,
                                                                const char  *root,
                                                                GSList     **list);

gboolean                  scdm_settings_parse_value_as_boolean  (const char *value,
                                                                gboolean   *boole);
gboolean                  scdm_settings_parse_value_as_integer  (const char *value,
                                                                int        *intval);
gboolean                  scdm_settings_parse_value_as_double   (const char *value,
                                                                gdouble    *doubleval);

char *                    scdm_settings_parse_boolean_as_value  (gboolean    boolval);
char *                    scdm_settings_parse_integer_as_value  (int         intval);
char *                    scdm_settings_parse_double_as_value   (gdouble     doubleval);


G_END_DECLS

#endif /* __SCDM_SETTINGS_UTILS_H */
