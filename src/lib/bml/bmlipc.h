/* Buzz Machine Loader
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BMLIPC_H
#define BMLIPC_H

#include "strpool.h"

// TODO(ensonic): figure max message block size
// buzzmachines use MAX_BUFFER_LENGTH = 256 * sizeof(float) = 1024 bytes for
// wave data
#define IPC_BUF_SIZE 2048

typedef struct {
  char buffer[IPC_BUF_SIZE];
  int pos;
  int size;
  int io_error;
} BmlIpcBuf;

#define IPC_BUF_INIT {{0,},0, }

BmlIpcBuf *bmlipc_new (void);
void bmlipc_free (BmlIpcBuf *self);

void bmlipc_clear (BmlIpcBuf * self);

int bmlipc_read_int (BmlIpcBuf * self);
char *bmlipc_read_string (BmlIpcBuf * self);
char *bmlipc_read_data (BmlIpcBuf * self, int size);
void bmlipc_read (BmlIpcBuf * self, StrPool *sp, char *fmt, ...);

void bmlipc_write_int (BmlIpcBuf * self, int buffer);
void bmlipc_write_string (BmlIpcBuf * self, char *buffer);
void bmlipc_write_data (BmlIpcBuf * self, int size, char *buffer);
void bmlipc_write (BmlIpcBuf * self, char *fmt, ...);

#endif // BMLIPC_H
