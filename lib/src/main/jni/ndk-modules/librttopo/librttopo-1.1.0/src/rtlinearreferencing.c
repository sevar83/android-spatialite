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
 * Copyright (C) 2015 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2011 Paul Ramsey
 *
 **********************************************************************/



#include "rttopo_config.h"
#include "librttopo_geom_internal.h"
#include "rtgeom_log.h"
#include "measures3d.h"

static int
segment_locate_along(const RTCTX *ctx, const RTPOINT4D *p1, const RTPOINT4D *p2, double m, double offset, RTPOINT4D *pn)
{
  double m1 = p1->m;
  double m2 = p2->m;
  double mprop;

  /* M is out of range, no new point generated. */
  if ( (m < FP_MIN(m1,m2)) || (m > FP_MAX(m1,m2)) )
  {
    return RT_FALSE;
  }

  if( m1 == m2 )
  {
    /* Degenerate case: same M on both points.
       If they are the same point we just return one of them. */
    if ( p4d_same(ctx, p1,p2) )
    {
      *pn = *p1;
      return RT_TRUE;
    }
    /* If the points are different we can out.
       Correct behavior is probably an mprop of 0.5? */
    rterror(ctx, "Zero measure-length line encountered!");
    return RT_FALSE;
  }

  /* M is in range, new point to be generated. */
  mprop = (m - m1) / (m2 - m1);
  pn->x = p1->x + (p2->x - p1->x) * mprop;
  pn->y = p1->y + (p2->y - p1->y) * mprop;
  pn->z = p1->z + (p2->z - p1->z) * mprop;
  pn->m = m;

  /* Offset to the left or right, if necessary. */
  if ( offset != 0.0 )
  {
    double theta = atan2(p2->y - p1->y, p2->x - p1->x);
    pn->x -= sin(theta) * offset;
    pn->y += cos(theta) * offset;
  }

  return RT_TRUE;
}


static RTPOINTARRAY*
ptarray_locate_along(const RTCTX *ctx, const RTPOINTARRAY *pa, double m, double offset)
{
  int i;
  RTPOINT4D p1, p2, pn;
  RTPOINTARRAY *dpa = NULL;

  /* Can't do anything with degenerate point arrays */
  if ( ! pa || pa->npoints < 2 ) return NULL;

  /* Walk through each segment in the point array */
  for ( i = 1; i < pa->npoints; i++ )
  {
    rt_getPoint4d_p(ctx, pa, i-1, &p1);
    rt_getPoint4d_p(ctx, pa, i, &p2);

    /* No derived point? Move to next segment. */
    if ( segment_locate_along(ctx, &p1, &p2, m, offset, &pn) == RT_FALSE )
      continue;

    /* No pointarray, make a fresh one */
    if ( dpa == NULL )
      dpa = ptarray_construct_empty(ctx, ptarray_has_z(ctx, pa), ptarray_has_m(ctx, pa), 8);

    /* Add our new point to the array */
    ptarray_append_point(ctx, dpa, &pn, 0);
  }

  return dpa;
}

static RTMPOINT*
rtline_locate_along(const RTCTX *ctx, const RTLINE *rtline, double m, double offset)
{
  RTPOINTARRAY *opa = NULL;
  RTMPOINT *mp = NULL;
  RTGEOM *rtg = rtline_as_rtgeom(ctx, rtline);
  int hasz, hasm, srid;

  /* Return degenerates upwards */
  if ( ! rtline ) return NULL;

  /* Create empty return shell */
  srid = rtgeom_get_srid(ctx, rtg);
  hasz = rtgeom_has_z(ctx, rtg);
  hasm = rtgeom_has_m(ctx, rtg);

  if ( hasm )
  {
    /* Find points along */
    opa = ptarray_locate_along(ctx, rtline->points, m, offset);
  }
  else
  {
    RTLINE *rtline_measured = rtline_measured_from_rtline(ctx, rtline, 0.0, 1.0);
    opa = ptarray_locate_along(ctx, rtline_measured->points, m, offset);
    rtline_free(ctx, rtline_measured);
  }

  /* Return NULL as EMPTY */
  if ( ! opa )
    return rtmpoint_construct_empty(ctx, srid, hasz, hasm);

  /* Convert pointarray into a multipoint */
  mp = rtmpoint_construct(ctx, srid, opa);
  ptarray_free(ctx, opa);
  return mp;
}

static RTMPOINT*
rtmline_locate_along(const RTCTX *ctx, const RTMLINE *rtmline, double m, double offset)
{
  RTMPOINT *rtmpoint = NULL;
  RTGEOM *rtg = rtmline_as_rtgeom(ctx, rtmline);
  int i, j;

  /* Return degenerates upwards */
  if ( (!rtmline) || (rtmline->ngeoms < 1) ) return NULL;

  /* Construct return */
  rtmpoint = rtmpoint_construct_empty(ctx, rtgeom_get_srid(ctx, rtg), rtgeom_has_z(ctx, rtg), rtgeom_has_m(ctx, rtg));

  /* Locate along each sub-line */
  for ( i = 0; i < rtmline->ngeoms; i++ )
  {
    RTMPOINT *along = rtline_locate_along(ctx, rtmline->geoms[i], m, offset);
    if ( along )
    {
      if ( ! rtgeom_is_empty(ctx, (RTGEOM*)along) )
      {
        for ( j = 0; j < along->ngeoms; j++ )
        {
          rtmpoint_add_rtpoint(ctx, rtmpoint, along->geoms[j]);
        }
      }
      /* Free the containing geometry, but leave the sub-geometries around */
      along->ngeoms = 0;
      rtmpoint_free(ctx, along);
    }
  }
  return rtmpoint;
}

static RTMPOINT*
rtpoint_locate_along(const RTCTX *ctx, const RTPOINT *rtpoint, double m, double offset)
{
  double point_m = rtpoint_get_m(ctx, rtpoint);
  RTGEOM *rtg = rtpoint_as_rtgeom(ctx, rtpoint);
  RTMPOINT *r = rtmpoint_construct_empty(ctx, rtgeom_get_srid(ctx, rtg), rtgeom_has_z(ctx, rtg), rtgeom_has_m(ctx, rtg));
  if ( FP_EQUALS(m, point_m) )
  {
    rtmpoint_add_rtpoint(ctx, r, rtpoint_clone(ctx, rtpoint));
  }
  return r;
}

