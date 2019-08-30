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
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright 2009 David Skea <David.Skea@gov.bc.ca>
 *
 **********************************************************************/



#include "rttopo_config.h"
#include "librttopo_geom_internal.h"
#include "rtgeodetic.h"
#include "rtgeom_log.h"

/**
* For testing geodetic bounding box, we have a magic global variable.
* When this is true (when the cunit tests set it), use the slow, but
* guaranteed correct, algorithm. Otherwise use the regular one.
*/
int gbox_geocentric_slow = RT_FALSE;

/**
* Convert a longitude to the range of -PI,PI
*/
double longitude_radians_normalize(const RTCTX *ctx, double lon)
{
  if ( lon == -1.0 * M_PI )
    return M_PI;
  if ( lon == -2.0 * M_PI )
    return 0.0;

  if ( lon > 2.0 * M_PI )
    lon = remainder(lon, 2.0 * M_PI);

  if ( lon < -2.0 * M_PI )
    lon = remainder(lon, -2.0 * M_PI);

  if ( lon > M_PI )
    lon = -2.0 * M_PI + lon;

  if ( lon < -1.0 * M_PI )
    lon = 2.0 * M_PI + lon;

  if ( lon == -2.0 * M_PI )
    lon *= -1.0;

  return lon;
}

/**
* Convert a latitude to the range of -PI/2,PI/2
*/
double latitude_radians_normalize(const RTCTX *ctx, double lat)
{

  if ( lat > 2.0 * M_PI )
    lat = remainder(lat, 2.0 * M_PI);

  if ( lat < -2.0 * M_PI )
    lat = remainder(lat, -2.0 * M_PI);

  if ( lat > M_PI )
    lat = M_PI - lat;

  if ( lat < -1.0 * M_PI )
    lat = -1.0 * M_PI - lat;

  if ( lat > M_PI_2 )
    lat = M_PI - lat;

  if ( lat < -1.0 * M_PI_2 )
    lat = -1.0 * M_PI - lat;

  return lat;
}

/**
* Convert a longitude to the range of -180,180
* @param lon longitude in degrees
*/
double longitude_degrees_normalize(const RTCTX *ctx, double lon)
{
  if ( lon > 360.0 )
    lon = remainder(lon, 360.0);

  if ( lon < -360.0 )
    lon = remainder(lon, -360.0);

  if ( lon > 180.0 )
    lon = -360.0 + lon;

  if ( lon < -180.0 )
    lon = 360 + lon;

  if ( lon == -180.0 )
    return 180.0;

  if ( lon == -360.0 )
    return 0.0;

  return lon;
}

/**
* Convert a latitude to the range of -90,90
* @param lat latitude in degrees
*/
double latitude_degrees_normalize(const RTCTX *ctx, double lat)
{

  if ( lat > 360.0 )
    lat = remainder(lat, 360.0);

  if ( lat < -360.0 )
    lat = remainder(lat, -360.0);

  if ( lat > 180.0 )
    lat = 180.0 - lat;

  if ( lat < -180.0 )
    lat = -180.0 - lat;

  if ( lat > 90.0 )
    lat = 180.0 - lat;

  if ( lat < -90.0 )
    lat = -180.0 - lat;

  return lat;
}

/**
* Shift a point around by a number of radians
*/
void point_shift(const RTCTX *ctx, GEOGRAPHIC_POINT *p, double shift)
{
  double lon = p->lon + shift;
  if ( lon > M_PI )
    p->lon = -1.0 * M_PI + (lon - M_PI);
  else
    p->lon = lon;
  return;
}

int geographic_point_equals(const RTCTX *ctx, const GEOGRAPHIC_POINT *g1, const GEOGRAPHIC_POINT *g2)
{
  return FP_EQUALS(g1->lat, g2->lat) && FP_EQUALS(g1->lon, g2->lon);
}

/**
* Initialize a geographic point
* @param lon longitude in degrees
* @param lat latitude in degrees
*/
void geographic_point_init(const RTCTX *ctx, double lon, double lat, GEOGRAPHIC_POINT *g)
{
  g->lat = latitude_radians_normalize(ctx, deg2rad(lat));
  g->lon = longitude_radians_normalize(ctx, deg2rad(lon));
}

/** Returns the angular height (latitudinal span) of the box in radians */
double
gbox_angular_height(const RTCTX *ctx, const RTGBOX* gbox)
{
  double d[6];
  int i;
  double zmin = FLT_MAX;
  double zmax = -1 * FLT_MAX;
  POINT3D pt;

  /* Take a copy of the box corners so we can treat them as a list */
  /* Elements are xmin, xmax, ymin, ymax, zmin, zmax */
  memcpy(d, &(gbox->xmin), 6*sizeof(double));

  /* Generate all 8 corner vectors of the box */
  for ( i = 0; i < 8; i++ )
  {
    pt.x = d[i / 4];
    pt.y = d[2 + (i % 4) / 2];
    pt.z = d[4 + (i % 2)];
    normalize(ctx, &pt);
    if ( pt.z < zmin ) zmin = pt.z;
    if ( pt.z > zmax ) zmax = pt.z;
  }
  return asin(zmax) - asin(zmin);
}

/** Returns the angular width (longitudinal span) of the box in radians */
double
gbox_angular_width(const RTCTX *ctx, const RTGBOX* gbox)
{
  double d[6];
  int i, j;
  POINT3D pt[3];
  double maxangle;
  double magnitude;

  /* Take a copy of the box corners so we can treat them as a list */
  /* Elements are xmin, xmax, ymin, ymax, zmin, zmax */
  memcpy(d, &(gbox->xmin), 6*sizeof(double));

  /* Start with the bottom corner */
  pt[0].x = gbox->xmin;
  pt[0].y = gbox->ymin;
  magnitude = sqrt(pt[0].x*pt[0].x + pt[0].y*pt[0].y);
  pt[0].x /= magnitude;
  pt[0].y /= magnitude;

  /* Generate all 8 corner vectors of the box */
  /* Find the vector furthest from our seed vector */
  for ( j = 0; j < 2; j++ )
  {
    maxangle = -1 * FLT_MAX;
    for ( i = 0; i < 4; i++ )
    {
      double angle, dotprod;
      POINT3D pt_n;

      pt_n.x = d[i / 2];
      pt_n.y = d[2 + (i % 2)];
      magnitude = sqrt(pt_n.x*pt_n.x + pt_n.y*pt_n.y);
      pt_n.x /= magnitude;
      pt_n.y /= magnitude;
      pt_n.z = 0.0;

      dotprod = pt_n.x*pt[j].x + pt_n.y*pt[j].y;
      angle = acos(dotprod > 1.0 ? 1.0 : dotprod);
      if ( angle > maxangle )
      {
        pt[j+1] = pt_n;
        maxangle = angle;
      }
    }
  }

  /* Return the distance between the two furthest vectors */
  return maxangle;
}

/** Computes the average(ish) center of the box and returns success. */
int
gbox_centroid(const RTCTX *ctx, const RTGBOX* gbox, RTPOINT2D* out)
{
  double d[6];
  GEOGRAPHIC_POINT g;
  POINT3D pt;
  int i;

  /* Take a copy of the box corners so we can treat them as a list */
  /* Elements are xmin, xmax, ymin, ymax, zmin, zmax */
  memcpy(d, &(gbox->xmin), 6*sizeof(double));

  /* Zero out our return vector */
  pt.x = pt.y = pt.z = 0.0;

  for ( i = 0; i < 8; i++ )
  {
    POINT3D pt_n;

    pt_n.x = d[i / 4];
    pt_n.y = d[2 + ((i % 4) / 2)];
    pt_n.z = d[4 + (i % 2)];
    normalize(ctx, &pt_n);

    pt.x += pt_n.x;
    pt.y += pt_n.y;
    pt.z += pt_n.z;
  }

  pt.x /= 8.0;
  pt.y /= 8.0;
  pt.z /= 8.0;
  normalize(ctx, &pt);

  cart2geog(ctx, &pt, &g);
  out->x = longitude_degrees_normalize(ctx, rad2deg(g.lon));
  out->y = latitude_degrees_normalize(ctx, rad2deg(g.lat));

  return RT_SUCCESS;
}

/**
* Check to see if this geocentric gbox is wrapped around a pole.
* Only makes sense if this gbox originated from a polygon, as it's assuming
* the box is generated from external edges and there's an "interior" which
* contains the pole.
*
* This function is overdetermined, for very large polygons it might add an
* unwarranted pole. STILL NEEDS WORK!
*/
static int gbox_check_poles(const RTCTX *ctx, RTGBOX *gbox)
{
  int rv = RT_FALSE;
  RTDEBUG(ctx, 4, "checking poles");
  RTDEBUGF(ctx, 4, "gbox %s", gbox_to_string(ctx, gbox));
  /* Z axis */
  if ( gbox->xmin < 0.0 && gbox->xmax > 0.0 &&
       gbox->ymin < 0.0 && gbox->ymax > 0.0 )
  {
    if ( (gbox->zmin + gbox->zmax) > 0.0 )
    {
      RTDEBUG(ctx, 4, "enclosed positive z axis");
      gbox->zmax = 1.0;
    }
    else
    {
      RTDEBUG(ctx, 4, "enclosed negative z axis");
      gbox->zmin = -1.0;
    }
    rv = RT_TRUE;
  }

  /* Y axis */
  if ( gbox->xmin < 0.0 && gbox->xmax > 0.0 &&
       gbox->zmin < 0.0 && gbox->zmax > 0.0 )
  {
    if ( gbox->ymin + gbox->ymax > 0.0 )
    {
      RTDEBUG(ctx, 4, "enclosed positive y axis");
      gbox->ymax = 1.0;
    }
    else
    {
      RTDEBUG(ctx, 4, "enclosed negative y axis");
      gbox->ymin = -1.0;
    }
    rv = RT_TRUE;
  }

  /* X axis */
  if ( gbox->ymin < 0.0 && gbox->ymax > 0.0 &&
       gbox->zmin < 0.0 && gbox->zmax > 0.0 )
  {
    if ( gbox->xmin + gbox->xmax > 0.0 )
    {
      RTDEBUG(ctx, 4, "enclosed positive x axis");
      gbox->xmax = 1.0;
    }
    else
    {
      RTDEBUG(ctx, 4, "enclosed negative x axis");
      gbox->xmin = -1.0;
    }
    rv = RT_TRUE;
  }

  return rv;
}

/**
* Convert spherical coordinates to cartesion coordinates on unit sphere
*/
void geog2cart(const RTCTX *ctx, const GEOGRAPHIC_POINT *g, POINT3D *p)
{
  p->x = cos(g->lat) * cos(g->lon);
  p->y = cos(g->lat) * sin(g->lon);
  p->z = sin(g->lat);
}

/**
* Convert cartesion coordinates on unit sphere to spherical coordinates
*/
void cart2geog(const RTCTX *ctx, const POINT3D *p, GEOGRAPHIC_POINT *g)
{
  g->lon = atan2(p->y, p->x);
  g->lat = asin(p->z);
}

/**
* Convert lon/lat coordinates to cartesion coordinates on unit sphere
*/
void ll2cart(const RTCTX *ctx, const RTPOINT2D *g, POINT3D *p)
{
  double x_rad = M_PI * g->x / 180.0;
  double y_rad = M_PI * g->y / 180.0;
  double cos_y_rad = cos(y_rad);
  p->x = cos_y_rad * cos(x_rad);
  p->y = cos_y_rad * sin(x_rad);
  p->z = sin(y_rad);
}

/**
* Convert cartesion coordinates on unit sphere to lon/lat coordinates
static void cart2ll(const POINT3D *p, RTPOINT2D *g)
{
  g->x = longitude_degrees_normalize(ctx, 180.0 * atan2(p->y, p->x) / M_PI);
  g->y = latitude_degrees_normalize(ctx, 180.0 * asin(p->z) / M_PI);
}
*/

/**
* Calculate the dot product of two unit vectors
* (-1 == opposite, 0 == orthogonal, 1 == identical)
*/
static double dot_product(const RTCTX *ctx, const POINT3D *p1, const POINT3D *p2)
{
  return (p1->x*p2->x) + (p1->y*p2->y) + (p1->z*p2->z);
}

/**
* Calculate the cross product of two vectors
*/
static void cross_product(const RTCTX *ctx, const POINT3D *a, const POINT3D *b, POINT3D *n)
{
  n->x = a->y * b->z - a->z * b->y;
  n->y = a->z * b->x - a->x * b->z;
  n->z = a->x * b->y - a->y * b->x;
  return;
}

/**
* Calculate the sum of two vectors
*/
void vector_sum(const RTCTX *ctx, const POINT3D *a, const POINT3D *b, POINT3D *n)
{
  n->x = a->x + b->x;
  n->y = a->y + b->y;
  n->z = a->z + b->z;
  return;
}

/**
* Calculate the difference of two vectors
*/
static void vector_difference(const RTCTX *ctx, const POINT3D *a, const POINT3D *b, POINT3D *n)
{
  n->x = a->x - b->x;
  n->y = a->y - b->y;
  n->z = a->z - b->z;
  return;
}

/**
* Scale a vector out by a factor
*/
static void vector_scale(const RTCTX *ctx, POINT3D *n, double scale)
{
  n->x *= scale;
  n->y *= scale;
  n->z *= scale;
  return;
}

/*
* static inline double vector_magnitude(const POINT3D* v)
* {
*  return sqrt(v->x*v->x + v->y*v->y + v->z*v->z);
* }
*/

/**
* Angle between two unit vectors
*/
double vector_angle(const RTCTX *ctx, const POINT3D* v1, const POINT3D* v2)
{
  POINT3D v3, normal;
  double angle, x, y;

  cross_product(ctx, v1, v2, &normal);
  normalize(ctx, &normal);
  cross_product(ctx, &normal, v1, &v3);

  x = dot_product(ctx, v1, v2);
  y = dot_product(ctx, v2, &v3);

  angle = atan2(y, x);
  return angle;
}

/**
* Normalize to a unit vector.
*/
static void normalize2d(const RTCTX *ctx, RTPOINT2D *p)
{
  double d = sqrt(p->x*p->x + p->y*p->y);
  if (FP_IS_ZERO(d))
  {
    p->x = p->y = 0.0;
    return;
  }
  p->x = p->x / d;
  p->y = p->y / d;
  return;
}

