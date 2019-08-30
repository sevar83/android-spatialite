/*

 routing_test.c -- SpatiaLite Test Case

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

#ifndef OMIT_GEOS		/* GEOS is supported */

static int
perform_bad_test (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing a generic invalid test */
    int ret;
    int result = 1;

    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		result = sqlite3_column_int (stmt, 0);
	    }
	  else
	    {
		fprintf (stderr, "CreateRouting Invalid: %s\n",
			 sqlite3_errmsg (handle));
		result = 0;
		break;
	    }
      }
    if (result)
	return -2;

    return 0;
}

static int
do_test_invalid_table (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - wrong table-name */
    sqlite3_bind_text (stmt, 1, "invalid", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "oneway_from_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "oneway_to_from", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_invalid_from (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - wrong node-from */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "roads", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "invalid", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "oneway_from_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "oneway_to_from", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_invalid_to (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - wrong node-to */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "roads", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "invalid", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "oneway_from_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "oneway_to_from", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_invalid_geom (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - wrong node-to */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "roads", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "invalid", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "oneway_from_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "oneway_to_from", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_invalid_cost (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - wrong road-name */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "roads", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "invalid", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "oneway_from_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "oneway_to_from", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_invalid_name (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - wrong road-name */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "roads", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "invalid", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "oneway_from_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "oneway_to_from", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_invalid_oneway_from (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - wrong oneway-from */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "roads", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "invalid", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "oneway_to_from", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_invalid_oneway_to (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - wrong oneway-to */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "roads", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "oneway_to_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "invalid", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_bad_1 (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - bad input table #1 */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "bad_roads_1", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "oneway_to_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "oneway_from_to", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_bad_2 (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - bad input table #2 */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "bad_roads_2", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "oneway_to_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "oneway_from_to", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_bad_3 (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - bad input table #3 */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "bad_roads_3", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "oneway_to_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "oneway_from_to", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_bad_4 (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - bad input table #4 */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "bad_roads_4", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "oneway_to_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "oneway_from_to", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_bad_5 (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - bad input table #5 */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "bad_roads_5", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "oneway_to_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "oneway_from_to", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_bad_6 (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - bad input table #6 */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "bad_roads_6", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "oneway_to_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "oneway_from_to", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_bad_7 (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - bad input table #7 */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "bad_roads_7", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "oneway_to_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "oneway_from_to", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_bad_8 (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - bad input table #8 */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "bad_roads_8", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "oneway_to_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "oneway_from_to", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_bad_9 (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - bad input table #9 */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "bad_roads_9", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "oneway_to_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "oneway_from_to", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_bad_10 (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - bad input table #10 */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "bad_roads_10", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "oneway_to_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "oneway_from_to", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_bad_11 (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - bad input table #11 */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "bad_roads_11", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "oneway_to_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "oneway_from_to", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_bad_12 (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - bad input table #12 */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "bad_roads_12", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "oneway_to_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "oneway_from_to", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_bad_13 (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - bad input table #13 */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "bad_roads_13", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "oneway_to_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "oneway_from_to", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_bad_14 (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - bad input table #14 */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "bad_roads_14", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "oneway_to_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "oneway_from_to", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_bad_15 (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - bad input table #15 */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "bad_roads_15", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 1);
    sqlite3_bind_text (stmt, 8, "oneway_to_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 9, "oneway_from_to", -1, SQLITE_TRANSIENT);
    return perform_bad_test (handle, stmt);
}

static int
do_test_bad_16 (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* performing an invalid testcase - bad input table #16 */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, "bad_roads_16", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 2, "node_from", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 3, "node_to", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 4, "geom", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 5, "cost", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (stmt, 6, "road_name", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 7, 0);
    sqlite3_bind_null (stmt, 8);
    sqlite3_bind_null (stmt, 9);
    return perform_bad_test (handle, stmt);
}

static int
do_test_invalid (sqlite3 * handle)
{
/* performing invalid testcases */
    const char *sql;
    sqlite3_stmt *stmt = NULL;
    int ret;

    sql =
	"SELECT CreateRouting('invalid_data', 'invalid', ?, ?, ?, ?, ?, ?, 0, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateRouting #3: %s\n", sqlite3_errmsg (handle));
	  return -1;
      }

    if (do_test_invalid_table (handle, stmt) != 0)
	return -1;
    if (do_test_invalid_from (handle, stmt) != 0)
	return -1;
    if (do_test_invalid_to (handle, stmt) != 0)
	return -1;
    if (do_test_invalid_geom (handle, stmt) != 0)
	return -1;
    if (do_test_invalid_cost (handle, stmt) != 0)
	return -1;
    if (do_test_invalid_name (handle, stmt) != 0)
	return -1;
    if (do_test_invalid_oneway_from (handle, stmt) != 0)
	return -1;
    if (do_test_invalid_oneway_to (handle, stmt) != 0)
	return -1;

    if (do_test_bad_1 (handle, stmt) != 0)
	return -1;
    if (do_test_bad_2 (handle, stmt) != 0)
	return -1;
    if (do_test_bad_3 (handle, stmt) != 0)
	return -1;
    if (do_test_bad_4 (handle, stmt) != 0)
	return -1;
    if (do_test_bad_5 (handle, stmt) != 0)
	return -1;
    if (do_test_bad_6 (handle, stmt) != 0)
	return -1;
    if (do_test_bad_7 (handle, stmt) != 0)
	return -1;
    if (do_test_bad_8 (handle, stmt) != 0)
	return -1;
    if (do_test_bad_9 (handle, stmt) != 0)
	return -1;
    if (do_test_bad_10 (handle, stmt) != 0)
	return -1;
    if (do_test_bad_11 (handle, stmt) != 0)
	return -1;
    if (do_test_bad_12 (handle, stmt) != 0)
	return -1;
    if (do_test_bad_13 (handle, stmt) != 0)
	return -1;
    if (do_test_bad_14 (handle, stmt) != 0)
	return -1;
    if (do_test_bad_15 (handle, stmt) != 0)
	return -1;
    if (do_test_bad_16 (handle, stmt) != 0)
	return -1;

    sqlite3_finalize (stmt);
    return 0;
}

static int
do_create_spatial_index (sqlite3 * handle, const char *geom)
{
/* creating a Spatial Index */
    int ok = 0;
    sqlite3_stmt *stmt = NULL;
    char *sql =
	sqlite3_mprintf ("SELECT CreateSpatialIndex('roads', %Q)", geom);
    int ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateSpatialIndex: roads.%s #1: %s\n", geom,
		   sqlite3_errmsg (handle));
	  return 0;
      }
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	      ok = sqlite3_column_int (stmt, 0);
	  else
	    {
		fprintf (stderr, "CreateSpatialIndex: roads.%s #2: %s\n", geom,
			 sqlite3_errmsg (handle));
		ok = 0;
		break;
	    }
      }
    sqlite3_finalize (stmt);
    return ok;
}

static void
set_algorithm (sqlite3_stmt * stmt, int algorithm)
{
/* setting a routing algorithm */

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    if (algorithm == 1)
	sqlite3_bind_text (stmt, 1, "A*", -1, SQLITE_STATIC);
    else
	sqlite3_bind_text (stmt, 1, "DIJKSTRA", -1, SQLITE_STATIC);
    sqlite3_step (stmt);
}

static void
set_request (sqlite3_stmt * stmt, int request)
{
/* setting a routing request */

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    if (request == 1)
	sqlite3_bind_text (stmt, 1, "TSP NN", -1, SQLITE_STATIC);
    else if (request == 2)
	sqlite3_bind_text (stmt, 1, "TSP GA", -1, SQLITE_STATIC);
    else
	sqlite3_bind_text (stmt, 1, "SHORTEST PATH", -1, SQLITE_STATIC);
    sqlite3_step (stmt);
}

static void
set_option (sqlite3_stmt * stmt, int option)
{
/* setting a routing option */

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    if (option == 1)
	sqlite3_bind_text (stmt, 1, "NO LINKS", -1, SQLITE_STATIC);
    else if (option == 2)
	sqlite3_bind_text (stmt, 1, "NO GEOMETRIES", -1, SQLITE_STATIC);
    else if (option == 3)
	sqlite3_bind_text (stmt, 1, "SIMPLE", -1, SQLITE_STATIC);
    else
	sqlite3_bind_text (stmt, 1, "FULL", -1, SQLITE_STATIC);
    sqlite3_step (stmt);
}

static int
do_routing (sqlite3 * handle, sqlite3_stmt * stmt, const char *base_name,
	    int which, int multi)
{
/* testing a routing solution */
    int ret;
    int ind = strlen (base_name) - 4;
    int has_ids = 0;
    const char *list_codes =
	"RT05301806761GZ,RT05301806955GZ,RT05301806819GZ,RT05301806691GZ,RT05301806772GZ,RT05301806760GZ,RT05301806624GZ";
    const char *list_ids = "58,133,295,352,203,235,342";
    int count = 0;

    if (base_name[ind] == 'i')
	has_ids = 1;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    if (has_ids)
      {
	  if (which == 0)
	    {
		sqlite3_bind_int (stmt, 1, 273);
		if (multi == 0)
		    sqlite3_bind_int (stmt, 2, 352);
		else
		    sqlite3_bind_text (stmt, 2, list_ids, strlen (list_ids),
				       SQLITE_STATIC);
	    }
	  else
	    {
		if (multi == 0)
		    sqlite3_bind_int (stmt, 1, 352);
		else
		    sqlite3_bind_text (stmt, 1, list_ids, strlen (list_ids),
				       SQLITE_STATIC);
		sqlite3_bind_int (stmt, 2, 317);
	    }
      }
    else
      {
	  if (which == 0)
	    {
		sqlite3_bind_text (stmt, 1, "RT05301806875GZ", -1,
				   SQLITE_STATIC);
		if (multi == 0)
		    sqlite3_bind_text (stmt, 2, "RT05301806761GZ", -1,
				       SQLITE_STATIC);
		else
		    sqlite3_bind_text (stmt, 2, list_codes, strlen (list_codes),
				       SQLITE_STATIC);
	    }
	  else
	    {
		if (multi == 0)
		    sqlite3_bind_text (stmt, 1, "RT05301806761GZ", -1,
				       SQLITE_STATIC);
		else
		    sqlite3_bind_text (stmt, 1, list_codes, strlen (list_codes),
				       SQLITE_STATIC);
		sqlite3_bind_text (stmt, 2, "RT05301806875GZ", -1,
				   SQLITE_STATIC);
	    }
      }

    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	      count++;
	  else
	    {
		fprintf (stderr, "CreateRouting #2: %s\n",
			 sqlite3_errmsg (handle));
		break;
	    }
      }
    return count;
}

static int
do_range (sqlite3 * handle, sqlite3_stmt * stmt, const char *base_name,
	  int which, int cost)
{
/* testing a range solution */
    int ret;
    int ind = strlen (base_name) - 4;
    int has_ids = 0;
    int count = 0;

    if (base_name[ind] == 'i')
	has_ids = 1;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    if (which == 0)
      {
	  if (has_ids)
	    {
		sqlite3_bind_int (stmt, 1, 154);
	    }
	  else
	    {
		sqlite3_bind_text (stmt, 1, "RT05301806955GZ", -1,
				   SQLITE_STATIC);
	    }
	  if (cost == 0)
	      sqlite3_bind_double (stmt, 2, 50.0);
	  else
	      sqlite3_bind_int (stmt, 2, 50);
      }
    else
      {
	  if (cost == 0)
	      sqlite3_bind_double (stmt, 1, 50.0);
	  else
	      sqlite3_bind_int (stmt, 1, 50);
	  if (has_ids)
	    {
		sqlite3_bind_int (stmt, 2, 154);
	    }
	  else
	    {
		sqlite3_bind_text (stmt, 2, "RT05301806955GZ", -1,
				   SQLITE_STATIC);
	    }
      }

    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	      count++;
	  else
	    {
		fprintf (stderr, "CreateRouting #2: %s\n",
			 sqlite3_errmsg (handle));
		break;
	    }
      }
    return count;
}