static RTMPOINT*
rtmpoint_locate_along(const RTCTX *ctx, const RTMPOINT *rtin, double m, double offset)
{
  RTGEOM *rtg = rtmpoint_as_rtgeom(ctx, rtin);
  RTMPOINT *rtout = NULL;
  int i;

  /* Construct return */
  rtout = rtmpoint_construct_empty(ctx, rtgeom_get_srid(ctx, rtg), rtgeom_has_z(ctx, rtg), rtgeom_has_m(ctx, rtg));

  for ( i = 0; i < rtin->ngeoms; i++ )
  {
    double point_m = rtpoint_get_m(ctx, rtin->geoms[i]);
    if ( FP_EQUALS(m, point_m) )
    {
      rtmpoint_add_rtpoint(ctx, rtout, rtpoint_clone(ctx, rtin->geoms[i]));
    }
  }

  return rtout;
}

RTGEOM*
rtgeom_locate_along(const RTCTX *ctx, const RTGEOM *rtin, double m, double offset)
{
  if ( ! rtin ) return NULL;

  if ( ! rtgeom_has_m(ctx, rtin) )
    rterror(ctx, "Input geometry does not have a measure dimension");

  switch (rtin->type)
  {
  case RTPOINTTYPE:
    return (RTGEOM*)rtpoint_locate_along(ctx, (RTPOINT*)rtin, m, offset);
  case RTMULTIPOINTTYPE:
    return (RTGEOM*)rtmpoint_locate_along(ctx, (RTMPOINT*)rtin, m, offset);
  case RTLINETYPE:
    return (RTGEOM*)rtline_locate_along(ctx, (RTLINE*)rtin, m, offset);
  case RTMULTILINETYPE:
    return (RTGEOM*)rtmline_locate_along(ctx, (RTMLINE*)rtin, m, offset);
  /* Only line types supported right now */
  /* TO DO: CurveString, CompoundCurve, MultiCurve */
  /* TO DO: Point, MultiPoint */
  default:
    rterror(ctx, "Only linear geometries are supported, %s provided.",rttype_name(ctx, rtin->type));
    return NULL;
  }
  return NULL;
}

/**
* Given a RTPOINT4D and an ordinate number, return
* the value of the ordinate.
* @param p input point
* @param ordinate number (1=x, 2=y, 3=z, 4=m)
* @return d value at that ordinate
*/
double rtpoint_get_ordinate(const RTCTX *ctx, const RTPOINT4D *p, char ordinate)
{
  if ( ! p )
  {
    rterror(ctx, "Null input geometry.");
    return 0.0;
  }

  if ( ! ( ordinate == 'X' || ordinate == 'Y' || ordinate == 'Z' || ordinate == 'M' ) )
  {
    rterror(ctx, "Cannot extract %c ordinate.", ordinate);
    return 0.0;
  }

  if ( ordinate == 'X' )
    return p->x;
  if ( ordinate == 'Y' )
    return p->y;
  if ( ordinate == 'Z' )
    return p->z;
  if ( ordinate == 'M' )
    return p->m;

  /* X */
  return p->x;

}

/**
* Given a point, ordinate number and value, set that ordinate on the
* point.
*/
void rtpoint_set_ordinate(const RTCTX *ctx, RTPOINT4D *p, char ordinate, double value)
{
  if ( ! p )
  {
    rterror(ctx, "Null input geometry.");
    return;
  }

  if ( ! ( ordinate == 'X' || ordinate == 'Y' || ordinate == 'Z' || ordinate == 'M' ) )
  {
    rterror(ctx, "Cannot set %c ordinate.", ordinate);
    return;
  }

  RTDEBUGF(ctx, 4, "    setting ordinate %c to %g", ordinate, value);

  switch ( ordinate )
  {
  case 'X':
    p->x = value;
    return;
  case 'Y':
    p->y = value;
    return;
  case 'Z':
    p->z = value;
    return;
  case 'M':
    p->m = value;
    return;
  }
}

/**
* Given two points, a dimensionality, an ordinate, and an interpolation value
* generate a new point that is proportionally between the input points,
* using the values in the provided dimension as the scaling factors.
*/
int point_interpolate(const RTCTX *ctx, const RTPOINT4D *p1, const RTPOINT4D *p2, RTPOINT4D *p, int hasz, int hasm, char ordinate, double interpolation_value)
{
  static char* dims = "XYZM";
  double p1_value = rtpoint_get_ordinate(ctx, p1, ordinate);
  double p2_value = rtpoint_get_ordinate(ctx, p2, ordinate);
  double proportion;
  int i = 0;

  if ( ! ( ordinate == 'X' || ordinate == 'Y' || ordinate == 'Z' || ordinate == 'M' ) )
  {
    rterror(ctx, "Cannot set %c ordinate.", ordinate);
    return 0;
  }

  if ( FP_MIN(p1_value, p2_value) > interpolation_value ||
          FP_MAX(p1_value, p2_value) < interpolation_value )
  {
    rterror(ctx, "Cannot interpolate to a value (%g) not between the input points (%g, %g).", interpolation_value, p1_value, p2_value);
    return 0;
  }

  proportion = fabs((interpolation_value - p1_value) / (p2_value - p1_value));

  for ( i = 0; i < 4; i++ )
  {
    double newordinate = 0.0;
    if ( dims[i] == 'Z' && ! hasz ) continue;
    if ( dims[i] == 'M' && ! hasm ) continue;
    p1_value = rtpoint_get_ordinate(ctx, p1, dims[i]);
    p2_value = rtpoint_get_ordinate(ctx, p2, dims[i]);
    newordinate = p1_value + proportion * (p2_value - p1_value);
    rtpoint_set_ordinate(ctx, p, dims[i], newordinate);
    RTDEBUGF(ctx, 4, "   clip ordinate(%c) p1_value(%g) p2_value(%g) proportion(%g) newordinate(%g) ", dims[i], p1_value, p2_value, proportion, newordinate );
  }

  return 1;
}


