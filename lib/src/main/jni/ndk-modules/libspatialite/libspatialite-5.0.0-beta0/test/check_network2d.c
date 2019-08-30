/*

 check_network2d.c -- SpatiaLite Test Case

 Author: Sandro Furieri <a.furieri@lqt.it>

 ------------------------------------------------------------------------------
 
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
 
Portions created by the Initial Developer are Copyright (C) 2011
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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#include "sqlite3.h"
#include "spatialite.h"

#ifdef ENABLE_RTTOPO		/* only if RTTOPO is enabled */

static int
do_level3_tests (sqlite3 * handle, int *retcode)
{
/* performing basic tests: level 3 */
    int ret;
    char *err_msg = NULL;

/* testing RegisterTopoNetCoverage */
    ret =
	sqlite3_exec (handle,
		      "SELECT CreateStylingTables(1)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateStylingTables() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -240;
	  return 0;
      }

/* testing RegisterTopoNetCoverage - short form */
    ret =
	sqlite3_exec (handle,
		      "SELECT SE_RegisterTopoNetCoverage('net', 'roads')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SE_RegisterTopoNetCoverage() #1 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -241;
	  return 0;
      }

/* testing UnRegisterVectorCoverage */
    ret =
	sqlite3_exec (handle,
		      "SELECT SE_UnRegisterVectorCoverage('net')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SE_RegisterVectorCoverage() #1 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -342;
	  return 0;
      }

/* testing RegisterTopoNetCoverage - long form */
    ret =
	sqlite3_exec (handle,
		      "SELECT SE_RegisterTopoNetCoverage('net', 'roads', 'title', 'abstract', 1, 1)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SE_RegisterTopoNetCoverage() #2 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -241;
	  return 0;
      }

    return 1;
}

