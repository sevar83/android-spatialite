/*

 check_stored_proc.c -- SpatiaLite Test Case
 
 Testing Stored Procedures

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
 
Portions created by the Initial Developer are Copyright (C) 2017
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

#ifndef OMIT_ICONV		/* only if ICONV is supported */

static int
do_level0_tests (sqlite3 * handle, int *retcode)
{
/* performing Level 0 tests - DB initialization */
    int ret;
    char *err_msg = NULL;
    char **results;
    int rows;
    int columns;

/* creating Stored Procedures tables */
    ret =
	sqlite3_get_table (handle, "SELECT StoredProc_CreateTables()", &results,
			   &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredProc_CreateTables() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -1;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "StoredProc_CreateTables() #1 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -2;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "StoredProc_CreateTables() #1 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -3;
	  return 0;
      }
    sqlite3_free_table (results);

/* creating the first test table */
    ret = sqlite3_exec (handle, "CREATE TABLE test_1 ("
			"id INTEGER NOT NULL PRIMARY KEY,\n"
			"name TEXT NOT NULL,\n"
			"value DOUBLE NOT NULL)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE test_1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -4;
      }

/* creating the second test table */
    ret = sqlite3_exec (handle, "CREATE TABLE test_2 ("
			"pk_uid INTEGER NOT NULL PRIMARY KEY,\n"
			"code TEXT NOT NULL,\n"
			"description TEXT NOT NULL,\n"
			"measure DOUBLE NOT NULL)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE test_2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -5;
      }

/* populating the first test table */
    ret = sqlite3_exec (handle, "INSERT INTO test_1 (id, name, value) "
			"VALUES (NULL, 'alpha', 1.5)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO test_1 #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -6;
      }
    ret = sqlite3_exec (handle, "INSERT INTO test_1 (id, name, value) "
			"VALUES (NULL, 'beta', 2.5)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO test_1 #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -7;
      }
    ret = sqlite3_exec (handle, "INSERT INTO test_1 (id, name, value) "
			"VALUES (NULL, 'gamma', 3.5)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO test_1 #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -8;
      }

/* populating the second test table */
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO test_2 (pk_uid, code, description, measure) "
		      "VALUES (NULL, 'alpha', 'first', 1000.5)", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO test_2 #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -9;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO test_2 (pk_uid, code, description, measure) "
		      "VALUES (NULL, 'beta', 'second', 2000.5)", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO test_2 #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -10;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO test_2 (pk_uid, code, description, measure) "
		      "VALUES (NULL, 'gamma', 'third', 3000.5)", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO test_2 #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -11;
      }

    return 1;
}

static int
do_level1_tests (sqlite3 * handle, int *retcode)
{
/* performing Level 1 tests - Stored Procedures basics */
    const char *sql;
    int ret;
    char *err_msg = NULL;
    char **results;
    int rows;
    int columns;

/* registering a first Stored Procedure */
    sql = "SELECT StoredProc_Register('proc_1', 'this is title one', "
	"SqlProc_FromText('--\n-- comment\n--\n"
	"CREATE TABLE @output_1@ AS\n"
	"SELECT @col_1@, @col_2@ FROM @table_1@ WHERE @col_3@ = @value_1@;\n\n"
	".echo on\n\n"
	"--\n-- another comment\n--\n"
	"CREATE TABLE @output_2@ AS\n"
	"SELECT @col_4@, @col_5@ FROM @table_2@ WHERE @col_6@ = @value_2@;\n\n"
	".echo off\n\n" "--\n-- end comment\n--\n\n'))";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredProc_Register() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -12;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "StoredProc_Register() #1 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -13;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "StoredProc_Register() #1 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -14;
	  return 0;
      }
    sqlite3_free_table (results);

/* registering a second Stored Procedure */
    sql = "SELECT StoredProc_Register('proc_2', 'this is title two', "
	"SqlProc_FromText('SELECT @col_1@, @col_2@ FROM @table_1@ WHERE @col_3@ = @value_1@;'))";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredProc_Register() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -15;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "StoredProc_Register() #2 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -16;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "StoredProc_Register() #2 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -17;
	  return 0;
      }
    sqlite3_free_table (results);

/* updating Stored Procedure title - expected failure */
    sql =
	"SELECT StoredProc_UpdateTitle('no_proc', 'this is an updated title two')";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredProc_UpdateTitle() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -18;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "StoredProc_UpdateTitle() #1 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -19;
	  return 0;
      }
    if (atoi (*(results + 1)) != 0)
      {
	  fprintf (stderr, "StoredProc_UpdateTitle() #1 unexpected success\n");
	  sqlite3_free_table (results);
	  *retcode = -20;
	  return 0;
      }
    sqlite3_free_table (results);

