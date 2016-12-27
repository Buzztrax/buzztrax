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

/* we can configure the packge with --enable-debug to get LOG defined
 * if log is defined logging is compiled in otherwise out
 * if it is in we can still control the amount of logging by setting bits in
 * BML_DEBUG:
 *   1 : only logging from the windows adapter (BuzzMachineLoader.dll)
 *   2 : only logging from bml and dllwrapper
 */

#include "config.h"

#include <sys/time.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "bmllog.h"

static double _first_ts = 0.0;

static double
_get_timestamp (void)
{
#ifdef HAVE_CLOCK_GETTIME
  struct timespec ts;

  clock_gettime (CLOCK_MONOTONIC, &ts);
  return (((double) ts.tv_sec + (double) ts.tv_nsec * 1.0e-9) - _first_ts);
#else
  struct timeval ts;

  gettimeofday (&ts, NULL);
  return (((double) ts.tv_sec + (double) ts.tv_usec * 1.0e-6) - _first_ts);
#endif
}

static void
_log_stdout_printf (const char *file, const int line, const char *func,
    const char *fmt, ...)
{
  va_list ap;
  char msg[4000];

  va_start (ap, fmt);
  vsnprintf (msg, 4000, fmt, ap);
  va_end (ap);
  msg[3999] = '\0';
  fprintf (stderr, "%10.4lf: %s:%d:%s: %s", _get_timestamp (),
      (file ? file : ""), line, (func ? func : ""), msg);
}

static void
_log_null_printf (const char *file, const int line, const char *func,
    const char *fmt, ...)
{
}

// name is also used in dllwrapper/debugtools.h
BmlLogger _log_printf = _log_null_printf;

// collect lines up to 1000 chars
static void
_log_stdout_logger (char *str)
{
#ifdef USE_DEBUG
  static char lbuf[1000 + 1];
  static int p = 0;
  int i = 0;

  if (!str)
    return;

  while ((str[i] != '\0') && (str[i] != '\n')) {
    if (p < 1000)
      lbuf[p++] = str[i++];
  }
  if (str[i] == '\n') {
    lbuf[p] = '\0';
    _log_printf (NULL, 0, NULL, "%s\n", lbuf);
    p = 0;
  }
#endif
}

static void
_log_null_logger (char *str)
{
}

BMLDebugLogger
_bmllog_init (int debug_log_flags)
{
  BMLDebugLogger logger = _log_null_logger;

  _first_ts = _get_timestamp ();
  // for BuzzMachineLoader
  if (debug_log_flags & 0x1) {
    logger = _log_stdout_logger;
  }
  // for libbml itself
  if (debug_log_flags & 0x2) {
    _log_printf = _log_stdout_printf;
  }
  return logger;
}
