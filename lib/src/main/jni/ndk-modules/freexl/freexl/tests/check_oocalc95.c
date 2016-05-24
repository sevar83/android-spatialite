/* 
/ check_oocalc95.c
/
/ Test cases for LibreOffice BIFF5 format files
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

    ret = freexl_open ("testdata/oocalc_simple95.xls", &handle);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "OPEN ERROR: %d\n", ret);
	return -1;
    }
    
    ret = freexl_get_info(handle, FREEXL_CFBF_VERSION, &info);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for CFBF version: %d\n", ret);
	return -3;
    }
    if (info != FREEXL_CFBF_VER_3)
    {
	fprintf(stderr, "Unexpected CFBF_VERSION: %d\n", info);
	return -4;
    }

    ret = freexl_get_info(handle, FREEXL_CFBF_SECTOR_SIZE, &info);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for sector size: %d\n", ret);
	return -5;
    }
    if (info != FREEXL_CFBF_SECTOR_512)
    {
	fprintf(stderr, "Unexpected CFBF_SECTOR_SIZE: %d\n", info);
	return -6;
    }
    
    ret = freexl_get_info(handle, FREEXL_CFBF_FAT_COUNT, &info);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for fat count: %d\n", ret);
	return -7;
    }
    if (info != 128)
    {
	fprintf(stderr, "Unexpected CFBF_FAT_COUNT: %d\n", info);
	return -8;
    }

    ret = freexl_get_info(handle, FREEXL_BIFF_VERSION, &info);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for BIFF version: %d\n", ret);
	return -9;
    }
    if (info != FREEXL_BIFF_VER_5)
    {
	fprintf(stderr, "Unexpected BIFF version: %d\n", info);
	return -10;
    }
 
    ret = freexl_get_info(handle, FREEXL_BIFF_MAX_RECSIZE, &info);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for BIFF record size: %d\n", ret);
	return -11;
    }
    if (info != FREEXL_BIFF_MAX_RECSZ_2080)
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
	fprintf(stderr, "Unexpected BIFF codepage: 0x%x\n", info);
	return -18;
    }

    ret = freexl_get_info(handle, FREEXL_BIFF_SHEET_COUNT, &info);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for BIFF worksheet count: %d\n", ret);
	return -19;
    }
    if (info != 3) {
	fprintf(stderr, "Unexpected BIFF worksheet count: %d\n", info);
	return -20;
    }

    ret = freexl_get_info(handle, FREEXL_BIFF_FORMAT_COUNT, &info);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for BIFF format count: %d\n", ret);
	return -21;
    }
    if (info != 3) {
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
    
    ret = freexl_get_info(handle, FREEXL_BIFF_STRING_COUNT, &info);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for SST string count: %d\n", ret);
	return -61;
    }
    if (info != 0) {
	fprintf(stderr, "Unexpected SST string count: %d\n", info);
	return -62;
    }

    /* We start with the first worksheet (zero index) */
    ret = freexl_get_worksheet_name(handle, 0, &worksheet_name);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting worksheet name: %d\n", ret);
	return -25;
    }
    if (strcmp(worksheet_name, "Sheet1") != 0) {
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
    if ((num_rows != 2) || (num_columns != 7)) {
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
    if (strcmp(cell_value.value.text_value, "Col 1") != 0) {
	fprintf(stderr, "Unexpected cell (0,0) value: %s\n", cell_value.value.text_value);
	return -34;
    }
    
    ret = freexl_get_cell_value(handle, 1, 0, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (1,0): %d\n", ret);
	return -35;
    }
    if (cell_value.type != FREEXL_CELL_TEXT) {
	fprintf(stderr, "Unexpected cell (1,0) type: %u\n", cell_value.type);
	return -36;
    }
    if (strcmp(cell_value.value.text_value, "A") != 0) {
	fprintf(stderr, "Unexpected cell (1,0) value: %s\n", cell_value.value.text_value);
	return -37;
    }    

    ret = freexl_get_cell_value(handle, 1, 1, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (1,1): %d\n", ret);
	return -38;
    }
    if (cell_value.type != FREEXL_CELL_TEXT) {
	fprintf(stderr, "Unexpected cell (1,1) type: %u\n", cell_value.type);
	return -39;
    }
    if (strcmp(cell_value.value.text_value, "B") != 0) {
	fprintf(stderr, "Unexpected cell (1,1) value: %s\n", cell_value.value.text_value);
	return -40;
    }    

    ret = freexl_get_cell_value(handle, 1, 2, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (1,2): %d\n", ret);
	return -41;
    }
    if (cell_value.type != FREEXL_CELL_INT) {
	fprintf(stderr, "Unexpected cell (1,2) type: %u\n", cell_value.type);
	return -42;
    }
    if (cell_value.value.int_value != 2) {
	fprintf(stderr, "Unexpected cell (1,2) value: %d\n", cell_value.value.int_value);
	return -43;
    } 

    ret = freexl_get_cell_value(handle, 1, 3, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (1,3): %d\n", ret);
	return -44;
    }
    if (cell_value.type != FREEXL_CELL_DOUBLE) {
	fprintf(stderr, "Unexpected cell (1,3) type: %u\n", cell_value.type);
	return -45;
    }
    if (cell_value.value.double_value != 4.2) {
	fprintf(stderr, "Unexpected cell (1,3) value: %g\n", cell_value.value.double_value);
	return -46;
    } 
 
    ret = freexl_get_cell_value(handle, 1, 4, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (1,4): %d\n", ret);
	return -47;
    }
    if (cell_value.type != FREEXL_CELL_TIME) {
	fprintf(stderr, "Unexpected cell (1,4) type: %u\n", cell_value.type);
	return -48;
    }
    if (strcmp(cell_value.value.text_value, "11:00:00") != 0) {
	fprintf(stderr, "Unexpected cell (1,4) value: %s\n", cell_value.value.text_value);
	return -49;
    } 

    ret = freexl_get_cell_value(handle, 1, 5, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (1,5): %d\n", ret);
	return -50;
    }
    if (cell_value.type != FREEXL_CELL_DATETIME) {
	fprintf(stderr, "Unexpected cell (1,5) type: %u\n", cell_value.type);
	return -51;
    }
    if (strcmp(cell_value.value.text_value, "2011-08-21 10:23:45") != 0) {
	fprintf(stderr, "Unexpected cell (1,5) value: %s\n", cell_value.value.text_value);
	return -52;
    } 

    ret = freexl_get_cell_value(handle, 1, 6, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (1,6): %d\n", ret);
	return -53;
    }
    if (cell_value.type != FREEXL_CELL_DATETIME) {
	fprintf(stderr, "Unexpected cell (1,6) type: %u\n", cell_value.type);
	return -54;
    }
    if (strcmp(cell_value.value.text_value, "1967-03-11 00:00:00") != 0) {
	fprintf(stderr, "Unexpected cell (1,6) value: %s\n", cell_value.value.text_value);
	return -55;
    } 

    ret = freexl_get_cell_value(handle, 0, 2, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (0,2): %d\n", ret);
	return -56;
    }
    if (cell_value.type != FREEXL_CELL_NULL) {
	fprintf(stderr, "Unexpected cell (0,2) type: %u\n", cell_value.type);
	return -57;
    }

    /* error cases */
    ret = freexl_get_cell_value(handle, 5, 1, &cell_value);
    if (ret != FREEXL_ILLEGAL_CELL_ROW_COL) {
	fprintf(stderr, "Unexpected result for (5,1): %d\n", ret);
	return -56;
    }
    ret = freexl_get_cell_value(handle, 1, 99, &cell_value);
    if (ret != FREEXL_ILLEGAL_CELL_ROW_COL) {
	fprintf(stderr, "Unexpected result for (1,99): %d\n", ret);
	return -57;
    }
    ret = freexl_get_cell_value(handle, 2, 2, &cell_value);
    if (ret != FREEXL_ILLEGAL_CELL_ROW_COL) {
	fprintf(stderr, "Unexpected result for (2,2): %d\n", ret);
	return -58;
    }
    ret = freexl_get_cell_value(handle, 1, 7, &cell_value);
    if (ret != FREEXL_ILLEGAL_CELL_ROW_COL) {
	fprintf(stderr, "Unexpected result for (1,7): %d\n", ret);
	return -59;
    }

    /* Check the second worksheet */
    ret = freexl_get_worksheet_name(handle, 1, &worksheet_name);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting worksheet name: %d\n", ret);
	return -60;
    }
    if (strcmp(worksheet_name, "Excel95 format sheet 2") != 0) {
	fprintf(stderr, "Unexpected worksheet name: %s\n", worksheet_name);
	return -63;
    }   

    ret = freexl_close (handle);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "CLOSE ERROR: %d\n", ret);
	return -2;
    }

    return 0;
}