/**
* Calculates the unit normal to two vectors, trying to avoid
* problems with over-narrow or over-wide cases.
*/
void unit_normal(const RTCTX *ctx, const POINT3D *P1, const POINT3D *P2, POINT3D *normal)
{
  double p_dot = dot_product(ctx, P1, P2);
  POINT3D P3;

  /* If edge is really large, calculate a narrower equivalent angle A1/A3. */
  if ( p_dot < 0 )
  {
    vector_sum(ctx, P1, P2, &P3);
    normalize(ctx, &P3);
  }
  /* If edge is narrow, calculate a wider equivalent angle A1/A3. */
  else if ( p_dot > 0.95 )
  {
    vector_difference(ctx, P2, P1, &P3);
    normalize(ctx, &P3);
  }
  /* Just keep the current angle in A1/A3. */
  else
  {
    P3 = *P2;
  }

  /* Normals to the A-plane and B-plane */
  cross_product(ctx, P1, &P3, normal);
  normalize(ctx, normal);
}

/**
* Rotates v1 through an angle (in radians) within the plane defined by v1/v2, returns
* the rotated vector in n.
*/
void vector_rotate(const RTCTX *ctx, const POINT3D* v1, const POINT3D* v2, double angle, POINT3D* n)
{
  POINT3D u;
  double cos_a = cos(angle);
  double sin_a = sin(angle);
  double uxuy, uyuz, uxuz;
  double ux2, uy2, uz2;
  double rxx, rxy, rxz, ryx, ryy, ryz, rzx, rzy, rzz;

  /* Need a unit vector normal to rotate around */
  unit_normal(ctx, v1, v2, &u);

  uxuy = u.x * u.y;
  uxuz = u.x * u.z;
  uyuz = u.y * u.z;

  ux2 = u.x * u.x;
  uy2 = u.y * u.y;
  uz2 = u.z * u.z;

  rxx = cos_a + ux2 * (1 - cos_a);
  rxy = uxuy * (1 - cos_a) - u.z * sin_a;
  rxz = uxuz * (1 - cos_a) + u.y * sin_a;

  ryx = uxuy * (1 - cos_a) + u.z * sin_a;
  ryy = cos_a + uy2 * (1 - cos_a);
  ryz = uyuz * (1 - cos_a) - u.x * sin_a;

  rzx = uxuz * (1 - cos_a) - u.y * sin_a;
  rzy = uyuz * (1 - cos_a) + u.x * sin_a;
  rzz = cos_a + uz2 * (1 - cos_a);

  n->x = rxx * v1->x + rxy * v1->y + rxz * v1->z;
  n->y = ryx * v1->x + ryy * v1->y + ryz * v1->z;
  n->z = rzx * v1->x + rzy * v1->y + rzz * v1->z;

  normalize(ctx, n);
}

/**
* Normalize to a unit vector.
*/
void normalize(const RTCTX *ctx, POINT3D *p)
{
  double d = sqrt(p->x*p->x + p->y*p->y + p->z*p->z);
  if (FP_IS_ZERO(d))
  {
    p->x = p->y = p->z = 0.0;
    return;
  }
  p->x = p->x / d;
  p->y = p->y / d;
  p->z = p->z / d;
  return;
}


/**
* Computes the cross product of two vectors using their lat, lng representations.
* Good even for small distances between p and q.
*/
void robust_cross_product(const RTCTX *ctx, const GEOGRAPHIC_POINT *p, const GEOGRAPHIC_POINT *q, POINT3D *a)
{
  double lon_qpp = (q->lon + p->lon) / -2.0;
  double lon_qmp = (q->lon - p->lon) / 2.0;
  double sin_p_lat_minus_q_lat = sin(p->lat-q->lat);
  double sin_p_lat_plus_q_lat = sin(p->lat+q->lat);
  double sin_lon_qpp = sin(lon_qpp);
  double sin_lon_qmp = sin(lon_qmp);
  double cos_lon_qpp = cos(lon_qpp);
  double cos_lon_qmp = cos(lon_qmp);
  a->x = sin_p_lat_minus_q_lat * sin_lon_qpp * cos_lon_qmp -
         sin_p_lat_plus_q_lat * cos_lon_qpp * sin_lon_qmp;
  a->y = sin_p_lat_minus_q_lat * cos_lon_qpp * cos_lon_qmp +
         sin_p_lat_plus_q_lat * sin_lon_qpp * sin_lon_qmp;
  a->z = cos(p->lat) * cos(q->lat) * sin(q->lon-p->lon);
}

void x_to_z(const RTCTX *ctx, POINT3D *p)
{
  double tmp = p->z;
  p->z = p->x;
  p->x = tmp;
}

void y_to_z(const RTCTX *ctx, POINT3D *p)
{
  double tmp = p->z;
  p->z = p->y;
  p->y = tmp;
}


int crosses_dateline(const RTCTX *ctx, const GEOGRAPHIC_POINT *s, const GEOGRAPHIC_POINT *e)
{
  double sign_s = signum(s->lon);
  double sign_e = signum(e->lon);
  double ss = fabs(s->lon);
  double ee = fabs(e->lon);
  if ( sign_s == sign_e )
  {
    return RT_FALSE;
  }
  else
  {
    double dl = ss + ee;
    if ( dl < M_PI )
      return RT_FALSE;
    else if ( FP_EQUALS(dl, M_PI) )
      return RT_FALSE;
    else
      return RT_TRUE;
  }
}

/**
* Returns -1 if the point is to the left of the plane formed
* by the edge, 1 if the point is to the right, and 0 if the
* point is on the plane.
*/
static int
edge_point_side(const RTCTX *ctx, const GEOGRAPHIC_EDGE *e, const GEOGRAPHIC_POINT *p)
{
  POINT3D normal, pt;
  double w;
  /* Normal to the plane defined by e */
  robust_cross_product(ctx, &(e->start), &(e->end), &normal);
  normalize(ctx, &normal);
  geog2cart(ctx, p, &pt);
  /* We expect the dot product of with normal with any vector in the plane to be zero */
  w = dot_product(ctx, &normal, &pt);
  RTDEBUGF(ctx, 4,"dot product %.9g",w);
  if ( FP_IS_ZERO(w) )
  {
    RTDEBUG(ctx, 4, "point is on plane (dot product is zero)");
    return 0;
  }

  if ( w < 0 )
    return -1;
  else
    return 1;
}

/**
* Returns the angle in radians at point B of the triangle formed by A-B-C
*/
static double
sphere_angle(const RTCTX *ctx, const GEOGRAPHIC_POINT *a, const GEOGRAPHIC_POINT *b,  const GEOGRAPHIC_POINT *c)
{
  POINT3D normal1, normal2;
  robust_cross_product(ctx, b, a, &normal1);
  robust_cross_product(ctx, b, c, &normal2);
  normalize(ctx, &normal1);
  normalize(ctx, &normal2);
  return sphere_distance_cartesian(ctx, &normal1, &normal2);
}

/**
* Computes the spherical area of a triangle. If C is to the left of A/B,
* the area is negative. If C is to the right of A/B, the area is positive.
*
* @param a The first triangle vertex.
* @param b The second triangle vertex.
* @param c The last triangle vertex.
* @return the signed area in radians.
*/
static double
sphere_signed_area(const RTCTX *ctx, const GEOGRAPHIC_POINT *a, const GEOGRAPHIC_POINT *b, const GEOGRAPHIC_POINT *c)
{
  double angle_a, angle_b, angle_c;
  double area_radians = 0.0;
  int side;
  GEOGRAPHIC_EDGE e;

  angle_a = sphere_angle(ctx, b,a,c);
  angle_b = sphere_angle(ctx, a,b,c);
  angle_c = sphere_angle(ctx, b,c,a);

  area_radians = angle_a + angle_b + angle_c - M_PI;

  /* What's the direction of the B/C edge? */
  e.start = *a;
  e.end = *b;
  side = edge_point_side(ctx, &e, c);

  /* Co-linear points implies no area */
  if ( side == 0 )
    return 0.0;

  /* Add the sign to the area */
  return side * area_radians;
}



/**
* Returns true if the point p is on the great circle plane.
* Forms the scalar triple product of A,B,p and if the volume of the
* resulting parallelepiped is near zero the point p is on the
* great circle plane.
*/
int edge_point_on_plane(const RTCTX *ctx, const GEOGRAPHIC_EDGE *e, const GEOGRAPHIC_POINT *p)
{
  int side = edge_point_side(ctx, e, p);
  if ( side == 0 )
    return RT_TRUE;

  return RT_FALSE;
}

/**
* Returns true if the point p is inside the cone defined by the
* two ends of the edge e.
*/
int edge_point_in_cone(const RTCTX *ctx, const GEOGRAPHIC_EDGE *e, const GEOGRAPHIC_POINT *p)
{
  POINT3D vcp, vs, ve, vp;
  double vs_dot_vcp, vp_dot_vcp;
  geog2cart(ctx, &(e->start), &vs);
  geog2cart(ctx, &(e->end), &ve);
  /* Antipodal case, everything is inside. */
  if ( vs.x == -1.0 * ve.x && vs.y == -1.0 * ve.y && vs.z == -1.0 * ve.z )
    return RT_TRUE;
  geog2cart(ctx, p, &vp);
  /* The normalized sum bisects the angle between start and end. */
  vector_sum(ctx, &vs, &ve, &vcp);
  normalize(ctx, &vcp);
  /* The projection of start onto the center defines the minimum similarity */
  vs_dot_vcp = dot_product(ctx, &vs, &vcp);
  RTDEBUGF(ctx, 4,"vs_dot_vcp %.19g",vs_dot_vcp);
  /* The projection of candidate p onto the center */
  vp_dot_vcp = dot_product(ctx, &vp, &vcp);
  RTDEBUGF(ctx, 4,"vp_dot_vcp %.19g",vp_dot_vcp);
  /* If p is more similar than start then p is inside the cone */
  RTDEBUGF(ctx, 4,"fabs(vp_dot_vcp - vs_dot_vcp) %.39g",fabs(vp_dot_vcp - vs_dot_vcp));

  /*
  ** We want to test that vp_dot_vcp is >= vs_dot_vcp but there are
  ** numerical stability issues for values that are very very nearly
  ** equal. Unfortunately there are also values of vp_dot_vcp that are legitimately
  ** very close to but still less than vs_dot_vcp which we also need to catch.
  ** The tolerance of 10-17 seems to do the trick on 32-bit and 64-bit architectures,
  ** for the test cases here.
  ** However, tuning the tolerance value feels like a dangerous hack.
  ** Fundamentally, the problem is that this test is so sensitive.
  */

  /* 1.1102230246251565404236316680908203125e-16 */

  if ( vp_dot_vcp > vs_dot_vcp || fabs(vp_dot_vcp - vs_dot_vcp) < 2e-16 )
  {
    RTDEBUG(ctx, 4, "point is in cone");
    return RT_TRUE;
  }
  RTDEBUG(ctx, 4, "point is not in cone");
  return RT_FALSE;
}

/**
* True if the longitude of p is within the range of the longitude of the ends of e
*/
int edge_contains_coplanar_point(const RTCTX *ctx, const GEOGRAPHIC_EDGE *e, const GEOGRAPHIC_POINT *p)
{
  GEOGRAPHIC_EDGE g;
  GEOGRAPHIC_POINT q;
  double slon = fabs((e->start).lon) + fabs((e->end).lon);
  double dlon = fabs(fabs((e->start).lon) - fabs((e->end).lon));
  double slat = (e->start).lat + (e->end).lat;

  RTDEBUGF(ctx, 4, "e.start == GPOINT(%.6g %.6g) ", (e->start).lat, (e->start).lon);
  RTDEBUGF(ctx, 4, "e.end == GPOINT(%.6g %.6g) ", (e->end).lat, (e->end).lon);
  RTDEBUGF(ctx, 4, "p == GPOINT(%.6g %.6g) ", p->lat, p->lon);

  /* Copy values into working registers */
  g = *e;
  q = *p;

  /* Vertical plane, we need to do this calculation in latitude */
  if ( FP_EQUALS( g.start.lon, g.end.lon ) )
  {
    RTDEBUG(ctx, 4, "vertical plane, we need to do this calculation in latitude");
    /* Supposed to be co-planar... */
    if ( ! FP_EQUALS( q.lon, g.start.lon ) )
      return RT_FALSE;

    if ( ( g.start.lat <= q.lat && q.lat <= g.end.lat ) ||
         ( g.end.lat <= q.lat && q.lat <= g.start.lat ) )
    {
      return RT_TRUE;
    }
    else
    {
      return RT_FALSE;
    }
  }

  /* Over the pole, we need normalize latitude and do this calculation in latitude */
  if ( FP_EQUALS( slon, M_PI ) && ( signum(g.start.lon) != signum(g.end.lon) || FP_EQUALS(dlon, M_PI) ) )
  {
    RTDEBUG(ctx, 4, "over the pole...");
    /* Antipodal, everything (or nothing?) is inside */
    if ( FP_EQUALS( slat, 0.0 ) )
      return RT_TRUE;

    /* Point *is* the north pole */
    if ( slat > 0.0 && FP_EQUALS(q.lat, M_PI_2 ) )
      return RT_TRUE;

    /* Point *is* the south pole */
    if ( slat < 0.0 && FP_EQUALS(q.lat, -1.0 * M_PI_2) )
      return RT_TRUE;

    RTDEBUG(ctx, 4, "coplanar?...");

    /* Supposed to be co-planar... */
    if ( ! FP_EQUALS( q.lon, g.start.lon ) )
      return RT_FALSE;

    RTDEBUG(ctx, 4, "north or south?...");

    /* Over north pole, test based on south pole */
    if ( slat > 0.0 )
    {
      RTDEBUG(ctx, 4, "over the north pole...");
      if ( q.lat > FP_MIN(g.start.lat, g.end.lat) )
        return RT_TRUE;
      else
        return RT_FALSE;
    }
    else
      /* Over south pole, test based on north pole */
    {
      RTDEBUG(ctx, 4, "over the south pole...");
      if ( q.lat < FP_MAX(g.start.lat, g.end.lat) )
        return RT_TRUE;
      else
        return RT_FALSE;
    }
  }

  /* Dateline crossing, flip everything to the opposite hemisphere */
  else if ( slon > M_PI && ( signum(g.start.lon) != signum(g.end.lon) ) )
  {
    RTDEBUG(ctx, 4, "crosses dateline, flip longitudes...");
    if ( g.start.lon > 0.0 )
      g.start.lon -= M_PI;
    else
      g.start.lon += M_PI;
    if ( g.end.lon > 0.0 )
      g.end.lon -= M_PI;
    else
      g.end.lon += M_PI;

    if ( q.lon > 0.0 )
      q.lon -= M_PI;
    else
      q.lon += M_PI;
  }

  if ( ( g.start.lon <= q.lon && q.lon <= g.end.lon ) ||
       ( g.end.lon <= q.lon && q.lon <= g.start.lon ) )
  {
    RTDEBUG(ctx, 4, "true, this edge contains point");
    return RT_TRUE;
  }

  RTDEBUG(ctx, 4, "false, this edge does not contain point");
  return RT_FALSE;
}