/**
* Clip an input POINT between two values, on any ordinate input.
*/
RTCOLLECTION*
rtpoint_clip_to_ordinate_range(const RTCTX *ctx, const RTPOINT *point, char ordinate, double from, double to)
{
  RTCOLLECTION *rtgeom_out = NULL;
  char hasz, hasm;
  RTPOINT4D p4d;
  double ordinate_value;

  /* Nothing to do with NULL */
  if ( ! point )
    rterror(ctx, "Null input geometry.");

  /* Ensure 'from' is less than 'to'. */
  if ( to < from )
  {
    double t = from;
    from = to;
    to = t;
  }

  /* Read Z/M info */
  hasz = rtgeom_has_z(ctx, rtpoint_as_rtgeom(ctx, point));
  hasm = rtgeom_has_m(ctx, rtpoint_as_rtgeom(ctx, point));

  /* Prepare return object */
  rtgeom_out = rtcollection_construct_empty(ctx, RTMULTIPOINTTYPE, point->srid, hasz, hasm);

  /* Test if ordinate is in range */
  rtpoint_getPoint4d_p(ctx, point, &p4d);
  ordinate_value = rtpoint_get_ordinate(ctx, &p4d, ordinate);
  if ( from <= ordinate_value && to >= ordinate_value )
  {
    RTPOINT *rtp = rtpoint_clone(ctx, point);
    rtcollection_add_rtgeom(ctx, rtgeom_out, rtpoint_as_rtgeom(ctx, rtp));
  }

  /* Set the bbox, if necessary */
  if ( rtgeom_out->bbox )
  {
    rtgeom_drop_bbox(ctx, (RTGEOM*)rtgeom_out);
    rtgeom_add_bbox(ctx, (RTGEOM*)rtgeom_out);
  }

  return rtgeom_out;
}



/**
* Clip an input MULTIPOINT between two values, on any ordinate input.
*/
RTCOLLECTION*
rtmpoint_clip_to_ordinate_range(const RTCTX *ctx, const RTMPOINT *mpoint, char ordinate, double from, double to)
{
  RTCOLLECTION *rtgeom_out = NULL;
  char hasz, hasm;
  int i;

  /* Nothing to do with NULL */
  if ( ! mpoint )
    rterror(ctx, "Null input geometry.");

  /* Ensure 'from' is less than 'to'. */
  if ( to < from )
  {
    double t = from;
    from = to;
    to = t;
  }

  /* Read Z/M info */
  hasz = rtgeom_has_z(ctx, rtmpoint_as_rtgeom(ctx, mpoint));
  hasm = rtgeom_has_m(ctx, rtmpoint_as_rtgeom(ctx, mpoint));

  /* Prepare return object */
  rtgeom_out = rtcollection_construct_empty(ctx, RTMULTIPOINTTYPE, mpoint->srid, hasz, hasm);

  /* For each point, is its ordinate value between from and to? */
  for ( i = 0; i < mpoint->ngeoms; i ++ )
  {
    RTPOINT4D p4d;
    double ordinate_value;

    rtpoint_getPoint4d_p(ctx, mpoint->geoms[i], &p4d);
    ordinate_value = rtpoint_get_ordinate(ctx, &p4d, ordinate);

    if ( from <= ordinate_value && to >= ordinate_value )
    {
      RTPOINT *rtp = rtpoint_clone(ctx, mpoint->geoms[i]);
      rtcollection_add_rtgeom(ctx, rtgeom_out, rtpoint_as_rtgeom(ctx, rtp));
    }
  }

  /* Set the bbox, if necessary */
  if ( rtgeom_out->bbox )
  {
    rtgeom_drop_bbox(ctx, (RTGEOM*)rtgeom_out);
    rtgeom_add_bbox(ctx, (RTGEOM*)rtgeom_out);
  }

  return rtgeom_out;
}

/**
* Clip an input MULTILINESTRING between two values, on any ordinate input.
*/
RTCOLLECTION*
rtmline_clip_to_ordinate_range(const RTCTX *ctx, const RTMLINE *mline, char ordinate, double from, double to)
{
  RTCOLLECTION *rtgeom_out = NULL;

  if ( ! mline )
  {
    rterror(ctx, "Null input geometry.");
    return NULL;
  }

  if ( mline->ngeoms == 1)
  {
    rtgeom_out = rtline_clip_to_ordinate_range(ctx, mline->geoms[0], ordinate, from, to);
  }
  else
  {
    RTCOLLECTION *col;
    char hasz = rtgeom_has_z(ctx, rtmline_as_rtgeom(ctx, mline));
    char hasm = rtgeom_has_m(ctx, rtmline_as_rtgeom(ctx, mline));
    int i, j;
    char homogeneous = 1;
    size_t geoms_size = 0;
    rtgeom_out = rtcollection_construct_empty(ctx, RTMULTILINETYPE, mline->srid, hasz, hasm);
    RTFLAGS_SET_Z(rtgeom_out->flags, hasz);
    RTFLAGS_SET_M(rtgeom_out->flags, hasm);
    for ( i = 0; i < mline->ngeoms; i ++ )
    {
      col = rtline_clip_to_ordinate_range(ctx, mline->geoms[i], ordinate, from, to);
      if ( col )
      {
        /* Something was left after the clip. */
        if ( rtgeom_out->ngeoms + col->ngeoms > geoms_size )
        {
          geoms_size += 16;
          if ( rtgeom_out->geoms )
          {
            rtgeom_out->geoms = rtrealloc(ctx, rtgeom_out->geoms, geoms_size * sizeof(RTGEOM*));
          }
          else
          {
            rtgeom_out->geoms = rtalloc(ctx, geoms_size * sizeof(RTGEOM*));
          }
        }
        for ( j = 0; j < col->ngeoms; j++ )
        {
          rtgeom_out->geoms[rtgeom_out->ngeoms] = col->geoms[j];
          rtgeom_out->ngeoms++;
        }
        if ( col->type != mline->type )
        {
          homogeneous = 0;
        }
        /* Shallow free the struct, leaving the geoms behind. */
        if ( col->bbox ) rtfree(ctx, col->bbox);
        rtfree(ctx, col->geoms);
        rtfree(ctx, col);
      }
    }
    if ( rtgeom_out->bbox )
    {
      rtgeom_drop_bbox(ctx, (RTGEOM*)rtgeom_out);
      rtgeom_add_bbox(ctx, (RTGEOM*)rtgeom_out);
    }

    if ( ! homogeneous )
    {
      rtgeom_out->type = RTCOLLECTIONTYPE;
    }
  }

  if ( ! rtgeom_out || rtgeom_out->ngeoms == 0 ) /* Nothing left after clip. */
  {
    return NULL;
  }

  return rtgeom_out;

}


