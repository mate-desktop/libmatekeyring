/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-keyring-private.h - private header for keyring

   Copyright (C) 2003 Red Hat, Inc

   The Mate Keyring Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Keyring Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
   Boston, MA 02110-1301, USA.

   Author: Alexander Larsson <alexl@redhat.com>
*/

#ifndef MATE_KEYRING_PRIVATE_H
#define MATE_KEYRING_PRIVATE_H

#include "mate-keyring.h"

#include "gkr-callback.h"
#include "gkr-operation.h"

#include <dbus/dbus.h>

struct MateKeyringApplicationRef {
	char *display_name;
	char *pathname;
};

struct MateKeyringAccessControl {
	MateKeyringApplicationRef *application; /* null for all */
	MateKeyringAccessType types_allowed;
};

struct MateKeyringInfo {
	gboolean lock_on_idle;
	guint32 lock_timeout;
	time_t mtime;
	time_t ctime;
	gboolean is_locked;
};

struct MateKeyringItemInfo {
	MateKeyringItemType type;
	char *display_name;
	char *secret;
	time_t mtime;
	time_t ctime;
};

void   _mate_keyring_memory_dump (void);
extern gboolean mate_keyring_memory_warning;

#define BROKEN                         MATE_KEYRING_RESULT_IO_ERROR

#define SECRETS_SERVICE                "org.freedesktop.secrets"
#define SERVICE_PATH                   "/org/freedesktop/secrets"
#define COLLECTION_INTERFACE           "org.freedesktop.Secret.Collection"
#define ITEM_INTERFACE                 "org.freedesktop.Secret.Item"
#define PROMPT_INTERFACE               "org.freedesktop.Secret.Prompt"
#define SERVICE_INTERFACE              "org.freedesktop.Secret.Service"
#define COLLECTION_PREFIX              "/org/freedesktop/secrets/collection/"
#define COLLECTION_DEFAULT             "/org/freedesktop/secrets/aliases/default"

#define ERROR_IS_LOCKED                "org.freedesktop.Secret.Error.IsLocked"
#define ERROR_NO_SESSION               "org.freedesktop.Secret.Error.NoSession"
#define ERROR_NO_SUCH_OBJECT           "org.freedesktop.Secret.Error.NoSuchObject"

#define NORMAL_ALLOCATOR  ((EggBufferAllocator)g_realloc)
#define SECURE_ALLOCATOR  ((EggBufferAllocator)mate_keyring_memory_realloc)

#endif /* MATE_KEYRING_PRIVATE_H */