/**
* Given two points on a unit sphere, calculate their distance apart in radians.
*/
double sphere_distance(const RTCTX *ctx, const GEOGRAPHIC_POINT *s, const GEOGRAPHIC_POINT *e)
{
  double d_lon = e->lon - s->lon;
  double cos_d_lon = cos(d_lon);
  double cos_lat_e = cos(e->lat);
  double sin_lat_e = sin(e->lat);
  double cos_lat_s = cos(s->lat);
  double sin_lat_s = sin(s->lat);

  double a1 = POW2(cos_lat_e * sin(d_lon));
  double a2 = POW2(cos_lat_s * sin_lat_e - sin_lat_s * cos_lat_e * cos_d_lon);
  double a = sqrt(a1 + a2);
  double b = sin_lat_s * sin_lat_e + cos_lat_s * cos_lat_e * cos_d_lon;
  return atan2(a, b);
}

/**
* Given two unit vectors, calculate their distance apart in radians.
*/
double sphere_distance_cartesian(const RTCTX *ctx, const POINT3D *s, const POINT3D *e)
{
  return acos(dot_product(ctx, s, e));
}

/**
* Given two points on a unit sphere, calculate the direction from s to e.
*/
double sphere_direction(const RTCTX *ctx, const GEOGRAPHIC_POINT *s, const GEOGRAPHIC_POINT *e, double d)
{
  double heading = 0.0;
  double f;

  /* Starting from the poles? Special case. */
  if ( FP_IS_ZERO(cos(s->lat)) )
    return (s->lat > 0.0) ? M_PI : 0.0;

  f = (sin(e->lat) - sin(s->lat) * cos(d)) / (sin(d) * cos(s->lat));
  if ( FP_EQUALS(f, 1.0) )
    heading = 0.0;
  else if ( fabs(f) > 1.0 )
  {
    RTDEBUGF(ctx, 4, "f = %g", f);
    heading = acos(f);
  }
  else
    heading = acos(f);

  if ( sin(e->lon - s->lon) < 0.0 )
    heading = -1 * heading;

  return heading;
}

#if 0 /* unused */
/**
* Computes the spherical excess of a spherical triangle defined by
* the three vectices A, B, C. Computes on the unit sphere (i.e., divides
* edge lengths by the radius, even if the radius is 1.0). The excess is
* signed based on the sign of the delta longitude of A and B.
*
* @param a The first triangle vertex.
* @param b The second triangle vertex.
* @param c The last triangle vertex.
* @return the signed spherical excess.
*/
static double sphere_excess(const GEOGRAPHIC_POINT *a, const GEOGRAPHIC_POINT *b, const GEOGRAPHIC_POINT *c)
{
  double a_dist = sphere_distance(ctx, b, c);
  double b_dist = sphere_distance(ctx, c, a);
  double c_dist = sphere_distance(ctx, a, b);
  double hca = sphere_direction(ctx, c, a, b_dist);
  double hcb = sphere_direction(ctx, c, b, a_dist);
  double sign = signum(hcb-hca);
  double ss = (a_dist + b_dist + c_dist) / 2.0;
  double E = tan(ss/2.0)*tan((ss-a_dist)/2.0)*tan((ss-b_dist)/2.0)*tan((ss-c_dist)/2.0);
  return 4.0 * atan(sqrt(fabs(E))) * sign;
}
#endif


/**
* Returns true if the point p is on the minor edge defined by the
* end points of e.
*/
int edge_contains_point(const RTCTX *ctx, const GEOGRAPHIC_EDGE *e, const GEOGRAPHIC_POINT *p)
{
  if ( edge_point_in_cone(ctx, e, p) && edge_point_on_plane(ctx, e, p) )
    /*  if ( edge_contains_coplanar_point(ctx, e, p) && edge_point_on_plane(ctx, e, p) ) */
  {
    RTDEBUG(ctx, 4, "point is on edge");
    return RT_TRUE;
  }
  RTDEBUG(ctx, 4, "point is not on edge");
  return RT_FALSE;
}

/**
* Used in great circle to compute the pole of the great circle.
*/
double z_to_latitude(const RTCTX *ctx, double z, int top)
{
  double sign = signum(z);
  double tlat = acos(z);
  RTDEBUGF(ctx, 4, "inputs: z(%.8g) sign(%.8g) tlat(%.8g)", z, sign, tlat);
  if (FP_IS_ZERO(z))
  {
    if (top) return M_PI_2;
    else return -1.0 * M_PI_2;
  }
  if (fabs(tlat) > M_PI_2 )
  {
    tlat = sign * (M_PI - fabs(tlat));
  }
  else
  {
    tlat = sign * tlat;
  }
  RTDEBUGF(ctx, 4, "output: tlat(%.8g)", tlat);
  return tlat;
}

/**
* Computes the pole of the great circle disk which is the intersection of
* the great circle with the line of maximum/minimum gradiant that lies on
* the great circle plane.
*/
int clairaut_cartesian(const RTCTX *ctx, const POINT3D *start, const POINT3D *end, GEOGRAPHIC_POINT *g_top, GEOGRAPHIC_POINT *g_bottom)
{
  POINT3D t1, t2;
  GEOGRAPHIC_POINT vN1, vN2;
  RTDEBUG(ctx, 4,"entering function");
  unit_normal(ctx, start, end, &t1);
  unit_normal(ctx, end, start, &t2);
  RTDEBUGF(ctx, 4, "unit normal t1 == POINT(%.8g %.8g %.8g)", t1.x, t1.y, t1.z);
  RTDEBUGF(ctx, 4, "unit normal t2 == POINT(%.8g %.8g %.8g)", t2.x, t2.y, t2.z);
  cart2geog(ctx, &t1, &vN1);
  cart2geog(ctx, &t2, &vN2);
  g_top->lat = z_to_latitude(ctx, t1.z,RT_TRUE);
  g_top->lon = vN2.lon;
  g_bottom->lat = z_to_latitude(ctx, t2.z,RT_FALSE);
  g_bottom->lon = vN1.lon;
  RTDEBUGF(ctx, 4, "clairaut top == GPOINT(%.6g %.6g)", g_top->lat, g_top->lon);
  RTDEBUGF(ctx, 4, "clairaut bottom == GPOINT(%.6g %.6g)", g_bottom->lat, g_bottom->lon);
  return RT_SUCCESS;
}

/**
* Computes the pole of the great circle disk which is the intersection of
* the great circle with the line of maximum/minimum gradiant that lies on
* the great circle plane.
*/
int clairaut_geographic(const RTCTX *ctx, const GEOGRAPHIC_POINT *start, const GEOGRAPHIC_POINT *end, GEOGRAPHIC_POINT *g_top, GEOGRAPHIC_POINT *g_bottom)
{
  POINT3D t1, t2;
  GEOGRAPHIC_POINT vN1, vN2;
  RTDEBUG(ctx, 4,"entering function");
  robust_cross_product(ctx, start, end, &t1);
  normalize(ctx, &t1);
  robust_cross_product(ctx, end, start, &t2);
  normalize(ctx, &t2);
  RTDEBUGF(ctx, 4, "unit normal t1 == POINT(%.8g %.8g %.8g)", t1.x, t1.y, t1.z);
  RTDEBUGF(ctx, 4, "unit normal t2 == POINT(%.8g %.8g %.8g)", t2.x, t2.y, t2.z);
  cart2geog(ctx, &t1, &vN1);
  cart2geog(ctx, &t2, &vN2);
  g_top->lat = z_to_latitude(ctx, t1.z,RT_TRUE);
  g_top->lon = vN2.lon;
  g_bottom->lat = z_to_latitude(ctx, t2.z,RT_FALSE);
  g_bottom->lon = vN1.lon;
  RTDEBUGF(ctx, 4, "clairaut top == GPOINT(%.6g %.6g)", g_top->lat, g_top->lon);
  RTDEBUGF(ctx, 4, "clairaut bottom == GPOINT(%.6g %.6g)", g_bottom->lat, g_bottom->lon);
  return RT_SUCCESS;
}

/**
* Returns true if an intersection can be calculated, and places it in *g.
* Returns false otherwise.
*/
int edge_intersection(const RTCTX *ctx, const GEOGRAPHIC_EDGE *e1, const GEOGRAPHIC_EDGE *e2, GEOGRAPHIC_POINT *g)
{
  POINT3D ea, eb, v;
  RTDEBUGF(ctx, 4, "e1 start(%.20g %.20g) end(%.20g %.20g)", e1->start.lat, e1->start.lon, e1->end.lat, e1->end.lon);
  RTDEBUGF(ctx, 4, "e2 start(%.20g %.20g) end(%.20g %.20g)", e2->start.lat, e2->start.lon, e2->end.lat, e2->end.lon);

  RTDEBUGF(ctx, 4, "e1 start(%.20g %.20g) end(%.20g %.20g)", rad2deg(e1->start.lon), rad2deg(e1->start.lat), rad2deg(e1->end.lon), rad2deg(e1->end.lat));
  RTDEBUGF(ctx, 4, "e2 start(%.20g %.20g) end(%.20g %.20g)", rad2deg(e2->start.lon), rad2deg(e2->start.lat), rad2deg(e2->end.lon), rad2deg(e2->end.lat));

  if ( geographic_point_equals(ctx, &(e1->start), &(e2->start)) )
  {
    *g = e1->start;
    return RT_TRUE;
  }
  if ( geographic_point_equals(ctx, &(e1->end), &(e2->end)) )
  {
    *g = e1->end;
    return RT_TRUE;
  }
  if ( geographic_point_equals(ctx, &(e1->end), &(e2->start)) )
  {
    *g = e1->end;
    return RT_TRUE;
  }
  if ( geographic_point_equals(ctx, &(e1->start), &(e2->end)) )
  {
    *g = e1->start;
    return RT_TRUE;
  }

  robust_cross_product(ctx, &(e1->start), &(e1->end), &ea);
  normalize(ctx, &ea);
  robust_cross_product(ctx, &(e2->start), &(e2->end), &eb);
  normalize(ctx, &eb);
  RTDEBUGF(ctx, 4, "e1 cross product == POINT(%.12g %.12g %.12g)", ea.x, ea.y, ea.z);
  RTDEBUGF(ctx, 4, "e2 cross product == POINT(%.12g %.12g %.12g)", eb.x, eb.y, eb.z);
  RTDEBUGF(ctx, 4, "fabs(dot_product(ctx, ea, eb)) == %.14g", fabs(dot_product(ctx, &ea, &eb)));
  if ( FP_EQUALS(fabs(dot_product(ctx, &ea, &eb)), 1.0) )
  {
    RTDEBUGF(ctx, 4, "parallel edges found! dot_product = %.12g", dot_product(ctx, &ea, &eb));
    /* Parallel (maybe equal) edges! */
    /* Hack alert, only returning ONE end of the edge right now, most do better later. */
    /* Hack alart #2, returning a value of 2 to indicate a co-linear crossing event. */
    if ( edge_contains_point(ctx, e1, &(e2->start)) )
    {
      *g = e2->start;
      return 2;
    }
    if ( edge_contains_point(ctx, e1, &(e2->end)) )
    {
      *g = e2->end;
      return 2;
    }
    if ( edge_contains_point(ctx, e2, &(e1->start)) )
    {
      *g = e1->start;
      return 2;
    }
    if ( edge_contains_point(ctx, e2, &(e1->end)) )
    {
      *g = e1->end;
      return 2;
    }
  }
  unit_normal(ctx, &ea, &eb, &v);
  RTDEBUGF(ctx, 4, "v == POINT(%.12g %.12g %.12g)", v.x, v.y, v.z);
  g->lat = atan2(v.z, sqrt(v.x * v.x + v.y * v.y));
  g->lon = atan2(v.y, v.x);
  RTDEBUGF(ctx, 4, "g == GPOINT(%.12g %.12g)", g->lat, g->lon);
  RTDEBUGF(ctx, 4, "g == POINT(%.12g %.12g)", rad2deg(g->lon), rad2deg(g->lat));
  if ( edge_contains_point(ctx, e1, g) && edge_contains_point(ctx, e2, g) )
  {
    return RT_TRUE;
  }
  else
  {
    RTDEBUG(ctx, 4, "flipping point to other side of sphere");
    g->lat = -1.0 * g->lat;
    g->lon = g->lon + M_PI;
    if ( g->lon > M_PI )
    {
      g->lon = -1.0 * (2.0 * M_PI - g->lon);
    }
    if ( edge_contains_point(ctx, e1, g) && edge_contains_point(ctx, e2, g) )
    {
      return RT_TRUE;
    }
  }
  return RT_FALSE;
}

double edge_distance_to_point(const RTCTX *ctx, const GEOGRAPHIC_EDGE *e, const GEOGRAPHIC_POINT *gp, GEOGRAPHIC_POINT *closest)
{
  double d1 = 1000000000.0, d2, d3, d_nearest;
  POINT3D n, p, k;
  GEOGRAPHIC_POINT gk, g_nearest;

  /* Zero length edge, */
  if ( geographic_point_equals(ctx, &(e->start), &(e->end)) )
  {
    *closest = e->start;
    return sphere_distance(ctx, &(e->start), gp);
  }

  robust_cross_product(ctx, &(e->start), &(e->end), &n);
  normalize(ctx, &n);
  geog2cart(ctx, gp, &p);
  vector_scale(ctx, &n, dot_product(ctx, &p, &n));
  vector_difference(ctx, &p, &n, &k);
  normalize(ctx, &k);
  cart2geog(ctx, &k, &gk);
  if ( edge_contains_point(ctx, e, &gk) )
  {
    d1 = sphere_distance(ctx, gp, &gk);
  }
  d2 = sphere_distance(ctx, gp, &(e->start));
  d3 = sphere_distance(ctx, gp, &(e->end));

  d_nearest = d1;
  g_nearest = gk;

  if ( d2 < d_nearest )
  {
    d_nearest = d2;
    g_nearest = e->start;
  }
  if ( d3 < d_nearest )
  {
    d_nearest = d3;
    g_nearest = e->end;
  }
  if (closest)
    *closest = g_nearest;

  return d_nearest;
}