static int
do_point2point (sqlite3 * handle, sqlite3_stmt * stmt)
{
/* testing a poin2point solution */
    int ret;
    int count = 0;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);

    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	      count++;
	  else
	    {
		fprintf (stderr, "CreateRouting #2: %s\n",
			 sqlite3_errmsg (handle));
		break;
	    }
      }
    return count;
}

static int
do_create_routing_nodes (sqlite3 * handle)
{
/* adding "id_node_from" and "id_node_to" columns */
    int result = 0;
    int ret;
    sqlite3_stmt *stmt;
    const char *sql =
	"SELECT CreateRoutingNodes(NULL, 'roads', 'geom', 'id_node_from', 'id_node_to')";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateRoutingNodes #1: %s\n",
		   sqlite3_errmsg (handle));
	  return -1;
      }
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		result = sqlite3_column_int (stmt, 0);
	    }
	  else
	    {
		fprintf (stderr, "CreateRoutingNodes #2: %s\n",
			 sqlite3_errmsg (handle));
		break;
	    }
      }
    sqlite3_finalize (stmt);
    if (!result)
	return -2;
    return 1;
}

static int
do_test (sqlite3 * handle, const char *base_name, int srid, int has_z,
	 int with_codes, int name, int cost, int oneways)
{
/* performing one routing test */
    char *sql;
    char *data_name;
    sqlite3_stmt *stmt;
    sqlite3_stmt *stmt_tsp_1 = NULL;
    sqlite3_stmt *stmt_tsp_2 = NULL;
    sqlite3_stmt *stmt_rng_1 = NULL;
    sqlite3_stmt *stmt_rng_2 = NULL;
    sqlite3_stmt *stmt_p2p_1 = NULL;
    sqlite3_stmt *stmt_p2p_2 = NULL;
    sqlite3_stmt *stmt_alg = NULL;
    sqlite3_stmt *stmt_req = NULL;
    sqlite3_stmt *stmt_opt = NULL;
    int ret;
    int result = 0;
    int with_astar;
    int alg;
    int req;
    int opt;
    int count = 0;

    sql = "SELECT CreateRouting(?, ?, 'roads', ?, ?, ?, ?, ?, ?, 1, ?, ?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateRouting #1: %s\n", sqlite3_errmsg (handle));
	  return -1;
      }

    data_name = sqlite3_mprintf ("%s_data", base_name);
    sqlite3_bind_text (stmt, 1, data_name, strlen (data_name),
		       SQLITE_TRANSIENT);
    sqlite3_free (data_name);
    sqlite3_bind_text (stmt, 2, base_name, strlen (base_name), SQLITE_STATIC);
    if (with_codes)
      {
	  sqlite3_bind_text (stmt, 3, "node_from", -1, SQLITE_TRANSIENT);
	  sqlite3_bind_text (stmt, 4, "node_to", -1, SQLITE_TRANSIENT);
      }
    else
      {
	  sqlite3_bind_text (stmt, 3, "id_node_from", -1, SQLITE_TRANSIENT);
	  sqlite3_bind_text (stmt, 4, "id_node_to", -1, SQLITE_TRANSIENT);
      }
    if (srid == 3003)
      {
	  if (has_z)
	      sqlite3_bind_text (stmt, 5, "geom_z", -1, SQLITE_TRANSIENT);
	  else
	      sqlite3_bind_text (stmt, 5, "geom", -1, SQLITE_TRANSIENT);
      }
    else if (srid == 4326)
      {
	  if (has_z)
	      sqlite3_bind_text (stmt, 5, "geom_wgs84_z", -1, SQLITE_TRANSIENT);
	  else
	      sqlite3_bind_text (stmt, 5, "geom_wgs84", -1, SQLITE_TRANSIENT);
      }
    else
	sqlite3_bind_null (stmt, 5);
    if (cost)
	sqlite3_bind_text (stmt, 6, "cost", -1, SQLITE_TRANSIENT);
    else
	sqlite3_bind_null (stmt, 6);
    if (name)
	sqlite3_bind_text (stmt, 7, "road_name", -1, SQLITE_TRANSIENT);
    else
	sqlite3_bind_null (stmt, 7);
    if (srid == 3003 || srid == 4326)
	with_astar = 1;
    else
	with_astar = 0;
    sqlite3_bind_int (stmt, 8, with_astar);
    if (oneways)
      {
	  sqlite3_bind_text (stmt, 9, "oneway_from_to", -1, SQLITE_TRANSIENT);
	  sqlite3_bind_text (stmt, 10, "oneway_to_from", -1, SQLITE_TRANSIENT);
      }
    else
      {
	  sqlite3_bind_null (stmt, 9);
	  sqlite3_bind_null (stmt, 10);
      }

    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		result = sqlite3_column_int (stmt, 0);
	    }
	  else
	    {
		fprintf (stderr, "CreateRouting #2: %s\n",
			 sqlite3_errmsg (handle));
		break;
	    }
      }
    sqlite3_finalize (stmt);
    if (!result)
	return -2;

