/* 
 stored_procedues.h -- SQL Procedures and Stored Procedures functions
  
 version 4.5, 2017 October 22

 Author: Sandro Furieri a.furieri@lqt.it

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

/**
 \file stored_procedures.h

 SQL Procedures and Stored Procedures functions
 */
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifdef DLL_EXPORT
#define SQLPROC_DECLARE __declspec(dllexport)
#else
#define SQLPROC_DECLARE extern
#endif
#endif

#ifndef _SQLPROC_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _SQLPROC_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/* constants */
/** SQL Procedure BLOB start marker */
#define SQLPROC_START	0xcd
/** SQL Procedure BLOB delimiter marker */
#define SQLPROC_DELIM	0x87
/** SQL Procedure BLOB stop marker */
#define SQLPROC_STOP	0xdc

/* data structures */
/**
 SqlProc: Variable with value
 */
    typedef struct gaiaSqlProc_VariableStruct
    {
/** Variable Name */
	char *Name;
/** Variable Value */
	char *Value;
/** Pointer to next Variable (linked list) */
	struct gaiaSqlProc_VariableStruct *Next;
    } SqlProc_Variable;
/**
 Typedef for SqlProc Variable structure

 \sa SqlProc_VarList
 */
    typedef SqlProc_Variable *SqlProc_VariablePtr;

/**
 SqlProc: List of Variables with values
 */
    typedef struct gaiaSqlProc_VarListStruct
    {
/** invalid object */
	int Error;
/** Error Message (if any) */
	char *ErrMessage;
/** pointer to first Variable [linked list] */
	SqlProc_VariablePtr First;	/* Variables linked list - first */
/** pointer to last Variable [linked list] */
	SqlProc_VariablePtr Last;	/* Variables linked list - last */
    } SqlProc_VarList;
/**
 Typedef for SqlProc Variables List structure

 \sa SqlProc_Variable
 */
    typedef SqlProc_VarList *SqlProc_VarListPtr;


/* function prototypes */

/**
 Return the most recent SQL Procedure error (if any)

 \param p_cache a memory pointer returned by spatialite_alloc_connection()

 \return the most recent SQL Procedure error message (if any); 
  NULL in any other case.
 */
    SQLPROC_DECLARE char *gaia_sql_proc_get_last_error (const void *p_cache);

/**
 Will enable/disable a Logfile supporting Execute methods

 \param p_cache a memory pointer returned by spatialite_alloc_connection()
 \param filepath the pathname of the Logfile. NULL to disable logging.
 \param append if TRUE the Logfile will be opened in append mode,
 otherwise will be trucated.
 
 \return 0 on failure, any other value on success.
 
 \sa gaia_sql_proc_execute
 */
    SQLPROC_DECLARE int gaia_sql_proc_logfile (const void *p_cache,
					       const char *filepath,
					       int append);

/**
 Creates an empty list of Variables with Values
 
 \return pointer to the Variables List
 
 \sa gaia_sql_proc_destroy_variables, gaia_sql_proc_add_variable,
 gaia_sql_proc_cooked_sql, gaia_sql_proc_execute
 
 \note you are responsible to destroy (before or after) the Variables List
 Object returned by this function by calling gaia_sql_proc_destroy_variables().
 */
    SQLPROC_DECLARE SqlProc_VarListPtr gaia_sql_proc_create_variables ();

/**
 Destroys a list of Variables with Values
 
 \param list pointer to the Variables List Object to be destroyed.
 
 \sa gaia_sql_proc_create_variables
 */
    SQLPROC_DECLARE void
	gaia_sql_proc_destroy_variables (SqlProc_VarListPtr list);

/**
 Add a Variable with Value to the List
 
 \param list pointer to the Variables List Object.
 \param str text string expected to contain a Variable with Value
 in the canonical form '@varname@=value'.
 
 \return 0 on failure, any other value on success.
 
 \sa gaia_sql_proc_create_variables, gaia_sql_proc_destroy_variables
 */
    SQLPROC_DECLARE int
	gaia_sql_proc_add_variable (SqlProc_VarListPtr list, const char *str);

