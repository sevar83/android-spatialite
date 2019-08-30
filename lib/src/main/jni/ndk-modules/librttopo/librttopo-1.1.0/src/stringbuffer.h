/**********************************************************************
 *
 * rttopo - topology library
 * http://git.osgeo.org/gitea/rttopo/librttopo
 *
 * rttopo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * rttopo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with rttopo.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright 2002 Thamer Alharbash
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 **********************************************************************/



#ifndef _STRINGBUFFER_H
#define _STRINGBUFFER_H 1

#include "librttopo_geom_internal.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#define STRINGBUFFER_STARTSIZE 128

typedef struct
{
  size_t capacity;
  char *str_end;
  char *str_start;
}
stringbuffer_t;

extern stringbuffer_t *stringbuffer_create_with_size(const RTCTX *ctx, size_t size);
extern stringbuffer_t *stringbuffer_create(const RTCTX *ctx);
extern void stringbuffer_destroy(const RTCTX *ctx, stringbuffer_t *sb);
extern void stringbuffer_clear(const RTCTX *ctx, stringbuffer_t *sb);
void stringbuffer_set(const RTCTX *ctx, stringbuffer_t *sb, const char *s);
void stringbuffer_copy(const RTCTX *ctx, stringbuffer_t *sb, stringbuffer_t *src);
extern void stringbuffer_append(const RTCTX *ctx, stringbuffer_t *sb, const char *s);
extern int stringbuffer_aprintf(const RTCTX *ctx, stringbuffer_t *sb, const char *fmt, ...);
extern const char *stringbuffer_getstring(const RTCTX *ctx, stringbuffer_t *sb);
extern char *stringbuffer_getstringcopy(const RTCTX *ctx, stringbuffer_t *sb);
extern int stringbuffer_getlength(const RTCTX *ctx, stringbuffer_t *sb);
extern char stringbuffer_lastchar(const RTCTX *ctx, stringbuffer_t *s);
extern int stringbuffer_trim_trailing_white(const RTCTX *ctx, stringbuffer_t *s);
extern int stringbuffer_trim_trailing_zeroes(const RTCTX *ctx, stringbuffer_t *s);

#endif /* _STRINGBUFFER_H */