/**
* Calculate the distance between two edges.
* IMPORTANT: this test does not check for edge intersection!!! (distance == 0)
* You have to check for intersection before calling this function.
*/
double edge_distance_to_edge(const RTCTX *ctx, const GEOGRAPHIC_EDGE *e1, const GEOGRAPHIC_EDGE *e2, GEOGRAPHIC_POINT *closest1, GEOGRAPHIC_POINT *closest2)
{
  double d;
  GEOGRAPHIC_POINT gcp1s, gcp1e, gcp2s, gcp2e, c1, c2;
  double d1s = edge_distance_to_point(ctx, e1, &(e2->start), &gcp1s);
  double d1e = edge_distance_to_point(ctx, e1, &(e2->end), &gcp1e);
  double d2s = edge_distance_to_point(ctx, e2, &(e1->start), &gcp2s);
  double d2e = edge_distance_to_point(ctx, e2, &(e1->end), &gcp2e);

  d = d1s;
  c1 = gcp1s;
  c2 = e2->start;

  if ( d1e < d )
  {
    d = d1e;
    c1 = gcp1e;
    c2 = e2->end;
  }

  if ( d2s < d )
  {
    d = d2s;
    c1 = e1->start;
    c2 = gcp2s;
  }

  if ( d2e < d )
  {
    d = d2e;
    c1 = e1->end;
    c2 = gcp2e;
  }

  if ( closest1 ) *closest1 = c1;
  if ( closest2 ) *closest2 = c2;

  return d;
}


/**
* Given a starting location r, a distance and an azimuth
* to the new point, compute the location of the projected point on the unit sphere.
*/
int sphere_project(const RTCTX *ctx, const GEOGRAPHIC_POINT *r, double distance, double azimuth, GEOGRAPHIC_POINT *n)
{
  double d = distance;
  double lat1 = r->lat;
  double lon1 = r->lon;
  double lat2, lon2;

  lat2 = asin(sin(lat1)*cos(d) + cos(lat1)*sin(d)*cos(azimuth));

  /* If we're going straight up or straight down, we don't need to calculate the longitude */
  /* TODO: this isn't quite true, what if we're going over the pole? */
  if ( FP_EQUALS(azimuth, M_PI) || FP_EQUALS(azimuth, 0.0) )
  {
    lon2 = r->lon;
  }
  else
  {
    lon2 = lon1 + atan2(sin(azimuth)*sin(d)*cos(lat1), cos(d)-sin(lat1)*sin(lat2));
  }

  if ( isnan(lat2) || isnan(lon2) )
    return RT_FAILURE;

  n->lat = lat2;
  n->lon = lon2;

  return RT_SUCCESS;
}


int edge_calculate_gbox_slow(const RTCTX *ctx, const GEOGRAPHIC_EDGE *e, RTGBOX *gbox)
{
  int steps = 1000000;
  int i;
  double dx, dy, dz;
  double distance = sphere_distance(ctx, &(e->start), &(e->end));
  POINT3D pn, p, start, end;

  /* Edge is zero length, just return the naive box */
  if ( FP_IS_ZERO(distance) )
  {
    RTDEBUG(ctx, 4, "edge is zero length. returning");
    geog2cart(ctx, &(e->start), &start);
    geog2cart(ctx, &(e->end), &end);
    gbox_init_point3d(ctx, &start, gbox);
    gbox_merge_point3d(ctx, &end, gbox);
    return RT_SUCCESS;
  }

  /* Edge is antipodal (one point on each side of the globe),
     set the box to contain the whole world and return */
  if ( FP_EQUALS(distance, M_PI) )
  {
    RTDEBUG(ctx, 4, "edge is antipodal. setting to maximum size box, and returning");
    gbox->xmin = gbox->ymin = gbox->zmin = -1.0;
    gbox->xmax = gbox->ymax = gbox->zmax = 1.0;
    return RT_SUCCESS;
  }

  /* Walk along the chord between start and end incrementally,
     normalizing at each step. */
  geog2cart(ctx, &(e->start), &start);
  geog2cart(ctx, &(e->end), &end);
  dx = (end.x - start.x)/steps;
  dy = (end.y - start.y)/steps;
  dz = (end.z - start.z)/steps;
  p = start;
  gbox->xmin = gbox->xmax = p.x;
  gbox->ymin = gbox->ymax = p.y;
  gbox->zmin = gbox->zmax = p.z;
  for ( i = 0; i < steps; i++ )
  {
    p.x += dx;
    p.y += dy;
    p.z += dz;
    pn = p;
    normalize(ctx, &pn);
    gbox_merge_point3d(ctx, &pn, gbox);
  }
  return RT_SUCCESS;
}

/**
* The magic function, given an edge in spherical coordinates, calculate a
* 3D bounding box that fully contains it, taking into account the curvature
* of the sphere on which it is inscribed.
*
* Any arc on the sphere defines a plane that bisects the sphere. In this plane,
* the arc is a portion of a unit circle.
* Projecting the end points of the axes (1,0,0), (-1,0,0) etc, into the plane
* and normalizing yields potential extrema points. Those points on the
* side of the plane-dividing line formed by the end points that is opposite
* the origin of the plane are extrema and should be added to the bounding box.
*/
int edge_calculate_gbox(const RTCTX *ctx, const POINT3D *A1, const POINT3D *A2, RTGBOX *gbox)
{
  RTPOINT2D R1, R2, RX, O;
  POINT3D AN, A3;
  POINT3D X[6];
  int i, o_side;

  /* Initialize the box with the edge end points */
  gbox_init_point3d(ctx, A1, gbox);
  gbox_merge_point3d(ctx, A2, gbox);

  /* Zero length edge, just return! */
  if ( p3d_same(ctx, A1, A2) )
    return RT_SUCCESS;

  /* Error out on antipodal edge */
  if ( FP_EQUALS(A1->x, -1*A2->x) && FP_EQUALS(A1->y, -1*A2->y) && FP_EQUALS(A1->z, -1*A2->z) )
  {
    rterror(ctx, "Antipodal (180 degrees long) edge detected!");
    return RT_FAILURE;
  }

  /* Create A3, a vector in the plane of A1/A2, orthogonal to A1  */
  unit_normal(ctx, A1, A2, &AN);
  unit_normal(ctx, &AN, A1, &A3);

  /* Project A1 and A2 into the 2-space formed by the plane A1/A3 */
  R1.x = 1.0;
  R1.y = 0.0;
  R2.x = dot_product(ctx, A2, A1);
  R2.y = dot_product(ctx, A2, &A3);

  /* Initialize our 3-space axis points (x+, x-, y+, y-, z+, z-) */
  memset(X, 0, sizeof(POINT3D) * 6);
  X[0].x = X[2].y = X[4].z =  1.0;
  X[1].x = X[3].y = X[5].z = -1.0;

  /* Initialize a 2-space origin point. */
  O.x = O.y = 0.0;
  /* What side of the line joining R1/R2 is O? */
  o_side = rt_segment_side(ctx, &R1, &R2, &O);

  /* Add any extrema! */
  for ( i = 0; i < 6; i++ )
  {
    /* Convert 3-space axis points to 2-space unit vectors */
    RX.x = dot_product(ctx, &(X[i]), A1);
    RX.y = dot_product(ctx, &(X[i]), &A3);
    normalize2d(ctx, &RX);

    /* Any axis end on the side of R1/R2 opposite the origin */
    /* is an extreme point in the arc, so we add the 3-space */
    /* version of the point on R1/R2 to the gbox */
    if ( rt_segment_side(ctx, &R1, &R2, &RX) != o_side )
    {
      POINT3D Xn;
      Xn.x = RX.x * A1->x + RX.y * A3.x;
      Xn.y = RX.x * A1->y + RX.y * A3.y;
      Xn.z = RX.x * A1->z + RX.y * A3.z;

      gbox_merge_point3d(ctx, &Xn, gbox);
    }
  }

  return RT_SUCCESS;
}

void rtpoly_pt_outside(const RTCTX *ctx, const RTPOLY *poly, RTPOINT2D *pt_outside)
{
  /* Make sure we have boxes */
  if ( poly->bbox )
  {
    gbox_pt_outside(ctx, poly->bbox, pt_outside);
    return;
  }
  else
  {
    RTGBOX gbox;
    rtgeom_calculate_gbox_geodetic(ctx, (RTGEOM*)poly, &gbox);
    gbox_pt_outside(ctx, &gbox, pt_outside);
    return;
  }
}

/**
* Given a unit geocentric gbox, return a lon/lat (degrees) coordinate point point that is
* guaranteed to be outside the box (and therefore anything it contains).
*/
void gbox_pt_outside(const RTCTX *ctx, const RTGBOX *gbox, RTPOINT2D *pt_outside)
{
  double grow = M_PI / 180.0 / 60.0; /* one arc-minute */
  int i;
  RTGBOX ge;
  POINT3D corners[8];
  POINT3D pt;
  GEOGRAPHIC_POINT g;

  while ( grow < M_PI )
  {
    /* Assign our box and expand it slightly. */
    ge = *gbox;
    if ( ge.xmin > -1 ) ge.xmin -= grow;
    if ( ge.ymin > -1 ) ge.ymin -= grow;
    if ( ge.zmin > -1 ) ge.zmin -= grow;
    if ( ge.xmax < 1 )  ge.xmax += grow;
    if ( ge.ymax < 1 )  ge.ymax += grow;
    if ( ge.zmax < 1 )  ge.zmax += grow;

    /* Build our eight corner points */
    corners[0].x = ge.xmin;
    corners[0].y = ge.ymin;
    corners[0].z = ge.zmin;

    corners[1].x = ge.xmin;
    corners[1].y = ge.ymax;
    corners[1].z = ge.zmin;

    corners[2].x = ge.xmin;
    corners[2].y = ge.ymin;
    corners[2].z = ge.zmax;

    corners[3].x = ge.xmax;
    corners[3].y = ge.ymin;
    corners[3].z = ge.zmin;

    corners[4].x = ge.xmax;
    corners[4].y = ge.ymax;
    corners[4].z = ge.zmin;

    corners[5].x = ge.xmax;
    corners[5].y = ge.ymin;
    corners[5].z = ge.zmax;

    corners[6].x = ge.xmin;
    corners[6].y = ge.ymax;
    corners[6].z = ge.zmax;

    corners[7].x = ge.xmax;
    corners[7].y = ge.ymax;
    corners[7].z = ge.zmax;

    RTDEBUG(ctx, 4, "trying to use a box corner point...");
    for ( i = 0; i < 8; i++ )
    {
      normalize(ctx, &(corners[i]));
      RTDEBUGF(ctx, 4, "testing corner %d: POINT(%.8g %.8g %.8g)", i, corners[i].x, corners[i].y, corners[i].z);
      if ( ! gbox_contains_point3d(ctx, gbox, &(corners[i])) )
      {
        RTDEBUGF(ctx, 4, "corner %d is outside our gbox", i);
        pt = corners[i];
        normalize(ctx, &pt);
        cart2geog(ctx, &pt, &g);
        pt_outside->x = rad2deg(g.lon);
        pt_outside->y = rad2deg(g.lat);
        RTDEBUGF(ctx, 4, "returning POINT(%.8g %.8g) as outside point", pt_outside->x, pt_outside->y);
        return;
      }
    }

    /* Try a wider growth to push the corners outside the original box. */
    grow *= 2.0;
  }

  /* This should never happen! */
  rterror(ctx, "BOOM! Could not generate outside point!");
  return;
}


/**
* Create a new point array with no segment longer than the input segment length (expressed in radians!)
* @param pa_in - input point array pointer
* @param max_seg_length - maximum output segment length in radians
*/
static RTPOINTARRAY*
ptarray_segmentize_sphere(const RTCTX *ctx, const RTPOINTARRAY *pa_in, double max_seg_length)
{
  RTPOINTARRAY *pa_out;
  int hasz = ptarray_has_z(ctx, pa_in);
  int hasm = ptarray_has_m(ctx, pa_in);
  int pa_in_offset = 0; /* input point offset */
  RTPOINT4D p1, p2, p;
  POINT3D q1, q2, q, qn;
  GEOGRAPHIC_POINT g1, g2, g;
  double d;

  /* Just crap out on crazy input */
  if ( ! pa_in )
    rterror(ctx, "ptarray_segmentize_sphere: null input pointarray");
  if ( max_seg_length <= 0.0 )
    rterror(ctx, "ptarray_segmentize_sphere: maximum segment length must be positive");

  /* Empty starting array */
  pa_out = ptarray_construct_empty(ctx, hasz, hasm, pa_in->npoints);

  /* Add first point */
  rt_getPoint4d_p(ctx, pa_in, pa_in_offset, &p1);
  ptarray_append_point(ctx, pa_out, &p1, RT_FALSE);
  geographic_point_init(ctx, p1.x, p1.y, &g1);
  pa_in_offset++;

  while ( pa_in_offset < pa_in->npoints )
  {
    rt_getPoint4d_p(ctx, pa_in, pa_in_offset, &p2);
    geographic_point_init(ctx, p2.x, p2.y, &g2);

    /* Skip duplicate points (except in case of 2-point lines!) */
    if ( (pa_in->npoints > 2) && p4d_same(ctx, &p1, &p2) )
    {
      /* Move one offset forward */
      p1 = p2;
      g1 = g2;
      pa_in_offset++;
      continue;
    }

    /* How long is this edge? */
    d = sphere_distance(ctx, &g1, &g2);

    /* We need to segmentize this edge */
    if ( d > max_seg_length )
    {
      int nsegs = 1 + d / max_seg_length;
      int i;
      double dx, dy, dz, dzz = 0, dmm = 0;

      geog2cart(ctx, &g1, &q1);
      geog2cart(ctx, &g2, &q2);

      dx = (q2.x - q1.x) / nsegs;
      dy = (q2.y - q1.y) / nsegs;
      dz = (q2.z - q1.z) / nsegs;

      /* The independent Z/M values on the ptarray */
      if ( hasz ) dzz = (p2.z - p1.z) / nsegs;
      if ( hasm ) dmm = (p2.m - p1.m) / nsegs;

      q = q1;
      p = p1;

      for ( i = 0; i < nsegs - 1; i++ )
      {
        /* Move one increment forwards */
        q.x += dx; q.y += dy; q.z += dz;
        qn = q;
        normalize(ctx, &qn);

        /* Back to spherical coordinates */
        cart2geog(ctx, &qn, &g);
        /* Back to lon/lat */
        p.x = rad2deg(g.lon);
        p.y = rad2deg(g.lat);
        if ( hasz )
          p.z += dzz;
        if ( hasm )
          p.m += dmm;
        ptarray_append_point(ctx, pa_out, &p, RT_FALSE);
      }

      ptarray_append_point(ctx, pa_out, &p2, RT_FALSE);
    }
    /* This edge is already short enough */
    else
    {
      ptarray_append_point(ctx, pa_out, &p2, (pa_in->npoints==2)?RT_TRUE:RT_FALSE);
    }

    /* Move one offset forward */
    p1 = p2;
    g1 = g2;
    pa_in_offset++;
  }

  return pa_out;
}