/* creating aux UPDATE queries */
    sql = sqlite3_mprintf ("UPDATE %s SET algorithm = ?", base_name);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_alg, NULL);
    sqlite3_free (sql);
    sql = sqlite3_mprintf ("UPDATE %s SET request = ?", base_name);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_req, NULL);
    sqlite3_free (sql);
    sql = sqlite3_mprintf ("UPDATE %s SET options = ?", base_name);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_opt, NULL);
    sqlite3_free (sql);

/* creating the ShortestPath / TSP queries */
    sql =
	sqlite3_mprintf ("SELECT * FROM %s WHERE NodeFrom = ? AND NodeTo = ?",
			 base_name);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_tsp_1, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Testing Routing #1a-0: %s\n",
		   sqlite3_errmsg (handle));
	  return -1;
      }
    sql =
	sqlite3_mprintf ("SELECT * FROM %s WHERE NodeTo = ? AND NodeFrom = ?",
			 base_name);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_tsp_2, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Testing Routing #1a-1: %s\n",
		   sqlite3_errmsg (handle));
	  return -2;
      }

/* creating the Range queries */
    sql =
	sqlite3_mprintf ("SELECT * FROM %s WHERE NodeFrom = ? AND Cost <= ?",
			 base_name);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_rng_1, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Testing Routing #1b-0: %s\n",
		   sqlite3_errmsg (handle));
	  return -3;
      }
    sql =
	sqlite3_mprintf ("SELECT * FROM %s WHERE Cost <= ? AND NodeFrom = ?",
			 base_name);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_rng_2, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Testing Routing #1b-1: %s\n",
		   sqlite3_errmsg (handle));
	  return -4;
      }

