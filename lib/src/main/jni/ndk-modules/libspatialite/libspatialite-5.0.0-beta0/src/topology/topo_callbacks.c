/*

 topo_callbacks.c -- implementation of Topology callback functions 
    
 version 4.3, 2015 July 18

 Author: Sandro Furieri a.furieri@lqt.it

 -----------------------------------------------------------------------------
 
 Version: MPL 1.1/GPL 2.0/LGPL 2.1
 
 The contents of this file are subject to the Mozilla Public License Version
 1.1 (the "License"); you may not use this file except in compliance with
 the License. You may obtain a copy of the License at
 http://www.mozilla.org/MPL/
 
Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
for the specific language governing rights and limitations under the
License.

The Original Code is the SpatiaLite library

The Initial Developer of the Original Code is Alessandro Furieri
 
Portions created by the Initial Developer are Copyright (C) 2015
the Initial Developer. All Rights Reserved.

Contributor(s): 

Alternatively, the contents of this file may be used under the terms of
either the GNU General Public License Version 2 or later (the "GPL"), or
the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
in which case the provisions of the GPL or the LGPL are applicable instead
of those above. If you wish to allow use of your version of this file only
under the terms of either the GPL or the LGPL, and not to allow others to
use your version of this file under the terms of the MPL, indicate your
decision by deleting the provisions above and replace them with the notice
and other provisions required by the GPL or the LGPL. If you do not delete
the provisions above, a recipient may use your version of this file under
the terms of any one of the MPL, the GPL or the LGPL.
 
*/

/*
 
CREDITS:

this module has been completely funded by:
Regione Toscana - Settore Sistema Informativo Territoriale ed Ambientale
(Topology support) 

CIG: 6038019AE5

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#ifdef ENABLE_RTTOPO		/* only if RTTOPO is enabled */

#include <spatialite/sqlite.h>
#include <spatialite/debug.h>
#include <spatialite/gaiageo.h>
#include <spatialite/gaia_topology.h>
#include <spatialite/gaiaaux.h>

#include <spatialite.h>
#include <spatialite_private.h>

#include <librttopo.h>

#include "topology_private.h"

struct topo_node
{
/* a struct wrapping a Topology Node */
    sqlite3_int64 node_id;
    sqlite3_int64 containing_face;
    double x;
    double y;
    double z;
    int has_z;
    struct topo_node *next;
};

struct topo_nodes_list
{
/* a struct wrapping a list of Topology Nodes */
    struct topo_node *first;
    struct topo_node *last;
    int count;
};

struct topo_edge
{
/* a struct wrapping a Topology Edge */
    sqlite3_int64 edge_id;
    sqlite3_int64 start_node;
    sqlite3_int64 end_node;
    sqlite3_int64 face_left;
    sqlite3_int64 face_right;
    sqlite3_int64 next_left;
    sqlite3_int64 next_right;
    gaiaLinestringPtr geom;
    struct topo_edge *next;
};

struct topo_edges_list
{
/* a struct wrapping a list of Topology Edegs */
    struct topo_edge *first;
    struct topo_edge *last;
    int count;
};

struct topo_face
{
/* a struct wrapping a Topology Face */
    sqlite3_int64 id;
    sqlite3_int64 face_id;
    double minx;
    double miny;
    double maxx;
    double maxy;
    struct topo_face *next;
};

struct topo_faces_list
{
/* a struct wrapping a list of Topology Edegs */
    struct topo_face *first;
    struct topo_face *last;
    int count;
};

static struct topo_node *
create_topo_node (sqlite3_int64 node_id, sqlite3_int64 containing_face,
		  double x, double y, double z, int has_z)
{
/* creating a Topology Node */
    struct topo_node *ptr = malloc (sizeof (struct topo_node));
    ptr->node_id = node_id;
    ptr->containing_face = containing_face;
    ptr->x = x;
    ptr->y = y;
    ptr->z = z;
    ptr->has_z = has_z;
    ptr->next = NULL;
    return ptr;
}

static void
destroy_topo_node (struct topo_node *ptr)
{
/* destroying a Topology Node */
    if (ptr == NULL)
	return;
    free (ptr);
}

static struct topo_edge *
create_topo_edge (sqlite3_int64 edge_id, sqlite3_int64 start_node,
		  sqlite3_int64 end_node, sqlite3_int64 face_left,
		  sqlite3_int64 face_right, sqlite3_int64 next_left,
		  sqlite3_int64 next_right, gaiaLinestringPtr ln)
{
/* creating a Topology Edge */
    struct topo_edge *ptr = malloc (sizeof (struct topo_edge));
    ptr->edge_id = edge_id;
    ptr->start_node = start_node;
    ptr->end_node = end_node;
    ptr->face_left = face_left;
    ptr->face_right = face_right;
    ptr->next_left = next_left;
    ptr->next_right = next_right;
    ptr->geom = ln;
    ptr->next = NULL;
    return ptr;
}

static void
destroy_topo_edge (struct topo_edge *ptr)
{
/* destroying a Topology Edge */
    if (ptr == NULL)
	return;
    if (ptr->geom != NULL)
	gaiaFreeLinestring (ptr->geom);
    free (ptr);
}

static struct topo_face *
create_topo_face (sqlite3_int64 id, sqlite3_int64 face_id, double minx,
		  double miny, double maxx, double maxy)
{
/* creating a Topology Face */
    struct topo_face *ptr = malloc (sizeof (struct topo_face));
    ptr->id = id;
    ptr->face_id = face_id;
    ptr->minx = minx;
    ptr->miny = miny;
    ptr->maxx = maxx;
    ptr->maxy = maxy;
    ptr->next = NULL;
    return ptr;
}

static void
destroy_topo_face (struct topo_face *ptr)
{
/* destroying a Topology Face */
    if (ptr == NULL)
	return;
    free (ptr);
}

static struct topo_nodes_list *
create_nodes_list (void)
{
/* creating an empty list of Topology Nodes */
    struct topo_nodes_list *ptr = malloc (sizeof (struct topo_nodes_list));
    ptr->first = NULL;
    ptr->last = NULL;
    ptr->count = 0;
    return ptr;
}

static void
destroy_nodes_list (struct topo_nodes_list *ptr)
{
/* destroying a list of Topology Nodes */
    struct topo_node *p;
    struct topo_node *pn;
    if (ptr == NULL)
	return;

    p = ptr->first;
    while (p != NULL)
      {
	  pn = p->next;
	  destroy_topo_node (p);
	  p = pn;
      }
    free (ptr);
}

static void
add_node_2D (struct topo_nodes_list *list, sqlite3_int64 node_id,
	     sqlite3_int64 containing_face, double x, double y)
{
/* inserting a Topology Node 2D into the list */
    struct topo_node *ptr;
    if (list == NULL)
	return;

    ptr = create_topo_node (node_id, containing_face, x, y, 0.0, 0);
    if (list->first == NULL)
	list->first = ptr;
    if (list->last != NULL)
	list->last->next = ptr;
    list->last = ptr;
    list->count++;
}

static void
add_node_3D (struct topo_nodes_list *list, sqlite3_int64 node_id,
	     sqlite3_int64 containing_face, double x, double y, double z)
{
/* inserting a Topology Node 3D into the list */
    struct topo_node *ptr;
    if (list == NULL)
	return;

    ptr = create_topo_node (node_id, containing_face, x, y, z, 1);
    if (list->first == NULL)
	list->first = ptr;
    if (list->last != NULL)
	list->last->next = ptr;
    list->last = ptr;
    list->count++;
}

static struct topo_edges_list *
create_edges_list (void)
{
/* creating an empty list of Topology Edges */
    struct topo_edges_list *ptr = malloc (sizeof (struct topo_edges_list));
    ptr->first = NULL;
    ptr->last = NULL;
    ptr->count = 0;
    return ptr;
}

static void
destroy_edges_list (struct topo_edges_list *ptr)
{
/* destroying a list of Topology Edges */
    struct topo_edge *p;
    struct topo_edge *pn;
    if (ptr == NULL)
	return;

    p = ptr->first;
    while (p != NULL)
      {
	  pn = p->next;
	  destroy_topo_edge (p);
	  p = pn;
      }
    free (ptr);
}

static void
add_edge (struct topo_edges_list *list, sqlite3_int64 edge_id,
	  sqlite3_int64 start_node, sqlite3_int64 end_node,
	  sqlite3_int64 face_left, sqlite3_int64 face_right,
	  sqlite3_int64 next_left, sqlite3_int64 next_right,
	  gaiaLinestringPtr ln)
{
/* inserting a Topology Edge into the list */
    struct topo_edge *ptr;
    if (list == NULL)
	return;

    ptr = list->first;
    while (ptr != NULL)
      {
	  /* avoiding to insert duplicate entries */
	  if (ptr->edge_id == edge_id)
	      return;
	  ptr = ptr->next;
      }

    ptr =
	create_topo_edge (edge_id, start_node, end_node, face_left, face_right,
			  next_left, next_right, ln);
    if (list->first == NULL)
	list->first = ptr;
    if (list->last != NULL)
	list->last->next = ptr;
    list->last = ptr;
    list->count++;
}

static struct topo_faces_list *
create_faces_list (void)
{
/* creating an empty list of Topology Faces */
    struct topo_faces_list *ptr = malloc (sizeof (struct topo_faces_list));
    ptr->first = NULL;
    ptr->last = NULL;
    ptr->count = 0;
    return ptr;
}

static void
destroy_faces_list (struct topo_faces_list *ptr)
{
/* destroying a list of Topology Faces */
    struct topo_face *p;
    struct topo_face *pn;
    if (ptr == NULL)
	return;

    p = ptr->first;
    while (p != NULL)
      {
	  pn = p->next;
	  destroy_topo_face (p);
	  p = pn;
      }
    free (ptr);
}

static void
add_face (struct topo_faces_list *list, sqlite3_int64 id, sqlite3_int64 face_id,
	  double minx, double miny, double maxx, double maxy)
{
/* inserting a Topology Face into the list */
    struct topo_face *ptr;
    if (list == NULL)
	return;

    ptr = create_topo_face (id, face_id, minx, miny, maxx, maxy);
    if (list->first == NULL)
	list->first = ptr;
    if (list->last != NULL)
	list->last->next = ptr;
    list->last = ptr;
    list->count++;
}

TOPOLOGY_PRIVATE RTLINE *
gaia_convert_linestring_to_rtline (const RTCTX * ctx, gaiaLinestringPtr ln,
				   int srid, int has_z)
{
/* converting a Linestring into an RTLINE */
    RTPOINTARRAY *pa;
    RTPOINT4D point;
    int iv;
    double x;
    double y;
    double z;
    double m;

    pa = ptarray_construct (ctx, has_z, 0, ln->Points);
    for (iv = 0; iv < ln->Points; iv++)
      {
	  /* copying vertices */
	  if (ln->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
	    }
	  else if (ln->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
	    }
	  else if (ln->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (ln->Coords, iv, &x, &y);
	    }
	  point.x = x;
	  point.y = y;
	  if (has_z)
	      point.z = z;
	  ptarray_set_point4d (ctx, pa, iv, &point);
      }
    return rtline_construct (ctx, srid, NULL, pa);
}

static int
check_unclosed_ring (gaiaRingPtr rng)
{
/* checks if a Ring is closed or not */
    double x0;
    double y0;
    double z0 = 0.0;
    double m0 = 0.0;
    double x1;
    double y1;
    double z1 = 0.0;
    double m1 = 0.0;
    int last = rng->Points - 1;
    if (rng->DimensionModel == GAIA_XY_Z)
      {
	  gaiaGetPointXYZ (rng->Coords, 0, &x0, &y0, &z0);
      }
    else if (rng->DimensionModel == GAIA_XY_M)
      {
	  gaiaGetPointXYM (rng->Coords, 0, &x0, &y0, &m0);
      }
    else if (rng->DimensionModel == GAIA_XY_Z_M)
      {
	  gaiaGetPointXYZM (rng->Coords, 0, &x0, &y0, &z0, &m0);
      }
    else
      {
	  gaiaGetPoint (rng->Coords, 0, &x0, &y0);
      }
    if (rng->DimensionModel == GAIA_XY_Z)
      {
	  gaiaGetPointXYZ (rng->Coords, last, &x1, &y1, &z1);
      }
    else if (rng->DimensionModel == GAIA_XY_M)
      {
	  gaiaGetPointXYM (rng->Coords, last, &x1, &y1, &m1);
      }
    else if (rng->DimensionModel == GAIA_XY_Z_M)
      {
	  gaiaGetPointXYZM (rng->Coords, last, &x1, &y1, &z1, &m1);
      }
    else
      {
	  gaiaGetPoint (rng->Coords, last, &x1, &y1);
      }
    if (x0 == x1 && y0 == y1 && z0 == z1 && m0 == m1)
	return 0;
    return 1;
}

TOPOLOGY_PRIVATE RTPOLY *
gaia_convert_polygon_to_rtpoly (const RTCTX * ctx, gaiaPolygonPtr pg, int srid,
				int has_z)
{
/* converting a Polygon into an RTPOLY */
    int ngeoms;
    RTPOINTARRAY **ppaa;
    RTPOINT4D point;
    gaiaRingPtr rng;
    int close_ring;
    int ib;
    int iv;
    double x;
    double y;
    double z;
    double m;

    ngeoms = pg->NumInteriors;
    ppaa = rtalloc (ctx, sizeof (RTPOINTARRAY *) * (ngeoms + 1));
    rng = pg->Exterior;
    close_ring = check_unclosed_ring (rng);
    if (close_ring)
	ppaa[0] = ptarray_construct (ctx, has_z, 0, rng->Points + 1);
    else
	ppaa[0] = ptarray_construct (ctx, has_z, 0, rng->Points);
    for (iv = 0; iv < rng->Points; iv++)
      {
	  /* copying vertices */
	  if (rng->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
	    }
	  else if (rng->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
	    }
	  else if (rng->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (rng->Coords, iv, &x, &y);
	    }
	  point.x = x;
	  point.y = y;
	  if (has_z)
	      point.z = z;
	  ptarray_set_point4d (ctx, ppaa[0], iv, &point);
      }
    if (close_ring)
      {
	  /* making an unclosed ring to be closed */
	  if (rng->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (rng->Coords, 0, &x, &y, &z);
	    }
	  else if (rng->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (rng->Coords, 0, &x, &y, &m);
	    }
	  else if (rng->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (rng->Coords, 0, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (rng->Coords, 0, &x, &y);
	    }
	  point.x = x;
	  point.y = y;
	  if (has_z)
	      point.z = z;
	  ptarray_set_point4d (ctx, ppaa[0], rng->Points, &point);
      }
    for (ib = 0; ib < pg->NumInteriors; ib++)
      {
	  /* copying vertices - Interior Rings */
	  rng = pg->Interiors + ib;
	  close_ring = check_unclosed_ring (rng);
	  if (close_ring)
	      ppaa[1 + ib] = ptarray_construct (ctx, has_z, 0, rng->Points + 1);
	  else
	      ppaa[1 + ib] = ptarray_construct (ctx, has_z, 0, rng->Points);
	  for (iv = 0; iv < rng->Points; iv++)
	    {
		/* copying vertices */
		if (rng->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
		  }
		else if (rng->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
		  }
		else if (rng->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (rng->Coords, iv, &x, &y);
		  }
		point.x = x;
		point.y = y;
		if (has_z)
		    point.z = z;
		ptarray_set_point4d (ctx, ppaa[1 + ib], iv, &point);
	    }
	  if (close_ring)
	    {
		/* making an unclosed ring to be closed */
		if (rng->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (rng->Coords, 0, &x, &y, &z);
		  }
		else if (rng->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (rng->Coords, 0, &x, &y, &m);
		  }
		else if (rng->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (rng->Coords, 0, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (rng->Coords, 0, &x, &y);
		  }
		point.x = x;
		point.y = y;
		if (has_z)
		    point.z = z;
		ptarray_set_point4d (ctx, ppaa[1 + ib], rng->Points, &point);
	    }
      }
    return rtpoly_construct (ctx, srid, NULL, ngeoms + 1, ppaa);
}

static gaiaGeomCollPtr
do_rtline_to_geom (const RTCTX * ctx, RTLINE * rtline, int srid)
{
/* converting a RTLINE into a Geometry (Linestring) */
    RTPOINTARRAY *pa;
    RTPOINT4D pt4d;
    int has_z = 0;
    double x;
    double y;
    double z;
    int iv;
    gaiaGeomCollPtr geom;
    gaiaLinestringPtr ln;

    pa = rtline->points;
    if (RTFLAGS_GET_Z (pa->flags))
	has_z = 1;
    if (has_z)
	geom = gaiaAllocGeomCollXYZ ();
    else
	geom = gaiaAllocGeomColl ();
    ln = gaiaAddLinestringToGeomColl (geom, pa->npoints);
    for (iv = 0; iv < pa->npoints; iv++)
      {
	  /* copying LINESTRING vertices */
	  rt_getPoint4d_p (ctx, pa, iv, &pt4d);
	  x = pt4d.x;
	  y = pt4d.y;
	  if (has_z)
	      z = pt4d.z;
	  if (has_z)
	    {
		gaiaSetPointXYZ (ln->Coords, iv, x, y, z);
	    }
	  else
	    {
		gaiaSetPoint (ln->Coords, iv, x, y);
	    }
      }
    geom->DeclaredType = GAIA_LINESTRING;
    geom->Srid = srid;

    return geom;
}

static char *
do_prepare_read_node (const char *topology_name, int fields, int has_z)
{
/* preparing the auxiliary "read_node" SQL statement */
    char *sql;
    char *prev;
    char *table;
    char *xtable;
    int comma = 0;

    sql = sqlite3_mprintf ("SELECT ");
    prev = sql;
    if (fields & RTT_COL_NODE_NODE_ID)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, node_id", prev);
	  else
	      sql = sqlite3_mprintf ("%s node_id", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_NODE_CONTAINING_FACE)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, containing_face", prev);
	  else
	      sql = sqlite3_mprintf ("%s containing_face", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_NODE_GEOM)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, ST_X(geom), ST_Y(geom)", prev);
	  else
	      sql = sqlite3_mprintf ("%s ST_X(geom), ST_Y(geom)", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
	  if (has_z)
	    {
		sql = sqlite3_mprintf ("%s, ST_Z(geom)", prev);
		sqlite3_free (prev);
		prev = sql;
	    }
      }
    table = sqlite3_mprintf ("%s_node", topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("%s FROM MAIN.\"%s\" WHERE node_id = ?", prev, xtable);
    sqlite3_free (prev);
    free (xtable);
    return sql;
}

static int
do_read_node (sqlite3_stmt * stmt, struct topo_nodes_list *list,
	      sqlite3_int64 id, int fields, int has_z,
	      const char *callback_name, char **errmsg)
{
/* reading Nodes out from the DBMS */
    int icol = 0;
    int ret;

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, id);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int ok_id = 0;
		int ok_face = 0;
		int ok_x = 0;
		int ok_y = 0;
		int ok_z = 0;
		sqlite3_int64 node_id = -1;
		sqlite3_int64 containing_face = -1;
		double x = 0.0;
		double y = 0.0;
		double z = 0.0;
		if (fields & RTT_COL_NODE_NODE_ID)
		  {
		      if (sqlite3_column_type (stmt, icol) == SQLITE_INTEGER)
			{
			    node_id = sqlite3_column_int64 (stmt, icol);
			    ok_id = 1;
			}
		      icol++;
		  }
		else
		    ok_id = 1;
		if (fields & RTT_COL_NODE_CONTAINING_FACE)
		  {
		      if (sqlite3_column_type (stmt, icol) == SQLITE_NULL)
			{
			    containing_face = -1;
			    ok_face = 1;
			}
		      if (sqlite3_column_type (stmt, icol) == SQLITE_INTEGER)
			{
			    containing_face = sqlite3_column_int64 (stmt, icol);
			    ok_face = 1;
			}
		      icol++;
		  }
		else
		    ok_face = 1;
		if (fields & RTT_COL_NODE_GEOM)
		  {
		      if (sqlite3_column_type (stmt, icol) == SQLITE_FLOAT)
			{
			    x = sqlite3_column_double (stmt, icol);
			    ok_x = 1;
			}
		      icol++;
		      if (sqlite3_column_type (stmt, icol) == SQLITE_FLOAT)
			{
			    y = sqlite3_column_double (stmt, icol);
			    ok_y = 1;
			}
		      icol++;
		      if (has_z)
			{
			    if (sqlite3_column_type (stmt, icol) ==
				SQLITE_FLOAT)
			      {
				  z = sqlite3_column_double (stmt, icol);
				  ok_z = 1;
			      }
			}
		  }
		else
		  {
		      ok_x = 1;
		      ok_y = 1;
		      ok_z = 1;
		  }
		if (has_z)
		  {
		      if (ok_id && ok_face && ok_x && ok_y && ok_z)
			{
			    add_node_3D (list, node_id, containing_face, x, y,
					 z);
			    *errmsg = NULL;
			    sqlite3_reset (stmt);
			    return 1;
			}
		  }
		else
		  {
		      if (ok_id && ok_face && ok_x && ok_y)
			{
			    add_node_2D (list, node_id, containing_face, x, y);
			    *errmsg = NULL;
			    sqlite3_reset (stmt);
			    return 1;
			}
		  }
		/* an invalid Node has been found */
		*errmsg =
		    sqlite3_mprintf
		    ("%s: found an invalid Node \"%lld\"", callback_name,
		     node_id);
		return 0;
	    }
      }
    *errmsg = NULL;
    sqlite3_reset (stmt);
    return 1;
}

