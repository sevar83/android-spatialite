/*

 virtualrouting.c -- SQLite3 extension [VIRTUAL TABLE Routing - shortest path]

 version 4.4, 2016 October 7

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
 
Portions created by the Initial Developer are Copyright (C) 2016
the Initial Developer. All Rights Reserved.

Contributor(s):
Luigi Costalli luigi.costalli@gmail.com

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

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <ctype.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#ifndef OMIT_GEOS		/* GEOS is supported */

#include <spatialite/sqlite.h>

#include <spatialite.h>
#include <spatialite/spatialite.h>
#include <spatialite/gaiaaux.h>
#include <spatialite/gaiageo.h>

static struct sqlite3_module my_route_module;

#define VROUTE_DIJKSTRA_ALGORITHM	1
#define VROUTE_A_STAR_ALGORITHM	2

#define VROUTE_ROUTING_SOLUTION		0xdd
#define VROUTE_POINT2POINT_SOLUTION	0xcc
#define VROUTE_POINT2POINT_ERROR	0xca
#define VROUTE_RANGE_SOLUTION		0xbb
#define VROUTE_TSP_SOLUTION			0xee

#define VROUTE_SHORTEST_PATH_FULL		0x70
#define VROUTE_SHORTEST_PATH_NO_LINKS	0x71
#define VROUTE_SHORTEST_PATH_NO_GEOMS	0x72
#define VROUTE_SHORTEST_PATH_SIMPLE		0x73
#define VROUTE_SHORTEST_PATH_QUICK		0x74

#define VROUTE_SHORTEST_PATH			0x91
#define VROUTE_TSP_NN					0x92
#define VROUTE_TSP_GA					0x93

#define VROUTE_INVALID_SRID	-1234

#define	VROUTE_TSP_GA_MAX_ITERATIONS	512

#define VROUTE_POINT2POINT_FROM	1
#define VROUTE_POINT2POINT_TO	2

#define VROUTE_POINT2POINT_NONE		0
#define VROUTE_POINT2POINT_INGRESS	1
#define VROUTE_POINT2POINT_START	2
#define VROUTE_POINT2POINT_END		3
#define VROUTE_POINT2POINT_EGRESS	4

#ifdef _WIN32
#define strcasecmp	_stricmp
#endif /* not WIN32 */

/******************************************************************************
/
/ helper structs for destintation candidates
/
******************************************************************************/

typedef struct DestinationCandidateStruct
{
/* a candidate destination */
    char *Code;
    sqlite3_int64 Id;
    char Valid;
    struct DestinationCandidateStruct *Next;
} DestinationCandidate;
typedef DestinationCandidate *DestinationCandidatePtr;

typedef struct DestinationCandidatesListStruct
{
/* a list of candidate destination */
    int NodeCode;
    DestinationCandidatePtr First;
    DestinationCandidatePtr Last;
    int ValidItems;
} DestinationCandidatesList;
typedef DestinationCandidatesList *DestinationCandidatesListPtr;

/******************************************************************************
/
/ virtualrouting structs
/
******************************************************************************/

typedef struct RouteLinkStruct
{
/* a LINK */
    const struct RouteNodeStruct *NodeFrom;
    const struct RouteNodeStruct *NodeTo;
    sqlite3_int64 LinkRowid;
    double Cost;
} RouteLink;
typedef RouteLink *RouteLinkPtr;

typedef struct RouteNodeStruct
{
/* a NODE */
    int InternalIndex;
    sqlite3_int64 Id;
    char *Code;
    double CoordX;
    double CoordY;
    int NumLinks;
    RouteLinkPtr Links;
} RouteNode;
typedef RouteNode *RouteNodePtr;

typedef struct RoutingStruct
{
/* the main NETWORK structure */
    int Net64;
    int AStar;
    int EndianArch;
    int MaxCodeLength;
    int CurrentIndex;
    int NodeCode;
    int NumNodes;
    char *TableName;
    char *FromColumn;
    char *ToColumn;
    char *GeometryColumn;
    char *NameColumn;
    double AStarHeuristicCoeff;
    int HasZ;
    int Srid;
    RouteNodePtr Nodes;
} Routing;
typedef Routing *RoutingPtr;

typedef struct LinkSolutionStruct
{
/* Geometry corresponding to a Link used by Dijkstra shortest path solution */
    sqlite3_int64 LinkRowid;
    char *FromCode;
    char *ToCode;
    sqlite3_int64 FromId;
    sqlite3_int64 ToId;
    int Points;
    double *Coords;
    int HasZ;
    int Srid;
    char *Name;
    struct LinkSolutionStruct *Next;

} LinkSolution;
typedef LinkSolution *LinkSolutionPtr;

typedef struct RowSolutionStruct
{
/* a row into the shortest path solution */
    RouteLinkPtr Link;
    char *Name;
    struct RowSolutionStruct *Next;

} RowSolution;
typedef RowSolution *RowSolutionPtr;

typedef struct RowNodeSolutionStruct
{
/* a row into the "within Cost range" solution */
    int RouteNum;
    int RouteRow;
    RouteNodePtr Node;
    double Cost;
    int Srid;
    struct RowNodeSolutionStruct *Next;

} RowNodeSolution;
typedef RowNodeSolution *RowNodeSolutionPtr;

typedef struct ShortestPathSolutionStruct
{
/* a shortest path solution */
    LinkSolutionPtr FirstLink;
    LinkSolutionPtr LastLink;
    RouteNodePtr From;
    RouteNodePtr To;
    char *Undefined;
    sqlite3_int64 UndefinedId;
    RowSolutionPtr First;
    RowSolutionPtr Last;
    RowNodeSolutionPtr FirstNode;
    RowNodeSolutionPtr LastNode;
    RowNodeSolutionPtr CurrentNodeRow;
    double TotalCost;
    gaiaGeomCollPtr Geometry;
    struct ShortestPathSolutionStruct *Next;
} ShortestPathSolution;
typedef ShortestPathSolution *ShortestPathSolutionPtr;

typedef struct ResultsetRowStruct
{
/* a row into the resultset */
    int RouteNum;
    int RouteRow;
    int Point2PointRole;
    RouteNodePtr From;
    RouteNodePtr To;
    char *Undefined;
    sqlite3_int64 UndefinedId;
    RowSolutionPtr linkRef;
    double TotalCost;
    gaiaGeomCollPtr Geometry;
    struct ResultsetRowStruct *Next;

} ResultsetRow;
typedef ResultsetRow *ResultsetRowPtr;

typedef struct RoutingMultiDestStruct
{
/* helper struct supporting multiple destinations */
    int CodeNode;
    int Items;
    int Next;
    RouteNodePtr *To;
    char *Found;
    sqlite3_int64 *Ids;
    char **Codes;
} RoutingMultiDest;
typedef RoutingMultiDest *RoutingMultiDestPtr;

typedef struct MultiSolutionStruct
{
/* multiple shortest path solutions */
    unsigned char Mode;
    RouteNodePtr From;
    double MaxCost;
    RoutingMultiDestPtr MultiTo;
    ResultsetRowPtr FirstRow;
    ResultsetRowPtr LastRow;
    ResultsetRowPtr CurrentRow;
    ShortestPathSolutionPtr First;
    ShortestPathSolutionPtr Last;
    RowNodeSolutionPtr FirstNode;
    RowNodeSolutionPtr LastNode;
    RowSolutionPtr FirstLink;
    RowSolutionPtr LastLink;
    gaiaGeomCollPtr FirstGeom;
    gaiaGeomCollPtr LastGeom;
    RowNodeSolutionPtr CurrentNodeRow;
    sqlite3_int64 CurrentRowId;
    int RouteNum;
} MultiSolution;
typedef MultiSolution *MultiSolutionPtr;

typedef struct Point2PointCandidateStruct
{
/* wrapping a Point2Point candidate */
    sqlite3_int64 linkRowid;
    char *codNodeFrom;
    char *codNodeTo;
    sqlite3_int64 idNodeFrom;
    sqlite3_int64 idNodeTo;
    int reverse;
    int valid;
    gaiaGeomCollPtr path;
    double pathLen;
    double extraLen;
    double percent;
    struct Point2PointCandidateStruct *next;
} Point2PointCandidate;
typedef Point2PointCandidate *Point2PointCandidatePtr;

typedef struct Point2PointNodeStruct
{
/* wrapping a Point2Point Node */
    char *codNode;
    sqlite3_int64 idNode;
    Point2PointCandidatePtr parent;
    struct Point2PointNodeStruct *next;
} Point2PointNode;
typedef Point2PointNode *Point2PointNodePtr;

typedef struct Point2PointSolutionStruct
{
/* shortest path solution - Point2Point */
    unsigned char Mode;
    int validFrom;
    double xFrom;
    double yFrom;
    double zFrom;
    int validTo;
    double xTo;
    double yTo;
    double zTo;
    int srid;
    Point2PointCandidatePtr firstFromCandidate;
    Point2PointCandidatePtr lastFromCandidate;
    Point2PointCandidatePtr firstToCandidate;
    Point2PointCandidatePtr lastToCandidate;
    Point2PointNodePtr firstFromNode;
    Point2PointNodePtr lastFromNode;
    Point2PointNodePtr firstToNode;
    Point2PointNodePtr lastToNode;
    double totalCost;
    Point2PointCandidatePtr fromCandidate;
    Point2PointCandidatePtr toCandidate;
    gaiaDynamicLinePtr dynLine;
    int hasZ;
    ResultsetRowPtr FirstRow;
    ResultsetRowPtr LastRow;
    ResultsetRowPtr CurrentRow;
    sqlite3_int64 CurrentRowId;
} Point2PointSolution;
typedef Point2PointSolution *Point2PointSolutionPtr;

/******************************************************************************
/
/ TSP structs
/
******************************************************************************/

typedef struct TspTargetsStruct
{
/* TSP helper struct */
    unsigned char Mode;
    double TotalCost;
    RouteNodePtr From;
    int Count;
    RouteNodePtr *To;
    char *Found;
    double *Costs;
    ShortestPathSolutionPtr *Solutions;
    ShortestPathSolutionPtr LastSolution;
} TspTargets;
typedef TspTargets *TspTargetsPtr;

typedef struct TspGaSubDistanceStruct
{
/* a cache sub-item for storing TSP GA costs */
    RouteNodePtr CityTo;
    double Cost;
} TspGaSubDistance;
typedef TspGaSubDistance *TspGaSubDistancePtr;

typedef struct TspGaDistanceStruct
{
/* a cache item for storing TSP GA costs */
    RouteNodePtr CityFrom;
    int Cities;
    TspGaSubDistancePtr *Distances;
    int NearestIndex;
} TspGaDistance;
typedef TspGaDistance *TspGaDistancePtr;

typedef struct TspGaSolutionStruct
{
/* TSP GA solution struct */
    int Cities;
    RouteNodePtr *CitiesFrom;
    RouteNodePtr *CitiesTo;
    double *Costs;
    double TotalCost;
} TspGaSolution;
typedef TspGaSolution *TspGaSolutionPtr;

typedef struct TspGaPopulationStruct
{
/* TSP GA helper struct */
    int Count;
    int Cities;
    TspGaSolutionPtr *Solutions;
    TspGaSolutionPtr *Offsprings;
    TspGaDistancePtr *Distances;
    char *RandomSolutionsSql;
    char *RandomIntervalSql;
} TspGaPopulation;
typedef TspGaPopulation *TspGaPopulationPtr;

/******************************************************************************
/
/ Dijkstra and A* common structs
/
******************************************************************************/

typedef struct RoutingNode
{
    int Id;
    struct RoutingNode **To;
    RouteLinkPtr *Link;
    int DimTo;
    struct RoutingNode *PreviousNode;
    RouteNodePtr Node;
    RouteLinkPtr xLink;
    double Distance;
    double HeuristicDistance;
    int Inspected;
} RoutingNode;
typedef RoutingNode *RoutingNodePtr;

typedef struct RoutingNodes
{
    RoutingNodePtr Nodes;
    RouteLinkPtr *LinksBuffer;
    RoutingNodePtr *NodesBuffer;
    int Dim;
    int DimLink;
} RoutingNodes;
typedef RoutingNodes *RoutingNodesPtr;

typedef struct HeapNode
{
    RoutingNodePtr Node;
    double Distance;
} HeapNode;
typedef HeapNode *HeapNodePtr;

typedef struct RoutingHeapStruct
{
    HeapNodePtr Nodes;
    int Count;
} RoutingHeap;
typedef RoutingHeap *RoutingHeapPtr;

/******************************************************************************
/
/ VirtualTable structs
/
******************************************************************************/

typedef struct virtualroutingStruct
{
/* extends the sqlite3_vtab struct */
    const sqlite3_module *pModule;	/* ptr to sqlite module: USED INTERNALLY BY SQLITE */
    int nRef;			/* # references: USED INTERNALLY BY SQLITE */
    char *zErrMsg;		/* error message: USE INTERNALLY BY SQLITE */
    sqlite3 *db;		/* the sqlite db holding the virtual table */
    RoutingPtr graph;		/* the NETWORK structure */
    RoutingNodesPtr routing;	/* the ROUTING structure */
    int currentAlgorithm;	/* the currently selected Shortest Path Algorithm */
    int currentRequest;		/* the currently selected Shortest Path Request */
    int currentOptions;		/* the currently selected Shortest Path Options */
    char currentDelimiter;	/* the currently set delimiter char */
    double Tolerance;		/* the currently set Tolerance value [Point2Point] */
    MultiSolutionPtr multiSolution;	/* the current multiple solution */
    Point2PointSolutionPtr point2PointSolution;	/* the current Point2Point solution */
    int eof;			/* the EOF marker */
} virtualrouting;
typedef virtualrouting *virtualroutingPtr;

typedef struct virtualroutingCursortStruct
{
/* extends the sqlite3_vtab_cursor struct */
    virtualroutingPtr pVtab;	/* Virtual table of this cursor */
} virtualroutingCursor;
typedef virtualroutingCursor *virtualroutingCursorPtr;

/*
/
/  implementation of the Dijkstra Shortest Path algorithm
/
////////////////////////////////////////////////////////////
/
/ Author: Luigi Costalli luigi.costalli@gmail.com
/ version 1.0. 2008 October 21
/
*/

static RoutingNodesPtr
routing_init (RoutingPtr graph)
{
/* allocating and initializing the ROUTING struct */
    int i;
    int j;
    int cnt = 0;
    RoutingNodesPtr nd;
    RoutingNodePtr ndn;
    RouteNodePtr nn;
/* allocating the main Nodes struct */
    nd = malloc (sizeof (RoutingNodes));
/* allocating and initializing  Nodes array */
    nd->Nodes = malloc (sizeof (RoutingNode) * graph->NumNodes);
    nd->Dim = graph->NumNodes;
    nd->DimLink = 0;
/* pre-alloc buffer strategy - GENSCHER 2010-01-05 */
    for (i = 0; i < graph->NumNodes; cnt += graph->Nodes[i].NumLinks, i++);
    nd->NodesBuffer = malloc (sizeof (RoutingNodePtr) * cnt);
    nd->LinksBuffer = malloc (sizeof (RouteLinkPtr) * cnt);

    cnt = 0;
    for (i = 0; i < graph->NumNodes; i++)
      {
	  /* initializing the Nodes array */
	  nn = graph->Nodes + i;
	  ndn = nd->Nodes + i;
	  ndn->Id = nn->InternalIndex;
	  ndn->DimTo = nn->NumLinks;
	  ndn->Node = nn;
	  ndn->To = &(nd->NodesBuffer[cnt]);
	  ndn->Link = &(nd->LinksBuffer[cnt]);
	  cnt += nn->NumLinks;

	  for (j = 0; j < nn->NumLinks; j++)
	    {
		/*  setting the outcoming Links for the current Node */
		nd->DimLink++;
		ndn->To[j] = nd->Nodes + nn->Links[j].NodeTo->InternalIndex;
		ndn->Link[j] = nn->Links + j;
	    }
      }
    return (nd);
}

static void
routing_free (RoutingNodes * e)
{
/* memory cleanup; freeing the ROUTING struct */
    free (e->LinksBuffer);
    free (e->NodesBuffer);
    free (e->Nodes);
    free (e);
}

static RoutingHeapPtr
routing_heap_init (int n)
{
/* allocating and initializing the Heap (min-priority queue) */
    RoutingHeapPtr heap = malloc (sizeof (RoutingHeap));
    heap->Count = 0;
    heap->Nodes = malloc (sizeof (HeapNode) * (n + 1));
    return heap;
}

static void
routing_heap_reset (RoutingHeapPtr heap)
{
/* resetting the Heap (min-priority queue) */
    if (heap == NULL)
	return;
    heap->Count = 0;
}

static void
routing_heap_free (RoutingHeapPtr heap)
{
/* freeing the Heap (min-priority queue) */
    if (heap->Nodes != NULL)
	free (heap->Nodes);
    free (heap);
}

static void
dijkstra_insert (RoutingNodePtr node, HeapNodePtr heap, int size)
{
/* inserting a new Node and rearranging the heap */
    int i;
    HeapNode tmp;
    i = size + 1;
    heap[i].Node = node;
    heap[i].Distance = node->Distance;
    if (i / 2 < 1)
	return;
    while (heap[i].Distance < heap[i / 2].Distance)
      {
	  tmp = heap[i];
	  heap[i] = heap[i / 2];
	  heap[i / 2] = tmp;
	  i /= 2;
	  if (i / 2 < 1)
	      break;
      }
}

static void
dijkstra_enqueue (RoutingHeapPtr heap, RoutingNodePtr node)
{
/* enqueuing a Node into the heap */
    dijkstra_insert (node, heap->Nodes, heap->Count);
    heap->Count += 1;
}

static void
dijkstra_shiftdown (HeapNodePtr heap, int size, int i)
{
/* rearranging the heap after removing */
    int c;
    HeapNode tmp;
    for (;;)
      {
	  c = i * 2;
	  if (c > size)
	      break;
	  if (c < size)
	    {
		if (heap[c].Distance > heap[c + 1].Distance)
		    ++c;
	    }
	  if (heap[c].Distance < heap[i].Distance)
	    {
		/* swapping two Nodes */
		tmp = heap[c];
		heap[c] = heap[i];
		heap[i] = tmp;
		i = c;
	    }
	  else
	      break;
      }
}

static RoutingNodePtr
dijkstra_remove_min (HeapNodePtr heap, int size)
{
/* removing the min-priority Node from the heap */
    RoutingNodePtr node = heap[1].Node;
    heap[1] = heap[size];
    --size;
    dijkstra_shiftdown (heap, size, 1);
    return node;
}

static RoutingNodePtr
routing_dequeue (RoutingHeapPtr heap)
{
/* dequeuing a Node from the heap */
    RoutingNodePtr node = dijkstra_remove_min (heap->Nodes, heap->Count);
    heap->Count -= 1;
    return node;
}

/* END of Luigi Costalli Dijkstra Shortest Path implementation */

static void
delete_solution (ShortestPathSolutionPtr solution)
{
/* deleting the current solution */
    LinkSolutionPtr pA;
    LinkSolutionPtr pAn;
    RowSolutionPtr pR;
    RowSolutionPtr pRn;
    RowNodeSolutionPtr pN;
    RowNodeSolutionPtr pNn;
    if (!solution)
	return;
    pA = solution->FirstLink;
    while (pA)
      {
	  pAn = pA->Next;
	  if (pA->FromCode)
	      free (pA->FromCode);
	  if (pA->ToCode)
	      free (pA->ToCode);
	  if (pA->Coords)
	      free (pA->Coords);
	  if (pA->Name)
	      free (pA->Name);
	  free (pA);
	  pA = pAn;
      }
    pR = solution->First;
    while (pR)
      {
	  pRn = pR->Next;
	  if (pR->Name)
	      free (pR->Name);
	  free (pR);
	  pR = pRn;
      }
    pN = solution->FirstNode;
    while (pN)
      {
	  pNn = pN->Next;
	  free (pN);
	  pN = pNn;
      }
    if (solution->Undefined != NULL)
	free (solution->Undefined);
    if (solution->Geometry)
	gaiaFreeGeomColl (solution->Geometry);
    free (solution);
}

static ShortestPathSolutionPtr
alloc_solution (void)
{
/* allocates and initializes the current solution */
    ShortestPathSolutionPtr p = malloc (sizeof (ShortestPathSolution));
    p->FirstLink = NULL;
    p->LastLink = NULL;
    p->From = NULL;
    p->To = NULL;
    p->Undefined = NULL;
    p->UndefinedId = 0;
    p->First = NULL;
    p->Last = NULL;
    p->FirstNode = NULL;
    p->LastNode = NULL;
    p->CurrentNodeRow = NULL;
    p->TotalCost = 0.0;
    p->Geometry = NULL;
    p->Next = NULL;
    return p;
}

static void
add_link_to_solution (ShortestPathSolutionPtr solution, RouteLinkPtr link)
{
/* inserts a Link into the Shortest Path solution */
    RowSolutionPtr p = malloc (sizeof (RowSolution));
    p->Link = link;
    p->Name = NULL;
    p->Next = NULL;
    solution->TotalCost += link->Cost;
    if (!(solution->First))
	solution->First = p;
    if (solution->Last)
	solution->Last->Next = p;
    solution->Last = p;
}

static void
add_node_to_solution (MultiSolutionPtr multiSolution, RoutingNodePtr node,
		      int srid, int index)
{
/* inserts a Node into the "within Cost range" solution */
    RowNodeSolutionPtr p = malloc (sizeof (RowNodeSolution));
    p->RouteNum = 0;
    p->RouteRow = index;
    p->Node = node->Node;
    p->Cost = node->Distance;
    p->Srid = srid;
    p->Next = NULL;
    if (!(multiSolution->FirstNode))
	multiSolution->FirstNode = p;
    if (multiSolution->LastNode)
	multiSolution->LastNode->Next = p;
    multiSolution->LastNode = p;
}

static void
set_link_name_into_solution (ShortestPathSolutionPtr solution,
			     sqlite3_int64 link_id, const char *name)
{
/* sets the Name identifyin a Link into the Solution */
    RowSolutionPtr row = solution->First;
    while (row != NULL)
      {
	  if (row->Link->LinkRowid == link_id)
	    {
		int len = strlen (name);
		if (row->Name != NULL)
		    free (row->Name);
		row->Name = malloc (len + 1);
		strcpy (row->Name, name);
		return;
	    }
	  row = row->Next;
      }
}

static void
add_link_geometry_to_solution (ShortestPathSolutionPtr solution,
			       sqlite3_int64 link_id, const char *from_code,
			       const char *to_code, sqlite3_int64 from_id,
			       sqlite3_int64 to_id, gaiaGeomCollPtr geom,
			       const char *name)
{
/* inserts a Link Geometry into the Shortest Path solution */
    int iv;
    int points;
    double *coords;
    int has_z;
    int len;
    LinkSolutionPtr p = malloc (sizeof (LinkSolution));
    p->LinkRowid = link_id;
    p->FromCode = NULL;
    len = strlen (from_code);
    if (len > 0)
      {
	  p->FromCode = malloc (len + 1);
	  strcpy (p->FromCode, from_code);
      }
    p->ToCode = NULL;
    len = strlen (to_code);
    if (len > 0)
      {
	  p->ToCode = malloc (len + 1);
	  strcpy (p->ToCode, to_code);
      }
    p->FromId = from_id;
    p->ToId = to_id;

/* preparing the Link's Geometry */
    points = geom->FirstLinestring->Points;
    if (geom->DimensionModel == GAIA_XY_Z
	|| geom->DimensionModel == GAIA_XY_Z_M)
      {
	  has_z = 1;
	  coords = malloc (sizeof (double) * (points * 3));
      }
    else
      {
	  has_z = 0;
	  coords = malloc (sizeof (double) * (points * 2));
      }
    for (iv = 0; iv < points; iv++)
      {
	  double x;
	  double y;
	  double z;
	  double m;
	  if (geom->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (geom->FirstLinestring->Coords, iv, &x, &y, &z);
	    }
	  else if (geom->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (geom->FirstLinestring->Coords, iv, &x, &y, &m);
	    }
	  else if (geom->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (geom->FirstLinestring->Coords, iv, &x, &y, &z,
				  &m);
	    }
	  else
	    {
		gaiaGetPoint (geom->FirstLinestring->Coords, iv, &x, &y);
	    }
	  if (has_z)
	    {
		*(coords + ((iv * 3) + 0)) = x;
		*(coords + ((iv * 3) + 1)) = y;
		*(coords + ((iv * 3) + 2)) = z;
	    }
	  else
	    {
		*(coords + ((iv * 2) + 0)) = x;
		*(coords + ((iv * 2) + 1)) = y;
	    }
      }
    p->Points = points;
    p->Coords = coords;
    p->HasZ = has_z;
    p->Srid = geom->Srid;
    if (name == NULL)
	p->Name = NULL;
    else
      {
	  len = strlen (name);
	  p->Name = malloc (len + 1);
	  strcpy (p->Name, name);
      }
    p->Next = NULL;
    if (!(solution->FirstLink))
	solution->FirstLink = p;
    if (solution->LastLink)
	solution->LastLink->Next = p;
    solution->LastLink = p;
}

static void
normalizeCoords (LinkSolutionPtr pA)
{
/* inverting the Link's direction */
    int iv;
    int pt = 0;
    double x;
    double y;
    double z;
    double *coords;

    if (pA->HasZ)
	coords = malloc (sizeof (double) * (pA->Points * 3));
    else
	coords = malloc (sizeof (double) * (pA->Points * 2));
/* copying Link's vertices in reverse order */
    for (iv = pA->Points - 1; iv >= 0; iv--)
      {
	  if (pA->HasZ)
	    {
		x = *(pA->Coords + ((iv * 3) + 0));
		y = *(pA->Coords + ((iv * 3) + 1));
		z = *(pA->Coords + ((iv * 3) + 2));
		*(coords + ((pt * 3) + 0)) = x;
		*(coords + ((pt * 3) + 1)) = y;
		*(coords + ((pt * 3) + 2)) = z;
		pt++;
	    }
	  else
	    {
		x = *(pA->Coords + ((iv * 2) + 0));
		y = *(pA->Coords + ((iv * 2) + 1));
		*(coords + ((pt * 2) + 0)) = x;
		*(coords + ((pt * 2) + 1)) = y;
		pt++;
	    }
      }
    free (pA->Coords);
    pA->Coords = coords;
}

static void
addLinkPoints2DynLine (gaiaDynamicLinePtr dyn, LinkSolutionPtr pA, double cost)
{
/* adding Link's points to the final solution Geometry */
    int iv;
    double x;
    double y;
    double z = 0.0;
    double m = 0.0;
    double base_m = 0.0;
    gaiaGeomCollPtr geom;
    gaiaGeomCollPtr geo2;
    gaiaLinestringPtr ln;

    if (pA->HasZ)
	geom = gaiaAllocGeomCollXYZ ();
    else
	geom = gaiaAllocGeomColl ();
    ln = gaiaAddLinestringToGeomColl (geom, pA->Points);

    for (iv = 0; iv < pA->Points; iv++)
      {
	  if (pA->HasZ)
	    {
		x = *(pA->Coords + ((iv * 3) + 0));
		y = *(pA->Coords + ((iv * 3) + 1));
		z = *(pA->Coords + ((iv * 3) + 2));
		gaiaSetPointXYZ (ln->Coords, iv, x, y, z);
	    }
	  else
	    {
		x = *(pA->Coords + ((iv * 2) + 0));
		y = *(pA->Coords + ((iv * 2) + 1));
		gaiaSetPoint (ln->Coords, iv, x, y);
	    }
      }
    geo2 = gaiaAddMeasure (geom, 0.0, cost);
    ln = geo2->FirstLinestring;
    gaiaFreeGeomColl (geom);
    if (dyn->Last != NULL)
	base_m = dyn->Last->M;

    for (iv = 0; iv < ln->Points; iv++)
      {
	  z = 0.0;
	  if (pA->HasZ)
	    {
		gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
	    }
	  if (dyn->Last != NULL)
	    {
		double prev_x = dyn->Last->X;
		double prev_y = dyn->Last->Y;
		if (x == prev_x && y == prev_y)
		    continue;	/* skipping repeated points */
	    }
	  gaiaAppendPointZMToDynamicLine (dyn, x, y, z, base_m + m);
      }
    gaiaFreeGeomColl (geo2);
}

static double
findLinkCost (sqlite3_int64 linkRowid, ShortestPathSolutionPtr solution)
{
/* searching for the Cost of the given Link */
    RowSolutionPtr row = solution->First;
    while (row != NULL)
      {
	  if (row->Link != NULL)
	    {
		if (row->Link->LinkRowid == linkRowid)
		    return row->Link->Cost;
	    }
	  row = row->Next;
      }
    return -1.5;
}

