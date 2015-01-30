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

#ifndef BMLW_H
#define BMLW_H

typedef enum {
  BM__QUIT = 0,
  BM_SET_MASTER_INFO,
  BM_OPEN,
  BM_CLOSE,
  BM_GET_MACHINE_INFO,
  BM_GET_GLOBAL_PARAMETER_INFO,
  BM_GET_TRACK_PARAMETER_INFO,
  BM_GET_ATTRIBUTE_INFO,
  BM_DESCRIBE_GLOBAL_VALUE,
  BM_DESCRIBE_TRACK_VALUE,
  BM_NEW,
  BM_FREE,
  BM_INIT,
  BM_GET_TRACK_PARAMETER_VALUE,
  BM_SET_TRACK_PARAMETER_VALUE,
  BM_GET_GLOBAL_PARAMETER_VALUE,
  BM_SET_GLOBAL_PARAMETER_VALUE,
  BM_GET_ATTRIBUTE_VALUE,
  BM_SET_ATTRIBUTE_VALUE,
  BM_TICK,
  BM_WORK,
  BM_WORK_M2S,
  BM_STOP,
  BM_ATTRIBUTES_CHANGED,
  BM_SET_NUM_TRACKS,
  BM_SET_CALLBACKS
} BmAPI;

int _bmlw_setup(BMLDebugLogger logger);
void _bmlw_finalize(void);

#endif // BMLW_H