static int
do_read_node_by_face (sqlite3_stmt * stmt, struct topo_nodes_list *list,
		      sqlite3_int64 id, int fields, const RTGBOX * box,
		      int has_z, const char *callback_name, char **errmsg)
{
/* reading Nodes out from the DBMS */
    int icol = 0;
    int ret;

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, id);
    if (box != NULL)
      {
	  sqlite3_bind_double (stmt, 2, box->xmin);
	  sqlite3_bind_double (stmt, 3, box->ymin);
	  sqlite3_bind_double (stmt, 4, box->xmax);
	  sqlite3_bind_double (stmt, 5, box->ymax);
      }

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int ok_id = 0;
		int ok_face = 0;
		int ok_x = 0;
		int ok_y = 0;
		int ok_z = 0;
		sqlite3_int64 node_id = -1;
		sqlite3_int64 containing_face = -1;
		double x = 0.0;
		double y = 0.0;
		double z = 0.0;
		if (fields & RTT_COL_NODE_NODE_ID)
		  {
		      if (sqlite3_column_type (stmt, icol) == SQLITE_INTEGER)
			{
			    node_id = sqlite3_column_int64 (stmt, icol);
			    ok_id = 1;
			}
		      icol++;
		  }
		else
		    ok_id = 1;
		if (fields & RTT_COL_NODE_CONTAINING_FACE)
		  {
		      if (sqlite3_column_type (stmt, icol) == SQLITE_NULL)
			{
			    containing_face = -1;
			    ok_face = 1;
			}
		      if (sqlite3_column_type (stmt, icol) == SQLITE_INTEGER)
			{
			    containing_face = sqlite3_column_int64 (stmt, icol);
			    ok_face = 1;
			}
		      icol++;
		  }
		else
		    ok_face = 1;
		if (fields & RTT_COL_NODE_GEOM)
		  {
		      if (sqlite3_column_type (stmt, icol) == SQLITE_FLOAT)
			{
			    x = sqlite3_column_double (stmt, icol);
			    ok_x = 1;
			}
		      icol++;
		      if (sqlite3_column_type (stmt, icol) == SQLITE_FLOAT)
			{
			    y = sqlite3_column_double (stmt, icol);
			    ok_y = 1;
			}
		      icol++;
		      if (has_z)
			{
			    if (sqlite3_column_type (stmt, icol) ==
				SQLITE_FLOAT)
			      {
				  z = sqlite3_column_double (stmt, icol);
				  ok_z = 1;
			      }
			}
		  }
		else
		  {
		      ok_x = 1;
		      ok_y = 1;
		      ok_z = 1;
		  }
		if (has_z)
		  {
		      if (ok_id && ok_face && ok_x && ok_y && ok_z)
			{
			    add_node_3D (list, node_id, containing_face, x, y,
					 z);
			    *errmsg = NULL;
			    sqlite3_reset (stmt);
			    return 1;
			}
		  }
		else
		  {
		      if (ok_id && ok_face && ok_x && ok_y)
			{
			    add_node_2D (list, node_id, containing_face, x, y);
			    *errmsg = NULL;
			    sqlite3_reset (stmt);
			    return 1;
			}
		  }
		/* an invalid Node has been found */
		*errmsg =
		    sqlite3_mprintf
		    ("%s: found an invalid Node \"%lld\"", callback_name,
		     node_id);
		return 0;
	    }
      }
    *errmsg = NULL;
    sqlite3_reset (stmt);
    return 1;
}

static char *
do_prepare_read_edge (const char *topology_name, int fields)
{
/* preparing the auxiliary "read_edge" SQL statement */
    char *sql;
    char *prev;
    char *table;
    char *xtable;
    int comma = 0;

    sql = sqlite3_mprintf ("SELECT ");
    prev = sql;
    /* unconditionally querying the Edge ID */
    if (comma)
	sql = sqlite3_mprintf ("%s, edge_id", prev);
    else
	sql = sqlite3_mprintf ("%s edge_id", prev);
    comma = 1;
    sqlite3_free (prev);
    prev = sql;
    if (fields & RTT_COL_EDGE_START_NODE)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, start_node", prev);
	  else
	      sql = sqlite3_mprintf ("%s start_node", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_EDGE_END_NODE)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, end_node", prev);
	  else
	      sql = sqlite3_mprintf ("%s end_node", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_EDGE_FACE_LEFT)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, left_face", prev);
	  else
	      sql = sqlite3_mprintf ("%s left_face", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_EDGE_FACE_RIGHT)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, right_face", prev);
	  else
	      sql = sqlite3_mprintf ("%s right_face", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_EDGE_NEXT_LEFT)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, next_left_edge", prev);
	  else
	      sql = sqlite3_mprintf ("%s next_left_edge", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_EDGE_NEXT_RIGHT)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, next_right_edge", prev);
	  else
	      sql = sqlite3_mprintf ("%s next_right_edge", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_EDGE_GEOM)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, geom", prev);
	  else
	      sql = sqlite3_mprintf ("%s geom", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    table = sqlite3_mprintf ("%s_edge", topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("%s FROM MAIN.\"%s\" WHERE edge_id = ?", prev, xtable);
    free (xtable);
    sqlite3_free (prev);
    return sql;
}

static int
do_read_edge_row (sqlite3_stmt * stmt, struct topo_edges_list *list, int fields,
		  const char *callback_name, char **errmsg)
{
/* reading an Edge Row out from the resultset */
    int icol = 0;

    int ok_id = 0;
    int ok_start = 0;
    int ok_end = 0;
    int ok_left = 0;
    int ok_right = 0;
    int ok_next_left = 0;
    int ok_next_right = 0;
    int ok_geom = 0;
    sqlite3_int64 edge_id;
    sqlite3_int64 start_node;
    sqlite3_int64 end_node;
    sqlite3_int64 face_left;
    sqlite3_int64 face_right;
    sqlite3_int64 next_left_edge;
    sqlite3_int64 next_right_edge;
    gaiaGeomCollPtr geom = NULL;
    gaiaLinestringPtr ln = NULL;
    /* unconditionally querying the Edge ID */
    if (sqlite3_column_type (stmt, icol) == SQLITE_INTEGER)
      {
	  edge_id = sqlite3_column_int64 (stmt, icol);
	  ok_id = 1;
      }
    icol++;
    if (fields & RTT_COL_EDGE_START_NODE)
      {
	  if (sqlite3_column_type (stmt, icol) == SQLITE_INTEGER)
	    {
		start_node = sqlite3_column_int64 (stmt, icol);
		ok_start = 1;
	    }
	  icol++;
      }
    else
	ok_start = 1;
    if (fields & RTT_COL_EDGE_END_NODE)
      {
	  if (sqlite3_column_type (stmt, icol) == SQLITE_INTEGER)
	    {
		end_node = sqlite3_column_int64 (stmt, icol);
		ok_end = 1;
	    }
	  icol++;
      }
    else
	ok_end = 1;
    if (fields & RTT_COL_EDGE_FACE_LEFT)
      {
	  if (sqlite3_column_type (stmt, icol) == SQLITE_NULL)
	    {
		face_left = -1;
		ok_left = 1;
	    }
	  if (sqlite3_column_type (stmt, icol) == SQLITE_INTEGER)
	    {
		face_left = sqlite3_column_int64 (stmt, icol);
		ok_left = 1;
	    }
	  icol++;
      }
    else
	ok_left = 1;
    if (fields & RTT_COL_EDGE_FACE_RIGHT)
      {
	  if (sqlite3_column_type (stmt, icol) == SQLITE_NULL)
	    {
		face_right = -1;
		ok_right = 1;
	    }
	  if (sqlite3_column_type (stmt, icol) == SQLITE_INTEGER)
	    {
		face_right = sqlite3_column_int64 (stmt, icol);
		ok_right = 1;
	    }
	  icol++;
      }
    else
	ok_right = 1;
    if (fields & RTT_COL_EDGE_NEXT_LEFT)
      {
	  if (sqlite3_column_type (stmt, icol) == SQLITE_INTEGER)
	    {
		next_left_edge = sqlite3_column_int64 (stmt, icol);
		ok_next_left = 1;
	    }
	  icol++;
      }
    else
	ok_next_left = 1;
    if (fields & RTT_COL_EDGE_NEXT_RIGHT)
      {
	  if (sqlite3_column_type (stmt, icol) == SQLITE_INTEGER)
	    {
		next_right_edge = sqlite3_column_int64 (stmt, icol);
		ok_next_right = 1;
	    }
	  icol++;
      }
    else
	ok_next_right = 1;
    if (fields & RTT_COL_EDGE_GEOM)
      {
	  if (sqlite3_column_type (stmt, icol) == SQLITE_BLOB)
	    {
		const unsigned char *blob = sqlite3_column_blob (stmt, icol);
		int blob_sz = sqlite3_column_bytes (stmt, icol);
		geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		if (geom != NULL)
		  {
		      if (geom->FirstPoint == NULL
			  && geom->FirstPolygon == NULL
			  && geom->FirstLinestring ==
			  geom->LastLinestring && geom->FirstLinestring != NULL)
			{
			    ok_geom = 1;
			    ln = geom->FirstLinestring;
			}
		  }
	    }
	  icol++;
      }
    else
	ok_geom = 1;
    if (ok_id && ok_start && ok_end && ok_left && ok_right
	&& ok_next_left && ok_next_right && ok_geom)
      {
	  add_edge (list, edge_id, start_node, end_node,
		    face_left, face_right, next_left_edge, next_right_edge, ln);
	  if (geom != NULL)
	    {
		/* releasing ownership on Linestring */
		geom->FirstLinestring = NULL;
		geom->LastLinestring = NULL;
		gaiaFreeGeomColl (geom);
	    }
	  *errmsg = NULL;
	  return 1;
      }
/* an invalid Edge has been found */
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    *errmsg =
	sqlite3_mprintf
	("%s: found an invalid Edge \"%lld\"", callback_name, edge_id);
    return 0;
}

static int
do_read_edge (sqlite3_stmt * stmt, struct topo_edges_list *list,
	      sqlite3_int64 edge_id, int fields, const char *callback_name,
	      char **errmsg)
{
/* reading a single Edge out from the DBMS */
    int ret;

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, edge_id);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (!do_read_edge_row
		    (stmt, list, fields, callback_name, errmsg))
		  {
		      sqlite3_reset (stmt);
		      return 0;
		  }
	    }
      }
    sqlite3_reset (stmt);
    return 1;
}

static int
do_read_edge_by_node (sqlite3_stmt * stmt, struct topo_edges_list *list,
		      sqlite3_int64 node_id, int fields,
		      const char *callback_name, char **errmsg)
{
/* reading a single Edge out from the DBMS */
    int ret;

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, node_id);
    sqlite3_bind_int64 (stmt, 2, node_id);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (!do_read_edge_row
		    (stmt, list, fields, callback_name, errmsg))
		  {
		      sqlite3_reset (stmt);
		      return 0;
		  }
	    }
      }
    sqlite3_reset (stmt);
    return 1;
}

static int
do_read_edge_by_face (sqlite3_stmt * stmt, struct topo_edges_list *list,
		      sqlite3_int64 face_id, int fields, const RTGBOX * box,
		      const char *callback_name, char **errmsg)
{
/* reading a single Edge out from the DBMS */
    int ret;

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, face_id);
    sqlite3_bind_int64 (stmt, 2, face_id);
    if (box != NULL)
      {
	  sqlite3_bind_double (stmt, 3, box->xmin);
	  sqlite3_bind_double (stmt, 4, box->ymin);
	  sqlite3_bind_double (stmt, 5, box->xmax);
	  sqlite3_bind_double (stmt, 6, box->ymax);
      }

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (!do_read_edge_row
		    (stmt, list, fields, callback_name, errmsg))
		  {
		      sqlite3_reset (stmt);
		      return 0;
		  }
	    }
      }
    sqlite3_reset (stmt);
    return 1;
}