static void
build_solution (sqlite3 * handle, int options, RoutingPtr graph,
		ShortestPathSolutionPtr solution,
		RouteLinkPtr * shortest_path, int cnt)
{
/* formatting the Shortest Path solution */
    int i;
    char *sql;
    int err;
    int error = 0;
    int ret;
    sqlite3_int64 link_id;
    const unsigned char *blob;
    int size;
    sqlite3_int64 from_id;
    sqlite3_int64 to_id;
    char *from_code;
    char *to_code;
    char *name;
    int tbd;
    int ind;
    int base = 0;
    int block = 128;
    int how_many;
    sqlite3_stmt *stmt = NULL;
    char *xfrom;
    char *xto;
    char *xgeom;
    char *xname;
    char *xtable;
    gaiaOutBuffer sql_statement;
    if (options == VROUTE_SHORTEST_PATH_SIMPLE)
      {
	  /* building the solution */
	  for (i = 0; i < cnt; i++)
	      solution->TotalCost += shortest_path[i]->Cost;
      }
    else if (options == VROUTE_SHORTEST_PATH_QUICK)
      {
	  /* only for internal usage: Point2Point */
	  for (i = 0; i < cnt; i++)
	      add_link_to_solution (solution, shortest_path[i]);
	  if (shortest_path)
	      free (shortest_path);
	  return;
      }
    else
      {
	  /* building the solution */
	  for (i = 0; i < cnt; i++)
	      add_link_to_solution (solution, shortest_path[i]);
      }
    if (options == VROUTE_SHORTEST_PATH_SIMPLE)
      {
	  if (shortest_path)
	      free (shortest_path);
	  return;
      }
    if (graph->GeometryColumn == NULL && graph->NameColumn == NULL)
      {
	  if (shortest_path)
	      free (shortest_path);
	  return;
      }
    if (options == VROUTE_SHORTEST_PATH_NO_GEOMS && graph->NameColumn == NULL)
      {
	  if (shortest_path)
	      free (shortest_path);
	  return;
      }

    tbd = cnt;
    while (tbd > 0)
      {
	  /* requesting max 128 links at each time */
	  if (tbd < block)
	      how_many = tbd;
	  else
	      how_many = block;
/* preparing the Geometry representing this solution [reading links] */
	  gaiaOutBufferInitialize (&sql_statement);
	  if (graph->NameColumn)
	    {
		/* a Name column is defined */
		if (graph->GeometryColumn == NULL
		    || options == VROUTE_SHORTEST_PATH_NO_GEOMS)
		  {
		      xfrom = gaiaDoubleQuotedSql (graph->FromColumn);
		      xto = gaiaDoubleQuotedSql (graph->ToColumn);
		      xname = gaiaDoubleQuotedSql (graph->NameColumn);
		      xtable = gaiaDoubleQuotedSql (graph->TableName);
		      sql =
			  sqlite3_mprintf
			  ("SELECT ROWID, \"%s\", \"%s\", NULL, \"%s\" FROM \"%s\" WHERE ROWID IN (",
			   xfrom, xto, xname, xtable);
		      free (xfrom);
		      free (xto);
		      free (xname);
		      free (xtable);
		  }
		else
		  {
		      xfrom = gaiaDoubleQuotedSql (graph->FromColumn);
		      xto = gaiaDoubleQuotedSql (graph->ToColumn);
		      xgeom = gaiaDoubleQuotedSql (graph->GeometryColumn);
		      xname = gaiaDoubleQuotedSql (graph->NameColumn);
		      xtable = gaiaDoubleQuotedSql (graph->TableName);
		      sql =
			  sqlite3_mprintf
			  ("SELECT ROWID, \"%s\", \"%s\", \"%s\", \"%s\" FROM \"%s\" WHERE ROWID IN (",
			   xfrom, xto, xgeom, xname, xtable);
		      free (xfrom);
		      free (xto);
		      free (xgeom);
		      free (xname);
		      free (xtable);
		  }
		gaiaAppendToOutBuffer (&sql_statement, sql);
		sqlite3_free (sql);
	    }
	  else
	    {
		/* no Name column is defined */
		xfrom = gaiaDoubleQuotedSql (graph->FromColumn);
		xto = gaiaDoubleQuotedSql (graph->ToColumn);
		xgeom = gaiaDoubleQuotedSql (graph->GeometryColumn);
		xtable = gaiaDoubleQuotedSql (graph->TableName);
		sql =
		    sqlite3_mprintf
		    ("SELECT ROWID, \"%s\", \"%s\", \"%s\" FROM \"%s\" WHERE ROWID IN (",
		     xfrom, xto, xgeom, xtable);
		free (xfrom);
		free (xto);
		free (xgeom);
		free (xtable);
		gaiaAppendToOutBuffer (&sql_statement, sql);
		sqlite3_free (sql);
	    }
	  for (i = 0; i < how_many; i++)
	    {
		if (i == 0)
		    gaiaAppendToOutBuffer (&sql_statement, "?");
		else
		    gaiaAppendToOutBuffer (&sql_statement, ",?");
	    }
	  gaiaAppendToOutBuffer (&sql_statement, ")");
	  if (sql_statement.Error == 0 && sql_statement.Buffer != NULL)
	    {
		ret =
		    sqlite3_prepare_v2 (handle, sql_statement.Buffer,
					strlen (sql_statement.Buffer), &stmt,
					NULL);
	    }
	  else
	      ret = SQLITE_ERROR;
	  gaiaOutBufferReset (&sql_statement);
	  if (ret != SQLITE_OK)
	    {
		error = 1;
		goto abort;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  ind = 1;
	  for (i = 0; i < cnt; i++)
	    {
		if (i < base)
		    continue;
		if (i >= (base + how_many))
		    break;
		sqlite3_bind_int64 (stmt, ind, shortest_path[i]->LinkRowid);
		ind++;
	    }
	  while (1)
	    {
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		    break;
		if (ret == SQLITE_ROW)
		  {
		      link_id = -1;
		      from_id = -1;
		      to_id = -1;
		      from_code = NULL;
		      to_code = NULL;
		      blob = NULL;
		      size = 0;
		      name = NULL;
		      err = 0;
		      if (sqlite3_column_type (stmt, 0) == SQLITE_INTEGER)
			  link_id = sqlite3_column_int64 (stmt, 0);
		      else
			  err = 1;
		      if (graph->NodeCode)
			{
			    /* nodes are identified by TEXT codes */
			    if (sqlite3_column_type (stmt, 1) == SQLITE_TEXT)
				from_code =
				    (char *) sqlite3_column_text (stmt, 1);
			    else
				err = 1;
			    if (sqlite3_column_type (stmt, 2) == SQLITE_TEXT)
				to_code =
				    (char *) sqlite3_column_text (stmt, 2);
			    else
				err = 1;
			}
		      else
			{
			    /* nodes are identified by INTEGER ids */
			    if (sqlite3_column_type (stmt, 1) == SQLITE_INTEGER)
				from_id = sqlite3_column_int64 (stmt, 1);
			    else
				err = 1;
			    if (sqlite3_column_type (stmt, 2) == SQLITE_INTEGER)
				to_id = sqlite3_column_int64 (stmt, 2);
			    else
				err = 1;
			}
		      if (graph->GeometryColumn != NULL
			  && options != VROUTE_SHORTEST_PATH_NO_GEOMS)
			{
			    if (sqlite3_column_type (stmt, 3) == SQLITE_BLOB)
			      {
				  blob =
				      (const unsigned char *)
				      sqlite3_column_blob (stmt, 3);
				  size = sqlite3_column_bytes (stmt, 3);
			      }
			    else
				err = 1;
			}
		      if (graph->NameColumn)
			{
			    if (sqlite3_column_type (stmt, 4) == SQLITE_TEXT)
				name = (char *) sqlite3_column_text (stmt, 4);
			}
		      if (err)
			  error = 1;
		      else if (graph->GeometryColumn != NULL
			       && options != VROUTE_SHORTEST_PATH_NO_GEOMS)
			{
			    /* saving the Link's geometry into the temporary struct */
			    gaiaGeomCollPtr geom =
				gaiaFromSpatiaLiteBlobWkb (blob, size);
			    if (geom)
			      {
				  /* OK, we have fetched a valid Geometry */
				  if (geom->FirstPoint == NULL
				      && geom->FirstPolygon == NULL
				      && geom->FirstLinestring != NULL
				      && geom->FirstLinestring ==
				      geom->LastLinestring)
				    {
					/* Geometry is LINESTRING as expected */

					if (from_code == NULL)
					    from_code = "";
					if (to_code == NULL)
					    to_code = "";
					add_link_geometry_to_solution (solution,
								       link_id,
								       from_code,
								       to_code,
								       from_id,
								       to_id,
								       geom,
								       name);
				    }
				  else
				      error = 1;
				  gaiaFreeGeomColl (geom);
			      }
			    else
				error = 1;
			}
		      else if (name != NULL)
			  set_link_name_into_solution (solution, link_id, name);
		  }
	    }
	  sqlite3_finalize (stmt);
	  tbd -= how_many;
	  base += how_many;
      }
  abort:
    if (shortest_path)
	free (shortest_path);
    if (!error && graph->GeometryColumn != NULL
	&& options != VROUTE_SHORTEST_PATH_NO_GEOMS)
      {
	  /* building the Geometry representing the Shortest Path Solution */
	  gaiaLinestringPtr ln;
	  int iv = 0;
	  int tot_pts = 0;
	  RowSolutionPtr pR;
	  LinkSolutionPtr pA;
	  int srid = -1;
	  int has_z = 0;
	  gaiaGeomCollPtr geom;
	  gaiaPointPtr pt;
	  gaiaDynamicLinePtr dyn = gaiaAllocDynamicLine ();
	  if (solution->FirstLink)
	    {
		srid = solution->FirstLink->Srid;
		has_z = solution->FirstLink->HasZ;
	    }
	  pR = solution->First;
	  while (pR)
	    {
		/* building the Solution as a Dynamic Line */
		pA = solution->FirstLink;
		while (pA)
		  {
		      if (pR->Link->LinkRowid == pA->LinkRowid)
			{
			    /* copying vertices from corresponding Link's Geometry */
			    int reverse;
			    double cost;
			    if (graph->NodeCode)
			      {
				  /* nodes are identified by TEXT codes */
				  if (strcmp
				      (pR->Link->NodeFrom->Code,
				       pA->ToCode) == 0)
				      reverse = 1;
				  else
				      reverse = 0;
			      }
			    else
			      {
				  /* nodes are identified by INTEGER ids */
				  if (pR->Link->NodeFrom->Id == pA->ToId)
				      reverse = 1;
				  else
				      reverse = 0;
			      }
			    if (reverse)
				normalizeCoords (pA);
			    cost = findLinkCost (pA->LinkRowid, solution);
			    addLinkPoints2DynLine (dyn, pA, cost);
			    if (pA->Name)
			      {
				  int len = strlen (pA->Name);
				  pR->Name = malloc (len + 1);
				  strcpy (pR->Name, pA->Name);
			      }
			    break;
			}
		      pA = pA->Next;
		  }
		pR = pR->Next;
	    }
	  /* building the result path Geometry */
	  if (has_z)
	      geom = gaiaAllocGeomCollXYZM ();
	  else
	      geom = gaiaAllocGeomCollXYM ();
	  geom->Srid = srid;
	  pt = dyn->First;
	  while (pt != NULL)
	    {
		/* counting how many points are there */
		tot_pts++;
		pt = pt->Next;
	    }
	  ln = gaiaAddLinestringToGeomColl (geom, tot_pts);
	  iv = 0;
	  pt = dyn->First;
	  while (pt != NULL)
	    {
		/* copying points from Dynamic Line to Linestring */
		if (has_z)
		  {
		      gaiaSetPointXYZM (ln->Coords, iv, pt->X, pt->Y, pt->Z,
					pt->M);
		  }
		else
		  {
		      gaiaSetPointXYM (ln->Coords, iv, pt->X, pt->Y, pt->M);
		  }
		iv++;
		pt = pt->Next;
	    }
	  solution->Geometry = geom;
	  gaiaFreeDynamicLine (dyn);
      }
    else
      {
	  RowSolutionPtr pR;
	  LinkSolutionPtr pA;
	  if (graph->NameColumn != NULL)
	    {
		pR = solution->First;
		while (pR)
		  {
		      pA = solution->FirstLink;
		      while (pA)
			{
			    if (pA->Name)
			      {
				  int len = strlen (pA->Name);
				  pR->Name = malloc (len + 1);
				  strcpy (pR->Name, pA->Name);
			      }
			    pA = pA->Next;
			}
		      pR = pR->Next;
		  }
	    }
      }

    if (options == VROUTE_SHORTEST_PATH_NO_LINKS)
      {
	  /* deleting Links */
	  RowSolutionPtr pR;
	  RowSolutionPtr pRn;
	  pR = solution->First;
	  while (pR)
	    {
		pRn = pR->Next;
		if (pR->Name)
		    free (pR->Name);
		free (pR);
		pR = pRn;
	    }
	  solution->First = NULL;
	  solution->Last = NULL;
      }
}

static void
build_multi_solution (MultiSolutionPtr multiSolution)
{
/* formatting the Shortest Path resultset */
    ShortestPathSolutionPtr pS;
    int route_num = 0;
    pS = multiSolution->First;
    while (pS != NULL)
      {
	  /* inserting the Route Header */
	  int route_row = 0;
	  RowSolutionPtr pA;
	  ResultsetRowPtr row = malloc (sizeof (ResultsetRow));
	  route_num = multiSolution->RouteNum++;
	  row->RouteNum = route_num;
	  row->RouteRow = route_row++;
	  row->Point2PointRole = VROUTE_POINT2POINT_NONE;
	  row->From = pS->From;
	  row->To = pS->To;
	  row->Undefined = pS->Undefined;
	  pS->Undefined = NULL;
	  row->UndefinedId = pS->UndefinedId;
	  row->linkRef = NULL;
	  row->TotalCost = pS->TotalCost;
	  row->Geometry = pS->Geometry;
	  row->Next = NULL;
	  if (multiSolution->FirstRow == NULL)
	      multiSolution->FirstRow = row;
	  if (multiSolution->LastRow != NULL)
	      multiSolution->LastRow->Next = row;
	  multiSolution->LastRow = row;

	  pA = pS->First;
	  while (pA != NULL)
	    {
		/* inserting Route's traversed Links */
		row = malloc (sizeof (ResultsetRow));
		row->RouteNum = route_num;
		row->RouteRow = route_row++;
		row->Point2PointRole = VROUTE_POINT2POINT_NONE;
		row->From = NULL;
		row->To = NULL;
		row->Undefined = NULL;
		row->linkRef = pA;
		row->TotalCost = 0.0;
		row->Geometry = NULL;
		row->Next = NULL;
		if (multiSolution->FirstRow == NULL)
		    multiSolution->FirstRow = row;
		if (multiSolution->LastRow != NULL)
		    multiSolution->LastRow->Next = row;
		multiSolution->LastRow = row;
		pA = pA->Next;
	    }
	  pS = pS->Next;
      }
}

static int
addPoint2DynLine (const double *coords, int dimensionModel, int iv,
		  gaiaDynamicLinePtr dyn, double base_m)
{
/* adding a Point from Linestring to Dynamic Line */
    double x;
    double y;
    double z = 0.0;
    double m = 0.0;
    int has_z = 0;
    if (dimensionModel == GAIA_XY_Z)
      {
	  has_z = 1;
	  gaiaGetPointXYZ (coords, iv, &x, &y, &z);
      }
    else if (dimensionModel == GAIA_XY_M)
      {
	  gaiaGetPointXYM (coords, iv, &x, &y, &m);
      }
    else if (dimensionModel == GAIA_XY_Z_M)
      {
	  has_z = 1;
	  gaiaGetPointXYZM (coords, iv, &x, &y, &z, &m);
      }
    else
      {
	  gaiaGetPoint (coords, iv, &x, &y);
      }
    if (dyn->Last != NULL)
      {
	  double prev_x = dyn->Last->X;
	  double prev_y = dyn->Last->Y;
	  if (x == prev_x && y == prev_y)
	      return has_z;	/* skipping repeated points */
      }
    m += base_m;
    gaiaAppendPointZMToDynamicLine (dyn, x, y, z, m);
    return has_z;
}

static void
aux_tsp_add_solution (MultiSolutionPtr multiSolution,
		      ShortestPathSolutionPtr pS, int *route_num,
		      gaiaDynamicLinePtr dyn)
{
/* helper function: adding a solutiont into the TSP resultset */
    RowSolutionPtr pA;
    ResultsetRowPtr row;
    int route_row = 0;

/* inserting the Route Header */
    row = malloc (sizeof (ResultsetRow));
    row->RouteNum = *route_num;
    *route_num += 1;
    row->RouteRow = route_row;
    row->Point2PointRole = VROUTE_POINT2POINT_NONE;
    route_row += 1;
    row->From = pS->From;
    row->To = pS->To;
    row->Undefined = NULL;
    row->linkRef = NULL;
    row->TotalCost = pS->TotalCost;
    row->Geometry = pS->Geometry;
    row->Next = NULL;
    if (multiSolution->FirstRow == NULL)
	multiSolution->FirstRow = row;
    if (multiSolution->LastRow != NULL)
	multiSolution->LastRow->Next = row;
    multiSolution->LastRow = row;
    /* adding the Geometry to the multiSolution list */
    if (multiSolution->FirstGeom == NULL)
	multiSolution->FirstGeom = pS->Geometry;
    if (multiSolution->LastGeom != NULL)
	multiSolution->LastGeom->Next = pS->Geometry;
    multiSolution->LastGeom = pS->Geometry;
    /* removing Geometry ownership form Solution */
    pS->Geometry = NULL;

    /* copying points into the Dynamic Line - TSP Geometry */
    if (row->Geometry != NULL)
      {
	  gaiaLinestringPtr ln = row->Geometry->FirstLinestring;
	  if (ln != NULL)
	    {
		int iv;
		double base_m = 0.0;
		if (dyn->Last != NULL)
		    base_m = dyn->Last->M;
		for (iv = 0; iv < ln->Points; iv++)
		    addPoint2DynLine (ln->Coords, ln->DimensionModel, iv, dyn,
				      base_m);
	    }
      }

    pA = pS->First;
    while (pA != NULL)
      {
	  /* inserting Route's traversed Links */
	  row = malloc (sizeof (ResultsetRow));
	  row->RouteNum = *route_num;
	  row->RouteRow = route_row;
	  row->Point2PointRole = VROUTE_POINT2POINT_NONE;
	  route_row += 1;
	  row->From = NULL;
	  row->To = NULL;
	  row->Undefined = NULL;
	  row->linkRef = pA;
	  row->TotalCost = 0.0;
	  row->Geometry = NULL;
	  row->Next = NULL;
	  if (multiSolution->FirstRow == NULL)
	      multiSolution->FirstRow = row;
	  if (multiSolution->LastRow != NULL)
	      multiSolution->LastRow->Next = row;
	  multiSolution->LastRow = row;
	  /* adding the Link to the multiSolution list */
	  if (multiSolution->FirstLink == NULL)
	      multiSolution->FirstLink = pA;
	  if (multiSolution->LastLink != NULL)
	      multiSolution->LastLink->Next = pA;
	  multiSolution->LastLink = pA;
	  pA = pA->Next;
      }
    route_num += 1;
    /* removing Links ownership from Solution */
    pS->First = NULL;
    pS->Last = NULL;
}

static gaiaGeomCollPtr
aux_build_tsp (MultiSolutionPtr multiSolution, TspTargetsPtr targets,
	       int route_num, int srid)
{
/* helper function: formatting the TSP resultset */
    int has_z = 0;
    gaiaGeomCollPtr geom = NULL;
    gaiaLinestringPtr ln;
    int i;
    int pts = 0;
    gaiaPointPtr pt;
    gaiaDynamicLinePtr dyn = gaiaAllocDynamicLine ();
    for (i = 0; i < targets->Count; i++)
      {
	  /* adding all City to City solutions */
	  ShortestPathSolutionPtr pS = *(targets->Solutions + i);
	  aux_tsp_add_solution (multiSolution, pS, &route_num, dyn);
      }
    /* adding the final trip closing the circular path */
    if (targets->LastSolution->Geometry == NULL)
      {
	  gaiaFreeDynamicLine (dyn);
	  return NULL;
      }
    if (targets->LastSolution->Geometry->DimensionModel == GAIA_XY_Z
	|| targets->LastSolution->Geometry->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    aux_tsp_add_solution (multiSolution, targets->LastSolution, &route_num,
			  dyn);

    /* building the TSP solution as a genuine Geometry */
    if (has_z)
	geom = gaiaAllocGeomCollXYZM ();
    else
	geom = gaiaAllocGeomCollXYM ();
    geom->Srid = srid;
    pt = dyn->First;
    while (pt != NULL)
      {
	  /* counting how many points are there */
	  pts++;
	  pt = pt->Next;
      }
    ln = gaiaAddLinestringToGeomColl (geom, pts);
    pt = dyn->First;
    i = 0;
    while (pt != NULL)
      {
	  /* copying points from Dynamic Line to Linestring */
	  if (has_z)
	    {
		gaiaSetPointXYZM (ln->Coords, i, pt->X, pt->Y, pt->Z, pt->M);
	    }
	  else
	    {
		gaiaSetPointXYM (ln->Coords, i, pt->X, pt->Y, pt->M);
	    }
	  i++;
	  pt = pt->Next;
      }
    gaiaFreeDynamicLine (dyn);
    return geom;
}

static void
build_tsp_solution (MultiSolutionPtr multiSolution, TspTargetsPtr targets,
		    int srid)
{
/* formatting the TSP resultset */
    ShortestPathSolutionPtr *oldS;
    ShortestPathSolutionPtr pS;
    ResultsetRowPtr row;
    RouteNodePtr from;
    int i;
    int k;
    int unreachable = 0;
    int route_row = 0;
    int route_num = 0;
    int found;

/* checking for undefined or unreacheable targets */
    for (i = 0; i < targets->Count; i++)
      {
	  RouteNodePtr to = *(targets->To + i);
	  if (to == NULL)
	      continue;
	  if (*(targets->Found + i) != 'Y')
	      unreachable = 1;
      }

/* inserting the TSP Header */
    row = malloc (sizeof (ResultsetRow));
    row->RouteNum = route_num++;
    row->RouteRow = route_row++;
    row->Point2PointRole = VROUTE_POINT2POINT_NONE;
    row->From = multiSolution->From;
    row->To = multiSolution->From;
    row->Undefined = NULL;
    row->linkRef = NULL;
    if (unreachable)
	row->TotalCost = 0.0;
    else
	row->TotalCost = targets->TotalCost;
    row->Geometry = NULL;
    row->Next = NULL;
    if (multiSolution->FirstRow == NULL)
	multiSolution->FirstRow = row;
    if (multiSolution->LastRow != NULL)
	multiSolution->LastRow->Next = row;
    multiSolution->LastRow = row;

    if (unreachable)
      {
	  /* inserting unreacheable targets */
	  for (i = 0; i < targets->Count; i++)
	    {
		RouteNodePtr to = *(targets->To + i);
		if (to == NULL)
		    continue;
		if (*(targets->Found + i) != 'Y')
		  {
		      /* unreacheable target */
		      row = malloc (sizeof (ResultsetRow));
		      row->RouteNum = route_num++;
		      row->RouteRow = 0;
		      row->Point2PointRole = VROUTE_POINT2POINT_NONE;
		      row->From = to;
		      row->To = to;
		      row->Undefined = NULL;
		      row->linkRef = NULL;
		      row->TotalCost = 0.0;
		      row->Geometry = NULL;
		      row->Next = NULL;
		      if (multiSolution->FirstRow == NULL)
			  multiSolution->FirstRow = row;
		      if (multiSolution->LastRow != NULL)
			  multiSolution->LastRow->Next = row;
		      multiSolution->LastRow = row;
		  }
	    }
	  return;
      }

    /* reordering the TSP solution */
    oldS = targets->Solutions;
    targets->Solutions =
	malloc (sizeof (ShortestPathSolutionPtr) * targets->Count);
    from = multiSolution->From;
    for (k = 0; k < targets->Count; k++)
      {
	  /* building the right sequence */
	  found = 0;
	  for (i = 0; i < targets->Count; i++)
	    {
		pS = *(oldS + i);
		if (pS->From == from)
		  {
		      *(targets->Solutions + k) = pS;
		      from = pS->To;
		      found = 1;
		      break;
		  }
	    }
	  if (!found)
	    {
		if (targets->LastSolution->From == from)
		  {
		      *(targets->Solutions + k) = targets->LastSolution;
		      from = targets->LastSolution->To;
		  }
	    }
      }
    /* adjusting the last route so to close a circular path */
    for (i = 0; i < targets->Count; i++)
      {
	  pS = *(oldS + i);
	  if (pS->From == from)
	    {
		targets->LastSolution = pS;
		break;
	    }
      }
    free (oldS);
    row->Geometry = aux_build_tsp (multiSolution, targets, route_num, srid);
    if (row->Geometry != NULL)
      {
	  if (multiSolution->FirstGeom == NULL)
	      multiSolution->FirstGeom = row->Geometry;
	  if (multiSolution->LastGeom != NULL)
	      multiSolution->LastGeom->Next = row->Geometry;
	  multiSolution->LastGeom = row->Geometry;
      }
}

static void
build_tsp_illegal_solution (MultiSolutionPtr multiSolution,
			    TspTargetsPtr targets)
{
/* formatting the TSP resultset - undefined targets */
    ResultsetRowPtr row;
    int i;
    int route_num = 0;

/* inserting the TSP Header */
    row = malloc (sizeof (ResultsetRow));
    row->RouteNum = route_num++;
    row->RouteRow = 0;
    row->Point2PointRole = VROUTE_POINT2POINT_NONE;
    row->From = multiSolution->From;
    row->To = multiSolution->From;
    row->Undefined = NULL;
    row->linkRef = NULL;
    row->TotalCost = 0.0;
    row->Geometry = NULL;
    row->Next = NULL;
    if (multiSolution->FirstRow == NULL)
	multiSolution->FirstRow = row;
    if (multiSolution->LastRow != NULL)
	multiSolution->LastRow->Next = row;
    multiSolution->LastRow = row;

    for (i = 0; i < targets->Count; i++)
      {
	  char xid[128];
	  const char *code = "unknown";
	  RouteNodePtr to = *(targets->To + i);
	  if (multiSolution->MultiTo->CodeNode)
	      code = *(multiSolution->MultiTo->Codes + i);
	  else
	    {
		sprintf (xid, "%lld", *(multiSolution->MultiTo->Ids + 1));
		code = xid;
	    }
	  if (to == NULL)
	    {
		/* unknown target */
		int len;
		row = malloc (sizeof (ResultsetRow));
		row->RouteNum = route_num++;
		row->RouteRow = 0;
		row->Point2PointRole = VROUTE_POINT2POINT_NONE;
		row->From = to;
		row->To = to;
		len = strlen (code);
		row->Undefined = malloc (len + 1);
		strcpy (row->Undefined, code);
		row->linkRef = NULL;
		row->TotalCost = 0.0;
		row->Geometry = NULL;
		row->Next = NULL;
		if (multiSolution->FirstRow == NULL)
		    multiSolution->FirstRow = row;
		if (multiSolution->LastRow != NULL)
		    multiSolution->LastRow->Next = row;
		multiSolution->LastRow = row;
	    }
	  if (*(targets->Found + i) != 'Y')
	    {
		/* unreachable target */
		row = malloc (sizeof (ResultsetRow));
		row->RouteNum = route_num++;
		row->RouteRow = 0;
		row->Point2PointRole = VROUTE_POINT2POINT_NONE;
		row->From = to;
		row->To = to;
		row->Undefined = NULL;
		row->linkRef = NULL;
		row->TotalCost = 0.0;
		row->Geometry = NULL;
		row->Next = NULL;
		if (multiSolution->FirstRow == NULL)
		    multiSolution->FirstRow = row;
		if (multiSolution->LastRow != NULL)
		    multiSolution->LastRow->Next = row;
		multiSolution->LastRow = row;
	    }
      }
}

static ShortestPathSolutionPtr
add2multiSolution (MultiSolutionPtr multiSolution, RouteNodePtr pfrom,
		   RouteNodePtr pto)
{
/* adding a route solution to a multiple destinations request */
    ShortestPathSolutionPtr solution = alloc_solution ();
    solution->From = pfrom;
    solution->To = pto;
    if (multiSolution->First == NULL)
	multiSolution->First = solution;
    if (multiSolution->Last != NULL)
	multiSolution->Last->Next = solution;
    multiSolution->Last = solution;
    return solution;
}

static ShortestPathSolutionPtr
add2tspLastSolution (TspTargetsPtr targets, RouteNodePtr from, RouteNodePtr to)
{
/* adding the last route solution to a TSP request */
    ShortestPathSolutionPtr solution = alloc_solution ();
    solution->From = from;
    solution->To = to;
    targets->LastSolution = solution;
    return solution;
}

static ShortestPathSolutionPtr
add2tspSolution (TspTargetsPtr targets, RouteNodePtr from, RouteNodePtr to)
{
/* adding a route solution to a TSP request */
    int i;
    ShortestPathSolutionPtr solution = alloc_solution ();
    solution->From = from;
    solution->To = to;
    for (i = 0; i < targets->Count; i++)
      {
	  if (*(targets->To + i) == to)
	    {
		*(targets->Solutions + i) = solution;
		break;
	    }
      }
    return solution;
}

static RouteNodePtr
check_multiTo (RoutingNodePtr node, RoutingMultiDestPtr multiple)
{
/* testing destinations */
    int i;
    for (i = 0; i < multiple->Items; i++)
      {
	  RouteNodePtr to = *(multiple->To + i);
	  if (to == NULL)
	      continue;
	  if (*(multiple->Found + i) == 'Y')
	      continue;
	  if (node->Id == to->InternalIndex)
	    {
		*(multiple->Found + i) = 'Y';
		return to;
	    }
      }
    return NULL;
}

static int
end_multiTo (RoutingMultiDestPtr multiple)
{
/* testing if there are unresolved destinations */
    int i;
    for (i = 0; i < multiple->Items; i++)
      {
	  RouteNodePtr to = *(multiple->To + i);
	  if (to == NULL)
	      continue;
	  if (*(multiple->Found + i) == 'Y')
	      continue;
	  return 0;
      }
    return 1;
}

static void
dijkstra_multi_shortest_path (sqlite3 * handle, int options, RoutingPtr graph,
			      RoutingNodesPtr e, MultiSolutionPtr multiSolution)
{
/* Shortest Path (multiple destinations) - Dijkstra's algorithm */
    int from;
    int i;
    int k;
    ShortestPathSolutionPtr solution;
    RouteNodePtr destination;
    RoutingNodePtr n;
    RoutingNodePtr p_to;
    RouteLinkPtr p_link;
    RoutingHeapPtr heap;
/* setting From */
    from = multiSolution->From->InternalIndex;
/* initializing the heap */
    heap = routing_heap_init (e->DimLink);
/* initializing the graph */
    for (i = 0; i < e->Dim; i++)
      {
	  n = e->Nodes + i;
	  n->PreviousNode = NULL;
	  n->xLink = NULL;
	  n->Inspected = 0;
	  n->Distance = DBL_MAX;
      }
/* queuing the From node into the heap */
    e->Nodes[from].Distance = 0.0;
    dijkstra_enqueue (heap, e->Nodes + from);
    while (heap->Count > 0)
      {
	  /* Dijsktra loop */
	  n = routing_dequeue (heap);
	  destination = check_multiTo (n, multiSolution->MultiTo);
	  if (destination != NULL)
	    {
		/* reached one of the multiple destinations */
		RouteLinkPtr *result;
		int cnt = 0;
		int to = destination->InternalIndex;
		n = e->Nodes + to;
		while (n->PreviousNode != NULL)
		  {
		      /* counting how many Links are into the Shortest Path solution */
		      cnt++;
		      n = n->PreviousNode;
		  }
		/* allocating the solution */
		result = malloc (sizeof (RouteLinkPtr) * cnt);
		k = cnt - 1;
		n = e->Nodes + to;
		while (n->PreviousNode != NULL)
		  {
		      /* inserting a Link into the solution */
		      result[k] = n->xLink;
		      n = n->PreviousNode;
		      k--;
		  }
		solution =
		    add2multiSolution (multiSolution, multiSolution->From,
				       destination);
		build_solution (handle, options, graph, solution, result, cnt);
		/* testing for end (all destinations already reached) */
		if (end_multiTo (multiSolution->MultiTo))
		    break;
	    }
	  n->Inspected = 1;
	  for (i = 0; i < n->DimTo; i++)
	    {
		p_to = *(n->To + i);
		p_link = *(n->Link + i);
		if (p_to->Inspected == 0)
		  {
		      if (p_to->Distance == DBL_MAX)
			{
			    /* queuing a new node into the heap */
			    p_to->Distance = n->Distance + p_link->Cost;
			    p_to->PreviousNode = n;
			    p_to->xLink = p_link;
			    dijkstra_enqueue (heap, p_to);
			}
		      else if (p_to->Distance > n->Distance + p_link->Cost)
			{
			    /* updating an already inserted node */
			    p_to->Distance = n->Distance + p_link->Cost;
			    p_to->PreviousNode = n;
			    p_to->xLink = p_link;
			}
		  }
	    }
      }
    routing_heap_free (heap);
}

static RouteNodePtr
check_targets (RoutingNodePtr node, TspTargetsPtr targets)
{
/* testing targets */
    int i;
    for (i = 0; i < targets->Count; i++)
      {
	  RouteNodePtr to = *(targets->To + i);
	  if (to == NULL)
	      continue;
	  if (*(targets->Found + i) == 'Y')
	      continue;
	  if (node->Id == to->InternalIndex)
	    {
		*(targets->Found + i) = 'Y';
		return to;
	    }
      }
    return NULL;
}

static void
update_targets (TspTargetsPtr targets, RouteNodePtr destination, double cost,
		int *stop)
{
/* tupdating targets */
    int i;
    *stop = 1;
    for (i = 0; i < targets->Count; i++)
      {
	  RouteNodePtr to = *(targets->To + i);
	  if (to == NULL)
	      continue;
	  if (to == destination)
	      *(targets->Costs + i) = cost;
	  if (*(targets->Found + i) == 'Y')
	      continue;
	  *stop = 0;
      }
}


static void
dijkstra_targets_solve (RoutingNodesPtr e, TspTargetsPtr targets)
{
/* Shortest Path (multiple destinations) for TSP GA */
    int from;
    int i;
    RouteNodePtr destination;
    RoutingNodePtr n;
    RoutingNodePtr p_to;
    RouteLinkPtr p_link;
    RoutingHeapPtr heap;
/* setting From */
    from = targets->From->InternalIndex;
/* initializing the heap */
    heap = routing_heap_init (e->DimLink);
/* initializing the graph */
    for (i = 0; i < e->Dim; i++)
      {
	  n = e->Nodes + i;
	  n->PreviousNode = NULL;
	  n->xLink = NULL;
	  n->Inspected = 0;
	  n->Distance = DBL_MAX;
      }
/* queuing the From node into the heap */
    e->Nodes[from].Distance = 0.0;
    dijkstra_enqueue (heap, e->Nodes + from);
    while (heap->Count > 0)
      {
	  /* Dijsktra loop */
	  n = routing_dequeue (heap);
	  destination = check_targets (n, targets);
	  if (destination != NULL)
	    {
		/* reached one of the targets */
		int stop = 0;
		double totalCost = 0.0;
		int to = destination->InternalIndex;
		n = e->Nodes + to;
		while (n->PreviousNode != NULL)
		  {
		      /* computing the total Cost */
		      totalCost += n->xLink->Cost;
		      n = n->PreviousNode;
		  }
		/* updating targets */
		update_targets (targets, destination, totalCost, &stop);
		if (stop)
		    break;
	    }
	  n->Inspected = 1;
	  for (i = 0; i < n->DimTo; i++)
	    {
		p_to = *(n->To + i);
		p_link = *(n->Link + i);
		if (p_to->Inspected == 0)
		  {
		      if (p_to->Distance == DBL_MAX)
			{
			    /* queuing a new node into the heap */
			    p_to->Distance = n->Distance + p_link->Cost;
			    p_to->PreviousNode = n;
			    p_to->xLink = p_link;
			    dijkstra_enqueue (heap, p_to);
			}
		      else if (p_to->Distance > n->Distance + p_link->Cost)
			{
			    /* updating an already inserted node */
			    p_to->Distance = n->Distance + p_link->Cost;
			    p_to->PreviousNode = n;
			    p_to->xLink = p_link;
			}
		  }
	    }
      }
    routing_heap_free (heap);
}

static void
destroy_tsp_targets (TspTargetsPtr targets)
{
/* memory cleanup; destroying a TSP helper struct */
    if (targets == NULL)
	return;
    if (targets->To != NULL)
	free (targets->To);
    if (targets->Found != NULL)
	free (targets->Found);
    if (targets->Costs != NULL)
	free (targets->Costs);
    if (targets->Solutions != NULL)
      {
	  int i;
	  for (i = 0; i < targets->Count; i++)
	    {
		ShortestPathSolutionPtr pS = *(targets->Solutions + i);
		if (pS != NULL)
		    delete_solution (pS);
	    }
	  free (targets->Solutions);
      }
    if (targets->LastSolution != NULL)
	delete_solution (targets->LastSolution);
    free (targets);
}

static TspTargetsPtr
randomize_targets (sqlite3 * handle, RoutingPtr graph,
		   MultiSolutionPtr multiSolution)
{
/* creating and initializing the TSP helper struct */
    char *sql;
    char *prev_sql;
    int ret;
    int i;
    int n_rows;
    int n_columns;
    const char *value = NULL;
    char **results;
    int from;
    TspTargetsPtr targets = malloc (sizeof (TspTargets));
    targets->Mode = multiSolution->Mode;
    targets->TotalCost = 0.0;
    targets->Count = multiSolution->MultiTo->Items;
    targets->To = malloc (sizeof (RouteNodePtr *) * targets->Count);
    targets->Found = malloc (sizeof (char) * targets->Count);
    targets->Costs = malloc (sizeof (double) * targets->Count);
    targets->Solutions =
	malloc (sizeof (ShortestPathSolutionPtr) * targets->Count);
    targets->LastSolution = NULL;
    for (i = 0; i < targets->Count; i++)
      {
	  *(targets->To + i) = NULL;
	  *(targets->Found + i) = 'N';
	  *(targets->Costs + i) = DBL_MAX;
	  *(targets->Solutions + i) = NULL;
      }

    sql =
	sqlite3_mprintf ("SELECT %d, Random() AS rnd\n",
			 multiSolution->From->InternalIndex);
    for (i = 0; i < multiSolution->MultiTo->Items; i++)
      {
	  RouteNodePtr p_to = *(multiSolution->MultiTo->To + i);
	  if (p_to == NULL)
	    {
		sqlite3_free (sql);
		goto illegal;
	    }
	  prev_sql = sql;
	  sql =
	      sqlite3_mprintf ("%sUNION\nSELECT %d, Random() AS rnd\n",
			       prev_sql, p_to->InternalIndex);
	  sqlite3_free (prev_sql);
      }
    prev_sql = sql;
    sql = sqlite3_mprintf ("%sORDER BY rnd LIMIT 1", prev_sql);
    sqlite3_free (prev_sql);
    ret = sqlite3_get_table (handle, sql, &results, &n_rows, &n_columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto illegal;
    if (n_rows >= 1)
      {
	  for (i = 1; i <= n_rows; i++)
	    {
		value = results[(i * n_columns) + 0];
		from = atoi (value);
	    }
      }
    sqlite3_free_table (results);

    if (from == multiSolution->From->InternalIndex)
      {
	  targets->From = multiSolution->From;
	  for (i = 0; i < multiSolution->MultiTo->Items; i++)
	    {
		RouteNodePtr p_to = *(multiSolution->MultiTo->To + i);
		*(targets->To + i) = p_to;
		*(targets->Found + i) = 'N';
		*(targets->Costs + i) = DBL_MAX;
		*(targets->Solutions + i) = NULL;
	    }
      }
    else
      {
	  int j = 0;
	  targets->From = graph->Nodes + from;
	  *(targets->To + j++) = multiSolution->From;
	  for (i = 0; i < multiSolution->MultiTo->Items; i++)
	    {
		RouteNodePtr p_to = *(multiSolution->MultiTo->To + i);
		*(targets->Found + i) = 'N';
		*(targets->Costs + i) = DBL_MAX;
		*(targets->Solutions + i) = NULL;
		if (p_to == targets->From)
		    continue;
		*(targets->To + j++) = p_to;
	    }
      }
    return targets;

  illegal:
    for (i = 0; i < targets->Count; i++)
      {
	  *(targets->To + i) = NULL;
	  *(targets->Found + i) = 'N';
	  *(targets->Costs + i) = DBL_MAX;
	  *(targets->Solutions + i) = NULL;
      }
    for (i = 0; i < multiSolution->MultiTo->Items; i++)
      {
	  RouteNodePtr p_to = *(multiSolution->MultiTo->To + i);
	  if (p_to == NULL)
	      continue;
	  *(targets->To + i) = p_to;
      }
    return targets;
}

static RouteNodePtr
check_TspTo (RoutingNodePtr node, TspTargetsPtr targets)
{
/* testing TSP destinations */
    int i;
    for (i = 0; i < targets->Count; i++)
      {
	  RouteNodePtr to = *(targets->To + i);
	  if (to == NULL)
	      continue;
	  if (*(targets->Found + i) == 'Y')
	      continue;
	  if (node->Id == to->InternalIndex)
	    {
		*(targets->Found + i) = 'Y';
		return to;
	    }
      }
    return NULL;
}

static RouteNodePtr
check_TspFinal (RoutingNodePtr node, TspTargetsPtr targets)
{
/* testing TSP final destination (= FROM) */
    RouteNodePtr from = targets->From;
    if (node->Id == from->InternalIndex)
	return from;
    return NULL;
}

static int
end_TspTo (TspTargetsPtr targets)
{
/* testing if there are unresolved TSP targets */
    int i;
    for (i = 0; i < targets->Count; i++)
      {
	  RouteNodePtr to = *(targets->To + i);
	  if (to == NULL)
	      continue;
	  if (*(targets->Found + i) == 'Y')
	      continue;
	  return 0;
      }
    return 1;
}

static void
dijkstra_tsp_nn (sqlite3 * handle, int options, RoutingPtr graph,
		 RoutingNodesPtr e, TspTargetsPtr targets)
{
/* TSP NN - Dijkstra's algorithm */
    int from;
    int i;
    int k;
    int last_route = 0;
    ShortestPathSolutionPtr solution;
    RouteNodePtr origin;
    RouteNodePtr destination;
    RoutingNodePtr n;
    RoutingNodePtr p_to;
    RouteLinkPtr p_link;
    RoutingHeapPtr heap;

/* setting From */
    from = targets->From->InternalIndex;
    origin = targets->From;
/* initializing the heap */
    heap = routing_heap_init (e->DimLink);
/* initializing the graph */
    for (i = 0; i < e->Dim; i++)
      {
	  n = e->Nodes + i;
	  n->PreviousNode = NULL;
	  n->xLink = NULL;
	  n->Inspected = 0;
	  n->Distance = DBL_MAX;
      }
/* queuing the From node into the heap */
    e->Nodes[from].Distance = 0.0;
    dijkstra_enqueue (heap, e->Nodes + from);
    while (heap->Count > 0)
      {
	  /* Dijsktra loop */
	  n = routing_dequeue (heap);
	  if (last_route)
	      destination = check_TspFinal (n, targets);
	  else
	      destination = check_TspTo (n, targets);
	  if (destination != NULL)
	    {
		/* reached one of the target destinations */
		RouteLinkPtr *result;
		int cnt = 0;
		int to = destination->InternalIndex;
		n = e->Nodes + to;
		while (n->PreviousNode != NULL)
		  {
		      /* counting how many Links are into the Shortest Path solution */
		      cnt++;
		      n = n->PreviousNode;
		  }
		/* allocating the solution */
		result = malloc (sizeof (RouteLinkPtr) * cnt);
		k = cnt - 1;
		n = e->Nodes + to;
		while (n->PreviousNode != NULL)
		  {
		      /* inserting a Link into the solution */
		      result[k] = n->xLink;
		      n = n->PreviousNode;
		      k--;
		  }
		if (last_route)
		    solution =
			add2tspLastSolution (targets, origin, destination);
		else
		    solution = add2tspSolution (targets, origin, destination);
		build_solution (handle, options, graph, solution, result, cnt);
		targets->TotalCost += solution->TotalCost;

		/* testing for end (all destinations already reached) */
		if (last_route)
		    break;
		if (end_TspTo (targets))
		  {
		      /* determining the final route so to close the circular path */
		      last_route = 1;
		  }

		/* restarting from the current target */
		from = to;
		for (i = 0; i < e->Dim; i++)
		  {
		      n = e->Nodes + i;
		      n->PreviousNode = NULL;
		      n->xLink = NULL;
		      n->Inspected = 0;
		      n->Distance = DBL_MAX;
		  }
		e->Nodes[from].Distance = 0.0;
		routing_heap_reset (heap);
		dijkstra_enqueue (heap, e->Nodes + from);
		origin = destination;
		continue;
	    }
	  n->Inspected = 1;
	  for (i = 0; i < n->DimTo; i++)
	    {
		p_to = *(n->To + i);
		p_link = *(n->Link + i);
		if (p_to->Inspected == 0)
		  {
		      if (p_to->Distance == DBL_MAX)
			{
			    /* queuing a new node into the heap */
			    p_to->Distance = n->Distance + p_link->Cost;
			    p_to->PreviousNode = n;
			    p_to->xLink = p_link;
			    dijkstra_enqueue (heap, p_to);
			}
		      else if (p_to->Distance > n->Distance + p_link->Cost)
			{
			    /* updating an already inserted node */
			    p_to->Distance = n->Distance + p_link->Cost;
			    p_to->PreviousNode = n;
			    p_to->xLink = p_link;
			}
		  }
	    }
      }
    routing_heap_free (heap);
}

static RoutingNodePtr *
dijkstra_range_analysis (RoutingNodesPtr e, RouteNodePtr pfrom,
			 double max_cost, int *ll)
{
/* identifying all Nodes within a given Cost range - Dijkstra's algorithm */
    int from;
    int i;
    RoutingNodePtr p_to;
    RoutingNodePtr n;
    RouteLinkPtr p_link;
    int cnt;
    RoutingNodePtr *result;
    RoutingHeapPtr heap;
/* setting From */
    from = pfrom->InternalIndex;
/* initializing the heap */
    heap = routing_heap_init (e->DimLink);
/* initializing the graph */
    for (i = 0; i < e->Dim; i++)
      {
	  n = e->Nodes + i;
	  n->PreviousNode = NULL;
	  n->xLink = NULL;
	  n->Inspected = 0;
	  n->Distance = DBL_MAX;
      }
/* queuing the From node into the heap */
    e->Nodes[from].Distance = 0.0;
    dijkstra_enqueue (heap, e->Nodes + from);
    while (heap->Count > 0)
      {
	  /* Dijsktra loop */
	  n = routing_dequeue (heap);
	  n->Inspected = 1;
	  for (i = 0; i < n->DimTo; i++)
	    {
		p_to = *(n->To + i);
		p_link = *(n->Link + i);
		if (p_to->Inspected == 0)
		  {
		      if (p_to->Distance == DBL_MAX)
			{
			    /* queuing a new node into the heap */
			    if (n->Distance + p_link->Cost <= max_cost)
			      {
				  p_to->Distance = n->Distance + p_link->Cost;
				  p_to->PreviousNode = n;
				  p_to->xLink = p_link;
				  dijkstra_enqueue (heap, p_to);
			      }
			}
		      else if (p_to->Distance > n->Distance + p_link->Cost)
			{
			    /* updating an already inserted node */
			    p_to->Distance = n->Distance + p_link->Cost;
			    p_to->PreviousNode = n;
			    p_to->xLink = p_link;
			}
		  }
	    }
      }
    routing_heap_free (heap);
    cnt = 0;
    for (i = 0; i < e->Dim; i++)
      {
	  /* counting how many traversed Nodes */
	  n = e->Nodes + i;
	  if (n->Node->InternalIndex == from)
	      continue;
	  if (n->Inspected)
	      cnt++;
      }
/* allocating the solution */
    result = malloc (sizeof (RouteNodePtr) * cnt);
    cnt = 0;
    for (i = 0; i < e->Dim; i++)
      {
	  /* populating the resultset */
	  n = e->Nodes + i;
	  if (n->Node->InternalIndex == from)
	      continue;
	  if (n->Inspected)
	      result[cnt++] = n;
      }
    *ll = cnt;
    return (result);
}

/*
/
/  implementation of the A* Shortest Path algorithm
/
*/

static void
astar_insert (RoutingNodePtr node, HeapNodePtr heap, int size)
{
/* inserting a new Node and rearranging the heap */
    int i;
    HeapNode tmp;
    i = size + 1;
    heap[i].Node = node;
    heap[i].Distance = node->HeuristicDistance;
    if (i / 2 < 1)
	return;
    while (heap[i].Distance < heap[i / 2].Distance)
      {
	  tmp = heap[i];
	  heap[i] = heap[i / 2];
	  heap[i / 2] = tmp;
	  i /= 2;
	  if (i / 2 < 1)
	      break;
      }
}

static void
astar_enqueue (RoutingHeapPtr heap, RoutingNodePtr node)
{
/* enqueuing a Node into the heap */
    astar_insert (node, heap->Nodes, heap->Count);
    heap->Count += 1;
}

static double
astar_heuristic_distance (RouteNodePtr n1, RouteNodePtr n2, double coeff)
{
/* computing the euclidean distance intercurring between two nodes */
    double dx = n1->CoordX - n2->CoordX;
    double dy = n1->CoordY - n2->CoordY;
    double dist = sqrt ((dx * dx) + (dy * dy)) * coeff;
    return dist;
}

static RouteLinkPtr *
astar_shortest_path (RoutingNodesPtr e, RouteNodePtr nodes,
		     RouteNodePtr pfrom, RouteNodePtr pto,
		     double heuristic_coeff, int *ll)
{
/* identifying the Shortest Path - A* algorithm */
    int from;
    int to;
    int i;
    int k;
    RoutingNodePtr pAux;
    RoutingNodePtr n;
    RoutingNodePtr p_to;
    RouteNodePtr pOrg;
    RouteNodePtr pDest;
    RouteLinkPtr p_link;
    int cnt;
    RouteLinkPtr *result;
    RoutingHeapPtr heap;
/* setting From/To */
    from = pfrom->InternalIndex;
    to = pto->InternalIndex;
    pAux = e->Nodes + from;
    pOrg = nodes + pAux->Id;
    pAux = e->Nodes + to;
    pDest = nodes + pAux->Id;
/* initializing the heap */
    heap = routing_heap_init (e->DimLink);
/* initializing the graph */
    for (i = 0; i < e->Dim; i++)
      {
	  n = e->Nodes + i;
	  n->PreviousNode = NULL;
	  n->xLink = NULL;
	  n->Inspected = 0;
	  n->Distance = DBL_MAX;
	  n->HeuristicDistance = DBL_MAX;
      }
/* queuing the From node into the heap */
    e->Nodes[from].Distance = 0.0;
    e->Nodes[from].HeuristicDistance =
	astar_heuristic_distance (pOrg, pDest, heuristic_coeff);
    astar_enqueue (heap, e->Nodes + from);
    while (heap->Count > 0)
      {
	  /* A* loop */
	  n = routing_dequeue (heap);
	  if (n->Id == to)
	    {
		/* destination reached */
		break;
	    }
	  n->Inspected = 1;
	  for (i = 0; i < n->DimTo; i++)
	    {
		p_to = *(n->To + i);
		p_link = *(n->Link + i);
		if (p_to->Inspected == 0)
		  {
		      if (p_to->Distance == DBL_MAX)
			{
			    /* queuing a new node into the heap */
			    p_to->Distance = n->Distance + p_link->Cost;
			    pOrg = nodes + p_to->Id;
			    p_to->HeuristicDistance =
				p_to->Distance + astar_heuristic_distance (pOrg,
									   pDest,
									   heuristic_coeff);
			    p_to->PreviousNode = n;
			    p_to->xLink = p_link;
			    astar_enqueue (heap, p_to);
			}
		      else if (p_to->Distance > n->Distance + p_link->Cost)
			{
			    /* updating an already inserted node */
			    p_to->Distance = n->Distance + p_link->Cost;
			    pOrg = nodes + p_to->Id;
			    p_to->HeuristicDistance =
				p_to->Distance + astar_heuristic_distance (pOrg,
									   pDest,
									   heuristic_coeff);
			    p_to->PreviousNode = n;
			    p_to->xLink = p_link;
			}
		  }
	    }
      }
    routing_heap_free (heap);
    cnt = 0;
    n = e->Nodes + to;
    while (n->PreviousNode != NULL)
      {
	  /* counting how many Links are into the Shortest Path solution */
	  cnt++;
	  n = n->PreviousNode;
      }
/* allocating the solution */
    result = malloc (sizeof (RouteLinkPtr) * cnt);
    k = cnt - 1;
    n = e->Nodes + to;
    while (n->PreviousNode != NULL)
      {
	  /* inserting a Link into the solution */
	  result[k] = n->xLink;
	  n = n->PreviousNode;
	  k--;
      }
    *ll = cnt;
    return (result);
}

/* END of A* Shortest Path implementation */

static int
cmp_nodes_code (const void *p1, const void *p2)
{
/* compares two nodes  by CODE [for BSEARCH] */
    RouteNodePtr pN1 = (RouteNodePtr) p1;
    RouteNodePtr pN2 = (RouteNodePtr) p2;
    return strcmp (pN1->Code, pN2->Code);
}

static int
cmp_nodes_id (const void *p1, const void *p2)
{
/* compares two nodes  by ID [for BSEARCH] */
    RouteNodePtr pN1 = (RouteNodePtr) p1;
    RouteNodePtr pN2 = (RouteNodePtr) p2;
    if (pN1->Id == pN2->Id)
	return 0;
    if (pN1->Id > pN2->Id)
	return 1;
    return -1;
}

static RouteNodePtr
find_node_by_code (RoutingPtr graph, const char *code)
{
/* searching a Node (by Code) into the sorted list */
    RouteNodePtr ret;
    RouteNode pN;
    pN.Code = (char *) code;
    ret =
	bsearch (&pN, graph->Nodes, graph->NumNodes, sizeof (RouteNode),
		 cmp_nodes_code);
    return ret;
}

static RouteNodePtr
find_node_by_id (RoutingPtr graph, const sqlite3_int64 id)
{
/* searching a Node (by Id) into the sorted list */
    RouteNodePtr ret;
    RouteNode pN;
    pN.Id = id;
    ret =
	bsearch (&pN, graph->Nodes, graph->NumNodes, sizeof (RouteNode),
		 cmp_nodes_id);
    return ret;
}

static void
vroute_delete_multiple_destinations (RoutingMultiDestPtr multiple)
{
/* deleting a multiple destinations request */
    if (multiple == NULL)
	return;
    if (multiple->Found != NULL)
	free (multiple->Found);
    if (multiple->To != NULL)
	free (multiple->To);
    if (multiple->Ids != NULL)
	free (multiple->Ids);
    if (multiple->Codes != NULL)
      {
	  int i;
	  for (i = 0; i < multiple->Items; i++)
	    {
		char *p = *(multiple->Codes + i);
		if (p != NULL)
		    free (p);
	    }
	  free (multiple->Codes);
      }
    free (multiple);
}

static void
vroute_add_multiple_code (RoutingMultiDestPtr multiple, char *code)
{
/* adding an item to an helper struct for Multiple Destinations (by Code) */
    *(multiple->Codes + multiple->Next) = code;
    multiple->Next += 1;
}

static void
vroute_add_multiple_id (RoutingMultiDestPtr multiple, int id)
{
/* adding an item to an helper struct for Multiple Destinations (by Code) */
    *(multiple->Ids + multiple->Next) = id;
    multiple->Next += 1;
}

static char *
vroute_parse_multiple_item (const char *start, const char *last)
{
/* attempting to parse an item form the multiple destinations list */
    char *item;
    const char *p_in = start;
    char *p_out;
    int len = last - start;
    if (len <= 0)
	return NULL;

    item = malloc (len + 1);
    p_in = start;
    p_out = item;
    while (p_in < last)
	*p_out++ = *p_in++;
    *p_out = '\0';
    return item;
}

static RoutingMultiDestPtr
vroute_as_multiple_destinations (sqlite3_int64 value)
{
/* allocatint a multiple destinations containign a single target */
    RoutingMultiDestPtr multiple = NULL;

/* allocating the helper struct */
    multiple = malloc (sizeof (RoutingMultiDest));
    multiple->CodeNode = 0;
    multiple->Found = malloc (sizeof (char));
    multiple->To = malloc (sizeof (RouteNodePtr));
    *(multiple->Found + 0) = 'N';
    *(multiple->To + 0) = NULL;
    multiple->Items = 1;
    multiple->Next = 0;
    multiple->Ids = malloc (sizeof (sqlite3_int64));
    multiple->Codes = NULL;
    *(multiple->Ids + 0) = value;
    return multiple;
}

static DestinationCandidatesListPtr
alloc_candidates (int code_node)
{
/* allocating a list of multiple destination candidates */
    DestinationCandidatesListPtr list =
	malloc (sizeof (DestinationCandidatesList));
    list->NodeCode = code_node;
    list->First = NULL;
    list->Last = NULL;
    list->ValidItems = 0;
    return list;
}

static void
delete_candidates (DestinationCandidatesListPtr list)
{
/* memory clean-up: destroying a list of multiple destination candidates */
    DestinationCandidatePtr pC;
    DestinationCandidatePtr pCn;
    if (list == NULL)
	return;

    pC = list->First;
    while (pC != NULL)
      {
	  pCn = pC->Next;
	  free (pC);
	  pC = pCn;
      }
    free (list);
}

static int
checkNumber (const char *str)
{
/* testing for an unsignd integer */
    int len;
    int i;
    len = strlen (str);
    for (i = 0; i < len; i++)
      {
	  if (*(str + i) < '0' || *(str + i) > '9')
	      return 0;
      }
    return 1;
}

static void
addMultiCandidate (DestinationCandidatesListPtr list, char *item)
{
/* adding a candidate to the list */
    DestinationCandidatePtr pC;
    if (list == NULL)
	return;
    if (item == NULL)
	return;
    if (list->NodeCode == 0)
      {
	  if (!checkNumber (item))
	    {
		/* invalid: not a number */
		free (item);
		return;
	    }
      }

    pC = malloc (sizeof (DestinationCandidate));
    if (list->NodeCode)
      {
	  pC->Code = item;
	  pC->Id = -1;
      }
    else
      {
	  pC->Code = NULL;
	  pC->Id = atoll (item);
	  free (item);
      }
    pC->Valid = 'Y';
    pC->Next = NULL;
    if (list->First == NULL)
	list->First = pC;
    if (list->Last != NULL)
	list->Last->Next = pC;
    list->Last = pC;
}

static void
validateMultiCandidates (DestinationCandidatesListPtr list)
{
/* validating candidates */
    DestinationCandidatePtr pC = list->First;
    while (pC != NULL)
      {
	  DestinationCandidatePtr pC2;
	  if (pC->Valid == 'N')
	    {
		pC = pC->Next;
		continue;
	    }
	  pC2 = pC->Next;
	  while (pC2 != NULL)
	    {
		/* testing for duplicates */
		if (pC2->Valid == 'N')
		  {
		      pC2 = pC2->Next;
		      continue;
		  }
		if (list->NodeCode)
		  {
		      if (strcmp (pC->Code, pC2->Code) == 0)
			{
			    free (pC2->Code);
			    pC2->Code = NULL;
			    pC2->Valid = 'N';
			}
		  }
		else
		  {
		      if (pC->Id == pC2->Id)
			  pC2->Valid = 'N';
		  }
		pC2 = pC2->Next;
	    }
	  pC = pC->Next;
      }

    list->ValidItems = 0;
    pC = list->First;
    while (pC != NULL)
      {
	  if (pC->Valid == 'Y')
	      list->ValidItems += 1;
	  pC = pC->Next;
      }
}

static RoutingMultiDestPtr
vroute_get_multiple_destinations (int code_node, char delimiter,
				  const char *str)
{
/* parsing a multiple destinations request string */
    RoutingMultiDestPtr multiple = NULL;
    DestinationCandidatesListPtr list = alloc_candidates (code_node);
    DestinationCandidatePtr pC;
    char *item;
    int i;
    const char *prev = str;
    const char *ptr = str;

/* populating a linked list */
    while (1)
      {
	  int whitespace = 0;
	  if (*ptr == '\0')
	    {
		item = vroute_parse_multiple_item (prev, ptr);
		addMultiCandidate (list, item);
		break;
	    }
	  if (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r')
	      whitespace = 1;
	  if (*ptr == delimiter || whitespace)
	    {
		item = vroute_parse_multiple_item (prev, ptr);
		addMultiCandidate (list, item);
		ptr++;
		prev = ptr;
		continue;
	    }
	  ptr++;
      }
    validateMultiCandidates (list);
    if (list->ValidItems <= 0)
      {
	  delete_candidates (list);
	  return NULL;
      }

/* allocating the helper struct */
    multiple = malloc (sizeof (RoutingMultiDest));
    multiple->CodeNode = code_node;
    multiple->Found = malloc (sizeof (char) * list->ValidItems);
    multiple->To = malloc (sizeof (RouteNodePtr) * list->ValidItems);
    for (i = 0; i < list->ValidItems; i++)
      {
	  *(multiple->Found + i) = 'N';
	  *(multiple->To + i) = NULL;
      }
    multiple->Items = list->ValidItems;
    multiple->Next = 0;
    if (code_node)
      {
	  multiple->Ids = NULL;
	  multiple->Codes = malloc (sizeof (char *) * list->ValidItems);
      }
    else
      {
	  multiple->Ids = malloc (sizeof (sqlite3_int64) * list->ValidItems);
	  multiple->Codes = NULL;
      }

/* populating the array */
    pC = list->First;
    while (pC != NULL)
      {
	  if (pC->Valid == 'Y')
	    {
		if (code_node)
		    vroute_add_multiple_code (multiple, pC->Code);
		else
		    vroute_add_multiple_id (multiple, pC->Id);
	    }
	  pC = pC->Next;
      }
    delete_candidates (list);
    return multiple;
}

static void
delete_multiSolution (MultiSolutionPtr multiSolution)
{
/* deleting the current solution */
    ResultsetRowPtr pR;
    ResultsetRowPtr pRn;
    ShortestPathSolutionPtr pS;
    ShortestPathSolutionPtr pSn;
    RowNodeSolutionPtr pN;
    RowNodeSolutionPtr pNn;
    RowSolutionPtr pA;
    RowSolutionPtr pAn;
    gaiaGeomCollPtr pG;
    gaiaGeomCollPtr pGn;
    if (!multiSolution)
	return;
    if (multiSolution->MultiTo != NULL)
	vroute_delete_multiple_destinations (multiSolution->MultiTo);
    pS = multiSolution->First;
    while (pS != NULL)
      {
	  pSn = pS->Next;
	  delete_solution (pS);
	  pS = pSn;
      }
    pN = multiSolution->FirstNode;
    while (pN != NULL)
      {
	  pNn = pN->Next;
	  free (pN);
	  pN = pNn;
      }
    pR = multiSolution->FirstRow;
    while (pR != NULL)
      {
	  pRn = pR->Next;
	  if (pR->Undefined != NULL)
	      free (pR->Undefined);
	  free (pR);
	  pR = pRn;
      }
    pA = multiSolution->FirstLink;
    while (pA != NULL)
      {
	  pAn = pA->Next;
	  if (pA->Name)
	      free (pA->Name);
	  free (pA);
	  pA = pAn;
      }
    pG = multiSolution->FirstGeom;
    while (pG != NULL)
      {
	  pGn = pG->Next;
	  gaiaFreeGeomColl (pG);
	  pG = pGn;
      }
    free (multiSolution);
}

static void
reset_multiSolution (MultiSolutionPtr multiSolution)
{
/* resetting the current solution */
    ResultsetRowPtr pR;
    ResultsetRowPtr pRn;
    ShortestPathSolutionPtr pS;
    ShortestPathSolutionPtr pSn;
    RowNodeSolutionPtr pN;
    RowNodeSolutionPtr pNn;
    RowSolutionPtr pA;
    RowSolutionPtr pAn;
    gaiaGeomCollPtr pG;
    gaiaGeomCollPtr pGn;
    if (!multiSolution)
	return;
    if (multiSolution->MultiTo != NULL)
	vroute_delete_multiple_destinations (multiSolution->MultiTo);
    pS = multiSolution->First;
    while (pS != NULL)
      {
	  pSn = pS->Next;
	  delete_solution (pS);
	  pS = pSn;
      }
    pN = multiSolution->FirstNode;
    while (pN != NULL)
      {
	  pNn = pN->Next;
	  free (pN);
	  pN = pNn;
      }
    pR = multiSolution->FirstRow;
    while (pR != NULL)
      {
	  pRn = pR->Next;
	  free (pR);
	  pR = pRn;
      }
    pA = multiSolution->FirstLink;
    while (pA != NULL)
      {
	  pAn = pA->Next;
	  if (pA->Name)
	      free (pA->Name);
	  free (pA);
	  pA = pAn;
      }
    pG = multiSolution->FirstGeom;
    while (pG != NULL)
      {
	  pGn = pG->Next;
	  gaiaFreeGeomColl (pG);
	  pG = pGn;
      }
    multiSolution->From = NULL;
    multiSolution->MultiTo = NULL;
    multiSolution->First = NULL;
    multiSolution->Last = NULL;
    multiSolution->FirstRow = NULL;
    multiSolution->LastRow = NULL;
    multiSolution->FirstNode = NULL;
    multiSolution->LastNode = NULL;
    multiSolution->CurrentRow = NULL;
    multiSolution->CurrentNodeRow = NULL;
    multiSolution->CurrentRowId = 0;
    multiSolution->FirstLink = NULL;
    multiSolution->LastLink = NULL;
    multiSolution->FirstGeom = NULL;
    multiSolution->LastGeom = NULL;
}

static MultiSolutionPtr
alloc_multiSolution (void)
{
/* allocates and initializes the current multiple-destinations solution */
    MultiSolutionPtr p = malloc (sizeof (MultiSolution));
    p->From = NULL;
    p->MultiTo = NULL;
    p->First = NULL;
    p->Last = NULL;
    p->FirstRow = NULL;
    p->LastRow = NULL;
    p->CurrentRow = NULL;
    p->FirstNode = NULL;
    p->LastNode = NULL;
    p->CurrentNodeRow = NULL;
    p->CurrentRowId = 0;
    p->FirstLink = NULL;
    p->LastLink = NULL;
    p->FirstGeom = NULL;
    p->LastGeom = NULL;
    p->RouteNum = 0;
    return p;
}

static void
delete_point2PointNode (Point2PointNodePtr p)
{
/* deleting a Point2Point Node */
    if (p->codNode != NULL)
	free (p->codNode);
    free (p);
}

static void
delete_point2PointCandidate (Point2PointCandidatePtr p)
{
/* deleting a Point2Point candidate */
    if (p->codNodeFrom != NULL)
	free (p->codNodeFrom);
    if (p->codNodeTo != NULL)
	free (p->codNodeTo);
    if (p->path != NULL)
	gaiaFreeGeomColl (p->path);
    free (p);
}

static void
delete_point2PointSolution (Point2PointSolutionPtr p2pSolution)
{
/* deleting the current Point2Point solution */
    Point2PointCandidatePtr pC;
    Point2PointCandidatePtr pCn;
    Point2PointNodePtr pN;
    Point2PointNodePtr pNn;
    ResultsetRowPtr pR;
    ResultsetRowPtr pRn;
    pC = p2pSolution->firstFromCandidate;
    while (pC != NULL)
      {
	  pCn = pC->next;
	  delete_point2PointCandidate (pC);
	  pC = pCn;
      }
    pC = p2pSolution->firstToCandidate;
    while (pC != NULL)
      {
	  pCn = pC->next;
	  delete_point2PointCandidate (pC);
	  pC = pCn;
      }
    pN = p2pSolution->firstFromNode;
    while (pN != NULL)
      {
	  pNn = pN->next;
	  delete_point2PointNode (pN);
	  pN = pNn;
      }
    pN = p2pSolution->firstToNode;
    while (pN != NULL)
      {
	  pNn = pN->next;
	  delete_point2PointNode (pN);
	  pN = pNn;
      }
    pR = p2pSolution->FirstRow;
    while (pR != NULL)
      {
	  pRn = pR->Next;
	  if (pR->Point2PointRole == VROUTE_POINT2POINT_START
	      || pR->Point2PointRole == VROUTE_POINT2POINT_END)
	    {
		/* deleting partial Links */
		if (pR->linkRef != NULL)
		  {
		      if (pR->linkRef->Link != NULL)
			  free (pR->linkRef->Link);
		      if (pR->linkRef->Name != NULL)
			  free (pR->linkRef->Name);
		      free (pR->linkRef);
		  }
	    }
	  if (pR->Geometry != NULL)
	      gaiaFreeGeomColl (pR->Geometry);
	  free (pR);
	  pR = pRn;
      }
    if (p2pSolution->dynLine != NULL)
	gaiaFreeDynamicLine (p2pSolution->dynLine);
    free (p2pSolution);
}

static void
reset_point2PointSolution (Point2PointSolutionPtr p2pSolution)
{
/* resetting the current solution [Point2Point] */
    Point2PointCandidatePtr pC;
    Point2PointCandidatePtr pCn;
    Point2PointNodePtr pN;
    Point2PointNodePtr pNn;
    ResultsetRowPtr pR;
    ResultsetRowPtr pRn;
    p2pSolution->validFrom = 0;
    p2pSolution->xFrom = 0.0;
    p2pSolution->yFrom = 0.0;
    p2pSolution->zFrom = 0.0;
    p2pSolution->validTo = 0;
    p2pSolution->xTo = 0.0;
    p2pSolution->yTo = 0.0;
    p2pSolution->zTo = 0.0;
    p2pSolution->srid = -1;
    pC = p2pSolution->firstFromCandidate;
    while (pC != NULL)
      {
	  pCn = pC->next;
	  delete_point2PointCandidate (pC);
	  pC = pCn;
      }
    pC = p2pSolution->firstToCandidate;
    while (pC != NULL)
      {
	  pCn = pC->next;
	  delete_point2PointCandidate (pC);
	  pC = pCn;
      }
    p2pSolution->firstFromCandidate = NULL;
    p2pSolution->lastFromCandidate = NULL;
    p2pSolution->firstToCandidate = NULL;
    p2pSolution->lastToCandidate = NULL;
    pN = p2pSolution->firstFromNode;
    while (pN != NULL)
      {
	  pNn = pN->next;
	  delete_point2PointNode (pN);
	  pN = pNn;
      }
    pN = p2pSolution->firstToNode;
    while (pN != NULL)
      {
	  pNn = pN->next;
	  delete_point2PointNode (pN);
	  pN = pNn;
      }
    p2pSolution->firstFromNode = NULL;
    p2pSolution->lastFromNode = NULL;
    p2pSolution->firstToNode = NULL;
    p2pSolution->lastToNode = NULL;
    pR = p2pSolution->FirstRow;
    while (pR != NULL)
      {
	  pRn = pR->Next;
	  if (pR->Point2PointRole == VROUTE_POINT2POINT_START
	      || pR->Point2PointRole == VROUTE_POINT2POINT_END)
	    {
		/* deleting partial Links */
		if (pR->linkRef != NULL)
		  {
		      if (pR->linkRef->Link != NULL)
			  free (pR->linkRef->Link);
		      if (pR->linkRef->Name != NULL)
			  free (pR->linkRef->Name);
		      free (pR->linkRef);
		  }
	    }
	  if (pR->Geometry != NULL)
	      gaiaFreeGeomColl (pR->Geometry);
	  free (pR);
	  pR = pRn;
      }
    p2pSolution->FirstRow = NULL;
    p2pSolution->LastRow = NULL;
    p2pSolution->CurrentRow = NULL;
    p2pSolution->CurrentRowId = 0;
    p2pSolution->totalCost = DBL_MAX;
    p2pSolution->fromCandidate = NULL;
    p2pSolution->toCandidate = NULL;
    if (p2pSolution->dynLine != NULL)
	gaiaFreeDynamicLine (p2pSolution->dynLine);
    p2pSolution->dynLine = NULL;
    p2pSolution->hasZ = 0;
    p2pSolution->Mode = VROUTE_POINT2POINT_ERROR;
}

static Point2PointSolutionPtr
alloc_point2PointSolution (void)
{
/* allocates and initializes the current Point2Point solution */
    Point2PointSolutionPtr p = malloc (sizeof (Point2PointSolution));
    p->validFrom = 0;
    p->xFrom = 0.0;
    p->yFrom = 0.0;
    p->zFrom = 0.0;
    p->validTo = 0;
    p->xTo = 0.0;
    p->yTo = 0.0;
    p->zTo = 0.0;
    p->srid = -1;
    p->firstFromCandidate = NULL;
    p->lastFromCandidate = NULL;
    p->firstToCandidate = NULL;
    p->lastToCandidate = NULL;
    p->firstFromNode = NULL;
    p->lastFromNode = NULL;
    p->firstToNode = NULL;
    p->lastToNode = NULL;
    p->FirstRow = NULL;
    p->LastRow = NULL;
    p->CurrentRow = NULL;
    p->CurrentRowId = 0;
    p->totalCost = DBL_MAX;
    p->fromCandidate = NULL;
    p->toCandidate = NULL;
    p->dynLine = NULL;
    p->hasZ = 0;
    p->Mode = VROUTE_POINT2POINT_ERROR;
    return p;
}

static void
find_srid (sqlite3 * handle, RoutingPtr graph)
{
/* attempting to retrieve the appropriate Srid */
    sqlite3_stmt *stmt;
    int ret;
    int type = GAIA_UNKNOWN;
    int srid = VROUTE_INVALID_SRID;
    char *sql;

    graph->Srid = srid;
    graph->HasZ = 0;

    if (graph->GeometryColumn == NULL)
	return;

    sql =
	sqlite3_mprintf
	("SELECT geometry_type, srid FROM geometry_columns WHERE "
	 "Lower(f_table_name) = Lower(%Q) AND Lower(f_geometry_column) = Lower(%Q)",
	 graph->TableName, graph->GeometryColumn);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		type = sqlite3_column_int (stmt, 0);
		srid = sqlite3_column_int (stmt, 1);
	    }
      }
    sqlite3_finalize (stmt);

    if (srid == VROUTE_INVALID_SRID)
      {
	  /* it could be a Network based on some Spatial VIEW */
	  sql =
	      sqlite3_mprintf
	      ("SELECT g.geometry_type, g.srid FROM views_geometry_columns AS v "
	       "JOIN geometry_columns AS g ON (v.f_table_name = g.f_table_name "
	       "AND v.f_geometry_column = g.f_geometry_column) "
	       "WHERE Lower(v.view_name) = Lower(%Q) AND Lower(v.view_geometry) = Lower(%Q)",
	       graph->TableName, graph->GeometryColumn);
	  ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	      return;
	  while (1)
	    {
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		    break;
		if (ret == SQLITE_ROW)
		  {
		      type = sqlite3_column_int (stmt, 0);
		      srid = sqlite3_column_int (stmt, 1);
		  }
	    }
	  sqlite3_finalize (stmt);
      }

    switch (type)
      {
      case GAIA_POINTZ:
      case GAIA_LINESTRINGZ:
      case GAIA_POLYGONZ:
      case GAIA_MULTIPOINTZ:
      case GAIA_MULTILINESTRINGZ:
      case GAIA_MULTIPOLYGONZ:
      case GAIA_GEOMETRYCOLLECTIONZ:
      case GAIA_POINTZM:
      case GAIA_LINESTRINGZM:
      case GAIA_POLYGONZM:
      case GAIA_MULTIPOINTZM:
      case GAIA_MULTILINESTRINGZM:
      case GAIA_MULTIPOLYGONZM:
      case GAIA_GEOMETRYCOLLECTIONZM:
	  graph->HasZ = 1;
	  break;
      default:
	  graph->HasZ = 0;
	  break;
      };
    graph->Srid = srid;
}

static void
build_range_solution (MultiSolutionPtr multiSolution,
		      RoutingNodePtr * range_nodes, int cnt, int srid)
{
/* formatting the "within Cost range" solution */
    int i;
    if (cnt > 0)
      {
	  /* building the solution */
	  for (i = 0; i < cnt; i++)
	    {
		add_node_to_solution (multiSolution, range_nodes[i], srid, i);
	    }
      }
    if (range_nodes)
	free (range_nodes);
}

static RouteNodePtr
findSingleTo (RoutingMultiDestPtr multiple)
{
/* testing for a single destination */
    int i;
    RouteNodePtr to = NULL;
    int count = 0;
    for (i = 0; i < multiple->Items; i++)
      {
	  RouteNodePtr p_to = *(multiple->To + i);
	  if (p_to == NULL)
	      continue;
	  to = p_to;
	  count++;
      }
    if (count == 1)
	return to;
    return NULL;
}

static void
astar_solve (sqlite3 * handle, int options, RoutingPtr graph,
	     RoutingNodesPtr routing, MultiSolutionPtr multiSolution)
{
/* computing an A* Shortest Path solution */
    int cnt;
    RouteLinkPtr *shortest_path;
    ShortestPathSolutionPtr solution;
    RouteNodePtr to = findSingleTo (multiSolution->MultiTo);
    if (to == NULL)
	return;
    shortest_path =
	astar_shortest_path (routing, graph->Nodes, multiSolution->From, to,
			     graph->AStarHeuristicCoeff, &cnt);
    solution = add2multiSolution (multiSolution, multiSolution->From, to);
    build_solution (handle, options, graph, solution, shortest_path, cnt);
    build_multi_solution (multiSolution);
}

static void
dijkstra_multi_solve (sqlite3 * handle, int options, RoutingPtr graph,
		      RoutingNodesPtr routing, MultiSolutionPtr multiSolution)
{
/* computing a Dijkstra Shortest Path multiSolution */
    int i;
    RoutingMultiDestPtr multiple = multiSolution->MultiTo;
    int node_code = graph->NodeCode;

    dijkstra_multi_shortest_path (handle, options, graph, routing,
				  multiSolution);
/* testing if there are undefined or unresolved destinations */
    for (i = 0; i < multiple->Items; i++)
      {
	  ShortestPathSolutionPtr row;
	  RouteNodePtr to = *(multiple->To + i);
	  if (node_code)
	    {
		/* Nodes are identified by Codes */
		int len;
		const char *code = *(multiple->Codes + i);
		if (to == NULL)
		  {
		      row =
			  add2multiSolution (multiSolution, multiSolution->From,
					     NULL);
		      len = strlen (code);
		      row->Undefined = malloc (len + 1);
		      strcpy (row->Undefined, code);
		      continue;
		  }
		if (*(multiple->Found + i) != 'Y')
		  {
		      row =
			  add2multiSolution (multiSolution, multiSolution->From,
					     to);
		      len = strlen (code);
		      row->Undefined = malloc (len + 1);
		      strcpy (row->Undefined, code);
		  }
	    }
	  else
	    {
		/* Nodes are identified by Ids */
		sqlite3_int64 id = *(multiple->Ids + i);
		if (to == NULL)
		  {
		      row =
			  add2multiSolution (multiSolution, multiSolution->From,
					     NULL);
		      row->Undefined = malloc (4);
		      strcpy (row->Undefined, "???");
		      row->UndefinedId = id;
		      continue;
		  }
		if (*(multiple->Found + i) != 'Y')
		  {
		      row =
			  add2multiSolution (multiSolution, multiSolution->From,
					     to);
		      row->Undefined = malloc (4);
		      strcpy (row->Undefined, "???");
		      row->UndefinedId = id;
		  }
	    }
      }
    build_multi_solution (multiSolution);
}

static void
dijkstra_within_cost_range (RoutingNodesPtr routing,
			    MultiSolutionPtr multiSolution, int srid)
{
/* computing a Dijkstra "within cost range" solution */
    int cnt;
    RoutingNodePtr *range_nodes =
	dijkstra_range_analysis (routing, multiSolution->From,
				 multiSolution->MaxCost,
				 &cnt);
    build_range_solution (multiSolution, range_nodes, cnt, srid);
}

static void
tsp_nn_solve (sqlite3 * handle, int options, RoutingPtr graph,
	      RoutingNodesPtr routing, MultiSolutionPtr multiSolution)
{
/* computing a Dijkstra TSP NN Solution */
    int i;
    TspTargetsPtr targets = randomize_targets (handle, graph, multiSolution);
    for (i = 0; i < targets->Count; i++)
      {
	  /* checking for undefined targets */
	  if (*(targets->To + i) == NULL)
	      goto illegal;
      }
    dijkstra_tsp_nn (handle, options, graph, routing, targets);
    build_tsp_solution (multiSolution, targets, graph->Srid);
    destroy_tsp_targets (targets);
    return;

  illegal:
    build_tsp_illegal_solution (multiSolution, targets);
    destroy_tsp_targets (targets);
}

static void
tsp_ga_random_solutions_sql (TspGaPopulationPtr ga)
{
/* building the Solutions randomizing SQL query */
    int i;
    char *sql;
    char *prev_sql;

    for (i = 0; i < ga->Count; i++)
      {
	  if (i == 0)
	      sql = sqlite3_mprintf ("SELECT %d, Random() AS rnd\n", i);
	  else
	    {
		prev_sql = sql;
		sql =
		    sqlite3_mprintf ("%sUNION\nSELECT %d, Random() AS rnd\n",
				     prev_sql, i);
		sqlite3_free (prev_sql);
	    }
      }
    prev_sql = sql;
    sql = sqlite3_mprintf ("%sORDER BY rnd LIMIT 2", prev_sql);
    sqlite3_free (prev_sql);
    ga->RandomSolutionsSql = sql;
}

static void
tsp_ga_random_interval_sql (TspGaPopulationPtr ga)
{
/* building the Interval randomizing SQL query */
    int i;
    char *sql;
    char *prev_sql;

    for (i = 0; i < ga->Cities; i++)
      {
	  if (i == 0)
	      sql = sqlite3_mprintf ("SELECT %d, Random() AS rnd\n", i);
	  else
	    {
		prev_sql = sql;
		sql =
		    sqlite3_mprintf ("%sUNION\nSELECT %d, Random() AS rnd\n",
				     prev_sql, i);
		sqlite3_free (prev_sql);
	    }
      }
    prev_sql = sql;
    sql = sqlite3_mprintf ("%sORDER BY rnd LIMIT 2", prev_sql);
    sqlite3_free (prev_sql);
    ga->RandomIntervalSql = sql;
}

static TspGaDistancePtr
alloc_tsp_ga_distances (TspTargetsPtr targets)
{
/* allocating a TSP GA distances struct */
    int i;
    TspGaDistancePtr dist = malloc (sizeof (TspGaDistance));
    dist->Cities = targets->Count;
    dist->CityFrom = targets->From;
    dist->Distances = malloc (sizeof (TspGaSubDistancePtr) * dist->Cities);
    for (i = 0; i < dist->Cities; i++)
      {
	  TspGaSubDistancePtr sub = malloc (sizeof (TspGaSubDistance));
	  double cost = *(targets->Costs + i);
	  sub->CityTo = *(targets->To + i);
	  sub->Cost = cost;
	  *(dist->Distances + i) = sub;
      }
    dist->NearestIndex = -1;
    return dist;
}

static void
destroy_tsp_ga_distances (TspGaDistancePtr dist)
{
/* freeing a TSP GA distance struct */
    if (dist == NULL)
	return;
    if (dist->Distances != NULL)
      {
	  int i;
	  for (i = 0; i < dist->Cities; i++)
	    {
		TspGaSubDistancePtr sub = *(dist->Distances + i);
		if (sub != NULL)
		    free (sub);
	    }
	  free (dist->Distances);
      }
    free (dist);
}

static TspGaPopulationPtr
build_tsp_ga_population (int count)
{
/* creating a TSP GA Population */
    int i;
    TspGaPopulationPtr ga = malloc (sizeof (TspGaPopulation));
    ga->Count = count;
    ga->Cities = count;
    ga->Solutions = malloc (sizeof (TspGaSolutionPtr) * count);
    ga->Offsprings = malloc (sizeof (TspGaSolutionPtr) * count);
    for (i = 0; i < count; i++)
      {
	  *(ga->Offsprings + i) = NULL;
	  *(ga->Solutions + i) = NULL;
      }
    ga->Distances = malloc (sizeof (TspGaDistancePtr) * count);
    for (i = 0; i < count; i++)
	*(ga->Distances + i) = NULL;
    ga->RandomSolutionsSql = NULL;
    tsp_ga_random_solutions_sql (ga);
    ga->RandomIntervalSql = NULL;
    tsp_ga_random_interval_sql (ga);
    return ga;
}

static void
destroy_tsp_ga_solution (TspGaSolutionPtr solution)
{
/* memory cleanup: destroyng a GA Solution */
    if (solution == NULL)
	return;
    if (solution->CitiesFrom != NULL)
	free (solution->CitiesFrom);
    if (solution->CitiesTo != NULL)
	free (solution->CitiesTo);
    if (solution->Costs != NULL)
	free (solution->Costs);
    free (solution);
}

static void
free_tsp_ga_offsprings (TspGaPopulationPtr ga)
{
/* memory cleanup; freeing GA Offsprings */
    int i;
    if (ga == NULL)
	return;

    for (i = 0; i < ga->Count; i++)
      {
	  if (*(ga->Offsprings + i) != NULL)
	      destroy_tsp_ga_solution (*(ga->Offsprings + i));
	  *(ga->Offsprings + i) = NULL;
      }
}

static void
destroy_tsp_ga_population (TspGaPopulationPtr ga)
{
/* memory cleanup; destroyng a GA Population */
    int i;
    if (ga == NULL)
	return;

    for (i = 0; i < ga->Count; i++)
	destroy_tsp_ga_solution (*(ga->Solutions + i));
    free (ga->Solutions);
    free_tsp_ga_offsprings (ga);
    free (ga->Offsprings);
    if (ga->Distances != NULL)
      {
	  for (i = 0; i < ga->Cities; i++)
	    {
		TspGaDistancePtr dist = *(ga->Distances + i);
		if (dist != NULL)
		    destroy_tsp_ga_distances (dist);
	    }
      }
    free (ga->Distances);
    if (ga->RandomSolutionsSql != NULL)
	sqlite3_free (ga->RandomSolutionsSql);
    if (ga->RandomIntervalSql != NULL)
	sqlite3_free (ga->RandomIntervalSql);
    free (ga);
}

static int
cmp_dist_from (const void *p1, const void *p2)
{
/* compares FROM distances [for BSEARCH] */
    TspGaDistancePtr pD1 = (TspGaDistancePtr) p1;
    TspGaDistancePtr pD2 = *((TspGaDistancePtr *) p2);
    RouteNodePtr pN1 = pD1->CityFrom;
    RouteNodePtr pN2 = pD2->CityFrom;
    if (pN1 == pN2)
	return 0;
    if (pN1 > pN2)
	return 1;
    return -1;
}

static TspGaDistancePtr
tsp_ga_find_from_distance (TspGaPopulationPtr ga, RouteNodePtr from)
{
/* searching the main FROM distance */
    TspGaDistancePtr *ret;
    TspGaDistance dist;
    dist.CityFrom = from;
    ret =
	bsearch (&dist, ga->Distances, ga->Cities, sizeof (TspGaDistancePtr),
		 cmp_dist_from);
    if (ret == NULL)
	return NULL;
    return *ret;
}

static int
cmp_dist_to (const void *p1, const void *p2)
{
/* compares TO distances [for BSEARCH] */
    TspGaSubDistancePtr pD1 = (TspGaSubDistancePtr) p1;
    TspGaSubDistancePtr pD2 = *((TspGaSubDistancePtr *) p2);
    RouteNodePtr pN1 = pD1->CityTo;
    RouteNodePtr pN2 = pD2->CityTo;
    if (pN1 == pN2)
	return 0;
    if (pN1 > pN2)
	return 1;
    return -1;
}

static TspGaSubDistancePtr
tsp_ga_find_to_distance (TspGaDistancePtr dist, RouteNodePtr to)
{
/* searching the main FROM distance */
    TspGaSubDistancePtr *ret;
    TspGaSubDistance sub;
    sub.CityTo = to;
    ret =
	bsearch (&sub, dist->Distances, dist->Cities,
		 sizeof (TspGaSubDistancePtr), cmp_dist_to);
    if (ret == NULL)
	return NULL;
    return *ret;
}

static double
tsp_ga_find_distance (TspGaPopulationPtr ga, RouteNodePtr from, RouteNodePtr to)
{
/* searching a cached distance */
    TspGaSubDistancePtr sub;
    TspGaDistancePtr dist = tsp_ga_find_from_distance (ga, from);
    if (dist == NULL)
	return DBL_MAX;

    sub = tsp_ga_find_to_distance (dist, to);
    if (sub != NULL)
	return sub->Cost;
    return DBL_MAX;
}

static void
tsp_ga_random_solutions (sqlite3 * handle, TspGaPopulationPtr ga, int *index1,
			 int *index2)
{
/* fetching two random TSP GA solutions */
    int i;
    int ret;
    int n_rows;
    int n_columns;
    const char *value = NULL;
    char **results;

    *index1 = -1;
    *index2 = -1;

    ret =
	sqlite3_get_table (handle, ga->RandomSolutionsSql, &results, &n_rows,
			   &n_columns, NULL);
    if (ret != SQLITE_OK)
	return;
    if (n_rows >= 1)
      {
	  for (i = 1; i <= n_rows; i++)
	    {
		value = results[(i * n_columns) + 0];
		if (i == 1)
		    *index1 = atoi (value);
		else
		    *index2 = atoi (value);
	    }
      }
    sqlite3_free_table (results);
}

static void
tsp_ga_random_interval (sqlite3 * handle, TspGaPopulationPtr ga, int *index1,
			int *index2)
{
/* fetching a random TSP GA interval */
    int i;
    int ret;
    int n_rows;
    int n_columns;
    const char *value = NULL;
    char **results;

    *index1 = -1;
    *index2 = -1;

    ret =
	sqlite3_get_table (handle, ga->RandomIntervalSql, &results, &n_rows,
			   &n_columns, NULL);
    if (ret != SQLITE_OK)
	return;
    if (n_rows >= 1)
      {
	  for (i = 1; i <= n_rows; i++)
	    {
		value = results[(i * n_columns) + 0];
		if (i == 1)
		    *index1 = atoi (value);
		else
		    *index2 = atoi (value);
	    }
      }
    sqlite3_free_table (results);
}

static void
tps_ga_chromosome_update (TspGaSolutionPtr chromosome, RouteNodePtr from,
			  RouteNodePtr to, double cost)
{
/* updating a chromosome (missing distance) */
    int j;
    for (j = 0; j < chromosome->Cities; j++)
      {
	  RouteNodePtr n1 = *(chromosome->CitiesFrom + j);
	  RouteNodePtr n2 = *(chromosome->CitiesTo + j);
	  if (n1 == from && n2 == to)
	      *(chromosome->Costs + j) = cost;
      }
}

static void
tsp_ga_random_mutation (sqlite3 * handle, TspGaPopulationPtr ga,
			TspGaSolutionPtr mutant)
{
/* introducing a random mutation */
    RouteNodePtr mutation;
    int j;
    int idx1;
    int idx2;

/* applying a random mutation */
    tsp_ga_random_interval (handle, ga, &idx1, &idx2);
    mutation = *(mutant->CitiesFrom + idx1);
    *(mutant->CitiesFrom + idx1) = *(mutant->CitiesFrom + idx2);
    *(mutant->CitiesFrom + idx2) = mutation;

/* adjusting From/To */
    for (j = 1; j < mutant->Cities; j++)
      {
	  RouteNodePtr pFrom = *(mutant->CitiesFrom + j);
	  *(mutant->CitiesTo + j - 1) = pFrom;
      }
    *(mutant->CitiesTo + mutant->Cities - 1) = *(mutant->CitiesFrom + 0);

/* adjusting Costs */
    mutant->TotalCost = 0.0;
    for (j = 0; j < mutant->Cities; j++)
      {
	  RouteNodePtr pFrom = *(mutant->CitiesFrom + j);
	  RouteNodePtr pTo = *(mutant->CitiesTo + j);
	  double cost = tsp_ga_find_distance (ga, pFrom, pTo);
	  tps_ga_chromosome_update (mutant, pFrom, pTo, cost);
	  *(mutant->Costs + j) = cost;
	  mutant->TotalCost += cost;
      }
}

static TspGaSolutionPtr
tsp_ga_clone_solution (TspGaPopulationPtr ga, TspGaSolutionPtr original)
{
/* cloning a TSP GA solution */
    int j;
    TspGaSolutionPtr clone;
    if (original == NULL)
	return NULL;

    clone = malloc (sizeof (TspGaSolution));
    clone->Cities = original->Cities;
    clone->CitiesFrom = malloc (sizeof (RouteNodePtr) * ga->Cities);
    clone->CitiesTo = malloc (sizeof (RouteNodePtr) * ga->Cities);
    clone->Costs = malloc (sizeof (double) * ga->Cities);
    for (j = 0; j < ga->Cities; j++)
      {
	  *(clone->CitiesFrom + j) = *(original->CitiesFrom + j);
	  *(clone->CitiesTo + j) = *(original->CitiesTo + j);
	  *(clone->Costs + j) = *(original->Costs + j);
      }
    clone->TotalCost = 0.0;
    return clone;
}

static TspGaSolutionPtr
tsp_ga_crossover (sqlite3 * handle, TspGaPopulationPtr ga, int mutation1,
		  int mutation2)
{
/* creating a Crossover solution */
    int j;
    int idx1;
    int idx2;
    TspGaSolutionPtr hybrid;
    TspGaSolutionPtr parent1 = NULL;
    TspGaSolutionPtr parent2 = NULL;

/* randomly choosing two parents */
    tsp_ga_random_solutions (handle, ga, &idx1, &idx2);
    if (idx1 >= 0 && idx1 < ga->Count)
	parent1 = tsp_ga_clone_solution (ga, *(ga->Solutions + idx1));
    if (idx2 >= 0 && idx2 < ga->Count)
	parent2 = tsp_ga_clone_solution (ga, *(ga->Solutions + idx2));
    if (parent1 == NULL || parent2 == NULL)
	goto stop;

    if (mutation1)
	tsp_ga_random_mutation (handle, ga, parent1);
    if (mutation2)
	tsp_ga_random_mutation (handle, ga, parent2);

/* creating an empty hybrid */
    hybrid = malloc (sizeof (TspGaSolution));
    hybrid->Cities = ga->Cities;
    hybrid->CitiesFrom = malloc (sizeof (RouteNodePtr) * ga->Cities);
    hybrid->CitiesTo = malloc (sizeof (RouteNodePtr) * ga->Cities);
    hybrid->Costs = malloc (sizeof (double) * ga->Cities);
    for (j = 0; j < ga->Cities; j++)
      {
	  *(hybrid->CitiesFrom + j) = NULL;
	  *(hybrid->CitiesTo + j) = NULL;
	  *(hybrid->Costs + j) = DBL_MAX;
      }
    hybrid->TotalCost = 0.0;

/* step #1: inheritance from the fist parent */
    tsp_ga_random_interval (handle, ga, &idx1, &idx2);
    if (idx1 < idx2)
      {
	  for (j = idx1; j <= idx2; j++)
	      *(hybrid->CitiesFrom + j) = *(parent1->CitiesFrom + j);
      }
    else
      {
	  for (j = idx2; j <= idx1; j++)
	      *(hybrid->CitiesFrom + j) = *(parent1->CitiesFrom + j);
      }

/* step #2: inheritance from the second parent */
    for (j = 0; j < parent2->Cities; j++)
      {
	  RouteNodePtr p2From = *(parent2->CitiesFrom + j);
	  int k;
	  int found = 0;
	  if (p2From == NULL)
	      continue;
	  for (k = 0; k < hybrid->Cities; k++)
	    {
		RouteNodePtr p1From = *(hybrid->CitiesFrom + k);
		if (p1From == NULL)
		    continue;
		if (p1From == p2From)
		  {
		      /* already present: skipping */
		      found = 1;
		      break;
		  }
	    }
	  if (found)
	      continue;
	  for (k = 0; k < hybrid->Cities; k++)
	    {
		if (*(hybrid->CitiesFrom + k) == NULL
		    && *(hybrid->CitiesTo + k) == NULL
		    && *(hybrid->Costs + k) == DBL_MAX)
		  {
		      /* found an empty slot: inserting */
		      *(hybrid->CitiesFrom + k) = p2From;
		      break;
		  }
	    }
      }
    destroy_tsp_ga_solution (parent1);
    destroy_tsp_ga_solution (parent2);

/* adjusting From/To */
    for (j = 1; j < hybrid->Cities; j++)
      {
	  RouteNodePtr p1From = *(hybrid->CitiesFrom + j);
	  *(hybrid->CitiesTo + j - 1) = p1From;
      }
    *(hybrid->CitiesTo + hybrid->Cities - 1) = *(hybrid->CitiesFrom + 0);

/* retrieving cached costs */
    for (j = 0; j < hybrid->Cities; j++)
      {
	  RouteNodePtr pFrom = *(hybrid->CitiesFrom + j);
	  RouteNodePtr pTo = *(hybrid->CitiesTo + j);
	  double cost = tsp_ga_find_distance (ga, pFrom, pTo);
	  tps_ga_chromosome_update (hybrid, pFrom, pTo, cost);
	  hybrid->TotalCost += cost;
      }
    return hybrid;

  stop:
    if (parent1 != NULL)
	destroy_tsp_ga_solution (parent1);
    if (parent2 != NULL)
	destroy_tsp_ga_solution (parent2);
    return NULL;
}

static void
evalTspGaFitness (TspGaPopulationPtr ga)
{
/* evaluating the comparative fitness of parents and offsprings */
    int j;
    int i;
    int index;
    int already_defined = 0;

    for (j = 0; j < ga->Count; j++)
      {
	  /* evaluating an offsprings */
	  double max_cost = 0.0;
	  TspGaSolutionPtr hybrid = *(ga->Offsprings + j);

	  for (i = 0; i < ga->Count; i++)
	    {
		/* searching the worst parent */
		TspGaSolutionPtr old = *(ga->Solutions + i);
		if (old->TotalCost > max_cost)
		  {
		      max_cost = old->TotalCost;
		      index = i;
		  }
		if (old->TotalCost == hybrid->TotalCost)
		    already_defined = 1;
	    }
	  if (max_cost > hybrid->TotalCost && !already_defined)
	    {
		/* inserting the new hybrid by replacing the worst parent */
		TspGaSolutionPtr kill = *(ga->Solutions + index);
		*(ga->Solutions + index) = hybrid;
		*(ga->Offsprings + j) = NULL;
		destroy_tsp_ga_solution (kill);
	    }
      }
}

static MultiSolutionPtr
tsp_ga_compute_route (sqlite3 * handle, int options, RouteNodePtr origin,
		      RouteNodePtr destination, RoutingPtr graph,
		      RoutingNodesPtr routing)
{
/* computing a route from City to City */
    RoutingMultiDestPtr to = NULL;
    MultiSolutionPtr ms = alloc_multiSolution ();
    ms->From = origin;
    to = malloc (sizeof (RoutingMultiDest));
    ms->MultiTo = to;
    to->CodeNode = graph->NodeCode;
    to->Found = malloc (sizeof (char));
    to->To = malloc (sizeof (RouteNodePtr));
    *(to->Found + 0) = 'N';
    *(to->To + 0) = destination;
    to->Items = 1;
    to->Next = 0;
    if (graph->NodeCode)
      {
	  int len = strlen (destination->Code);
	  to->Ids = NULL;
	  to->Codes = malloc (sizeof (char *));
	  *(to->Codes + 0) = malloc (len + 1);
	  strcpy (*(to->Codes + 0), destination->Code);
      }
    else
      {
	  to->Ids = malloc (sizeof (sqlite3_int64));
	  to->Codes = NULL;
	  *(to->Ids + 0) = destination->Id;
      }

    dijkstra_multi_shortest_path (handle, options, graph, routing, ms);
    return (ms);
}

static void
completing_tsp_ga_solution (sqlite3 * handle, int options,
			    RouteNodePtr origin, RouteNodePtr destination,
			    RoutingPtr graph, RoutingNodesPtr routing,
			    TspTargetsPtr targets, int j)
{
/* completing a TSP GA solution */
    ShortestPathSolutionPtr solution;
    MultiSolutionPtr result =
	tsp_ga_compute_route (handle, options, origin, destination, graph,
			      routing);

    solution = result->First;
    while (solution != NULL)
      {
	  RowSolutionPtr old;
	  ShortestPathSolutionPtr newSolution = alloc_solution ();
	  newSolution->From = origin;
	  newSolution->To = destination;
	  newSolution->TotalCost += solution->TotalCost;
	  targets->TotalCost += solution->TotalCost;
	  newSolution->Geometry = solution->Geometry;
	  solution->Geometry = NULL;
	  if (j < 0)
	      targets->LastSolution = newSolution;
	  else
	      *(targets->Solutions + j) = newSolution;
	  old = solution->First;
	  while (old != NULL)
	    {
		/* inserts a Link into the Shortest Path solution */
		RowSolutionPtr p = malloc (sizeof (RowSolution));
		p->Link = old->Link;
		p->Name = old->Name;
		old->Name = NULL;
		p->Next = NULL;
		if (!(newSolution->First))
		    newSolution->First = p;
		if (newSolution->Last)
		    newSolution->Last->Next = p;
		newSolution->Last = p;
		old = old->Next;
	    }
	  solution = solution->Next;
      }
    delete_multiSolution (result);
}

static void
set_tsp_ga_targets (sqlite3 * handle, int options, RoutingPtr graph,
		    RoutingNodesPtr routing, TspGaSolutionPtr bestSolution,
		    TspTargetsPtr targets)
{
/* preparing TSP GA targets (best solution found) */
    int j;
    RouteNodePtr from;
    RouteNodePtr to;

    for (j = 0; j < targets->Count; j++)
      {
	  from = *(bestSolution->CitiesFrom + j);
	  to = *(bestSolution->CitiesTo + j);
	  completing_tsp_ga_solution (handle, options, from, to, graph, routing,
				      targets, j);
	  *(targets->To + j) = to;
	  *(targets->Found + j) = 'Y';
      }
    /* this is the final City closing the circular path */
    from = *(bestSolution->CitiesFrom + targets->Count);
    to = *(bestSolution->CitiesTo + targets->Count);
    completing_tsp_ga_solution (handle, options, from, to, graph, routing,
				targets, -1);
}

static TspTargetsPtr
build_tsp_ga_solution_targets (int count, RouteNodePtr from)
{
/* creating and initializing the TSP helper struct - final solution */
    int i;
    TspTargetsPtr targets = malloc (sizeof (TspTargets));
    targets->Mode = VROUTE_TSP_SOLUTION;
    targets->TotalCost = 0.0;
    targets->Count = count;
    targets->To = malloc (sizeof (RouteNodePtr *) * targets->Count);
    targets->Found = malloc (sizeof (char) * targets->Count);
    targets->Costs = malloc (sizeof (double) * targets->Count);
    targets->Solutions =
	malloc (sizeof (ShortestPathSolutionPtr) * targets->Count);
    targets->LastSolution = NULL;
    targets->From = from;
    for (i = 0; i < targets->Count; i++)
      {
	  *(targets->To + i) = NULL;
	  *(targets->Found + i) = 'N';
	  *(targets->Costs + i) = DBL_MAX;
	  *(targets->Solutions + i) = NULL;
      }
    return targets;
}

static TspTargetsPtr
tsp_ga_permuted_targets (RouteNodePtr from, RoutingMultiDestPtr multi,
			 int index)
{
/* initializing the TSP helper struct - permuted */
    int i;
    TspTargetsPtr targets = malloc (sizeof (TspTargets));
    targets->Mode = VROUTE_ROUTING_SOLUTION;
    targets->TotalCost = 0.0;
    targets->Count = multi->Items;
    targets->To = malloc (sizeof (RouteNodePtr *) * targets->Count);
    targets->Found = malloc (sizeof (char) * targets->Count);
    targets->Costs = malloc (sizeof (double) * targets->Count);
    targets->Solutions =
	malloc (sizeof (ShortestPathSolutionPtr) * targets->Count);
    targets->LastSolution = NULL;
    if (index < 0)
      {
	  /* no permutation */
	  targets->From = from;
	  for (i = 0; i < targets->Count; i++)
	    {
		*(targets->To + i) = *(multi->To + i);
		*(targets->Found + i) = 'N';
		*(targets->Costs + i) = DBL_MAX;
		*(targets->Solutions + i) = NULL;
	    }
      }
    else
      {
	  /* permuting */
	  targets->From = *(multi->To + index);
	  for (i = 0; i < targets->Count; i++)
	    {
		if (i == index)
		  {
		      *(targets->To + i) = from;
		      *(targets->Found + i) = 'N';
		      *(targets->Costs + i) = DBL_MAX;
		      *(targets->Solutions + i) = NULL;
		  }
		else
		  {
		      *(targets->To + i) = *(multi->To + i);
		      *(targets->Found + i) = 'N';
		      *(targets->Costs + i) = DBL_MAX;
		      *(targets->Solutions + i) = NULL;
		  }
	    }
      }
    return targets;
}

static int
build_tsp_nn_solution (TspGaPopulationPtr ga, TspTargetsPtr targets, int index)
{
/* building a TSP NN solution */
    int j;
    int i;
    RouteNodePtr origin;
    TspGaDistancePtr dist;
    TspGaSubDistancePtr sub;
    RouteNodePtr destination;
    double cost;
    TspGaSolutionPtr solution = malloc (sizeof (TspGaSolution));
    solution->Cities = targets->Count + 1;
    solution->CitiesFrom = malloc (sizeof (RouteNodePtr) * solution->Cities);
    solution->CitiesTo = malloc (sizeof (RouteNodePtr) * solution->Cities);
    solution->Costs = malloc (sizeof (double) * solution->Cities);
    solution->TotalCost = 0.0;

    origin = targets->From;
    for (j = 0; j < targets->Count; j++)
      {
	  /* searching the nearest City */
	  dist = tsp_ga_find_from_distance (ga, origin);
	  if (dist == NULL)
	      return 0;
	  destination = NULL;
	  cost = DBL_MAX;

	  sub = *(dist->Distances + dist->NearestIndex);
	  destination = sub->CityTo;
	  cost = sub->Cost;
	  /* excluding the last City (should be a closed circuit) */
	  if (destination == targets->From)
	      destination = NULL;
	  if (destination != NULL)
	    {
		for (i = 0; i < targets->Count; i++)
		  {
		      /* checking if this City was already reached */
		      RouteNodePtr city = *(targets->To + i);
		      if (city == destination)
			{
			    if (*(targets->Found + i) == 'Y')
				destination = NULL;
			    else
				*(targets->Found + i) = 'Y';
			    break;
			}
		  }
	    }
	  if (destination == NULL)
	    {
		/* searching for an alternative destination */
		double min = DBL_MAX;
		int ind = -1;
		int k;
		for (k = 0; k < dist->Cities; k++)
		  {
		      sub = *(dist->Distances + k);
		      RouteNodePtr city = sub->CityTo;
		      /* excluding the last City (should be a closed circuit) */
		      if (city == targets->From)
			  continue;
		      for (i = 0; i < targets->Count; i++)
			{
			    RouteNodePtr city2 = *(targets->To + i);
			    if (*(targets->Found + i) == 'Y')
				continue;
			    if (city == city2)
			      {
				  if (sub->Cost < min)
				    {
					min = sub->Cost;
					ind = k;
				    }
			      }
			}
		  }
		if (ind >= 0)
		  {
		      sub = *(dist->Distances + ind);
		      destination = sub->CityTo;
		      cost = min;
		      for (i = 0; i < targets->Count; i++)
			{
			    RouteNodePtr city = *(targets->To + i);
			    if (city == destination)
			      {
				  *(targets->Found + i) = 'Y';
				  break;
			      }
			}
		  }
	    }
	  if (destination == NULL)
	      return 0;
	  *(solution->CitiesFrom + j) = origin;
	  *(solution->CitiesTo + j) = destination;
	  *(solution->Costs + j) = cost;
	  solution->TotalCost += cost;
	  origin = destination;
      }

/* returning to FROM so to close the circular path */
    destination = targets->From;
    for (i = 0; i < ga->Cities; i++)
      {
	  TspGaDistancePtr d = *(ga->Distances + i);
	  if (d->CityFrom == origin)
	    {
		int k;
		dist = *(ga->Distances + i);
		for (k = 0; k < dist->Cities; k++)
		  {
		      sub = *(dist->Distances + k);
		      RouteNodePtr city = sub->CityTo;
		      if (city == destination)
			{
			    cost = sub->Cost;
			    *(solution->CitiesFrom + targets->Count) = origin;
			    *(solution->CitiesTo + targets->Count) =
				destination;
			    *(solution->Costs + targets->Count) = cost;
			    solution->TotalCost += cost;
			}
		  }
	    }
      }

/* inserting into the GA population */
    *(ga->Solutions + index) = solution;
    return 1;
}

static int
cmp_nodes_addr (const void *p1, const void *p2)
{
/* compares two nodes  by addr [for QSORT] */
    TspGaDistancePtr pD1 = *((TspGaDistancePtr *) p1);
    TspGaDistancePtr pD2 = *((TspGaDistancePtr *) p2);
    RouteNodePtr pN1 = pD1->CityFrom;
    RouteNodePtr pN2 = pD2->CityFrom;
    if (pN1 == pN2)
	return 0;
    if (pN1 > pN2)
	return 1;
    return -1;
}

static int
cmp_dist_addr (const void *p1, const void *p2)
{
/* compares two nodes  by addr [for QSORT] */
    TspGaSubDistancePtr pD1 = *((TspGaSubDistancePtr *) p1);
    TspGaSubDistancePtr pD2 = *((TspGaSubDistancePtr *) p2);
    RouteNodePtr pN1 = pD1->CityTo;
    RouteNodePtr pN2 = pD2->CityTo;
    if (pN1 == pN2)
	return 0;
    if (pN1 > pN2)
	return 1;
    return -1;
}

static void
tsp_ga_sort_distances (TspGaPopulationPtr ga)
{
/* sorting Distances/Costs by Node addr */
    int i;
    qsort (ga->Distances, ga->Cities, sizeof (RouteNodePtr), cmp_nodes_addr);
    for (i = 0; i < ga->Cities; i++)
      {
	  TspGaDistancePtr dist = *(ga->Distances + i);
	  qsort (dist->Distances, dist->Cities, sizeof (TspGaSubDistancePtr),
		 cmp_dist_addr);
      }
    for (i = 0; i < ga->Cities; i++)
      {
	  int k;
	  int index = -1;
	  double min = DBL_MAX;
	  TspGaDistancePtr dist = *(ga->Distances + i);
	  for (k = 0; k < dist->Cities; k++)
	    {
		TspGaSubDistancePtr sub = *(dist->Distances + k);
		if (sub->Cost < min)
		  {
		      min = sub->Cost;
		      index = k;
		  }
	    }
	  if (index >= 0)
	      dist->NearestIndex = index;
      }
}

static void
tsp_ga_solve (sqlite3 * handle, int options, RoutingPtr graph,
	      RoutingNodesPtr routing, MultiSolutionPtr multiSolution)
{
/* computing a Dijkstra TSP GA Solution */
    int i;
    int j;
    double min;
    TspGaSolutionPtr bestSolution;
    int count = 0;
    int max_iterations = VROUTE_TSP_GA_MAX_ITERATIONS;
    TspGaPopulationPtr ga = NULL;
    RoutingMultiDestPtr multi;
    TspTargetsPtr targets;
    TspGaDistancePtr dist;

    if (multiSolution == NULL)
	return;
    multi = multiSolution->MultiTo;
    if (multi == NULL)
	return;

/* initialinzing the TSP GA helper struct */
    ga = build_tsp_ga_population (multi->Items + 1);

    for (i = -1; i < multi->Items; i++)
      {
	  /* determining all City-to-City distances (costs) */
	  targets = tsp_ga_permuted_targets (multiSolution->From, multi, i);
	  for (j = 0; j < targets->Count; j++)
	    {
		/* checking for undefined targets */
		if (*(targets->To + j) == NULL)
		  {
		      int k;
		      for (k = 0; k < targets->Count; k++)
			{
			    /* maskinkg unreachable targets */
			    *(targets->Found + k) = 'Y';
			}
		      build_tsp_illegal_solution (multiSolution, targets);
		      destroy_tsp_targets (targets);
		      goto invalid;
		  }
	    }
	  dijkstra_targets_solve (routing, targets);
	  for (j = 0; j < targets->Count; j++)
	    {
		/* checking for unreachable targets */
		if (*(targets->Found + j) != 'Y')
		  {
		      build_tsp_illegal_solution (multiSolution, targets);
		      destroy_tsp_targets (targets);
		      goto invalid;
		  }
	    }
	  /* inserting the distances/costs into the helper struct */
	  dist = alloc_tsp_ga_distances (targets);
	  *(ga->Distances + i + 1) = dist;
	  destroy_tsp_targets (targets);
      }
    tsp_ga_sort_distances (ga);

    for (i = -1; i < multi->Items; i++)
      {
	  /* initializing GA using permuted NN solutions */
	  int ret;
	  targets = tsp_ga_permuted_targets (multiSolution->From, multi, i);
	  ret = build_tsp_nn_solution (ga, targets, i + 1);
	  destroy_tsp_targets (targets);
	  if (!ret)
	      goto invalid;
      }

    while (max_iterations >= 0)
      {
	  /* sexual reproduction and darwinian selection */
	  for (i = 0; i < ga->Count; i++)
	    {
		/* Genetic loop - with mutations */
		TspGaSolutionPtr hybrid;
		int mutation1 = 0;
		int mutation2 = 0;
		count++;
		if (count % 13 == 0)
		  {
		      /* introducing a random mutation on parent #1 */
		      mutation1 = 1;
		  }
		if (count % 16 == 0)
		  {
		      /* introducing a random mutation on parent #2 */
		      mutation2 = 1;
		  }
		hybrid = tsp_ga_crossover (handle, ga, mutation1, mutation2);
		*(ga->Offsprings + i) = hybrid;
	    }
	  evalTspGaFitness (ga);
	  free_tsp_ga_offsprings (ga);
	  max_iterations--;
      }

/* building the TSP GA solution */
    min = DBL_MAX;
    bestSolution = NULL;
    for (i = 0; i < ga->Count; i++)
      {
	  /* searching the best solution */
	  TspGaSolutionPtr old = *(ga->Solutions + i);
	  if (old == NULL)
	      continue;
	  if (old->TotalCost < min)
	    {
		min = old->TotalCost;
		bestSolution = old;
	    }
      }
    if (bestSolution != NULL)
      {
	  targets =
	      build_tsp_ga_solution_targets (multiSolution->MultiTo->Items,
					     multiSolution->From);
	  set_tsp_ga_targets (handle, options, graph, routing, bestSolution,
			      targets);
	  build_tsp_solution (multiSolution, targets, graph->Srid);
	  destroy_tsp_targets (targets);
      }
    destroy_tsp_ga_population (ga);
    return;

  invalid:
    destroy_tsp_ga_population (ga);
}

static void
network_free (RoutingPtr p)
{
/* memory cleanup; freeing any allocation for the network struct */
    RouteNodePtr pN;
    int i;
    if (!p)
	return;
    for (i = 0; i < p->NumNodes; i++)
      {
	  pN = p->Nodes + i;
	  if (pN->Code)
	      free (pN->Code);
	  if (pN->Links)
	      free (pN->Links);
      }
    if (p->Nodes)
	free (p->Nodes);
    if (p->TableName)
	free (p->TableName);
    if (p->FromColumn)
	free (p->FromColumn);
    if (p->ToColumn)
	free (p->ToColumn);
    if (p->GeometryColumn)
	free (p->GeometryColumn);
    if (p->NameColumn)
	free (p->NameColumn);
    free (p);
}

static RoutingPtr
network_init (const unsigned char *blob, int size)
{
/* parsing the HEADER block */
    RoutingPtr graph;
    int net64;
    int aStar = 0;
    int nodes;
    int node_code;
    int max_code_length;
    int endian_arch = gaiaEndianArch ();
    const char *table;
    const char *from;
    const char *to;
    const char *geom;
    const char *name = NULL;
    double a_star_coeff = 1.0;
    int len;
    int i;
    const unsigned char *ptr;
    if (size < 9)
	return NULL;
    if (*(blob + 0) == GAIA_NET_START)	/* signature - legacy format using 32bit ints */
	net64 = 0;
    else if (*(blob + 0) == GAIA_NET64_START)	/* signature - format using 64bit ints */
	net64 = 1;
    else if (*(blob + 0) == GAIA_NET64_A_STAR_START)	/* signature - format using 64bit ints AND supporting A* */
      {
	  net64 = 1;
	  aStar = 1;
      }
    else
	return NULL;
    if (*(blob + 1) != GAIA_NET_HEADER)	/* signature */
	return NULL;
    nodes = gaiaImport32 (blob + 2, 1, endian_arch);	/* # nodes */
    if (nodes <= 0)
	return NULL;
    if (*(blob + 6) == GAIA_NET_CODE)	/* Nodes identified by a TEXT code */
	node_code = 1;
    else if (*(blob + 6) == GAIA_NET_ID)	/* Nodes indentified by an INTEGER id */
	node_code = 0;
    else
	return NULL;
    max_code_length = *(blob + 7);	/* Max TEXT Code length */
    if (*(blob + 8) != GAIA_NET_TABLE)	/* signature for TABLE NAME */
	return NULL;
    ptr = blob + 9;
    len = gaiaImport16 (ptr, 1, endian_arch);	/* TABLE NAME is varlen */
    ptr += 2;
    table = (char *) ptr;
    ptr += len;
    if (*ptr != GAIA_NET_FROM)	/* signature for FromNode COLUMN */
	return NULL;
    ptr++;
    len = gaiaImport16 (ptr, 1, endian_arch);	/* FromNode COLUMN is varlen */
    ptr += 2;
    from = (char *) ptr;
    ptr += len;
    if (*ptr != GAIA_NET_TO)	/* signature for ToNode COLUMN */
	return NULL;
    ptr++;
    len = gaiaImport16 (ptr, 1, endian_arch);	/* ToNode COLUMN is varlen */
    ptr += 2;
    to = (char *) ptr;
    ptr += len;
    if (*ptr != GAIA_NET_GEOM)	/* signature for Geometry COLUMN */
	return NULL;
    ptr++;
    len = gaiaImport16 (ptr, 1, endian_arch);	/* Geometry COLUMN is varlen */
    ptr += 2;
    geom = (char *) ptr;
    ptr += len;
    if (net64)
      {
	  if (*ptr != GAIA_NET_NAME)	/* signature for Name COLUMN - may be empty */
	      return NULL;
	  ptr++;
	  len = gaiaImport16 (ptr, 1, endian_arch);	/* Name COLUMN is varlen */
	  ptr += 2;
	  name = (char *) ptr;
	  ptr += len;
      }
    if (net64 && aStar)
      {
	  if (*ptr != GAIA_NET_A_STAR_COEFF)	/* signature for A* Heuristic Coeff */
	      return NULL;
	  ptr++;
	  a_star_coeff = gaiaImport64 (ptr, 1, endian_arch);
	  ptr += 8;
      }
    if (*ptr != GAIA_NET_END)	/* signature */
	return NULL;
    graph = malloc (sizeof (Routing));
    graph->Srid = VROUTE_INVALID_SRID;
    graph->HasZ = 0;
    graph->Net64 = net64;
    graph->AStar = aStar;
    graph->EndianArch = endian_arch;
    graph->CurrentIndex = 0;
    graph->NodeCode = node_code;
    graph->MaxCodeLength = max_code_length;
    graph->NumNodes = nodes;
    graph->Nodes = malloc (sizeof (RouteNode) * nodes);
    for (i = 0; i < nodes; i++)
      {
	  graph->Nodes[i].Code = NULL;
	  graph->Nodes[i].NumLinks = 0;
	  graph->Nodes[i].Links = NULL;
      }
    len = strlen (table);
    graph->TableName = malloc (len + 1);
    strcpy (graph->TableName, table);
    len = strlen (from);
    graph->FromColumn = malloc (len + 1);
    strcpy (graph->FromColumn, from);
    len = strlen (to);
    graph->ToColumn = malloc (len + 1);
    strcpy (graph->ToColumn, to);
    len = strlen (geom);
    if (len <= 1)
	graph->GeometryColumn = NULL;
    else
      {
	  graph->GeometryColumn = malloc (len + 1);
	  strcpy (graph->GeometryColumn, geom);
      }
    if (!net64)
      {
	  /* Name column is not supported */
	  graph->NameColumn = NULL;
      }
    else
      {
	  len = strlen (name);
	  if (len <= 1)
	      graph->NameColumn = NULL;
	  else
	    {
		graph->NameColumn = malloc (len + 1);
		strcpy (graph->NameColumn, name);
	    }
      }
    graph->AStarHeuristicCoeff = a_star_coeff;
    return graph;
}

static int
network_block (RoutingPtr graph, const unsigned char *blob, int size)
{
/* parsing a NETWORK Block */
    const unsigned char *in = blob;
    int nodes;
    int i;
    int ia;
    int index;
    char *code = NULL;
    double x;
    double y;
    sqlite3_int64 nodeId = -1;
    int links;
    RouteNodePtr pN;
    RouteLinkPtr pA;
    int len;
    sqlite3_int64 linkId;
    int nodeToIdx;
    double cost;
    if (size < 3)
	goto error;
    if (*in++ != GAIA_NET_BLOCK)	/* signature */
	goto error;
    nodes = gaiaImport16 (in, 1, graph->EndianArch);	/* # Nodes */
    in += 2;
    code = malloc (graph->MaxCodeLength + 1);
    for (i = 0; i < nodes; i++)
      {
	  /* parsing each node */
	  if ((size - (in - blob)) < 5)
	      goto error;
	  if (*in++ != GAIA_NET_NODE)	/* signature */
	      goto error;
	  index = gaiaImport32 (in, 1, graph->EndianArch);	/* node internal index */
	  in += 4;
	  if (index < 0 || index >= graph->NumNodes)
	      goto error;
	  if (graph->NodeCode)
	    {
		/* Nodes are identified by a TEXT Code */
		if ((size - (in - blob)) < graph->MaxCodeLength)
		    goto error;
		memcpy (code, in, graph->MaxCodeLength);
		*(code + graph->MaxCodeLength) = '\0';
		in += graph->MaxCodeLength;
	    }
	  else
	    {
		/* Nodes are identified by an INTEGER Id */
		if (graph->Net64)
		  {
		      if ((size - (in - blob)) < 8)
			  goto error;
		      nodeId = gaiaImportI64 (in, 1, graph->EndianArch);	/* the Node ID: 64bit */
		      in += 8;
		  }
		else
		  {
		      if ((size - (in - blob)) < 4)
			  goto error;
		      nodeId = gaiaImport32 (in, 1, graph->EndianArch);	/* the Node ID: 32bit */
		      in += 4;
		  }
	    }
	  if (graph->AStar)
	    {
		/* fetching node's X,Y coords */
		if ((size - (in - blob)) < 8)
		    goto error;
		x = gaiaImport64 (in, 1, graph->EndianArch);	/* X coord */
		in += 8;
		if ((size - (in - blob)) < 8)
		    goto error;
		y = gaiaImport64 (in, 1, graph->EndianArch);	/* Y coord */
		in += 8;
	    }
	  else
	    {
		x = DBL_MAX;
		y = DBL_MAX;
	    }
	  if ((size - (in - blob)) < 2)
	      goto error;
	  links = gaiaImport16 (in, 1, graph->EndianArch);	/* # Links */
	  in += 2;
	  if (links < 0)
	      goto error;
	  /* initializing the Node */
	  pN = graph->Nodes + index;
	  pN->InternalIndex = index;
	  if (graph->NodeCode)
	    {
		/* Nodes are identified by a TEXT Code */
		pN->Id = -1;
		len = strlen (code);
		pN->Code = malloc (len + 1);
		strcpy (pN->Code, code);
	    }
	  else
	    {
		/* Nodes are identified by an INTEGER Id */
		pN->Id = nodeId;
		pN->Code = NULL;
	    }
	  pN->CoordX = x;
	  pN->CoordY = y;
	  pN->NumLinks = links;
	  if (links)
	    {
		/* parsing the Links */
		pN->Links = malloc (sizeof (RouteLink) * links);
		for (ia = 0; ia < links; ia++)
		  {
		      /* parsing each Link */
		      if (graph->Net64)
			{
			    if ((size - (in - blob)) < 22)
				goto error;
			}
		      else
			{
			    if ((size - (in - blob)) < 18)
				goto error;
			}
		      if (*in++ != GAIA_NET_ARC)	/* signature */
			  goto error;
		      if (graph->Net64)
			{
			    linkId = gaiaImportI64 (in, 1, graph->EndianArch);	/* # Link ROWID: 64bit */
			    in += 8;
			}
		      else
			{
			    linkId = gaiaImport32 (in, 1, graph->EndianArch);	/* # Link ROWID: 32bit */
			    in += 4;
			}
		      nodeToIdx = gaiaImport32 (in, 1, graph->EndianArch);	/* # NodeTo internal index */
		      in += 4;
		      cost = gaiaImport64 (in, 1, graph->EndianArch);	/* # Cost */
		      in += 8;
		      if (*in++ != GAIA_NET_END)	/* signature */
			  goto error;
		      pA = pN->Links + ia;
		      /* initializing the Link */
		      if (nodeToIdx < 0 || nodeToIdx >= graph->NumNodes)
			  goto error;
		      pA->NodeFrom = pN;
		      pA->NodeTo = graph->Nodes + nodeToIdx;
		      pA->LinkRowid = linkId;
		      pA->Cost = cost;
		  }
	    }
	  else
	      pN->Links = NULL;
	  if ((size - (in - blob)) < 1)
	      goto error;
	  if (*in++ != GAIA_NET_END)	/* signature */
	      goto error;
      }
    free (code);
    return 1;
  error:
    if (code != NULL)
	free (code);
    return 0;
}

static RoutingPtr
load_network (sqlite3 * handle, const char *table)
{
/* loads the NETWORK struct */
    RoutingPtr graph = NULL;
    sqlite3_stmt *stmt;
    char *sql;
    int ret;
    int header = 1;
    const unsigned char *blob;
    int size;
    char *xname;
    xname = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("SELECT NetworkData FROM \"%s\" ORDER BY Id", xname);
    free (xname);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto abort;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      blob =
			  (const unsigned char *) sqlite3_column_blob (stmt, 0);
		      size = sqlite3_column_bytes (stmt, 0);
		      if (header)
			{
			    /* parsing the HEADER block */
			    graph = network_init (blob, size);
			    header = 0;
			}
		      else
			{
			    /* parsing ordinary Blocks */
			    if (!graph)
			      {
				  sqlite3_finalize (stmt);
				  goto abort;
			      }
			    if (!network_block (graph, blob, size))
			      {
				  sqlite3_finalize (stmt);
				  goto abort;
			      }
			}
		  }
		else
		  {
		      sqlite3_finalize (stmt);
		      goto abort;
		  }
	    }
	  else
	    {
		sqlite3_finalize (stmt);
		goto abort;
	    }
      }
    sqlite3_finalize (stmt);
    find_srid (handle, graph);
    return graph;
  abort:
    network_free (graph);
    return NULL;
}