/* creating the Point2Point queries */
    sql =
	sqlite3_mprintf ("SELECT * FROM %s WHERE "
			 "PointFrom = (SELECT geom FROM house_nr WHERE code_hnr = 'RT053018003491CV') AND "
			 "PointTo = (SELECT geom FROM house_nr WHERE code_hnr = 'RT053018004313CV')",
			 base_name);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_p2p_1, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Testing Routing #1c-0: %s\n",
		   sqlite3_errmsg (handle));
	  return -5;
      }
    sql =
	sqlite3_mprintf ("SELECT * FROM %s WHERE "
			 "PointTo = (SELECT geom FROM house_nr WHERE code_hnr = 'RT053018004313CV') AND "
			 "PointFrom = (SELECT geom FROM house_nr WHERE code_hnr = 'RT053018003491CV')",
			 base_name);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_p2p_2, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Testing Routing #1c-1: %s\n",
		   sqlite3_errmsg (handle));
	  return -6;
      }

    for (alg = 0; alg < 2; alg++)
      {
	  if (!with_astar && alg != 0)
	      continue;		/* skipping A* test */
	  set_algorithm (stmt_alg, alg);
	  for (req = 0; req < 3; req++)
	    {
		set_request (stmt_req, req);
		for (opt = 0; opt < 4; opt++)
		  {
		      int multi = 1;
		      set_option (stmt_opt, opt);
		      if (req == 0)
			  multi = count % 3;
		      if (count % 2 == 0)
			{
			    if (do_routing
				(handle, stmt_tsp_1, base_name, 0, multi) <= 0)
				return -7;
			}
		      else
			{
			    if (do_routing
				(handle, stmt_tsp_2, base_name, 1, multi) <= 0)
				return -8;
			}
		      if (count % 2 == 0)
			{
			    if (do_range
				(handle, stmt_rng_1, base_name, 0,
				 count % 3) <= 0)
				return -9;
			}
		      else
			{
			    if (do_range
				(handle, stmt_rng_2, base_name, 1,
				 count % 3) <= 0)
				return -10;
			}
		      if (count % 2 == 0)
			{
			    if (do_point2point (handle, stmt_p2p_1) <= 0)
				return -11;
			}
		      else
			{
			    if (do_point2point (handle, stmt_p2p_2) <= 0)
				return -12;
			}
		      count++;
		  }
	    }
      }

    sqlite3_finalize (stmt_tsp_1);
    sqlite3_finalize (stmt_tsp_2);
    sqlite3_finalize (stmt_rng_1);
    sqlite3_finalize (stmt_rng_2);
    sqlite3_finalize (stmt_p2p_1);
    sqlite3_finalize (stmt_p2p_2);
    sqlite3_finalize (stmt_alg);
    sqlite3_finalize (stmt_req);
    sqlite3_finalize (stmt_opt);

    return 0;
}

