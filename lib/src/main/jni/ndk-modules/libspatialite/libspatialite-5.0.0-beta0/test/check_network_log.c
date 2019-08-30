/*

 check_network3d.c -- SpatiaLite Test Case

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
do_level2_tests (sqlite3 * handle, int *retcode)
{
/* performing basic tests: Level 2 */
    int ret;
    char *err_msg = NULL;

/* inserting three Nodes */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('roads', NULL)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNetNode() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -150;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('roads', NULL)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNetNode() #5 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -151;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('roads', NULL)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNetNode() #6 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -152;
	  return 0;
      }

/* inserting two Links */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 5, 4, NULL)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -153;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 5, 6, NULL)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() #5 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -154;
	  return 0;
      }

/* splitting a Link (New) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewLogLinkSplit('roads', 3)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_NewLogLinkSplit() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -155;
	  return 0;
      }

/* splitting a Link (Mod) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModLogLinkSplit('roads', 4)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_ModLogLinkSplit() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -156;
	  return 0;
      }

/* attempting to split a link (non-existing link) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewLogLinkSplit('roads', 333)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_NewLogLinkSplit() #2: expected failure\n");
	  *retcode = -157;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent link.") != 0)
      {
	  fprintf (stderr, "ST_NewLogLinkSplit() #2: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -158;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split a link (non-existing link) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModLogLinkSplit('roads', 333)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ModLogLinkSplit() #2: expected failure\n");
	  *retcode = -159;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent link.") != 0)
      {
	  fprintf (stderr, "ST_ModLogLinkSplit() #2: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -160;
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
	  *retcode = -161;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - Cannot heal link with itself.") != 0)
      {
	  fprintf (stderr, "ST_NewLinkSplit() #1: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -162;
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
	  *retcode = -163;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - Cannot heal link with itself.") != 0)
      {
	  fprintf (stderr, "ST_ModLinkSplit() #1: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -164;
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
	  *retcode = -165;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent first link.")
	!= 0)
      {
	  fprintf (stderr, "ST_NewLinkHeal() #2: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -166;
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
	  *retcode = -167;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent first link.")
	!= 0)
      {
	  fprintf (stderr, "ST_ModLinkHeal() #2: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -168;
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
	  *retcode = -169;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent second link.")
	!= 0)
      {
	  fprintf (stderr, "ST_NewLinkHeal() #3: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -170;
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
	  *retcode = -171;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent second link.")
	!= 0)
      {
	  fprintf (stderr, "ST_ModLinkHeal() #3: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -172;
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
	  *retcode = -173;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-connected links.") !=
	0)
      {
	  fprintf (stderr, "ST_NewLinkHeal() #4: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -174;
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
	  *retcode = -175;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-connected links.") !=
	0)
      {
	  fprintf (stderr, "ST_ModLinkHeal() #4: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -176;
	  return 0;
      }
    sqlite3_free (err_msg);

/* inserting one more Link */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 5, 2, NULL)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() #6 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -177;
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
	  *retcode = -178;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - other links connected.") !=
	0)
      {
	  fprintf (stderr, "ST_NewLinkHeal() #5: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -179;
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
	  *retcode = -180;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - other links connected.") !=
	0)
      {
	  fprintf (stderr, "ST_ModLinkHeal() #5: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -181;
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
	  *retcode = -182;
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
	  *retcode = -183;
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
	  *retcode = -184;
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
	  *retcode = -185;
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
	  *retcode = -186;
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
	  *retcode = -187;
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
	  *retcode = -188;
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
		      "SELECT ST_AddIsoNetNode('roads', NULL)",
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
		      "SELECT ST_AddIsoNetNode('roads', NULL)",
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
		      "SELECT ST_AddLink('roads', 1, 2, NULL)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -112;
	  return 0;
      }

/* inserting a third Node */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('roads', NULL)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNetNode() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -113;
	  return 0;
      }

/* moving the third Node */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNetNode('roads', 3, NULL)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_MoveIsoNetNode() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -114;
	  return 0;
      }

/* attempting to move a Node (non-existing node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNetNode('roads', 333, NULL)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_MoveIsoNetNode() #2: expected failure\n");
	  *retcode = -115;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent node.") != 0)
      {
	  fprintf (stderr, "ST_MoveIsoNetNode() #2: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -116;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert a Link (self-closed ring) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 1, 1, NULL)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() #2: expected failure\n");
	  *retcode = -117;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - self-closed links are forbidden.") != 0)
      {
	  fprintf (stderr, "ST_AddLink() #2: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -118;
	  return 0;
      }
    sqlite3_free (err_msg);

/* inserting a second Link */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 1, 3, NULL)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -119;
	  return 0;
      }

