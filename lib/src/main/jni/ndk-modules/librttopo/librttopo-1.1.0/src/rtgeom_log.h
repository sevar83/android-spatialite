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
 * Copyright 2011 Sandro Santilli <strk@kbt.io>
 * Copyright 2008 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright 2007-2008 Mark Cave-Ayland
 * Copyright 2001-2006 Refractions Research Inc.
 *
 **********************************************************************/



#ifndef RTGEOM_LOG_H
#define RTGEOM_LOG_H 1

#include "librttopo_geom_internal.h"
#include <stdarg.h>

/*
 * Debug macros
 */
#if RTGEOM_DEBUG_LEVEL > 0

/* Display a notice at the given debug level */
#define RTDEBUG(ctx, level, msg) \
        do { \
            if (RTGEOM_DEBUG_LEVEL >= level) \
              rtdebug(ctx, level, "[%s:%s:%d] " msg, __FILE__, __func__, __LINE__); \
        } while (0);

/* Display a formatted notice at the given debug level
 * (like printf, with variadic arguments) */
#define RTDEBUGF(ctx, level, msg, ...) \
        do { \
            if (RTGEOM_DEBUG_LEVEL >= level) \
              rtdebug(ctx, level, "[%s:%s:%d] " msg, \
                __FILE__, __func__, __LINE__, __VA_ARGS__); \
        } while (0);

#else /* RTGEOM_DEBUG_LEVEL <= 0 */

/* Empty prototype that can be optimised away by the compiler
 * for non-debug builds */
#define RTDEBUG(ctx, level, msg) \
        ((void) 0)

/* Empty prototype that can be optimised away by the compiler
 * for non-debug builds */
#define RTDEBUGF(ctx, level, msg, ...) \
        ((void) 0)

#endif /* RTGEOM_DEBUG_LEVEL <= 0 */

/**
 * Write a notice out to the notice handler.
 *
 * Uses standard printf() substitutions.
 * Use for messages you artays want output.
 * For debugging, use RTDEBUG() or RTDEBUGF().
 * @ingroup logging
 */
void rtnotice(const RTCTX *ctx, const char *fmt, ...);

/**
 * Write a notice out to the error handler.
 *
 * Uses standard printf() substitutions.
 * Use for errors you artays want output.
 * For debugging, use RTDEBUG() or RTDEBUGF().
 * @ingroup logging
 */
void rterror(const RTCTX *ctx, const char *fmt, ...);

/**
 * Write a debug message out.
 * Don't call this function directly, use the
 * macros, RTDEBUG() or RTDEBUGF(), for
 * efficiency.
 * @ingroup logging
 */
void rtdebug(const RTCTX *ctx, int level, const char *fmt, ...);



#endif /* RTGEOM_LOG_H */