#endif

int
main (int argc, char *argv[])
{
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    void *cache = spatialite_alloc_connection ();

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    ret = system ("cp orbetello.sqlite copy-orbetello.sqlite");
    if (ret != 0)
      {
	  fprintf (stderr, "cannot copy orbetello.sqlite database\n");
	  return -1;
      }

    ret =
	sqlite3_open_v2 ("copy-orbetello.sqlite", &handle,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open copy-orbetello.sqlite database: %s\n",
		   sqlite3_errmsg (handle));
	  sqlite3_close (handle);
	  return -1;
      }

    spatialite_init_ex (handle, cache, 0);

#ifndef OMIT_GEOS		/* GEOS is supported */

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

/* building all Spatial Index */
    if (!do_create_spatial_index (handle, "geom"))
	return -3;
    if (!do_create_spatial_index (handle, "geom_z"))
	return -4;
    if (!do_create_spatial_index (handle, "geom_wgs84"))
	return -5;
    if (!do_create_spatial_index (handle, "geom_wgs84_z"))
	return -6;

/* adding "id_node_from" and "id_node_to" columns */
    if (!do_create_routing_nodes (handle))
	return -7;

/* testing 2D projected - node-codes, no-name, no-cost, no-oneways */
    ret = do_test (handle, "test_3003_2d_cnnn", 3003, 0, 1, 0, 0, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #1 error\n");
	  return -8;
      }