/**
* Create a new, densified geometry where no segment is longer than max_seg_length.
* Input geometry is not altered, output geometry must be freed by caller.
* @param rtg_in = input geometry
* @param max_seg_length = maximum segment length in radians
*/
RTGEOM*
rtgeom_segmentize_sphere(const RTCTX *ctx, const RTGEOM *rtg_in, double max_seg_length)
{
  RTPOINTARRAY *pa_out;
  RTLINE *rtline;
  RTPOLY *rtpoly_in, *rtpoly_out;
  RTCOLLECTION *rtcol_in, *rtcol_out;
  int i;

  /* Reflect NULL */
  if ( ! rtg_in )
    return NULL;

  /* Clone empty */
  if ( rtgeom_is_empty(ctx, rtg_in) )
    return rtgeom_clone(ctx, rtg_in);

  switch (rtg_in->type)
  {
  case RTMULTIPOINTTYPE:
  case RTPOINTTYPE:
    return rtgeom_clone_deep(ctx, rtg_in);
    break;
  case RTLINETYPE:
    rtline = rtgeom_as_rtline(ctx, rtg_in);
    pa_out = ptarray_segmentize_sphere(ctx, rtline->points, max_seg_length);
    return rtline_as_rtgeom(ctx, rtline_construct(ctx, rtg_in->srid, NULL, pa_out));
    break;
  case RTPOLYGONTYPE:
    rtpoly_in = rtgeom_as_rtpoly(ctx, rtg_in);
    rtpoly_out = rtpoly_construct_empty(ctx, rtg_in->srid, rtgeom_has_z(ctx, rtg_in), rtgeom_has_m(ctx, rtg_in));
    for ( i = 0; i < rtpoly_in->nrings; i++ )
    {
      pa_out = ptarray_segmentize_sphere(ctx, rtpoly_in->rings[i], max_seg_length);
      rtpoly_add_ring(ctx, rtpoly_out, pa_out);
    }
    return rtpoly_as_rtgeom(ctx, rtpoly_out);
    break;
  case RTMULTILINETYPE:
  case RTMULTIPOLYGONTYPE:
  case RTCOLLECTIONTYPE:
    rtcol_in = rtgeom_as_rtcollection(ctx, rtg_in);
    rtcol_out = rtcollection_construct_empty(ctx, rtg_in->type, rtg_in->srid, rtgeom_has_z(ctx, rtg_in), rtgeom_has_m(ctx, rtg_in));
    for ( i = 0; i < rtcol_in->ngeoms; i++ )
    {
      rtcollection_add_rtgeom(ctx, rtcol_out, rtgeom_segmentize_sphere(ctx, rtcol_in->geoms[i], max_seg_length));
    }
    return rtcollection_as_rtgeom(ctx, rtcol_out);
    break;
  default:
    rterror(ctx, "rtgeom_segmentize_sphere: unsupported input geometry type: %d - %s",
            rtg_in->type, rttype_name(ctx, rtg_in->type));
    break;
  }

  rterror(ctx, "rtgeom_segmentize_sphere got to the end of the function, should not happen");
  return NULL;
}


/**
* Returns the area of the ring (ring must be closed) in square radians (surface of
* the sphere is 4*PI).
*/
double
ptarray_area_sphere(const RTCTX *ctx, const RTPOINTARRAY *pa)
{
  int i;
  const RTPOINT2D *p;
  GEOGRAPHIC_POINT a, b, c;
  double area = 0.0;

  /* Return zero on nonsensical inputs */
  if ( ! pa || pa->npoints < 4 )
    return 0.0;

  p = rt_getPoint2d_cp(ctx, pa, 0);
  geographic_point_init(ctx, p->x, p->y, &a);
  p = rt_getPoint2d_cp(ctx, pa, 1);
  geographic_point_init(ctx, p->x, p->y, &b);

  for ( i = 2; i < pa->npoints-1; i++ )
  {
    p = rt_getPoint2d_cp(ctx, pa, i);
    geographic_point_init(ctx, p->x, p->y, &c);
    area += sphere_signed_area(ctx, &a, &b, &c);
    b = c;
  }

  return fabs(area);
}


static double ptarray_distance_spheroid(const RTCTX *ctx, const RTPOINTARRAY *pa1, const RTPOINTARRAY *pa2, const SPHEROID *s, double tolerance, int check_intersection)
{
  GEOGRAPHIC_EDGE e1, e2;
  GEOGRAPHIC_POINT g1, g2;
  GEOGRAPHIC_POINT nearest1, nearest2;
  POINT3D A1, A2, B1, B2;
  const RTPOINT2D *p;
  double distance;
  int i, j;
  int use_sphere = (s->a == s->b ? 1 : 0);

  /* Make result really big, so that everything will be smaller than it */
  distance = FLT_MAX;

  /* Empty point arrays? Return negative */
  if ( pa1->npoints == 0 || pa2->npoints == 0 )
    return -1.0;

  /* Handle point/point case here */
  if ( pa1->npoints == 1 && pa2->npoints == 1 )
  {
    p = rt_getPoint2d_cp(ctx, pa1, 0);
    geographic_point_init(ctx, p->x, p->y, &g1);
    p = rt_getPoint2d_cp(ctx, pa2, 0);
    geographic_point_init(ctx, p->x, p->y, &g2);
    /* Sphere special case, axes equal */
    distance = s->radius * sphere_distance(ctx, &g1, &g2);
    if ( use_sphere )
      return distance;
    /* Below tolerance, actual distance isn't of interest */
    else if ( distance < 0.95 * tolerance )
      return distance;
    /* Close or greater than tolerance, get the real answer to be sure */
    else
      return spheroid_distance(ctx, &g1, &g2, s);
  }

  /* Handle point/line case here */
  if ( pa1->npoints == 1 || pa2->npoints == 1 )
  {
    /* Handle one/many case here */
    int i;
    const RTPOINTARRAY *pa_one;
    const RTPOINTARRAY *pa_many;

    if ( pa1->npoints == 1 )
    {
      pa_one = pa1;
      pa_many = pa2;
    }
    else
    {
      pa_one = pa2;
      pa_many = pa1;
    }

    /* Initialize our point */
    p = rt_getPoint2d_cp(ctx, pa_one, 0);
    geographic_point_init(ctx, p->x, p->y, &g1);

    /* Initialize start of line */
    p = rt_getPoint2d_cp(ctx, pa_many, 0);
    geographic_point_init(ctx, p->x, p->y, &(e1.start));

    /* Iterate through the edges in our line */
    for ( i = 1; i < pa_many->npoints; i++ )
    {
      double d;
      p = rt_getPoint2d_cp(ctx, pa_many, i);
      geographic_point_init(ctx, p->x, p->y, &(e1.end));
      /* Get the spherical distance between point and edge */
      d = s->radius * edge_distance_to_point(ctx, &e1, &g1, &g2);
      /* New shortest distance! Record this distance / location */
      if ( d < distance )
      {
        distance = d;
        nearest2 = g2;
      }
      /* We've gotten closer than the tolerance... */
      if ( d < tolerance )
      {
        /* Working on a sphere? The answer is correct, return */
        if ( use_sphere )
        {
          return d;
        }
        /* Far enough past the tolerance that the spheroid calculation won't change things */
        else if ( d < tolerance * 0.95 )
        {
          return d;
        }
        /* On a spheroid and near the tolerance? Confirm that we are *actually* closer than tolerance */
        else
        {
          d = spheroid_distance(ctx, &g1, &nearest2, s);
          /* Yes, closer than tolerance, return! */
          if ( d < tolerance )
            return d;
        }
      }
      e1.start = e1.end;
    }

    /* On sphere, return answer */
    if ( use_sphere )
      return distance;
    /* On spheroid, calculate final answer based on closest approach */
    else
      return spheroid_distance(ctx, &g1, &nearest2, s);

  }

  /* Initialize start of line 1 */
  p = rt_getPoint2d_cp(ctx, pa1, 0);
  geographic_point_init(ctx, p->x, p->y, &(e1.start));
  geog2cart(ctx, &(e1.start), &A1);


  /* Handle line/line case */
  for ( i = 1; i < pa1->npoints; i++ )
  {
    p = rt_getPoint2d_cp(ctx, pa1, i);
    geographic_point_init(ctx, p->x, p->y, &(e1.end));
    geog2cart(ctx, &(e1.end), &A2);

    /* Initialize start of line 2 */
    p = rt_getPoint2d_cp(ctx, pa2, 0);
    geographic_point_init(ctx, p->x, p->y, &(e2.start));
    geog2cart(ctx, &(e2.start), &B1);

    for ( j = 1; j < pa2->npoints; j++ )
    {
      double d;

      p = rt_getPoint2d_cp(ctx, pa2, j);
      geographic_point_init(ctx, p->x, p->y, &(e2.end));
      geog2cart(ctx, &(e2.end), &B2);

      RTDEBUGF(ctx, 4, "e1.start == GPOINT(%.6g %.6g) ", e1.start.lat, e1.start.lon);
      RTDEBUGF(ctx, 4, "e1.end == GPOINT(%.6g %.6g) ", e1.end.lat, e1.end.lon);
      RTDEBUGF(ctx, 4, "e2.start == GPOINT(%.6g %.6g) ", e2.start.lat, e2.start.lon);
      RTDEBUGF(ctx, 4, "e2.end == GPOINT(%.6g %.6g) ", e2.end.lat, e2.end.lon);

      if ( check_intersection && edge_intersects(ctx, &A1, &A2, &B1, &B2) )
      {
        RTDEBUG(ctx, 4,"edge intersection! returning 0.0");
        return 0.0;
      }
      d = s->radius * edge_distance_to_edge(ctx, &e1, &e2, &g1, &g2);
      RTDEBUGF(ctx, 4,"got edge_distance_to_edge %.8g", d);

      if ( d < distance )
      {
        distance = d;
        nearest1 = g1;
        nearest2 = g2;
      }
      if ( d < tolerance )
      {
        if ( use_sphere )
        {
          return d;
        }
        else
        {
          d = spheroid_distance(ctx, &nearest1, &nearest2, s);
          if ( d < tolerance )
            return d;
        }
      }

      /* Copy end to start to allow a new end value in next iteration */
      e2.start = e2.end;
      B1 = B2;
    }

    /* Copy end to start to allow a new end value in next iteration */
    e1.start = e1.end;
    A1 = A2;
  }
  RTDEBUGF(ctx, 4,"finished all loops, returning %.8g", distance);

  if ( use_sphere )
    return distance;
  else
    return spheroid_distance(ctx, &nearest1, &nearest2, s);
}


/**
* Calculate the area of an RTGEOM. Anything except POLYGON, MULTIPOLYGON
* and GEOMETRYCOLLECTION return zero immediately. Multi's recurse, polygons
* calculate external ring area and subtract internal ring area. A RTGBOX is
* required to calculate an outside point.
*/
double rtgeom_area_sphere(const RTCTX *ctx, const RTGEOM *rtgeom, const SPHEROID *spheroid)
{
  int type;
  double radius2 = spheroid->radius * spheroid->radius;

  assert(rtgeom);

  /* No area in nothing */
  if ( rtgeom_is_empty(ctx, rtgeom) )
    return 0.0;

  /* Read the geometry type number */
  type = rtgeom->type;

  /* Anything but polygons and collections returns zero */
  if ( ! ( type == RTPOLYGONTYPE || type == RTMULTIPOLYGONTYPE || type == RTCOLLECTIONTYPE ) )
    return 0.0;

  /* Actually calculate area */
  if ( type == RTPOLYGONTYPE )
  {
    RTPOLY *poly = (RTPOLY*)rtgeom;
    int i;
    double area = 0.0;

    /* Just in case there's no rings */
    if ( poly->nrings < 1 )
      return 0.0;

    /* First, the area of the outer ring */
    area += radius2 * ptarray_area_sphere(ctx, poly->rings[0]);

    /* Subtract areas of inner rings */
    for ( i = 1; i < poly->nrings; i++ )
    {
      area -= radius2 * ptarray_area_sphere(ctx, poly->rings[i]);
    }
    return area;
  }

  /* Recurse into sub-geometries to get area */
  if ( type == RTMULTIPOLYGONTYPE || type == RTCOLLECTIONTYPE )
  {
    RTCOLLECTION *col = (RTCOLLECTION*)rtgeom;
    int i;
    double area = 0.0;

    for ( i = 0; i < col->ngeoms; i++ )
    {
      area += rtgeom_area_sphere(ctx, col->geoms[i], spheroid);
    }
    return area;
  }

  /* Shouldn't get here. */
  return 0.0;
}


/**
* Calculate a projected point given a source point, a distance and a bearing.
* @param r - location of first point.
* @param spheroid - spheroid definition.
* @param distance - distance, in units of the spheroid def'n.
* @param azimuth - azimuth in radians.
* @return s - location of projected point.
*
*/
RTPOINT* rtgeom_project_spheroid(const RTCTX *ctx, const RTPOINT *r, const SPHEROID *spheroid, double distance, double azimuth)
{
  GEOGRAPHIC_POINT geo_source, geo_dest;
  RTPOINT4D pt_dest;
  double x, y;
  RTPOINTARRAY *pa;
  RTPOINT *rtp;

  /* Check the azimuth validity, convert to radians */
  if ( azimuth < -2.0 * M_PI || azimuth > 2.0 * M_PI )
  {
    rterror(ctx, "Azimuth must be between -2PI and 2PI");
    return NULL;
  }

  /* Check the distance validity */
  if ( distance < 0.0 || distance > (M_PI * spheroid->radius) )
  {
    rterror(ctx, "Distance must be between 0 and %g", M_PI * spheroid->radius);
    return NULL;
  }

  /* Convert to ta geodetic point */
  x = rtpoint_get_x(ctx, r);
  y = rtpoint_get_y(ctx, r);
  geographic_point_init(ctx, x, y, &geo_source);

  /* Try the projection */
  if( spheroid_project(ctx, &geo_source, spheroid, distance, azimuth, &geo_dest) == RT_FAILURE )
  {
    RTDEBUGF(ctx, 3, "Unable to project from (%g %g) with azimuth %g and distance %g", x, y, azimuth, distance);
    rterror(ctx, "Unable to project from (%g %g) with azimuth %g and distance %g", x, y, azimuth, distance);
    return NULL;
  }

  /* Build the output RTPOINT */
  pa = ptarray_construct(ctx, 0, 0, 1);
  pt_dest.x = rad2deg(longitude_radians_normalize(ctx, geo_dest.lon));
  pt_dest.y = rad2deg(latitude_radians_normalize(ctx, geo_dest.lat));
  pt_dest.z = pt_dest.m = 0.0;
  ptarray_set_point4d(ctx, pa, 0, &pt_dest);
  rtp = rtpoint_construct(ctx, r->srid, NULL, pa);
  rtgeom_set_geodetic(ctx, rtpoint_as_rtgeom(ctx, rtp), RT_TRUE);
  return rtp;
}