/* updating Stored Procedure title - expected success */
    sql =
	"SELECT StoredProc_UpdateTitle('proc_2', 'this is an updated title two')";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredProc_UpdateTitle() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -21;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "StoredProc_UpdateTitle() #2 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -22;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "StoredProc_UpdateTitle() #2 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -23;
	  return 0;
      }
    sqlite3_free_table (results);

/* updating Stored Procedure SQL Body - expected failure */
    sql = "SELECT StoredProc_UpdateSqlBody('no_proc', "
	"SqlProc_FromText('SELECT @fld1@, @fld2@ FROM @tbl@ WHERE @fld3@ = @val1@;'))";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredProc_UpdateSqlBody() #1 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -24;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "StoredProc_UpdateSqlBody() #1 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -25;
	  return 0;
      }
    if (atoi (*(results + 1)) != 0)
      {
	  fprintf (stderr,
		   "StoredProc_UpdateSqlBody() #1 unexpected success\n");
	  sqlite3_free_table (results);
	  *retcode = -26;
	  return 0;
      }
    sqlite3_free_table (results);

/* updating Stored Procedure SQL Body - expected success */
    sql = "SELECT StoredProc_UpdateSqlBody('proc_2', "
	"SqlProc_FromText('SELECT @fld1@, @fld2@ FROM @tbl@ WHERE @fld3@ = @val1@;'))";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredProc_UpdateSqlBody() #2 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -27;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "StoredProc_UpdateSqlBody() #2 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -28;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr,
		   "StoredProc_UpdateSqlBody() #2 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -29;
	  return 0;
      }
    sqlite3_free_table (results);

/* testing a Stored Procedure - expected failure */
    sql = "SELECT SqlProc_AllVariables(StoredProc_Get('no_proc'))";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "StoredProc_Get() #1 unexpected success\n");
	  *retcode = -30;
	  return 0;
      }
    sqlite3_free (err_msg);

/* testing a Stored Procedure - expected success */
    sql = "SELECT SqlProc_AllVariables(StoredProc_Get('proc_2'))";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredProc_Get() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -31;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr, "StoredProc_Get() #2 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -32;
	  return 0;
      }
    if (*(results + 1) == NULL)
      {
	  fprintf (stderr, "StoredProc_Get() #2 unexpected NULL\n");
	  sqlite3_free_table (results);
	  *retcode = -33;
	  return 0;
      }
    if (strcmp (*(results + 1), "@fld1@ @fld2@ @tbl@ @fld3@ @val1@") != 0)
      {
	  fprintf (stderr, "StoredProc_Get() #2 unexpected value \"%s\"\n",
		   *(results + 1));
	  sqlite3_free_table (results);
	  *retcode = -33;
	  return 0;
      }
    sqlite3_free_table (results);

/* deleting a Stored Procedure - expected failure */
    sql = "SELECT StoredProc_Delete('no_proc')";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredProc_Delete() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -34;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr, "StoredProc_Delete() #1 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -35;
	  return 0;
      }
    if (atoi (*(results + 1)) != 0)
      {
	  fprintf (stderr, "StoredProc_Delete() #1 unexpected success\n");
	  sqlite3_free_table (results);
	  *retcode = -36;
	  return 0;
      }
    sqlite3_free_table (results);

/* deleting a Stored Procedure - expected success */
    sql = "SELECT StoredProc_Delete('proc_2')";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredProc_Delete() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -37;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr, "StoredProc_Delete() #2 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -38;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "StoredProc_Delete() #2 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -39;
	  return 0;
      }
    sqlite3_free_table (results);

/* registering a Stored Procedure from filepath */
    sql = "SELECT StoredProc_Register('proc_file', 'this is title three', "
	"SqlProc_FromFile('./sqlproc_sample.txt'))";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredProc_Register() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -40;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "StoredProc_Register() #3 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -41;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "StoredProc_Register() #3 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -42;
	  return 0;
      }
    sqlite3_free_table (results);

    return 1;
}

static int
do_level2_tests (sqlite3 * handle, int *retcode)
{
/* performing Level 2 tests - Stored Variables basics */
    const char *sql;
    int ret;
    char *err_msg = NULL;
    char **results;
    int rows;
    int columns;

/* registering a first Stored Variable */
    sql = "SELECT StoredVar_Register('var_1', 'this is title one', 1234)";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredVar_Register() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -45;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "StoredVar_Register() #1 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -46;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "StoredVar_Register() #1 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -47;
	  return 0;
      }
    sqlite3_free_table (results);

