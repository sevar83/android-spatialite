/* 
/ check_excel2003_biff3.c
/
/ Test cases for Excel BIFF3 format files
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
*/

#include <stdlib.h>
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
   
    ret = freexl_open ("testdata/simple2003_3.xls", &handle);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "OPEN ERROR: %d\n", ret);
	return -1;
    }
    
    ret = freexl_get_info(handle, FREEXL_CFBF_VERSION, &info);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for CFBF version: %d\n", ret);
	return -3;
    }
    if (info != FREEXL_UNKNOWN)
    {
	fprintf(stderr, "Unexpected CFBF_VERSION: %d\n", info);
	return -4;
    }

    ret = freexl_get_info(handle, FREEXL_CFBF_SECTOR_SIZE, &info);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for sector size: %d\n", ret);
	return -5;
    }
    if (info != FREEXL_UNKNOWN)
    {
	fprintf(stderr, "Unexpected CFBF_SECTOR_SIZE: %d\n", info);
	return -6;
    }
    
    ret = freexl_get_info(handle, FREEXL_CFBF_FAT_COUNT, &info);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for fat count: %d\n", ret);
	return -7;
    }
    if (info != 0)
    {
	fprintf(stderr, "Unexpected CFBF_FAT_COUNT: %d\n", info);
	return -8;
    }

    ret = freexl_get_info(handle, FREEXL_BIFF_VERSION, &info);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for BIFF version: %d\n", ret);
	return -9;
    }
    if (info != 3)
    {
	fprintf(stderr, "Unexpected BIFF version: %d\n", info);
	return -10;
    }
 
    ret = freexl_get_info(handle, FREEXL_BIFF_MAX_RECSIZE, &info);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for BIFF record size: %d\n", ret);
	return -11;
    }
    if (info != FREEXL_UNKNOWN)
    {
	fprintf(stderr, "Unexpected BIFF max record size: %d\n", info);
	return -12;
    }
    
    ret = freexl_get_info(handle, FREEXL_BIFF_DATEMODE, &info);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for BIFF date mode: %d\n", ret);
	return -13;
    }
    if (info != FREEXL_BIFF_DATEMODE_1900)
    {
	fprintf(stderr, "Unexpected BIFF date mode: %d\n", info);
	return -14;
    }

    ret = freexl_get_info(handle, FREEXL_BIFF_PASSWORD, &info);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for BIFF password mode: %d\n", ret);
	return -15;
    }
    if (info != FREEXL_BIFF_PLAIN) {
	fprintf(stderr, "Unexpected BIFF password mode: %d\n", info);
	return -16;
    }

    ret = freexl_get_info(handle, FREEXL_BIFF_CODEPAGE, &info);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for BIFF codepage: %d\n", ret);
	return -17;
    }
    if (info != FREEXL_BIFF_CP1252) {
	fprintf(stderr, "Unexpected BIFF codepage: %d\n", info);
	return -18;
    }

    ret = freexl_get_info(handle, FREEXL_BIFF_SHEET_COUNT, &info);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for BIFF worksheet count: %d\n", ret);
	return -19;
    }
    if (info != 1) {
	fprintf(stderr, "Unexpected BIFF worksheet count: %d\n", info);
	return -20;
    }

    ret = freexl_get_info(handle, FREEXL_BIFF_FORMAT_COUNT, &info);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for BIFF format count: %d\n", ret);
	return -21;
    }
    if (info != 10) {
	fprintf(stderr, "Unexpected BIFF format count: %d\n", info);
	return -22;
    }

    ret = freexl_get_info(handle, FREEXL_BIFF_XF_COUNT, &info);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for BIFF extended format count: %d\n", ret);
	return -23;
    }
    if (info != 24) {
	fprintf(stderr, "Unexpected BIFF extended format count: %d\n", info);
	return -24;
    }
    
    /* We only have one worksheet, zero index */
    ret = freexl_get_worksheet_name(handle, 0, &worksheet_name);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting worksheet name: %d\n", ret);
	return -25;
    }
    if (strcmp(worksheet_name, "Worksheet") != 0) {
	fprintf(stderr, "Unexpected worksheet name: %s\n", worksheet_name);
	return -26;
    }    

    ret = freexl_select_active_worksheet (handle, 0);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error setting active worksheet: %d\n", ret);
	return -27;
    }
    
    ret = freexl_get_active_worksheet (handle, &active_idx);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting active worksheet: %d\n", ret);
	return -28;
    }
    if (active_idx != 0) {
	fprintf(stderr, "Unexpected active sheet: %d\n", info);
	return -29;
    }
    
    ret = freexl_worksheet_dimensions (handle, &num_rows, &num_columns);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting worksheet dimensions: %d\n", ret);
	return -30;
    }
    if ((num_rows != 4) || (num_columns != 5)) {
	fprintf(stderr, "Unexpected active sheet dimensions: %u x %u\n",
	    num_rows, num_columns);
	return -31;
    }

    ret = freexl_get_cell_value (handle, 0, 0, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (0,0): %d\n", ret);
	return -32;
    }
    if (cell_value.type != FREEXL_CELL_TEXT) {
	fprintf(stderr, "Unexpected cell (0,0) type: %u\n", cell_value.type);
	return -33;
    }
    if (strcmp(cell_value.value.text_value, "Column 1") != 0) {
	fprintf(stderr, "Unexpected cell (0,0) value: %s\n", cell_value.value.text_value);
	return -34;
    }
    
    ret = freexl_get_cell_value(handle, 3, 0, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (3,0): %d\n", ret);
	return -35;
    }
    if (cell_value.type != FREEXL_CELL_DOUBLE) {
	fprintf(stderr, "Unexpected cell (3,0) type: %u\n", cell_value.type);
	return -36;
    }
    if (cell_value.value.double_value != 3.14) {
	fprintf(stderr, "Unexpected cell (3,0) value: %g\n", cell_value.value.double_value);
	return -37;
    }    

    ret = freexl_get_cell_value(handle, 3, 1, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (3,1): %d\n", ret);
	return -38;
    }
    if (cell_value.type != FREEXL_CELL_DOUBLE) {
	fprintf(stderr, "Unexpected cell (3,1) type: %u\n", cell_value.type);
	return -39;
    }
    if (cell_value.value.double_value != -56.3089) {
	fprintf(stderr, "Unexpected cell (3,1) value: %g\n", cell_value.value.double_value);
	return -40;
    } 

    ret = freexl_get_cell_value(handle, 3, 2, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (3,2): %d\n", ret);
	return -41;
    }
    if (cell_value.type != FREEXL_CELL_DOUBLE) {
	fprintf(stderr, "Unexpected cell (3,2) type: %u\n", cell_value.type);
	return -42;
    }
    if (cell_value.value.double_value != 0.67) {
	fprintf(stderr, "Unexpected cell (3,2) value: %g\n", cell_value.value.double_value);
	return -43;
    } 

    ret = freexl_get_cell_value(handle, 3, 3, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (3,3): %d\n", ret);
	return -44;
    }
    if (cell_value.type != FREEXL_CELL_DATE) {
	fprintf(stderr, "Unexpected cell (3,3) type: %u\n", cell_value.type);
	return -45;
    }
    if (strcmp(cell_value.value.text_value, "1967-10-01") != 0) {
	fprintf(stderr, "Unexpected cell (3,3) value: %s\n", cell_value.value.text_value);
	return -46;
    } 

    ret = freexl_get_cell_value(handle, 3, 4, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (3,4): %d\n", ret);
	return -47;
    }
    if (cell_value.type != FREEXL_CELL_DOUBLE) {
	fprintf(stderr, "Unexpected cell (3,4) type: %u\n", cell_value.type);
	return -48;
    }
    if (cell_value.value.double_value != 4) {
	fprintf(stderr, "Unexpected cell (3,4) value: %f\n", cell_value.value.double_value);
	return -49;
    }

    /* error cases */
    ret = freexl_get_cell_value(handle, 7, 3, &cell_value);
    if (ret != FREEXL_ILLEGAL_CELL_ROW_COL) {
	fprintf(stderr, "Unexpected result for (7,3): %d\n", ret);
	return -50;
    }
    ret = freexl_get_cell_value(handle, 2, 99, &cell_value);
    if (ret != FREEXL_ILLEGAL_CELL_ROW_COL) {
	fprintf(stderr, "Unexpected result for (2,99): %d\n", ret);
	return -51;
    }
    ret = freexl_get_cell_value(handle, 4, 2, &cell_value);
    if (ret != FREEXL_ILLEGAL_CELL_ROW_COL) {
	fprintf(stderr, "Unexpected result for (4,2): %d\n", ret);
	return -52;
    }
    ret = freexl_get_cell_value(handle, 3, 5, &cell_value);
    if (ret != FREEXL_ILLEGAL_CELL_ROW_COL) {
	fprintf(stderr, "Unexpected result for (3,5): %d\n", ret);
	return -53;
    }
        
    ret = freexl_close (handle);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "CLOSE ERROR: %d\n", ret);
	return -2;
    }

    return 0;
}