/**
* Take in a LINESTRING and return a MULTILINESTRING of those portions of the
* LINESTRING between the from/to range for the specified ordinate (XYZM)
*/
RTCOLLECTION*
rtline_clip_to_ordinate_range(const RTCTX *ctx, const RTLINE *line, char ordinate, double from, double to)
{

  RTPOINTARRAY *pa_in = NULL;
  RTCOLLECTION *rtgeom_out = NULL;
  RTPOINTARRAY *dp = NULL;
  int i;
  int added_last_point = 0;
  RTPOINT4D *p = NULL, *q = NULL, *r = NULL;
  double ordinate_value_p = 0.0, ordinate_value_q = 0.0;
  char hasz = rtgeom_has_z(ctx, rtline_as_rtgeom(ctx, line));
  char hasm = rtgeom_has_m(ctx, rtline_as_rtgeom(ctx, line));
  char dims = RTFLAGS_NDIMS(line->flags);

  /* Null input, nothing we can do. */
  if ( ! line )
  {
    rterror(ctx, "Null input geometry.");
    return NULL;
  }

  /* Ensure 'from' is less than 'to'. */
  if ( to < from )
  {
    double t = from;
    from = to;
    to = t;
  }

  RTDEBUGF(ctx, 4, "from = %g, to = %g, ordinate = %c", from, to, ordinate);
  RTDEBUGF(ctx, 4, "%s", rtgeom_to_ewkt(ctx, (RTGEOM*)line));

  /* Asking for an ordinate we don't have. Error. */
  if ( (ordinate == 'Z' && ! hasz) || (ordinate == 'M' && ! hasm) )
  {
    rterror(ctx, "Cannot clip on ordinate %d in a %d-d geometry.", ordinate, dims);
    return NULL;
  }

  /* Prepare our working point objects. */
  p = rtalloc(ctx, sizeof(RTPOINT4D));
  q = rtalloc(ctx, sizeof(RTPOINT4D));
  r = rtalloc(ctx, sizeof(RTPOINT4D));

  /* Construct a collection to hold our outputs. */
  rtgeom_out = rtcollection_construct_empty(ctx, RTMULTILINETYPE, line->srid, hasz, hasm);

  /* Get our input point array */
  pa_in = line->points;

  for ( i = 0; i < pa_in->npoints; i++ )
  {
    RTDEBUGF(ctx, 4, "Point #%d", i);
    RTDEBUGF(ctx, 4, "added_last_point %d", added_last_point);
    if ( i > 0 )
    {
      *q = *p;
      ordinate_value_q = ordinate_value_p;
    }
    rt_getPoint4d_p(ctx, pa_in, i, p);
    ordinate_value_p = rtpoint_get_ordinate(ctx, p, ordinate);
    RTDEBUGF(ctx, 4, " ordinate_value_p %g (current)", ordinate_value_p);
    RTDEBUGF(ctx, 4, " ordinate_value_q %g (previous)", ordinate_value_q);

    /* Is this point inside the ordinate range? Yes. */
    if ( ordinate_value_p >= from && ordinate_value_p <= to )
    {
      RTDEBUGF(ctx, 4, " inside ordinate range (%g, %g)", from, to);

      if ( ! added_last_point )
      {
        RTDEBUG(ctx, 4,"  new ptarray required");
        /* We didn't add the previous point, so this is a new segment.
        *  Make a new point array. */
        dp = ptarray_construct_empty(ctx, hasz, hasm, 32);

        /* We're transiting into the range so add an interpolated
        *  point at the range boundary.
        *  If we're on a boundary and crossing from the far side,
        *  we also need an interpolated point. */
        if ( i > 0 && ( /* Don't try to interpolate if this is the first point */
                    ( ordinate_value_p > from && ordinate_value_p < to ) || /* Inside */
                    ( ordinate_value_p == from && ordinate_value_q > to ) || /* Hopping from above */
                    ( ordinate_value_p == to && ordinate_value_q < from ) ) ) /* Hopping from below */
        {
          double interpolation_value;
          (ordinate_value_q > to) ? (interpolation_value = to) : (interpolation_value = from);
          point_interpolate(ctx, q, p, r, hasz, hasm, ordinate, interpolation_value);
          ptarray_append_point(ctx, dp, r, RT_FALSE);
          RTDEBUGF(ctx, 4, "[0] interpolating between (%g, %g) with interpolation point (%g)", ordinate_value_q, ordinate_value_p, interpolation_value);
        }
      }
      /* Add the current vertex to the point array. */
      ptarray_append_point(ctx, dp, p, RT_FALSE);
      if ( ordinate_value_p == from || ordinate_value_p == to )
      {
        added_last_point = 2; /* Added on boundary. */
      }
      else
      {
        added_last_point = 1; /* Added inside range. */
      }
    }
    /* Is this point inside the ordinate range? No. */
    else
    {
      RTDEBUGF(ctx, 4, "  added_last_point (%d)", added_last_point);
      if ( added_last_point == 1 )
      {
        /* We're transiting out of the range, so add an interpolated point
        *  to the point array at the range boundary. */
        double interpolation_value;
        (ordinate_value_p > to) ? (interpolation_value = to) : (interpolation_value = from);
        point_interpolate(ctx, q, p, r, hasz, hasm, ordinate, interpolation_value);
        ptarray_append_point(ctx, dp, r, RT_FALSE);
        RTDEBUGF(ctx, 4, " [1] interpolating between (%g, %g) with interpolation point (%g)", ordinate_value_q, ordinate_value_p, interpolation_value);
      }
      else if ( added_last_point == 2 )
      {
        /* We're out and the last point was on the boundary.
        *  If the last point was the near boundary, nothing to do.
        *  If it was the far boundary, we need an interpolated point. */
        if ( from != to && (
                    (ordinate_value_q == from && ordinate_value_p > from) ||
                    (ordinate_value_q == to && ordinate_value_p < to) ) )
        {
          double interpolation_value;
          (ordinate_value_p > to) ? (interpolation_value = to) : (interpolation_value = from);
          point_interpolate(ctx, q, p, r, hasz, hasm, ordinate, interpolation_value);
          ptarray_append_point(ctx, dp, r, RT_FALSE);
          RTDEBUGF(ctx, 4, " [2] interpolating between (%g, %g) with interpolation point (%g)", ordinate_value_q, ordinate_value_p, interpolation_value);
        }
      }
      else if ( i && ordinate_value_q < from && ordinate_value_p > to )
      {
        /* We just hopped over the whole range, from bottom to top,
        *  so we need to add *two* interpolated points! */
        dp = ptarray_construct(ctx, hasz, hasm, 2);
        /* Interpolate lower point. */
        point_interpolate(ctx, p, q, r, hasz, hasm, ordinate, from);
        ptarray_set_point4d(ctx, dp, 0, r);
        /* Interpolate upper point. */
        point_interpolate(ctx, p, q, r, hasz, hasm, ordinate, to);
        ptarray_set_point4d(ctx, dp, 1, r);
      }
      else if ( i && ordinate_value_q > to && ordinate_value_p < from )
      {
        /* We just hopped over the whole range, from top to bottom,
        *  so we need to add *two* interpolated points! */
        dp = ptarray_construct(ctx, hasz, hasm, 2);
        /* Interpolate upper point. */
        point_interpolate(ctx, p, q, r, hasz, hasm, ordinate, to);
        ptarray_set_point4d(ctx, dp, 0, r);
        /* Interpolate lower point. */
        point_interpolate(ctx, p, q, r, hasz, hasm, ordinate, from);
        ptarray_set_point4d(ctx, dp, 1, r);
      }
      /* We have an extant point-array, save it out to a multi-line. */
      if ( dp )
      {
        RTDEBUG(ctx, 4, "saving pointarray to multi-line (1)");

        /* Only one point, so we have to make an rtpoint to hold this
        *  and set the overall output type to a generic collection. */
        if ( dp->npoints == 1 )
        {
          RTPOINT *opoint = rtpoint_construct(ctx, line->srid, NULL, dp);
          rtgeom_out->type = RTCOLLECTIONTYPE;
          rtgeom_out = rtcollection_add_rtgeom(ctx, rtgeom_out, rtpoint_as_rtgeom(ctx, opoint));

        }
        else
        {
          RTLINE *oline = rtline_construct(ctx, line->srid, NULL, dp);
          rtgeom_out = rtcollection_add_rtgeom(ctx, rtgeom_out, rtline_as_rtgeom(ctx, oline));
        }

        /* Pointarray is now owned by rtgeom_out, so drop reference to it */
        dp = NULL;
      }
      added_last_point = 0;

    }
  }

  /* Still some points left to be saved out. */
  if ( dp && dp->npoints > 0 )
  {
    RTDEBUG(ctx, 4, "saving pointarray to multi-line (2)");
    RTDEBUGF(ctx, 4, "dp->npoints == %d", dp->npoints);
    RTDEBUGF(ctx, 4, "rtgeom_out->ngeoms == %d", rtgeom_out->ngeoms);

    if ( dp->npoints == 1 )
    {
      RTPOINT *opoint = rtpoint_construct(ctx, line->srid, NULL, dp);
      rtgeom_out->type = RTCOLLECTIONTYPE;
      rtgeom_out = rtcollection_add_rtgeom(ctx, rtgeom_out, rtpoint_as_rtgeom(ctx, opoint));
    }
    else
    {
      RTLINE *oline = rtline_construct(ctx, line->srid, NULL, dp);
      rtgeom_out = rtcollection_add_rtgeom(ctx, rtgeom_out, rtline_as_rtgeom(ctx, oline));
    }

    /* Pointarray is now owned by rtgeom_out, so drop reference to it */
    dp = NULL;
  }

  rtfree(ctx, p);
  rtfree(ctx, q);
  rtfree(ctx, r);

  if ( rtgeom_out->bbox && rtgeom_out->ngeoms > 0 )
  {
    rtgeom_drop_bbox(ctx, (RTGEOM*)rtgeom_out);
    rtgeom_add_bbox(ctx, (RTGEOM*)rtgeom_out);
  }

  return rtgeom_out;

}