/**
 Builds a SQL Procedure BLOB object from Text
 
 \param cache the same memory pointer passed to the corresponding call to
 spatialite_init_ex() and returned by spatialite_alloc_connection()
 \param sql body of the SQL-script
 \param charset the GNU ICONV name identifying the sql body charset
 \param blob on succesfull completion this pointer will reference the
  BLOB SQL Procedure Object (NULL on failure).
 \param blob_sz on succesfull completion this pointer will reference 
 the size (in bytes) of the BLOB SQL Procedure Object.
 
 \return 0 on failure, any other value on success.
 
 \sa gaia_sql_proc_import, gaia_sql_proc_get_last_error,
 gaia_sql_proc_is_valid, gaia_sql_proc_count,
 gaia_sql_proc_variable, gaia_sql_proc_all_variables,
 gaia_sql_proc_raw_sql, gaia_sql_proc_cooked_sql
 
 \note you are responsible to free (before or after) the BLOB
 SQL Procedure Object returned by this function.
 */
    SQLPROC_DECLARE int gaia_sql_proc_parse (const void *cache,
					     const char *sql,
					     const char *charset,
					     unsigned char **blob,
					     int *blob_sz);

/**
 Builds a SQL Procedure BLOB object from an external file
 
 \param cache the same memory pointer passed to the corresponding call to
 spatialite_init_ex() and returned by spatialite_alloc_connection()
 \param filepath path to the SQL-script to be loaded.
 \param charset the GNU ICONV name identifying the sql body charset
 \param blob on succesfull completion this pointer will reference the
  BLOB SQL Procedure Object (NULL on failure).
 \param blob_sz on succesfull completion this pointer will reference 
 the size (in bytes) of the BLOB SQL Procedure Object.
 
 \return 0 on failure, any other value on success.
 
 \sa gaia_sql_proc_parse, gaia_sql_proc_get_last_error
 
 \note you are responsible to free (before or after) the BLOB
 SQL Procedure Object returned by this function.
 */
    SQLPROC_DECLARE int gaia_sql_proc_import (const void *cache,
					      const char *filepath,
					      const char *charset,
					      unsigned char **blob,
					      int *blob_sz);

/**
 Checks if a BLOB is a valid SQL Procedure Object
 
 \param blob pointer to the BLOB Object.
 \param blob_sz size (in bytes) of the BLOB Object.
 
 \return 0 on failure: any other different value on success.
 
 \sa gaia_sql_proc_parse
*/
    SQLPROC_DECLARE int gaia_sql_proc_is_valid (const unsigned char
						*blob, int blob_sz);

/**
 Checks if a TEXT is a valid SQL Variable with Value
 
 \param str the text string to be evaluated.
 
 \return 0 on failure: any other different value on success.
*/
    SQLPROC_DECLARE int gaia_sql_proc_is_valid_var_value (const char *str);

/**
 Return the total count of Variables from a SQL Procedure Object
 
 \param blob pointer to the BLOB Object.
 \param blob_sz size (in bytes) of the BLOB Object.
 
 \return the total count of Variables or -1 on invalid Object
 
 \sa gaia_sql_proc_parse, gaia_sql_proc_variable
*/
    SQLPROC_DECLARE int gaia_sql_proc_var_count (const unsigned char
						 *blob, int blob_sz);

/**
 Return the Name of the Nth Variable from a SQL Procedure Object
 
 \param blob pointer to the BLOB Object.
 \param blob_sz size (in bytes) of the BLOB Object.
 \param index the first Variable has Index=0.
 
 \return the name of the Nth Variable or NULL on invalid Object or
 invalid Index.
 
 \sa gaia_sql_proc_parse, gaia_sql_proc_var_count,
 gaia_sql_proc_all_variables, gaia_sql_proc_raw_sql
 
 \note you are responsible to free (before or after) the Variable Name
 returned by this function.
*/
    SQLPROC_DECLARE char *gaia_sql_proc_variable (const unsigned
						  char *blob,
						  int blob_sz, int index);

/**
 Return the Names of all Variables from a SQL Procedure Object
 
 \param blob pointer to the BLOB Object.
 \param blob_sz size (in bytes) of the BLOB Object.
 
 \return a list of Variable Names separated by spaces or NULL on
 invalid arguments.
 
 \sa gaia_sql_proc_parse, gaia_sql_proc_var_count
 
 \note you are responsible to free (before or after) the Variable Names
 returned by this function.
*/
    SQLPROC_DECLARE char *gaia_sql_proc_all_variables (const
						       unsigned char
						       *blob, int blob_sz);

