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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "bmlipc.h"
#include "bmllog.h"

BmlIpcBuf *
bmlipc_new (void)
{
  return calloc (1, sizeof (BmlIpcBuf));
}

void
bmlipc_free (BmlIpcBuf * self)
{
  free (self);
}

void
bmlipc_clear (BmlIpcBuf * self)
{
  self->size = self->pos = self->io_error = 0;
}

// reader

static int
mem_read (BmlIpcBuf * self, void *ptr, int size, int n_items)
{
  int bytes = size * n_items;

  if (self->pos + bytes <= self->size) {
    memmove (ptr, &self->buffer[self->pos], bytes);
    self->pos += bytes;
    return n_items;
  }
  return 0;
}

int
bmlipc_read_int (BmlIpcBuf * self)
{
  int buffer = 0;

  if (mem_read (self, &buffer, sizeof (buffer), 1) != 1) {
    self->io_error = 1;
  }
  return buffer;
}

char *
bmlipc_read_string (BmlIpcBuf * self)
{
  char *buffer = &self->buffer[self->pos];
  char *c = buffer;
  int p = self->pos;

  while (*c && p < self->size) {
    c++;
    p++;
  }
  if (*c) {
    self->io_error = 1;
    return NULL;
  }
  self->pos = p;
  return buffer;
}

char *
bmlipc_read_data (BmlIpcBuf * self, int size)
{
  char *buffer = &self->buffer[self->pos];
  self->pos += size;
  return buffer;
}

void
bmlipc_read (BmlIpcBuf * self, StrPool * sp, char *fmt, ...)
{
  va_list var_args;
  char *t = fmt;

  va_start (var_args, fmt);
  while (t && *t) {
    switch (*t) {
      case 'i':{
        int *v = va_arg (var_args, int *);
        *v = bmlipc_read_int (self);
        break;
      }
      case 's':{
        const char **v = va_arg (var_args, const char **);
        *v = sp_intern (sp, bmlipc_read_string (self));
        break;
      }
      case 'd':{
        int *s = va_arg (var_args, int *);
        char *v = va_arg (var_args, char *);
        *s = bmlipc_read_int (self);
        memcpy (v, bmlipc_read_data (self, *s), *s);
        break;
      }
      default:
        TRACE ("unhandled type: '%c'", t);
        break;
    }
    t++;
  }
  va_end (var_args);
}


// writer

static int
mem_write (BmlIpcBuf * self, void *ptr, int size, int n_items)
{
  int bytes = size * n_items;

  if (self->pos + bytes <= IPC_BUF_SIZE) {
    memmove (&self->buffer[self->pos], ptr, bytes);
    self->pos += bytes;
    self->size += bytes;
    return n_items;
  }
  return 0;
}

void
bmlipc_write_int (BmlIpcBuf * self, int buffer)
{
  if (mem_write (self, &buffer, sizeof (buffer), 1) != 1) {
    self->io_error = 1;
  }
}

void
bmlipc_write_string (BmlIpcBuf * self, char *buffer)
{
  if (mem_write (self, buffer, strlen (buffer) + 1, 1) != 1) {
    self->io_error = 1;
  }
}

void
bmlipc_write_data (BmlIpcBuf * self, int size, char *buffer)
{
  if (mem_write (self, buffer, size, 1) != 1) {
    self->io_error = 1;
  }
}

void
bmlipc_write (BmlIpcBuf * self, char *fmt, ...)
{
  va_list var_args;
  char *t = fmt;

  va_start (var_args, fmt);
  while (t && *t) {
    switch (*t) {
      case 'i':{
        int v = va_arg (var_args, int);
        bmlipc_write_int (self, v);
        break;
      }
      case 's':{
        char *v = va_arg (var_args, char *);
        bmlipc_write_string (self, v);
        break;
      }
      case 'd':{
        int s = va_arg (var_args, int);
        char *v = va_arg (var_args, char *);
        bmlipc_write_int (self, s);
        bmlipc_write_data (self, s, v);
        break;
      }
      default:
        TRACE ("unhandled type: '%c'", t);
        break;
    }
    t++;
  }
  va_end (var_args);
}