/**
* Calculate a bearing (azimuth) given a source and destination point.
* @param r - location of first point.
* @param s - location of second point.
* @param spheroid - spheroid definition.
* @return azimuth - azimuth in radians.
*
*/
double rtgeom_azumith_spheroid(const RTCTX *ctx, const RTPOINT *r, const RTPOINT *s, const SPHEROID *spheroid)
{
  GEOGRAPHIC_POINT g1, g2;
  double x1, y1, x2, y2;

  /* Convert r to a geodetic point */
  x1 = rtpoint_get_x(ctx, r);
  y1 = rtpoint_get_y(ctx, r);
  geographic_point_init(ctx, x1, y1, &g1);

  /* Convert s to a geodetic point */
  x2 = rtpoint_get_x(ctx, s);
  y2 = rtpoint_get_y(ctx, s);
  geographic_point_init(ctx, x2, y2, &g2);

  /* Same point, return NaN */
  if ( FP_EQUALS(x1, x2) && FP_EQUALS(y1, y2) )
  {
    return NAN;
  }

  /* Do the direction calculation */
  return spheroid_direction(ctx, &g1, &g2, spheroid);
}

/**
* Calculate the distance between two RTGEOMs, using the coordinates are
* longitude and latitude. Return immediately when the calulated distance drops
* below the tolerance (useful for dwithin calculations).
* Return a negative distance for incalculable cases.
*/
double rtgeom_distance_spheroid(const RTCTX *ctx, const RTGEOM *rtgeom1, const RTGEOM *rtgeom2, const SPHEROID *spheroid, double tolerance)
{
  uint8_t type1, type2;
  int check_intersection = RT_FALSE;
  RTGBOX gbox1, gbox2;

  gbox_init(ctx, &gbox1);
  gbox_init(ctx, &gbox2);

  assert(rtgeom1);
  assert(rtgeom2);

  RTDEBUGF(ctx, 4, "entered function, tolerance %.8g", tolerance);

  /* What's the distance to an empty geometry? We don't know.
     Return a negative number so the caller can catch this case. */
  if ( rtgeom_is_empty(ctx, rtgeom1) || rtgeom_is_empty(ctx, rtgeom2) )
  {
    return -1.0;
  }

  type1 = rtgeom1->type;
  type2 = rtgeom2->type;

  /* Make sure we have boxes */
  if ( rtgeom1->bbox )
    gbox1 = *(rtgeom1->bbox);
  else
    rtgeom_calculate_gbox_geodetic(ctx, rtgeom1, &gbox1);

  /* Make sure we have boxes */
  if ( rtgeom2->bbox )
    gbox2 = *(rtgeom2->bbox);
  else
    rtgeom_calculate_gbox_geodetic(ctx, rtgeom2, &gbox2);

  /* If the boxes aren't disjoint, we have to check for edge intersections */
  if ( gbox_overlaps(ctx, &gbox1, &gbox2) )
    check_intersection = RT_TRUE;

  /* Point/line combinations can all be handled with simple point array iterations */
  if ( ( type1 == RTPOINTTYPE || type1 == RTLINETYPE ) &&
       ( type2 == RTPOINTTYPE || type2 == RTLINETYPE ) )
  {
    RTPOINTARRAY *pa1, *pa2;

    if ( type1 == RTPOINTTYPE )
      pa1 = ((RTPOINT*)rtgeom1)->point;
    else
      pa1 = ((RTLINE*)rtgeom1)->points;

    if ( type2 == RTPOINTTYPE )
      pa2 = ((RTPOINT*)rtgeom2)->point;
    else
      pa2 = ((RTLINE*)rtgeom2)->points;

    return ptarray_distance_spheroid(ctx, pa1, pa2, spheroid, tolerance, check_intersection);
  }

  /* Point/Polygon cases, if point-in-poly, return zero, else return distance. */
  if ( ( type1 == RTPOLYGONTYPE && type2 == RTPOINTTYPE ) ||
       ( type2 == RTPOLYGONTYPE && type1 == RTPOINTTYPE ) )
  {
    const RTPOINT2D *p;
    RTPOLY *rtpoly;
    RTPOINT *rtpt;
    double distance = FLT_MAX;
    int i;

    if ( type1 == RTPOINTTYPE )
    {
      rtpt = (RTPOINT*)rtgeom1;
      rtpoly = (RTPOLY*)rtgeom2;
    }
    else
    {
      rtpt = (RTPOINT*)rtgeom2;
      rtpoly = (RTPOLY*)rtgeom1;
    }
    p = rt_getPoint2d_cp(ctx, rtpt->point, 0);

    /* Point in polygon implies zero distance */
    if ( rtpoly_covers_point2d(ctx, rtpoly, p) )
    {
      return 0.0;
    }

    /* Not inside, so what's the actual distance? */
    for ( i = 0; i < rtpoly->nrings; i++ )
    {
      double ring_distance = ptarray_distance_spheroid(ctx, rtpoly->rings[i], rtpt->point, spheroid, tolerance, check_intersection);
      if ( ring_distance < distance )
        distance = ring_distance;
      if ( distance < tolerance )
        return distance;
    }
    return distance;
  }

  /* Line/polygon case, if start point-in-poly, return zero, else return distance. */
  if ( ( type1 == RTPOLYGONTYPE && type2 == RTLINETYPE ) ||
       ( type2 == RTPOLYGONTYPE && type1 == RTLINETYPE ) )
  {
    const RTPOINT2D *p;
    RTPOLY *rtpoly;
    RTLINE *rtline;
    double distance = FLT_MAX;
    int i;

    if ( type1 == RTLINETYPE )
    {
      rtline = (RTLINE*)rtgeom1;
      rtpoly = (RTPOLY*)rtgeom2;
    }
    else
    {
      rtline = (RTLINE*)rtgeom2;
      rtpoly = (RTPOLY*)rtgeom1;
    }
    p = rt_getPoint2d_cp(ctx, rtline->points, 0);

    RTDEBUG(ctx, 4, "checking if a point of line is in polygon");

    /* Point in polygon implies zero distance */
    if ( rtpoly_covers_point2d(ctx, rtpoly, p) )
      return 0.0;

    RTDEBUG(ctx, 4, "checking ring distances");

    /* Not contained, so what's the actual distance? */
    for ( i = 0; i < rtpoly->nrings; i++ )
    {
      double ring_distance = ptarray_distance_spheroid(ctx, rtpoly->rings[i], rtline->points, spheroid, tolerance, check_intersection);
      RTDEBUGF(ctx, 4, "ring[%d] ring_distance = %.8g", i, ring_distance);
      if ( ring_distance < distance )
        distance = ring_distance;
      if ( distance < tolerance )
        return distance;
    }
    RTDEBUGF(ctx, 4, "all rings checked, returning distance = %.8g", distance);
    return distance;

  }

  /* Polygon/polygon case, if start point-in-poly, return zero, else return distance. */
  if ( ( type1 == RTPOLYGONTYPE && type2 == RTPOLYGONTYPE ) ||
       ( type2 == RTPOLYGONTYPE && type1 == RTPOLYGONTYPE ) )
  {
    const RTPOINT2D *p;
    RTPOLY *rtpoly1 = (RTPOLY*)rtgeom1;
    RTPOLY *rtpoly2 = (RTPOLY*)rtgeom2;
    double distance = FLT_MAX;
    int i, j;

    /* Point of 2 in polygon 1 implies zero distance */
    p = rt_getPoint2d_cp(ctx, rtpoly1->rings[0], 0);
    if ( rtpoly_covers_point2d(ctx, rtpoly2, p) )
      return 0.0;

    /* Point of 1 in polygon 2 implies zero distance */
    p = rt_getPoint2d_cp(ctx, rtpoly2->rings[0], 0);
    if ( rtpoly_covers_point2d(ctx, rtpoly1, p) )
      return 0.0;

    /* Not contained, so what's the actual distance? */
    for ( i = 0; i < rtpoly1->nrings; i++ )
    {
      for ( j = 0; j < rtpoly2->nrings; j++ )
      {
        double ring_distance = ptarray_distance_spheroid(ctx, rtpoly1->rings[i], rtpoly2->rings[j], spheroid, tolerance, check_intersection);
        if ( ring_distance < distance )
          distance = ring_distance;
        if ( distance < tolerance )
          return distance;
      }
    }
    return distance;
  }

  /* Recurse into collections */
  if ( rttype_is_collection(ctx, type1) )
  {
    int i;
    double distance = FLT_MAX;
    RTCOLLECTION *col = (RTCOLLECTION*)rtgeom1;

    for ( i = 0; i < col->ngeoms; i++ )
    {
      double geom_distance = rtgeom_distance_spheroid(ctx, col->geoms[i], rtgeom2, spheroid, tolerance);
      if ( geom_distance < distance )
        distance = geom_distance;
      if ( distance < tolerance )
        return distance;
    }
    return distance;
  }

  /* Recurse into collections */
  if ( rttype_is_collection(ctx, type2) )
  {
    int i;
    double distance = FLT_MAX;
    RTCOLLECTION *col = (RTCOLLECTION*)rtgeom2;

    for ( i = 0; i < col->ngeoms; i++ )
    {
      double geom_distance = rtgeom_distance_spheroid(ctx, rtgeom1, col->geoms[i], spheroid, tolerance);
      if ( geom_distance < distance )
        distance = geom_distance;
      if ( distance < tolerance )
        return distance;
    }
    return distance;
  }


  rterror(ctx, "arguments include unsupported geometry type (%s, %s)", rttype_name(ctx, type1), rttype_name(ctx, type1));
  return -1.0;

}


int rtgeom_covers_rtgeom_sphere(const RTCTX *ctx, const RTGEOM *rtgeom1, const RTGEOM *rtgeom2)
{
  int type1, type2;
  RTGBOX gbox1, gbox2;
  gbox1.flags = gbox2.flags = 0;

  assert(rtgeom1);
  assert(rtgeom2);

  type1 = rtgeom1->type;
  type2 = rtgeom2->type;

  /* Currently a restricted implementation */
  if ( ! ( (type1 == RTPOLYGONTYPE || type1 == RTMULTIPOLYGONTYPE || type1 == RTCOLLECTIONTYPE) &&
           (type2 == RTPOINTTYPE || type2 == RTMULTIPOINTTYPE || type2 == RTCOLLECTIONTYPE) ) )
  {
    rterror(ctx, "rtgeom_covers_rtgeom_sphere: only POLYGON covers POINT tests are currently supported");
    return RT_FALSE;
  }

  /* Make sure we have boxes */
  if ( rtgeom1->bbox )
    gbox1 = *(rtgeom1->bbox);
  else
    rtgeom_calculate_gbox_geodetic(ctx, rtgeom1, &gbox1);

  /* Make sure we have boxes */
  if ( rtgeom2->bbox )
    gbox2 = *(rtgeom2->bbox);
  else
    rtgeom_calculate_gbox_geodetic(ctx, rtgeom2, &gbox2);


  /* Handle the polygon/point case */
  if ( type1 == RTPOLYGONTYPE && type2 == RTPOINTTYPE )
  {
    RTPOINT2D pt_to_test;
    rt_getPoint2d_p(ctx, ((RTPOINT*)rtgeom2)->point, 0, &pt_to_test);
    return rtpoly_covers_point2d(ctx, (RTPOLY*)rtgeom1, &pt_to_test);
  }

  /* If any of the first argument parts covers the second argument, it's true */
  if ( rttype_is_collection(ctx,  type1 ) )
  {
    int i;
    RTCOLLECTION *col = (RTCOLLECTION*)rtgeom1;

    for ( i = 0; i < col->ngeoms; i++ )
    {
      if ( rtgeom_covers_rtgeom_sphere(ctx, col->geoms[i], rtgeom2) )
      {
        return RT_TRUE;
      }
    }
    return RT_FALSE;
  }

  /* Only if all of the second arguments are covered by the first argument is the condition true */
  if ( rttype_is_collection(ctx,  type2 ) )
  {
    int i;
    RTCOLLECTION *col = (RTCOLLECTION*)rtgeom2;

    for ( i = 0; i < col->ngeoms; i++ )
    {
      if ( ! rtgeom_covers_rtgeom_sphere(ctx, rtgeom1, col->geoms[i]) )
      {
        return RT_FALSE;
      }
    }
    return RT_TRUE;
  }

  /* Don't get here */
  rterror(ctx, "rtgeom_covers_rtgeom_sphere: reached end of function without resolution");
  return RT_FALSE;

}

