/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2012 Red Hat, Inc.
 * Copyright (C) 2012 Giovanni Campagna <scampa.giovanni@gmail.com>
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

#ifndef __GDM_CLIENT_H
#define __GDM_CLIENT_H

#include <glib-object.h>
#include "scdm-client-glue.h"

G_BEGIN_DECLS

#define GDM_TYPE_CLIENT (gdm_client_get_type ())
G_DECLARE_FINAL_TYPE (ScdmClient, gdm_client, GDM, CLIENT, GObject)

#define GDM_CLIENT_ERROR (gdm_client_error_quark ())

typedef enum _ScdmClientError {
        GDM_CLIENT_ERROR_GENERIC = 0,
} ScdmClientError;

GQuark             gdm_client_error_quark              (void);

ScdmClient         *gdm_client_new                      (void);
void               gdm_client_set_enabled_extensions   (ScdmClient *client,
                                                        const char * const * extensions);

void               gdm_client_open_reauthentication_channel (ScdmClient     *client,
                                                             const char           *username,
                                                             GCancellable         *cancellable,
                                                             GAsyncReadyCallback   callback,
                                                             gpointer              user_data);

ScdmUserVerifier   *gdm_client_open_reauthentication_channel_finish (ScdmClient  *client,
                                                                    GAsyncResult      *result,
                                                                    GError           **error);

ScdmUserVerifier   *gdm_client_open_reauthentication_channel_sync (ScdmClient *client,
                                                                  const char       *username,
                                                                  GCancellable     *cancellable,
                                                                  GError          **error);

void               gdm_client_get_user_verifier         (ScdmClient     *client,
                                                         GCancellable         *cancellable,
                                                         GAsyncReadyCallback   callback,
                                                         gpointer              user_data);
ScdmUserVerifier   *gdm_client_get_user_verifier_finish  (ScdmClient     *client,
                                                         GAsyncResult         *result,
                                                         GError              **error);
ScdmUserVerifier   *gdm_client_get_user_verifier_sync    (ScdmClient *client,
                                                         GCancellable     *cancellable,
                                                         GError          **error);

ScdmUserVerifierChoiceList *gdm_client_get_user_verifier_choice_list  (ScdmClient *client);

void               gdm_client_get_greeter               (ScdmClient     *client,
                                                         GCancellable         *cancellable,
                                                         GAsyncReadyCallback   callback,
                                                         gpointer              user_data);
ScdmGreeter        *gdm_client_get_greeter_finish        (ScdmClient *client,
                                                         GAsyncResult     *result,
                                                         GError          **error);
ScdmGreeter        *gdm_client_get_greeter_sync          (ScdmClient *client,
                                                         GCancellable     *cancellable,
                                                         GError          **error);

void               gdm_client_get_remote_greeter        (ScdmClient     *client,
                                                         GCancellable         *cancellable,
                                                         GAsyncReadyCallback   callback,
                                                         gpointer              user_data);
ScdmRemoteGreeter  *gdm_client_get_remote_greeter_finish (ScdmClient *client,
                                                         GAsyncResult     *result,
                                                         GError          **error);
ScdmRemoteGreeter  *gdm_client_get_remote_greeter_sync   (ScdmClient *client,
                                                         GCancellable     *cancellable,
                                                         GError          **error);

void               gdm_client_get_chooser               (ScdmClient     *client,
                                                         GCancellable         *cancellable,
                                                         GAsyncReadyCallback   callback,
                                                         gpointer              user_data);
ScdmChooser        *gdm_client_get_chooser_finish        (ScdmClient *client,
                                                         GAsyncResult     *result,
                                                         GError          **error);
ScdmChooser        *gdm_client_get_chooser_sync          (ScdmClient *client,
                                                         GCancellable     *cancellable,
                                                         GError          **error);

G_END_DECLS

#endif /* __GDM_CLIENT_H */
