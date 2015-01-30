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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "debug.h"

DebugLoggerFunc debug_log_func=NULL;

void DebugLogger(const char *file, unsigned int line, const char *func, const char *obj, const char *fmt,...) {
    va_list args;

    // Initialize variable arguments.
    va_start(args, fmt);

    if(debug_log_func) {
        char str[2048];

        if(obj) {
            sprintf(str,"%s:%d:%s:%s ",file,line,func,obj);
        }
        else {
            sprintf(str,"%s:%d:%s ",file,line,func);
        }
        debug_log_func(str);
        vsprintf(str,fmt,args);
        debug_log_func(str);
    }
    else {
        if(obj) {
            fprintf(stdout,"%s:%d:%s:%s ",file,line,func,obj);
        }
        else {
            fprintf(stdout,"%s:%d:%s ",file,line,func);
        }
        vfprintf(stdout,fmt,args);
        fflush(stdout);
    }
    va_end(args);
}