/**
* Given a polygon (lon/lat decimal degrees) and point (lon/lat decimal degrees) and
* a guaranteed outside point (lon/lat decimal degrees) (calculate with gbox_pt_outside(ctx))
* return RT_TRUE if point is inside or on edge of polygon.
*/
int rtpoly_covers_point2d(const RTCTX *ctx, const RTPOLY *poly, const RTPOINT2D *pt_to_test)
{
  int i;
  int in_hole_count = 0;
  POINT3D p;
  GEOGRAPHIC_POINT gpt_to_test;
  RTPOINT2D pt_outside;
  RTGBOX gbox;
  gbox.flags = 0;

  /* Nulls and empties don't contain anything! */
  if ( ! poly || rtgeom_is_empty(ctx, (RTGEOM*)poly) )
  {
    RTDEBUG(ctx, 4,"returning false, geometry is empty or null");
    return RT_FALSE;
  }

  /* Make sure we have boxes */
  if ( poly->bbox )
    gbox = *(poly->bbox);
  else
    rtgeom_calculate_gbox_geodetic(ctx, (RTGEOM*)poly, &gbox);

  /* Point not in box? Done! */
  geographic_point_init(ctx, pt_to_test->x, pt_to_test->y, &gpt_to_test);
  geog2cart(ctx, &gpt_to_test, &p);
  if ( ! gbox_contains_point3d(ctx, &gbox, &p) )
  {
    RTDEBUG(ctx, 4, "the point is not in the box!");
    return RT_FALSE;
  }

  /* Calculate our outside point from the gbox */
  gbox_pt_outside(ctx, &gbox, &pt_outside);

  RTDEBUGF(ctx, 4, "pt_outside POINT(%.18g %.18g)", pt_outside.x, pt_outside.y);
  RTDEBUGF(ctx, 4, "pt_to_test POINT(%.18g %.18g)", pt_to_test->x, pt_to_test->y);
  RTDEBUGF(ctx, 4, "polygon %s", rtgeom_to_ewkt(ctx, (RTGEOM*)poly));
  RTDEBUGF(ctx, 4, "gbox %s", gbox_to_string(ctx, &gbox));

  /* Not in outer ring? We're done! */
  if ( ! ptarray_contains_point_sphere(ctx, poly->rings[0], &pt_outside, pt_to_test) )
  {
    RTDEBUG(ctx, 4,"returning false, point is outside ring");
    return RT_FALSE;
  }

  RTDEBUGF(ctx, 4, "testing %d rings", poly->nrings);

  /* But maybe point is in a hole... */
  for ( i = 1; i < poly->nrings; i++ )
  {
    RTDEBUGF(ctx, 4, "ring test loop %d", i);
    /* Count up hole containment. Odd => outside boundary. */
    if ( ptarray_contains_point_sphere(ctx, poly->rings[i], &pt_outside, pt_to_test) )
      in_hole_count++;
  }

  RTDEBUGF(ctx, 4, "in_hole_count == %d", in_hole_count);

  if ( in_hole_count % 2 )
  {
    RTDEBUG(ctx, 4,"returning false, inner ring containment count is odd");
    return RT_FALSE;
  }

  RTDEBUG(ctx, 4,"returning true, inner ring containment count is even");
  return RT_TRUE;
}


/**
* This function can only be used on RTGEOM that is built on top of
* GSERIALIZED, otherwise alignment errors will ensue.
*/
int rt_getPoint2d_p_ro(const RTCTX *ctx, const RTPOINTARRAY *pa, int n, RTPOINT2D **point)
{
  uint8_t *pa_ptr = NULL;
  assert(pa);
  assert(n >= 0);
  assert(n < pa->npoints);

  pa_ptr = rt_getPoint_internal(ctx, pa, n);
  /* printf( "pa_ptr[0]: %g\n", *((double*)pa_ptr)); */
  *point = (RTPOINT2D*)pa_ptr;

  return RT_SUCCESS;
}

int ptarray_calculate_gbox_geodetic(const RTCTX *ctx, const RTPOINTARRAY *pa, RTGBOX *gbox)
{
  int i;
  int first = RT_TRUE;
  const RTPOINT2D *p;
  POINT3D A1, A2;
  RTGBOX edge_gbox;

  assert(gbox);
  assert(pa);

  gbox_init(ctx, &edge_gbox);
  edge_gbox.flags = gbox->flags;

  if ( pa->npoints == 0 ) return RT_FAILURE;

  if ( pa->npoints == 1 )
  {
    p = rt_getPoint2d_cp(ctx, pa, 0);
    ll2cart(ctx, p, &A1);
    gbox->xmin = gbox->xmax = A1.x;
    gbox->ymin = gbox->ymax = A1.y;
    gbox->zmin = gbox->zmax = A1.z;
    return RT_SUCCESS;
  }

  p = rt_getPoint2d_cp(ctx, pa, 0);
  ll2cart(ctx, p, &A1);

  for ( i = 1; i < pa->npoints; i++ )
  {

    p = rt_getPoint2d_cp(ctx, pa, i);
    ll2cart(ctx, p, &A2);

    edge_calculate_gbox(ctx, &A1, &A2, &edge_gbox);

    /* Initialize the box */
    if ( first )
    {
      gbox_duplicate(ctx, &edge_gbox, gbox);
      first = RT_FALSE;
    }
    /* Expand the box where necessary */
    else
    {
      gbox_merge(ctx, &edge_gbox, gbox);
    }

    A1 = A2;
  }

  return RT_SUCCESS;
}

static int rtpoint_calculate_gbox_geodetic(const RTCTX *ctx, const RTPOINT *point, RTGBOX *gbox)
{
  assert(point);
  return ptarray_calculate_gbox_geodetic(ctx, point->point, gbox);
}

static int rtline_calculate_gbox_geodetic(const RTCTX *ctx, const RTLINE *line, RTGBOX *gbox)
{
  assert(line);
  return ptarray_calculate_gbox_geodetic(ctx, line->points, gbox);
}

static int rtpolygon_calculate_gbox_geodetic(const RTCTX *ctx, const RTPOLY *poly, RTGBOX *gbox)
{
  RTGBOX ringbox;
  int i;
  int first = RT_TRUE;
  assert(poly);
  if ( poly->nrings == 0 )
    return RT_FAILURE;
  ringbox.flags = gbox->flags;
  for ( i = 0; i < poly->nrings; i++ )
  {
    if ( ptarray_calculate_gbox_geodetic(ctx, poly->rings[i], &ringbox) == RT_FAILURE )
      return RT_FAILURE;
    if ( first )
    {
      gbox_duplicate(ctx, &ringbox, gbox);
      first = RT_FALSE;
    }
    else
    {
      gbox_merge(ctx, &ringbox, gbox);
    }
  }

  /* If the box wraps a poly, push that axis to the absolute min/max as appropriate */
  gbox_check_poles(ctx, gbox);

  return RT_SUCCESS;
}

static int rttriangle_calculate_gbox_geodetic(const RTCTX *ctx, const RTTRIANGLE *triangle, RTGBOX *gbox)
{
  assert(triangle);
  return ptarray_calculate_gbox_geodetic(ctx, triangle->points, gbox);
}


static int rtcollection_calculate_gbox_geodetic(const RTCTX *ctx, const RTCOLLECTION *coll, RTGBOX *gbox)
{
  RTGBOX subbox;
  int i;
  int result = RT_FAILURE;
  int first = RT_TRUE;
  assert(coll);
  if ( coll->ngeoms == 0 )
    return RT_FAILURE;

  subbox.flags = gbox->flags;

  for ( i = 0; i < coll->ngeoms; i++ )
  {
    if ( rtgeom_calculate_gbox_geodetic(ctx, (RTGEOM*)(coll->geoms[i]), &subbox) == RT_SUCCESS )
    {
      /* Keep a copy of the sub-bounding box for later */
      if ( coll->geoms[i]->bbox )
        rtfree(ctx, coll->geoms[i]->bbox);
      coll->geoms[i]->bbox = gbox_copy(ctx, &subbox);
      if ( first )
      {
        gbox_duplicate(ctx, &subbox, gbox);
        first = RT_FALSE;
      }
      else
      {
        gbox_merge(ctx, &subbox, gbox);
      }
      result = RT_SUCCESS;
    }
  }
  return result;
}

int rtgeom_calculate_gbox_geodetic(const RTCTX *ctx, const RTGEOM *geom, RTGBOX *gbox)
{
  int result = RT_FAILURE;
  RTDEBUGF(ctx, 4, "got type %d", geom->type);

  /* Add a geodetic flag to the incoming gbox */
  gbox->flags = gflags(ctx, RTFLAGS_GET_Z(geom->flags),RTFLAGS_GET_M(geom->flags),1);

  switch (geom->type)
  {
  case RTPOINTTYPE:
    result = rtpoint_calculate_gbox_geodetic(ctx, (RTPOINT*)geom, gbox);
    break;
  case RTLINETYPE:
    result = rtline_calculate_gbox_geodetic(ctx, (RTLINE *)geom, gbox);
    break;
  case RTPOLYGONTYPE:
    result = rtpolygon_calculate_gbox_geodetic(ctx, (RTPOLY *)geom, gbox);
    break;
  case RTTRIANGLETYPE:
    result = rttriangle_calculate_gbox_geodetic(ctx, (RTTRIANGLE *)geom, gbox);
    break;
  case RTMULTIPOINTTYPE:
  case RTMULTILINETYPE:
  case RTMULTIPOLYGONTYPE:
  case RTPOLYHEDRALSURFACETYPE:
  case RTTINTYPE:
  case RTCOLLECTIONTYPE:
    result = rtcollection_calculate_gbox_geodetic(ctx, (RTCOLLECTION *)geom, gbox);
    break;
  default:
    rterror(ctx, "rtgeom_calculate_gbox_geodetic: unsupported input geometry type: %d - %s",
            geom->type, rttype_name(ctx, geom->type));
    break;
  }
  return result;
}



static int ptarray_check_geodetic(const RTCTX *ctx, const RTPOINTARRAY *pa)
{
  int t;
  RTPOINT2D pt;

  assert(pa);

  for (t=0; t<pa->npoints; t++)
  {
    rt_getPoint2d_p(ctx, pa, t, &pt);
    /* printf( "%d (%g, %g)\n", t, pt.x, pt.y); */
    if ( pt.x < -180.0 || pt.y < -90.0 || pt.x > 180.0 || pt.y > 90.0 )
      return RT_FALSE;
  }

  return RT_TRUE;
}

static int rtpoint_check_geodetic(const RTCTX *ctx, const RTPOINT *point)
{
  assert(point);
  return ptarray_check_geodetic(ctx, point->point);
}

static int rtline_check_geodetic(const RTCTX *ctx, const RTLINE *line)
{
  assert(line);
  return ptarray_check_geodetic(ctx, line->points);
}

static int rtpoly_check_geodetic(const RTCTX *ctx, const RTPOLY *poly)
{
  int i = 0;
  assert(poly);

  for ( i = 0; i < poly->nrings; i++ )
  {
    if ( ptarray_check_geodetic(ctx, poly->rings[i]) == RT_FALSE )
      return RT_FALSE;
  }
  return RT_TRUE;
}

static int rttriangle_check_geodetic(const RTCTX *ctx, const RTTRIANGLE *triangle)
{
  assert(triangle);
  return ptarray_check_geodetic(ctx, triangle->points);
}


static int rtcollection_check_geodetic(const RTCTX *ctx, const RTCOLLECTION *col)
{
  int i = 0;
  assert(col);

  for ( i = 0; i < col->ngeoms; i++ )
  {
    if ( rtgeom_check_geodetic(ctx, col->geoms[i]) == RT_FALSE )
      return RT_FALSE;
  }
  return RT_TRUE;
}

int rtgeom_check_geodetic(const RTCTX *ctx, const RTGEOM *geom)
{
  if ( rtgeom_is_empty(ctx, geom) )
    return RT_TRUE;

  switch (geom->type)
  {
  case RTPOINTTYPE:
    return rtpoint_check_geodetic(ctx, (RTPOINT *)geom);
  case RTLINETYPE:
    return rtline_check_geodetic(ctx, (RTLINE *)geom);
  case RTPOLYGONTYPE:
    return rtpoly_check_geodetic(ctx, (RTPOLY *)geom);
  case RTTRIANGLETYPE:
    return rttriangle_check_geodetic(ctx, (RTTRIANGLE *)geom);
  case RTMULTIPOINTTYPE:
  case RTMULTILINETYPE:
  case RTMULTIPOLYGONTYPE:
  case RTPOLYHEDRALSURFACETYPE:
  case RTTINTYPE:
  case RTCOLLECTIONTYPE:
    return rtcollection_check_geodetic(ctx, (RTCOLLECTION *)geom);
  default:
    rterror(ctx, "rtgeom_check_geodetic: unsupported input geometry type: %d - %s",
            geom->type, rttype_name(ctx, geom->type));
  }
  return RT_FALSE;
}

static int ptarray_force_geodetic(const RTCTX *ctx, RTPOINTARRAY *pa)
{
  int t;
  int changed = RT_FALSE;
  RTPOINT4D pt;

  assert(pa);

  for ( t=0; t < pa->npoints; t++ )
  {
    rt_getPoint4d_p(ctx, pa, t, &pt);
    if ( pt.x < -180.0 || pt.x > 180.0 || pt.y < -90.0 || pt.y > 90.0 )
    {
      pt.x = longitude_degrees_normalize(ctx, pt.x);
      pt.y = latitude_degrees_normalize(ctx, pt.y);
      ptarray_set_point4d(ctx, pa, t, &pt);
      changed = RT_TRUE;
    }
  }
  return changed;
}

static int rtpoint_force_geodetic(const RTCTX *ctx, RTPOINT *point)
{
  assert(point);
  return ptarray_force_geodetic(ctx, point->point);
}

static int rtline_force_geodetic(const RTCTX *ctx, RTLINE *line)
{
  assert(line);
  return ptarray_force_geodetic(ctx, line->points);
}

static int rtpoly_force_geodetic(const RTCTX *ctx, RTPOLY *poly)
{
  int i = 0;
  int changed = RT_FALSE;
  assert(poly);

  for ( i = 0; i < poly->nrings; i++ )
  {
    if ( ptarray_force_geodetic(ctx, poly->rings[i]) == RT_TRUE )
      changed = RT_TRUE;
  }
  return changed;
}

static int rtcollection_force_geodetic(const RTCTX *ctx, RTCOLLECTION *col)
{
  int i = 0;
  int changed = RT_FALSE;
  assert(col);

  for ( i = 0; i < col->ngeoms; i++ )
  {
    if ( rtgeom_force_geodetic(ctx, col->geoms[i]) == RT_TRUE )
      changed = RT_TRUE;
  }
  return changed;
}

int rtgeom_force_geodetic(const RTCTX *ctx, RTGEOM *geom)
{
  switch ( rtgeom_get_type(ctx, geom) )
  {
    case RTPOINTTYPE:
      return rtpoint_force_geodetic(ctx, (RTPOINT *)geom);
    case RTLINETYPE:
      return rtline_force_geodetic(ctx, (RTLINE *)geom);
    case RTPOLYGONTYPE:
      return rtpoly_force_geodetic(ctx, (RTPOLY *)geom);
    case RTMULTIPOINTTYPE:
    case RTMULTILINETYPE:
    case RTMULTIPOLYGONTYPE:
    case RTCOLLECTIONTYPE:
      return rtcollection_force_geodetic(ctx, (RTCOLLECTION *)geom);
    default:
      rterror(ctx, "unsupported input geometry type: %d", rtgeom_get_type(ctx, geom));
  }
  return RT_FALSE;
}