/* testing 2D projected - node-codes, name, no-cost, no-oneways */
    ret = do_test (handle, "test_3003_2d_cynn", 3003, 0, 1, 1, 0, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #2 error\n");
	  return -9;
      }

/* testing 2D projected - node-codes, name, cost, no-oneways */
    ret = do_test (handle, "test_3003_2d_cyyn", 3003, 0, 1, 1, 1, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #3 error\n");
	  return -10;
      }

/* testing 2D projected - node-codes, name, cost, oneways */
    ret = do_test (handle, "test_3003_2d_cyyy", 3003, 0, 1, 1, 1, 1);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #4 error\n");
	  return -11;
      }

/* testing 3D projected - node-codes, no-name, no-cost, no-oneways */
    ret = do_test (handle, "test_3003_3d_cnnn", 3003, 1, 1, 0, 0, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #1 error\n");
	  return -12;
      }

/* testing 3D projected - node-codes, name, no-cost, no-oneways */
    ret = do_test (handle, "test_3003_3d_cynn", 3003, 1, 1, 1, 0, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #2 error\n");
	  return -13;
      }

/* testing 3D projected - node-codes, name, cost, no-oneways */
    ret = do_test (handle, "test_3003_3d_cyyn", 3003, 1, 1, 1, 1, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #3 error\n");
	  return -14;
      }