static int
do_read_face (sqlite3_stmt * stmt, struct topo_faces_list *list,
	      sqlite3_int64 id, int fields, const char *callback_name,
	      char **errmsg)
{
/* reading Faces out from the DBMS */
    int icol = 0;
    int ret;

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    if (id <= 0)
	sqlite3_bind_int64 (stmt, 1, 0);
    else
	sqlite3_bind_int64 (stmt, 1, id);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int ok_id = 0;
		int ok_minx = 0;
		int ok_miny = 0;
		int ok_maxx = 0;
		int ok_maxy = 0;
		sqlite3_int64 face_id = -1;
		double minx = 0.0;
		double miny = 0.0;
		double maxx = 0.0;
		double maxy = 0.0;
		if (fields & RTT_COL_FACE_FACE_ID)
		  {
		      if (sqlite3_column_type (stmt, icol) == SQLITE_INTEGER)
			{
			    face_id = sqlite3_column_int64 (stmt, icol);
			    ok_id = 1;
			}
		      icol++;
		  }
		else
		    ok_id = 1;
		if (fields & RTT_COL_FACE_MBR)
		  {
		      if (id <= 0)
			{
			    ok_minx = 1;
			    ok_miny = 1;
			    ok_maxx = 1;
			    ok_maxy = 1;
			}
		      else
			{
			    if (sqlite3_column_type (stmt, icol) ==
				SQLITE_FLOAT)
			      {
				  minx = sqlite3_column_double (stmt, icol);
				  ok_minx = 1;
			      }
			    icol++;
			    if (sqlite3_column_type (stmt, icol) ==
				SQLITE_FLOAT)
			      {
				  miny = sqlite3_column_double (stmt, icol);
				  ok_miny = 1;
			      }
			    icol++;
			    if (sqlite3_column_type (stmt, icol) ==
				SQLITE_FLOAT)
			      {
				  maxx = sqlite3_column_double (stmt, icol);
				  ok_maxx = 1;
			      }
			    icol++;
			    if (sqlite3_column_type (stmt, icol) ==
				SQLITE_FLOAT)
			      {
				  maxy = sqlite3_column_double (stmt, icol);
				  ok_maxy = 1;
			      }
			    icol++;
			}
		  }
		else
		  {
		      ok_minx = 1;
		      ok_miny = 1;
		      ok_maxx = 1;
		      ok_maxy = 1;
		  }
		if (ok_id && ok_minx && ok_miny && ok_maxx && ok_maxy)
		  {
		      add_face (list, id, face_id, minx, miny, maxx, maxy);
		      *errmsg = NULL;
		      sqlite3_reset (stmt);
		      return 1;
		  }
		/* an invalid Face has been found */
		*errmsg =
		    sqlite3_mprintf
		    ("%s: found an invalid Face \"%lld\"", callback_name,
		     face_id);
		sqlite3_reset (stmt);
		return 0;
	    }
      }
    *errmsg = NULL;
    sqlite3_reset (stmt);
    return 1;
}

const char *
callback_lastErrorMessage (const RTT_BE_DATA * be)
{
    return gaiatopo_get_last_exception ((GaiaTopologyAccessorPtr) be);
}

RTT_BE_TOPOLOGY *
callback_loadTopologyByName (const RTT_BE_DATA * be, const char *name)
{
/* callback function: loadTopologyByName */
    struct gaia_topology *ptr = (struct gaia_topology *) be;
    char *topology_name;
    int srid;
    double tolerance;
    int has_z;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) ptr->cache;
    if (gaiaReadTopologyFromDBMS
	(ptr->db_handle, name, &topology_name, &srid, &tolerance, &has_z))
      {
	  ptr->topology_name = topology_name;
	  ptr->srid = srid;
	  ptr->tolerance = tolerance;
	  ptr->has_z = has_z;
	  /* registering into the Internal Cache double linked list */
	  if (cache->firstTopology == NULL)
	      cache->firstTopology = ptr;
	  if (cache->lastTopology != NULL)
	    {
		struct gaia_topology *p2 =
		    (struct gaia_topology *) (cache->lastTopology);
		p2->next = ptr;
	    }
	  cache->lastTopology = ptr;
	  return (RTT_BE_TOPOLOGY *) ptr;
      }
    else
	return NULL;
}

int
callback_freeTopology (RTT_BE_TOPOLOGY * rtt_topo)
{
/* callback function: freeTopology - does nothing */
    if (rtt_topo != NULL)
	rtt_topo = NULL;	/* silencing stupid compiler warnings on unuse args */
    return 1;
}