/**
 Return the raw SQL body from a SQL Procedure Object
 
 \param blob pointer to the BLOB Object.
 \param blob_sz size (in bytes) of the BLOB Object.
 
 \return the raw SQL body or NULL on invalid arguments.
 
 \sa gaia_sql_proc_parse, gaia_sql_proc_var_count
 
 \note you are responsible to free (before or after) the raw SQL body
 returned by this function.
*/
    SQLPROC_DECLARE char *gaia_sql_proc_raw_sql (const unsigned char
						 *blob, int blob_sz);

/**
 Return a cooked SQL body from a raw SQL body by replacing Variable Values
 
 \param handle pointer to the current DB connection.
 \param cache the same memory pointer passed to the corresponding call to
 spatialite_init_ex() and returned by spatialite_alloc_connection()
 \param blob pointer to the BLOB Object.
 \param blob_sz size (in bytes) of the BLOB Object.
 \param variables list of Variables with Values.
 \param sql on succesfull completions will point to the "cooked" SQL body;
 NULL on failure.
 
 \return 0 on failure: any other different value on success.
 
 \sa gaia_sql_proc_parse, gaia_sql_proc_var_count,
 gaia_sql_proc_create_variables
 
 \note you are responsible to free (before or after) the cooked SQL body
 returned by this function.
*/
    SQLPROC_DECLARE int gaia_sql_proc_cooked_sql (sqlite3 * handle,
						  const void *cache,
						  const unsigned char *blob,
						  int blob_sz,
						  SqlProc_VarListPtr variables,
						  char **sql);

/**
 Will attempt to create the Stored Procedures Tables if not already existing
  
 \param handle pointer to the current DB connection.
 \param cache the same memory pointer passed to the corresponding call to
 spatialite_init_ex() and returned by spatialite_alloc_connection()
 
 \return 0 on failure: any other different value on success.
 
 \sa gaia_stored_proc_store, gaia_stored_proc_fetch,
 gaia_stored_proc_delete, gaia_stored_proc_update_title, 
 gaia_stored_proc_update_sql, gaia_stored_var_store,
 gaia_stored_var_fetch, gaia_stored_var_delete,
 gaia_stored_var_update_title, gaia_stored_var_update_value
*/
    SQLPROC_DECLARE int gaia_stored_proc_create_tables (sqlite3 *
							handle,
							const void *cache);

/**
 Permanently inserts a Stored Procedure into the DB
  
 \param handle pointer to the current DB connection.
 \param cache the same memory pointer passed to the corresponding call to
 spatialite_init_ex() and returned by spatialite_alloc_connection()
 \param name unique identifier of the Stored Procedure.
 \param title short description of the Stored Procedure.
 \param blob pointer to the BLOB Object.
 \param blob_sz size (in bytes) of the BLOB Object.
 
 \return 0 on failure: any other different value on success.
 
 \sa gaia_stored_proc_create_tables, gaia_stored_proc_fetch,
 gaia_stored_proc_delete, gaia_stored_proc_update_title, 
 gaia_stored_proc_update_sql, gaia_stored_var_store,
 gaia_stored_var_fetch, gaia_stored_var_delete,
 gaia_stored_var_update_title, gaia_stored_var_update_value
*/
    SQLPROC_DECLARE int gaia_stored_proc_store (sqlite3 * handle,
						const void *cache,
						const char *name,
						const char *title,
						const unsigned char
						*blob, int blob_sz);

/**
 Retrieves a permanent Stored Procedure from the DB
  
 \param handle pointer to the current DB connection.
 \param cache the same memory pointer passed to the corresponding call to
 spatialite_init_ex() and returned by spatialite_alloc_connection()
 \param name unique identifier of the Stored Procedure.
 \param blob on succesfull completion this pointer will reference the
  BLOB SQL Procedure Object (NULL on failure).
 \param blob_sz on succesfull completion this pointer will reference 
 the size (in bytes) of the BLOB SQL Procedure Object.
 
 \return 0 on failure: any other different value on success.
 
 \sa gaia_stored_proc_create_tables, gaia_stored_proc_store,
 gaia_stored_proc_delete, gaia_stored_proc_update_title, 
 gaia_stored_proc_update_sql, gaia_stored_var_store,
 gaia_stored_var_fetch, gaia_stored_var_delete,
 gaia_stored_var_update_title, gaia_stored_var_update_value
 
 \note you are responsible to free (before or after) the BLOB
 SQL Procedure Object returned by this function.
*/
    SQLPROC_DECLARE int gaia_stored_proc_fetch (sqlite3 * handle,
						const void *cache,
						const char *name,
						unsigned char **blob,
						int *blob_sz);