RTCOLLECTION*
rtgeom_clip_to_ordinate_range(const RTCTX *ctx, const RTGEOM *rtin, char ordinate, double from, double to, double offset)
{
  RTCOLLECTION *out_col;
  RTCOLLECTION *out_offset;
  int i;

  if ( ! rtin )
    rterror(ctx, "rtgeom_clip_to_ordinate_range: null input geometry!");

  switch ( rtin->type )
  {
  case RTLINETYPE:
    out_col = rtline_clip_to_ordinate_range(ctx, (RTLINE*)rtin, ordinate, from, to);
    break;
  case RTMULTILINETYPE:
    out_col = rtmline_clip_to_ordinate_range(ctx, (RTMLINE*)rtin, ordinate, from, to);
    break;
  case RTMULTIPOINTTYPE:
    out_col = rtmpoint_clip_to_ordinate_range(ctx, (RTMPOINT*)rtin, ordinate, from, to);
    break;
  case RTPOINTTYPE:
    out_col = rtpoint_clip_to_ordinate_range(ctx, (RTPOINT*)rtin, ordinate, from, to);
    break;
  default:
    rterror(ctx, "This function does not accept %s geometries.", rttype_name(ctx, rtin->type));
    return NULL;;
  }

  /* Stop if result is NULL */
  if ( out_col == NULL )
    rterror(ctx, "rtgeom_clip_to_ordinate_range clipping routine returned NULL");

  /* Return if we aren't going to offset the result */
  if ( FP_EQUALS(offset, 0.0) || rtgeom_is_empty(ctx, rtcollection_as_rtgeom(ctx, out_col)) )
    return out_col;

  /* Construct a collection to hold our outputs. */
  /* Things get ugly: GEOS offset drops Z's and M's so we have to drop ours */
  out_offset = rtcollection_construct_empty(ctx, RTMULTILINETYPE, rtin->srid, 0, 0);

  /* Try and offset the linear portions of the return value */
  for ( i = 0; i < out_col->ngeoms; i++ )
  {
    int type = out_col->geoms[i]->type;
    if ( type == RTPOINTTYPE )
    {
      rtnotice(ctx, "rtgeom_clip_to_ordinate_range cannot offset a clipped point");
      continue;
    }
    else if ( type == RTLINETYPE )
    {
      /* rtgeom_offsetcurve(ctx, line, offset, quadsegs, joinstyle (round), mitrelimit) */
      RTGEOM *rtoff = rtgeom_offsetcurve(ctx, rtgeom_as_rtline(ctx, out_col->geoms[i]), offset, 8, 1, 5.0);
      if ( ! rtoff )
      {
        rterror(ctx, "rtgeom_offsetcurve returned null");
      }
      rtcollection_add_rtgeom(ctx, out_offset, rtoff);
    }
    else
    {
      rterror(ctx, "rtgeom_clip_to_ordinate_range found an unexpected type (%s) in the offset routine",rttype_name(ctx, type));
    }
  }

  return out_offset;
}

RTCOLLECTION*
rtgeom_locate_between(const RTCTX *ctx, const RTGEOM *rtin, double from, double to, double offset)
{
  if ( ! rtgeom_has_m(ctx, rtin) )
    rterror(ctx, "Input geometry does not have a measure dimension");

  return rtgeom_clip_to_ordinate_range(ctx, rtin, 'M', from, to, offset);
}