static void
set_multi_by_id (RoutingMultiDestPtr multiple, RoutingPtr graph)
{
// setting Node pointers to multiple destinations */
    int i;
    for (i = 0; i < multiple->Items; i++)
      {
	  sqlite3_int64 id = *(multiple->Ids + i);
	  if (id >= 1)
	      *(multiple->To + i) = find_node_by_id (graph, id);
      }
}

static void
set_multi_by_code (RoutingMultiDestPtr multiple, RoutingPtr graph)
{
// setting Node pointers to multiple destinations */
    int i;
    for (i = 0; i < multiple->Items; i++)
      {
	  const char *code = *(multiple->Codes + i);
	  if (code != NULL)
	      *(multiple->To + i) = find_node_by_code (graph, code);
      }
}

static int
do_check_valid_point (gaiaGeomCollPtr geom, int srid)
{
/* checking for a valid Point geometry */
    if (geom == NULL)
	return 0;
    if (geom->FirstLinestring != NULL)
	return 0;
    if (geom->FirstPolygon != NULL)
	return 0;
    if (geom->FirstPoint == NULL)
	return 0;
    if (geom->FirstPoint != geom->LastPoint)
	return 0;
    if (geom->Srid != srid)
	return 0;
    return 1;
}

static void
add_by_code_to_point2point (virtualroutingPtr net, sqlite3_int64 rowid,
			    const char *node_from, const char *node_to,
			    int reverse, int mode)
{
/* adding to the Point2Point list a new candidate */
    int len;
    Point2PointSolutionPtr p2p = net->point2PointSolution;
    Point2PointCandidatePtr p = malloc (sizeof (Point2PointCandidate));
    p->linkRowid = rowid;
    len = strlen (node_from);
    p->codNodeFrom = malloc (len + 1);
    strcpy (p->codNodeFrom, node_from);
    len = strlen (node_to);
    p->codNodeTo = malloc (len + 1);
    strcpy (p->codNodeTo, node_to);
    p->reverse = reverse;
    p->valid = 0;
    p->path = NULL;
    p->pathLen = 0.0;
    p->extraLen = 0.0;
    p->percent = 0.0;
    p->next = NULL;
/* adding to the list */
    if (mode == VROUTE_POINT2POINT_FROM)
      {
	  if (p2p->firstFromCandidate == NULL)
	      p2p->firstFromCandidate = p;
	  if (p2p->lastFromCandidate != NULL)
	      p2p->lastFromCandidate->next = p;
	  p2p->lastFromCandidate = p;
      }
    else
      {
	  if (p2p->firstToCandidate == NULL)
	      p2p->firstToCandidate = p;
	  if (p2p->lastToCandidate != NULL)
	      p2p->lastToCandidate->next = p;
	  p2p->lastToCandidate = p;
      }
}

