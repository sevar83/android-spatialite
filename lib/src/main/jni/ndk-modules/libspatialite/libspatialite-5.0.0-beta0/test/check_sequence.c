
/*

 check_sequence.c -- SpatiaLite Test Case

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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sqlite3.h"
#include "spatialite.h"

#include "spatialite/gg_sequence.h"

static int
test_sequence (sqlite3 * sqlite, const char *seq_name)
{
/* testing a single sequence */
    char *sql;
    int ret;
    char *err_msg = NULL;
    char **results;
    int rows;
    int columns;
    int cnt;

    for (cnt = 1; cnt <= 10; cnt++)
      {
	  if (seq_name == NULL)
	      sql = sqlite3_mprintf ("SELECT sequence_nextval(NULL)");
	  else
	      sql = sqlite3_mprintf ("SELECT sequence_nextval(%Q)", seq_name);
	  ret = sqlite3_get_table (sqlite, sql, &results, &rows,
				   &columns, &err_msg);
	  sqlite3_free (sql);

	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "nextval - Error: %s\n", err_msg);
		sqlite3_free (err_msg);
		return 0;
	    }
	  if ((rows != 1) || (columns != 1))
	    {
		fprintf (stderr,
			 "nextval - Unexpected error: bad result: %i/%i.\n",
			 rows, columns);
		return 0;
	    }

	  if (atoi (results[1]) != cnt)
	    {
		fprintf (stderr, "nextval - Unexpected value \"%s\" (%d)\n",
			 results[1], cnt);
		return 0;
	    }
	  sqlite3_free_table (results);

	  if (seq_name == NULL)
	      sql = sqlite3_mprintf ("SELECT sequence_currval(NULL)");
	  else
	      sql = sqlite3_mprintf ("SELECT sequence_currval(%Q)", seq_name);
	  ret = sqlite3_get_table (sqlite, sql, &results, &rows,
				   &columns, &err_msg);
	  sqlite3_free (sql);

	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "currval - Error: %s\n", err_msg);
		sqlite3_free (err_msg);
		return 0;
	    }
	  if ((rows != 1) || (columns != 1))
	    {
		fprintf (stderr,
			 "currval - Unexpected error: bad result: %i/%i.\n",
			 rows, columns);
		return 0;
	    }

	  if (atoi (results[1]) != cnt)
	    {
		fprintf (stderr, "currval - Unexpected value \"%s\" (%d)\n",
			 results[1], cnt);
		return 0;
	    }
	  sqlite3_free_table (results);
      }

    return 1;
}

static int
reset_sequence (sqlite3 * sqlite, const char *seq_name, int value)
{
/* resetting some sequence */
    char *sql;
    int ret;
    char *err_msg = NULL;
    char **results;
    int rows;
    int columns;

    if (seq_name == NULL)
	sql = sqlite3_mprintf ("SELECT sequence_setval(NULL, %d)", value);
    else
	sql =
	    sqlite3_mprintf ("SELECT sequence_setval(%Q, %d)", seq_name, value);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free (sql);

    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "setval - Error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    if ((rows != 1) || (columns != 1))
      {
	  fprintf (stderr, "setval - Unexpected error: bad result: %i/%i.\n",
		   rows, columns);
	  return 0;
      }

    if (atoi (results[1]) != abs (value))
      {
	  fprintf (stderr, "setval - Unexpected value \"%s\" (%d)\n",
		   results[1], value);
	  return 0;
      }
    sqlite3_free_table (results);

    return 1;
}

