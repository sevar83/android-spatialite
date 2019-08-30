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
 *
 **********************************************************************/



#ifndef LIBRTGEOM_TOPO_INTERNAL_H
#define LIBRTGEOM_TOPO_INTERNAL_H 1

#include "geos_c.h"

#include "librttopo_geom.h"
#include "librttopo.h"

/************************************************************************
 *
 * Generic SQL handler
 *
 ************************************************************************/

struct RTT_BE_IFACE_T
{
  const RTT_BE_DATA *data;
  const RTT_BE_CALLBACKS *cb;
  const RTCTX *ctx;
};

const char* rtt_be_lastErrorMessage(const RTT_BE_IFACE* be);

RTT_BE_TOPOLOGY * rtt_be_loadTopologyByName(RTT_BE_IFACE *be, const char *name);

int rtt_be_freeTopology(RTT_TOPOLOGY *topo);

RTT_ISO_NODE* rtt_be_getNodeWithinDistance2D(RTT_TOPOLOGY* topo, RTPOINT* pt, double dist, int* numelems, int fields, int limit);

RTT_ISO_NODE* rtt_be_getNodeById(RTT_TOPOLOGY* topo, const RTT_ELEMID* ids, int* numelems, int fields);

int rtt_be_ExistsCoincidentNode(RTT_TOPOLOGY* topo, RTPOINT* pt);
int rtt_be_insertNodes(RTT_TOPOLOGY* topo, RTT_ISO_NODE* node, int numelems);

int rtt_be_ExistsEdgeIntersectingPoint(RTT_TOPOLOGY* topo, RTPOINT* pt);

RTT_ELEMID rtt_be_getNextEdgeId(RTT_TOPOLOGY* topo);
RTT_ISO_EDGE* rtt_be_getEdgeById(RTT_TOPOLOGY* topo, const RTT_ELEMID* ids,
                               int* numelems, int fields);
RTT_ISO_EDGE* rtt_be_getEdgeWithinDistance2D(RTT_TOPOLOGY* topo, RTPOINT* pt,
                               double dist, int* numelems, int fields,
                               int limit);
int
rtt_be_insertEdges(RTT_TOPOLOGY* topo, RTT_ISO_EDGE* edge, int numelems);
int
rtt_be_updateEdges(RTT_TOPOLOGY* topo, const RTT_ISO_EDGE* sel_edge, int sel_fields, const RTT_ISO_EDGE* upd_edge, int upd_fields, const RTT_ISO_EDGE* exc_edge, int exc_fields);
int
rtt_be_deleteEdges(RTT_TOPOLOGY* topo, const RTT_ISO_EDGE* sel_edge, int sel_fields);

RTT_ELEMID rtt_be_getFaceContainingPoint(RTT_TOPOLOGY* topo, RTPOINT* pt);

int rtt_be_updateTopoGeomEdgeSplit(RTT_TOPOLOGY* topo, RTT_ELEMID split_edge, RTT_ELEMID new_edge1, RTT_ELEMID new_edge2);


/************************************************************************
 *
 * Internal objects
 *
 ************************************************************************/

struct RTT_TOPOLOGY_T
{
  const RTT_BE_IFACE *be_iface;
  RTT_BE_TOPOLOGY *be_topo;
  int srid;
  double precision;
  int hasZ;
};

/************************************************************************
 *
 * Backend interaction wrappers
 *
 ************************************************************************/

RTT_ISO_EDGE*
rtt_be_getEdgeWithinBox2D( const RTT_TOPOLOGY* topo,
                           const RTGBOX* box, int* numelems, int fields,
                           int limit );

/************************************************************************
 *
 * Utility functions
 *
 ************************************************************************/

void
rtt_release_edges(const RTCTX *ctx, RTT_ISO_EDGE *edges, int num_edges);

#endif /* LIBRTGEOM_TOPO_INTERNAL_H */
