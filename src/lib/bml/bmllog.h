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
 
#ifndef BMLLOG_H
#define BMLLOG_H

#include "bml.h"

typedef void (*BmlLogger)(const char *file, const int line, const char *func, const char *fmt, ...);

extern BmlLogger _log_printf;

BMLDebugLogger _bmllog_init (int debug_log_flags);

#ifdef USE_DEBUG
#  define TRACE_INIT(flags) _bmllog_init((flags))
#  define TRACE(...) _log_printf(__FILE__,__LINE__,__FUNCTION__,__VA_ARGS__)
#else
#  define TRACE_INIT(flags) NULL
#  define TRACE(...)
#endif

#endif /* BMLLOG_H */