static int
test_all_sequences (sqlite3 * sqlite)
{
/* testing all sequences */
    const char *sql;
    int ret;
    char *err_msg = NULL;
    char **results;
    int rows;
    int columns;
    int cnt;

    for (cnt = 1; cnt <= 10; cnt++)
      {
	  sql =
	      "SELECT sequence_nextval(NULL), sequence_nextval('Sequence_B'), "
	      "sequence_currval(NULL), sequence_lastval(), sequence_nextval('SeqA'), "
	      "sequence_lastval(), sequence_nextval('Sequence_C'), "
	      "sequence_currval('z'), sequence_nextval('z')";
	  ret =
	      sqlite3_get_table (sqlite, sql, &results, &rows, &columns,
				 &err_msg);

	  if (ret != SQLITE_OK)
	    {
		fprintf (stderr, "multitest - Error: %s\n", err_msg);
		sqlite3_free (err_msg);
		return 0;
	    }
	  if ((rows != 1) || (columns != 9))
	    {
		fprintf (stderr,
			 "multitest - Unexpected error: bad result: %i/%i.\n",
			 rows, columns);
		return 0;
	    }

	  if (atoi (results[9]) != cnt)
	    {
		fprintf (stderr, "nextval #1 - Unexpected value \"%s\" (%d)\n",
			 results[9], cnt);
		return 0;
	    }
	  if (atoi (results[10]) != cnt + 1000)
	    {
		fprintf (stderr, "nextval #2 - Unexpected value \"%s\" (%d)\n",
			 results[10], cnt + 1000);
		return 0;
	    }
	  if (atoi (results[11]) != cnt)
	    {
		fprintf (stderr, "currval #1 - Unexpected value \"%s\" (%d)\n",
			 results[11], cnt);
		return 0;
	    }
	  if (atoi (results[12]) != cnt + 1000)
	    {
		fprintf (stderr, "lasttval #1 - Unexpected value \"%s\" (%d)\n",
			 results[12], cnt + 1000);
		return 0;
	    }
	  if (atoi (results[13]) != cnt + 100)
	    {
		fprintf (stderr, "nextval #3 - Unexpected value \"%s\" (%d)\n",
			 results[13], cnt + 100);
		return 0;
	    }
	  if (atoi (results[14]) != cnt + 100)
	    {
		fprintf (stderr, "lastval #1 - Unexpected value \"%s\" (%d)\n",
			 results[14], cnt + 100);
		return 0;
	    }
	  if (atoi (results[15]) != cnt + 10000)
	    {
		fprintf (stderr, "nextval #4 - Unexpected value \"%s\" (%d)\n",
			 results[15], cnt + 10000);
		return 0;
	    }
	  if (cnt == 1)
	    {
		if (results[16] != NULL)
		  {
		      fprintf (stderr,
			       "currval #2 - Unexpected value \"%s\" (NULL)\n",
			       results[16]);
		      return 0;
		  }
	    }
	  else
	    {
		if (atoi (results[16]) != cnt - 1)
		  {
		      fprintf (stderr,
			       "currval #2 - Unexpected value \"%s\" (%d)\n",
			       results[16], cnt);
		      return 0;
		  }
	    }
	  if (atoi (results[17]) != cnt)
	    {
		fprintf (stderr,
			 "nextval #5 - Unexpected value \"%s\" (%d)\n",
			 results[17], cnt);
		return 0;
	    }
	  sqlite3_free_table (results);
      }

    return 1;
}

static int
test_capi (void *cache)
{
/* testing the C API */
    int value;
    if (gaiaCreateSequence (NULL, "xx") != NULL)
      {
	  fprintf (stderr, "CAPI create #1 - unexpected result\n");
	  return 0;
      }
    if (gaiaFindSequence (NULL, "xx") != NULL)
      {
	  fprintf (stderr, "CAPI find #1 - unexpected result\n");
	  return 0;
      }
    if (gaiaLastUsedSequence (NULL, &value) != 0)
      {
	  fprintf (stderr, "CAPI last #1 - unexpected result\n");
	  return 0;
      }
    if (gaiaSequenceNext (NULL, NULL) != 0)
      {
	  fprintf (stderr, "CAPI next #1 - unexpected result\n");
	  return 0;
      }
    if (gaiaSequenceNext (cache, NULL) != 0)
      {
	  fprintf (stderr, "CAPI next #2 - unexpected result\n");
	  return 0;
      }
    if (gaiaResetSequence (NULL, 1) != 0)
      {
	  fprintf (stderr, "CAPI reset #1 - unexpected result\n");
	  return 0;
      }
    if (gaiaCreateSequence (cache, NULL) == NULL)
      {
	  fprintf (stderr, "CAPI create #2 - unexpected result\n");
	  return 0;
      }
    if (gaiaCreateSequence (cache, NULL) == NULL)
      {
	  fprintf (stderr, "CAPI create #3 - unexpected result\n");
	  return 0;
      }
    if (gaiaCreateSequence (cache, "abc") == NULL)
      {
	  fprintf (stderr, "CAPI create #4 - unexpected result\n");
	  return 0;
      }
    if (gaiaCreateSequence (cache, "abc") == NULL)
      {
	  fprintf (stderr, "CAPI create #5 - unexpected result\n");
	  return 0;
      }

    return 1;
}

int
main (int argc, char *argv[])
{
    int ret;
    sqlite3 *handle;
    void *cache = spatialite_alloc_connection ();

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    ret =
	sqlite3_open_v2 (":memory:", &handle,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open in-memory db: %s\n",
		   sqlite3_errmsg (handle));
	  sqlite3_close (handle);
	  return -100;
      }

    spatialite_init_ex (handle, cache, 0);

/* testing the Unnamed Sequence */
    if (!test_sequence (handle, NULL))
	return -1;

/* testing SeqA */
    if (!test_sequence (handle, "SeqA"))
	return -2;

/* testing Sequence_B */
    if (!test_sequence (handle, "Sequence_B"))
	return -3;

/* resetting the Unnamed Sequence */
    if (!reset_sequence (handle, NULL, 0))
	return -4;

/* resetting SeqA */
    if (!reset_sequence (handle, "SeqA", 100))
	return -5;

/* resetting Sequence_B */
    if (!reset_sequence (handle, "Sequence_B", 1000))
	return -6;

/* resetting Sequence_C */
    if (!reset_sequence (handle, "Sequence_C", -10000))
	return -6;

/* testing all sequences */
    if (!test_all_sequences (handle))
	return -7;

/* testing the C API */
    if (!test_capi (cache))
	return -8;

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "sqlite3_close() error: %s\n",
		   sqlite3_errmsg (handle));
	  return -133;
      }

    spatialite_cleanup_ex (cache);
    spatialite_shutdown ();

    return 0;
}