RTT_ISO_NODE *
callback_getNodeById (const RTT_BE_TOPOLOGY * rtt_topo,
		      const RTT_ELEMID * ids, int *numelems, int fields)
{
/* callback function: getNodeById */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    sqlite3_stmt *stmt_aux = NULL;
    int ret;
    int i;
    char *sql;
    RTPOINTARRAY *pa;
    RTPOINT4D pt4d;
    struct topo_nodes_list *list = NULL;
    RTT_ISO_NODE *result = NULL;
    if (accessor == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    cache = (struct splite_internal_cache *) accessor->cache;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

    /* preparing the SQL statement */
    sql =
	do_prepare_read_node (accessor->topology_name, fields, accessor->has_z);
    ret =
	sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql), &stmt_aux,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_getNodeById AUX error: \"%s\"",
				       sqlite3_errmsg (accessor->db_handle));
	  gaiatopo_set_last_error_msg (topo, msg);
	  sqlite3_free (msg);
	  *numelems = -1;
	  return NULL;
      }

    list = create_nodes_list ();
    for (i = 0; i < *numelems; i++)
      {
	  char *msg;
	  if (!do_read_node
	      (stmt_aux, list, *(ids + i), fields, accessor->has_z,
	       "callback_getNodeById", &msg))
	    {
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    if (list->count == 0)
      {
	  /* no node was found */
	  *numelems = list->count;
      }
    else
      {
	  struct topo_node *p_nd;
	  result = rtalloc (ctx, sizeof (RTT_ISO_NODE) * list->count);
	  p_nd = list->first;
	  i = 0;
	  while (p_nd != NULL)
	    {
		RTT_ISO_NODE *nd = result + i;
		if (fields & RTT_COL_NODE_NODE_ID)
		    nd->node_id = p_nd->node_id;
		if (fields & RTT_COL_NODE_CONTAINING_FACE)
		    nd->containing_face = p_nd->containing_face;
		if (fields & RTT_COL_NODE_GEOM)
		  {
		      pa = ptarray_construct (ctx, accessor->has_z, 0, 1);
		      pt4d.x = p_nd->x;
		      pt4d.y = p_nd->y;
		      if (accessor->has_z)
			  pt4d.z = p_nd->z;
		      ptarray_set_point4d (ctx, pa, 0, &pt4d);
		      nd->geom =
			  rtpoint_construct (ctx, accessor->srid, NULL, pa);
		  }
		i++;
		p_nd = p_nd->next;
	    }
	  *numelems = list->count;
      }
    sqlite3_finalize (stmt_aux);
    destroy_nodes_list (list);
    return result;

  error:
    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    if (list != NULL)
	destroy_nodes_list (list);
    *numelems = -1;
    return NULL;
}

RTT_ISO_NODE *
callback_getNodeWithinDistance2D (const RTT_BE_TOPOLOGY * rtt_topo,
				  const RTPOINT * pt, double dist,
				  int *numelems, int fields, int limit)
{
/* callback function: getNodeWithinDistance2D */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    sqlite3_stmt *stmt;
    int ret;
    double cx;
    double cy;
    RTPOINTARRAY *pa;
    RTPOINT4D pt4d;
    int count = 0;
    sqlite3_stmt *stmt_aux = NULL;
    char *sql;
    struct topo_nodes_list *list = NULL;
    RTT_ISO_NODE *result = NULL;
    if (accessor == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    stmt = accessor->stmt_getNodeWithinDistance2D;
    if (stmt == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    cache = (struct splite_internal_cache *) accessor->cache;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

    if (limit >= 0)
      {
	  /* preparing the auxiliary SQL statement */
	  sql =
	      do_prepare_read_node (accessor->topology_name, fields,
				    accessor->has_z);
	  ret =
	      sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql),
				  &stmt_aux, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		char *msg =
		    sqlite3_mprintf
		    ("Prepare_getNodeWithinDistance2D AUX error: \"%s\"",
		     sqlite3_errmsg (accessor->db_handle));
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		*numelems = -1;
		return NULL;
	    }
      }

/* extracting X and Y from RTPOINT */
    pa = pt->point;
    rt_getPoint4d_p (ctx, pa, 0, &pt4d);
    cx = pt4d.x;
    cy = pt4d.y;

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, cx);
    sqlite3_bind_double (stmt, 2, cy);
    sqlite3_bind_double (stmt, 3, dist);
    sqlite3_bind_double (stmt, 4, cx);
    sqlite3_bind_double (stmt, 5, cy);
    sqlite3_bind_double (stmt, 6, dist);
    list = create_nodes_list ();

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 node_id = sqlite3_column_int64 (stmt, 0);
		if (stmt_aux != NULL)
		  {
		      char *msg;
		      if (!do_read_node
			  (stmt_aux, list, node_id, fields, accessor->has_z,
			   "callback_getNodeWithinDistance2D", &msg))
			{
			    gaiatopo_set_last_error_msg (topo, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
		count++;
		if (limit > 0)
		  {
		      if (count > limit)
			  break;
		  }
		if (limit < 0)
		    break;
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("callback_getNodeWithinDistance2D: %s",
				     sqlite3_errmsg (accessor->db_handle));
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    if (limit < 0)
      {
	  result = NULL;
	  *numelems = count;
      }
    else
      {
	  if (list->count <= 0)
	    {
		result = NULL;
		*numelems = 0;
	    }
	  else
	    {
		int i = 0;
		struct topo_node *p_nd;
		result = rtalloc (ctx, sizeof (RTT_ISO_NODE) * list->count);
		p_nd = list->first;
		while (p_nd != NULL)
		  {
		      RTT_ISO_NODE *nd = result + i;
		      if (fields & RTT_COL_NODE_NODE_ID)
			  nd->node_id = p_nd->node_id;
		      if (fields & RTT_COL_NODE_CONTAINING_FACE)
			  nd->containing_face = p_nd->containing_face;
		      if (fields & RTT_COL_NODE_GEOM)
			{
			    pa = ptarray_construct (ctx, accessor->has_z, 0, 1);
			    pt4d.x = p_nd->x;
			    pt4d.y = p_nd->y;
			    if (accessor->has_z)
				pt4d.z = p_nd->z;
			    ptarray_set_point4d (ctx, pa, 0, &pt4d);
			    nd->geom =
				rtpoint_construct (ctx, accessor->srid, NULL,
						   pa);
			}
		      i++;
		      p_nd = p_nd->next;
		  }
		*numelems = list->count;
	    }
      }

    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    destroy_nodes_list (list);
    sqlite3_reset (stmt);
    return result;

  error:
    sqlite3_reset (stmt);
    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    if (list != NULL)
	destroy_nodes_list (list);
    *numelems = -1;
    return NULL;
}

int
callback_insertNodes (const RTT_BE_TOPOLOGY * rtt_topo, RTT_ISO_NODE * nodes,
		      int numelems)
{
/* callback function: insertNodes */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    sqlite3_stmt *stmt;
    int ret;
    int i;
    double x;
    double y;
    double z;
    RTPOINTARRAY *pa;
    RTPOINT4D pt4d;
    gaiaGeomCollPtr geom;
    unsigned char *p_blob;
    int n_bytes;
    int gpkg_mode = 0;
    int tiny_point = 0;
    if (accessor == NULL)
	return 0;

    stmt = accessor->stmt_insertNodes;
    if (stmt == NULL)
	return 0;

    cache = (struct splite_internal_cache *) accessor->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    if (accessor->cache != NULL)
      {
	  struct splite_internal_cache *cache =
	      (struct splite_internal_cache *) (accessor->cache);
	  gpkg_mode = cache->gpkg_mode;
	  tiny_point = cache->tinyPointEnabled;
      }

    for (i = 0; i < numelems; i++)
      {
	  RTT_ISO_NODE *nd = nodes + i;
	  /* setting up the prepared statement */
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  if (nd->node_id <= 0)
	      sqlite3_bind_null (stmt, 1);
	  else
	      sqlite3_bind_int64 (stmt, 1, nd->node_id);
	  if (nd->containing_face < 0)
	      sqlite3_bind_null (stmt, 2);
	  else
	      sqlite3_bind_int64 (stmt, 2, nd->containing_face);
	  if (accessor->has_z)
	      geom = gaiaAllocGeomCollXYZ ();
	  else
	      geom = gaiaAllocGeomColl ();
	  /* extracting X and Y from RTPOINT */
	  pa = nd->geom->point;
	  rt_getPoint4d_p (ctx, pa, 0, &pt4d);
	  x = pt4d.x;
	  y = pt4d.y;
	  if (accessor->has_z)
	    {
		z = pt4d.z;
		gaiaAddPointToGeomCollXYZ (geom, x, y, z);
	    }
	  else
	      gaiaAddPointToGeomColl (geom, x, y);
	  geom->Srid = accessor->srid;
	  geom->DeclaredType = GAIA_POINT;
	  gaiaToSpatiaLiteBlobWkbEx2 (geom, &p_blob, &n_bytes, gpkg_mode,
				      tiny_point);
	  gaiaFreeGeomColl (geom);
	  sqlite3_bind_blob (stmt, 3, p_blob, n_bytes, free);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	    {
		/* retrieving the PK value */
		nd->node_id = sqlite3_last_insert_rowid (accessor->db_handle);
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("callback_insertNodes: \"%s\"",
					     sqlite3_errmsg
					     (accessor->db_handle));
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_reset (stmt);
    return 1;

  error:
    sqlite3_reset (stmt);
    return 0;
}

RTT_ISO_EDGE *
callback_getEdgeById (const RTT_BE_TOPOLOGY * rtt_topo,
		      const RTT_ELEMID * ids, int *numelems, int fields)
{
/* callback function: getEdgeById */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    int ret;
    int i;
    sqlite3_stmt *stmt_aux = NULL;
    char *sql;
    struct topo_edges_list *list = NULL;
    RTT_ISO_EDGE *result = NULL;
    if (accessor == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    cache = (struct splite_internal_cache *) accessor->cache;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

    /* preparing the SQL statement */
    sql = do_prepare_read_edge (accessor->topology_name, fields);
    ret =
	sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql),
			    &stmt_aux, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_getEdgeById AUX error: \"%s\"",
				       sqlite3_errmsg (accessor->db_handle));
	  gaiatopo_set_last_error_msg (topo, msg);
	  sqlite3_free (msg);
	  *numelems = -1;
	  return NULL;
      }

    list = create_edges_list ();
    for (i = 0; i < *numelems; i++)
      {
	  char *msg;
	  if (!do_read_edge
	      (stmt_aux, list, *(ids + i), fields, "callback_getEdgeById",
	       &msg))
	    {
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    if (list->count == 0)
      {
	  /* no edge was found */
	  *numelems = list->count;
      }
    else
      {
	  struct topo_edge *p_ed;
	  result = rtalloc (ctx, sizeof (RTT_ISO_EDGE) * list->count);
	  p_ed = list->first;
	  i = 0;
	  while (p_ed != NULL)
	    {
		RTT_ISO_EDGE *ed = result + i;
		if (fields & RTT_COL_EDGE_EDGE_ID)
		    ed->edge_id = p_ed->edge_id;
		if (fields & RTT_COL_EDGE_START_NODE)
		    ed->start_node = p_ed->start_node;
		if (fields & RTT_COL_EDGE_END_NODE)
		    ed->end_node = p_ed->end_node;
		if (fields & RTT_COL_EDGE_FACE_LEFT)
		    ed->face_left = p_ed->face_left;
		if (fields & RTT_COL_EDGE_FACE_RIGHT)
		    ed->face_right = p_ed->face_right;
		if (fields & RTT_COL_EDGE_NEXT_LEFT)
		    ed->next_left = p_ed->next_left;
		if (fields & RTT_COL_EDGE_NEXT_RIGHT)
		    ed->next_right = p_ed->next_right;
		if (fields & RTT_COL_EDGE_GEOM)
		    ed->geom =
			gaia_convert_linestring_to_rtline (ctx, p_ed->geom,
							   accessor->srid,
							   accessor->has_z);
		i++;
		p_ed = p_ed->next;
	    }
	  *numelems = list->count;
      }
    sqlite3_finalize (stmt_aux);
    destroy_edges_list (list);
    return result;

  error:
    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    if (list != NULL)
	destroy_edges_list (list);
    *numelems = -1;
    return NULL;
}

RTT_ISO_EDGE *
callback_getEdgeWithinDistance2D (const RTT_BE_TOPOLOGY * rtt_topo,
				  const RTPOINT * pt, double dist,
				  int *numelems, int fields, int limit)
{
/* callback function: getEdgeWithinDistance2D */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    sqlite3_stmt *stmt;
    int ret;
    double cx;
    double cy;
    RTPOINTARRAY *pa;
    RTPOINT4D pt4d;
    int count = 0;
    sqlite3_stmt *stmt_aux = NULL;
    char *sql;
    struct topo_edges_list *list = NULL;
    RTT_ISO_EDGE *result = NULL;
    if (accessor == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    stmt = accessor->stmt_getEdgeWithinDistance2D;
    if (stmt == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    cache = (struct splite_internal_cache *) accessor->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    cache = (struct splite_internal_cache *) accessor->cache;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

    if (limit >= 0)
      {
	  /* preparing the auxiliary SQL statement */
	  sql = do_prepare_read_edge (accessor->topology_name, fields);
	  ret =
	      sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql),
				  &stmt_aux, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		char *msg =
		    sqlite3_mprintf ("Prepare_getEdgeById AUX error: \"%s\"",
				     sqlite3_errmsg (accessor->db_handle));
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		*numelems = -1;
		return NULL;
	    }
      }

/* extracting X and Y from RTPOINT */
    pa = pt->point;
    rt_getPoint4d_p (ctx, pa, 0, &pt4d);
    cx = pt4d.x;
    cy = pt4d.y;

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, cx);
    sqlite3_bind_double (stmt, 2, cy);
    sqlite3_bind_double (stmt, 3, dist);
    sqlite3_bind_double (stmt, 4, cx);
    sqlite3_bind_double (stmt, 5, cy);
    sqlite3_bind_double (stmt, 6, dist);
    list = create_edges_list ();

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 edge_id = sqlite3_column_int64 (stmt, 0);
		if (stmt_aux != NULL)
		  {
		      char *msg;
		      if (!do_read_edge
			  (stmt_aux, list, edge_id, fields,
			   "callback_getEdgeWithinDistance2D", &msg))
			{
			    gaiatopo_set_last_error_msg (topo, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
		count++;
		if (limit > 0)
		  {
		      if (count > limit)
			  break;
		  }
		if (limit < 0)
		    break;
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("callback_getEdgeWithinDistance2D: %s",
				     sqlite3_errmsg (accessor->db_handle));
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    if (limit < 0)
      {
	  result = NULL;
	  *numelems = count;
      }
    else
      {
	  if (list->count <= 0)
	    {
		result = NULL;
		*numelems = 0;
	    }
	  else
	    {
		int i = 0;
		struct topo_edge *p_ed;
		result = rtalloc (ctx, sizeof (RTT_ISO_EDGE) * list->count);
		p_ed = list->first;
		while (p_ed != NULL)
		  {
		      RTT_ISO_EDGE *ed = result + i;
		      if (fields & RTT_COL_EDGE_EDGE_ID)
			  ed->edge_id = p_ed->edge_id;
		      if (fields & RTT_COL_EDGE_START_NODE)
			  ed->start_node = p_ed->start_node;
		      if (fields & RTT_COL_EDGE_END_NODE)
			  ed->end_node = p_ed->end_node;
		      if (fields & RTT_COL_EDGE_FACE_LEFT)
			  ed->face_left = p_ed->face_left;
		      if (fields & RTT_COL_EDGE_FACE_RIGHT)
			  ed->face_right = p_ed->face_right;
		      if (fields & RTT_COL_EDGE_NEXT_LEFT)
			  ed->next_left = p_ed->next_left;
		      if (fields & RTT_COL_EDGE_NEXT_RIGHT)
			  ed->next_right = p_ed->next_right;
		      if (fields & RTT_COL_EDGE_GEOM)
			  ed->geom =
			      gaia_convert_linestring_to_rtline (ctx,
								 p_ed->geom,
								 accessor->srid,
								 accessor->
								 has_z);
		      i++;
		      p_ed = p_ed->next;
		  }
		*numelems = list->count;
	    }
      }
    sqlite3_reset (stmt);
    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    destroy_edges_list (list);
    return result;

  error:
    sqlite3_reset (stmt);
    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    if (list != NULL)
	destroy_edges_list (list);
    *numelems = -1;
    return NULL;
}

RTT_ELEMID
callback_getNextEdgeId (const RTT_BE_TOPOLOGY * rtt_topo)
{
/* callback function: getNextEdgeId */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    sqlite3_stmt *stmt_in;
    sqlite3_stmt *stmt_out;
    int ret;
    sqlite3_int64 edge_id = -1;
    if (accessor == NULL)
	return -1;

    stmt_in = accessor->stmt_getNextEdgeId;
    if (stmt_in == NULL)
	return -1;
    stmt_out = accessor->stmt_setNextEdgeId;
    if (stmt_out == NULL)
	return -1;

    cache = (struct splite_internal_cache *) accessor->cache;
    if (cache == NULL)
	return -1;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return -1;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return -1;

/* setting up the prepared statement */
    sqlite3_reset (stmt_in);
    sqlite3_clear_bindings (stmt_in);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		edge_id = sqlite3_column_int64 (stmt_in, 0);
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("callback_getNextEdgeId: %s",
					     sqlite3_errmsg
					     (accessor->db_handle));
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto stop;
	    }
      }

/* updating next_edge_id */
    sqlite3_reset (stmt_out);
    sqlite3_clear_bindings (stmt_out);
    ret = sqlite3_step (stmt_out);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  sqlite3_reset (stmt_in);
	  sqlite3_reset (stmt_out);
	  return edge_id;
      }
    else
      {
	  char *msg = sqlite3_mprintf ("callback_setNextEdgeId: \"%s\"",
				       sqlite3_errmsg (accessor->db_handle));
	  gaiatopo_set_last_error_msg (topo, msg);
	  sqlite3_free (msg);
	  edge_id = -1;
      }
  stop:
    if (edge_id >= 0)
	edge_id++;
    sqlite3_reset (stmt_in);
    sqlite3_reset (stmt_out);
    return edge_id;
}

int
callback_insertEdges (const RTT_BE_TOPOLOGY * rtt_topo, RTT_ISO_EDGE * edges,
		      int numelems)
{
/* callback function: insertEdges */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    sqlite3_stmt *stmt;
    int ret;
    int i;
    gaiaGeomCollPtr geom;
    unsigned char *p_blob;
    int n_bytes;
    int gpkg_mode = 0;
    int tiny_point = 0;
    if (accessor == NULL)
	return 0;

    stmt = accessor->stmt_insertEdges;
    if (stmt == NULL)
	return 0;

    cache = (struct splite_internal_cache *) accessor->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    if (accessor->cache != NULL)
      {
	  struct splite_internal_cache *cache =
	      (struct splite_internal_cache *) (accessor->cache);
	  gpkg_mode = cache->gpkg_mode;
	  tiny_point = cache->tinyPointEnabled;
      }

    for (i = 0; i < numelems; i++)
      {
	  RTT_ISO_EDGE *eg = edges + i;
	  /* setting up the prepared statement */
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  if (eg->edge_id <= 0)
	      sqlite3_bind_null (stmt, 1);
	  else
	      sqlite3_bind_int64 (stmt, 1, eg->edge_id);
	  sqlite3_bind_int64 (stmt, 2, eg->start_node);
	  sqlite3_bind_int64 (stmt, 3, eg->end_node);
	  if (eg->face_left < 0)
	      sqlite3_bind_null (stmt, 4);
	  else
	      sqlite3_bind_int64 (stmt, 4, eg->face_left);
	  if (eg->face_right < 0)
	      sqlite3_bind_null (stmt, 5);
	  else
	      sqlite3_bind_int64 (stmt, 5, eg->face_right);
	  sqlite3_bind_int64 (stmt, 6, eg->next_left);
	  sqlite3_bind_int64 (stmt, 7, eg->next_right);
	  /* transforming the RTLINE into a Geometry-Linestring */
	  geom = do_rtline_to_geom (ctx, eg->geom, accessor->srid);
	  gaiaToSpatiaLiteBlobWkbEx2 (geom, &p_blob, &n_bytes, gpkg_mode,
				      tiny_point);
	  gaiaFreeGeomColl (geom);
	  sqlite3_bind_blob (stmt, 8, p_blob, n_bytes, free);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	    {
		/* retrieving the PK value */
		eg->edge_id = sqlite3_last_insert_rowid (accessor->db_handle);
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("callback_insertEdges: \"%s\"",
					     sqlite3_errmsg
					     (accessor->db_handle));
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_reset (stmt);
    return 1;

  error:
    sqlite3_reset (stmt);
    return 0;
}

int
callback_updateEdges (const RTT_BE_TOPOLOGY * rtt_topo,
		      const RTT_ISO_EDGE * sel_edge, int sel_fields,
		      const RTT_ISO_EDGE * upd_edge, int upd_fields,
		      const RTT_ISO_EDGE * exc_edge, int exc_fields)
{
/* callback function: updateEdges */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *prev;
    int comma = 0;
    char *table;
    char *xtable;
    int icol = 1;
    unsigned char *p_blob;
    int n_bytes;
    int gpkg_mode = 0;
    int tiny_point = 0;
    int changed = 0;
    if (accessor == NULL)
	return -1;

    cache = (struct splite_internal_cache *) accessor->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    if (accessor->cache != NULL)
      {
	  struct splite_internal_cache *cache =
	      (struct splite_internal_cache *) (accessor->cache);
	  gpkg_mode = cache->gpkg_mode;
	  tiny_point = cache->tinyPointEnabled;
      }

/* composing the SQL prepared statement */
    table = sqlite3_mprintf ("%s_edge", accessor->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("UPDATE MAIN.\"%s\" SET ", xtable);
    free (xtable);
    prev = sql;
    if (upd_fields & RTT_COL_EDGE_EDGE_ID)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, edge_id = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s edge_id = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (upd_fields & RTT_COL_EDGE_START_NODE)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, start_node = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s start_node = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (upd_fields & RTT_COL_EDGE_END_NODE)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, end_node = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s end_node = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (upd_fields & RTT_COL_EDGE_FACE_LEFT)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, left_face = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s left_face = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (upd_fields & RTT_COL_EDGE_FACE_RIGHT)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, right_face = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s right_face = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (upd_fields & RTT_COL_EDGE_NEXT_LEFT)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, next_left_edge = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s next_left_edge = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (upd_fields & RTT_COL_EDGE_NEXT_RIGHT)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, next_right_edge = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s next_right_edge = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (upd_fields & RTT_COL_EDGE_GEOM)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, geom = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s geom = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (exc_edge || sel_edge)
      {
	  sql = sqlite3_mprintf ("%s WHERE", prev);
	  sqlite3_free (prev);
	  prev = sql;
	  if (sel_edge)
	    {
		comma = 0;
		if (sel_fields & RTT_COL_EDGE_EDGE_ID)
		  {
		      if (comma)
			  sql = sqlite3_mprintf ("%s AND edge_id = ?", prev);
		      else
			  sql = sqlite3_mprintf ("%s edge_id = ?", prev);
		      comma = 1;
		      sqlite3_free (prev);
		      prev = sql;
		  }
		if (sel_fields & RTT_COL_EDGE_START_NODE)
		  {
		      if (comma)
			  sql = sqlite3_mprintf ("%s AND start_node = ?", prev);
		      else
			  sql = sqlite3_mprintf ("%s start_node = ?", prev);
		      comma = 1;
		      sqlite3_free (prev);
		      prev = sql;
		  }
		if (sel_fields & RTT_COL_EDGE_END_NODE)
		  {
		      if (comma)
			  sql = sqlite3_mprintf ("%s AND end_node = ?", prev);
		      else
			  sql = sqlite3_mprintf ("%s end_node = ?", prev);
		      comma = 1;
		      sqlite3_free (prev);
		      prev = sql;
		  }
		if (sel_fields & RTT_COL_EDGE_FACE_LEFT)
		  {
		      if (sel_edge->face_left < 0)
			{
			    if (comma)
				sql =
				    sqlite3_mprintf ("%s AND left_face IS NULL",
						     prev);
			    else
				sql =
				    sqlite3_mprintf ("%s left_face IS NULL",
						     prev);
			}
		      else
			{
			    if (comma)
				sql =
				    sqlite3_mprintf ("%s AND left_face = ?",
						     prev);
			    else
				sql =
				    sqlite3_mprintf ("%s left_face = ?", prev);
			}
		      comma = 1;
		      sqlite3_free (prev);
		      prev = sql;
		  }
		if (sel_fields & RTT_COL_EDGE_FACE_RIGHT)
		  {
		      if (sel_edge->face_right < 0)
			{
			    if (comma)
				sql =
				    sqlite3_mprintf
				    ("%s AND right_face IS NULL", prev);
			    else
				sql =
				    sqlite3_mprintf ("%s right_face IS NULL",
						     prev);
			}
		      else
			{
			    if (comma)
				sql =
				    sqlite3_mprintf ("%s AND right_face = ?",
						     prev);
			    else
				sql =
				    sqlite3_mprintf ("%s right_face = ?", prev);
			}
		      comma = 1;
		      sqlite3_free (prev);
		      prev = sql;
		  }
		if (sel_fields & RTT_COL_EDGE_NEXT_LEFT)
		  {
		      if (comma)
			  sql =
			      sqlite3_mprintf
			      ("%s AND next_left_edge = ?", prev);
		      else
			  sql = sqlite3_mprintf ("%s next_left_edge = ?", prev);
		      comma = 1;
		      sqlite3_free (prev);
		      prev = sql;
		  }
		if (sel_fields & RTT_COL_EDGE_NEXT_RIGHT)
		  {
		      if (comma)
			  sql =
			      sqlite3_mprintf
			      ("%s AND next_right_edge = ?", prev);
		      else
			  sql =
			      sqlite3_mprintf ("%s next_right_edge = ?", prev);
		      comma = 1;
		      sqlite3_free (prev);
		      prev = sql;
		  }
	    }
	  if (exc_edge)
	    {
		if (sel_edge)
		  {
		      sql = sqlite3_mprintf ("%s AND", prev);
		      sqlite3_free (prev);
		      prev = sql;
		  }
		comma = 0;
		if (exc_fields & RTT_COL_EDGE_EDGE_ID)
		  {
		      if (comma)
			  sql = sqlite3_mprintf ("%s AND edge_id <> ?", prev);
		      else
			  sql = sqlite3_mprintf ("%s edge_id <> ?", prev);
		      comma = 1;
		      sqlite3_free (prev);
		      prev = sql;
		  }
		if (exc_fields & RTT_COL_EDGE_START_NODE)
		  {
		      if (comma)
			  sql =
			      sqlite3_mprintf ("%s AND start_node <> ?", prev);
		      else
			  sql = sqlite3_mprintf ("%s start_node <> ?", prev);
		      comma = 1;
		      sqlite3_free (prev);
		      prev = sql;
		  }
		if (exc_fields & RTT_COL_EDGE_END_NODE)
		  {
		      if (comma)
			  sql = sqlite3_mprintf ("%s AND end_node <> ?", prev);
		      else
			  sql = sqlite3_mprintf ("%s end_node <> ?", prev);
		      comma = 1;
		      sqlite3_free (prev);
		      prev = sql;
		  }
		if (exc_fields & RTT_COL_EDGE_FACE_LEFT)
		  {
		      if (exc_edge->face_left < 0)
			{
			    if (comma)
				sql =
				    sqlite3_mprintf
				    ("%s AND left_face IS NOT NULL", prev);
			    else
				sql =
				    sqlite3_mprintf ("%s left_face IS NOT NULL",
						     prev);
			}
		      else
			{
			    if (comma)
				sql =
				    sqlite3_mprintf ("%s AND left_face <> ?",
						     prev);
			    else
				sql =
				    sqlite3_mprintf ("%s left_face <> ?", prev);
			}
		      comma = 1;
		      sqlite3_free (prev);
		      prev = sql;
		  }
		if (exc_fields & RTT_COL_EDGE_FACE_RIGHT)
		  {
		      if (exc_edge->face_right < 0)
			{
			    if (comma)
				sql =
				    sqlite3_mprintf
				    ("%s AND right_face IS NOT NULL", prev);
			    else
				sql =
				    sqlite3_mprintf
				    ("%s right_face IS NOT NULL", prev);
			}
		      else
			{
			    if (comma)
				sql =
				    sqlite3_mprintf ("%s AND right_face <> ?",
						     prev);
			    else
				sql =
				    sqlite3_mprintf ("%s right_face <> ?",
						     prev);
			}
		      comma = 1;
		      sqlite3_free (prev);
		      prev = sql;
		  }
		if (exc_fields & RTT_COL_EDGE_NEXT_LEFT)
		  {
		      if (comma)
			  sql =
			      sqlite3_mprintf
			      ("%s AND next_left_edge <> ?", prev);
		      else
			  sql =
			      sqlite3_mprintf ("%s next_left_edge <> ?", prev);
		      comma = 1;
		      sqlite3_free (prev);
		      prev = sql;
		  }
		if (exc_fields & RTT_COL_EDGE_NEXT_RIGHT)
		  {
		      if (comma)
			  sql =
			      sqlite3_mprintf
			      ("%s AND next_right_edge <> ?", prev);
		      else
			  sql =
			      sqlite3_mprintf ("%s next_right_edge <> ?", prev);
		      comma = 1;
		      sqlite3_free (prev);
		      prev = sql;
		  }
	    }
      }
    ret =
	sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql), &stmt,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_updateEdges error: \"%s\"",
				       sqlite3_errmsg (accessor->db_handle));
	  gaiatopo_set_last_error_msg (topo, msg);
	  sqlite3_free (msg);
	  return -1;
      }

/* parameter binding */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    if (upd_fields & RTT_COL_EDGE_EDGE_ID)
      {
	  sqlite3_bind_int64 (stmt, icol, upd_edge->edge_id);
	  icol++;
      }
    if (upd_fields & RTT_COL_EDGE_START_NODE)
      {
	  sqlite3_bind_int64 (stmt, icol, upd_edge->start_node);
	  icol++;
      }
    if (upd_fields & RTT_COL_EDGE_END_NODE)
      {
	  sqlite3_bind_int64 (stmt, icol, upd_edge->end_node);
	  icol++;
      }
    if (upd_fields & RTT_COL_EDGE_FACE_LEFT)
      {
	  if (upd_edge->face_left < 0)
	      sqlite3_bind_null (stmt, icol);
	  else
	      sqlite3_bind_int64 (stmt, icol, upd_edge->face_left);
	  icol++;
      }
    if (upd_fields & RTT_COL_EDGE_FACE_RIGHT)
      {
	  if (upd_edge->face_right < 0)
	      sqlite3_bind_null (stmt, icol);
	  else
	      sqlite3_bind_int64 (stmt, icol, upd_edge->face_right);
	  icol++;
      }
    if (upd_fields & RTT_COL_EDGE_NEXT_LEFT)
      {
	  sqlite3_bind_int64 (stmt, icol, upd_edge->next_left);
	  icol++;
      }
    if (upd_fields & RTT_COL_EDGE_NEXT_RIGHT)
      {
	  sqlite3_bind_int64 (stmt, icol, upd_edge->next_right);
	  icol++;
      }
    if (upd_fields & RTT_COL_EDGE_GEOM)
      {
	  /* transforming the RTLINE into a Geometry-Linestring */
	  gaiaGeomCollPtr geom =
	      do_rtline_to_geom (ctx, upd_edge->geom, accessor->srid);
	  gaiaToSpatiaLiteBlobWkbEx2 (geom, &p_blob, &n_bytes, gpkg_mode,
				      tiny_point);
	  gaiaFreeGeomColl (geom);
	  sqlite3_bind_blob (stmt, icol, p_blob, n_bytes, free);
	  icol++;
      }
    if (sel_edge)
      {
	  if (sel_fields & RTT_COL_EDGE_EDGE_ID)
	    {
		sqlite3_bind_int64 (stmt, icol, sel_edge->edge_id);
		icol++;
	    }
	  if (sel_fields & RTT_COL_EDGE_START_NODE)
	    {
		sqlite3_bind_int64 (stmt, icol, sel_edge->start_node);
		icol++;
	    }
	  if (sel_fields & RTT_COL_EDGE_END_NODE)
	    {
		sqlite3_bind_int64 (stmt, icol, sel_edge->end_node);
		icol++;
	    }
	  if (sel_fields & RTT_COL_EDGE_FACE_LEFT)
	    {
		if (sel_edge->face_left < 0)
		    sqlite3_bind_null (stmt, icol);
		else
		    sqlite3_bind_int64 (stmt, icol, sel_edge->face_left);
		icol++;
	    }
	  if (sel_fields & RTT_COL_EDGE_FACE_RIGHT)
	    {
		if (sel_edge->face_right < 0)
		    sqlite3_bind_null (stmt, icol);
		else
		    sqlite3_bind_int64 (stmt, icol, sel_edge->face_right);
		icol++;
	    }
	  if (sel_fields & RTT_COL_EDGE_NEXT_LEFT)
	    {
		sqlite3_bind_int64 (stmt, icol, sel_edge->next_left);
		icol++;
	    }
	  if (sel_fields & RTT_COL_EDGE_NEXT_RIGHT)
	    {
		sqlite3_bind_int64 (stmt, icol, sel_edge->next_right);
		icol++;
	    }
      }
    if (exc_edge)
      {
	  if (exc_fields & RTT_COL_EDGE_EDGE_ID)
	    {
		sqlite3_bind_int64 (stmt, icol, exc_edge->edge_id);
		icol++;
	    }
	  if (exc_fields & RTT_COL_EDGE_START_NODE)
	    {
		sqlite3_bind_int64 (stmt, icol, exc_edge->start_node);
		icol++;
	    }
	  if (exc_fields & RTT_COL_EDGE_END_NODE)
	    {
		sqlite3_bind_int64 (stmt, icol, exc_edge->end_node);
		icol++;
	    }
	  if (exc_fields & RTT_COL_EDGE_FACE_LEFT)
	    {
		if (exc_edge->face_left < 0)
		    sqlite3_bind_null (stmt, icol);
		else
		    sqlite3_bind_int64 (stmt, icol, exc_edge->face_left);
		icol++;
	    }
	  if (exc_fields & RTT_COL_EDGE_FACE_RIGHT)
	    {
		if (exc_edge->face_right < 0)
		    sqlite3_bind_null (stmt, icol);
		else
		    sqlite3_bind_int64 (stmt, icol, exc_edge->face_right);
		icol++;
	    }
	  if (exc_fields & RTT_COL_EDGE_NEXT_LEFT)
	    {
		sqlite3_bind_int64 (stmt, icol, exc_edge->next_left);
		icol++;
	    }
	  if (exc_fields & RTT_COL_EDGE_NEXT_RIGHT)
	    {
		sqlite3_bind_int64 (stmt, icol, exc_edge->next_right);
		icol++;
	    }
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	changed = sqlite3_changes (accessor->db_handle);
    else
      {
	  char *msg = sqlite3_mprintf ("callback_updateEdges: \"%s\"",
				       sqlite3_errmsg (accessor->db_handle));
	  gaiatopo_set_last_error_msg (topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    sqlite3_finalize (stmt);
    return changed;

  error:
    sqlite3_finalize (stmt);
    return -1;
}

RTT_ISO_FACE *
callback_getFaceById (const RTT_BE_TOPOLOGY * rtt_topo,
		      const RTT_ELEMID * ids, int *numelems, int fields)
{
/* callback function: getFeceById */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    sqlite3_stmt *stmt_aux = NULL;
    int ret;
    int i;
    char *sql;
    char *prev;
    char *table;
    char *xtable;
    int comma = 0;
    struct topo_faces_list *list = NULL;
    RTT_ISO_FACE *result = NULL;
    if (accessor == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    cache = (struct splite_internal_cache *) accessor->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    /* preparing the SQL statement */
    sql = sqlite3_mprintf ("SELECT ");
    prev = sql;
    if (fields & RTT_COL_FACE_FACE_ID)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, face_id", prev);
	  else
	      sql = sqlite3_mprintf ("%s face_id", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_FACE_MBR)
      {
	  if (comma)
	      sql =
		  sqlite3_mprintf
		  ("%s, MbrMinX(mbr), MbrMinY(mbr), MbrMaxX(mbr), MbrMaxY(mbr)",
		   prev);
	  else
	      sql =
		  sqlite3_mprintf
		  ("%s MbrMinX(mbr), MbrMinY(mbr), MbrMaxX(mbr), MbrMaxY(mbr)",
		   prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    table = sqlite3_mprintf ("%s_face", accessor->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("%s FROM MAIN.\"%s\" WHERE face_id = ?", prev, xtable);
    sqlite3_free (prev);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql), &stmt_aux,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_getFaceById AUX error: \"%s\"",
				       sqlite3_errmsg (accessor->db_handle));
	  gaiatopo_set_last_error_msg (topo, msg);
	  sqlite3_free (msg);
	  *numelems = -1;
	  return NULL;
      }

    list = create_faces_list ();
    for (i = 0; i < *numelems; i++)
      {
	  char *msg;
	  if (!do_read_face
	      (stmt_aux, list, *(ids + i), fields, "callback_getFaceById",
	       &msg))
	    {
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    if (list->count == 0)
      {
	  /* no face was found */
	  *numelems = list->count;
      }
    else
      {
	  struct topo_face *p_fc;
	  result = rtalloc (ctx, sizeof (RTT_ISO_FACE) * list->count);
	  p_fc = list->first;
	  i = 0;
	  while (p_fc != NULL)
	    {
		RTT_ISO_FACE *fc = result + i;
		if (fields & RTT_COL_FACE_FACE_ID)
		    fc->face_id = p_fc->face_id;
		if (fields & RTT_COL_FACE_MBR)
		  {
		      if (p_fc->id == 0)
			  fc->mbr = NULL;
		      else
			{
			    fc->mbr = gbox_new (ctx, 0);
			    fc->mbr->xmin = p_fc->minx;
			    fc->mbr->ymin = p_fc->miny;
			    fc->mbr->xmax = p_fc->maxx;
			    fc->mbr->ymax = p_fc->maxy;
			}
		  }
		i++;
		p_fc = p_fc->next;
	    }
	  *numelems = list->count;
      }
    sqlite3_finalize (stmt_aux);
    destroy_faces_list (list);
    return result;

  error:
    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    if (list != NULL)
	destroy_faces_list (list);
    *numelems = -1;
    return NULL;
}

RTT_ELEMID
callback_getFaceContainingPoint (const RTT_BE_TOPOLOGY * rtt_topo,
				 const RTPOINT * pt)
{
/* callback function: getFaceContainingPoint */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    sqlite3_stmt *stmt;
    sqlite3_stmt *stmt_aux;
    int ret;
    double cx;
    double cy;
    float fx;
    float fy;
    double tic;
    double tic2;
    RTPOINTARRAY *pa;
    RTPOINT4D pt4d;
    int count = 0;
    sqlite3_int64 face_id;
    if (accessor == NULL)
	return -2;

    stmt = accessor->stmt_getFaceContainingPoint_1;
    if (stmt == NULL)
	return -2;
    stmt_aux = accessor->stmt_getFaceContainingPoint_2;
    if (stmt_aux == NULL)
	return -2;

    cache = (struct splite_internal_cache *) accessor->cache;
    if (cache == NULL)
	return -1;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return -1;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return -1;

/* extracting X and Y from RTPOINT */
    pa = pt->point;
    rt_getPoint4d_p (ctx, pa, 0, &pt4d);
    cx = pt4d.x;
    cy = pt4d.y;

/* adjusting the MBR so to compensate for DOUBLE/FLOAT truncations */
    fx = (float) cx;
    fy = (float) cy;
    tic = fabs (cx - fx);
    tic2 = fabs (cy - fy);
    if (tic2 > tic)
	tic = tic2;
    tic2 = fabs (cx - fx);
    if (tic2 > tic)
	tic = tic2;
    tic2 = fabs (cy - fy);
    if (tic2 > tic)
	tic = tic2;
    tic *= 2.0;

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, cx + tic);
    sqlite3_bind_double (stmt, 2, cx - tic);
    sqlite3_bind_double (stmt, 3, cy + tic);
    sqlite3_bind_double (stmt, 4, cy - tic);

    while (1)
      {
	  /* scrolling the result set rows [R*Tree] */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 id = sqlite3_column_int64 (stmt, 0);
		/* testing for real intersection */
		sqlite3_reset (stmt_aux);
		sqlite3_clear_bindings (stmt_aux);
		sqlite3_bind_int64 (stmt_aux, 1, id);
		sqlite3_bind_double (stmt_aux, 2, cx);
		sqlite3_bind_double (stmt_aux, 3, cy);
		while (1)
		  {
		      ret = sqlite3_step (stmt_aux);
		      if (ret == SQLITE_DONE)
			  break;	/* end of result set */
		      if (ret == SQLITE_ROW)
			{
			    if (sqlite3_column_type (stmt_aux, 0) ==
				SQLITE_INTEGER)
			      {
				  if (sqlite3_column_int (stmt_aux, 0) == 1)
				    {
					face_id = id;
					count++;
					break;
				    }
			      }
			}
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("callback_getFaceContainingPoint #2: %s",
				 sqlite3_errmsg (accessor->db_handle));
			    gaiatopo_set_last_error_msg (topo, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
		if (count > 0)
		    break;
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("callback_getFaceContainingPoint #1: %s",
				     sqlite3_errmsg (accessor->db_handle));
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    sqlite3_reset (stmt);
    if (count == 0)
	return -1;		/* none found */
    return face_id;

  error:
    sqlite3_reset (stmt);
    return -2;
}

int
callback_deleteEdges (const RTT_BE_TOPOLOGY * rtt_topo,
		      const RTT_ISO_EDGE * sel_edge, int sel_fields)
{
/* callback function: deleteEdgest */
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *prev;
    int comma = 0;
    char *table;
    char *xtable;
    int icol = 1;
    int changed = 0;
    if (accessor == NULL)
	return -1;

/* composing the SQL prepared statement */
    table = sqlite3_mprintf ("%s_edge", accessor->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM MAIN.\"%s\" WHERE", xtable);
    free (xtable);
    prev = sql;
    if (sel_fields & RTT_COL_EDGE_EDGE_ID)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s AND edge_id = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s edge_id = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (sel_fields & RTT_COL_EDGE_START_NODE)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s AND start_node = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s start_node = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (sel_fields & RTT_COL_EDGE_END_NODE)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s AND end_node = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s end_node = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (sel_fields & RTT_COL_EDGE_FACE_LEFT)
      {
	  if (sel_edge->face_left < 0)
	    {
		if (comma)
		    sql = sqlite3_mprintf ("%s AND left_face IS NULL", prev);
		else
		    sql = sqlite3_mprintf ("%s left_face IS NULL", prev);
	    }
	  else
	    {
		if (comma)
		    sql = sqlite3_mprintf ("%s AND left_face = ?", prev);
		else
		    sql = sqlite3_mprintf ("%s left_face = ?", prev);
	    }
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (sel_fields & RTT_COL_EDGE_FACE_RIGHT)
      {
	  if (sel_edge->face_right < 0)
	    {
		if (comma)
		    sql = sqlite3_mprintf ("%s AND right_face IS NULL", prev);
		else
		    sql = sqlite3_mprintf ("%s right_face IS NULL", prev);
	    }
	  else
	    {
		if (comma)
		    sql = sqlite3_mprintf ("%s AND right_face = ?", prev);
		else
		    sql = sqlite3_mprintf ("%s right_face = ?", prev);
	    }
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (sel_fields & RTT_COL_EDGE_NEXT_LEFT)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s AND next_left_edge = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s next_left_edge = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (sel_fields & RTT_COL_EDGE_NEXT_RIGHT)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s AND next_right_edge = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s next_right_edge = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (sel_fields & RTT_COL_EDGE_GEOM)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s AND geom = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s geom = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    ret =
	sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql), &stmt,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_deleteEdges error: \"%s\"",
				       sqlite3_errmsg (accessor->db_handle));
	  gaiatopo_set_last_error_msg (topo, msg);
	  sqlite3_free (msg);
	  return -1;
      }

/* parameter binding */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    if (sel_fields & RTT_COL_EDGE_EDGE_ID)
      {
	  sqlite3_bind_int64 (stmt, icol, sel_edge->edge_id);
	  icol++;
      }
    if (sel_fields & RTT_COL_EDGE_START_NODE)
      {
	  sqlite3_bind_int64 (stmt, icol, sel_edge->start_node);
	  icol++;
      }
    if (sel_fields & RTT_COL_EDGE_END_NODE)
      {
	  sqlite3_bind_int64 (stmt, icol, sel_edge->end_node);
	  icol++;
      }
    if (sel_fields & RTT_COL_EDGE_FACE_LEFT)
      {
	  if (sel_edge->face_left < 0)
	      sqlite3_bind_null (stmt, icol);
	  else
	      sqlite3_bind_int64 (stmt, icol, sel_edge->face_left);
	  icol++;
      }
    if (sel_fields & RTT_COL_EDGE_FACE_RIGHT)
      {
	  if (sel_edge->face_right < 0)
	      sqlite3_bind_null (stmt, icol);
	  else
	      sqlite3_bind_int64 (stmt, icol, sel_edge->face_right);
	  icol++;
      }
    if (sel_fields & RTT_COL_EDGE_NEXT_LEFT)
      {
	  sqlite3_bind_int64 (stmt, icol, sel_edge->next_left);
	  icol++;
      }
    if (sel_fields & RTT_COL_EDGE_NEXT_RIGHT)
      {
	  sqlite3_bind_int64 (stmt, icol, sel_edge->next_right);
	  icol++;
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	changed = sqlite3_changes (accessor->db_handle);
    else
      {
	  char *msg = sqlite3_mprintf ("callback_deleteEdges: \"%s\"",
				       sqlite3_errmsg (accessor->db_handle));
	  gaiatopo_set_last_error_msg (topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    sqlite3_finalize (stmt);
    return changed;

  error:
    sqlite3_finalize (stmt);
    return -1;
}

RTT_ISO_NODE *
callback_getNodeWithinBox2D (const RTT_BE_TOPOLOGY * rtt_topo,
			     const RTGBOX * box, int *numelems,
			     int fields, int limit)
{
/* callback function: getNodeWithinBox2D */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    sqlite3_stmt *stmt;
    int ret;
    RTPOINTARRAY *pa;
    RTPOINT4D pt4d;
    int count = 0;
    sqlite3_stmt *stmt_aux = NULL;
    char *sql;
    struct topo_nodes_list *list = NULL;
    RTT_ISO_NODE *result = NULL;
    if (accessor == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    stmt = accessor->stmt_getNodeWithinBox2D;
    if (stmt == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    cache = (struct splite_internal_cache *) accessor->cache;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

    if (limit >= 0)
      {
	  /* preparing the auxiliary SQL statement */
	  sql =
	      do_prepare_read_node (accessor->topology_name, fields,
				    accessor->has_z);
	  ret =
	      sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql),
				  &stmt_aux, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		char *msg =
		    sqlite3_mprintf
		    ("Prepare_getNodeWithinBox2D AUX error: \"%s\"",
		     sqlite3_errmsg (accessor->db_handle));
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		*numelems = -1;
		return NULL;
	    }
      }

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, box->xmin);
    sqlite3_bind_double (stmt, 2, box->ymin);
    sqlite3_bind_double (stmt, 3, box->xmax);
    sqlite3_bind_double (stmt, 4, box->ymax);
    list = create_nodes_list ();

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 node_id = sqlite3_column_int64 (stmt, 0);
		if (stmt_aux != NULL)
		  {
		      char *msg;
		      if (!do_read_node
			  (stmt_aux, list, node_id, fields, accessor->has_z,
			   "callback_getNodeWithinBox2D", &msg))
			{
			    gaiatopo_set_last_error_msg (topo, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
		count++;
		if (limit > 0)
		  {
		      if (count > limit)
			  break;
		  }
		if (limit < 0)
		    break;
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("callback_getNodeWithinBox2D: %s",
					     sqlite3_errmsg
					     (accessor->db_handle));
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    if (limit < 0)
      {
	  result = NULL;
	  *numelems = count;
      }
    else
      {
	  if (list->count <= 0)
	    {
		result = NULL;
		*numelems = 0;
	    }
	  else
	    {
		int i = 0;
		struct topo_node *p_nd;
		result = rtalloc (ctx, sizeof (RTT_ISO_NODE) * list->count);
		p_nd = list->first;
		while (p_nd != NULL)
		  {
		      RTT_ISO_NODE *nd = result + i;
		      if (fields & RTT_COL_NODE_NODE_ID)
			  nd->node_id = p_nd->node_id;
		      if (fields & RTT_COL_NODE_CONTAINING_FACE)
			  nd->containing_face = p_nd->containing_face;
		      if (fields & RTT_COL_NODE_GEOM)
			{
			    pa = ptarray_construct (ctx, accessor->has_z, 0, 1);
			    pt4d.x = p_nd->x;
			    pt4d.y = p_nd->y;
			    if (accessor->has_z)
				pt4d.z = p_nd->z;
			    ptarray_set_point4d (ctx, pa, 0, &pt4d);
			    nd->geom =
				rtpoint_construct (ctx, accessor->srid, NULL,
						   pa);
			}
		      i++;
		      p_nd = p_nd->next;
		  }
		*numelems = list->count;
	    }
      }

    sqlite3_reset (stmt);
    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    destroy_nodes_list (list);
    return result;

  error:
    sqlite3_reset (stmt);
    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    if (list != NULL)
	destroy_nodes_list (list);
    *numelems = 1;
    return NULL;
}

RTT_ISO_EDGE *
callback_getEdgeWithinBox2D (const RTT_BE_TOPOLOGY * rtt_topo,
			     const RTGBOX * box, int *numelems,
			     int fields, int limit)
{
/* callback function: getEdgeWithinBox2D */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    sqlite3_stmt *stmt;
    int ret;
    int count = 0;
    sqlite3_stmt *stmt_aux = NULL;
    char *sql;
    struct topo_edges_list *list = NULL;
    RTT_ISO_EDGE *result = NULL;

    if (box == NULL)
      {
	  /* special case - ignoring the Spatial Index and returning ALL edges */
	  return callback_getAllEdges (rtt_topo, numelems, fields, limit);
      }

    if (accessor == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    stmt = accessor->stmt_getEdgeWithinBox2D;
    if (stmt == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    cache = (struct splite_internal_cache *) accessor->cache;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

    if (limit >= 0)
      {
	  /* preparing the auxiliary SQL statement */
	  sql = do_prepare_read_edge (accessor->topology_name, fields);
	  ret =
	      sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql),
				  &stmt_aux, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		char *msg =
		    sqlite3_mprintf
		    ("Prepare_getEdgeWithinBox2D AUX error: \"%s\"",
		     sqlite3_errmsg (accessor->db_handle));
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		*numelems = -1;
		return NULL;
	    }
      }

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, box->xmin);
    sqlite3_bind_double (stmt, 2, box->ymin);
    sqlite3_bind_double (stmt, 3, box->xmax);
    sqlite3_bind_double (stmt, 4, box->ymax);
    list = create_edges_list ();

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 edge_id = sqlite3_column_int64 (stmt, 0);
		if (stmt_aux != NULL)
		  {
		      char *msg;
		      if (!do_read_edge
			  (stmt_aux, list, edge_id, fields,
			   "callback_getEdgeWithinBox2D", &msg))
			{
			    gaiatopo_set_last_error_msg (topo, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
		count++;
		if (limit > 0)
		  {
		      if (count > limit)
			  break;
		  }
		if (limit < 0)
		    break;
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("callback_getEdgeWithinBox2D: %s",
					     sqlite3_errmsg
					     (accessor->db_handle));
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    if (limit < 0)
      {
	  result = NULL;
	  *numelems = count;
      }
    else
      {
	  if (list->count <= 0)
	    {
		result = NULL;
		*numelems = 0;
	    }
	  else
	    {
		int i = 0;
		struct topo_edge *p_ed;
		result = rtalloc (ctx, sizeof (RTT_ISO_EDGE) * list->count);
		p_ed = list->first;
		while (p_ed != NULL)
		  {
		      RTT_ISO_EDGE *ed = result + i;
		      if (fields & RTT_COL_EDGE_EDGE_ID)
			  ed->edge_id = p_ed->edge_id;
		      if (fields & RTT_COL_EDGE_START_NODE)
			  ed->start_node = p_ed->start_node;
		      if (fields & RTT_COL_EDGE_END_NODE)
			  ed->end_node = p_ed->end_node;
		      if (fields & RTT_COL_EDGE_FACE_LEFT)
			  ed->face_left = p_ed->face_left;
		      if (fields & RTT_COL_EDGE_FACE_RIGHT)
			  ed->face_right = p_ed->face_right;
		      if (fields & RTT_COL_EDGE_NEXT_LEFT)
			  ed->next_left = p_ed->next_left;
		      if (fields & RTT_COL_EDGE_NEXT_RIGHT)
			  ed->next_right = p_ed->next_right;
		      if (fields & RTT_COL_EDGE_GEOM)
			  ed->geom =
			      gaia_convert_linestring_to_rtline (ctx,
								 p_ed->geom,
								 accessor->srid,
								 accessor->
								 has_z);
		      i++;
		      p_ed = p_ed->next;
		  }
		*numelems = list->count;
	    }
      }
    sqlite3_reset (stmt);
    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    destroy_edges_list (list);
    return result;

  error:
    sqlite3_reset (stmt);
    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    if (list != NULL)
	destroy_edges_list (list);
    *numelems = -1;
    return NULL;
}

RTT_ISO_EDGE *
callback_getAllEdges (const RTT_BE_TOPOLOGY * rtt_topo, int *numelems,
		      int fields, int limit)
{
/* callback function: getAllEdges */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    sqlite3_stmt *stmt;
    int ret;
    char *table;
    char *xtable;
    char *sql;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    int count = 0;
    RTT_ISO_EDGE *result = NULL;
    if (accessor == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    stmt = accessor->stmt_getAllEdges;
    if (stmt == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    cache = (struct splite_internal_cache *) accessor->cache;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

/* counting how many EDGEs are there */
    table = sqlite3_mprintf ("%s_edge", accessor->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT Count(*) FROM MAIN.\"%s\"", xtable);
    free (xtable);
    ret =
	sqlite3_get_table (accessor->db_handle, sql, &results, &rows, &columns,
			   &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return NULL;
      }
    for (i = 1; i <= rows; i++)
	count = atoi (results[(i * columns) + 0]);
    sqlite3_free_table (results);

    if (limit < 0)
      {
	  if (count <= 0)
	      *numelems = 0;
	  else
	      *numelems = 1;
	  return NULL;
      }
    if (count <= 0)
      {
	  *numelems = 0;
	  return NULL;
      }

/* allocating an Edge's array */
    if (limit > 0)
      {
	  if (limit > count)
	      *numelems = count;
	  else
	      *numelems = limit;
      }
    else
	*numelems = count;
    result = rtalloc (ctx, sizeof (RTT_ISO_EDGE) * *numelems);

    sqlite3_reset (stmt);
    i = 0;
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		RTT_ISO_EDGE *ed = result + i;
		if (fields & RTT_COL_EDGE_EDGE_ID)
		    ed->edge_id = sqlite3_column_int64 (stmt, 0);
		if (fields & RTT_COL_EDGE_START_NODE)
		    ed->start_node = sqlite3_column_int64 (stmt, 1);
		if (fields & RTT_COL_EDGE_END_NODE)
		    ed->end_node = sqlite3_column_int64 (stmt, 2);
		if (fields & RTT_COL_EDGE_FACE_LEFT)
		  {
		      if (sqlite3_column_type (stmt, 3) == SQLITE_NULL)
			  ed->face_left = -1;
		      else
			  ed->face_left = sqlite3_column_int64 (stmt, 3);
		  }
		if (fields & RTT_COL_EDGE_FACE_RIGHT)
		  {
		      if (sqlite3_column_type (stmt, 4) == SQLITE_NULL)
			  ed->face_right = -1;
		      else
			  ed->face_right = sqlite3_column_int64 (stmt, 4);
		  }
		if (fields & RTT_COL_EDGE_NEXT_LEFT)
		    ed->next_left = sqlite3_column_int64 (stmt, 5);
		if (fields & RTT_COL_EDGE_NEXT_RIGHT)
		    ed->next_right = sqlite3_column_int64 (stmt, 6);
		if (fields & RTT_COL_EDGE_GEOM)
		  {
		      if (sqlite3_column_type (stmt, 7) == SQLITE_BLOB)
			{
			    const unsigned char *blob =
				sqlite3_column_blob (stmt, 7);
			    int blob_sz = sqlite3_column_bytes (stmt, 7);
			    gaiaGeomCollPtr geom =
				gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
			    if (geom != NULL)
			      {
				  if (geom->FirstPoint == NULL
				      && geom->FirstPolygon == NULL
				      && geom->FirstLinestring ==
				      geom->LastLinestring
				      && geom->FirstLinestring != NULL)
				    {
					gaiaLinestringPtr ln =
					    geom->FirstLinestring;
					ed->geom =
					    gaia_convert_linestring_to_rtline
					    (ctx, ln, accessor->srid,
					     accessor->has_z);
				    }
				  gaiaFreeGeomColl (geom);
			      }
			}
		  }
		i++;
		if (limit > 0 && i >= limit)
		    break;
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("callback_getAllEdges: %s",
					     sqlite3_errmsg
					     (accessor->db_handle));
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_reset (stmt);
    return result;

  error:
    sqlite3_reset (stmt);
    *numelems = -1;
    return NULL;
}

RTT_ISO_EDGE *
callback_getEdgeByNode (const RTT_BE_TOPOLOGY * rtt_topo,
			const RTT_ELEMID * ids, int *numelems, int fields)
{
/* callback function: getEdgeByNode */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    int ret;
    char *sql;
    char *prev;
    char *table;
    char *xtable;
    int comma = 0;
    int i;
    sqlite3_stmt *stmt_aux = NULL;
    struct topo_edges_list *list = NULL;
    RTT_ISO_EDGE *result = NULL;
    if (accessor == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    cache = (struct splite_internal_cache *) accessor->cache;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

    /* preparing the SQL statement */
    sql = sqlite3_mprintf ("SELECT ");
    prev = sql;
    /* unconditionally querying the Edge ID */
    if (comma)
	sql = sqlite3_mprintf ("%s, edge_id", prev);
    else
	sql = sqlite3_mprintf ("%s edge_id", prev);
    comma = 1;
    sqlite3_free (prev);
    prev = sql;
    if (fields & RTT_COL_EDGE_START_NODE)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, start_node", prev);
	  else
	      sql = sqlite3_mprintf ("%s start_node", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_EDGE_END_NODE)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, end_node", prev);
	  else
	      sql = sqlite3_mprintf ("%s end_node", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_EDGE_FACE_LEFT)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, left_face", prev);
	  else
	      sql = sqlite3_mprintf ("%s left_face", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_EDGE_FACE_RIGHT)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, right_face", prev);
	  else
	      sql = sqlite3_mprintf ("%s right_face", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_EDGE_NEXT_LEFT)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, next_left_edge", prev);
	  else
	      sql = sqlite3_mprintf ("%s next_left_edge", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_EDGE_NEXT_RIGHT)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, next_right_edge", prev);
	  else
	      sql = sqlite3_mprintf ("%s next_right_edge", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_EDGE_GEOM)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, geom", prev);
	  else
	      sql = sqlite3_mprintf ("%s geom", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    table = sqlite3_mprintf ("%s_edge", accessor->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("%s FROM MAIN.\"%s\" WHERE start_node = ? OR end_node = ?", prev,
	 xtable);
    free (xtable);
    sqlite3_free (prev);
    ret =
	sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql),
			    &stmt_aux, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("Prepare_getEdgeByNode AUX error: \"%s\"",
			       sqlite3_errmsg (accessor->db_handle));
	  gaiatopo_set_last_error_msg (topo, msg);
	  sqlite3_free (msg);
	  *numelems = -1;
	  return NULL;
      }

    list = create_edges_list ();
    for (i = 0; i < *numelems; i++)
      {
	  char *msg;
	  if (!do_read_edge_by_node
	      (stmt_aux, list, *(ids + i), fields, "callback_getEdgeByNode",
	       &msg))
	    {
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    if (list->count == 0)
      {
	  /* no edge was found */
	  *numelems = list->count;
      }
    else
      {
	  struct topo_edge *p_ed;
	  result = rtalloc (ctx, sizeof (RTT_ISO_EDGE) * list->count);
	  p_ed = list->first;
	  i = 0;
	  while (p_ed != NULL)
	    {
		RTT_ISO_EDGE *ed = result + i;
		if (fields & RTT_COL_EDGE_EDGE_ID)
		    ed->edge_id = p_ed->edge_id;
		if (fields & RTT_COL_EDGE_START_NODE)
		    ed->start_node = p_ed->start_node;
		if (fields & RTT_COL_EDGE_END_NODE)
		    ed->end_node = p_ed->end_node;
		if (fields & RTT_COL_EDGE_FACE_LEFT)
		    ed->face_left = p_ed->face_left;
		if (fields & RTT_COL_EDGE_FACE_RIGHT)
		    ed->face_right = p_ed->face_right;
		if (fields & RTT_COL_EDGE_NEXT_LEFT)
		    ed->next_left = p_ed->next_left;
		if (fields & RTT_COL_EDGE_NEXT_RIGHT)
		    ed->next_right = p_ed->next_right;
		if (fields & RTT_COL_EDGE_GEOM)
		    ed->geom =
			gaia_convert_linestring_to_rtline (ctx, p_ed->geom,
							   accessor->srid,
							   accessor->has_z);
		i++;
		p_ed = p_ed->next;
	    }
	  *numelems = list->count;
      }
    sqlite3_finalize (stmt_aux);
    destroy_edges_list (list);
    return result;

  error:
    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    if (list != NULL)
	destroy_edges_list (list);
    *numelems = -1;
    return NULL;
}

int
callback_updateNodes (const RTT_BE_TOPOLOGY * rtt_topo,
		      const RTT_ISO_NODE * sel_node, int sel_fields,
		      const RTT_ISO_NODE * upd_node, int upd_fields,
		      const RTT_ISO_NODE * exc_node, int exc_fields)
{
/* callback function: updateNodes */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *prev;
    int comma = 0;
    char *table;
    char *xtable;
    int icol = 1;
    int changed = 0;
    RTPOINTARRAY *pa;
    RTPOINT4D pt4d;
    double x;
    double y;
    double z;
    if (accessor == NULL)
	return -1;

    cache = (struct splite_internal_cache *) accessor->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

/* composing the SQL prepared statement */
    table = sqlite3_mprintf ("%s_node", accessor->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("UPDATE MAIN.\"%s\" SET ", xtable);
    free (xtable);
    prev = sql;
    if (upd_fields & RTT_COL_NODE_NODE_ID)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, node_id = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s node_id = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (upd_fields & RTT_COL_NODE_CONTAINING_FACE)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, containing_face = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s containing_face = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (upd_fields & RTT_COL_NODE_GEOM)
      {
	  if (accessor->has_z)
	    {
		if (comma)
		    sql =
			sqlite3_mprintf ("%s, geom = MakePointZ(?, ?, ?, %d)",
					 prev, accessor->srid);
		else
		    sql =
			sqlite3_mprintf ("%s geom = MakePointZ(?, ?, ?, %d)",
					 prev, accessor->srid);
	    }
	  else
	    {
		if (comma)
		    sql =
			sqlite3_mprintf ("%s, geom = MakePoint(?, ?, %d)", prev,
					 accessor->srid);
		else
		    sql =
			sqlite3_mprintf ("%s geom = MakePoint(?, ?, %d)", prev,
					 accessor->srid);
	    }
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (exc_node || sel_node)
      {
	  sql = sqlite3_mprintf ("%s WHERE", prev);
	  sqlite3_free (prev);
	  prev = sql;
	  if (sel_node)
	    {
		comma = 0;
		if (sel_fields & RTT_COL_NODE_NODE_ID)
		  {
		      if (comma)
			  sql = sqlite3_mprintf ("%s AND node_id = ?", prev);
		      else
			  sql = sqlite3_mprintf ("%s node_id = ?", prev);
		      comma = 1;
		      sqlite3_free (prev);
		      prev = sql;
		  }
		if (sel_fields & RTT_COL_NODE_CONTAINING_FACE)
		  {
		      if (sel_node->containing_face < 0)
			{
			    if (comma)
				sql =
				    sqlite3_mprintf
				    ("%s AND containing_face IS NULL", prev);
			    else
				sql =
				    sqlite3_mprintf
				    ("%s containing_face IS NULL", prev);
			}
		      else
			{
			    if (comma)
				sql =
				    sqlite3_mprintf
				    ("%s AND containing_face = ?", prev);
			    else
				sql =
				    sqlite3_mprintf ("%s containing_face = ?",
						     prev);
			}
		      comma = 1;
		      sqlite3_free (prev);
		      prev = sql;
		  }
	    }
	  if (exc_node)
	    {
		if (sel_node)
		  {
		      sql = sqlite3_mprintf ("%s AND", prev);
		      sqlite3_free (prev);
		      prev = sql;
		  }
		comma = 0;
		if (exc_fields & RTT_COL_NODE_NODE_ID)
		  {
		      if (comma)
			  sql = sqlite3_mprintf ("%s AND node_id <> ?", prev);
		      else
			  sql = sqlite3_mprintf ("%s node_id <> ?", prev);
		      comma = 1;
		      sqlite3_free (prev);
		      prev = sql;
		  }
		if (exc_fields & RTT_COL_NODE_CONTAINING_FACE)
		  {
		      if (exc_node->containing_face < 0)
			{
			    if (comma)
				sql =
				    sqlite3_mprintf
				    ("%s AND containing_face IS NOT NULL",
				     prev);
			    else
				sql =
				    sqlite3_mprintf
				    ("%s containing_face IS NOT NULL", prev);
			}
		      else
			{
			    if (comma)
				sql =
				    sqlite3_mprintf
				    ("%s AND containing_face <> ?", prev);
			    else
				sql =
				    sqlite3_mprintf ("%s containing_face <> ?",
						     prev);
			}
		      comma = 1;
		      sqlite3_free (prev);
		      prev = sql;
		  }
	    }
      }
    ret =
	sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql), &stmt,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_updateNodes error: \"%s\"",
				       sqlite3_errmsg (accessor->db_handle));
	  gaiatopo_set_last_error_msg (topo, msg);
	  sqlite3_free (msg);
	  return -1;
      }

/* parameter binding */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    if (upd_fields & RTT_COL_NODE_NODE_ID)
      {
	  sqlite3_bind_int64 (stmt, icol, upd_node->node_id);
	  icol++;
      }
    if (upd_fields & RTT_COL_NODE_CONTAINING_FACE)
      {
	  if (upd_node->containing_face < 0)
	      sqlite3_bind_null (stmt, icol);
	  else
	      sqlite3_bind_int64 (stmt, icol, upd_node->containing_face);
	  icol++;
      }
    if (upd_fields & RTT_COL_NODE_GEOM)
      {
	  /* extracting X and Y from RTTOPO */
	  pa = upd_node->geom->point;
	  rt_getPoint4d_p (ctx, pa, 0, &pt4d);
	  x = pt4d.x;
	  y = pt4d.y;
	  if (accessor->has_z)
	      z = pt4d.z;
	  sqlite3_bind_double (stmt, icol, x);
	  icol++;
	  sqlite3_bind_double (stmt, icol, y);
	  icol++;
	  if (accessor->has_z)
	    {
		sqlite3_bind_double (stmt, icol, z);
		icol++;
	    }
      }
    if (sel_node)
      {
	  if (sel_fields & RTT_COL_NODE_NODE_ID)
	    {
		sqlite3_bind_int64 (stmt, icol, sel_node->node_id);
		icol++;
	    }
	  if (sel_fields & RTT_COL_NODE_CONTAINING_FACE)
	    {
		if (sel_node->containing_face < 0)
		    ;
		else
		  {
		      sqlite3_bind_int64 (stmt, icol,
					  sel_node->containing_face);
		      icol++;
		  }
	    }
      }
    if (exc_node)
      {
	  if (exc_fields & RTT_COL_NODE_NODE_ID)
	    {
		sqlite3_bind_int64 (stmt, icol, exc_node->node_id);
		icol++;
	    }
	  if (exc_fields & RTT_COL_NODE_CONTAINING_FACE)
	    {
		if (exc_node->containing_face < 0)
		    ;
		else
		  {
		      sqlite3_bind_int64 (stmt, icol,
					  exc_node->containing_face);
		      icol++;
		  }
	    }
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	changed = sqlite3_changes (accessor->db_handle);
    else
      {
	  char *msg = sqlite3_mprintf ("callback_updateNodes: \"%s\"",
				       sqlite3_errmsg (accessor->db_handle));
	  gaiatopo_set_last_error_msg (topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    sqlite3_finalize (stmt);
    return changed;

  error:
    sqlite3_finalize (stmt);
    return -1;
}

int
callback_insertFaces (const RTT_BE_TOPOLOGY * rtt_topo, RTT_ISO_FACE * faces,
		      int numelems)
{
/* callback function: insertFaces */
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    int ret;
    int i;
    int count = 0;
    sqlite3_stmt *stmt;
    if (accessor == NULL)
	return -1;

    stmt = accessor->stmt_insertFaces;
    if (stmt == NULL)
	return -1;

    for (i = 0; i < numelems; i++)
      {
	  RTT_ISO_FACE *fc = faces + i;
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  if (fc->face_id <= 0)
	      sqlite3_bind_null (stmt, 1);
	  else
	      sqlite3_bind_int64 (stmt, 1, fc->face_id);
	  sqlite3_bind_double (stmt, 2, fc->mbr->xmin);
	  sqlite3_bind_double (stmt, 3, fc->mbr->ymin);
	  sqlite3_bind_double (stmt, 4, fc->mbr->xmax);
	  sqlite3_bind_double (stmt, 5, fc->mbr->ymax);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	    {
		if (fc->face_id <= 0)
		    fc->face_id =
			sqlite3_last_insert_rowid (accessor->db_handle);
		count++;
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("callback_insertFaces: \"%s\"",
					     sqlite3_errmsg
					     (accessor->db_handle));
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_reset (stmt);
    return count;

  error:
    sqlite3_reset (stmt);
    return -1;
}

int
callback_updateFacesById (const RTT_BE_TOPOLOGY * rtt_topo,
			  const RTT_ISO_FACE * faces, int numfaces)
{
/* callback function: updateFacesById */
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    sqlite3_stmt *stmt = NULL;
    int ret;
    int i;
    int changed = 0;
    if (accessor == NULL)
	return -1;

    stmt = accessor->stmt_updateFacesById;
    if (stmt == NULL)
	return -1;

    for (i = 0; i < numfaces; i++)
      {
	  /* parameter binding */
	  const RTT_ISO_FACE *fc = faces + i;
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_double (stmt, 1, fc->mbr->xmin);
	  sqlite3_bind_double (stmt, 2, fc->mbr->ymin);
	  sqlite3_bind_double (stmt, 3, fc->mbr->xmax);
	  sqlite3_bind_double (stmt, 4, fc->mbr->ymax);
	  sqlite3_bind_int64 (stmt, 5, fc->face_id);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      changed += sqlite3_changes (accessor->db_handle);
	  else
	    {
		char *msg = sqlite3_mprintf ("callback_updateFacesById: \"%s\"",
					     sqlite3_errmsg
					     (accessor->db_handle));
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    return changed;

  error:
    return -1;
}

int
callback_deleteFacesById (const RTT_BE_TOPOLOGY * rtt_topo,
			  const RTT_ELEMID * ids, int numelems)
{
/* callback function: deleteFacesById */
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    sqlite3_stmt *stmt = NULL;
    int ret;
    int i;
    int changed = 0;
    if (accessor == NULL)
	return -1;

    stmt = accessor->stmt_deleteFacesById;
    if (stmt == NULL)
	return -1;

    for (i = 0; i < numelems; i++)
      {
	  /* parameter binding */
	  sqlite3_int64 id = *(ids + i);
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_int64 (stmt, 1, id);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	    {
		changed += sqlite3_changes (accessor->db_handle);
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("callback_deleteFacesById: \"%s\"",
					     sqlite3_errmsg
					     (accessor->db_handle));
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_reset (stmt);
    return changed;

  error:
    sqlite3_reset (stmt);
    return -1;
}

int
callback_deleteNodesById (const RTT_BE_TOPOLOGY * rtt_topo,
			  const RTT_ELEMID * ids, int numelems)
{
/* callback function: deleteNodesById */
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    sqlite3_stmt *stmt = NULL;
    int ret;
    int i;
    int changed = 0;
    if (accessor == NULL)
	return -1;

    stmt = accessor->stmt_deleteNodesById;
    if (stmt == NULL)
	return -1;

    for (i = 0; i < numelems; i++)
      {
	  /* parameter binding */
	  sqlite3_int64 id = *(ids + i);
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_int64 (stmt, 1, id);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	    {
		changed += sqlite3_changes (accessor->db_handle);
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("callback_deleteNodesById: \"%s\"",
					     sqlite3_errmsg
					     (accessor->db_handle));
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_reset (stmt);
    return changed;

  error:
    sqlite3_reset (stmt);
    return -1;
}

RTT_ELEMID *
callback_getRingEdges (const RTT_BE_TOPOLOGY * rtt_topo,
		       RTT_ELEMID edge, int *numedges, int limit)
{
/* callback function: getRingEdges */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    int ret;
    int i;
    int count = 0;
    sqlite3_stmt *stmt;

    struct topo_edges_list *list = NULL;
    RTT_ELEMID *result = NULL;
    if (accessor == NULL)
      {
	  *numedges = -1;
	  return NULL;
      }

    stmt = accessor->stmt_getRingEdges;
    if (stmt == NULL)
      {
	  *numedges = -1;
	  return NULL;
      }

    cache = (struct splite_internal_cache *) accessor->cache;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, edge);
    sqlite3_bind_double (stmt, 2, edge);
    list = create_edges_list ();

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 edge_id = sqlite3_column_int64 (stmt, 0);
		add_edge (list, edge_id, -1, -1, -1, -1, -1, -1, NULL);
		count++;
		if (limit > 0)
		  {
		      if (count > limit)
			  break;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("callback_getNodeWithinDistance2D: %s",
				     sqlite3_errmsg (accessor->db_handle));
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    if (limit < 0)
      {
	  result = NULL;
	  *numedges = count;
      }
    else
      {
	  if (list->count == 0)
	    {
		/* no edge was found */
		*numedges = 0;
	    }
	  else
	    {
		struct topo_edge *p_ed;
		result = rtalloc (ctx, sizeof (RTT_ELEMID) * list->count);
		p_ed = list->first;
		i = 0;
		while (p_ed != NULL)
		  {
		      *(result + i) = p_ed->edge_id;
		      i++;
		      p_ed = p_ed->next;
		  }
		*numedges = list->count;
	    }
      }
    destroy_edges_list (list);
    sqlite3_reset (stmt);
    return result;

  error:
    if (list != NULL)
	destroy_edges_list (list);
    *numedges = -1;
    sqlite3_reset (stmt);
    return NULL;
}

int
callback_updateEdgesById (const RTT_BE_TOPOLOGY * rtt_topo,
			  const RTT_ISO_EDGE * edges, int numedges,
			  int upd_fields)
{
/* callback function: updateEdgesById */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *prev;
    int comma = 0;
    char *table;
    char *xtable;
    int i;
    int changed = 0;
    unsigned char *p_blob;
    int n_bytes;
    int gpkg_mode = 0;
    int tiny_point = 0;
    if (accessor == NULL)
	return -1;

    cache = (struct splite_internal_cache *) accessor->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    if (accessor->cache != NULL)
      {
	  struct splite_internal_cache *cache =
	      (struct splite_internal_cache *) (accessor->cache);
	  gpkg_mode = cache->gpkg_mode;
	  tiny_point = cache->tinyPointEnabled;
      }

/* composing the SQL prepared statement */
    table = sqlite3_mprintf ("%s_edge", accessor->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("UPDATE MAIN.\"%s\" SET", xtable);
    free (xtable);
    prev = sql;
    if (upd_fields & RTT_COL_EDGE_EDGE_ID)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, edge_id = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s edge_id = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (upd_fields & RTT_COL_EDGE_START_NODE)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, start_node = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s start_node = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (upd_fields & RTT_COL_EDGE_END_NODE)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, end_node = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s end_node = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (upd_fields & RTT_COL_EDGE_FACE_LEFT)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, left_face = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s left_face = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (upd_fields & RTT_COL_EDGE_FACE_RIGHT)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, right_face = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s right_face = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (upd_fields & RTT_COL_EDGE_NEXT_LEFT)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, next_left_edge = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s next_left_edge = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (upd_fields & RTT_COL_EDGE_NEXT_RIGHT)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, next_right_edge = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s next_right_edge = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (upd_fields & RTT_COL_EDGE_GEOM)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, geom = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s geom = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    sql = sqlite3_mprintf ("%s WHERE edge_id = ?", prev);
    sqlite3_free (prev);
    ret =
	sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql), &stmt,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_updateEdgesById error: \"%s\"",
				       sqlite3_errmsg (accessor->db_handle));
	  gaiatopo_set_last_error_msg (topo, msg);
	  sqlite3_free (msg);
	  return -1;
      }

    for (i = 0; i < numedges; i++)
      {
	  /* parameter binding */
	  int icol = 1;
	  const RTT_ISO_EDGE *upd_edge = edges + i;
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  if (upd_fields & RTT_COL_EDGE_EDGE_ID)
	    {
		sqlite3_bind_int64 (stmt, icol, upd_edge->edge_id);
		icol++;
	    }
	  if (upd_fields & RTT_COL_EDGE_START_NODE)
	    {
		sqlite3_bind_int64 (stmt, icol, upd_edge->start_node);
		icol++;
	    }
	  if (upd_fields & RTT_COL_EDGE_END_NODE)
	    {
		sqlite3_bind_int64 (stmt, icol, upd_edge->end_node);
		icol++;
	    }
	  if (upd_fields & RTT_COL_EDGE_FACE_LEFT)
	    {
		if (upd_edge->face_left < 0)
		    sqlite3_bind_null (stmt, icol);
		else
		    sqlite3_bind_int64 (stmt, icol, upd_edge->face_left);
		icol++;
	    }
	  if (upd_fields & RTT_COL_EDGE_FACE_RIGHT)
	    {
		if (upd_edge->face_right < 0)
		    sqlite3_bind_null (stmt, icol);
		else
		    sqlite3_bind_int64 (stmt, icol, upd_edge->face_right);
		icol++;
	    }
	  if (upd_fields & RTT_COL_EDGE_NEXT_LEFT)
	    {
		sqlite3_bind_int64 (stmt, icol, upd_edge->next_left);
		icol++;
	    }
	  if (upd_fields & RTT_COL_EDGE_NEXT_RIGHT)
	    {
		sqlite3_bind_int64 (stmt, icol, upd_edge->next_right);
		icol++;
	    }
	  if (upd_fields & RTT_COL_EDGE_GEOM)
	    {
		/* transforming the RTLINE into a Geometry-Linestring */
		gaiaGeomCollPtr geom =
		    do_rtline_to_geom (ctx, upd_edge->geom, accessor->srid);
		gaiaToSpatiaLiteBlobWkbEx2 (geom, &p_blob, &n_bytes, gpkg_mode,
					    tiny_point);
		gaiaFreeGeomColl (geom);
		sqlite3_bind_blob (stmt, icol, p_blob, n_bytes, free);
		icol++;
	    }
	  sqlite3_bind_int64 (stmt, icol, upd_edge->edge_id);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      changed += sqlite3_changes (accessor->db_handle);
	  else
	    {
		char *msg = sqlite3_mprintf ("callback_updateEdgesById: \"%s\"",
					     sqlite3_errmsg
					     (accessor->db_handle));
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    return changed;

  error:
    sqlite3_finalize (stmt);
    return -1;
}

RTT_ISO_EDGE *
callback_getEdgeByFace (const RTT_BE_TOPOLOGY * rtt_topo,
			const RTT_ELEMID * ids, int *numelems, int fields,
			const RTGBOX * box)
{
/* callback function: getEdgeByFace */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    int ret;
    char *sql;
    char *prev;
    char *table;
    char *xtable;
    int comma = 0;
    int i;
    sqlite3_stmt *stmt_aux = NULL;
    struct topo_edges_list *list = NULL;
    RTT_ISO_EDGE *result = NULL;
    if (accessor == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    cache = (struct splite_internal_cache *) accessor->cache;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

    /* preparing the SQL statement */
    sql = sqlite3_mprintf ("SELECT ");
    prev = sql;
    /* unconditionally querying the Edge ID */
    if (comma)
	sql = sqlite3_mprintf ("%s, edge_id", prev);
    else
	sql = sqlite3_mprintf ("%s edge_id", prev);
    comma = 1;
    sqlite3_free (prev);
    prev = sql;
    if (fields & RTT_COL_EDGE_START_NODE)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, start_node", prev);
	  else
	      sql = sqlite3_mprintf ("%s start_node", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_EDGE_END_NODE)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, end_node", prev);
	  else
	      sql = sqlite3_mprintf ("%s end_node", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_EDGE_FACE_LEFT)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, left_face", prev);
	  else
	      sql = sqlite3_mprintf ("%s left_face", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_EDGE_FACE_RIGHT)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, right_face", prev);
	  else
	      sql = sqlite3_mprintf ("%s right_face", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_EDGE_NEXT_LEFT)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, next_left_edge", prev);
	  else
	      sql = sqlite3_mprintf ("%s next_left_edge", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_EDGE_NEXT_RIGHT)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, next_right_edge", prev);
	  else
	      sql = sqlite3_mprintf ("%s next_right_edge", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_EDGE_GEOM)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, geom", prev);
	  else
	      sql = sqlite3_mprintf ("%s geom", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    table = sqlite3_mprintf ("%s_edge", accessor->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("%s FROM MAIN.\"%s\" WHERE (left_face = ? OR right_face = ?)", prev,
	 xtable);
    free (xtable);
    sqlite3_free (prev);
    if (box != NULL)
      {
	  table = sqlite3_mprintf ("%s_edge", accessor->topology_name);
	  prev = sql;
	  sql =
	      sqlite3_mprintf
	      ("%s AND ROWID IN (SELECT ROWID FROM SpatialIndex WHERE "
	       "f_table_name = %Q AND f_geometry_column = 'geom' AND search_frame = BuildMBR(?, ?, ?, ?))",
	       sql, table);
	  sqlite3_free (table);
	  sqlite3_free (prev);
      }
    ret =
	sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql),
			    &stmt_aux, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("Prepare_getEdgeByFace AUX error: \"%s\"",
			       sqlite3_errmsg (accessor->db_handle));
	  gaiatopo_set_last_error_msg (topo, msg);
	  sqlite3_free (msg);
	  *numelems = -1;
	  return NULL;
      }

    list = create_edges_list ();
    for (i = 0; i < *numelems; i++)
      {
	  char *msg;
	  if (!do_read_edge_by_face
	      (stmt_aux, list, *(ids + i), fields, box,
	       "callback_getEdgeByFace", &msg))
	    {
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    if (list->count == 0)
      {
	  /* no edge was found */
	  *numelems = list->count;
      }
    else
      {
	  struct topo_edge *p_ed;
	  result = rtalloc (ctx, sizeof (RTT_ISO_EDGE) * list->count);
	  p_ed = list->first;
	  i = 0;
	  while (p_ed != NULL)
	    {
		RTT_ISO_EDGE *ed = result + i;
		if (fields & RTT_COL_EDGE_EDGE_ID)
		    ed->edge_id = p_ed->edge_id;
		if (fields & RTT_COL_EDGE_START_NODE)
		    ed->start_node = p_ed->start_node;
		if (fields & RTT_COL_EDGE_END_NODE)
		    ed->end_node = p_ed->end_node;
		if (fields & RTT_COL_EDGE_FACE_LEFT)
		    ed->face_left = p_ed->face_left;
		if (fields & RTT_COL_EDGE_FACE_RIGHT)
		    ed->face_right = p_ed->face_right;
		if (fields & RTT_COL_EDGE_NEXT_LEFT)
		    ed->next_left = p_ed->next_left;
		if (fields & RTT_COL_EDGE_NEXT_RIGHT)
		    ed->next_right = p_ed->next_right;
		if (fields & RTT_COL_EDGE_GEOM)
		    ed->geom =
			gaia_convert_linestring_to_rtline (ctx, p_ed->geom,
							   accessor->srid,
							   accessor->has_z);
		i++;
		p_ed = p_ed->next;
	    }
	  *numelems = list->count;
      }
    sqlite3_finalize (stmt_aux);
    destroy_edges_list (list);
    return result;

  error:
    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    if (list != NULL)
	destroy_edges_list (list);
    *numelems = -1;
    return NULL;
}

RTT_ISO_NODE *
callback_getNodeByFace (const RTT_BE_TOPOLOGY * rtt_topo,
			const RTT_ELEMID * faces, int *numelems, int fields,
			const RTGBOX * box)
{
/* callback function: getNodeByFace */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    sqlite3_stmt *stmt_aux = NULL;
    int ret;
    int i;
    char *sql;
    char *prev;
    char *table;
    char *xtable;
    int comma = 0;
    RTPOINTARRAY *pa;
    RTPOINT4D pt4d;
    struct topo_nodes_list *list = NULL;
    RTT_ISO_NODE *result = NULL;
    if (accessor == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    cache = (struct splite_internal_cache *) accessor->cache;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

    /* preparing the SQL statement */
    sql = sqlite3_mprintf ("SELECT ");
    prev = sql;
    if (fields & RTT_COL_NODE_NODE_ID)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, node_id", prev);
	  else
	      sql = sqlite3_mprintf ("%s node_id", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_NODE_CONTAINING_FACE)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, containing_face", prev);
	  else
	      sql = sqlite3_mprintf ("%s containing_face", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & RTT_COL_NODE_GEOM)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, ST_X(geom), ST_Y(geom)", prev);
	  else
	      sql = sqlite3_mprintf ("%s ST_X(geom), ST_Y(geom)", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
	  if (accessor->has_z)
	    {
		sql = sqlite3_mprintf ("%s, ST_Z(geom)", prev);
		sqlite3_free (prev);
		prev = sql;
	    }
      }
    table = sqlite3_mprintf ("%s_node", accessor->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("%s FROM MAIN.\"%s\" WHERE containing_face = ?", prev,
			 xtable);
    free (xtable);
    sqlite3_free (prev);
    if (box != NULL)
      {
	  table = sqlite3_mprintf ("%s_node", accessor->topology_name);
	  prev = sql;
	  sql =
	      sqlite3_mprintf
	      ("%s AND ROWID IN (SELECT ROWID FROM SpatialIndex WHERE "
	       "f_table_name = %Q AND f_geometry_column = 'geom' AND search_frame = BuildMBR(?, ?, ?, ?))",
	       sql, table);
	  sqlite3_free (table);
	  sqlite3_free (prev);
      }
    ret =
	sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql), &stmt_aux,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("Prepare_getNodeByFace AUX error: \"%s\"",
			       sqlite3_errmsg (accessor->db_handle));
	  gaiatopo_set_last_error_msg (topo, msg);
	  sqlite3_free (msg);
	  *numelems = -1;
	  return NULL;
      }

    list = create_nodes_list ();
    for (i = 0; i < *numelems; i++)
      {
	  char *msg;
	  if (!do_read_node_by_face
	      (stmt_aux, list, *(faces + i), fields, box, accessor->has_z,
	       "callback_getNodeByFace", &msg))
	    {
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    if (list->count == 0)
      {
	  /* no node was found */
	  *numelems = list->count;
      }
    else
      {
	  struct topo_node *p_nd;
	  result = rtalloc (ctx, sizeof (RTT_ISO_NODE) * list->count);
	  p_nd = list->first;
	  i = 0;
	  while (p_nd != NULL)
	    {
		RTT_ISO_NODE *nd = result + i;
		if (fields & RTT_COL_NODE_NODE_ID)
		    nd->node_id = p_nd->node_id;
		if (fields & RTT_COL_NODE_CONTAINING_FACE)
		    nd->containing_face = p_nd->containing_face;
		if (fields & RTT_COL_NODE_GEOM)
		  {
		      pa = ptarray_construct (ctx, accessor->has_z, 0, 1);
		      pt4d.x = p_nd->x;
		      pt4d.y = p_nd->y;
		      if (accessor->has_z)
			  pt4d.z = p_nd->z;
		      ptarray_set_point4d (ctx, pa, 0, &pt4d);
		      nd->geom =
			  rtpoint_construct (ctx, accessor->srid, NULL, pa);
		  }
		i++;
		p_nd = p_nd->next;
	    }
	  *numelems = list->count;
      }
    sqlite3_finalize (stmt_aux);
    destroy_nodes_list (list);
    return result;

  error:
    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    if (list != NULL)
	destroy_nodes_list (list);
    *numelems = -1;
    return NULL;
}

int
callback_updateNodesById (const RTT_BE_TOPOLOGY * rtt_topo,
			  const RTT_ISO_NODE * nodes, int numnodes,
			  int upd_fields)
{
/* callback function: updateNodesById */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *prev;
    int comma = 0;
    char *table;
    char *xtable;
    int icol = 1;
    int i;
    int changed = 0;
    if (accessor == NULL)
	return -1;

    cache = (struct splite_internal_cache *) accessor->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

/* composing the SQL prepared statement */
    table = sqlite3_mprintf ("%s_node", accessor->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("UPDATE MAIN.\"%s\" SET", xtable);
    free (xtable);
    prev = sql;
    if (upd_fields & RTT_COL_NODE_NODE_ID)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, node_id = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s node_id = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (upd_fields & RTT_COL_NODE_CONTAINING_FACE)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, containing_face = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s containing_face = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (upd_fields & RTT_COL_NODE_GEOM)
      {
	  if (accessor->has_z)
	    {
		if (comma)
		    sql =
			sqlite3_mprintf ("%s, geom = MakePointZ(?, ?. ?, %d)",
					 prev, accessor->srid);
		else
		    sql =
			sqlite3_mprintf ("%s geom = MakePointZ(?, ?, ?, %d)",
					 prev, accessor->srid);
	    }
	  else
	    {
		if (comma)
		    sql =
			sqlite3_mprintf ("%s, geom = MakePoint(?, ?, %d)", prev,
					 accessor->srid);
		else
		    sql =
			sqlite3_mprintf ("%s geom = MakePoint(?, ?, %d)", prev,
					 accessor->srid);
	    }
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    sql = sqlite3_mprintf ("%s WHERE node_id = ?", prev);
    sqlite3_free (prev);
    ret =
	sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql), &stmt,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_updateNodesById error: \"%s\"",
				       sqlite3_errmsg (accessor->db_handle));
	  gaiatopo_set_last_error_msg (topo, msg);
	  sqlite3_free (msg);
	  return -1;
      }

    for (i = 0; i < numnodes; i++)
      {
	  /* parameter binding */
	  const RTT_ISO_NODE *nd = nodes + i;
	  icol = 1;
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  if (upd_fields & RTT_COL_NODE_NODE_ID)
	    {
		sqlite3_bind_int64 (stmt, icol, nd->node_id);
		icol++;
	    }
	  if (upd_fields & RTT_COL_NODE_CONTAINING_FACE)
	    {
		if (nd->containing_face < 0)
		    sqlite3_bind_null (stmt, icol);
		else
		    sqlite3_bind_int64 (stmt, icol, nd->containing_face);
		icol++;
	    }
	  if (upd_fields & RTT_COL_NODE_GEOM)
	    {
		RTPOINTARRAY *pa;
		RTPOINT4D pt4d;
		double x;
		double y;
		double z;
		/* extracting X and Y from RTPOINT */
		pa = nd->geom->point;
		rt_getPoint4d_p (ctx, pa, 0, &pt4d);
		x = pt4d.x;
		y = pt4d.y;
		if (accessor->has_z)
		    z = pt4d.z;
		sqlite3_bind_double (stmt, icol, x);
		icol++;
		sqlite3_bind_double (stmt, icol, y);
		icol++;
		if (accessor->has_z)
		  {
		      sqlite3_bind_double (stmt, icol, z);
		      icol++;
		  }
	    }
	  sqlite3_bind_int64 (stmt, icol, nd->node_id);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      changed += sqlite3_changes (accessor->db_handle);
	  else
	    {
		char *msg = sqlite3_mprintf ("callback_updateNodesById: \"%s\"",
					     sqlite3_errmsg
					     (accessor->db_handle));
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    return changed;

  error:
    sqlite3_finalize (stmt);
    return -1;
}

RTT_ISO_FACE *
callback_getFaceWithinBox2D (const RTT_BE_TOPOLOGY * rtt_topo,
			     const RTGBOX * box, int *numelems, int fields,
			     int limit)
{
/* callback function: getFaceWithinBox2D */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    sqlite3_stmt *stmt;
    int ret;
    int count = 0;
    struct topo_faces_list *list = NULL;
    RTT_ISO_FACE *result = NULL;
    if (accessor == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    stmt = accessor->stmt_getFaceWithinBox2D;
    if (stmt == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    cache = (struct splite_internal_cache *) accessor->cache;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, box->xmax);
    sqlite3_bind_double (stmt, 2, box->xmin);
    sqlite3_bind_double (stmt, 3, box->ymax);
    sqlite3_bind_double (stmt, 4, box->ymin);
    list = create_faces_list ();

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 face_id = sqlite3_column_int64 (stmt, 0);
		double minx = sqlite3_column_double (stmt, 1);
		double miny = sqlite3_column_double (stmt, 2);
		double maxx = sqlite3_column_double (stmt, 3);
		double maxy = sqlite3_column_double (stmt, 4);
		add_face (list, face_id, face_id, minx, miny, maxx, maxy);
		count++;
		if (limit > 0)
		  {
		      if (count > limit)
			  break;
		  }
		if (limit < 0)
		    break;
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("callback_getFaceWithinBox2D: %s",
					     sqlite3_errmsg
					     (accessor->db_handle));
		gaiatopo_set_last_error_msg (topo, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    if (limit < 0)
      {
	  result = NULL;
	  *numelems = count;
      }
    else
      {
	  if (list->count <= 0)
	    {
		result = NULL;
		*numelems = 0;
	    }
	  else
	    {
		int i = 0;
		struct topo_face *p_fc;
		result = rtalloc (ctx, sizeof (RTT_ISO_FACE) * list->count);
		p_fc = list->first;
		while (p_fc != NULL)
		  {
		      RTT_ISO_FACE *fc = result + i;
		      if (fields & RTT_COL_FACE_FACE_ID)
			  fc->face_id = p_fc->face_id;
		      if (fields & RTT_COL_FACE_MBR)
			{
			    fc->mbr = gbox_new (ctx, 0);
			    fc->mbr->xmin = p_fc->minx;
			    fc->mbr->ymin = p_fc->miny;
			    fc->mbr->xmax = p_fc->maxx;
			    fc->mbr->ymax = p_fc->maxy;
			}
		      i++;
		      p_fc = p_fc->next;
		  }
		*numelems = list->count;
	    }
      }
    destroy_faces_list (list);
    sqlite3_reset (stmt);
    return result;

  error:
    if (list != NULL)
	destroy_faces_list (list);
    *numelems = -1;
    sqlite3_reset (stmt);
    return NULL;
}

int
callback_updateTopoGeomEdgeSplit (const RTT_BE_TOPOLOGY * topo,
				  RTT_ELEMID split_edge, RTT_ELEMID new_edge1,
				  RTT_ELEMID new_edge2)
{
/* does nothing */
    if (topo != NULL && split_edge == 0 && new_edge1 == 0 && new_edge2 == 0)
	topo = NULL;		/* silencing stupid compiler warnings on unused args */
    return 1;
}

int
callback_updateTopoGeomFaceSplit (const RTT_BE_TOPOLOGY * topo,
				  RTT_ELEMID split_face,
				  RTT_ELEMID new_face1, RTT_ELEMID new_face2)
{
/* does nothing */
    if (topo != NULL && split_face == 0 && new_face1 == 0 && new_face2 == 0)
	topo = NULL;		/* silencing stupid compiler warnings on unused args */
    return 1;
}

int
callback_checkTopoGeomRemEdge (const RTT_BE_TOPOLOGY * topo,
			       RTT_ELEMID rem_edge, RTT_ELEMID face_left,
			       RTT_ELEMID face_right)
{
/* does nothing */
    if (topo != NULL && rem_edge == 0 && face_left == 0 && face_right == 0)
	topo = NULL;		/* silencing stupid compiler warnings on unused args */
    return 1;
}

int
callback_updateTopoGeomFaceHeal (const RTT_BE_TOPOLOGY * topo, RTT_ELEMID face1,
				 RTT_ELEMID face2, RTT_ELEMID newface)
{
/* does nothing */
    if (topo != NULL && face1 == 0 && face2 == 0 && newface == 0)
	topo = NULL;		/* silencing stupid compiler warnings on unused args */
    return 1;
}

int
callback_checkTopoGeomRemNode (const RTT_BE_TOPOLOGY * topo,
			       RTT_ELEMID rem_node, RTT_ELEMID e1,
			       RTT_ELEMID e2)
{
/* does nothing */
    if (topo != NULL && rem_node == 0 && e1 == 0 && e2 == 0)
	topo = NULL;		/* silencing stupid compiler warnings on unused args */
    return 1;
}

int
callback_updateTopoGeomEdgeHeal (const RTT_BE_TOPOLOGY * topo, RTT_ELEMID edge1,
				 RTT_ELEMID edge2, RTT_ELEMID newedge)
{
/* does nothing */
    if (topo != NULL && edge1 == 0 && edge2 == 0 && newedge == 0)
	topo = NULL;		/* silencing stupid compiler warnings on unused args */
    return 1;
}

int
callback_topoGetSRID (const RTT_BE_TOPOLOGY * rtt_topo)
{
/* callback function: topoGetSRID */
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    if (accessor == NULL)
	return -1;

    return accessor->srid;
}

double
callback_topoGetPrecision (const RTT_BE_TOPOLOGY * rtt_topo)
{
/* callback function: topoGetPrecision */
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    if (accessor == NULL)
	return 0.0;

    return accessor->tolerance;
}

int
callback_topoHasZ (const RTT_BE_TOPOLOGY * rtt_topo)
{
/* callback function: topoHasZ */
    GaiaTopologyAccessorPtr topo = (GaiaTopologyAccessorPtr) rtt_topo;
    struct gaia_topology *accessor = (struct gaia_topology *) topo;
    if (accessor == NULL)
	return 0;

    return accessor->has_z;
}

#endif /* end ENABLE_RTTOPO conditionals */