/* registering a second Stored Variable */
    sql = "SELECT StoredVar_Register('var_2', 'this is title two', 'abcdef')";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredVar_Register() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -48;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "StoredVar_Register() #2 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -49;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "StoredVar_Register() #2 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -50;
	  return 0;
      }
    sqlite3_free_table (results);

/* updating Stored Variable title - expected failure */
    sql =
	"SELECT StoredVar_UpdateTitle('no_var', 'this is an updated title two')";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredVar_UpdateTitle() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -51;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "StoredVar_UpdateTitle() #1 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -52;
	  return 0;
      }
    if (atoi (*(results + 1)) != 0)
      {
	  fprintf (stderr, "StoredVar_UpdateTitle() #1 unexpected success\n");
	  sqlite3_free_table (results);
	  *retcode = -53;
	  return 0;
      }
    sqlite3_free_table (results);

/* updating Stored Variable title - expected success */
    sql =
	"SELECT StoredVar_UpdateTitle('var_2', 'this is an updated title two')";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredVar_UpdateTitle() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -54;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "StoredVar_UpdateTitle() #2 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -55;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "StoredVar_UpdateTitle() #2 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -56;
	  return 0;
      }
    sqlite3_free_table (results);

/* updating Stored Variable with Value - expected failure */
    sql = "SELECT StoredVar_UpdateValue('no_var', 1956)";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredVar_UpdateValue() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -57;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "StoredVar_UpdateValue() #1 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -58;
	  return 0;
      }
    if (atoi (*(results + 1)) != 0)
      {
	  fprintf (stderr, "StoredVar_UpdateValue() #1 unexpected success\n");
	  sqlite3_free_table (results);
	  *retcode = -59;
	  return 0;
      }
    sqlite3_free_table (results);

/* updating Stored Variable with Value - expected success */
    sql = "SELECT StoredVar_UpdateValue('var_2', 1956)";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredVar_UpdateValue() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -60;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "StoredVar_UpdateValue() #2 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -61;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "StoredVar_UpdateValue() #2 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -62;
	  return 0;
      }
    sqlite3_free_table (results);

/* testing a Stored Variable - expected failure */
    sql = "SELECT StoredVar_Get('no_var')";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredVar_Get() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -63;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr, "StoredVar_Get() #1 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -64;
	  return 0;
      }
    if (*(results + 1) != NULL)
      {
	  fprintf (stderr, "StoredVar_Get() #1 unexpected success\n");
	  sqlite3_free_table (results);
	  *retcode = -65;
	  return 0;
      }
    sqlite3_free_table (results);

/* testing a Stored Variable - expected success */
    sql = "SELECT StoredVar_Get('var_2')";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredVar_Get() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -66;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr, "StoredVar_Get() #2 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -67;
	  return 0;
      }
    if (*(results + 1) == NULL)
      {
	  fprintf (stderr, "StoredVar_Get() #2 unexpected NULL\n");
	  sqlite3_free_table (results);
	  *retcode = -68;
	  return 0;
      }
    if (strcmp (*(results + 1), "@var_2@=1956") != 0)
      {
	  fprintf (stderr, "StoredVar_Get() #2 unexpected value \"%s\"\n",
		   *(results + 1));
	  sqlite3_free_table (results);
	  *retcode = -69;
	  return 0;
      }
    sqlite3_free_table (results);

/* testing a Stored Variable - expected failure */
    sql = "SELECT StoredVar_GetValue('no_var')";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredVar_GetValue() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -63;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "StoredVar_GetValue() #1 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -64;
	  return 0;
      }
    if (*(results + 1) != NULL)
      {
	  fprintf (stderr, "StoredVar_GetValue() #1 unexpected success\n");
	  sqlite3_free_table (results);
	  *retcode = -65;
	  return 0;
      }
    sqlite3_free_table (results);

/* testing a Stored Variable - expected success */
    sql = "SELECT StoredVar_GetValue('var_2')";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredVar_GetValue() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -66;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "StoredVar_GetValue() #2 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -67;
	  return 0;
      }
    if (*(results + 1) == NULL)
      {
	  fprintf (stderr, "StoredVar_GetValue() #2 unexpected NULL\n");
	  sqlite3_free_table (results);
	  *retcode = -68;
	  return 0;
      }
    if (strcmp (*(results + 1), "1956") != 0)
      {
	  fprintf (stderr, "StoredVar_GetValue() #2 unexpected value \"%s\"\n",
		   *(results + 1));
	  sqlite3_free_table (results);
	  *retcode = -69;
	  return 0;
      }
    sqlite3_free_table (results);