/* moving the second Link */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ChangeLinkGeom('roads', 2, NULL)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_ChangeLinkGeom() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -120;
	  return 0;
      }

/* attempting to move a Node (non-isolated) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNetNode('roads', 3, NULL)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_MoveIsoNetNode() #3: expected failure\n");
	  *retcode = -121;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - not isolated node.") != 0)
      {
	  fprintf (stderr, "ST_MoveIsoNetNode() #3: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -122;
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
	  fprintf (stderr, "ST_RemIsoNetNode() #1: expected failure\n");
	  *retcode = -123;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - not isolated node.") != 0)
      {
	  fprintf (stderr, "ST_RemIsoNetNode() #1: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -124;
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
	  *retcode = -125;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent link.") != 0)
      {
	  fprintf (stderr, "ST_RemoveLink() #1: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -126;
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
	  *retcode = -127;
	  return 0;
      }

/* removing the third Node */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_RemIsoNetNode('roads', 3)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_RemIsoNetNode() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -128;
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

/* attempting to insert a Node (not null geom) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('roads', MakePointZ(0, 10, 1, 3003))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_AddIsoNetNode() NotNULL Geom: expected failure\n");
	  *retcode = -10;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - Logical Network can't accept not null geometry.")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_AddIsoNetNode() NotNULL Geom: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -11;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to move a Node (not null geom) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNetNode('roads', 1, MakePointZ(0, 10, 1, 3003))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_MoveIsoNetNode() NotNULL Geom: expected failure\n");
	  *retcode = -12;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - Logical Network can't accept not null geometry.")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_MoveIsoNetNode() NotNULL Geom: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -13;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert a Link (not null geom) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('roads', 1, 2, GeomFromText('LINESTRINGZ(-40 -50 1,  -50 -40 1)', 3003))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() NotNULL Geom: expected failure\n");
	  *retcode = -14;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - Logical Network can't accept not null geometry.")
	!= 0)
      {
	  fprintf (stderr, "ST_AddLink() NotNULL Geom: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -15;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to move a Link (not null geom) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ChangeLinkGeom('roads', 1, GeomFromText('LINESTRINGZ(-40 -50 1,  -50 -40 1)', 3003))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_ChangeLinkGeom() NotNULL Geom: expected failure\n");
	  *retcode = -16;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - Logical Network can't accept not null geometry.")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_ChangeLinkGeom() NotNULL Geom: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -17;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split a Link (not null geom) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewGeoLinkSplit('roads', 1, MakePointZ(0, 10, 1, 3003))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_NewGeoLinkSplit() NotNULL Geom: expected failure\n");
	  *retcode = -18;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - ST_NewGeoLinkSplit can't support Logical Network; try using ST_NewLogLinkSplit.")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_NewGeoLinkSplit() NotNULL Geom: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -19;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split a Link (Geom) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewGeoLinkSplit('roads', 1, MakePointZ(0, 10, 1, 3003))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_ModGeoLinkSplit() invalid SRID: expected failure\n");
	  *retcode = -20;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - ST_NewGeoLinkSplit can't support Logical Network; try using ST_NewLogLinkSplit.")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_ModGeoLinkSplit() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -21;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to retrieve a NetNode by Point (Logical Network) */
    ret =
	sqlite3_exec (handle,
		      "SELECT GetNetNodeByPoint('roads', MakePoint(1, 100), 3.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "GetNetNodeByPoint() #1: expected failure\n");
	  *retcode = -22;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "GetNetNodekByPoint() cannot be applied to Logical Network.") != 0)
      {
	  fprintf (stderr, "GetNetNodeByPoint() #1: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -23;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to retrieve a Link by Point (Logical Network) */
    ret =
	sqlite3_exec (handle,
		      "SELECT GetLinkByPoint('roads', MakePoint(1, 95), 3.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "GetLinkByPoint() #1: expected failure\n");
	  *retcode = -24;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "GetLinkByPoint() cannot be applied to Logical Network.") != 0)
      {
	  fprintf (stderr, "GetLinkByPoint() #1: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -25;
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

    ret =
	sqlite3_exec (handle, "SELECT InitSpatialMetadata(1)", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -2;
      }

/* creating a Logical Network */
    ret =
	sqlite3_exec (handle, "SELECT CreateNetwork('roads', 0)", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateNetwork() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -3;
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

/* dropping the Logical Network */
    ret =
	sqlite3_exec (handle, "SELECT DropNetwork('roads')", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DropNetwork() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -4;
      }

  end:
    sqlite3_close (handle);
    spatialite_cleanup_ex (cache);

#endif /* end RTTOPO conditional */

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    spatialite_shutdown ();
    return retcode;
}