static int
do_check_by_code_point2point_oneway (RoutingPtr graph, sqlite3_int64 rowid,
				     const char *node_from, const char *node_to)
{
/* checking if the Link do really joins the two nodes */
    int j;
    RouteNodePtr node = find_node_by_code (graph, node_from);
    if (node == NULL)
	return 0;
    for (j = 0; j < node->NumLinks; j++)
      {
	  RouteLinkPtr link = &(node->Links[j]);
	  if (strcmp (link->NodeFrom->Code, node_from) == 0
	      && strcmp (link->NodeTo->Code, node_to) == 0
	      && link->LinkRowid == rowid)
	      return 1;
      }
    return 0;
}

static int
do_check_by_id_point2point_oneway (RoutingPtr graph, sqlite3_int64 rowid,
				   sqlite3_int64 node_from,
				   sqlite3_int64 node_to)
{
/* checking if the Link do really joins the two nodes */
    int j;
    RouteNodePtr node = find_node_by_id (graph, node_from);
    if (node == NULL)
	return 0;
    for (j = 0; j < node->NumLinks; j++)
      {
	  RouteLinkPtr link = &(node->Links[j]);
	  if (link->NodeFrom->Id == node_from && link->NodeTo->Id == node_to
	      && link->LinkRowid == rowid)
	      return 1;
      }
    return 0;
}