/* deleting a Stored Variable - expected failure */
    sql = "SELECT StoredVar_Delete('no_var')";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredVar_Delete() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -70;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr, "StoredVar_Delete() #1 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -71;
	  return 0;
      }
    if (atoi (*(results + 1)) != 0)
      {
	  fprintf (stderr, "StoredVar_Delete() #1 unexpected success\n");
	  sqlite3_free_table (results);
	  *retcode = -72;
	  return 0;
      }
    sqlite3_free_table (results);

/* deleting a Stored Procedure - expected success */
    sql = "SELECT StoredVar_Delete('var_2')";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredVar_Delete() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -73;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr, "StoredVar_Delete() #2 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -74;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "StoredVar_Delete() #2 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -75;
	  return 0;
      }
    sqlite3_free_table (results);

    return 1;
}

static int
do_level3_tests (sqlite3 * handle, int *retcode)
{
/* performing Level 3 tests - Stored Procedure Execute */
    const char *sql;
    int ret;
    char *err_msg = NULL;
    char **results;
    int rows;
    int columns;

/* executing a Stored Procedure */
    sql = "SELECT StoredProc_Execute('proc_1', '@output_1@=out_test_1', "
	"'@col_1@=name', '@col_2@=value', '@table_1@=test_1', "
	"'@col_3@=id', '@value_1@=2', '@output_2@=out_test_2', "
	"'@col_4@=description', '@col_5@=measure', '@table_2@=test_2', "
	"'@col_6@=pk_uid', '@value_2@=3')";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredProc_Execute() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -76;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "StoredProc_Execute() #1 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -77;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "StoredProc_Execute() #1 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -78;
	  return 0;
      }
    sqlite3_free_table (results);

/* executing another Stored Procedure */
    sql = "SELECT StoredProc_Execute('proc_file', '@col1@=description', "
	"'@col2@=measure', '@table@=test_2')";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredProc_Execute() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -79;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "StoredProc_Execute() #2 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -80;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "StoredProc_Execute() #2 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -81;
	  return 0;
      }
    sqlite3_free_table (results);

    return 1;
}

static int
do_level4_tests (sqlite3 * handle, int *retcode)
{
/* performing Level 4 tests - SQL Logfile */
    const char *sql;
    int ret;
    char *err_msg = NULL;
    char **results;
    int rows;
    int columns;

/* enabling a SQL Logfile */
    sql = "SELECT SqlProc_SetLogfile('./sql_logfile', 0)";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SqlProc_SetLogfile() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -82;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "SqlProc_SetLogfile() #1 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -83;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "SqlProc_SetLogfile() #1 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -84;
	  return 0;
      }
    sqlite3_free_table (results);

/* checking the SQL Logfile */
    sql = "SELECT SqlProc_GetLogfile()";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SqlProc_GetLogfile() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -85;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "SqlProc_GetLogfile() #1 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -86;
	  return 0;
      }
    if (strcmp (*(results + 1), "./sql_logfile") != 0)
      {
	  fprintf (stderr, "SqlProc_GetLogfile() #1 unexpected failure (%s)\n",
		   *(results + 1));
	  sqlite3_free_table (results);
	  *retcode = -87;
	  return 0;
      }
    sqlite3_free_table (results);

/* executing a Stored Procedure - valid SQL */
    sql = "SELECT SqlProc_Execute(SqlProc_FromFile('./sqlproc_logfile.txt'))";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SqlProc_Execute() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -87;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "SqlProc_Execute() #1 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -88;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "SqlProc_Execute() #1 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -89;
	  return 0;
      }
    sqlite3_free_table (results);

/* executing a Stored Procedure - invalid SQL */
    sql = "SELECT SqlProc_Execute(SqlProc_FromFile('./sqlproc_error.txt'))";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "SqlProc_Execute() #2 unexpected success\n");
	  *retcode = -90;
	  return 0;
      }
    sqlite3_free (err_msg);

    return 1;
}

static int
do_level5_tests (sqlite3 * handle, int *retcode)
{
/* performing Level 5 tests - SQL Logfile */
    const char *sql;
    int ret;
    char *err_msg = NULL;
    char **results;
    int rows;
    int columns;

/* enabling a SQL Logfile - append mode */
    sql = "SELECT SqlProc_SetLogfile('./sql_logfile', 1)";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SqlProc_SetLogfile() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -91;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "SqlProc_SetLogfile() #2 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -92;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "SqlProc_SetLogfile() #2 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -93;
	  return 0;
      }
    sqlite3_free_table (results);

