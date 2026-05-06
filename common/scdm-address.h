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


#ifndef __GDM_ADDRESS_H
#define __GDM_ADDRESS_H

#include <glib-object.h>
#ifndef G_OS_WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#else
#include <winsock2.h>
#undef interface
#endif

G_BEGIN_DECLS

#define GDM_TYPE_ADDRESS (scdm_address_get_type ())
#define	scdm_sockaddr_len(sa) ((sa)->ss_family == AF_INET6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in))

typedef struct _ScdmAddress ScdmAddress;

GType                    scdm_address_get_type                  (void);

ScdmAddress *             scdm_address_new_from_sockaddr         (struct sockaddr *sa,
                                                                size_t           size);

int                      scdm_address_get_family_type           (ScdmAddress              *address);
struct sockaddr_storage *scdm_address_get_sockaddr_storage      (ScdmAddress              *address);
struct sockaddr_storage *scdm_address_peek_sockaddr_storage     (ScdmAddress              *address);

gboolean                 scdm_address_get_hostname              (ScdmAddress              *address,
                                                                char                   **hostname);
gboolean                 scdm_address_get_numeric_info          (ScdmAddress              *address,
                                                                char                   **numeric_hostname,
                                                                char                   **service);
gboolean                 scdm_address_is_local                  (ScdmAddress              *address);
gboolean                 scdm_address_is_loopback               (ScdmAddress              *address);

gboolean                 scdm_address_equal                     (ScdmAddress              *a,
                                                                ScdmAddress              *b);

ScdmAddress *             scdm_address_copy                      (ScdmAddress              *address);
void                     scdm_address_free                      (ScdmAddress              *address);


void                     scdm_address_debug                     (ScdmAddress              *address);

const GList *            scdm_address_peek_local_list           (void);


G_END_DECLS

#endif /* __GDM_ADDRESS_H */