static void
add_by_id_to_point2point (virtualroutingPtr net, sqlite3_int64 rowid,
			  sqlite_int64 node_from, sqlite3_int64 node_to,
			  int reverse, int mode)
{
/* adding to the Point2Point list a new candidate */
    Point2PointSolutionPtr p2p = net->point2PointSolution;
    Point2PointCandidatePtr p = malloc (sizeof (Point2PointCandidate));
    p->linkRowid = rowid;
    p->codNodeFrom = NULL;
    p->codNodeTo = NULL;
    p->idNodeFrom = node_from;
    p->idNodeTo = node_to;
    p->reverse = reverse;
    p->valid = 0;
    p->path = NULL;
    p->pathLen = 0.0;
    p->extraLen = 0.0;
    p->percent = 0.0;
    p->next = NULL;
/* adding to the list */
    if (mode == VROUTE_POINT2POINT_FROM)
      {
	  if (p2p->firstFromCandidate == NULL)
	      p2p->firstFromCandidate = p;
	  if (p2p->lastFromCandidate != NULL)
	      p2p->lastFromCandidate->next = p;
	  p2p->lastFromCandidate = p;
      }
    else
      {
	  if (p2p->firstToCandidate == NULL)
	      p2p->firstToCandidate = p;
	  if (p2p->lastToCandidate != NULL)
	      p2p->lastToCandidate->next = p;
	  p2p->lastToCandidate = p;
      }
}

static int
do_prepare_point (virtualroutingPtr net, int mode)
{
/* preparing a Point for Point2Point */
    RoutingPtr graph = net->graph;
    Point2PointSolutionPtr p2p = net->point2PointSolution;
    char *xfrom;
    char *xto;
    char *xtable;
    char *xgeom;
    char *sql;
    int ret;
    int ok = 0;
    sqlite3 *sqlite = net->db;
    sqlite3_stmt *stmt = NULL;

    xfrom = gaiaDoubleQuotedSql (graph->FromColumn);
    xto = gaiaDoubleQuotedSql (graph->ToColumn);
    xtable = gaiaDoubleQuotedSql (graph->TableName);
    xgeom = gaiaDoubleQuotedSql (graph->GeometryColumn);
    sql =
	sqlite3_mprintf ("SELECT r.rowid, r.\"%s\", r.\"%s\", "
			 "ST_Distance(p.geom, r.\"%s\") AS dist "
			 "FROM \"%s\" AS r, (SELECT MakePoint(?, ?) AS geom) AS p "
			 "WHERE dist <= ? AND r.rowid IN "
			 "(SELECT rowid FROM SpatialIndex WHERE f_table_name = %Q  AND "
			 "f_geometry_column = %Q AND search_frame = BuildCircleMBR(?, ?, ?)) "
			 "ORDER BY dist", xfrom, xto, xgeom, xtable,
			 graph->TableName, graph->GeometryColumn);
    free (xfrom);
    free (xto);
    free (xtable);
    free (xgeom);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  return 0;
      }

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    if (mode == VROUTE_POINT2POINT_FROM)
      {
	  sqlite3_bind_double (stmt, 1, p2p->xFrom);
	  sqlite3_bind_double (stmt, 2, p2p->yFrom);
	  sqlite3_bind_double (stmt, 3, net->Tolerance);
	  sqlite3_bind_double (stmt, 4, p2p->xFrom);
	  sqlite3_bind_double (stmt, 5, p2p->yFrom);
	  sqlite3_bind_double (stmt, 6, net->Tolerance);
      }
    else
      {
	  sqlite3_bind_double (stmt, 1, p2p->xTo);
	  sqlite3_bind_double (stmt, 2, p2p->yTo);
	  sqlite3_bind_double (stmt, 3, net->Tolerance);
	  sqlite3_bind_double (stmt, 4, p2p->xTo);
	  sqlite3_bind_double (stmt, 5, p2p->yTo);
	  sqlite3_bind_double (stmt, 6, net->Tolerance);
      }
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int ok_node_from = 0;
		int ok_node_to = 0;
		const char *cod_node_from;
		const char *cod_node_to;
		sqlite3_int64 id_node_from;
		sqlite3_int64 id_node_to;

		sqlite3_int64 rowid = sqlite3_column_int64 (stmt, 0);
		if (graph->NodeCode)
		  {
		      if (sqlite3_column_type (stmt, 1) == SQLITE_TEXT)
			{
			    ok_node_from = 1;
			    cod_node_from =
				(const char *) sqlite3_column_text (stmt, 1);
			}
		      if (sqlite3_column_type (stmt, 2) == SQLITE_TEXT)
			{
			    ok_node_to = 1;
			    cod_node_to =
				(const char *) sqlite3_column_text (stmt, 2);
			}
		  }
		else
		  {
		      if (sqlite3_column_type (stmt, 1) == SQLITE_INTEGER)
			{
			    ok_node_from = 1;
			    id_node_from = sqlite3_column_int64 (stmt, 1);
			}
		      if (sqlite3_column_type (stmt, 2) == SQLITE_INTEGER)
			{
			    ok_node_to = 1;
			    id_node_to = sqlite3_column_int64 (stmt, 2);
			}
		  }
		if (ok_node_from && ok_node_to)
		  {
		      if (graph->NodeCode)
			{
			    /* direct connection */
			    if (do_check_by_code_point2point_oneway
				(graph, rowid, cod_node_from, cod_node_to))
			      {
				  add_by_code_to_point2point (net, rowid,
							      cod_node_from,
							      cod_node_to, 0,
							      mode);
				  ok = 1;
			      }
			    /* reverse connection */
			    if (do_check_by_code_point2point_oneway
				(graph, rowid, cod_node_to, cod_node_from))
			      {
				  add_by_code_to_point2point (net, rowid,
							      cod_node_to,
							      cod_node_from, 1,
							      mode);
				  ok = 1;
			      }
			}
		      else
			{
			    /* direct connection */
			    if (do_check_by_id_point2point_oneway
				(graph, rowid, id_node_from, id_node_to))
			      {
				  add_by_id_to_point2point (net, rowid,
							    id_node_from,
							    id_node_to, 0,
							    mode);
				  ok = 1;
			      }
			    /* reverse connection */
			    if (do_check_by_id_point2point_oneway
				(graph, rowid, id_node_to, id_node_from))
			      {
				  add_by_id_to_point2point (net, rowid,
							    id_node_to,
							    id_node_from, 1,
							    mode);
				  ok = 1;
			      }
			}
		  }
	    }
      }
    sqlite3_finalize (stmt);
    return ok;
}

static double
doComputeExtraLength (virtualroutingPtr net, double xFrom, double yFrom,
		      Point2PointCandidatePtr ptr, int end_node)
{
/* computing an Ingress / Egress length */
    RouteNodePtr node = NULL;
    RoutingPtr graph = net->graph;
    if (ptr->reverse)
      {
	  if (graph->NodeCode)
	    {
		/* nodes are identified by TEXT codes */
		if (end_node)
		    node = find_node_by_code (graph, ptr->codNodeFrom);
		else
		    node = find_node_by_code (graph, ptr->codNodeTo);
	    }
	  else
	    {
		/* nodes are identified by integer IDs */
		if (end_node)
		    node = find_node_by_id (graph, ptr->idNodeFrom);
		else
		    node = find_node_by_id (graph, ptr->idNodeTo);
	    }
      }
    else
      {
	  if (graph->NodeCode)
	    {
		/* nodes are identified by TEXT codes */
		if (end_node)
		    node = find_node_by_code (graph, ptr->codNodeTo);
		else
		    node = find_node_by_code (graph, ptr->codNodeFrom);
	    }
	  else
	    {
		/* nodes are identified by integer IDs */
		if (end_node)
		    node = find_node_by_id (graph, ptr->idNodeTo);
		else
		    node = find_node_by_id (graph, ptr->idNodeFrom);
	    }
      }
    if (node == NULL)
	return 0.0;

    return sqrt (((node->CoordX - xFrom) * (node->CoordX - xFrom)) +
		 ((node->CoordY - yFrom) * (node->CoordY - yFrom)));
}

static int
build_ingress_path (virtualroutingPtr net, double xFrom, double yFrom,
		    Point2PointCandidatePtr ptr, int srid)
{
/* Point2Point - attempting to build an Ingress Path */
    RoutingPtr graph = net->graph;
    char *sql;
    char *xtable;
    char *xgeom;
    sqlite3 *sqlite = net->db;
    sqlite3_stmt *stmt = NULL;
    int ret;
    int ok = 0;
    double percent;
    double length;
    gaiaGeomCollPtr geom = NULL;
    int is_geographic = 0;

    if (!srid_is_geographic (sqlite, srid, &is_geographic))
	return 0;

/* locating the insertion point */
    xtable = gaiaDoubleQuotedSql (graph->TableName);
    xgeom = gaiaDoubleQuotedSql (graph->GeometryColumn);
    if (ptr->reverse)
	sql =
	    sqlite3_mprintf ("SELECT ST_Line_Locate_Point(ST_Reverse(\"%s\"), "
			     "MakePoint(?, ?)) FROM \"%s\" WHERE rowid = ?",
			     xgeom, xtable);
    else
	sql =
	    sqlite3_mprintf ("SELECT ST_Line_Locate_Point(\"%s\", "
			     "MakePoint(?, ?)) FROM \"%s\" WHERE rowid = ?",
			     xgeom, xtable);
    free (xgeom);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  return 0;
      }

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, xFrom);
    sqlite3_bind_double (stmt, 2, yFrom);
    sqlite3_bind_int64 (stmt, 3, ptr->linkRowid);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		percent = sqlite3_column_double (stmt, 0);
		ok = 1;
	    }
      }
    sqlite3_finalize (stmt);
    if (!ok)
	return 0;
    if (percent <= 0.0)
      {
	  /* special case: the insertion point is the Start Node */
	  ptr->valid = 1;
	  ptr->extraLen = doComputeExtraLength (net, xFrom, yFrom, ptr, 0);
	  return 1;
      }
    if (percent >= 1.0)
      {
	  /* special case: the insertion point is the End Node */
	  ptr->valid = 1;
	  ptr->extraLen = doComputeExtraLength (net, xFrom, yFrom, ptr, 1);
	  return 1;
      }

/* determining the ingress path */
    xtable = gaiaDoubleQuotedSql (graph->TableName);
    xgeom = gaiaDoubleQuotedSql (graph->GeometryColumn);
    if (is_geographic)
      {
	  if (ptr->reverse)
	      sql =
		  sqlite3_mprintf ("SELECT g.geom, ST_Length(g.geom, 1) FROM "
				   "(SELECT ST_Line_Substring(ST_Reverse(\"%s\"), ?, 100.0) AS geom "
				   "FROM \"%s\" WHERE rowid = ?) AS g", xgeom,
				   xtable);
	  else
	      sql =
		  sqlite3_mprintf ("SELECT g.geom, ST_Length(g.geom, 1) FROM "
				   "(SELECT ST_Line_Substring(\"%s\", ?, 100.0) AS geom "
				   "FROM \"%s\" WHERE rowid = ?) AS g", xgeom,
				   xtable);
      }
    else
      {
	  if (ptr->reverse)
	      sql =
		  sqlite3_mprintf ("SELECT g.geom, ST_Length(g.geom) FROM "
				   "(SELECT ST_Line_Substring(ST_Reverse(\"%s\"), ?, 100.0) AS geom "
				   "FROM \"%s\" WHERE rowid = ?) AS g", xgeom,
				   xtable);
	  else
	      sql =
		  sqlite3_mprintf ("SELECT g.geom, ST_Length(g.geom) FROM "
				   "(SELECT ST_Line_Substring(\"%s\", ?, 100.0) AS geom "
				   "FROM \"%s\" WHERE rowid = ?) AS g", xgeom,
				   xtable);
      }
    free (xgeom);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  return 0;
      }

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, percent);
    sqlite3_bind_int64 (stmt, 2, ptr->linkRowid);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		      int size = sqlite3_column_bytes (stmt, 0);
		      geom = gaiaFromSpatiaLiteBlobWkb (blob, size);
		      if (geom != NULL)
			{
			    gaiaLinestringPtr ln = geom->FirstLinestring;
			    double x;
			    double y;
			    double z;
			    double m;
			    if (ln->DimensionModel == GAIA_XY_Z)
			      {
				  gaiaGetPointXYZ (ln->Coords, 0, &x, &y, &z);
			      }
			    else if (ln->DimensionModel == GAIA_XY_M)
			      {
				  gaiaGetPointXYM (ln->Coords, 0, &x, &y, &m);
			      }
			    else if (ln->DimensionModel == GAIA_XY_Z_M)
			      {
				  gaiaGetPointXYZM (ln->Coords, 0, &x, &y, &z,
						    &m);
			      }
			    else
			      {
				  gaiaGetPoint (ln->Coords, 0, &x, &y);
			      }
			    length = sqlite3_column_double (stmt, 1);
			    ptr->path = geom;
			    ptr->pathLen = length;
			    if (x == xFrom && y == yFrom)
				;
			    else
			      {
				  length =
				      sqrt (((x - xFrom) * (x - xFrom)) +
					    ((y - yFrom) * (y - yFrom)));
				  ptr->extraLen = length;
			      }
			    ptr->valid = 1;
			}
		  }
	    }
      }
    sqlite3_finalize (stmt);

    return 1;
}

static int
build_egress_path (virtualroutingPtr net, double xTo, double yTo,
		   Point2PointCandidatePtr ptr, int srid)
{
/* Point2Point - attempting to build an Egress Path */
    RoutingPtr graph = net->graph;
    char *sql;
    char *xtable;
    char *xgeom;
    sqlite3 *sqlite = net->db;
    sqlite3_stmt *stmt = NULL;
    int ret;
    int ok = 0;
    double percent;
    double length;
    gaiaGeomCollPtr geom = NULL;
    int is_geographic = 0;

    if (!srid_is_geographic (sqlite, srid, &is_geographic))
	return 0;

/* locating the insertion point */
    xtable = gaiaDoubleQuotedSql (graph->TableName);
    xgeom = gaiaDoubleQuotedSql (graph->GeometryColumn);
    if (ptr->reverse)
	sql =
	    sqlite3_mprintf ("SELECT ST_Line_Locate_Point(ST_Reverse(\"%s\"), "
			     "MakePoint(?, ?)) FROM \"%s\" WHERE rowid = ?",
			     xgeom, xtable);
    else
	sql =
	    sqlite3_mprintf ("SELECT ST_Line_Locate_Point(\"%s\", "
			     "MakePoint(?, ?)) FROM \"%s\" WHERE rowid = ?",
			     xgeom, xtable);
    free (xgeom);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, xTo);
    sqlite3_bind_double (stmt, 2, yTo);
    sqlite3_bind_int64 (stmt, 3, ptr->linkRowid);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		percent = sqlite3_column_double (stmt, 0);
		ok = 1;
	    }
      }
    sqlite3_finalize (stmt);
    if (!ok)
	return 0;
    if (percent >= 1.0)
      {
	  /* special case: the insertion point is the Start Node */
	  ptr->valid = 1;
	  ptr->extraLen = doComputeExtraLength (net, xTo, yTo, ptr, 0);
	  return 1;
      }
    if (percent >= 1.0)
      {
	  /* special case: the insertion point is the End Node */
	  ptr->valid = 1;
	  ptr->extraLen = doComputeExtraLength (net, xTo, yTo, ptr, 1);
	  return 1;
      }

