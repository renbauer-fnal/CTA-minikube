/*
 * @project        The CERN Tape Archive (CTA)
 * @copyright      Copyright(C) 2003-2021 CERN
 * @license        This program is free software: you can redistribute it and/or modify
 *                 it under the terms of the GNU General Public License as published by
 *                 the Free Software Foundation, either version 3 of the License, or
 *                 (at your option) any later version.
 *
 *                 This program is distributed in the hope that it will be useful,
 *                 but WITHOUT ANY WARRANTY; without even the implied warranty of
 *                 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *                 GNU General Public License for more details.
 *
 *                 You should have received a copy of the GNU General Public License
 *                 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "osdep.h"

#include <sys/types.h>
#include <sys/socket.h>

EXTERN_C int   netread (int, char *, int); /* Network receive function */
EXTERN_C int   netwrite(const int, const char *, const int); /* Network send function */
EXTERN_C char* neterror(void); /* Network error function */

EXTERN_C ssize_t  netread_timeout (int, void *, ssize_t, int);
EXTERN_C ssize_t  netwrite_timeout (int, void *, ssize_t, int);
EXTERN_C int netconnect_timeout (int, struct sockaddr *, size_t, int);
