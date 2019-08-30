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
 * Copyright (C) 2004-2016 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2006 Mark Leslie <mark.leslie@lisasoft.com>
 * Copyright (C) 2008-2009 Mark Cave-Ayland <mark.cave-ayland@siriusit.co.uk>
 * Copyright (C) 2009-2015 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright (C) 2010 Olivier Courtin <olivier.courtin@camptocamp.com>
 *
 **********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h> /* for tolower */

#include "rttopo_config.h"
/*#define RTGEOM_DEBUG_LEVEL 4*/
#include "librttopo_geom_internal.h"
#include "rtgeom_log.h"

/* Global variables */

/* Default allocators */
static void * default_allocator(size_t size);
static void default_freeor(void *mem);
static void * default_reallocator(void *mem, size_t size);

#define RT_MSG_MAXLEN 256

static char *rtgeomTypeName[] =
{
  "Unknown",
  "Point",
  "LineString",
  "Polygon",
  "MultiPoint",
  "MultiLineString",
  "MultiPolygon",
  "GeometryCollection",
  "CircularString",
  "CompoundCurve",
  "CurvePolygon",
  "MultiCurve",
  "MultiSurface",
  "PolyhedralSurface",
  "Triangle",
  "Tin"
};

/*
 * Default allocators
 *
 * We include some default allocators that use malloc/free/realloc
 * along with stdout/stderr since this is the most common use case
 *
 */

static void *
default_allocator(size_t size)
{
  void *mem = malloc(size);
  return mem;
}

static void
default_freeor(void *mem)
{
  free(mem);
}

static void *
default_reallocator(void *mem, size_t size)
{
  void *ret = realloc(mem, size);
  return ret;
}

/*
 * Default rtnotice/rterror handlers
 *
 * Since variadic functions cannot pass their parameters directly, we need
 * wrappers for these functions to convert the arguments into a va_list
 * structure.
 */

static void
default_noticereporter(const char *fmt, va_list ap, void *arg)
{
  char msg[RT_MSG_MAXLEN+1];
  vsnprintf (msg, RT_MSG_MAXLEN, fmt, ap);
  msg[RT_MSG_MAXLEN]='\0';
  printf("%s\n", msg);
}

static void
default_debuglogger(int level, const char *fmt, va_list ap, void *arg)
{
  char msg[RT_MSG_MAXLEN+1];
  if ( RTGEOM_DEBUG_LEVEL >= level )
  {
    /* Space pad the debug output */
    int i;
    for ( i = 0; i < level; i++ )
      msg[i] = ' ';
    vsnprintf(msg+i, RT_MSG_MAXLEN-i, fmt, ap);
    msg[RT_MSG_MAXLEN]='\0';
    printf("%s\n", msg);
  }
}

static void
default_errorreporter(const char *fmt, va_list ap, void *arg)
{
  char msg[RT_MSG_MAXLEN+1];
  vsnprintf (msg, RT_MSG_MAXLEN, fmt, ap);
  msg[RT_MSG_MAXLEN]='\0';
  fprintf(stderr, "%s\n", msg);
  exit(1);
}

RTCTX *
rtgeom_init(rtallocator allocator,
                   rtreallocator reallocator,
                   rtfreeor freeor)
{
  RTCTX *ctx = allocator ? allocator(sizeof(RTCTX))
                 : default_allocator(sizeof(RTCTX));

  memset(ctx, '\0', sizeof(RTCTX));

  ctx->rtalloc_var = default_allocator;
  ctx->rtrealloc_var = default_reallocator;
  ctx->rtfree_var = default_freeor;

  if ( allocator ) ctx->rtalloc_var = allocator;
  if ( reallocator ) ctx->rtrealloc_var = reallocator;
  if ( freeor ) ctx->rtfree_var = freeor;

  ctx->notice_logger = default_noticereporter;
  ctx->error_logger = default_errorreporter;
  ctx->debug_logger = default_debuglogger;

  return ctx;
}

void
rtgeom_finish(RTCTX *ctx)
{
  if (ctx->gctx != NULL)
    GEOS_finish_r(ctx->gctx);
  ctx->rtfree_var(ctx);
}

void
rtgeom_set_error_logger(RTCTX *ctx, rtreporter logger, void *arg)
{
  ctx->error_logger = logger;
  ctx->error_logger_arg = arg;
}

void
rtgeom_set_notice_logger(RTCTX *ctx, rtreporter logger, void *arg)
{
  ctx->notice_logger = logger;
  ctx->notice_logger_arg = arg;
}

void
rtgeom_set_debug_logger(RTCTX *ctx, rtdebuglogger logger, void *arg)
{
  ctx->debug_logger = logger;
  ctx->debug_logger_arg = arg;
}

void
rtnotice(const RTCTX *ctx, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);

  /* Call the supplied function */
  (*ctx->notice_logger)(fmt, ap, ctx->notice_logger_arg);

  va_end(ap);
}

void
rterror(const RTCTX *ctx, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);

  /* Call the supplied function */
  (*ctx->error_logger)(fmt, ap, ctx->error_logger_arg);

  va_end(ap);
}