/* determining the egress path */
    xtable = gaiaDoubleQuotedSql (graph->TableName);
    xgeom = gaiaDoubleQuotedSql (graph->GeometryColumn);
    if (is_geographic)
      {
	  if (ptr->reverse)
	      sql =
		  sqlite3_mprintf ("SELECT g.geom, ST_Length(g.geom, 1) FROM "
				   "(SELECT ST_Line_Substring(ST_Reverse(\"%s\"), 0.0, ?) AS geom "
				   "FROM \"%s\" WHERE rowid = ?) AS g", xgeom,
				   xtable);
	  else
	      sql =
		  sqlite3_mprintf ("SELECT g.geom, ST_Length(g.geom, 1) FROM "
				   "(SELECT ST_Line_Substring(\"%s\", 0.0, ?) AS geom "
				   "FROM \"%s\" WHERE rowid = ?) AS g", xgeom,
				   xtable);
      }
    else
      {
	  if (ptr->reverse)
	      sql =
		  sqlite3_mprintf ("SELECT g.geom, ST_Length(g.geom) FROM "
				   "(SELECT ST_Line_Substring(ST_Reverse(\"%s\"), 0.0, ?) AS geom "
				   "FROM \"%s\" WHERE rowid = ?) AS g", xgeom,
				   xtable);
	  else
	      sql =
		  sqlite3_mprintf ("SELECT g.geom, ST_Length(g.geom) FROM "
				   "(SELECT ST_Line_Substring(\"%s\", 0.0, ?) AS geom "
				   "FROM \"%s\" WHERE rowid = ?) AS g", xgeom,
				   xtable);
      }
    free (xgeom);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, percent);
    sqlite3_bind_int64 (stmt, 2, ptr->linkRowid);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		      int size = sqlite3_column_bytes (stmt, 0);
		      geom = gaiaFromSpatiaLiteBlobWkb (blob, size);
		      if (geom != NULL)
			{
			    gaiaLinestringPtr ln = geom->FirstLinestring;
			    double x;
			    double y;
			    double z;
			    double m;
			    int last_pt = ln->Points - 1;
			    if (ln->DimensionModel == GAIA_XY_Z)
			      {
				  gaiaGetPointXYZ (ln->Coords, last_pt, &x, &y,
						   &z);
			      }
			    else if (ln->DimensionModel == GAIA_XY_M)
			      {
				  gaiaGetPointXYM (ln->Coords, last_pt, &x, &y,
						   &m);
			      }
			    else if (ln->DimensionModel == GAIA_XY_Z_M)
			      {
				  gaiaGetPointXYZM (ln->Coords, last_pt, &x, &y,
						    &z, &m);
			      }
			    else
			      {
				  gaiaGetPoint (ln->Coords, last_pt, &x, &y);
			      }
			    length = sqlite3_column_double (stmt, 1);
			    ptr->path = geom;
			    ptr->pathLen = length;
			    if (x == xTo && y == yTo)
				;
			    else
			      {
				  length =
				      sqrt (((x - xTo) * (x - xTo)) +
					    ((y - yTo) * (y - yTo)));
				  ptr->extraLen = length;
			      }
			    ptr->valid = 1;
			}
		  }
	    }
      }
    sqlite3_finalize (stmt);

    return 1;
}

static int
do_define_ingress_paths (virtualroutingPtr net)
{
/* Point2Point - defining all Ingress Paths */
    Point2PointSolutionPtr p2p = net->point2PointSolution;
    Point2PointCandidatePtr ptr;

    ptr = p2p->firstFromCandidate;
    while (ptr != NULL)
      {
	  if (!build_ingress_path
	      (net, p2p->xFrom, p2p->yFrom, ptr, net->graph->Srid))
	      return 0;
	  ptr = ptr->next;
      }
    return 1;
}

static int
do_define_egress_paths (virtualroutingPtr net)
{
/* Point2Point - defining all Egress Paths */
    Point2PointSolutionPtr p2p = net->point2PointSolution;
    Point2PointCandidatePtr ptr;

    ptr = p2p->firstToCandidate;
    while (ptr != NULL)
      {
	  if (!build_egress_path
	      (net, p2p->xTo, p2p->yTo, ptr, net->graph->Srid))
	      return 0;
	  ptr = ptr->next;
      }
    return 1;
}

static void
add_point2point_node_from_by_code (Point2PointSolutionPtr p2p,
				   const char *node,
				   Point2PointCandidatePtr parent)
{
/* adding to the Point2Point FromNodes list (if not already defined) */
    int len;
    Point2PointNodePtr p = p2p->firstFromNode;
    while (p != NULL)
      {
	  if (strcmp (p->codNode, node) == 0)
	    {
		/* already defined */
		return;
	    }
	  p = p->next;
      }

/* adding the Node */
    p = malloc (sizeof (Point2PointNode));
    len = strlen (node);
    p->codNode = malloc (len + 1);
    strcpy (p->codNode, node);
    p->parent = parent;
    p->next = NULL;
    if (p2p->firstFromNode == NULL)
	p2p->firstFromNode = p;
    if (p2p->lastFromNode != NULL)
	p2p->lastFromNode->next = p;
    p2p->lastFromNode = p;
}

static void
add_point2point_node_from_by_id (Point2PointSolutionPtr p2p, sqlite3_int64 id,
				 Point2PointCandidatePtr parent)
{
/* adding to the Point2Point FromNodes list (if not already defined) */
    Point2PointNodePtr p = p2p->firstFromNode;
    while (p != NULL)
      {
	  if (p->idNode == id)
	    {
		/* already defined */
		return;
	    }
	  p = p->next;
      }

/* adding the Node */
    p = malloc (sizeof (Point2PointNode));
    p->idNode = id;
    p->codNode = NULL;
    p->parent = parent;
    p->next = NULL;
    if (p2p->firstFromNode == NULL)
	p2p->firstFromNode = p;
    if (p2p->lastFromNode != NULL)
	p2p->lastFromNode->next = p;
    p2p->lastFromNode = p;
}

static void
add_point2point_node_to_by_code (Point2PointSolutionPtr p2p, const char *node,
				 Point2PointCandidatePtr parent)
{
/* adding to the Point2Point ToNodes list (if not already defined) */
    int len;
    Point2PointNodePtr p = p2p->firstToNode;
    while (p != NULL)
      {
	  if (strcmp (p->codNode, node) == 0)
	    {
		/* already defined */
		return;
	    }
	  p = p->next;
      }

/* adding the Node */
    p = malloc (sizeof (Point2PointNode));
    len = strlen (node);
    p->codNode = malloc (len + 1);
    strcpy (p->codNode, node);
    p->parent = parent;
    p->next = NULL;
    if (p2p->firstToNode == NULL)
	p2p->firstToNode = p;
    if (p2p->lastToNode != NULL)
	p2p->lastToNode->next = p;
    p2p->lastToNode = p;
}

static void
add_point2point_node_to_by_id (Point2PointSolutionPtr p2p, sqlite3_int64 id,
			       Point2PointCandidatePtr parent)
{
/* adding to the Point2Point ToNodes list (if not already defined) */
    Point2PointNodePtr p = p2p->firstToNode;
    while (p != NULL)
      {
	  if (p->idNode == id)
	    {
		/* already defined */
		return;
	    }
	  p = p->next;
      }

/* adding the Node */
    p = malloc (sizeof (Point2PointNode));
    p->idNode = id;
    p->codNode = NULL;
    p->parent = parent;
    p->next = NULL;
    if (p2p->firstToNode == NULL)
	p2p->firstToNode = p;
    if (p2p->lastToNode != NULL)
	p2p->lastToNode->next = p;
    p2p->lastToNode = p;
}

static void
point2point_eval_solution (Point2PointSolutionPtr p2p,
			   ShortestPathSolutionPtr solution, int nodeCode)
{
/* attempting to identify the optimal Point2Point solution */
    Point2PointCandidatePtr p_from = p2p->firstFromCandidate;
    while (p_from != NULL)
      {
	  /* searching FROM candidates */
	  int ok = 0;
	  if (nodeCode)
	    {
		if (strcmp (solution->From->Code, p_from->codNodeTo) == 0)
		    ok = 1;
	    }
	  else
	    {
		if (solution->From->Id == p_from->idNodeTo)
		    ok = 1;
	    }
	  if (ok)
	    {
		Point2PointCandidatePtr p_to = p2p->firstToCandidate;
		while (p_to != NULL)
		  {
		      /* searching TO candidates */
		      int ok2 = 0;
		      if (nodeCode)
			{
			    if (strcmp (solution->To->Code, p_to->codNodeFrom)
				== 0)
				ok2 = 1;
			}
		      else
			{
			    if (solution->To->Id == p_to->idNodeFrom)
				ok2 = 1;
			}
		      if (ok2)
			{
			    double tot =
				solution->TotalCost + p_from->pathLen +
				p_from->extraLen + p_to->pathLen +
				p_to->extraLen;
			    if (tot < p2p->totalCost)
			      {
				  /* saving a better solution */
				  p2p->totalCost = tot;
				  p2p->fromCandidate = p_from;
				  p2p->toCandidate = p_to;
				  return;
			      }
			}
		      p_to = p_to->next;
		  }
	    }
	  p_from = p_from->next;
      }
}

static RouteLinkPtr
find_link (sqlite3 * sqlite, RoutingPtr graph, sqlite3_int64 linkRowid)
{
/* attempting to create a partial Link object */
    int ret;
    char *sql;
    char *xfrom;
    char *xto;
    char *xtable;
    sqlite3_stmt *stmt = NULL;
    RouteLinkPtr ptr = NULL;

    xfrom = gaiaDoubleQuotedSql (graph->FromColumn);
    xto = gaiaDoubleQuotedSql (graph->ToColumn);
    xtable = gaiaDoubleQuotedSql (graph->TableName);
    sql =
	sqlite3_mprintf ("SELECT \"%s\", \"%s\" FROM \"%s\" WHERE ROWID = ?",
			 xfrom, xto, xtable);
    free (xfrom);
    free (xto);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto stop;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, linkRowid);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		RouteNodePtr from = NULL;
		RouteNodePtr to = NULL;
		if (graph->NodeCode)
		  {
		      /* nodes are identified by TEXT codes */
		      char *code;
		      if (sqlite3_column_type (stmt, 0) == SQLITE_TEXT)
			{
			    code = (char *) sqlite3_column_text (stmt, 0);
			    from = find_node_by_code (graph, code);
			}
		      if (sqlite3_column_type (stmt, 1) == SQLITE_TEXT)
			{
			    code = (char *) sqlite3_column_text (stmt, 1);
			    to = find_node_by_code (graph, code);
			}
		  }
		else
		  {
		      sqlite3_int64 id;
		      /* nodes are identified by INTEGER ids */
		      if (sqlite3_column_type (stmt, 0) == SQLITE_INTEGER)
			{
			    id = sqlite3_column_int64 (stmt, 0);
			    from = find_node_by_id (graph, id);
			}
		      if (sqlite3_column_type (stmt, 1) == SQLITE_INTEGER)
			{
			    id = sqlite3_column_int64 (stmt, 1);
			    to = find_node_by_id (graph, id);
			}
		  }
		if (from != NULL && to != NULL)
		  {
		      if (ptr != NULL)
			  free (ptr);
		      ptr = malloc (sizeof (RouteLink));
		      ptr->NodeFrom = from;
		      ptr->NodeTo = to;
		      ptr->LinkRowid = linkRowid;
		      ptr->Cost = 0.0;
		  }
	    }
      }
    sqlite3_finalize (stmt);
    return ptr;

  stop:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return ptr;
}

static void
add2DynLine (gaiaDynamicLinePtr dyn, gaiaGeomCollPtr geom, int reverse,
	     double extra, double cost)
{
/* adding points from Linestring to Dynamic line */
    int iv;
    double startCost;
    double endCost;
    gaiaGeomCollPtr geo2;

    if (geom == NULL)
	return;

    /* interpolating M-Values */
    if (dyn->Last == NULL)
	startCost = 0.0;
    else
	startCost = dyn->Last->M + extra;
    endCost = startCost + cost;
    if (reverse)
	geo2 = gaiaAddMeasure (geom, endCost, startCost);
    else
	geo2 = gaiaAddMeasure (geom, startCost, endCost);

    gaiaLinestringPtr line = geo2->FirstLinestring;
    if (reverse)
      {
	  /* reverse direction */
	  for (iv = line->Points - 1; iv >= 0; iv--)
	      addPoint2DynLine
		  (line->Coords, line->DimensionModel, iv, dyn, 0.0);
      }
    else
      {
	  /* natural direction */
	  for (iv = 0; iv < line->Points; iv++)
	      addPoint2DynLine
		  (line->Coords, line->DimensionModel, iv, dyn, 0.0);
      }
    gaiaFreeGeomColl (geo2);
}

static void
build_point2point_solution (sqlite3 * sqlite, int options, RoutingPtr graph,
			    Point2PointSolutionPtr p2p)
{
/* fully completing a Point2Point Solution */
    ResultsetRowPtr row;
    int error = 0;

    p2p->hasZ = graph->HasZ;

    if (graph->GeometryColumn == NULL && graph->NameColumn == NULL)
	return;
    if (options == VROUTE_SHORTEST_PATH_NO_GEOMS && graph->NameColumn == NULL)
	return;

    if (options == VROUTE_SHORTEST_PATH_NO_GEOMS
	|| options == VROUTE_SHORTEST_PATH_SIMPLE
	|| graph->GeometryColumn == NULL)
	;
    else
      {
	  /* preparing the Dynamic Line - result path */
	  p2p->dynLine = gaiaAllocDynamicLine ();
      }

    row = p2p->FirstRow;
    while (row != NULL)
      {
	  /* looping on rows */
	  char *sql;
	  char *xfrom;
	  char *xto;
	  char *xname;
	  char *xgeom;
	  char *xtable;
	  int no_geom = 0;
	  int ask_geom = 1;
	  int ask_name = 1;
	  int ret;
	  sqlite3_stmt *stmt = NULL;

	  if (options == VROUTE_SHORTEST_PATH_NO_LINKS)
	      ask_name = 0;
	  if (graph->NameColumn == NULL)
	      ask_name = 0;
	  if (row->Point2PointRole == VROUTE_POINT2POINT_START
	      || row->Point2PointRole == VROUTE_POINT2POINT_END)
	      ask_geom = 0;
	  if (options == VROUTE_SHORTEST_PATH_NO_GEOMS
	      || graph->GeometryColumn == NULL)
	    {
		no_geom = 1;
		ask_geom = 0;
	    }
	  if (options == VROUTE_SHORTEST_PATH_SIMPLE)
	    {
		no_geom = 1;
		ask_geom = 0;
		ask_name = 0;
	    }

	  if (!no_geom)
	    {
		if (row->Point2PointRole == VROUTE_POINT2POINT_INGRESS)
		    gaiaAppendPointZMToDynamicLine (p2p->dynLine, p2p->xFrom,
						    p2p->yFrom, p2p->zFrom,
						    0.0);
		if (row->Point2PointRole == VROUTE_POINT2POINT_START)
		    add2DynLine
			(p2p->dynLine, p2p->fromCandidate->path, 0,
			 p2p->fromCandidate->extraLen,
			 p2p->fromCandidate->pathLen);
		if (row->Point2PointRole == VROUTE_POINT2POINT_END)
		    add2DynLine
			(p2p->dynLine, p2p->toCandidate->path, 0, 0.0,
			 p2p->toCandidate->pathLen);
		if (row->Point2PointRole == VROUTE_POINT2POINT_EGRESS)
		    gaiaAppendPointZMToDynamicLine (p2p->dynLine, p2p->xTo,
						    p2p->yTo, p2p->zTo,
						    p2p->totalCost);
	    }
	  if (row->linkRef == NULL)
	      goto next_row;

	  if (ask_geom && ask_name)
	    {
		xname = gaiaDoubleQuotedSql (graph->NameColumn);
		xfrom = gaiaDoubleQuotedSql (graph->FromColumn);
		xto = gaiaDoubleQuotedSql (graph->ToColumn);
		xgeom = gaiaDoubleQuotedSql (graph->GeometryColumn);
		xtable = gaiaDoubleQuotedSql (graph->TableName);
		sql =
		    sqlite3_mprintf
		    ("SELECT \"%s\", \"%s\", \"%s\", \"%s\" FROM \"%s\" WHERE ROWID = ?",
		     xname, xfrom, xto, xgeom, xtable);
		free (xname);
		free (xfrom);
		free (xto);
		free (xgeom);
		free (xtable);
	    }
	  else if (ask_geom)
	    {
		xfrom = gaiaDoubleQuotedSql (graph->FromColumn);
		xto = gaiaDoubleQuotedSql (graph->ToColumn);
		xgeom = gaiaDoubleQuotedSql (graph->GeometryColumn);
		xtable = gaiaDoubleQuotedSql (graph->TableName);
		sql =
		    sqlite3_mprintf
		    ("SELECT \"%s\", \"%s\", \"%s\" FROM \"%s\" WHERE ROWID = ?",
		     xfrom, xto, xgeom, xtable);
		free (xfrom);
		free (xto);
		free (xgeom);
		free (xtable);
	    }
	  else if (ask_name)
	    {
		xname = gaiaDoubleQuotedSql (graph->NameColumn);
		xtable = gaiaDoubleQuotedSql (graph->TableName);
		sql =
		    sqlite3_mprintf
		    ("SELECT \"%s\" FROM \"%s\" WHERE ROWID = ?", xname,
		     xtable);
		free (xname);
		free (xtable);
	    }
	  else
	      goto next_row;
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	      goto next_row;

	  /* querying the resultset */
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_int64 (stmt, 1, row->linkRef->Link->LinkRowid);
	  while (1)
	    {
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		    break;
		if (ret == SQLITE_ROW)
		  {
		      const char *from_code;
		      const char *to_code;
		      sqlite3_int64 from_id;
		      sqlite3_int64 to_id;
		      const unsigned char *blob = NULL;
		      int blob_sz;
		      int err = 0;

		      if (ask_name)
			{
			    /* the Name column */
			    char *name;
			    const char *xname =
				(const char *) sqlite3_column_text (stmt, 0);
			    int len = sqlite3_column_bytes (stmt, 0);
			    name = malloc (len + 1);
			    strcpy (name, xname);
			    row->linkRef->Name = name;
			    if (ask_geom)
			      {
				  if (graph->NodeCode)
				    {
					/* nodes are identified by TEXT codes */
					if (sqlite3_column_type (stmt, 1) ==
					    SQLITE_TEXT)
					    from_code =
						(const char *)
						sqlite3_column_text (stmt, 1);
					else
					    err = 1;
					if (sqlite3_column_type (stmt, 2) ==
					    SQLITE_TEXT)
					    to_code =
						(const char *)
						sqlite3_column_text (stmt, 2);
					else
					    err = 1;
				    }
				  else
				    {
					/* nodes are identified by INTEGER ids */
					if (sqlite3_column_type (stmt, 1) ==
					    SQLITE_INTEGER)
					    from_id =
						sqlite3_column_int64 (stmt, 1);
					else
					    err = 1;
					if (sqlite3_column_type (stmt, 2) ==
					    SQLITE_INTEGER)
					    to_id =
						sqlite3_column_int64 (stmt, 2);
					else
					    err = 1;
				    }
				  if (sqlite3_column_type (stmt, 3) ==
				      SQLITE_BLOB)
				    {
					blob =
					    (const unsigned char *)
					    sqlite3_column_blob (stmt, 3);
					blob_sz =
					    sqlite3_column_bytes (stmt, 3);
				    }
			      }
			}
		      else if (ask_geom)
			{
			    if (graph->NodeCode)
			      {
				  /* nodes are identified by TEXT codes */
				  if (sqlite3_column_type (stmt, 0) ==
				      SQLITE_TEXT)
				      from_code =
					  (const char *)
					  sqlite3_column_text (stmt, 0);
				  else
				      err = 1;
				  if (sqlite3_column_type (stmt, 1) ==
				      SQLITE_TEXT)
				      to_code =
					  (const char *)
					  sqlite3_column_text (stmt, 1);
				  else
				      err = 1;
			      }
			    else
			      {
				  /* nodes are identified by INTEGER ids */
				  if (sqlite3_column_type (stmt, 0) ==
				      SQLITE_INTEGER)
				      from_id = sqlite3_column_int64 (stmt, 0);
				  else
				      err = 1;
				  if (sqlite3_column_type (stmt, 1) ==
				      SQLITE_INTEGER)
				      to_id = sqlite3_column_int64 (stmt, 1);
				  else
				      err = 1;
			      }
			    if (sqlite3_column_type (stmt, 2) == SQLITE_BLOB)
			      {
				  blob =
				      (const unsigned char *)
				      sqlite3_column_blob (stmt, 2);
				  blob_sz = sqlite3_column_bytes (stmt, 2);
			      }
			}
		      if (ask_geom)
			{
			    if (blob != NULL && !err)
			      {
				  /* adding the Link's geometry to the Solution geometry */
				  gaiaGeomCollPtr geom =
				      gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
				  if (geom)
				    {
					/* OK, we have fetched a valid Geometry */
					if (geom->FirstPoint == NULL
					    && geom->FirstPolygon == NULL
					    && geom->FirstLinestring != NULL
					    && geom->FirstLinestring ==
					    geom->LastLinestring)
					  {
					      /* Geometry is LINESTRING as expected */
					      int reverse = 1;
					      if (graph->NodeCode)
						{
						    /* nodes are identified by TEXT codes */
						    const char *from =
							row->linkRef->
							Link->NodeFrom->Code;
						    const char *to =
							row->linkRef->
							Link->NodeTo->Code;
						    if (strcmp (from_code, from)
							== 0
							&& strcmp (to_code,
								   to) == 0)
							reverse = 0;
						}
					      else
						{
						    /* nodes are identified by INTEGER ids */
						    sqlite3_int64 from =
							row->linkRef->
							Link->NodeFrom->Id;
						    sqlite3_int64 to =
							row->linkRef->
							Link->NodeTo->Id;
						    if (from_id == from
							&& to_id == to)
							reverse = 0;
						}
					      add2DynLine
						  (p2p->dynLine, geom, reverse,
						   0.0,
						   row->linkRef->Link->Cost);
					  }
					else
					    error = 1;
					gaiaFreeGeomColl (geom);
				    }
				  else
				      error = 1;
			      }
			    else
				error = 1;
			}
		  }
		else
		    error = 1;
	    }

	next_row:
	  if (stmt != NULL)
	      sqlite3_finalize (stmt);
	  row = row->Next;
      }

    if (error)
      {
	  if (p2p->dynLine != NULL)
	      gaiaFreeDynamicLine (p2p->dynLine);
	  p2p->dynLine = NULL;
      }

    if (p2p->dynLine != NULL)
      {
	  /* building the result path Geometry */
	  gaiaGeomCollPtr geom;
	  gaiaLinestringPtr ln;
	  gaiaPointPtr pt;
	  int points = 0;
	  int iv;

	  row = p2p->FirstRow;
	  if (row->Geometry != NULL)
	      gaiaFreeGeomColl (row->Geometry);
	  if (p2p->hasZ)
	      geom = gaiaAllocGeomCollXYZM ();
	  else
	      geom = gaiaAllocGeomCollXYM ();
	  geom->Srid = graph->Srid;
	  pt = p2p->dynLine->First;
	  while (pt != NULL)
	    {
		/* counting how many points are there */
		points++;
		pt = pt->Next;
	    }
	  ln = gaiaAddLinestringToGeomColl (geom, points);
	  iv = 0;
	  pt = p2p->dynLine->First;
	  while (pt != NULL)
	    {
		/* copying points from Dynamic Line to Linestring */
		if (p2p->hasZ)
		  {
		      gaiaSetPointXYZM (ln->Coords, iv, pt->X, pt->Y, pt->Z,
					pt->M);
		  }
		else
		  {
		      gaiaSetPointXYM (ln->Coords, iv, pt->X, pt->Y, pt->M);
		  }
		iv++;
		pt = pt->Next;
	    }
	  row->Geometry = geom;

	  gaiaFreeDynamicLine (p2p->dynLine);
	  p2p->dynLine = NULL;
      }

    if (options == VROUTE_SHORTEST_PATH_NO_LINKS
	|| options == VROUTE_SHORTEST_PATH_SIMPLE)
      {
	  /* deleting Links */
	  ResultsetRowPtr pR;
	  ResultsetRowPtr pRn;
	  pR = p2p->FirstRow;
	  while (pR != NULL)
	    {
		pRn = pR->Next;
		if (pR == p2p->FirstRow)
		  {
		      /* skipping the first row */
		      pR->Next = NULL;
		      pR = pRn;
		      continue;
		  }
		if (pR->Point2PointRole == VROUTE_POINT2POINT_START
		    || pR->Point2PointRole == VROUTE_POINT2POINT_END)
		  {
		      /* deleting partial Links */
		      if (pR->linkRef != NULL)
			{
			    if (pR->linkRef->Link != NULL)
				free (pR->linkRef->Link);
			    if (pR->linkRef->Name != NULL)
				free (pR->linkRef->Name);
			    free (pR->linkRef);
			}
		  }
		if (pR->Geometry != NULL)
		    gaiaFreeGeomColl (pR->Geometry);
		free (pR);
		pR = pRn;
	    }
	  p2p->LastRow = p2p->FirstRow;
      }
}

static void
point2point_resolve (virtualroutingCursorPtr cursor)
{
/* resolving a Point2Point Shortest Path */
    virtualroutingPtr net = (virtualroutingPtr) cursor->pVtab;
    RoutingPtr graph = cursor->pVtab->graph;
    RoutingMultiDestPtr multiple = NULL;
    Point2PointNodePtr p_node_from;
    Point2PointNodePtr p_node_to;
    ShortestPathSolutionPtr solution;
    Point2PointSolutionPtr p2p = cursor->pVtab->point2PointSolution;
    Point2PointCandidatePtr p_from = p2p->firstFromCandidate;
    Point2PointCandidatePtr p_to = p2p->firstToCandidate;
    int route_row;
    RowSolutionPtr pA;
    ResultsetRowPtr row;
    RouteLinkPtr link;
    while (p_from != NULL)
      {
	  /* extracting all FROM candidates */
	  if (graph->NodeCode)
	      add_point2point_node_from_by_code (p2p, p_from->codNodeTo,
						 p_from);
	  else
	      add_point2point_node_from_by_id (p2p, p_from->idNodeTo, p_from);
	  p_from = p_from->next;
      }
    while (p_to != NULL)
      {
	  /* extracting all TO candidates */
	  if (graph->NodeCode)
	      add_point2point_node_to_by_code (p2p, p_to->codNodeFrom, p_to);
	  else
	      add_point2point_node_to_by_id (p2p, p_to->idNodeFrom, p_to);
	  p_to = p_to->next;
      }
    p_node_from = p2p->firstFromNode;
    while (p_node_from != NULL)
      {
	  /* exploring all FROM Nodes */
	  int i;
	  int items = 0;
	  RoutingMultiDestPtr multiple;
	  reset_multiSolution (cursor->pVtab->multiSolution);
	  cursor->pVtab->multiSolution->Mode = VROUTE_ROUTING_SOLUTION;
	  p_node_to = p2p->firstToNode;
	  while (p_node_to != NULL)
	    {
		/* counting how many destinations */
		items++;
		p_node_to = p_node_to->next;
	    }
	  /* allocating the helper struct */
	  multiple = malloc (sizeof (RoutingMultiDest));
	  cursor->pVtab->multiSolution->MultiTo = multiple;
	  multiple->CodeNode = graph->NodeCode;
	  multiple->Found = malloc (sizeof (char) * items);
	  multiple->To = malloc (sizeof (RouteNodePtr) * items);
	  for (i = 0; i < items; i++)
	    {
		*(multiple->Found + i) = 'N';
		*(multiple->To + i) = NULL;
	    }
	  multiple->Items = items;
	  multiple->Next = 0;
	  if (multiple->CodeNode)
	    {
		multiple->Ids = NULL;
		multiple->Codes = malloc (sizeof (char *) * items);
	    }
	  else
	    {
		multiple->Ids = malloc (sizeof (sqlite3_int64) * items);
		multiple->Codes = NULL;
	    }
	  if (graph->NodeCode)
	      cursor->pVtab->multiSolution->From =
		  find_node_by_code (graph, p_node_from->codNode);
	  else
	      cursor->pVtab->multiSolution->From =
		  find_node_by_id (graph, p_node_from->idNode);
	  p_node_to = p2p->firstToNode;
	  while (p_node_to != NULL)
	    {
		/* exploring all TO Nodes */
		if (graph->NodeCode)
		  {
		      int l = strlen (p_node_to->codNode);
		      char *code = malloc (l + 1);
		      strcpy (code, p_node_to->codNode);
		      vroute_add_multiple_code (multiple, code);
		  }
		else
		    vroute_add_multiple_id (multiple, p_node_to->idNode);
		p_node_to = p_node_to->next;
	    }
	  if (graph->NodeCode)
	      set_multi_by_code (multiple, graph);
	  else
	      set_multi_by_id (multiple, graph);
/* always using Dijktra's */
	  dijkstra_multi_solve (cursor->pVtab->db,
				VROUTE_SHORTEST_PATH_SIMPLE, graph,
				cursor->pVtab->routing,
				cursor->pVtab->multiSolution);
	  solution = cursor->pVtab->multiSolution->First;
	  while (solution != NULL)
	    {
		point2point_eval_solution (p2p, solution, graph->NodeCode);
		solution = solution->Next;
	    }
	  p_node_from = p_node_from->next;
      }

    if (p2p->fromCandidate == NULL || p2p->toCandidate == NULL)
	return;			/* invalid solution */

/* fully building the optimal Point2Point solution */
    reset_multiSolution (cursor->pVtab->multiSolution);
    cursor->pVtab->multiSolution->Mode = VROUTE_ROUTING_SOLUTION;
    if (graph->NodeCode)
	cursor->pVtab->multiSolution->From =
	    find_node_by_code (graph, p2p->fromCandidate->codNodeTo);
    else
	cursor->pVtab->multiSolution->From =
	    find_node_by_id (graph, p2p->fromCandidate->idNodeTo);
/* allocating the helper struct */
    multiple = malloc (sizeof (RoutingMultiDest));
    cursor->pVtab->multiSolution->MultiTo = multiple;
    multiple->CodeNode = graph->NodeCode;
    multiple->Found = malloc (sizeof (char));
    multiple->To = malloc (sizeof (RouteNodePtr));
    *(multiple->Found + 0) = 'N';
    *(multiple->To + 0) = NULL;
    multiple->Items = 1;
    multiple->Next = 0;
    if (multiple->CodeNode)
      {
	  int l = strlen (p2p->toCandidate->codNodeFrom);
	  multiple->Ids = NULL;
	  multiple->Codes = malloc (sizeof (char *));
	  char *code = malloc (l + 1);
	  strcpy (code, p2p->toCandidate->codNodeFrom);
	  vroute_add_multiple_code (multiple, code);
	  set_multi_by_code (multiple, graph);
      }
    else
      {
	  multiple->Ids = malloc (sizeof (sqlite3_int64));
	  multiple->Codes = NULL;
	  vroute_add_multiple_id (multiple, p2p->toCandidate->idNodeFrom);
	  set_multi_by_id (multiple, graph);
      }
    if (net->currentAlgorithm == VROUTE_A_STAR_ALGORITHM)
	astar_solve (cursor->pVtab->db, VROUTE_SHORTEST_PATH_QUICK, graph,
		     cursor->pVtab->routing, cursor->pVtab->multiSolution);
    else
	dijkstra_multi_solve (cursor->pVtab->db, VROUTE_SHORTEST_PATH_QUICK,
			      graph, cursor->pVtab->routing,
			      cursor->pVtab->multiSolution);
    solution = cursor->pVtab->multiSolution->First;

/* building the solution rows */
    p2p->totalCost =
	solution->TotalCost + p2p->fromCandidate->pathLen +
	p2p->fromCandidate->extraLen + p2p->toCandidate->pathLen +
	p2p->toCandidate->extraLen;
/* inserting the Route Header */
    route_row = 0;
    row = malloc (sizeof (ResultsetRow));
    row->RouteNum = 0;
    row->RouteRow = route_row++;
    row->Point2PointRole = VROUTE_POINT2POINT_NONE;
    row->From = NULL;
    row->To = NULL;
    row->Undefined = solution->Undefined;
    solution->Undefined = NULL;
    row->UndefinedId = solution->UndefinedId;
    row->linkRef = NULL;
    row->TotalCost = p2p->totalCost;
    row->Geometry = NULL;
    row->Next = NULL;
    if (p2p->FirstRow == NULL)
	p2p->FirstRow = row;
    if (p2p->LastRow != NULL)
	p2p->LastRow->Next = row;
    p2p->LastRow = row;
    if (p2p->fromCandidate->extraLen > 0.0)
      {
	  /* inserting the Route Ingress */
	  row = malloc (sizeof (ResultsetRow));
	  row->RouteNum = 0;
	  row->RouteRow = route_row++;
	  row->Point2PointRole = VROUTE_POINT2POINT_INGRESS;
	  row->From = NULL;
	  row->To = NULL;
	  row->Undefined = solution->Undefined;
	  solution->Undefined = NULL;
	  row->UndefinedId = solution->UndefinedId;
	  row->linkRef = NULL;
	  row->TotalCost = p2p->fromCandidate->extraLen;
	  row->Geometry = NULL;
	  row->Next = NULL;
	  if (p2p->FirstRow == NULL)
	      p2p->FirstRow = row;
	  if (p2p->LastRow != NULL)
	      p2p->LastRow->Next = row;
	  p2p->LastRow = row;
      }
    if (p2p->fromCandidate->pathLen > 0.0)
      {
	  /* inserting the Start partial Link */
	  row = malloc (sizeof (ResultsetRow));
	  row->RouteNum = 0;
	  row->RouteRow = route_row++;
	  row->Point2PointRole = VROUTE_POINT2POINT_START;
	  row->From = NULL;
	  row->To = NULL;
	  row->Undefined = solution->Undefined;
	  solution->Undefined = NULL;
	  row->UndefinedId = solution->UndefinedId;
	  /* creating a RowSolution object */
	  link =
	      find_link (cursor->pVtab->db, cursor->pVtab->graph,
			 p2p->fromCandidate->linkRowid);
	  row->linkRef = malloc (sizeof (RowSolution));
	  row->linkRef->Link = link;
	  row->linkRef->Name = NULL;
	  row->linkRef->Next = NULL;
	  row->TotalCost = p2p->fromCandidate->pathLen;
	  row->Geometry = NULL;
	  row->Next = NULL;
	  if (p2p->FirstRow == NULL)
	      p2p->FirstRow = row;
	  if (p2p->LastRow != NULL)
	      p2p->LastRow->Next = row;
	  p2p->LastRow = row;
      }

    pA = solution->First;
    while (pA != NULL)
      {
	  /* inserting Route's traversed Links */
	  row = malloc (sizeof (ResultsetRow));
	  row->RouteNum = 0;
	  row->RouteRow = route_row++;
	  row->Point2PointRole = VROUTE_POINT2POINT_NONE;
	  row->From = NULL;
	  row->To = NULL;
	  row->Undefined = NULL;
	  row->linkRef = pA;
	  row->TotalCost = 0.0;
	  row->Geometry = NULL;
	  row->Next = NULL;
	  if (p2p->FirstRow == NULL)
	      p2p->FirstRow = row;
	  if (p2p->LastRow != NULL)
	      p2p->LastRow->Next = row;
	  p2p->LastRow = row;
	  pA = pA->Next;
      }

    if (p2p->toCandidate->pathLen > 0.0)
      {
	  /* inserting the End partial Link */
	  row = malloc (sizeof (ResultsetRow));
	  row->RouteNum = 0;
	  row->RouteRow = route_row++;
	  row->Point2PointRole = VROUTE_POINT2POINT_END;
	  row->From = NULL;
	  row->To = NULL;
	  row->Undefined = solution->Undefined;
	  solution->Undefined = NULL;
	  row->UndefinedId = solution->UndefinedId;
	  /* creating a RowSolution object */
	  link =
	      find_link (cursor->pVtab->db, cursor->pVtab->graph,
			 p2p->toCandidate->linkRowid);
	  row->linkRef = malloc (sizeof (RowSolution));
	  row->linkRef->Link = link;
	  row->linkRef->Name = NULL;
	  row->linkRef->Next = NULL;
	  row->TotalCost = p2p->toCandidate->pathLen;
	  row->Geometry = NULL;
	  row->Next = NULL;
	  if (p2p->FirstRow == NULL)
	      p2p->FirstRow = row;
	  if (p2p->LastRow != NULL)
	      p2p->LastRow->Next = row;
	  p2p->LastRow = row;
      }
    if (p2p->toCandidate->extraLen > 0.0)
      {
	  /* inserting the Route Egress */
	  row = malloc (sizeof (ResultsetRow));
	  row->RouteNum = 0;
	  row->RouteRow = route_row++;
	  row->Point2PointRole = VROUTE_POINT2POINT_EGRESS;
	  row->From = NULL;
	  row->To = NULL;
	  row->Undefined = solution->Undefined;
	  solution->Undefined = NULL;
	  row->UndefinedId = solution->UndefinedId;
	  row->linkRef = NULL;
	  row->TotalCost = p2p->toCandidate->extraLen;
	  row->Geometry = NULL;
	  row->Next = NULL;
	  if (p2p->FirstRow == NULL)
	      p2p->FirstRow = row;
	  if (p2p->LastRow != NULL)
	      p2p->LastRow->Next = row;
	  p2p->LastRow = row;
      }

    build_point2point_solution (cursor->pVtab->db,
				cursor->pVtab->currentOptions,
				cursor->pVtab->graph, p2p);
    cursor->pVtab->multiSolution->Mode = VROUTE_POINT2POINT_SOLUTION;
}