double
rtgeom_interpolate_point(const RTCTX *ctx, const RTGEOM *rtin, const RTPOINT *rtpt)
{
  RTPOINT4D p, p_proj;
  double ret = 0.0;

  if ( ! rtin )
    rterror(ctx, "rtgeom_interpolate_point: null input geometry!");

  if ( ! rtgeom_has_m(ctx, rtin) )
    rterror(ctx, "Input geometry does not have a measure dimension");

  if ( rtgeom_is_empty(ctx, rtin) || rtpoint_is_empty(ctx, rtpt) )
    rterror(ctx, "Input geometry is empty");

  switch ( rtin->type )
  {
  case RTLINETYPE:
  {
    RTLINE *rtline = rtgeom_as_rtline(ctx, rtin);
    rtpoint_getPoint4d_p(ctx, rtpt, &p);
    ret = ptarray_locate_point(ctx, rtline->points, &p, NULL, &p_proj);
    ret = p_proj.m;
    break;
  }
  default:
    rterror(ctx, "This function does not accept %s geometries.", rttype_name(ctx, rtin->type));
  }
  return ret;
}

/*
 * Time of closest point of approach
 *
 * Given two vectors (p1-p2 and q1-q2) and
 * a time range (t1-t2) return the time in which
 * a point p is closest to a point q on their
 * respective vectors, and the actual points
 *
 * Here we use algorithm from softsurfer.com
 * that can be found here
 * http://softsurfer.com/Archive/algorithm_0106/algorithm_0106.htm
 *
 * @param p0 start of first segment, will be set to actual
 *           closest point of approach on segment.
 * @param p1 end of first segment
 * @param q0 start of second segment, will be set to actual
 *           closest point of approach on segment.
 * @param q1 end of second segment
 * @param t0 start of travel time
 * @param t1 end of travel time
 *
 * @return time of closest point of approach
 *
 */
static double
segments_tcpa(const RTCTX *ctx, RTPOINT4D* p0, const RTPOINT4D* p1,
              RTPOINT4D* q0, const RTPOINT4D* q1,
              double t0, double t1)
{
  RTPOINT3DZ pv; /* velocity of p, aka u */
  RTPOINT3DZ qv; /* velocity of q, aka v */
  RTPOINT3DZ dv; /* velocity difference */
  RTPOINT3DZ w0; /* vector between first points */

  /*
    rtnotice(ctx, "FROM %g,%g,%g,%g -- %g,%g,%g,%g",
      p0->x, p0->y, p0->z, p0->m,
      p1->x, p1->y, p1->z, p1->m);
    rtnotice(ctx, "  TO %g,%g,%g,%g -- %g,%g,%g,%g",
      q0->x, q0->y, q0->z, q0->m,
      q1->x, q1->y, q1->z, q1->m);
  */

  /* PV aka U */
  pv.x = ( p1->x - p0->x );
  pv.y = ( p1->y - p0->y );
  pv.z = ( p1->z - p0->z );
  /*rtnotice(ctx, "PV:  %g, %g, %g", pv.x, pv.y, pv.z);*/

  /* QV aka V */
  qv.x = ( q1->x - q0->x );
  qv.y = ( q1->y - q0->y );
  qv.z = ( q1->z - q0->z );
  /*rtnotice(ctx, "QV:  %g, %g, %g", qv.x, qv.y, qv.z);*/

  dv.x = pv.x - qv.x;
  dv.y = pv.y - qv.y;
  dv.z = pv.z - qv.z;
  /*rtnotice(ctx, "DV:  %g, %g, %g", dv.x, dv.y, dv.z);*/

  double dv2 = DOT(dv,dv);
  /*rtnotice(ctx, "DOT: %g", dv2);*/

  if ( dv2 == 0.0 )
  {
    /* Distance is the same at any time, we pick the earliest */
    return t0;
  }

  /* Distance at any given time, with t0 */
  w0.x = ( p0->x - q0->x );
  w0.y = ( p0->y - q0->y );
  w0.z = ( p0->z - q0->z );

  /*rtnotice(ctx, "W0:  %g, %g, %g", w0.x, w0.y, w0.z);*/

  /* Check that at distance dt w0 is distance */

  /* This is the fraction of measure difference */
  double t = -DOT(w0,dv) / dv2;
  /*rtnotice(ctx, "CLOSEST TIME (fraction): %g", t);*/

  if ( t > 1.0 )
  {
    /* Getting closer as we move to the end */
    /*rtnotice(ctx, "Converging");*/
    t = 1;
  }
  else if ( t < 0.0 )
  {
    /*rtnotice(ctx, "Diverging");*/
    t = 0;
  }

  /* Interpolate the actual points now */

  p0->x += pv.x * t;
  p0->y += pv.y * t;
  p0->z += pv.z * t;

  q0->x += qv.x * t;
  q0->y += qv.y * t;
  q0->z += qv.z * t;

  t = t0 + (t1 - t0) * t;
  /*rtnotice(ctx, "CLOSEST TIME (real): %g", t);*/

  return t;
}

static int
ptarray_collect_mvals(const RTCTX *ctx, const RTPOINTARRAY *pa, double tmin, double tmax, double *mvals)
{
  RTPOINT4D pbuf;
  int i, n=0;
  for (i=0; i<pa->npoints; ++i)
  {
    rt_getPoint4d_p(ctx, pa, i, &pbuf); /* could be optimized */
    if ( pbuf.m >= tmin && pbuf.m <= tmax )
      mvals[n++] = pbuf.m;
  }
  return n;
}

static int
compare_double(const void *pa, const void *pb)
{
  double a = *((double *)pa);
  double b = *((double *)pb);
  if ( a < b )
    return -1;
  else if ( a > b )
    return 1;
  else
    return 0;
}

/* Return number of elements in unique array */
static int
uniq(const RTCTX *ctx, double *vals, int nvals)
{
  int i, last=0;
  for (i=1; i<nvals; ++i)
  {
    // rtnotice(ctx, "(I%d):%g", i, vals[i]);
    if ( vals[i] != vals[last] )
    {
      vals[++last] = vals[i];
      // rtnotice(ctx, "(O%d):%g", last, vals[last]);
    }
  }
  return last+1;
}

/*
 * Find point at a given measure
 *
 * The function assumes measures are linear so that artays a single point
 * is returned for a single measure.
 *
 * @param pa the point array to perform search on
 * @param m the measure to search for
 * @param p the point to write result into
 * @param from the segment number to start from
 *
 * @return the segment number the point was found into
 *         or -1 if given measure was out of the known range.
 */
