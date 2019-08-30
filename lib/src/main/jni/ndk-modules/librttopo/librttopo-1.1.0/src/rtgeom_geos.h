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
 *
 **********************************************************************/



#include "librttopo_geom.h"

/*
** Public prototypes for GEOS utility functions.
*/
RTGEOM *GEOS2RTGEOM(const RTCTX *ctx, const GEOSGeometry *geom, char want3d);
GEOSGeometry * RTGEOM2GEOS(const RTCTX *ctx, const RTGEOM *g, int autofix);
GEOSGeometry * GBOX2GEOS(const RTCTX *ctx, const RTGBOX *g);
GEOSGeometry * RTGEOM_GEOS_buildArea(const RTCTX *ctx, const GEOSGeometry* geom_in);

RTPOINTARRAY *ptarray_from_GEOSCoordSeq(const RTCTX *ctx, const GEOSCoordSequence *cs, char want3d);

/* Return (read-only) last geos error message */
const char *rtgeom_get_last_geos_error(const RTCTX *ctx);

extern void rtgeom_geos_error(const char *msg, void *ctx);

extern void rtgeom_geos_ensure_init(const RTCTX *ctx);