static int
vroute_create (sqlite3 * db, void *pAux, int argc, const char *const *argv,
	       sqlite3_vtab ** ppVTab, char **pzErr)
{
/* creates the virtual table connected to some shapefile */
    virtualroutingPtr p_vt;
    int err;
    int ret;
    int i;
    int n_rows;
    int n_columns;
    char *vtable = NULL;
    char *table = NULL;
    const char *col_name = NULL;
    char **results;
    char *err_msg = NULL;
    char *sql;
    int ok_id;
    int ok_data;
    char *xname;
    RoutingPtr graph = NULL;
    if (pAux)
	pAux = pAux;		/* unused arg warning suppression */
/* checking for table_name and geo_column_name */
    if (argc == 4)
      {
	  vtable = gaiaDequotedSql (argv[2]);
	  table = gaiaDequotedSql (argv[3]);
      }
    else
      {
	  *pzErr =
	      sqlite3_mprintf
	      ("[virtualrouting module] CREATE VIRTUAL: illegal arg list {NETWORK-DATAtable}\n");
	  goto error;
      }
/* retrieving the base table columns */
    err = 0;
    ok_id = 0;
    ok_data = 0;
    xname = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("PRAGMA table_info(\"%s\")", xname);
    free (xname);
    ret = sqlite3_get_table (db, sql, &results, &n_rows, &n_columns, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  err = 1;
	  goto illegal;
      }
    if (n_rows > 1)
      {
	  for (i = 1; i <= n_rows; i++)
	    {
		col_name = results[(i * n_columns) + 1];
		if (strcasecmp (col_name, "id") == 0)
		    ok_id = 1;
		if (strcasecmp (col_name, "networkdata") == 0)
		    ok_data = 1;
	    }
	  sqlite3_free_table (results);
	  if (!ok_id)
	      err = 1;
	  if (!ok_data)
	      err = 1;
      }
    else
	err = 1;
  illegal:
    if (err)
      {
	  /* something is going the wrong way */
	  *pzErr =
	      sqlite3_mprintf
	      ("[virtualrouting module] cannot build a valid NETWORK\n");
	  return SQLITE_ERROR;
      }
    p_vt = (virtualroutingPtr) sqlite3_malloc (sizeof (virtualrouting));
    if (!p_vt)
	return SQLITE_NOMEM;
    graph = load_network (db, table);
    if (!graph)
      {
	  /* something is going the wrong way */
	  *pzErr =
	      sqlite3_mprintf
	      ("[virtualrouting module] cannot build a valid NETWORK\n");
	  goto error;
      }
    p_vt->db = db;
    p_vt->graph = graph;
    p_vt->currentAlgorithm = VROUTE_DIJKSTRA_ALGORITHM;
    p_vt->currentRequest = VROUTE_SHORTEST_PATH;
    p_vt->currentOptions = VROUTE_SHORTEST_PATH_FULL;
    p_vt->currentDelimiter = ',';
    p_vt->Tolerance = 20.0;
    p_vt->routing = NULL;
    p_vt->pModule = &my_route_module;
    p_vt->nRef = 0;
    p_vt->zErrMsg = NULL;
/* preparing the COLUMNs for this VIRTUAL TABLE */
    xname = gaiaDoubleQuotedSql (vtable);
    if (p_vt->graph->NodeCode)
      {
	  if (p_vt->graph->NameColumn)
	    {
		sql =
		    sqlite3_mprintf ("CREATE TABLE \"%s\" (Algorithm TEXT, "
				     "Request TEXT, Options TEXT, Delimiter TEXT, "
				     "RouteId INTEGER, RouteRow INTEGER, Role TEXT, "
				     "LinkRowid INTEGER, NodeFrom TEXT, NodeTo TEXT,"
				     "PointFrom BLOB, PointTo BLOB, Tolerance DOUBLE, "
				     "Cost DOUBLE, Geometry BLOB, Name TEXT)",
				     xname);
	    }
	  else
	    {
		sql =
		    sqlite3_mprintf ("CREATE TABLE \"%s\" (Algorithm TEXT, "
				     "Request TEXT, Options TEXT, Delimiter TEXT, "
				     "RouteId INTEGER, RouteRow INTEGER, Role TEXT, "
				     "LinkRowid INTEGER, NodeFrom TEXT, NodeTo TEXT,"
				     "PointFrom BLOB, PointTo BLOB, Tolerance DOUBLE, "
				     "Cost DOUBLE, Geometry BLOB)", xname);
	    }
      }
    else
      {
	  if (p_vt->graph->NameColumn)
	    {
		sql =
		    sqlite3_mprintf ("CREATE TABLE \"%s\" (Algorithm TEXT, "
				     "Request TEXT, Options TEXT, Delimiter TEXT, "
				     "RouteId INTEGER, RouteRow INTEGER, Role TEXT, "
				     "LinkRowid INTEGER, NodeFrom INTEGER, NodeTo INTEGER, "
				     "PointFrom BLOB, PointTo BLOB, Tolerance Double, "
				     "Cost DOUBLE, Geometry BLOB, Name TEXT)",
				     xname);
	    }
	  else
	    {
		sql =
		    sqlite3_mprintf ("CREATE TABLE \"%s\" (Algorithm TEXT, "
				     "Request TEXT, Options TEXT, Delimiter TEXT, "
				     "RouteId INTEGER, RouteRow INTEGER, Role TEXT, "
				     "LinkRowid INTEGER, NodeFrom INTEGER, NodeTo INTEGER, "
				     "PointFrom BLOB, PointTo BLOB, Tolerance DOUBLE, "
				     "Cost DOUBLE, Geometry BLOB)", xname);
	    }
      }
    free (xname);
    if (sqlite3_declare_vtab (db, sql) != SQLITE_OK)
      {
	  *pzErr =
	      sqlite3_mprintf
	      ("[virtualrouting module] CREATE VIRTUAL: invalid SQL statement \"%s\"",
	       sql);
	  sqlite3_free (sql);
	  goto error;
      }
    sqlite3_free (sql);
    *ppVTab = (sqlite3_vtab *) p_vt;
    p_vt->routing = routing_init (p_vt->graph);
    free (table);
    free (vtable);
    return SQLITE_OK;
  error:
    if (table)
	free (table);
    if (vtable)
	free (vtable);
    return SQLITE_ERROR;
}

static int
vroute_connect (sqlite3 * db, void *pAux, int argc, const char *const *argv,
		sqlite3_vtab ** ppVTab, char **pzErr)
{
/* connects the virtual table to some shapefile - simply aliases vshp_create() */
    return vroute_create (db, pAux, argc, argv, ppVTab, pzErr);
}

static int
vroute_best_index (sqlite3_vtab * pVTab, sqlite3_index_info * pIdxInfo)
{
/* best index selection */
    int i;
    int errors = 0;
    int err = 1;
    int from = 0;
    int to = 0;
    int fromPoint = 0;
    int toPoint = 0;
    int cost = 0;
    int i_from = -1;
    int i_to = -1;
    int i_fromPoint = -1;
    int i_toPoint = -1;
    int i_cost = -1;
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    for (i = 0; i < pIdxInfo->nConstraint; i++)
      {
	  /* verifying the constraints */
	  struct sqlite3_index_constraint *p = &(pIdxInfo->aConstraint[i]);
	  if (p->usable)
	    {
		if (p->iColumn == 8 && p->op == SQLITE_INDEX_CONSTRAINT_EQ)
		  {
		      from++;
		      i_from = i;
		  }
		else if (p->iColumn == 9 && p->op == SQLITE_INDEX_CONSTRAINT_EQ)
		  {
		      to++;
		      i_to = i;
		  }
		else if (p->iColumn == 10
			 && p->op == SQLITE_INDEX_CONSTRAINT_EQ)
		  {
		      fromPoint++;
		      i_fromPoint = i;
		  }
		else if (p->iColumn == 11
			 && p->op == SQLITE_INDEX_CONSTRAINT_EQ)
		  {
		      toPoint++;
		      i_toPoint = i;
		  }
		else if (p->iColumn == 13
			 && p->op == SQLITE_INDEX_CONSTRAINT_LE)
		  {
		      cost++;
		      i_cost = i;
		  }
		else
		    errors++;
	    }
      }
    if (from == 1 && to == 1 && errors == 0)
      {
	  /* this one is a valid Shortest Path query */
	  if (i_from < i_to)
	      pIdxInfo->idxNum = 1;	/* first arg is FROM */
	  else
	      pIdxInfo->idxNum = 2;	/* first arg is TO */
	  pIdxInfo->estimatedCost = 1.0;
	  for (i = 0; i < pIdxInfo->nConstraint; i++)
	    {
		if (pIdxInfo->aConstraint[i].usable)
		  {
		      pIdxInfo->aConstraintUsage[i].argvIndex = i + 1;
		      pIdxInfo->aConstraintUsage[i].omit = 1;
		  }
	    }
	  err = 0;
      }
    if (fromPoint == 1 && toPoint == 1 && errors == 0)
      {
	  /* this one is a valid Shortest Path [Point2Point] query */
	  if (i_fromPoint < i_toPoint)
	      pIdxInfo->idxNum = 5;	/* first arg is FROM */
	  else
	      pIdxInfo->idxNum = 6;	/* first arg is TO */
	  pIdxInfo->estimatedCost = 1.0;
	  for (i = 0; i < pIdxInfo->nConstraint; i++)
	    {
		if (pIdxInfo->aConstraint[i].usable)
		  {
		      pIdxInfo->aConstraintUsage[i].argvIndex = i + 1;
		      pIdxInfo->aConstraintUsage[i].omit = 1;
		  }
	    }
	  err = 0;
      }
    if (from == 1 && cost == 1 && errors == 0)
      {
	  /* this one is a valid "within cost" query */
	  if (i_from < i_cost)
	      pIdxInfo->idxNum = 3;	/* first arg is FROM */
	  else
	      pIdxInfo->idxNum = 4;	/* first arg is COST */
	  pIdxInfo->estimatedCost = 1.0;
	  for (i = 0; i < pIdxInfo->nConstraint; i++)
	    {
		if (pIdxInfo->aConstraint[i].usable)
		  {
		      pIdxInfo->aConstraintUsage[i].argvIndex = i + 1;
		      pIdxInfo->aConstraintUsage[i].omit = 1;
		  }
	    }
	  err = 0;
      }
    if (err)
      {
	  /* illegal query */
	  pIdxInfo->idxNum = 0;
      }
    return SQLITE_OK;
}

static int
vroute_disconnect (sqlite3_vtab * pVTab)
{
/* disconnects the virtual table */
    virtualroutingPtr p_vt = (virtualroutingPtr) pVTab;
    if (p_vt->routing)
	routing_free (p_vt->routing);
    if (p_vt->graph)
	network_free (p_vt->graph);
    sqlite3_free (p_vt);
    return SQLITE_OK;
}

static int
vroute_destroy (sqlite3_vtab * pVTab)
{
/* destroys the virtual table - simply aliases vshp_disconnect() */
    return vroute_disconnect (pVTab);
}

static void
vroute_read_row (virtualroutingCursorPtr cursor)
{
/* trying to read a "row" from Shortest Path solution */
    if (cursor->pVtab->multiSolution->Mode == VROUTE_RANGE_SOLUTION)
      {
	  if (cursor->pVtab->multiSolution->CurrentNodeRow == NULL)
	      cursor->pVtab->eof = 1;
	  else
	      cursor->pVtab->eof = 0;
      }
    else
      {
	  if (cursor->pVtab->multiSolution->CurrentRow == NULL)
	      cursor->pVtab->eof = 1;
	  else
	      cursor->pVtab->eof = 0;
      }
    return;
}

static void
vroute_read_row_p2p (virtualroutingCursorPtr cursor)
{
/* trying to read a "row" from Point2Point solution */
    if (cursor->pVtab->point2PointSolution->CurrentRow == NULL)
	cursor->pVtab->eof = 1;
    else
	cursor->pVtab->eof = 0;
    return;
}

static int
vroute_open (sqlite3_vtab * pVTab, sqlite3_vtab_cursor ** ppCursor)
{
/* opening a new cursor */
    virtualroutingCursorPtr cursor =
	(virtualroutingCursorPtr)
	sqlite3_malloc (sizeof (virtualroutingCursor));
    if (cursor == NULL)
	return SQLITE_ERROR;
    cursor->pVtab = (virtualroutingPtr) pVTab;
    cursor->pVtab->multiSolution = alloc_multiSolution ();
    cursor->pVtab->point2PointSolution = alloc_point2PointSolution ();
    cursor->pVtab->eof = 0;
    *ppCursor = (sqlite3_vtab_cursor *) cursor;
    return SQLITE_OK;
}

static int
vroute_close (sqlite3_vtab_cursor * pCursor)
{
/* closing the cursor */
    virtualroutingCursorPtr cursor = (virtualroutingCursorPtr) pCursor;
    delete_multiSolution (cursor->pVtab->multiSolution);
    delete_point2PointSolution (cursor->pVtab->point2PointSolution);
    sqlite3_free (pCursor);
    return SQLITE_OK;
}

static int
vroute_filter (sqlite3_vtab_cursor * pCursor, int idxNum, const char *idxStr,
	       int argc, sqlite3_value ** argv)
{
/* setting up a cursor filter */
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt;
    unsigned char *p_blob = NULL;
    int n_bytes = 0;
    int node_code = 0;
    virtualroutingCursorPtr cursor = (virtualroutingCursorPtr) pCursor;
    virtualroutingPtr net = (virtualroutingPtr) cursor->pVtab;
    RoutingMultiDestPtr multiple = NULL;
    MultiSolutionPtr multiSolution = cursor->pVtab->multiSolution;
    Point2PointSolutionPtr p2p = cursor->pVtab->point2PointSolution;
    if (idxStr)
	idxStr = idxStr;	/* unused arg warning suppression */
    node_code = net->graph->NodeCode;
    reset_multiSolution (multiSolution);
    reset_point2PointSolution (p2p);
    cursor->pVtab->eof = 0;
    if (idxNum == 1 && argc == 2)
      {
	  /* retrieving the Shortest Path From/To params */
	  if (node_code)
	    {
		/* Nodes are identified by TEXT Codes */
		if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
		    multiSolution->From =
			find_node_by_code (net->graph,
					   (char *)
					   sqlite3_value_text (argv[0]));
		if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
		  {
		      multiple =
			  vroute_get_multiple_destinations (1,
							    cursor->
							    pVtab->currentDelimiter,
							    (const char *)
							    sqlite3_value_text
							    (argv[1]));
		      if (multiple != NULL)
			{
			    set_multi_by_code (multiple, net->graph);
			    multiSolution->MultiTo = multiple;
			}
		  }
	    }
	  else
	    {
		/* Nodes are identified by INT Ids */
		if (sqlite3_value_type (argv[0]) == SQLITE_INTEGER)
		    multiSolution->From =
			find_node_by_id (net->graph,
					 sqlite3_value_int (argv[0]));
		if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
		  {
		      multiple =
			  vroute_get_multiple_destinations (0,
							    cursor->
							    pVtab->currentDelimiter,
							    (const char *)
							    sqlite3_value_text
							    (argv[1]));
		      if (multiple != NULL)
			{
			    set_multi_by_id (multiple, net->graph);
			    multiSolution->MultiTo = multiple;
			}
		  }
		else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
		  {
		      multiple =
			  vroute_as_multiple_destinations (sqlite3_value_int
							   (argv[1]));
		      if (multiple != NULL)
			{
			    set_multi_by_id (multiple, net->graph);
			    multiSolution->MultiTo = multiple;
			}
		  }
	    }
      }
    if (idxNum == 2 && argc == 2)
      {
	  /* retrieving the Shortest Path To/From params */
	  if (node_code)
	    {
		/* Nodes are identified by TEXT Codes */
		if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
		  {
		      multiple =
			  vroute_get_multiple_destinations (1,
							    cursor->
							    pVtab->currentDelimiter,
							    (const char *)
							    sqlite3_value_text
							    (argv[0]));
		      if (multiple != NULL)
			{
			    set_multi_by_code (multiple, net->graph);
			    multiSolution->MultiTo = multiple;
			}
		  }
		if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
		  {
		      multiSolution->From =
			  find_node_by_code (net->graph,
					     (char *)
					     sqlite3_value_text (argv[1]));
		  }
	    }
	  else
	    {
		/* Nodes are identified by INT Ids */
		if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
		  {
		      multiple =
			  vroute_get_multiple_destinations (0,
							    cursor->
							    pVtab->currentDelimiter,
							    (const char *)
							    sqlite3_value_text
							    (argv[0]));
		      if (multiple != NULL)
			{
			    set_multi_by_id (multiple, net->graph);
			    multiSolution->MultiTo = multiple;
			}
		  }
		else if (sqlite3_value_type (argv[0]) == SQLITE_INTEGER)
		  {
		      multiple =
			  vroute_as_multiple_destinations (sqlite3_value_int
							   (argv[0]));
		      if (multiple != NULL)
			{
			    set_multi_by_id (multiple, net->graph);
			    multiSolution->MultiTo = multiple;
			}
		  }
		if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
		    multiSolution->From =
			find_node_by_id (net->graph,
					 sqlite3_value_int (argv[1]));
	    }
      }
    if (idxNum == 3 && argc == 2)
      {
	  /* retrieving the From and Cost param */
	  if (node_code)
	    {
		/* Nodes are identified by TEXT Codes */
		if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
		    multiSolution->From =
			find_node_by_code (net->graph,
					   (char *)
					   sqlite3_value_text (argv[0]));
	    }
	  else
	    {
		/* Nodes are identified by INT Ids */
		if (sqlite3_value_type (argv[0]) == SQLITE_INTEGER)
		    multiSolution->From =
			find_node_by_id (net->graph,
					 sqlite3_value_int (argv[0]));
	    }
	  if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	    {
		int cost = sqlite3_value_int (argv[1]);
		multiSolution->MaxCost = cost;
	    }
	  else if (sqlite3_value_type (argv[1]) == SQLITE_FLOAT)
	      multiSolution->MaxCost = sqlite3_value_double (argv[1]);
      }
    if (idxNum == 4 && argc == 2)
      {
	  /* retrieving the From and Cost param */
	  if (node_code)
	    {
		/* Nodes are identified by TEXT Codes */
		if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
		    multiSolution->From =
			find_node_by_code (net->graph,
					   (char *)
					   sqlite3_value_text (argv[1]));
	    }
	  else
	    {
		/* Nodes are identified by INT Ids */
		if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
		    multiSolution->From =
			find_node_by_id (net->graph,
					 sqlite3_value_int (argv[1]));
	    }
	  if (sqlite3_value_type (argv[0]) == SQLITE_INTEGER)
	    {
		int cost = sqlite3_value_int (argv[0]);
		multiSolution->MaxCost = cost;
	    }
	  else if (sqlite3_value_type (argv[0]) == SQLITE_FLOAT)
	      multiSolution->MaxCost = sqlite3_value_double (argv[0]);
      }
    if (idxNum == 5 && argc == 2)
      {
	  /* retrieving the Shortest Path FromPoint / ToPoint params */
	  if (net->graph->GeometryColumn == NULL)
	      ;			/* skipping Point2Point if No-Geometry */
	  else
	    {
		if (sqlite3_value_type (argv[0]) == SQLITE_BLOB)
		  {
		      p_blob = (unsigned char *) sqlite3_value_blob (argv[0]);
		      n_bytes = sqlite3_value_bytes (argv[0]);
		      point = gaiaFromSpatiaLiteBlobWkb (p_blob, n_bytes);
		      if (point != NULL)
			{
			    if (do_check_valid_point (point, net->graph->Srid))
			      {
				  /* ok, it's a valid Point */
				  pt = point->FirstPoint;
				  p2p->validFrom = 1;
				  p2p->xFrom = pt->X;
				  p2p->yFrom = pt->Y;
				  if (point->DimensionModel == GAIA_XY_Z
				      || point->DimensionModel == GAIA_XY_Z_M)
				      p2p->zFrom = pt->Z;
				  else
				      p2p->zFrom = 0.0;
			      }
			    gaiaFreeGeomColl (point);
			}
		  }
		if (sqlite3_value_type (argv[1]) == SQLITE_BLOB)
		  {
		      p_blob = (unsigned char *) sqlite3_value_blob (argv[1]);
		      n_bytes = sqlite3_value_bytes (argv[1]);
		      point = gaiaFromSpatiaLiteBlobWkb (p_blob, n_bytes);
		      if (point != NULL)
			{
			    if (do_check_valid_point (point, net->graph->Srid))
			      {
				  /* ok, it's a valid Point */
				  pt = point->FirstPoint;
				  p2p->validTo = 1;
				  p2p->xTo = pt->X;
				  p2p->yTo = pt->Y;
				  if (point->DimensionModel == GAIA_XY_Z
				      || point->DimensionModel == GAIA_XY_Z_M)
				      p2p->zTo = pt->Z;
				  else
				      p2p->zTo = 0.0;
			      }
			    gaiaFreeGeomColl (point);
			}
		  }
	    }
      }
    if (idxNum == 6 && argc == 2)
      {
	  /* retrieving the Shortest Path ToPoint / FromPoint [Point2Point] params */
	  if (net->graph->GeometryColumn == NULL)
	      ;			/* skipping Point2Point if No-Geometry */
	  else
	    {
		if (sqlite3_value_type (argv[0]) == SQLITE_BLOB)
		  {
		      p_blob = (unsigned char *) sqlite3_value_blob (argv[0]);
		      n_bytes = sqlite3_value_bytes (argv[0]);
		      point = gaiaFromSpatiaLiteBlobWkb (p_blob, n_bytes);
		      if (point != NULL)
			{
			    if (do_check_valid_point (point, net->graph->Srid))
			      {
				  /* ok, it's a valid Point */
				  pt = point->FirstPoint;
				  p2p->validTo = 1;
				  p2p->xTo = pt->X;
				  p2p->yTo = pt->Y;
				  if (point->DimensionModel == GAIA_XY_Z
				      || point->DimensionModel == GAIA_XY_Z_M)
				      p2p->zFrom = pt->Z;
				  else
				      p2p->zFrom = 0.0;
			      }
			    gaiaFreeGeomColl (point);
			}
		  }
		if (sqlite3_value_type (argv[1]) == SQLITE_BLOB)
		  {
		      p_blob = (unsigned char *) sqlite3_value_blob (argv[1]);
		      n_bytes = sqlite3_value_bytes (argv[1]);
		      point = gaiaFromSpatiaLiteBlobWkb (p_blob, n_bytes);
		      if (point != NULL)
			{
			    if (do_check_valid_point (point, net->graph->Srid))
			      {
				  /* ok, it's a valid Point */
				  pt = point->FirstPoint;
				  p2p->validFrom = 1;
				  p2p->xFrom = pt->X;
				  p2p->yFrom = pt->Y;
				  if (point->DimensionModel == GAIA_XY_Z
				      || point->DimensionModel == GAIA_XY_Z_M)
				      p2p->zTo = pt->Z;
				  else
				      p2p->zTo = 0.0;
			      }
			    gaiaFreeGeomColl (point);
			}
		  }
	    }
      }
    if (p2p->validFrom && p2p->validTo)
      {
	  /* preparing a Point2Point request */
	  cursor->pVtab->eof = 0;
	  p2p->CurrentRow = NULL;
	  p2p->CurrentRowId = 0;
	  p2p->Mode = VROUTE_POINT2POINT_SOLUTION;
	  /* searching candidates links (From) */
	  if (!do_prepare_point (cursor->pVtab, VROUTE_POINT2POINT_FROM))
	      p2p->Mode = VROUTE_POINT2POINT_ERROR;
	  else
	    {
		/* searching candidates links (To) */
		if (!do_prepare_point (cursor->pVtab, VROUTE_POINT2POINT_TO))
		    p2p->Mode = VROUTE_POINT2POINT_ERROR;
		else
		  {
		      /* defining Ingress paths */
		      if (!do_define_ingress_paths (cursor->pVtab))
			  p2p->Mode = VROUTE_POINT2POINT_ERROR;
		      else
			{
			    /* defining Egress paths */
			    if (!do_define_egress_paths (cursor->pVtab))
				p2p->Mode = VROUTE_POINT2POINT_ERROR;
			}
		  }
	    }
	  if (p2p->Mode == VROUTE_POINT2POINT_SOLUTION)
	    {
		point2point_resolve (cursor);
		p2p->CurrentRowId = 0;
		p2p->CurrentRow = p2p->FirstRow;
	    }
	  return SQLITE_OK;
      }
    if (multiSolution == NULL)
      {
	  cursor->pVtab->eof = 0;
	  return SQLITE_OK;
      }
    if (multiSolution->From && multiSolution->MultiTo)
      {
	  cursor->pVtab->eof = 0;
	  if (net->currentRequest == VROUTE_TSP_NN)
	    {
		multiSolution->Mode = VROUTE_TSP_SOLUTION;
		if (net->currentAlgorithm == VROUTE_DIJKSTRA_ALGORITHM)
		  {
		      tsp_nn_solve (net->db, net->currentOptions, net->graph,
				    net->routing, multiSolution);
		      multiSolution->CurrentRowId = 0;
		      multiSolution->CurrentRow = multiSolution->FirstRow;
		  }
	    }
	  else if (net->currentRequest == VROUTE_TSP_GA)
	    {
		multiSolution->Mode = VROUTE_TSP_SOLUTION;
		if (net->currentAlgorithm == VROUTE_DIJKSTRA_ALGORITHM)
		  {
		      tsp_ga_solve (net->db, net->currentOptions, net->graph,
				    net->routing, multiSolution);
		      multiSolution->CurrentRowId = 0;
		      multiSolution->CurrentRow = multiSolution->FirstRow;
		  }
	    }
	  else
	    {
		multiSolution->Mode = VROUTE_ROUTING_SOLUTION;
		if (net->currentAlgorithm == VROUTE_A_STAR_ALGORITHM)
		  {
		      if (multiSolution->MultiTo->Items > 1)
			{
			    /* multiple destinations: always defaulting to Dijkstra */
			    dijkstra_multi_solve (net->db, net->currentOptions,
						  net->graph, net->routing,
						  multiSolution);
			}
		      else
			  astar_solve (net->db, net->currentOptions, net->graph,
				       net->routing, multiSolution);
		  }
		else
		    dijkstra_multi_solve (net->db, net->currentOptions,
					  net->graph, net->routing,
					  multiSolution);
		multiSolution->CurrentRowId = 0;
		multiSolution->CurrentRow = multiSolution->FirstRow;
	    }
	  return SQLITE_OK;
      }
    if (multiSolution->From && multiSolution->MaxCost > 0.0)
      {
	  find_srid (net->db, net->graph);
	  cursor->pVtab->eof = 0;
	  multiSolution->Mode = VROUTE_RANGE_SOLUTION;
	  /* always defaulting to Dijkstra's Shortest Path */
	  dijkstra_within_cost_range (net->routing, multiSolution,
				      net->graph->Srid);
	  multiSolution->CurrentRowId = 0;
	  multiSolution->CurrentNodeRow = multiSolution->FirstNode;
	  return SQLITE_OK;
      }
    cursor->pVtab->eof = 0;
    multiSolution->CurrentRow = NULL;
    multiSolution->CurrentRowId = 0;
    multiSolution->Mode = VROUTE_ROUTING_SOLUTION;
    return SQLITE_OK;
}

static int
vroute_next (sqlite3_vtab_cursor * pCursor)
{
/* fetching a next row from cursor */
    virtualroutingCursorPtr cursor = (virtualroutingCursorPtr) pCursor;
    MultiSolutionPtr multiSolution = cursor->pVtab->multiSolution;
    Point2PointSolutionPtr p2p = cursor->pVtab->point2PointSolution;
    if (p2p != NULL)
      {
	  if (p2p->Mode == VROUTE_POINT2POINT_SOLUTION)
	    {
		if (p2p->CurrentRow == NULL)
		  {
		      cursor->pVtab->eof = 1;
		      return SQLITE_OK;
		  }
		p2p->CurrentRow = p2p->CurrentRow->Next;
		if (!(p2p->CurrentRow))
		  {
		      cursor->pVtab->eof = 1;
		      return SQLITE_OK;
		  }
		(p2p->CurrentRowId)++;
		vroute_read_row_p2p (cursor);
		return SQLITE_OK;
	    }
      }
    if (multiSolution->Mode == VROUTE_RANGE_SOLUTION)
      {
	  if (multiSolution->CurrentNodeRow == NULL)
	    {
		cursor->pVtab->eof = 1;
		return SQLITE_OK;
	    }
	  multiSolution->CurrentNodeRow = multiSolution->CurrentNodeRow->Next;
	  if (!(multiSolution->CurrentNodeRow))
	    {
		cursor->pVtab->eof = 1;
		return SQLITE_OK;
	    }
      }
    else
      {
	  if (multiSolution->CurrentRow == NULL)
	    {
		cursor->pVtab->eof = 1;
		return SQLITE_OK;
	    }
	  multiSolution->CurrentRow = multiSolution->CurrentRow->Next;
	  if (!(multiSolution->CurrentRow))
	    {
		cursor->pVtab->eof = 1;
		return SQLITE_OK;
	    }
      }
    (multiSolution->CurrentRowId)++;
    vroute_read_row (cursor);
    return SQLITE_OK;
}

static int
vroute_eof (sqlite3_vtab_cursor * pCursor)
{
/* cursor EOF */
    virtualroutingCursorPtr cursor = (virtualroutingCursorPtr) pCursor;
    return cursor->pVtab->eof;
}

static void
do_cost_range_column (virtualroutingCursorPtr cursor,
		      sqlite3_context * pContext, int node_code,
		      RowNodeSolutionPtr row_node, int column)
{
/* processing a "within Cost range" solution row */
    const char *algorithm;
    char delimiter[128];
    const char *role;
    RowNodeSolutionPtr first = cursor->pVtab->multiSolution->FirstNode;

    if (column == 0)
      {
	  /* the currently used Algorithm */
	  algorithm = "Dijkstra";
	  if (row_node != first)
	      sqlite3_result_null (pContext);
	  else
	      sqlite3_result_text (pContext, algorithm, strlen (algorithm),
				   SQLITE_TRANSIENT);
      }
    if (column == 1)
      {
	  /* the current Request type */
	  algorithm = "Isochrone";
	  if (row_node != first)
	      sqlite3_result_null (pContext);
	  else
	      sqlite3_result_text (pContext, algorithm, strlen (algorithm),
				   SQLITE_TRANSIENT);
      }
    if (column == 2)
      {
	  /* the currently set Options */
	  algorithm = "Full";
	  if (row_node != first)
	      sqlite3_result_null (pContext);
	  else
	      sqlite3_result_text (pContext, algorithm, strlen (algorithm),
				   SQLITE_TRANSIENT);
      }
    if (column == 3)
      {
	  /* the currently set delimiter char */
	  if (isprint (cursor->pVtab->currentDelimiter))
	      sprintf (delimiter, "%c [dec=%d, hex=%02x]",
		       cursor->pVtab->currentDelimiter,
		       cursor->pVtab->currentDelimiter,
		       cursor->pVtab->currentDelimiter);
	  else
	      sprintf (delimiter, "[dec=%d, hex=%02x]",
		       cursor->pVtab->currentDelimiter,
		       cursor->pVtab->currentDelimiter);
	  if (row_node != first)
	      sqlite3_result_null (pContext);
	  else
	      sqlite3_result_text (pContext, delimiter, strlen (delimiter),
				   SQLITE_TRANSIENT);
      }
    if (column == 4)
      {
	  /* the RouteNum column */
	  sqlite3_result_null (pContext);
      }
    if (column == 5)
      {
	  /* the RouteRow column */
	  sqlite3_result_null (pContext);
      }
    if (column == 6)
      {
	  /* role of this row */
	  role = "Solution";
	  sqlite3_result_text (pContext, role, strlen (role), SQLITE_TRANSIENT);
      }
    if (column == 7)
      {
	  /* the LinkRowId column */
	  sqlite3_result_null (pContext);
      }
    if (column == 8)
      {
	  /* the NodeFrom column */
	  if (node_code)
	      sqlite3_result_text (pContext,
				   cursor->pVtab->multiSolution->From->Code,
				   strlen (cursor->pVtab->multiSolution->
					   From->Code), SQLITE_STATIC);
	  else
	      sqlite3_result_int64 (pContext,
				    cursor->pVtab->multiSolution->From->Id);
      }
    if (column == 9)
      {
	  /* the NodeTo column */
	  if (row_node == NULL)
	      sqlite3_result_null (pContext);
	  else
	    {
		if (node_code)
		    sqlite3_result_text (pContext, row_node->Node->Code,
					 strlen (row_node->Node->Code),
					 SQLITE_STATIC);
		else
		    sqlite3_result_int64 (pContext, row_node->Node->Id);
	    }
      }
    if (column == 10)
      {
	  /* the PointFrom column */
	  sqlite3_result_null (pContext);
      }
    if (column == 11)
      {
	  /* the PointTo column */
	  sqlite3_result_null (pContext);
      }
    if (column == 12)
      {
	  /* the Tolerance column */
	  sqlite3_result_null (pContext);
      }
    if (column == 13)
      {
	  /* the Cost column */
	  if (row_node == NULL)
	      sqlite3_result_null (pContext);
	  else
	      sqlite3_result_double (pContext, row_node->Cost);
      }
    if (column == 14)
      {
	  /* the Geometry column */
	  if (row_node == NULL)
	      sqlite3_result_null (pContext);
	  else
	    {
		if (row_node->Srid == VROUTE_INVALID_SRID)
		    sqlite3_result_null (pContext);
		else
		  {
		      int len;
		      unsigned char *p_result = NULL;
		      gaiaGeomCollPtr geom = gaiaAllocGeomColl ();
		      geom->Srid = row_node->Srid;
		      gaiaAddPointToGeomColl (geom, row_node->Node->CoordX,
					      row_node->Node->CoordY);
		      gaiaToSpatiaLiteBlobWkb (geom, &p_result, &len);
		      sqlite3_result_blob (pContext, p_result, len, free);
		      gaiaFreeGeomColl (geom);
		  }
	    }
      }
    if (column == 15)
      {
	  /* the [optional] Name column */
	  sqlite3_result_null (pContext);
      }
}