static int
ptarray_locate_along_linear(const RTCTX *ctx, const RTPOINTARRAY *pa, double m, RTPOINT4D *p, int from)
{
  int i = from;
  RTPOINT4D p1, p2;

  /* Walk through each segment in the point array */
  rt_getPoint4d_p(ctx, pa, i, &p1);
  for ( i = from+1; i < pa->npoints; i++ )
  {
    rt_getPoint4d_p(ctx, pa, i, &p2);

    if ( segment_locate_along(ctx, &p1, &p2, m, 0, p) == RT_TRUE )
      return i-1; /* found */

    p1 = p2;
  }

  return -1; /* not found */
}

double
rtgeom_tcpa(const RTCTX *ctx, const RTGEOM *g1, const RTGEOM *g2, double *mindist)
{
  RTLINE *l1, *l2;
  int i;
  const RTGBOX *gbox1, *gbox2;
  double tmin, tmax;
  double *mvals;
  int nmvals = 0;
  double mintime;
  double mindist2 = FLT_MAX; /* minimum distance, squared */

  if ( ! rtgeom_has_m(ctx, g1) || ! rtgeom_has_m(ctx, g2) )
  {
    rterror(ctx, "Both input geometries must have a measure dimension");
    return -1;
  }

  l1 = rtgeom_as_rtline(ctx, g1);
  l2 = rtgeom_as_rtline(ctx, g2);

  if ( ! l1 || ! l2 )
  {
    rterror(ctx, "Both input geometries must be linestrings");
    return -1;
  }

  if ( l1->points->npoints < 2 || l2->points->npoints < 2 )
  {
    rterror(ctx, "Both input lines must have at least 2 points");
    return -1;
  }

  /* WARNING: these ranges may be wider than real ones */
  gbox1 = rtgeom_get_bbox(ctx, g1);
  gbox2 = rtgeom_get_bbox(ctx, g2);

  assert(gbox1); /* or the npoints check above would have failed */
  assert(gbox2); /* or the npoints check above would have failed */

  /*
   * Find overlapping M range
   * WARNING: may be larger than the real one
   */

  tmin = FP_MAX(gbox1->mmin, gbox2->mmin);
  tmax = FP_MIN(gbox1->mmax, gbox2->mmax);

  if ( tmax < tmin )
  {
    RTDEBUG(ctx, 1, "Inputs never exist at the same time");
    return -2;
  }

  // rtnotice(ctx, "Min:%g, Max:%g", tmin, tmax);

  /*
   * Collect M values in common time range from inputs
   */

  mvals = rtalloc(ctx,  sizeof(double) *
                   ( l1->points->npoints + l2->points->npoints ) );

  /* TODO: also clip the lines ? */
  nmvals  = ptarray_collect_mvals(ctx, l1->points, tmin, tmax, mvals);
  nmvals += ptarray_collect_mvals(ctx, l2->points, tmin, tmax, mvals + nmvals);

  /* Sort values in ascending order */
  qsort(mvals, nmvals, sizeof(double), compare_double);

  /* Remove duplicated values */
  nmvals = uniq(ctx, mvals, nmvals);

  if ( nmvals < 2 )
  {
    {
      /* there's a single time, must be that one... */
      double t0 = mvals[0];
      RTPOINT4D p0, p1;
      RTDEBUGF(ctx, 1, "Inputs only exist both at a single time (%g)", t0);
      if ( mindist )
      {
        if ( -1 == ptarray_locate_along_linear(ctx, l1->points, t0, &p0, 0) )
        {
          rtfree(ctx, mvals);
          rterror(ctx, "Could not find point with M=%g on first geom", t0);
          return -1;
        }
        if ( -1 == ptarray_locate_along_linear(ctx, l2->points, t0, &p1, 0) )
        {
          rtfree(ctx, mvals);
          rterror(ctx, "Could not find point with M=%g on second geom", t0);
          return -1;
        }
        *mindist = distance3d_pt_pt(ctx, (POINT3D*)&p0, (POINT3D*)&p1);
      }
      rtfree(ctx, mvals);
      return t0;
    }
  }

  /*
   * For each consecutive pair of measures, compute time of closest point
   * approach and actual distance between points at that time
   */
  mintime = tmin;
  for (i=1; i<nmvals; ++i)
  {
    double t0 = mvals[i-1];
    double t1 = mvals[i];
    double t;
    RTPOINT4D p0, p1, q0, q1;
    int seg;
    double dist2;

    // rtnotice(ctx, "T %g-%g", t0, t1);

    seg = ptarray_locate_along_linear(ctx, l1->points, t0, &p0, 0);
    if ( -1 == seg ) continue; /* possible, if RTGBOX is approximated */
    // rtnotice(ctx, "Measure %g on segment %d of line 1: %g, %g, %g", t0, seg, p0.x, p0.y, p0.z);

    seg = ptarray_locate_along_linear(ctx, l1->points, t1, &p1, seg);
    if ( -1 == seg ) continue; /* possible, if RTGBOX is approximated */
    // rtnotice(ctx, "Measure %g on segment %d of line 1: %g, %g, %g", t1, seg, p1.x, p1.y, p1.z);

    seg = ptarray_locate_along_linear(ctx, l2->points, t0, &q0, 0);
    if ( -1 == seg ) continue; /* possible, if RTGBOX is approximated */
    // rtnotice(ctx, "Measure %g on segment %d of line 2: %g, %g, %g", t0, seg, q0.x, q0.y, q0.z);

    seg = ptarray_locate_along_linear(ctx, l2->points, t1, &q1, seg);
    if ( -1 == seg ) continue; /* possible, if RTGBOX is approximated */
    // rtnotice(ctx, "Measure %g on segment %d of line 2: %g, %g, %g", t1, seg, q1.x, q1.y, q1.z);

    t = segments_tcpa(ctx, &p0, &p1, &q0, &q1, t0, t1);

    /*
    rtnotice(ctx, "Closest points: %g,%g,%g and %g,%g,%g at time %g",
    p0.x, p0.y, p0.z,
    q0.x, q0.y, q0.z, t);
    */

    dist2 = ( q0.x - p0.x ) * ( q0.x - p0.x ) +
            ( q0.y - p0.y ) * ( q0.y - p0.y ) +
            ( q0.z - p0.z ) * ( q0.z - p0.z );
    if ( dist2 < mindist2 )
    {
      mindist2 = dist2;
      mintime = t;
      // rtnotice(ctx, "MINTIME: %g", mintime);
    }
  }

  /*
   * Release memory
   */

  rtfree(ctx, mvals);

  if ( mindist )
  {
    *mindist = sqrt(mindist2);
  }
  /*rtnotice(ctx, "MINDIST: %g", sqrt(mindist2));*/

  return mintime;
}