static int
do_level2_tests (sqlite3 * handle, int *retcode)
{
/* performing basic tests: Level 2 */
    int ret;
    char *err_msg = NULL;

/* inserting three Nodes */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('roads', MakePoint(-100, -100, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNetNode() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -160;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('roads', MakePoint(-100, -10, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNetNode() #5 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -161;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('roads', MakePoint(-10, -10, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNetNode() #6 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -162;
	  return 0;
      }

/* inserting two Links */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 5, 4, GeomFromText('LINESTRING(-100 -10, -100 -100)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -163;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 5, 6, GeomFromText('LINESTRING(-100 -10, -10 -10)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() #5 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -164;
	  return 0;
      }

/* splitting a Link (New) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewGeoLinkSplit('roads', 3, MakePoint(-100, -50, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_NewGeoLinkSplit() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -165;
	  return 0;
      }

/* splitting a Link (Mod) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModGeoLinkSplit('roads', 4, MakePoint(-30, -10, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_ModGeoLinkSplit() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -166;
	  return 0;
      }

/* attempting to split a link (non-existing link) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewGeoLinkSplit('roads', 333, MakePoint(0, 0, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_NewGeoLinkSplit() #2: expected failure\n");
	  *retcode = -167;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent link.") != 0)
      {
	  fprintf (stderr, "ST_NewGeoLinkSplit() #2: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -168;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split a link (non-existing link) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModGeoLinkSplit('roads', 333, MakePoint(0, 0, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ModGeoLinkSplit() #2: expected failure\n");
	  *retcode = -169;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent link.") != 0)
      {
	  fprintf (stderr, "ST_ModGeoLinkSplit() #2: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -170;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split a link (point not on link) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewGeoLinkSplit('roads', 6, MakePoint(-101, -60, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_NewGeoLinkSplit() #3: expected failure\n");
	  *retcode = -171;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - point not on link.") != 0)
      {
	  fprintf (stderr, "ST_NewGeoLinkSplit() #3: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -172;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split a link (non-existing link) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModGeoLinkSplit('roads', 6, MakePoint(-101, -60, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ModGeoLinkSplit() #3: expected failure\n");
	  *retcode = -173;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - point not on link.") != 0)
      {
	  fprintf (stderr, "ST_ModGeoLinkSplit() #3: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -174;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to heal links (same link) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewLinkHeal('roads', 4, 4)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_NewLinkHeal() #1: expected failure\n");
	  *retcode = -175;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - Cannot heal link with itself.") != 0)
      {
	  fprintf (stderr, "ST_NewLinkSplit() #1: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -176;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to heal links (same link) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModLinkHeal('roads', 4, 4)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ModLinkHeal() #1: expected failure\n");
	  *retcode = -177;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - Cannot heal link with itself.") != 0)
      {
	  fprintf (stderr, "ST_ModLinkSplit() #1: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -178;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to heal links (non-existent first link) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewLinkHeal('roads', 333, 4)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_NewLinkHeal() #2: expected failure\n");
	  *retcode = -179;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent first link.")
	!= 0)
      {
	  fprintf (stderr, "ST_NewLinkHeal() #2: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -180;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to heal links (non-existent first link) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModLinkHeal('roads', 333, 4)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ModLinkHeal() #2: expected failure\n");
	  *retcode = -181;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent first link.")
	!= 0)
      {
	  fprintf (stderr, "ST_ModLinkHeal() #2: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -182;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to heal links (non-existent second link) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewLinkHeal('roads', 4, 333)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_NewLinkHeal() #3: expected failure\n");
	  *retcode = -183;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent second link.")
	!= 0)
      {
	  fprintf (stderr, "ST_NewLinkHeal() #3: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -184;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to heal links (non-existent second link) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModLinkHeal('roads', 4, 333)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ModLinkHeal() #3: expected failure\n");
	  *retcode = -185;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent second link.")
	!= 0)
      {
	  fprintf (stderr, "ST_ModLinkHeal() #3: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -186;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to heal links (non-connected links) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewLinkHeal('roads', 7, 1)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_NewLinkHeal() #4: expected failure\n");
	  *retcode = -187;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-connected links.") !=
	0)
      {
	  fprintf (stderr, "ST_NewLinkHeal() #4: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -188;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to heal links (non-connected links) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModLinkHeal('roads', 7, 1)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ModLinkHeal() #4: expected failure\n");
	  *retcode = -189;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-connected links.") !=
	0)
      {
	  fprintf (stderr, "ST_ModLinkHeal() #4: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -190;
	  return 0;
      }
    sqlite3_free (err_msg);

/* inserting one more Link */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 5, 2, GeomFromText('LINESTRING(-100 -10, 0 100)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() #6 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -191;
	  return 0;
      }

/* attempting to heal links (other links connected) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewLinkHeal('roads', 5, 4)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_NewLinkHeal() #5: expected failure\n");
	  *retcode = -192;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - other links connected.") !=
	0)
      {
	  fprintf (stderr, "ST_NewLinkHeal() #5: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -193;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to heal links (other links connected) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModLinkHeal('roads', 4, 5)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ModLinkHeal() #5: expected failure\n");
	  *retcode = -194;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - other links connected.") !=
	0)
      {
	  fprintf (stderr, "ST_ModLinkHeal() #5: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -195;
	  return 0;
      }
    sqlite3_free (err_msg);

/* deleting a Link */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_RemoveLink('roads', 8)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_RemoveLink() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -196;
	  return 0;
      }

/* healing a Link (New) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewLinkHeal('roads', 5, 4)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_NewLinkHeal() #6 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -197;
	  return 0;
      }

/* healing another Link (Mod) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModLinkHeal('roads', 7, 9)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_ModLinkHeal() #6 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -198;
	  return 0;
      }

/* healing a third Link (Mod) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModLinkHeal('roads', 7, 6)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_ModLinkHeal() #7 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -199;
	  return 0;
      }

/* deleting another Link */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_RemoveLink('roads', 7)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_RemoveLink() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -200;
	  return 0;
      }

/* deleting two Nodes */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_RemIsoNetNode('roads', 4)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_RemIsoNetNode() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -201;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_RemIsoNetNode('roads', 6)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_RemIsoNetNode() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -202;
	  return 0;
      }

/* inserting two Nodes */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('roads', MakePoint(2, 100, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNetNode() #7 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -203;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('roads', MakePoint(2, 90, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNetNode() #8 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -204;
	  return 0;
      }

/* inserting a Link */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 9, 10, GeometryFromText('LINESTRING(2 100, 2 90)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() #7 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -205;
	  return 0;
      }

/* retrieving a NetNode by Point */
    ret =
	sqlite3_exec (handle,
		      "SELECT GetNetNodeByPoint('roads', MakePoint(0, 100), 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "GetNetNodeByPoint() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -206;
	  return 0;
      }

/* attempting to retrieve a NetNode by Point (two NetNodes found) */
    ret =
	sqlite3_exec (handle,
		      "SELECT GetNetNodeByPoint('roads', MakePoint(1, 100), 3.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "GetNetNodeByPoint() #2: expected failure\n");
	  *retcode = -207;
	  return 0;
      }
    if (strcmp (err_msg, "Two or more net-nodes found") != 0)
      {
	  fprintf (stderr, "GetNetNodeByPoint() #2: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -208;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to retrieve a NetNode by Point (not found) */
    ret =
	sqlite3_exec (handle,
		      "SELECT GetNetNodeByPoint('roads', MakePoint(1, 1), 0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "GetNetNodeByPoint() #3: expected failure\n");
	  sqlite3_free (err_msg);
	  *retcode = -209;
	  return 0;
      }

/* retrieving a Link by Point */
    ret =
	sqlite3_exec (handle,
		      "SELECT GetLinkByPoint('roads', MakePoint(0, 50), 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "GetLinkByPoint() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -210;
	  return 0;
      }

/* attempting to retrieve a Link by Point (two Links found) */
    ret =
	sqlite3_exec (handle,
		      "SELECT GetLinkByPoint('roads', MakePoint(1, 95), 3.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "GetLinkByPoint() #2: expected failure\n");
	  *retcode = -211;
	  return 0;
      }
    if (strcmp (err_msg, "Two or more links found") != 0)
      {
	  fprintf (stderr, "GetLinkByPoint() #2: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -212;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to retrieve a Link by Point (not found) */
    ret =
	sqlite3_exec (handle,
		      "SELECT GetLinkByPoint('roads', MakePoint(1, 1), 0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "GetLinkByPoint() #3expected failure\n");
	  sqlite3_free (err_msg);
	  *retcode = -213;
	  return 0;
      }

    return 1;
}

static int
do_level1_tests (sqlite3 * handle, int *retcode)
{
/* performing basic tests: Level 1 */
    int ret;
    char *err_msg = NULL;

/* inserting a first Node */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('roads', MakePoint(0, 0, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNetNode() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -110;
	  return 0;
      }

/* inserting a second Node */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('roads', MakePoint(0, 100, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNetNode() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -111;
	  return 0;
      }

/* inserting a Link */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 1, 2, GeometryFromText('LINESTRING(0 0, 0 100)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -112;
	  return 0;
      }

/* attempting to insert a Node (coincident node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('roads', MakePoint(0, 100, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNetNode() #3: expected failure\n");
	  *retcode = -113;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - coincident node.") != 0)
      {
	  fprintf (stderr, "ST_AddIsoNetNode() #3: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -114;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert a Node (coincident link) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('roads', MakePoint(0, 50, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNetNode() #4: expected failure\n");
	  *retcode = -115;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - link crosses node.") != 0)
      {
	  fprintf (stderr, "ST_AddIsoNetNode() #4: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -116;
	  return 0;
      }
    sqlite3_free (err_msg);

/* inserting a third Node */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('roads', MakePoint(99, 99, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNetNode() #5 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -117;
	  return 0;
      }

/* moving the third Node */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNetNode('roads', 3, MakePoint(100, 100, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_MoveIsoNetNode() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -118;
	  return 0;
      }

/* attempting to move a Node (non-existing node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNetNode('roads', 333, MakePoint(50, 50, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_MoveIsoNetNode() #2: expected failure\n");
	  *retcode = -119;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent node.") != 0)
      {
	  fprintf (stderr, "ST_MoveIsoNetNode() #2: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -120;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to move a Node (coincident node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNetNode('roads', 3, MakePoint(0, 100, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_MoveIsoNetNode() #3: expected failure\n");
	  *retcode = -121;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - coincident node.") != 0)
      {
	  fprintf (stderr, "ST_MoveIsoNetNode() #3: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -122;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to move a Node (coincident link) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNetNode('roads', 3, MakePoint(0, 50, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_MoveIsoNetNode() #4: expected failure\n");
	  *retcode = -123;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - link crosses node.") != 0)
      {
	  fprintf (stderr, "ST_MoveIsoNetNode() #4: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -124;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert a Link (bad StartNode) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 1, 3, GeometryFromText('LINESTRING(1 1, 100 100)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() #2: expected failure\n");
	  *retcode = -125;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - start node not geometry start point.") !=
	0)
      {
	  fprintf (stderr, "ST_AddLink() #2: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -126;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert a Link (bad EndNode) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 1, 3, GeometryFromText('LINESTRING(0 0, 99 99)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() #3: expected failure\n");
	  *retcode = -127;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - end node not geometry end point.") != 0)
      {
	  fprintf (stderr, "ST_AddLink() #3: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -128;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert a Link (coincident Node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 1, 3, GeometryFromText('LINESTRING(0 0, 0 100, 100 100)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() #4: expected failure\n");
	  *retcode = -129;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - geometry crosses a node.")
	!= 0)
      {
	  fprintf (stderr, "ST_AddLink() #4: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -130;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert a Link (self-closed ring) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 1, 1, GeometryFromText('LINESTRING(0 0, 0 100, 100 100, 0 0)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() #5: expected failure\n");
	  *retcode = -131;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - self-closed links are forbidden.") != 0)
      {
	  fprintf (stderr, "ST_AddLink() #5: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -132;
	  return 0;
      }
    sqlite3_free (err_msg);

/* inserting a second Link */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 1, 3, GeometryFromText('LINESTRING(0 0, 100 100)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() #5 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -133;
	  return 0;
      }

/* attempting to move a Link (coincident node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ChangeLinkGeom('roads', 2, GeometryFromText('LINESTRING(0 0, 0 100, 100 100)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ChangeLinkGeom() #1: expected failure\n");
	  *retcode = -134;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - geometry crosses a node.")
	!= 0)
      {
	  fprintf (stderr, "ST_ChangeLinkGeom() #1: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -135;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to move a Link (bad start node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ChangeLinkGeom('roads', 2, GeometryFromText('LINESTRING(1 1, 100 100)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ChangeLinkGeom() #2: expected failure\n");
	  *retcode = -136;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - start node not geometry start point.") !=
	0)
      {
	  fprintf (stderr, "ST_ChangeLinkGeom() #2: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -138;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to move a Link (bad end node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ChangeLinkGeom('roads', 2, GeometryFromText('LINESTRING(0 0, 101 101)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ChangeLinkGeom() #3: expected failure\n");
	  *retcode = -139;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - end node not geometry end point.") != 0)
      {
	  fprintf (stderr, "ST_ChangeLinkGeom() #3: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -140;
	  return 0;
      }
    sqlite3_free (err_msg);

/* moving the second Link */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ChangeLinkGeom('roads', 2, GeometryFromText('LINESTRING(0 0, 0 50, 100 100)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_ChangeLinkGeom() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -141;
	  return 0;
      }

/* attempting to move a Node (non-isolated) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNetNode('roads', 3, MakePoint(0, 50, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_MoveIsoNetNode() #5: expected failure\n");
	  *retcode = -142;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - not isolated node.") != 0)
      {
	  fprintf (stderr, "ST_MoveIsoNetNode() #5: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -143;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to remove a Node (non-isolated) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_RemIsoNetNode('roads', 3)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_RemIsoNetNode() #5: expected failure\n");
	  *retcode = -144;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - not isolated node.") != 0)
      {
	  fprintf (stderr, "ST_RemIsoNetNode() #5: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -145;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to remove a Link (not existing) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_RemoveLink('roads', 33)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_RemoveLink() #1: expected failure\n");
	  *retcode = -146;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent link.") != 0)
      {
	  fprintf (stderr, "ST_RemoveLink() #1: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -147;
	  return 0;
      }
    sqlite3_free (err_msg);

/* removing the second Link */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_RemoveLink('roads', 2)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_RemoveLink() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -148;
	  return 0;
      }

/* removing the third Node */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_RemIsoNetNode('roads', 3)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_RemIsoNetNode() #6 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -149;
	  return 0;
      }

    return 1;
}

static int
do_level0_tests (sqlite3 * handle, int *retcode)
{
/* performing basic tests: Level 0 */
    int ret;
    char *err_msg = NULL;

/* attempting to insert a Node (invalid SRID) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('roads', MakePoint(0, 10, 3003))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_AddIsoNetNode() invalid SRID: expected failure\n");
	  *retcode = -10;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_AddIsoNetNode() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -11;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert a Node (invalid DIMs) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('roads', MakePointZ(0, 10, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_AddIsoNetNode() invalid SRID: expected failure\n");
	  *retcode = -12;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_AddIsoNetNode() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -13;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to move a Node (invalid SRID) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNetNode('roads', 1, MakePoint(0, 10, 3003))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_MoveIsoNetNode() invalid SRID: expected failure\n");
	  *retcode = -14;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_MoveIsoNetNode() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -15;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to move a Node (invalid DIMs) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNetNode('roads', 1, MakePointZ(0, 10, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_MoveIsoNetNode() invalid SRID: expected failure\n");
	  *retcode = -16;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_MoveIsoNetNode() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -17;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert a Link (invalid SRID) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 1, 2, GeomFromText('LINESTRING(-40 -50,  -50 -40)', 3003))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() invalid SRID: expected failure\n");
	  *retcode = -18;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr, "ST_AddLink() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -19;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert a Link (invalid DIMs) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 1, 2, GeomFromText('LINESTRINGZ(-40 -50 1,  -50 -40 2)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() invalid SRID: expected failure\n");
	  *retcode = -20;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr, "ST_AddLink() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -21;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to move a Link (invalid SRID) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ChangeLinkGeom('roads', 1, GeomFromText('LINESTRING(-40 -50,  -50 -40)', 3003))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_ChangeLinkGeom() invalid SRID: expected failure\n");
	  *retcode = -22;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_ChangeLinkGeom() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -23;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to move a Link (invalid DIMs) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ChangeLinkGeom('roads', 1, GeomFromText('LINESTRINGZ(-40 -50 1,  -50 -40 2)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_ChangeLinkGeom() invalid SRID: expected failure\n");
	  *retcode = -24;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_ChangeLinkGeom() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -25;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split a Link (invalid SRID) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewGeoLinkSplit('roads', 1, MakePoint(0, 10, 3003))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_NewGeoLinkSplit() invalid SRID: expected failure\n");
	  *retcode = -26;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_NewGeoLinkSplit() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -27;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split a Link (invalid DIMs) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewGeoLinkSplit('roads', 1, MakePointZ(0, 10, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_NewGeoLinkSplit() invalid SRID: expected failure\n");
	  *retcode = -28;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_NewGeoLinkSplit() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -29;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split a Link (invalid SRID) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewGeoLinkSplit('roads', 1, MakePoint(0, 10, 3003))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_ModGeoLinkSplit() invalid SRID: expected failure\n");
	  *retcode = -30;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_ModGeoLinkSplit() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -31;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split a Link (invalid DIMs) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModGeoLinkSplit('roads', 1, MakePointZ(0, 10, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_ModGeoLinkSplit() invalid SRID: expected failure\n");
	  *retcode = -32;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_ModGeoLinkSplit() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -33;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert a Node (int geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('roads', 1)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_AddIsoNetNode() INTEGER Geometry: expected failure\n");
	  *retcode = -34;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr,
		   "ST_AddIsoNetNode() INTEGER Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -35;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert a Node (NULL geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('roads', NULL)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_AddIsoNetNode() NULL Geometry: expected failure\n");
	  *retcode = -36;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - Spatial Network can't accept null geometry.")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_AddIsoNetNode() NULL Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -37;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert a Node (Linestring geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('roads', GeomFromText('LINESTRING(-40 -50,  -50 -40)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_AddIsoNetNode() LINESTRING Geometry: expected failure\n");
	  *retcode = -38;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr,
		   "ST_AddIsoNetNode() LINESTRING Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -39;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert a Node (Polygon geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('roads', GeomFromText('POLYGON((-40 -40, -40 -50, -50 -50, -50 -40, -40 -40))', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_AddIsoNetNode() POLYGON Geometry: expected failure\n");
	  *retcode = -40;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr,
		   "ST_AddIsoNetNode() POLYGON Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -41;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert a Node (MultiPoint geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('roads', GeomFromText('MULTIPOINT(-40 -50,  -50 -40)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_AddIsoNetNode() MULTIPOLYGON Geometry: expected failure\n");
	  *retcode = -42;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr,
		   "ST_AddIsoNetNode() MULTIPOLYGON Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -43;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to move a Node (int geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNetNode('roads', 1, 1)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_MoveIsoNetNode() INTEGER Geometry: expected failure\n");
	  *retcode = -44;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr,
		   "ST_MoveIsoNetNode() INTEGER Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -45;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to move a Node (NULL geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNetNode('roads', 1, NULL)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_MoveIsoNetNode() NULL Geometry: expected failure\n");
	  *retcode = -46;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - Spatial Network can't accept null geometry.")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_MoveIsoNetNode() NULL Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -47;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to move a Node (Linestring geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNetNode('roads', 1, GeomFromText('LINESTRING(-40 -50,  -50 -40)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_MoveIsoNetNode() LINESTRING Geometry: expected failure\n");
	  *retcode = -48;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr,
		   "ST_MoveIsoNetNode() LINESTRING Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -49;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to move a Node (Polygon geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNetNode('roads', 1, GeomFromText('POLYGON((-40 -40, -40 -50, -50 -50, -50 -40, -40 -40))', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_MoveIsoNetNode() POLYGON Geometry: expected failure\n");
	  *retcode = -50;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr,
		   "ST_MoveIsoNetNode() POLYGON Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -51;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to move a Node (MultiPoint geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNetNode('roads', 1, GeomFromText('MULTIPOINT(-40 -50,  -50 -40)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_MoveIsoNetNode() MULTIPOLYGON Geometry: expected failure\n");
	  *retcode = -52;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr,
		   "ST_MoveIsoNetNode() MULTIPOLYGON Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -53;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert a Link (int geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 1, 2, 1)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() INTEGER Geometry: expected failure\n");
	  *retcode = -54;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr, "ST_AddLink() INTEGER Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -55;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert a Link (NULL geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 1, 2, NULL)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() NULL Geometry: expected failure\n");
	  *retcode = -56;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - Spatial Network can't accept null geometry.")
	!= 0)
      {
	  fprintf (stderr, "ST_AddLink() NULL Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -57;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert a Link (Point geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 1, 2, MakePoint(1, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() POINT Geometry: expected failure\n");
	  *retcode = -58;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr, "ST_AddLink() POINT Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -59;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert a Link (Polygon geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 1, 2, GeomFromText('POLYGON((-40 -40, -40 -50, -50 -50, -50 -40, -40 -40))', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() POLYGON Geometry: expected failure\n");
	  *retcode = -60;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr, "ST_AddLink() POLYGON Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -61;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert a Link (MultiLinestring geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 1, 2, GeomFromText('MULTILINESTRING((-40 -50,  -50 -40), (-20 -20, -30 -30))', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_AddLink() MULTIPOLYGON Geometry: expected failure\n");
	  *retcode = -62;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr,
		   "ST_AddLink() MULTIPOLYGON Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -63;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to change a Link (int geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ChangeLinkGeom('roads', 1, 1)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_ChangeLinkGeom() INTEGER Geometry: expected failure\n");
	  *retcode = -64;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr,
		   "ST_ChangeLinkGeom() INTEGER Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -65;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to change a Link (NULL geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ChangeLinkGeom('roads', 1, NULL)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_ChangeLinkGeom() NULL Geometry: expected failure\n");
	  *retcode = -66;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - Spatial Network can't accept null geometry.")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_ChangeLinkGeom() NULL Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -67;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to change a Link (Point geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ChangeLinkGeom('roads', 1, MakePoint(1, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_ChangeLinkGeom() POINT Geometry: expected failure\n");
	  *retcode = -68;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr,
		   "ST_ChangeLinkGeom() POINT Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -69;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to change a Link (Polygon geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ChangeLinkGeom('roads', 1, GeomFromText('POLYGON((-40 -40, -40 -50, -50 -50, -50 -40, -40 -40))', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_ChangeLinkGeom() POLYGON Geometry: expected failure\n");
	  *retcode = -70;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr,
		   "ST_ChangeLinkGeom() POLYGON Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -71;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to change a Link (MultiLinestring geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ChangeLinkGeom('roads', 1, GeomFromText('MULTILINESTRING((-40 -50,  -50 -40), (-20 -20, -30 -30))', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_ChangeLinkGeom() MULTIPOLYGON Geometry: expected failure\n");
	  *retcode = -72;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr,
		   "ST_ChangeLinkGeom() MULTIPOLYGON Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -73;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split a Link (int geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewGeoLinkSplit('roads', 1, 1)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_NewGeoLinkSplit() INTEGER Geometry: expected failure\n");
	  *retcode = -74;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr,
		   "ST_NewGeoLinkSplit() INTEGER Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -75;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split a Link (NULL geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewGeoLinkSplit('roads', 1, NULL)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_NewGeoLinkSplit() NULL Geometry: expected failure\n");
	  *retcode = -76;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - Spatial Network can't accept null geometry.")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_NewGeoLinkSplit() NULL Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -77;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split a Link (Linestring geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewGeoLinkSplit('roads', 1, GeomFromText('LINESTRING(-40 -50,  -50 -40)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_NewGeoLinkSplit() LINESTRING Geometry: expected failure\n");
	  *retcode = -78;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr,
		   "ST_NewGeoLinkSplit() LINESTRING Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -79;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split a Link (Polygon geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewGeoLinkSplit('roads', 1, GeomFromText('POLYGON((-40 -40, -40 -50, -50 -50, -50 -40, -40 -40))', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_NewGeoLinkSplit() POLYGON Geometry: expected failure\n");
	  *retcode = -80;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr,
		   "ST_NewGeoLinkSplit() POLYGON Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -81;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split a Link (MultiPoint geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewGeoLinkSplit('roads', 1, GeomFromText('MULTIPOINT(-40 -50,  -50 -40)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_NewGeoLinkSplit() MULTIPOLYGON Geometry: expected failure\n");
	  *retcode = -82;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr,
		   "ST_NewGeoLinkSplit() MULTIPOLYGON Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -83;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split a Link (int geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModGeoLinkSplit('roads', 1, 1)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_ModGeoLinkSplit() INTEGER Geometry: expected failure\n");
	  *retcode = -84;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr,
		   "ST_ModGeoLinkSplit() INTEGER Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -85;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split a Link (NULL geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModGeoLinkSplit('roads', 1, NULL)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_ModGeoLinkSplit() NULL Geometry: expected failure\n");
	  *retcode = -86;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - Spatial Network can't accept null geometry.")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_ModGeoLinkSplit() NULL Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -87;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split a Link (Linestring geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModGeoLinkSplit('roads', 1, GeomFromText('LINESTRING(-40 -50,  -50 -40)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_ModGeoLinkSplit() LINESTRING Geometry: expected failure\n");
	  *retcode = -88;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr,
		   "ST_ModGeoLinkSplit() LINESTRING Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -89;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split a Link (Polygon geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModGeoLinkSplit('roads', 1, GeomFromText('POLYGON((-40 -40, -40 -50, -50 -50, -50 -40, -40 -40))', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_ModGeoLinkSplit() POLYGON Geometry: expected failure\n");
	  *retcode = -90;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr,
		   "ST_ModGeoLinkSplit() POLYGON Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -91;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split a Link (MultiPoint geometry) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModGeoLinkSplit('roads', 1, GeomFromText('MULTIPOINT(-40 -50,  -50 -40)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_ModGeoLinkSplit() MULTIPOLYGON Geometry: expected failure\n");
	  *retcode = -92;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr,
		   "ST_ModGeoLinkSplit() MULTIPOLYGON Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -93;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split a Link */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewLogLinkSplit('roads', 1)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_NewLogLinkSplit(): expected failure\n");
	  *retcode = -94;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - ST_NewLogLinkSplit can't support Spatial Network; try using ST_NewGeoLinkSplit.")
	!= 0)
      {
	  fprintf (stderr, "ST_NewLogLinkSplit(): unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -95;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split a Link */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModLogLinkSplit('roads', 1)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ModLogLinkSplit(): expected failure\n");
	  *retcode = -96;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - ST_ModLogLinkSplit can't support Spatial Network; try using ST_ModGeoLinkSplit.")
	!= 0)
      {
	  fprintf (stderr, "ST_ModLogLinkSplit(): unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -97;
	  return 0;
      }
    sqlite3_free (err_msg);

    return 1;
}

#endif

int
main (int argc, char *argv[])
{
    int retcode = 0;

#ifdef ENABLE_RTTOPO		/* only if RTTOPO is enabled */
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    void *cache = spatialite_alloc_connection ();

    ret =
	sqlite3_open_v2 (":memory:", &handle,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open \":memory:\" database: %s\n",
		   sqlite3_errmsg (handle));
	  sqlite3_close (handle);
	  return -1;
      }

    spatialite_init_ex (handle, cache, 0);

    ret = sqlite3_exec (handle, "PRAGMA foreign_keys=1", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "PRAGMA foreign_keys=1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -2;
      }

    ret =
	sqlite3_exec (handle, "SELECT InitSpatialMetadata(1)", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -3;
      }

/* creating a Network 2D */
    ret =
	sqlite3_exec (handle, "SELECT CreateNetwork('roads', 1, 4326, 0, 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateNetwork() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -4;
      }

/* basic tests: level 0 */
    if (!do_level0_tests (handle, &retcode))
	goto end;

/* basic tests: level 1 */
    if (!do_level1_tests (handle, &retcode))
	goto end;

/* basic tests: level 2 */
    if (!do_level2_tests (handle, &retcode))
	goto end;

#ifdef ENABLE_LIBXML2		/* only if LIBXML2 is supported */

/* testing RegisterTopoGeoCoverage */
    if (!do_level3_tests (handle, &retcode))
	goto end;

#endif

/* dropping the Network 2D */
    ret =
	sqlite3_exec (handle, "SELECT DropNetwork('roads')", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DropNetwork() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -5;
      }

  end:
    spatialite_finalize_topologies (cache);
    sqlite3_close (handle);
    spatialite_cleanup_ex (cache);

#endif /* end RTTOPO conditional */

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    spatialite_shutdown ();
    return retcode;
}