static void
do_common_column (virtualroutingCursorPtr cursor, virtualroutingPtr net,
		  sqlite3_context * pContext, int node_code,
		  ResultsetRowPtr row, int column)
{
/* processing an ordinary Routing (Shortest Path or TSP) solution row */
    const char *algorithm;
    char delimiter[128];
    const char *role;
    ResultsetRowPtr first = cursor->pVtab->multiSolution->FirstRow;
    MultiSolutionPtr multiSolution = cursor->pVtab->multiSolution;

    if (row == NULL)
      {
	  /* empty resultset */
	  if (column == 0)
	    {
		/* the currently used Algorithm */
		if (net->currentAlgorithm == VROUTE_A_STAR_ALGORITHM)
		    algorithm = "A*";
		else
		    algorithm = "Dijkstra";
		if (row != first)
		    sqlite3_result_null (pContext);
		else
		    sqlite3_result_text (pContext, algorithm,
					 strlen (algorithm), SQLITE_TRANSIENT);
	    }
	  else if (column == 1)
	    {
		/* the current Request type */
		if (net->currentRequest == VROUTE_TSP_NN)
		    algorithm = "TSP NN";
		else if (net->currentRequest == VROUTE_TSP_GA)
		    algorithm = "TSP GA";
		else
		    algorithm = "Shortest Path";
		if (row != first)
		    sqlite3_result_null (pContext);
		else
		    sqlite3_result_text (pContext, algorithm,
					 strlen (algorithm), SQLITE_TRANSIENT);
	    }
	  else if (column == 2)
	    {
		/* the currently set Options */
		if (net->currentOptions == VROUTE_SHORTEST_PATH_SIMPLE)
		    algorithm = "Simple";
		else if (net->currentOptions == VROUTE_SHORTEST_PATH_NO_LINKS)
		    algorithm = "No Links";
		else if (net->currentOptions == VROUTE_SHORTEST_PATH_NO_GEOMS)
		    algorithm = "No Geometries";
		else
		    algorithm = "Full";
		if (row != first)
		    sqlite3_result_null (pContext);
		else
		    sqlite3_result_text (pContext, algorithm,
					 strlen (algorithm), SQLITE_TRANSIENT);
	    }
	  else if (column == 3)
	    {
		/* the currently set delimiter char */
		if (isprint (cursor->pVtab->currentDelimiter))
		    sprintf (delimiter, "%c [dec=%d, hex=%02x]",
			     cursor->pVtab->currentDelimiter,
			     cursor->pVtab->currentDelimiter,
			     cursor->pVtab->currentDelimiter);
		else
		    sprintf (delimiter, "[dec=%d, hex=%02x]",
			     cursor->pVtab->currentDelimiter,
			     cursor->pVtab->currentDelimiter);
		if (row != first)
		    sqlite3_result_null (pContext);
		else
		    sqlite3_result_text (pContext, delimiter,
					 strlen (delimiter), SQLITE_TRANSIENT);
	    }
	  else
	      sqlite3_result_null (pContext);
	  return;
      }
    if (row->Undefined != NULL)
      {
	  /* special case: there is an undefined destination */
	  if (column == 0)
	    {
		/* the currently used Algorithm */
		fprintf (stderr, "common_column_undefined %d\n",
			 multiSolution->MultiTo->Items);
		if (multiSolution->MultiTo->Items > 1)
		  {
		      /* multiple destinations: always defaulting to Dijkstra */
		      algorithm = "Dijkstra";
		  }
		else
		  {
		      if (net->currentAlgorithm == VROUTE_A_STAR_ALGORITHM)
			  algorithm = "A*";
		      else
			  algorithm = "Dijkstra";
		  }
		if (row != first)
		    sqlite3_result_null (pContext);
		else
		    sqlite3_result_text (pContext, algorithm,
					 strlen (algorithm), SQLITE_TRANSIENT);
	    }
	  if (column == 1)
	    {
		/* the current Request type */
		if (net->currentRequest == VROUTE_TSP_NN)
		    algorithm = "TSP NN";
		else if (net->currentRequest == VROUTE_TSP_GA)
		    algorithm = "TSP GA";
		else
		    algorithm = "Shortest Path";
		if (row != first)
		    sqlite3_result_null (pContext);
		else
		    sqlite3_result_text (pContext, algorithm,
					 strlen (algorithm), SQLITE_TRANSIENT);
	    }
	  if (column == 2)
	    {
		/* the currently set Options */
		if (net->currentOptions == VROUTE_SHORTEST_PATH_SIMPLE)
		    algorithm = "Simple";
		else if (net->currentOptions == VROUTE_SHORTEST_PATH_NO_LINKS)
		    algorithm = "No Links";
		else if (net->currentOptions == VROUTE_SHORTEST_PATH_NO_GEOMS)
		    algorithm = "No Geometries";
		else
		    algorithm = "Full";
		if (row != first)
		    sqlite3_result_null (pContext);
		else
		    sqlite3_result_text (pContext, algorithm,
					 strlen (algorithm), SQLITE_TRANSIENT);
	    }
	  if (column == 3)
	    {
		/* the currently set delimiter char */
		if (isprint (cursor->pVtab->currentDelimiter))
		    sprintf (delimiter, "%c [dec=%d, hex=%02x]",
			     cursor->pVtab->currentDelimiter,
			     cursor->pVtab->currentDelimiter,
			     cursor->pVtab->currentDelimiter);
		else
		    sprintf (delimiter, "[dec=%d, hex=%02x]",
			     cursor->pVtab->currentDelimiter,
			     cursor->pVtab->currentDelimiter);
		if (row != first)
		    sqlite3_result_null (pContext);
		else
		    sqlite3_result_text (pContext, delimiter,
					 strlen (delimiter), SQLITE_TRANSIENT);
	    }
	  if (column == 4)
	    {
		/* the RouteNum column */
		sqlite3_result_null (pContext);
	    }
	  if (column == 5)
	    {
		/* the RouteRow column */
		sqlite3_result_null (pContext);
	    }
	  if (column == 6)
	    {
		/* role of this row */
		if (row->To != NULL)
		    role = "Unreachable NodeTo";
		else
		    role = "Undefined NodeTo";
		sqlite3_result_text (pContext, role, strlen (role),
				     SQLITE_TRANSIENT);
	    }
	  if (column == 7)
	    {
		/* the LinkRowId column */
		sqlite3_result_null (pContext);
	    }
	  if (column == 8)
	    {
		/* the NodeFrom column */
		if (row->From == NULL)
		  {
		      if (node_code)
			  sqlite3_result_text (pContext, row->Undefined,
					       strlen (row->Undefined),
					       SQLITE_STATIC);
		      else
			  sqlite3_result_int64 (pContext, row->UndefinedId);
		  }
		else
		  {
		      if (node_code)
			  sqlite3_result_text (pContext, row->From->Code,
					       strlen (row->From->Code),
					       SQLITE_STATIC);
		      else
			  sqlite3_result_int64 (pContext, row->From->Id);
		  }
	    }
	  if (column == 9)
	    {
		/* the NodeTo column */
		if (node_code)
		    sqlite3_result_text (pContext, row->Undefined,
					 strlen (row->Undefined),
					 SQLITE_STATIC);
		else
		    sqlite3_result_int64 (pContext, row->UndefinedId);
	    }
	  if (column == 10)
	    {
		/* the PointFrom column */
		sqlite3_result_null (pContext);
	    }
	  if (column == 11)
	    {
		/* the PointTo column */
		sqlite3_result_null (pContext);
	    }
	  if (column == 12)
	    {
		/* the Tolerance column */
		sqlite3_result_null (pContext);
	    }
	  if (column == 13)
	    {
		/* the Cost column */
		sqlite3_result_null (pContext);
	    }
	  if (column == 14)
	    {
		/* the Geometry column */
		sqlite3_result_null (pContext);
	    }
	  if (column == 15)
	    {
		/* the [optional] Name column */
		sqlite3_result_null (pContext);
	    }
	  return;
      }
    if (row->linkRef == NULL)
      {
	  /* special case: this one is the solution summary */
	  if (column == 0)
	    {
		/* the currently used Algorithm */
		fprintf (stderr, "common_column_summary %d\n",
			 multiSolution->MultiTo->Items);
		if (multiSolution->MultiTo->Items > 1)
		  {
		      /* multiple destinations: always defaulting to Dijkstra */
		      algorithm = "Dijkstra";
		  }
		else
		  {
		      if (net->currentAlgorithm == VROUTE_A_STAR_ALGORITHM)
			  algorithm = "A*";
		      else
			  algorithm = "Dijkstra";
		  }
		if (row != first)
		    sqlite3_result_null (pContext);
		else
		    sqlite3_result_text (pContext, algorithm,
					 strlen (algorithm), SQLITE_TRANSIENT);
	    }
	  if (column == 1)
	    {
		/* the current Request type */
		if (net->currentRequest == VROUTE_TSP_NN)
		    algorithm = "TSP NN";
		else if (net->currentRequest == VROUTE_TSP_GA)
		    algorithm = "TSP GA";
		else
		    algorithm = "Shortest Path";
		if (row != first)
		    sqlite3_result_null (pContext);
		else
		    sqlite3_result_text (pContext, algorithm,
					 strlen (algorithm), SQLITE_TRANSIENT);
	    }
	  if (column == 2)
	    {
		/* the currently set Options */
		if (net->currentOptions == VROUTE_SHORTEST_PATH_SIMPLE)
		    algorithm = "Simple";
		else if (net->currentOptions == VROUTE_SHORTEST_PATH_NO_LINKS)
		    algorithm = "No Links";
		else if (net->currentOptions == VROUTE_SHORTEST_PATH_NO_GEOMS)
		    algorithm = "No Geometries";
		else
		    algorithm = "Full";
		if (row != first)
		    sqlite3_result_null (pContext);
		else
		    sqlite3_result_text (pContext, algorithm,
					 strlen (algorithm), SQLITE_TRANSIENT);
	    }
	  if (column == 3)
	    {
		/* the currently set delimiter char */
		if (isprint (cursor->pVtab->currentDelimiter))
		    sprintf (delimiter, "%c [dec=%d, hex=%02x]",
			     cursor->pVtab->currentDelimiter,
			     cursor->pVtab->currentDelimiter,
			     cursor->pVtab->currentDelimiter);
		else
		    sprintf (delimiter, "[dec=%d, hex=%02x]",
			     cursor->pVtab->currentDelimiter,
			     cursor->pVtab->currentDelimiter);
		if (row != first)
		    sqlite3_result_null (pContext);
		else
		    sqlite3_result_text (pContext, delimiter,
					 strlen (delimiter), SQLITE_TRANSIENT);
	    }
	  if (column == 4)
	    {
		/* the RouteNum column */
		sqlite3_result_int (pContext, row->RouteNum);
	    }
	  if (column == 5)
	    {
		/* the RouteRow column */
		sqlite3_result_int (pContext, row->RouteRow);
	    }
	  if (column == 6)
	    {
		/* role of this row */
		if (cursor->pVtab->multiSolution->Mode == VROUTE_TSP_SOLUTION
		    && row->RouteNum == 0)
		    role = "TSP Solution";
		else
		  {
		      if (row->From == row->To)
			  role = "Unreachable NodeTo";
		      else
			  role = "Route";
		  }
		sqlite3_result_text (pContext, role, strlen (role),
				     SQLITE_TRANSIENT);
	    }
	  if (row->From == NULL || row->To == NULL)
	    {
		/* empty [uninitialized] solution */
		if (column > 0)
		    sqlite3_result_null (pContext);
		return;
	    }
	  if (column == 7)
	    {
		/* the LinkRowId column */
		sqlite3_result_null (pContext);
	    }
	  if (column == 8)
	    {
		/* the NodeFrom column */
		if (node_code)
		    sqlite3_result_text (pContext, row->From->Code,
					 strlen (row->From->Code),
					 SQLITE_STATIC);
		else
		    sqlite3_result_int64 (pContext, row->From->Id);
	    }
	  if (column == 9)
	    {
		/* the NodeTo column */
		if (node_code)
		    sqlite3_result_text (pContext, row->To->Code,
					 strlen (row->To->Code), SQLITE_STATIC);
		else
		    sqlite3_result_int64 (pContext, row->To->Id);
	    }
	  if (column == 10)
	    {
		/* the PointFrom column */
		sqlite3_result_null (pContext);
	    }
	  if (column == 11)
	    {
		/* the PointTo column */
		sqlite3_result_null (pContext);
	    }
	  if (column == 12)
	    {
		/* the Tolerance column */
		sqlite3_result_null (pContext);
	    }
	  if (column == 13)
	    {
		/* the Cost column */
		if (row->RouteNum != 0 && (row->From == row->To))
		    sqlite3_result_null (pContext);
		else
		    sqlite3_result_double (pContext, row->TotalCost);
	    }
	  if (column == 14)
	    {
		/* the Geometry column */
		if (!(row->Geometry))
		    sqlite3_result_null (pContext);
		else
		  {
		      /* builds the BLOB geometry to be returned */
		      int len;
		      unsigned char *p_result = NULL;
		      gaiaToSpatiaLiteBlobWkb (row->Geometry, &p_result, &len);
		      sqlite3_result_blob (pContext, p_result, len, free);
		  }
	    }
	  if (column == 15)
	    {
		/* the [optional] Name column */
		sqlite3_result_null (pContext);
	    }
      }
    else
      {
	  /* ordinary case: this one is a Link used by the solution */
	  if (column == 0)
	    {
		/* the currently used Algorithm - alwasy NULL */
		sqlite3_result_null (pContext);
	    }
	  if (column == 1)
	    {
		/* the current Request type - always NULL */
		sqlite3_result_null (pContext);
	    }
	  if (column == 2)
	    {
		/* the currently set Options- always NULL */
		sqlite3_result_null (pContext);
	    }
	  if (column == 3)
	    {
		/* the currently set delimiter char- always NULL */
		sqlite3_result_null (pContext);
	    }
	  if (column == 4)
	    {
		/* the RouteNum column */
		sqlite3_result_int (pContext, row->RouteNum);
	    }
	  if (column == 5)
	    {
		/* the RouteRow column */
		sqlite3_result_int (pContext, row->RouteRow);
	    }
	  if (column == 6)
	    {
		/* role of this row */
		role = "Link";
		sqlite3_result_text (pContext, role, strlen (role),
				     SQLITE_TRANSIENT);
	    }
	  if (column == 7)
	    {
		/* the LinkRowId column */
		sqlite3_result_int64 (pContext, row->linkRef->Link->LinkRowid);
	    }
	  if (column == 8)
	    {
		/* the NodeFrom column */
		if (node_code)
		    sqlite3_result_text (pContext,
					 row->linkRef->Link->NodeFrom->Code,
					 strlen (row->linkRef->Link->
						 NodeFrom->Code),
					 SQLITE_STATIC);
		else
		    sqlite3_result_int64 (pContext,
					  row->linkRef->Link->NodeFrom->Id);
	    }
	  if (column == 9)
	    {
		/* the NodeTo column */
		if (node_code)
		    sqlite3_result_text (pContext,
					 row->linkRef->Link->NodeTo->Code,
					 strlen (row->linkRef->Link->
						 NodeTo->Code), SQLITE_STATIC);
		else
		    sqlite3_result_int64 (pContext,
					  row->linkRef->Link->NodeTo->Id);
	    }
	  if (column == 10)
	    {
		/* the PointFrom column */
		sqlite3_result_null (pContext);
	    }
	  if (column == 11)
	    {
		/* the PointTo column */
		sqlite3_result_null (pContext);
	    }
	  if (column == 12)
	    {
		/* the Tolerance column */
		sqlite3_result_null (pContext);
	    }
	  if (column == 13)
	    {
		/* the Cost column */
		sqlite3_result_double (pContext, row->linkRef->Link->Cost);
	    }
	  if (column == 14)
	    {
		/* the Geometry column */
		sqlite3_result_null (pContext);
	    }
	  if (column == 15)
	    {
		/* the [optional] Name column */
		if (row->linkRef->Name)
		    sqlite3_result_text (pContext, row->linkRef->Name,
					 strlen (row->linkRef->Name),
					 SQLITE_STATIC);
		else
		    sqlite3_result_null (pContext);
	    }
      }
}

static void
do_point2point_column (virtualroutingCursorPtr cursor, virtualroutingPtr net,
		       sqlite3_context * pContext, int node_code,
		       ResultsetRowPtr row, int column)
{
/* processing a Point2Point solution row */
    const char *algorithm;
    char delimiter[128];
    const char *role;
    Point2PointSolutionPtr p2p = cursor->pVtab->point2PointSolution;
    if (row->linkRef == NULL || row->Point2PointRole == VROUTE_POINT2POINT_START
	|| row->Point2PointRole == VROUTE_POINT2POINT_END)
      {
	  /* special case: this one is the solution summary */
	  if (column == 0)
	    {
		/* the currently used Algorithm always defaults to Dijkstra */
		if (row->linkRef == NULL
		    && row->Point2PointRole == VROUTE_POINT2POINT_NONE)
		  {
		      algorithm = "Dijkstra";
		      sqlite3_result_text (pContext, algorithm,
					   strlen (algorithm),
					   SQLITE_TRANSIENT);
		  }
		else
		    sqlite3_result_null (pContext);
	    }
	  if (column == 1)
	    {
		/* the current Request type */
		if (row->linkRef == NULL
		    && row->Point2PointRole == VROUTE_POINT2POINT_NONE)
		  {
		      algorithm = "Point2Point Path";
		      sqlite3_result_text (pContext, algorithm,
					   strlen (algorithm),
					   SQLITE_TRANSIENT);
		  }
		else
		    sqlite3_result_null (pContext);
	    }
	  if (column == 2)
	    {
		/* the currently set Options */
		if (row->linkRef == NULL
		    && row->Point2PointRole == VROUTE_POINT2POINT_NONE)
		  {
		      if (net->currentOptions == VROUTE_SHORTEST_PATH_SIMPLE)
			  algorithm = "Simple";
		      else if (net->currentOptions ==
			       VROUTE_SHORTEST_PATH_NO_LINKS)
			  algorithm = "No Links";
		      else if (net->currentOptions ==
			       VROUTE_SHORTEST_PATH_NO_GEOMS)
			  algorithm = "No Geometries";
		      else
			  algorithm = "Full";
		      sqlite3_result_text (pContext, algorithm,
					   strlen (algorithm),
					   SQLITE_TRANSIENT);
		  }
		else
		    sqlite3_result_null (pContext);
	    }
	  if (column == 3)
	    {
		/* the currently set delimiter char */
		if (row->linkRef == NULL
		    && row->Point2PointRole == VROUTE_POINT2POINT_NONE)
		  {
		      if (isprint (cursor->pVtab->currentDelimiter))
			  sprintf (delimiter, "%c [dec=%d, hex=%02x]",
				   cursor->pVtab->currentDelimiter,
				   cursor->pVtab->currentDelimiter,
				   cursor->pVtab->currentDelimiter);
		      else
			  sprintf (delimiter, "[dec=%d, hex=%02x]",
				   cursor->pVtab->currentDelimiter,
				   cursor->pVtab->currentDelimiter);
		      sqlite3_result_text (pContext, delimiter,
					   strlen (delimiter),
					   SQLITE_TRANSIENT);
		  }
		else
		    sqlite3_result_null (pContext);
	    }
	  if (column == 4)
	    {
		/* the RouteNum column */
		sqlite3_result_int (pContext, row->RouteNum);
	    }
	  if (column == 5)
	    {
		/* the RouteRow column */
		sqlite3_result_int (pContext, row->RouteRow);
	    }
	  if (column == 6)
	    {
		/* role of this row */
		if (row->Point2PointRole == VROUTE_POINT2POINT_INGRESS)
		    role = "Ingress Path";
		else if (row->Point2PointRole == VROUTE_POINT2POINT_START)
		    role = "Partial Link (Start)";
		else if (row->Point2PointRole == VROUTE_POINT2POINT_END)
		    role = "Partial Link (End)";
		else if (row->Point2PointRole == VROUTE_POINT2POINT_EGRESS)
		    role = "Egress Path";
		else
		    role = "Point2Point Solution";
		sqlite3_result_text (pContext, role, strlen (role),
				     SQLITE_TRANSIENT);
	    }
	  if (column == 7)
	    {
		/* the LinkRowId column */
		if (row->Point2PointRole == VROUTE_POINT2POINT_START
		    || row->Point2PointRole == VROUTE_POINT2POINT_END)
		    sqlite3_result_int64 (pContext,
					  row->linkRef->Link->LinkRowid);
		else
		    sqlite3_result_null (pContext);
	    }
	  if (column == 8)
	    {
		/* the NodeFrom column */
		if (row->Point2PointRole == VROUTE_POINT2POINT_END)
		  {
		      if (node_code)
			  sqlite3_result_text (pContext,
					       row->linkRef->Link->
					       NodeFrom->Code,
					       strlen (row->linkRef->
						       Link->NodeFrom->Code),
					       SQLITE_STATIC);
		      else
			  sqlite3_result_int64 (pContext,
						row->linkRef->Link->
						NodeFrom->Id);
		  }
		else
		    sqlite3_result_null (pContext);
	    }
	  if (column == 9)
	    {
		/* the NodeTo column */
		if (row->Point2PointRole == VROUTE_POINT2POINT_START)
		  {
		      if (node_code)
			  sqlite3_result_text (pContext,
					       row->linkRef->Link->NodeTo->Code,
					       strlen (row->linkRef->
						       Link->NodeTo->Code),
					       SQLITE_STATIC);
		      else
			  sqlite3_result_int64 (pContext,
						row->linkRef->Link->NodeTo->Id);
		  }
		else
		    sqlite3_result_null (pContext);
	    }
	  if (column == 10)
	    {
		/* the PointFrom column */
		if (row->Point2PointRole == VROUTE_POINT2POINT_NONE)
		  {
		      unsigned char *blob;
		      int blob_size;
		      if (p2p->hasZ)
			  gaiaMakePointZ (p2p->xFrom, p2p->yFrom, p2p->zFrom,
					  p2p->srid, &blob, &blob_size);
		      else
			  gaiaMakePoint (p2p->xFrom, p2p->yFrom, p2p->srid,
					 &blob, &blob_size);
		      sqlite3_result_blob (pContext, blob, blob_size, free);
		  }
		else
		    sqlite3_result_null (pContext);
	    }
	  if (column == 11)
	    {
		/* the PointTo column */
		if (row->Point2PointRole == VROUTE_POINT2POINT_NONE)
		  {
		      unsigned char *blob;
		      int blob_size;
		      if (p2p->hasZ)
			  gaiaMakePointZ (p2p->xTo, p2p->yTo, p2p->zTo,
					  p2p->srid, &blob, &blob_size);
		      else
			  gaiaMakePoint (p2p->xTo, p2p->yTo, p2p->srid, &blob,
					 &blob_size);
		      sqlite3_result_blob (pContext, blob, blob_size, free);
		  }
		else
		    sqlite3_result_null (pContext);
	    }
	  if (column == 12)
	    {
		/* the Tolerance column */
		if (row->Point2PointRole == VROUTE_POINT2POINT_NONE)
		    sqlite3_result_double (pContext, cursor->pVtab->Tolerance);
		else
		    sqlite3_result_null (pContext);
	    }
	  if (column == 13)
	    {
		/* the Cost column */
		sqlite3_result_double (pContext, row->TotalCost);
	    }
	  if (column == 14)
	    {
		/* the Geometry column */
		if (!(row->Geometry))
		    sqlite3_result_null (pContext);
		else
		  {
		      /* builds the BLOB geometry to be returned */
		      int len;
		      unsigned char *p_result = NULL;
		      gaiaToSpatiaLiteBlobWkb (row->Geometry, &p_result, &len);
		      sqlite3_result_blob (pContext, p_result, len, free);
		  }
	    }
	  if (column == 15)
	    {
		/* the [optional] Name column */
		if (row->Point2PointRole == VROUTE_POINT2POINT_START
		    || row->Point2PointRole == VROUTE_POINT2POINT_END)
		  {
		      if (row->linkRef->Name)
			  sqlite3_result_text (pContext, row->linkRef->Name,
					       strlen (row->linkRef->Name),
					       SQLITE_STATIC);
		      else
			  sqlite3_result_null (pContext);
		  }
		else
		    sqlite3_result_null (pContext);
	    }
      }
    else
      {
	  /* ordinary case: this one is a Link used by the solution */
	  if (column == 0)
	    {
		/* the currently used Algorithm always defaults to NULL */
		sqlite3_result_null (pContext);
	    }
	  if (column == 1)
	    {
		/* the current Request type always defaults to NULL */
		sqlite3_result_null (pContext);
	    }
	  if (column == 2)
	    {
		/* the currently set Options always defaults to NULL */
		sqlite3_result_null (pContext);
	    }
	  if (column == 3)
	    {
		/* the currently set delimiter char always defaults to NULL */
		sqlite3_result_null (pContext);
	    }
	  if (column == 4)
	    {
		/* the RouteNum column */
		sqlite3_result_int (pContext, row->RouteNum);
	    }
	  if (column == 5)
	    {
		/* the RouteRow column */
		sqlite3_result_int (pContext, row->RouteRow);
	    }
	  if (column == 6)
	    {
		/* role of this row */
		role = "Link";
		sqlite3_result_text (pContext, role, strlen (role),
				     SQLITE_TRANSIENT);
	    }
	  if (column == 7)
	    {
		/* the LinkRowId column */
		sqlite3_result_int64 (pContext, row->linkRef->Link->LinkRowid);
	    }
	  if (column == 8)
	    {
		/* the NodeFrom column */
		if (node_code)
		    sqlite3_result_text (pContext,
					 row->linkRef->Link->NodeFrom->Code,
					 strlen (row->linkRef->Link->
						 NodeFrom->Code),
					 SQLITE_STATIC);
		else
		    sqlite3_result_int64 (pContext,
					  row->linkRef->Link->NodeFrom->Id);
	    }
	  if (column == 9)
	    {
		/* the NodeTo column */
		if (node_code)
		    sqlite3_result_text (pContext,
					 row->linkRef->Link->NodeTo->Code,
					 strlen (row->linkRef->Link->
						 NodeTo->Code), SQLITE_STATIC);
		else
		    sqlite3_result_int64 (pContext,
					  row->linkRef->Link->NodeTo->Id);
	    }
	  if (column == 13)
	    {
		/* the Cost column */
		sqlite3_result_double (pContext, row->linkRef->Link->Cost);
	    }
	  if (column == 14)
	    {
		/* the Geometry column */
		sqlite3_result_null (pContext);
	    }
	  if (column == 15)
	    {
		/* the [optional] Name column */
		if (row->linkRef->Name)
		    sqlite3_result_text (pContext, row->linkRef->Name,
					 strlen (row->linkRef->Name),
					 SQLITE_STATIC);
		else
		    sqlite3_result_null (pContext);
	    }
      }
}

static int
vroute_column (sqlite3_vtab_cursor * pCursor, sqlite3_context * pContext,
	       int column)
{
/* fetching value for the Nth column */
    ResultsetRowPtr row;
    RowNodeSolutionPtr row_node;
    int node_code = 0;
    virtualroutingCursorPtr cursor = (virtualroutingCursorPtr) pCursor;
    virtualroutingPtr net = (virtualroutingPtr) cursor->pVtab;
    node_code = net->graph->NodeCode;
    if (cursor->pVtab->multiSolution->Mode == VROUTE_RANGE_SOLUTION)
      {
	  /* processing "within Cost range" solution */
	  row_node = cursor->pVtab->multiSolution->CurrentNodeRow;
	  do_cost_range_column (cursor, pContext, node_code, row_node, column);
	  return SQLITE_OK;
      }
    else if (cursor->pVtab->multiSolution->Mode == VROUTE_ROUTING_SOLUTION
	     || cursor->pVtab->multiSolution->Mode == VROUTE_TSP_SOLUTION)
      {
	  /* processing an ordinary Routing (Shortest Path or TSP) solution */
	  row = cursor->pVtab->multiSolution->CurrentRow;
	  do_common_column (cursor, net, pContext, node_code, row, column);
	  return SQLITE_OK;
      }

    if (cursor->pVtab->point2PointSolution != NULL)
      {
	  if (cursor->pVtab->point2PointSolution->Mode ==
	      VROUTE_POINT2POINT_SOLUTION)
	    {
		/* processing a Point2Point solution */
		row = cursor->pVtab->point2PointSolution->CurrentRow;
		do_point2point_column (cursor, net, pContext, node_code, row,
				       column);
		return SQLITE_OK;
	    }
      }
    return SQLITE_OK;
}

static int
vroute_rowid (sqlite3_vtab_cursor * pCursor, sqlite_int64 * pRowid)
{
/* fetching the ROWID */
    virtualroutingCursorPtr cursor = (virtualroutingCursorPtr) pCursor;
    *pRowid = cursor->pVtab->multiSolution->CurrentRowId;
    return SQLITE_OK;
}

static int
vroute_update (sqlite3_vtab * pVTab, int argc, sqlite3_value ** argv,
	       sqlite_int64 * pRowid)
{
/* generic update [INSERT / UPDATE / DELETE */
    virtualroutingPtr p_vtab = (virtualroutingPtr) pVTab;
    if (pRowid)
	pRowid = pRowid;	/* unused arg warning suppression */
    if (argc == 1)
      {
	  /* performing a DELETE is forbidden */
	  return SQLITE_READONLY;
      }
    else
      {
	  if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	    {
		/* performing an INSERT is forbidden */
		return SQLITE_READONLY;
	    }
	  else
	    {
		/* performing an UPDATE */
		if (argc == 18)
		  {
		      p_vtab->currentAlgorithm = VROUTE_DIJKSTRA_ALGORITHM;
		      p_vtab->currentDelimiter = ',';
		      if (sqlite3_value_type (argv[2]) == SQLITE_TEXT)
			{
			    const unsigned char *algorithm =
				sqlite3_value_text (argv[2]);
			    if (strcasecmp ((char *) algorithm, "A*") == 0)
				p_vtab->currentAlgorithm =
				    VROUTE_A_STAR_ALGORITHM;
			}
		      if (p_vtab->graph->AStar == 0)
			  p_vtab->currentAlgorithm = VROUTE_DIJKSTRA_ALGORITHM;
		      if (sqlite3_value_type (argv[3]) == SQLITE_TEXT)
			{
			    const unsigned char *request =
				sqlite3_value_text (argv[3]);
			    if (strcasecmp ((char *) request, "TSP") == 0)
				p_vtab->currentRequest = VROUTE_TSP_NN;
			    else if (strcasecmp ((char *) request, "TSP NN") ==
				     0)
				p_vtab->currentRequest = VROUTE_TSP_NN;
			    else if (strcasecmp ((char *) request, "TSP GA") ==
				     0)
				p_vtab->currentRequest = VROUTE_TSP_GA;
			    else if (strcasecmp
				     ((char *) request, "SHORTEST PATH") == 0)
				p_vtab->currentRequest = VROUTE_SHORTEST_PATH;
			}
		      if (sqlite3_value_type (argv[4]) == SQLITE_TEXT)
			{
			    const unsigned char *options =
				sqlite3_value_text (argv[4]);
			    if (strcasecmp ((char *) options, "NO LINKS") == 0)
				p_vtab->currentOptions =
				    VROUTE_SHORTEST_PATH_NO_LINKS;
			    else if (strcasecmp
				     ((char *) options, "NO GEOMETRIES") == 0)
				p_vtab->currentOptions =
				    VROUTE_SHORTEST_PATH_NO_GEOMS;
			    else if (strcasecmp ((char *) options, "SIMPLE") ==
				     0)
				p_vtab->currentOptions =
				    VROUTE_SHORTEST_PATH_SIMPLE;
			    else if (strcasecmp ((char *) options, "FULL") == 0)
				p_vtab->currentOptions =
				    VROUTE_SHORTEST_PATH_FULL;
			}
		      if (sqlite3_value_type (argv[5]) == SQLITE_TEXT)
			{
			    const unsigned char *delimiter =
				sqlite3_value_text (argv[5]);
			    p_vtab->currentDelimiter = *delimiter;
			}
		      if (sqlite3_value_type (argv[14]) == SQLITE_FLOAT)
			  p_vtab->Tolerance = sqlite3_value_double (argv[14]);
		  }
		return SQLITE_OK;
	    }
      }
}

static int
vroute_begin (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vroute_sync (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vroute_commit (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vroute_rollback (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vroute_rename (sqlite3_vtab * pVTab, const char *zNew)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    if (zNew)
	zNew = zNew;		/* unused arg warning suppression */
    return SQLITE_ERROR;
}

static int
splitevirtualroutingInit (sqlite3 * db)
{
    int rc = SQLITE_OK;
    my_route_module.iVersion = 1;
    my_route_module.xCreate = &vroute_create;
    my_route_module.xConnect = &vroute_connect;
    my_route_module.xBestIndex = &vroute_best_index;
    my_route_module.xDisconnect = &vroute_disconnect;
    my_route_module.xDestroy = &vroute_destroy;
    my_route_module.xOpen = &vroute_open;
    my_route_module.xClose = &vroute_close;
    my_route_module.xFilter = &vroute_filter;
    my_route_module.xNext = &vroute_next;
    my_route_module.xEof = &vroute_eof;
    my_route_module.xColumn = &vroute_column;
    my_route_module.xRowid = &vroute_rowid;
    my_route_module.xUpdate = &vroute_update;
    my_route_module.xBegin = &vroute_begin;
    my_route_module.xSync = &vroute_sync;
    my_route_module.xCommit = &vroute_commit;
    my_route_module.xRollback = &vroute_rollback;
    my_route_module.xFindFunction = NULL;
    my_route_module.xRename = &vroute_rename;
    sqlite3_create_module_v2 (db, "virtualrouting", &my_route_module, NULL, 0);
    return rc;
}

SPATIALITE_PRIVATE int
virtualrouting_extension_init (void *xdb)
{
    sqlite3 *db = (sqlite3 *) xdb;
    return splitevirtualroutingInit (db);
}

#endif /* end GEOS conditional */
