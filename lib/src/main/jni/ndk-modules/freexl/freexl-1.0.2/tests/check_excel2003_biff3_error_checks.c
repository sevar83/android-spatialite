/* 
/ check_excel2003_biff3_error_checks.c
/
/ Test cases for non-normal Excel BIFF3 operations
/
/ Author: Brad Hards bradh@frogmouth.net
/
/ ------------------------------------------------------------------------------
/ 
/ Version: MPL 1.1/GPL 2.0/LGPL 2.1
/ 
/ The contents of this file are subject to the Mozilla Public License Version
/ 1.1 (the "License"); you may not use this file except in compliance with
/ the License. You may obtain a copy of the License at
/ http://www.mozilla.org/MPL/
/ 
/ Software distributed under the License is distributed on an "AS IS" basis,
/ WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
/ for the specific language governing rights and limitations under the
/ License.
/
/ The Original Code is the FreeXL library
/
/ The Initial Developer of the Original Code is Alessandro Furieri
/ 
/ Portions created by the Initial Developer are Copyright (C) 2011
/ the Initial Developer. All Rights Reserved.
/ 
/ Contributor(s):
/ Brad Hards 
/
/ Alternatively, the contents of this file may be used under the terms of
/ either the GNU General Public License Version 2 or later (the "GPL"), or
/ the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
/ in which case the provisions of the GPL or the LGPL are applicable instead
/ of those above. If you wish to allow use of your version of this file only
/ under the terms of either the GPL or the LGPL, and not to allow others to
/ use your version of this file under the terms of the MPL, indicate your
/ decision by deleting the provisions above and replace them with the notice
/ and other provisions required by the GPL or the LGPL. If you do not delete
/ the provisions above, a recipient may use your version of this file under
/ the terms of any one of the MPL, the GPL or the LGPL.
/ 
*/#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "freexl.h"

int main (int argc, char *argv[])
{
    const void *handle;
    int ret;
    unsigned int info;
    const char *worksheet_name;
    unsigned short active_idx;
    unsigned int num_rows;
    unsigned short num_columns;
    FreeXL_CellValue cell_value;
   
    ret = freexl_open_info ("testdata/simple2003_3.xls", &handle);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "OPEN INFO ERROR: %d\n", ret);
	return -1;
    }

    ret = freexl_get_info(NULL, FREEXL_CFBF_VERSION, &info);
    if (ret != FREEXL_NULL_HANDLE) {
	fprintf(stderr, "GET_INFO unexpected ret for CFBF version handle: %d\n", ret);
	return -3;
    }

    ret = freexl_get_info(handle, FREEXL_CFBF_VERSION, NULL);
    if (ret != FREEXL_NULL_ARGUMENT) {
	fprintf(stderr, "GET_INFO unexpected ret for CFBF version info: %d\n", ret);
	return -4;
    }

    ret = freexl_get_worksheet_name(NULL, 0, &worksheet_name);
    if (ret != FREEXL_NULL_HANDLE) {
	fprintf(stderr, "Unexpected result getting worksheet name handle: %d\n", ret);
	return -5;
    }

    ret = freexl_get_worksheet_name(handle, 0, NULL);
    if (ret != FREEXL_NULL_ARGUMENT) {
	fprintf(stderr, "Unexpected result getting worksheet name string: %d\n", ret);
	return -6;
    }

    ret = freexl_select_active_worksheet (NULL, 0);
    if (ret != FREEXL_NULL_HANDLE) {
	fprintf(stderr, "Unexpected result setting active worksheet handle: %d\n", ret);
	return -7;
    }
    
    ret = freexl_select_active_worksheet (handle, 34);
    if (ret != FREEXL_BIFF_ILLEGAL_SHEET_INDEX) {
	fprintf(stderr, "Unexpected result setting active worksheet index: %d\n", ret);
	return -8;
    }

    ret = freexl_get_active_worksheet (NULL, &active_idx);
    if (ret != FREEXL_NULL_HANDLE) {
	fprintf(stderr, "Unexpected result getting active worksheet handle: %d\n", ret);
	return -9;
    }

    ret = freexl_get_active_worksheet (handle, NULL);
    if (ret != FREEXL_NULL_ARGUMENT) {
	fprintf(stderr, "Unexpected result getting active worksheet arg: %d\n", ret);
	return -10;
    }
    
    ret = freexl_worksheet_dimensions (NULL, &num_rows, &num_columns);
    if (ret != FREEXL_NULL_HANDLE) {
	fprintf(stderr, "Unexpected result getting worksheet dimensions handle: %d\n", ret);
	return -12;
    }
    ret = freexl_worksheet_dimensions (handle, NULL, &num_columns);
    if (ret != FREEXL_NULL_ARGUMENT) {
	fprintf(stderr, "Unexpected result getting worksheet dimensions row: %d\n", ret);
	return -13;
    }
    ret = freexl_worksheet_dimensions (handle, &num_rows, NULL);
    if (ret != FREEXL_NULL_ARGUMENT) {
	fprintf(stderr, "Unexpected result getting worksheet dimensions col: %d\n", ret);
	return -14;
    }
    ret = freexl_worksheet_dimensions (handle, NULL, NULL);
    if (ret != FREEXL_NULL_ARGUMENT) {
	fprintf(stderr, "Unexpected result getting worksheet dimensions both: %d\n", ret);
	return -15;
    }

    ret = freexl_get_cell_value (NULL, 0, 0, &cell_value);
    if (ret != FREEXL_NULL_HANDLE) {
	fprintf(stderr, "Unexpected result getting cell value (0,0) handle: %d\n", ret);
	return -16;
    }

    ret = freexl_get_cell_value (handle, 0, 0, NULL);
    if (ret != FREEXL_NULL_ARGUMENT) {
	fprintf(stderr, "Unexpected result getting cell value (0,0) val: %d\n", ret);
	return -17;
    }
 
    ret = freexl_close (handle);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "CLOSE ERROR: %d\n", ret);
	return -2;
    }

    return 0;
}