void
rtdebug(const RTCTX *ctx, int level, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);

  /* Call the supplied function */
  (*ctx->debug_logger)(level, fmt, ap, ctx->debug_logger_arg);

  va_end(ap);
}



const char*
rttype_name(const RTCTX *ctx, uint8_t type)
{
  if ( type > 15 )
  {
    /* assert(0); */
    return "Invalid type";
  }
  return rtgeomTypeName[(int ) type];
}

void *
rtalloc(const RTCTX *ctx, size_t size)
{
  void *mem = ctx->rtalloc_var(size);
  RTDEBUGF(ctx, 5, "rtalloc: %d@%p", size, mem);
  return mem;
}

void *
rtrealloc(const RTCTX *ctx, void *mem, size_t size)
{
  RTDEBUGF(ctx, 5, "rtrealloc: %d@%p", size, mem);
  return ctx->rtrealloc_var(mem, size);
}

void
rtfree(const RTCTX *ctx, void *mem)
{
  ctx->rtfree_var(mem);
}

/*
 * Removes trailing zeros and dot for a %f formatted number.
 * Modifies input.
 */
void
trim_trailing_zeros(const RTCTX *ctx, char *str)
{
  char *ptr, *totrim=NULL;
  int len;
  int i;

  RTDEBUGF(ctx, 3, "input: %s", str);

  ptr = strchr(str, '.');
  if ( ! ptr ) return; /* no dot, no decimal digits */

  RTDEBUGF(ctx, 3, "ptr: %s", ptr);

  len = strlen(ptr);
  for (i=len-1; i; i--)
  {
    if ( ptr[i] != '0' ) break;
    totrim=&ptr[i];
  }
  if ( totrim )
  {
    if ( ptr == totrim-1 ) *ptr = '\0';
    else *totrim = '\0';
  }

  RTDEBUGF(ctx, 3, "output: %s", str);
}

/*
 * Returns a new string which contains a maximum of maxlength characters starting
 * from startpos and finishing at endpos (0-based indexing). If the string is
 * truncated then the first or last characters are replaced by "..." as
 * appropriate.
 *
 * The caller should specify start or end truncation by setting the truncdirection
 * parameter as follows:
 *    0 - start truncation (i.e. characters are removed from the beginning)
 *    1 - end trunctation (i.e. characters are removed from the end)
 */

char * rtmessage_truncate(const RTCTX *ctx, char *str, int startpos, int endpos, int maxlength, int truncdirection)
{
  char *output;
  char *outstart;

  /* Allocate space for new string */
  output = rtalloc(ctx, maxlength + 4);
  output[0] = '\0';

  /* Start truncation */
  if (truncdirection == 0)
  {
    /* Calculate the start position */
    if (endpos - startpos < maxlength)
    {
      outstart = str + startpos;
      strncat(output, outstart, endpos - startpos + 1);
    }
    else
    {
      if (maxlength >= 3)
      {
        /* Add "..." prefix */
        outstart = str + endpos + 1 - maxlength + 3;
        strncat(output, "...", 3);
        strncat(output, outstart, maxlength - 3);
      }
      else
      {
        /* maxlength is too small; just output "..." */
        strncat(output, "...", 3);
      }
    }
  }

  /* End truncation */
  if (truncdirection == 1)
  {
    /* Calculate the end position */
    if (endpos - startpos < maxlength)
    {
      outstart = str + startpos;
      strncat(output, outstart, endpos - startpos + 1);
    }
    else
    {
      if (maxlength >= 3)
      {
        /* Add "..." suffix */
        outstart = str + startpos;
        strncat(output, outstart, maxlength - 3);
        strncat(output, "...", 3);
      }
      else
      {
        /* maxlength is too small; just output "..." */
        strncat(output, "...", 3);
      }
    }
  }

  return output;
}


char
getMachineEndian(const RTCTX *ctx)
{
  static int endian_check_int = 1; /* dont modify this!!! */

  return *((char *) &endian_check_int); /* 0 = big endian | xdr,
                                         * 1 = little endian | ndr
                                         */
}


void
error_if_srid_mismatch(const RTCTX *ctx, int srid1, int srid2)
{
  if ( srid1 != srid2 )
  {
    rterror(ctx, "Operation on mixed SRID geometries");
  }
}

int
clamp_srid(const RTCTX *ctx, int srid)
{
  int newsrid = srid;

  if ( newsrid <= 0 ) {
    if ( newsrid != SRID_UNKNOWN ) {
      newsrid = SRID_UNKNOWN;
      rtnotice(ctx, "SRID value %d converted to the officially unknown SRID value %d", srid, newsrid);
    }
  } else if ( srid > SRID_MAXIMUM ) {
    newsrid = SRID_USER_MAXIMUM + 1 +
      /* -1 is to reduce likelyhood of clashes */
      /* NOTE: must match implementation in postgis_restore.pl */
      ( srid % ( SRID_MAXIMUM - SRID_USER_MAXIMUM - 1 ) );
    rtnotice(ctx, "SRID value %d > SRID_MAXIMUM converted to %d", srid, newsrid);
  }

  return newsrid;
}

