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
 * Copyright (C) 2004-2015 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2008-2011 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright (C) 2008 Mark Cave-Ayland <mark.cave-ayland@siriusit.co.uk>
 *
 **********************************************************************/


#include "rttopo_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "librttopo_geom_internal.h"

#ifndef EPSILON
#define EPSILON        1.0E-06
#endif
#ifndef FPeq
#define FPeq(A,B)     (fabs((A) - (B)) <= EPSILON)
#endif



RTGBOX *
box2d_clone(const RTCTX *ctx, const RTGBOX *in)
{
  RTGBOX *ret = rtalloc(ctx, sizeof(RTGBOX));
  memcpy(ret, in, sizeof(RTGBOX));
  return ret;
}