/**
 Removes a permanent Stored Procedure from the DB
  
 \param handle pointer to the current DB connection.
 \param cache the same memory pointer passed to the corresponding call to
 spatialite_init_ex() and returned by spatialite_alloc_connection()
 \param name unique identifier of the Stored Procedure.
 
 \return 0 on failure: any other different value on success.
 
 \sa gaia_stored_proc_create_tables, gaia_stored_proc_store,
 gaia_stored_proc_fetche, gaia_stored_proc_update_title, 
 gaia_stored_proc_update_sql, gaia_stored_var_store,
 gaia_stored_var_fetch, gaia_stored_var_delete,
 gaia_stored_var_update_title, gaia_stored_var_update_value
*/
    SQLPROC_DECLARE int gaia_stored_proc_delete (sqlite3 * handle,
						 const void *cache,
						 const char *name);

/**
 Updates the Title on a permanent Stored Procedure
  
 \param handle pointer to the current DB connection.
 \param cache the same memory pointer passed to the corresponding call to
 spatialite_init_ex() and returned by spatialite_alloc_connection()
 \param name unique identifier of the Stored Procedure.
 \param title short description of the Stored Procedure.
 
 \return 0 on failure: any other different value on success.
 
 \sa gaia_stored_proc_create_tables, gaia_stored_proc_store,
 gaia_stored_proc_fetch, gaia_stored_proc_delete, 
 gaia_stored_proc_update_sql, gaia_stored_var_store,
 gaia_stored_var_fetch, gaia_stored_var_delete,
 gaia_stored_var_update_title, gaia_stored_var_update_value
*/
    SQLPROC_DECLARE int gaia_stored_proc_update_title (sqlite3 *
						       handle,
						       const void
						       *cache,
						       const char
						       *name,
						       const char *title);

/**
 Updates the Raw SQL Body on a permanent Stored Procedure
  
 \param handle pointer to the current DB connection.
 \param cache the same memory pointer passed to the corresponding call to
 spatialite_init_ex() and returned by spatialite_alloc_connection()
 \param name unique identifier of the Stored Procedure.
 \param blob pointer to the BLOB Object.
 \param blob_sz size (in bytes) of the BLOB Object.
 
 \return 0 on failure: any other different value on success.
 
 \sa gaia_stored_proc_create_tables, gaia_stored_proc_store,
 gaia_stored_proc_fetch, gaia_stored_proc_delete, 
 gaia_stored_proc_update_title, gaia_stored_var_store,
 gaia_stored_var_fetch, gaia_stored_var_delete,
 gaia_stored_var_update_title, gaia_stored_var_update_value
*/
    SQLPROC_DECLARE int gaia_stored_proc_update_sql (sqlite3 *
						     handle,
						     const void
						     *cache,
						     const char
						     *name,
						     const
						     unsigned char
						     *blob, int blob_sz);

/**
 Permanently inserts a Stored Variable into the DB
  
 \param handle pointer to the current DB connection.
 \param cache the same memory pointer passed to the corresponding call to
 spatialite_init_ex() and returned by spatialite_alloc_connection()
 \param name unique identifier of the Stored Variable.
 \param title short description of the Stored Variable.
 \param value the Variable Value in its textual representation.
 
 \return 0 on failure: any other different value on success.
 
 \sa gaia_stored_proc_create_tables, gaia_stored_proc_store,
 gaia_stored_proc_fetch, gaia_stored_proc_delete, 
 gaia_stored_proc_update_title, gaia_stored_proc_update_sql,
 gaia_stored_var_fetch, gaia_stored_var_delete,
 gaia_stored_var_update_title, gaia_stored_var_update_value
*/
    SQLPROC_DECLARE int gaia_stored_var_store (sqlite3 * handle,
					       const void *cache,
					       const char *name,
					       const char *title,
					       const char *value);