int
rtgeom_cpa_within(const RTCTX *ctx, const RTGEOM *g1, const RTGEOM *g2, double maxdist)
{
  RTLINE *l1, *l2;
  int i;
  const RTGBOX *gbox1, *gbox2;
  double tmin, tmax;
  double *mvals;
  int nmvals = 0;
  double maxdist2 = maxdist * maxdist;
  int within = RT_FALSE;

  if ( ! rtgeom_has_m(ctx, g1) || ! rtgeom_has_m(ctx, g2) )
  {
    rterror(ctx, "Both input geometries must have a measure dimension");
    return RT_FALSE;
  }

  l1 = rtgeom_as_rtline(ctx, g1);
  l2 = rtgeom_as_rtline(ctx, g2);

  if ( ! l1 || ! l2 )
  {
    rterror(ctx, "Both input geometries must be linestrings");
    return RT_FALSE;
  }

  if ( l1->points->npoints < 2 || l2->points->npoints < 2 )
  {
    /* TODO: return distance between these two points */
    rterror(ctx, "Both input lines must have at least 2 points");
    return RT_FALSE;
  }

  /* WARNING: these ranges may be wider than real ones */
  gbox1 = rtgeom_get_bbox(ctx, g1);
  gbox2 = rtgeom_get_bbox(ctx, g2);

  assert(gbox1); /* or the npoints check above would have failed */
  assert(gbox2); /* or the npoints check above would have failed */

  /*
   * Find overlapping M range
   * WARNING: may be larger than the real one
   */

  tmin = FP_MAX(gbox1->mmin, gbox2->mmin);
  tmax = FP_MIN(gbox1->mmax, gbox2->mmax);

  if ( tmax < tmin )
  {
    RTDEBUG(ctx, 1, "Inputs never exist at the same time");
    return RT_FALSE;
  }

  // rtnotice(ctx, "Min:%g, Max:%g", tmin, tmax);

  /*
   * Collect M values in common time range from inputs
   */

  mvals = rtalloc(ctx,  sizeof(double) *
                   ( l1->points->npoints + l2->points->npoints ) );

  /* TODO: also clip the lines ? */
  nmvals  = ptarray_collect_mvals(ctx, l1->points, tmin, tmax, mvals);
  nmvals += ptarray_collect_mvals(ctx, l2->points, tmin, tmax, mvals + nmvals);

  /* Sort values in ascending order */
  qsort(mvals, nmvals, sizeof(double), compare_double);

  /* Remove duplicated values */
  nmvals = uniq(ctx, mvals, nmvals);

  if ( nmvals < 2 )
  {
    /* there's a single time, must be that one... */
    double t0 = mvals[0];
    RTPOINT4D p0, p1;
    RTDEBUGF(ctx, 1, "Inputs only exist both at a single time (%g)", t0);
    if ( -1 == ptarray_locate_along_linear(ctx, l1->points, t0, &p0, 0) )
    {
      rtnotice(ctx, "Could not find point with M=%g on first geom", t0);
      return RT_FALSE;
    }
    if ( -1 == ptarray_locate_along_linear(ctx, l2->points, t0, &p1, 0) )
    {
      rtnotice(ctx, "Could not find point with M=%g on second geom", t0);
      return RT_FALSE;
    }
    if ( distance3d_pt_pt(ctx, (POINT3D*)&p0, (POINT3D*)&p1) <= maxdist )
      within = RT_TRUE;
    rtfree(ctx, mvals);
    return within;
  }

  /*
   * For each consecutive pair of measures, compute time of closest point
   * approach and actual distance between points at that time
   */
  for (i=1; i<nmvals; ++i)
  {
    double t0 = mvals[i-1];
    double t1 = mvals[i];
#if RTGEOM_DEBUG_LEVEL >= 1
    double t;
#endif
    RTPOINT4D p0, p1, q0, q1;
    int seg;
    double dist2;

    // rtnotice(ctx, "T %g-%g", t0, t1);

    seg = ptarray_locate_along_linear(ctx, l1->points, t0, &p0, 0);
    if ( -1 == seg ) continue; /* possible, if RTGBOX is approximated */
    // rtnotice(ctx, "Measure %g on segment %d of line 1: %g, %g, %g", t0, seg, p0.x, p0.y, p0.z);

    seg = ptarray_locate_along_linear(ctx, l1->points, t1, &p1, seg);
    if ( -1 == seg ) continue; /* possible, if RTGBOX is approximated */
    // rtnotice(ctx, "Measure %g on segment %d of line 1: %g, %g, %g", t1, seg, p1.x, p1.y, p1.z);

    seg = ptarray_locate_along_linear(ctx, l2->points, t0, &q0, 0);
    if ( -1 == seg ) continue; /* possible, if RTGBOX is approximated */
    // rtnotice(ctx, "Measure %g on segment %d of line 2: %g, %g, %g", t0, seg, q0.x, q0.y, q0.z);

    seg = ptarray_locate_along_linear(ctx, l2->points, t1, &q1, seg);
    if ( -1 == seg ) continue; /* possible, if RTGBOX is approximated */
    // rtnotice(ctx, "Measure %g on segment %d of line 2: %g, %g, %g", t1, seg, q1.x, q1.y, q1.z);

#if RTGEOM_DEBUG_LEVEL >= 1
    t =
#endif
    segments_tcpa(ctx, &p0, &p1, &q0, &q1, t0, t1);

    /*
    rtnotice(ctx, "Closest points: %g,%g,%g and %g,%g,%g at time %g",
    p0.x, p0.y, p0.z,
    q0.x, q0.y, q0.z, t);
    */

    dist2 = ( q0.x - p0.x ) * ( q0.x - p0.x ) +
            ( q0.y - p0.y ) * ( q0.y - p0.y ) +
            ( q0.z - p0.z ) * ( q0.z - p0.z );
    if ( dist2 <= maxdist2 )
    {
      RTDEBUGF(ctx, 1, "Within distance %g at time %g, breaking", sqrt(dist2), t);
      within = RT_TRUE;
      break;
    }
  }

  /*
   * Release memory
   */

  rtfree(ctx, mvals);

  return within;
}