/* executing a SQL Procedure - with no arguments */
    sql = "SELECT SqlProc_Execute(SqlProc_FromText('SELECT @num1@ * @num2@;'))";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SqlProc_Execute() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -94;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "SqlProc_Execute() #3 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -95;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "SqlProc_Execute() #3 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -96;
	  return 0;
      }
    sqlite3_free_table (results);

/* registering a first Stored Variable */
    sql = "SELECT StoredVar_Register('num1', 'variable @num1@', 2)";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredVar_Register() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -97;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "StoredVar_Register() #3 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -98;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "StoredVar_Register() #3 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -99;
	  return 0;
      }
    sqlite3_free_table (results);

/* registering a second Stored Variable */
    sql = "SELECT StoredVar_Register('num2', 'variable @num1@', 3.14)";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "StoredVar_Register() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -100;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "StoredVar_Register() #4 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -101;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "StoredVar_Register() #4 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -102;
	  return 0;
      }
    sqlite3_free_table (results);

/* executing a SQL Procedure - with stored variables */
    sql = "SELECT SqlProc_Execute(SqlProc_FromText('SELECT @num1@ * @num2@;'))";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SqlProc_Execute() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -103;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "SqlProc_Execute() #4 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -104;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "SqlProc_Execute() #4 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -105;
	  return 0;
      }
    sqlite3_free_table (results);

/* executing a SQL Procedure - with explicitly set variables */
    sql = "SELECT SqlProc_Execute(SqlProc_FromText('SELECT @num1@ * @num2@;'), "
	"'@num1@=2.55', '@num2@=3.05');";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SqlProc_Execute() #5 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -106;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "SqlProc_Execute() #5 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -107;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "SqlProc_Execute() #5 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -108;
	  return 0;
      }
    sqlite3_free_table (results);

/* testing SqlProc_Exit */
    sql = "SELECT SqlProc_Exit();";
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SqlProc_Exit() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -109;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "SqlProc_Exit() #1 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -110;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "SqlProc_Exit() #1 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -111;
	  return 0;
      }
    sqlite3_free_table (results);

    return 1;
}

#endif

int
main (int argc, char *argv[])
{
    int retcode = 0;
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    void *cache = spatialite_alloc_connection ();
    char *old_SPATIALITE_SECURITY_ENV = NULL;
#ifdef _WIN32
    char *env;
#endif /* not WIN32 */

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    old_SPATIALITE_SECURITY_ENV = getenv ("SPATIALITE_SECURITY");
#ifdef _WIN32
    putenv ("SPATIALITE_SECURITY=relaxed");
#else /* not WIN32 */
    setenv ("SPATIALITE_SECURITY", "relaxed", 1);
#endif

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

#ifndef OMIT_ICONV		/* only if ICONV is enabled */

/*tests: level 0 */
    if (!do_level0_tests (handle, &retcode))
	goto end;

/*tests: level 1 */
    if (!do_level1_tests (handle, &retcode))
	goto end;

    if (old_SPATIALITE_SECURITY_ENV)
      {
#ifdef _WIN32
	  env =
	      sqlite3_mprintf ("SPATIALITE_SECURITY=%s",
			       old_SPATIALITE_SECURITY_ENV);
	  putenv (env);
	  sqlite3_free (env);
#else /* not WIN32 */
	  setenv ("SPATIALITE_SECURITY", old_SPATIALITE_SECURITY_ENV, 1);
#endif
      }
    else
      {
#ifdef _WIN32
	  putenv ("SPATIALITE_SECURITY=");
#else /* not WIN32 */
	  unsetenv ("SPATIALITE_SECURITY");
#endif
      }

/*tests: level 2 */
    if (!do_level2_tests (handle, &retcode))
	goto end;

/*tests: level 3 */
    if (!do_level3_tests (handle, &retcode))
	goto end;

/*tests: level 4 */
    if (!do_level4_tests (handle, &retcode))
	goto end;

/*tests: level 5 */
    if (!do_level5_tests (handle, &retcode))
	goto end;

  end:

#else
    if (old_SPATIALITE_SECURITY_ENV != NULL)
	old_SPATIALITE_SECURITY_ENV = NULL;	/* silencing stupid compiler warnings */
#endif /* end ICONV */

    sqlite3_close (handle);
    spatialite_cleanup_ex (cache);
    spatialite_shutdown ();
    return retcode;
}