/* testing 3D projected - node-codes, name, cost, oneways */
    ret = do_test (handle, "test_3003_3d_cyyy", 3003, 1, 1, 1, 1, 1);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #4 error\n");
	  return -15;
      }

/* testing 2D long/lat - node-codes, no-name, no-cost, no-oneways */
    ret = do_test (handle, "test_4326_2d_cnnn", 4326, 0, 1, 0, 0, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #5 error\n");
	  return -16;
      }

/* testing 2D long/lat - node-codes, name, no-cost, no-oneways */
    ret = do_test (handle, "test_4326_2d_cynn", 4326, 0, 1, 1, 0, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #6 error\n");
	  return -17;
      }

/* testing 2D long/lat - node-codes, name, cost, no-oneways */
    ret = do_test (handle, "test_4326_2d_cyyn", 4326, 0, 1, 1, 1, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #7 error\n");
	  return -18;
      }

/* testing 2D long/lat - node-codes, name, cost, oneways */
    ret = do_test (handle, "test_4326_2d_cyyy", 4326, 0, 1, 1, 1, 1);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #8 error\n");
	  return -19;
      }

/* testing 3D long/lat - node-codes, no-name, no-cost, no-oneways */
    ret = do_test (handle, "test_4326_3d_cnnn", 4326, 1, 1, 0, 0, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #9 error\n");
	  return -20;
      }

/* testing 3D long/lat - node-codes, name, no-cost, no-oneways */
    ret = do_test (handle, "test_4326_3d_cynn", 4326, 1, 1, 1, 0, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #10 error\n");
	  return -21;
      }

/* testing 3D long/lat - node-codes, name, cost, no-oneways */
    ret = do_test (handle, "test_4326_3d_cyyn", 4326, 1, 1, 1, 1, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #11 error\n");
	  return -22;
      }

/* testing 3D long/lat - node-codes, name, cost, oneways */
    ret = do_test (handle, "test_4326_3d_cyyy", 4326, 1, 1, 1, 1, 1);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #12 error\n");
	  return -23;
      }

/* testing no-geometry - node-codes, no-name, cost, no-oneways */
    ret = do_test (handle, "test_none_cnyn", 0, 0, 1, 0, 1, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #13 error\n");
	  return -24;
      }

/* testing no-geometry - node-codes, name, cost, no-oneways */
    ret = do_test (handle, "test_none_cyyn", 0, 0, 1, 1, 1, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #14 error\n");
	  return -25;
      }

/* testing no-geometry - node-codes, name, cost, oneways */
    ret = do_test (handle, "test_none_cyyy", 0, 0, 1, 1, 1, 1);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #15 error\n");
	  return -26;
      }

/* testing 2D projected - node-ids, no-name, no-cost, no-oneways */
    ret = do_test (handle, "test_3003_2d_innn", 3003, 0, 0, 0, 0, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #1 error\n");
	  return -27;
      }

/* testing 2D projected - node-ids, name, no-cost, no-oneways */
    ret = do_test (handle, "test_3003_2d_iynn", 3003, 0, 0, 1, 0, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #2 error\n");
	  return -28;
      }

/* testing 2D projected - node-ids, name, cost, no-oneways */
    ret = do_test (handle, "test_3003_2d_iyyn", 3003, 0, 0, 1, 1, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #3 error\n");
	  return -29;
      }

/* testing 2D projected - node-ids, name, cost, oneways */
    ret = do_test (handle, "test_3003_2d_iyyy", 3003, 0, 0, 1, 1, 1);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #4 error\n");
	  return -30;
      }