double ptarray_length_spheroid(const RTCTX *ctx, const RTPOINTARRAY *pa, const SPHEROID *s)
{
  GEOGRAPHIC_POINT a, b;
  double za = 0.0, zb = 0.0;
  RTPOINT4D p;
  int i;
  int hasz = RT_FALSE;
  double length = 0.0;
  double seglength = 0.0;

  /* Return zero on non-sensical inputs */
  if ( ! pa || pa->npoints < 2 )
    return 0.0;

  /* See if we have a third dimension */
  hasz = RTFLAGS_GET_Z(pa->flags);

  /* Initialize first point */
  rt_getPoint4d_p(ctx, pa, 0, &p);
  geographic_point_init(ctx, p.x, p.y, &a);
  if ( hasz )
    za = p.z;

  /* Loop and sum the length for each segment */
  for ( i = 1; i < pa->npoints; i++ )
  {
    seglength = 0.0;
    rt_getPoint4d_p(ctx, pa, i, &p);
    geographic_point_init(ctx, p.x, p.y, &b);
    if ( hasz )
      zb = p.z;

    /* Special sphere case */
    if ( s->a == s->b )
      seglength = s->radius * sphere_distance(ctx, &a, &b);
    /* Spheroid case */
    else
      seglength = spheroid_distance(ctx, &a, &b, s);

    /* Add in the vertical displacement if we're in 3D */
    if ( hasz )
      seglength = sqrt( (zb-za)*(zb-za) + seglength*seglength );

    /* Add this segment length to the total */
    length += seglength;

    /* B gets incremented in the next loop, so we save the value here */
    a = b;
    za = zb;
  }
  return length;
}

double rtgeom_length_spheroid(const RTCTX *ctx, const RTGEOM *geom, const SPHEROID *s)
{
  int type;
  int i = 0;
  double length = 0.0;

  assert(geom);

  /* No area in nothing */
  if ( rtgeom_is_empty(ctx, geom) )
    return 0.0;

  type = geom->type;

  if ( type == RTPOINTTYPE || type == RTMULTIPOINTTYPE )
    return 0.0;

  if ( type == RTLINETYPE )
    return ptarray_length_spheroid(ctx, ((RTLINE*)geom)->points, s);

  if ( type == RTPOLYGONTYPE )
  {
    RTPOLY *poly = (RTPOLY*)geom;
    for ( i = 0; i < poly->nrings; i++ )
    {
      length += ptarray_length_spheroid(ctx, poly->rings[i], s);
    }
    return length;
  }

  if ( type == RTTRIANGLETYPE )
    return ptarray_length_spheroid(ctx, ((RTTRIANGLE*)geom)->points, s);

  if ( rttype_is_collection(ctx,  type ) )
  {
    RTCOLLECTION *col = (RTCOLLECTION*)geom;

    for ( i = 0; i < col->ngeoms; i++ )
    {
      length += rtgeom_length_spheroid(ctx, col->geoms[i], s);
    }
    return length;
  }

  rterror(ctx, "unsupported type passed to rtgeom_length_sphere");
  return 0.0;
}

/**
* When features are snapped or sometimes they are just this way, they are very close to
* the geodetic bounds but slightly over. This routine nudges those points, and only
* those points, back over to the bounds.
* http://trac.osgeo.org/postgis/ticket/1292
*/
static int
ptarray_nudge_geodetic(const RTCTX *ctx, RTPOINTARRAY *pa)
{

  int i;
  RTPOINT4D p;
  int altered = RT_FALSE;
  int rv = RT_FALSE;
  static double tolerance = 1e-10;

  if ( ! pa )
    rterror(ctx, "ptarray_nudge_geodetic called with null input");

  for(i = 0; i < pa->npoints; i++ )
  {
    rt_getPoint4d_p(ctx, pa, i, &p);
    if ( p.x < -180.0 && (-180.0 - p.x < tolerance) )
    {
      p.x = -180.0;
      altered = RT_TRUE;
    }
    if ( p.x > 180.0 && (p.x - 180.0 < tolerance) )
    {
      p.x = 180.0;
      altered = RT_TRUE;
    }
    if ( p.y < -90.0 && (-90.0 - p.y < tolerance) )
    {
      p.y = -90.0;
      altered = RT_TRUE;
    }
    if ( p.y > 90.0 && (p.y - 90.0 < tolerance) )
    {
      p.y = 90.0;
      altered = RT_TRUE;
    }
    if ( altered == RT_TRUE )
    {
      ptarray_set_point4d(ctx, pa, i, &p);
      altered = RT_FALSE;
      rv = RT_TRUE;
    }
  }
  return rv;
}

/**
* When features are snapped or sometimes they are just this way, they are very close to
* the geodetic bounds but slightly over. This routine nudges those points, and only
* those points, back over to the bounds.
* http://trac.osgeo.org/postgis/ticket/1292
*/
int
rtgeom_nudge_geodetic(const RTCTX *ctx, RTGEOM *geom)
{
  int type;
  int i = 0;
  int rv = RT_FALSE;

  assert(geom);

  /* No points in nothing */
  if ( rtgeom_is_empty(ctx, geom) )
    return RT_FALSE;

  type = geom->type;

  if ( type == RTPOINTTYPE )
    return ptarray_nudge_geodetic(ctx, ((RTPOINT*)geom)->point);

  if ( type == RTLINETYPE )
    return ptarray_nudge_geodetic(ctx, ((RTLINE*)geom)->points);

  if ( type == RTPOLYGONTYPE )
  {
    RTPOLY *poly = (RTPOLY*)geom;
    for ( i = 0; i < poly->nrings; i++ )
    {
      int n = ptarray_nudge_geodetic(ctx, poly->rings[i]);
      rv = (rv == RT_TRUE ? rv : n);
    }
    return rv;
  }

  if ( type == RTTRIANGLETYPE )
    return ptarray_nudge_geodetic(ctx, ((RTTRIANGLE*)geom)->points);

  if ( rttype_is_collection(ctx,  type ) )
  {
    RTCOLLECTION *col = (RTCOLLECTION*)geom;

    for ( i = 0; i < col->ngeoms; i++ )
    {
      int n = rtgeom_nudge_geodetic(ctx, col->geoms[i]);
      rv = (rv == RT_TRUE ? rv : n);
    }
    return rv;
  }

  rterror(ctx, "unsupported type (%s) passed to rtgeom_nudge_geodetic", rttype_name(ctx, type));
  return rv;
}


/**
* Utility function for checking if P is within the cone defined by A1/A2.
*/
static int
point_in_cone(const RTCTX *ctx, const POINT3D *A1, const POINT3D *A2, const POINT3D *P)
{
  POINT3D AC; /* Center point of A1/A2 */
  double min_similarity, similarity;

  /* The normalized sum bisects the angle between start and end. */
  vector_sum(ctx, A1, A2, &AC);
  normalize(ctx, &AC);

  /* The projection of start onto the center defines the minimum similarity */
  min_similarity = dot_product(ctx, A1, &AC);

  /* The projection of candidate p onto the center */
  similarity = dot_product(ctx, P, &AC);

  /* If the point is more similar than the end, the point is in the cone */
  if ( similarity > min_similarity || fabs(similarity - min_similarity) < 2e-16 )
  {
    return RT_TRUE;
  }
  return RT_FALSE;
}


/**
* Utility function for ptarray_contains_point_sphere(ctx)
*/
static int
point3d_equals(const RTCTX *ctx, const POINT3D *p1, const POINT3D *p2)
{
  return FP_EQUALS(p1->x, p2->x) && FP_EQUALS(p1->y, p2->y) && FP_EQUALS(p1->z, p2->z);
}

/**
* Utility function for edge_intersects(ctx), signum with a tolerance
* in determining if the value is zero.
*/
static int
dot_product_side(const RTCTX *ctx, const POINT3D *p, const POINT3D *q)
{
  double dp = dot_product(ctx, p, q);

  if ( FP_IS_ZERO(dp) )
    return 0;

  return dp < 0.0 ? -1 : 1;
}

/**
* Returns non-zero if edges A and B interact. The type of interaction is given in the
* return value with the bitmask elements defined above.
*/
int
edge_intersects(const RTCTX *ctx, const POINT3D *A1, const POINT3D *A2, const POINT3D *B1, const POINT3D *B2)
{
  POINT3D AN, BN, VN;  /* Normals to plane A and plane B */
  double ab_dot;
  int a1_side, a2_side, b1_side, b2_side;
  int rv = PIR_NO_INTERACT;

  /* Normals to the A-plane and B-plane */
  unit_normal(ctx, A1, A2, &AN);
  unit_normal(ctx, B1, B2, &BN);

  /* Are A-plane and B-plane basically the same? */
  ab_dot = dot_product(ctx, &AN, &BN);
  if ( FP_EQUALS(fabs(ab_dot), 1.0) )
  {
    /* Co-linear case */
    if ( point_in_cone(ctx, A1, A2, B1) || point_in_cone(ctx, A1, A2, B2) ||
         point_in_cone(ctx, B1, B2, A1) || point_in_cone(ctx, B1, B2, A2) )
    {
      rv |= PIR_INTERSECTS;
      rv |= PIR_COLINEAR;
    }
    return rv;
  }

  /* What side of plane-A and plane-B do the end points */
  /* of A and B fall? */
  a1_side = dot_product_side(ctx, &BN, A1);
  a2_side = dot_product_side(ctx, &BN, A2);
  b1_side = dot_product_side(ctx, &AN, B1);
  b2_side = dot_product_side(ctx, &AN, B2);

  /* Both ends of A on the same side of plane B. */
  if ( a1_side == a2_side && a1_side != 0 )
  {
    /* No intersection. */
    return PIR_NO_INTERACT;
  }

  /* Both ends of B on the same side of plane A. */
  if ( b1_side == b2_side && b1_side != 0 )
  {
    /* No intersection. */
    return PIR_NO_INTERACT;
  }

  /* A straddles B and B straddles A, so... */
  if ( a1_side != a2_side && (a1_side + a2_side) == 0 &&
       b1_side != b2_side && (b1_side + b2_side) == 0 )
  {
    /* Have to check if intersection point is inside both arcs */
    unit_normal(ctx, &AN, &BN, &VN);
    if ( point_in_cone(ctx, A1, A2, &VN) && point_in_cone(ctx, B1, B2, &VN) )
    {
      return PIR_INTERSECTS;
    }

    /* Have to check if intersection point is inside both arcs */
    vector_scale(ctx, &VN, -1);
    if ( point_in_cone(ctx, A1, A2, &VN) && point_in_cone(ctx, B1, B2, &VN) )
    {
      return PIR_INTERSECTS;
    }

    return PIR_NO_INTERACT;
  }

  /* The rest are all intersects variants... */
  rv |= PIR_INTERSECTS;

  /* A touches B */
  if ( a1_side == 0 )
  {
    /* Touches at A1, A2 is on what side? */
    rv |= (a2_side < 0 ? PIR_A_TOUCH_RIGHT : PIR_A_TOUCH_LEFT);
  }
  else if ( a2_side == 0 )
  {
    /* Touches at A2, A1 is on what side? */
    rv |= (a1_side < 0 ? PIR_A_TOUCH_RIGHT : PIR_A_TOUCH_LEFT);
  }

  /* B touches A */
  if ( b1_side == 0 )
  {
    /* Touches at B1, B2 is on what side? */
    rv |= (b2_side < 0 ? PIR_B_TOUCH_RIGHT : PIR_B_TOUCH_LEFT);
  }
  else if ( b2_side == 0 )
  {
    /* Touches at B2, B1 is on what side? */
    rv |= (b1_side < 0 ? PIR_B_TOUCH_RIGHT : PIR_B_TOUCH_LEFT);
  }

  return rv;
}

/**
* This routine returns RT_TRUE if the stabline joining the pt_outside and pt_to_test
* crosses the ring an odd number of times, or if the pt_to_test is on the ring boundary itself,
* returning RT_FALSE otherwise.
* The pt_outside *must* be guaranteed to be outside the ring (use the geography_pt_outside() function
* to derive one in postgis, or the gbox_pt_outside(ctx) function if you don't mind burning CPU cycles
* building a gbox first).
*/
int ptarray_contains_point_sphere(const RTCTX *ctx, const RTPOINTARRAY *pa, const RTPOINT2D *pt_outside, const RTPOINT2D *pt_to_test)
{
  POINT3D S1, S2; /* Stab line end points */
  POINT3D E1, E2; /* Edge end points (3-space) */
  RTPOINT2D p; /* Edge end points (lon/lat) */
  int count = 0, i, inter;

  /* Null input, not enough points for a ring? You ain't closed! */
  if ( ! pa || pa->npoints < 4 )
    return RT_FALSE;

  /* Set up our stab line */
  ll2cart(ctx, pt_to_test, &S1);
  ll2cart(ctx, pt_outside, &S2);

  /* Initialize first point */
  rt_getPoint2d_p(ctx, pa, 0, &p);
  ll2cart(ctx, &p, &E1);

  /* Walk every edge and see if the stab line hits it */
  for ( i = 1; i < pa->npoints; i++ )
  {
    RTDEBUGF(ctx, 4, "testing edge (%d)", i);
    RTDEBUGF(ctx, 4, "  start point == POINT(%.12g %.12g)", p.x, p.y);

    /* Read next point. */
    rt_getPoint2d_p(ctx, pa, i, &p);
    ll2cart(ctx, &p, &E2);

    /* Skip over too-short edges. */
    if ( point3d_equals(ctx, &E1, &E2) )
    {
      continue;
    }

    /* Our test point is on an edge end! Point is "in ring" by our definition */
    if ( point3d_equals(ctx, &S1, &E1) )
    {
      return RT_TRUE;
    }

    /* Calculate relationship between stab line and edge */
    inter = edge_intersects(ctx, &S1, &S2, &E1, &E2);

    /* We have some kind of interaction... */
    if ( inter & PIR_INTERSECTS )
    {
      /* If the stabline is touching the edge, that implies the test point */
      /* is on the edge, so we're done, the point is in (on) the ring. */
      if ( (inter & PIR_A_TOUCH_RIGHT) || (inter & PIR_A_TOUCH_LEFT) )
      {
        return RT_TRUE;
      }

      /* It's a touching interaction, disregard all the left-side ones. */
      /* It's a co-linear intersection, ignore those. */
      if ( inter & PIR_B_TOUCH_RIGHT || inter & PIR_COLINEAR )
      {
        /* Do nothing, to avoid double counts. */
        RTDEBUGF(ctx, 4,"    edge (%d) crossed, disregarding to avoid double count", i, count);
      }
      else
      {
        /* Increment crossingn count. */
        count++;
        RTDEBUGF(ctx, 4,"    edge (%d) crossed, count == %d", i, count);
      }
    }
    else
    {
      RTDEBUGF(ctx, 4,"    edge (%d) did not cross", i);
    }

    /* Increment to next edge */
    E1 = E2;
  }

  RTDEBUGF(ctx, 4,"final count == %d", count);

  /* An odd number of crossings implies containment! */
  if ( count % 2 )
  {
    return RT_TRUE;
  }

  return RT_FALSE;
}