/**
 Retrieves a Stored Variable from the DB
  
 \param handle pointer to the current DB connection.
 \param cache the same memory pointer passed to the corresponding call to
 spatialite_init_ex() and returned by spatialite_alloc_connection()
 \param name unique identifier of the Stored Variable.
 \param var_with_val if set to TRUE value will point to a Variable with 
 Name string, otherwise (FALSE) it will point to a bare textual value. 
 \param value on succesfull completion this pointer will reference 
 the Stored Variable represented as a Variable with Value or as a bare
 textual value depending on var_with_val setting.
 
 \return 0 on failure: any other different value on success.
 
 \sa gaia_stored_proc_create_tables, gaia_stored_proc_store,
 gaia_stored_proc_fetch, gaia_stored_proc_delete, 
 gaia_stored_proc_update_title, gaia_stored_proc_update_sql,
 gaia_stored_var_store, gaia_stored_var_delete,
 gaia_stored_var_update_title, gaia_stored_var_update_value
 
 \note you are responsible to free (before or after) the Text
 String returned by this function.
*/
    SQLPROC_DECLARE int gaia_stored_var_fetch (sqlite3 * handle,
					       const void *cache,
					       const char *name,
					       int var_with_val, char **value);

/**
 Removes a Stored Variable from the DB
  
 \param handle pointer to the current DB connection.
 \param cache the same memory pointer passed to the corresponding call to
 spatialite_init_ex() and returned by spatialite_alloc_connection()
 \param name unique identifier of the Stored Variable.
 
 \return 0 on failure: any other different value on success.
 
 \sa gaia_stored_proc_create_tables, gaia_stored_proc_store,
 gaia_stored_proc_fetch, gaia_stored_proc_delete, 
 gaia_stored_proc_update_title, gaia_stored_proc_update_sql,
 gaia_stored_var_store, gaia_stored_var_fetch,
 gaia_stored_var_update_title, gaia_stored_var_update_value
*/
    SQLPROC_DECLARE int gaia_stored_var_delete (sqlite3 * handle,
						const void *cache,
						const char *name);

/**
 Updates the Title on a Stored Variable
  
 \param handle pointer to the current DB connection.
 \param cache the same memory pointer passed to the corresponding call to
 spatialite_init_ex() and returned by spatialite_alloc_connection()
 \param name unique identifier of the Stored Variable.
 \param title short description of the Stored Variable.
 
 \return 0 on failure: any other different value on success.
 
 \sa gaia_stored_proc_create_tables, gaia_stored_proc_store,
 gaia_stored_proc_fetch, gaia_stored_proc_delete, 
 gaia_stored_proc_update_title, gaia_stored_proc_update_sql,
 gaia_stored_var_store, gaia_stored_var_store, gaia_stored_var_fetch,
 gaia_stored_var_delete, gaia_stored_var_update_value
*/
    SQLPROC_DECLARE int gaia_stored_var_update_title (sqlite3 *
						      handle,
						      const void
						      *cache,
						      const char
						      *name, const char *title);

/**
 Updates the Value on a Stored Variable
  
 \param handle pointer to the current DB connection.
 \param cache the same memory pointer passed to the corresponding call to
 spatialite_init_ex() and returned by spatialite_alloc_connection()
 \param name unique identifier of the Stored Variable.
 \param value the Variable Value in its textual representation.
 
 \return 0 on failure: any other different value on success.
 
 \sa gaia_stored_proc_create_tables, gaia_stored_proc_store,
 gaia_stored_proc_fetch, gaia_stored_proc_delete, 
 gaia_stored_proc_update_title, gaia_stored_proc_update_sql,
 gaia_stored_var_store, gaia_stored_var_store, gaia_stored_var_fetch,
 gaia_stored_var_delete, gaia_stored_var_update_title
*/
    SQLPROC_DECLARE int gaia_stored_var_update_value (sqlite3 *
						      handle,
						      const void
						      *cache,
						      const char
						      *name, const char *value);

/**
 Executing a cooked SQL Procedure
  
 \param handle pointer to the current DB connection.
 \param cache the same memory pointer passed to the corresponding call to
 spatialite_init_ex() and returned by spatialite_alloc_connection()
 \param sql the cooked SQL Body to be executed.
 
 \return 0 on failure: any other different value on success.
 
 \sa gaia_sql_proc_logfile
*/
    SQLPROC_DECLARE int gaia_sql_proc_execute (sqlite3 *
					       handle,
					       const void *cache,
					       const char *sql);

#ifdef __cplusplus
}
#endif

#endif				/* _SQLPROC_H */