/* testing 3D projected - node-ids, no-name, no-cost, no-oneways */
    ret = do_test (handle, "test_3003_3d_innn", 3003, 1, 0, 0, 0, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #1 error\n");
	  return -31;
      }

/* testing 3D projected - node-ids, name, no-cost, no-oneways */
    ret = do_test (handle, "test_3003_3d_iynn", 3003, 1, 0, 1, 0, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #2 error\n");
	  return -32;
      }

/* testing 3D projected - node-ids, name, cost, no-oneways */
    ret = do_test (handle, "test_3003_3d_iyyn", 3003, 1, 0, 1, 1, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #3 error\n");
	  return -33;
      }

/* testing 3D projected - node-ids, name, cost, oneways */
    ret = do_test (handle, "test_3003_3d_iyyy", 3003, 1, 0, 1, 1, 1);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #4 error\n");
	  return -34;
      }

/* testing 2D long/lat - node-ids, no-name, no-cost, no-oneways */
    ret = do_test (handle, "test_4326_2d_innn", 4326, 0, 0, 0, 0, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #5 error\n");
	  return -35;
      }

/* testing 2D long/lat - node-ids, name, no-cost, no-oneways */
    ret = do_test (handle, "test_4326_2d_iynn", 4326, 0, 0, 1, 0, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #6 error\n");
	  return -36;
      }

/* testing 2D long/lat - node-ids, name, cost, no-oneways */
    ret = do_test (handle, "test_4326_2d_iyyn", 4326, 0, 0, 1, 1, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #7 error\n");
	  return -37;
      }

/* testing 2D long/lat - node-ids, name, cost, oneways */
    ret = do_test (handle, "test_4326_2d_iyyy", 4326, 0, 0, 1, 1, 1);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #8 error\n");
	  return -38;
      }

/* testing 3D long/lat - node-ids, no-name, no-cost, no-oneways */
    ret = do_test (handle, "test_4326_3d_innn", 4326, 1, 0, 0, 0, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #9 error\n");
	  return -39;
      }

/* testing 3D long/lat - node-ids, name, no-cost, no-oneways */
    ret = do_test (handle, "test_4326_3d_iynn", 4326, 1, 0, 1, 0, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #10 error\n");
	  return -40;
      }

/* testing 3D long/lat - node-ids, name, cost, no-oneways */
    ret = do_test (handle, "test_4326_3d_iyyn", 4326, 1, 0, 1, 1, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #11 error\n");
	  return -41;
      }

/* testing 3D long/lat - node-ids, name, cost, oneways */
    ret = do_test (handle, "test_4326_3d_iyyy", 4326, 1, 0, 1, 1, 1);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #12 error\n");
	  return -42;
      }

/* testing no-geometry - node-ids, no-name, cost, no-oneways */
    ret = do_test (handle, "test_none_inyn", 0, 0, 0, 0, 1, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #13 error\n");
	  return -43;
      }

/* testing no-geometry - node-ids, name, cost, no-oneways */
    ret = do_test (handle, "test_none_iyyn", 0, 0, 0, 1, 1, 0);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #14 error\n");
	  return -44;
      }

/* testing no-geometry - node-ids, name, cost, oneways */
    ret = do_test (handle, "test_none_iyyy", 0, 0, 0, 1, 1, 1);
    if (ret != 0)
      {
	  fprintf (stderr, "Test #15 error\n");
	  return -45;
      }

/* testing invalid cases */
    ret = do_test_invalid (handle);
    if (ret != 0)
      {
	  fprintf (stderr, "Test Invalid cases error\n");
	  return -4;
      }
      
#endif /* end GEOS conditional */

    sqlite3_close (handle);
    spatialite_cleanup_ex (cache);
    ret = unlink ("copy-orbetello.sqlite");
    if (ret != 0)
      {
	  fprintf (stderr, "cannot remove copy-orbetello.sqlite\n");
	  return -47;
      }

    spatialite_shutdown ();
    return 0;
}
