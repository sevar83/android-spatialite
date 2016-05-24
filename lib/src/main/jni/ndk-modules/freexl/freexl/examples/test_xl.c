/* 
/ test_xl.c
/
/ FreeXL Sample code
/
/ Author: Sandro Furieri a.furieri@lqt.it
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

int
main (int argc, char *argv[])
{
    const void *handle;
    int ret;
    unsigned int info;
    unsigned int fat_count;
    unsigned int sst_count;
    unsigned int worksheet_count;
    unsigned int format_count;
    unsigned int xf_count;
    unsigned int idx;
    unsigned int next_sector;
    int biff_v8 = 0;
    const char *utf8_string;
    int verbose = 0;

    if (argc == 2 || argc == 3)
      {
	  if (argc == 3)
	    {
		if (strcmp (argv[2], "-verbose") == 0)
		    verbose = 1;
	    }
      }
    else
      {
	  fprintf (stderr, "usage: text_xl path.xls [-verbose]\n");
	  return -1;
      }

/* opening the .XLS file [Workbook] */
    ret = freexl_open (argv[1], &handle);
    if (ret != FREEXL_OK)
      {
	  fprintf (stderr, "OPEN ERROR: %d\n", ret);
	  return -1;
      }

/* 
 * reporting .XLS information
 */

    printf ("\nExcel document: %s\n", argv[1]);
    printf ("==========================================================\n");

/* CFBF version */
    ret = freexl_get_info (handle, FREEXL_CFBF_VERSION, &info);
    if (ret != FREEXL_OK)
      {
	  fprintf (stderr, "GET-INFO [FREEXL_CFBF_VERSION] Error: %d\n", ret);
	  goto stop;
      }
    switch (info)
      {
      case FREEXL_CFBF_VER_3:
	  printf ("CFBF Version ........: 3\n");
	  break;
      case FREEXL_CFBF_VER_4:
	  printf ("CFBF Version ........: 4\n");
	  break;
      case FREEXL_UNKNOWN:
	  printf ("CFBF Version ........: UNKNOWN\n");
	  break;
      };

/* CFBF sector size */
    ret = freexl_get_info (handle, FREEXL_CFBF_SECTOR_SIZE, &info);
    if (ret != FREEXL_OK)
      {
	  fprintf (stderr, "GET-INFO [FREEXL_CFBF_SECTOR_SIZE] Error: %d\n",
		   ret);
	  goto stop;
      }
    switch (info)
      {
      case FREEXL_CFBF_SECTOR_512:
	  printf ("CFBF Sector size ....: 512\n");
	  break;
      case FREEXL_CFBF_SECTOR_4096:
	  printf ("CFBF Sector size ....: 4096\n");
	  break;
      case FREEXL_UNKNOWN:
	  printf ("CFBF Sector size ....: UNKNOWN\n");
	  break;
      };

/* CFBF FAT entries */
    ret = freexl_get_info (handle, FREEXL_CFBF_FAT_COUNT, &fat_count);
    if (ret != FREEXL_OK)
      {
	  fprintf (stderr, "GET-INFO [FREEXL_CFBF_FAT_COUNT] Error: %d\n", ret);
	  goto stop;
      }
    printf ("CFBF FAT entries ....: %u\n", fat_count);

/* BIFF version */
    ret = freexl_get_info (handle, FREEXL_BIFF_VERSION, &info);
    if (ret != FREEXL_OK)
      {
	  fprintf (stderr, "GET-INFO [FREEXL_BIFF_VERSION] Error: %d\n", ret);
	  goto stop;
      }
    switch (info)
      {
      case FREEXL_BIFF_VER_2:
	  printf ("BIFF Version ........: 2 [Excel 2.0]\n");
	  break;
      case FREEXL_BIFF_VER_3:
	  printf ("BIFF Version ........: 3 [Excel 3.0]\n");
	  break;
      case FREEXL_BIFF_VER_4:
	  printf ("BIFF Version ........: 4 [Excel 4.0]\n");
	  break;
      case FREEXL_BIFF_VER_5:
	  printf ("BIFF Version ........: 5 [Excel 5.0 / Excel 95]\n");
	  break;
      case FREEXL_BIFF_VER_8:
	  printf ("BIFF Version ........: 8 [Excel 98/XP/2003/2007/2010]\n");
	  biff_v8 = 1;
	  break;
      case FREEXL_UNKNOWN:
	  printf ("BIFF Version ........: UNKNOWN\n");
	  break;
      };

/* BIFF max record size */
    ret = freexl_get_info (handle, FREEXL_BIFF_MAX_RECSIZE, &info);
    if (ret != FREEXL_OK)
      {
	  fprintf (stderr, "GET-INFO [FREEXL_BIFF_MAX_RECSIZE] Error: %d\n",
		   ret);
	  goto stop;
      }
    switch (info)
      {
      case FREEXL_BIFF_MAX_RECSZ_2080:
	  printf ("BIFF Max record size : 2080\n");
	  break;
      case FREEXL_BIFF_MAX_RECSZ_8224:
	  printf ("BIFF Max record size : 8224\n");
	  break;
      case FREEXL_UNKNOWN:
	  printf ("BIFF Max record size : UNKNOWN\n");
	  break;
      };

/* BIFF DateMode */
    ret = freexl_get_info (handle, FREEXL_BIFF_DATEMODE, &info);
    if (ret != FREEXL_OK)
      {
	  fprintf (stderr, "GET-INFO [FREEXL_BIFF_DATEMODE] Error: %d\n", ret);
	  goto stop;
      }
    switch (info)
      {
      case FREEXL_BIFF_DATEMODE_1900:
	  printf ("BIFF DateMode .......: 0 [day#1 = '1900-01-01']\n");
	  break;
      case FREEXL_BIFF_DATEMODE_1904:
	  printf ("BIFF DateMode .......: 1 [day#1 = '1904-01-02']\n");
	  break;
      case FREEXL_UNKNOWN:
	  printf ("BIFF DateMode .......: UNKNOWN\n");
	  break;
      };

/* BIFF Obfuscated */
    ret = freexl_get_info (handle, FREEXL_BIFF_PASSWORD, &info);
    if (ret != FREEXL_OK)
      {
	  fprintf (stderr, "GET-INFO [FREEXL_BIFF_PASSWORD] Error: %d\n", ret);
	  goto stop;
      }
    switch (info)
      {
      case FREEXL_BIFF_OBFUSCATED:
	  printf ("BIFF Password/Crypted: YES, obfuscated (not accessible)\n");
	  break;
      case FREEXL_BIFF_PLAIN:
	  printf ("BIFF Password/Crypted: NO, clear data\n");
	  break;
      case FREEXL_UNKNOWN:
	  printf ("BIFF Password/Crypted: UNKNOWN\n");
	  break;
      };

/* BIFF CodePage */
    ret = freexl_get_info (handle, FREEXL_BIFF_CODEPAGE, &info);
    if (ret != FREEXL_OK)
      {
	  fprintf (stderr, "GET-INFO [FREEXL_BIFF_CODEPAGE] Error: %d\n", ret);
	  goto stop;
      }
    switch (info)
      {
      case FREEXL_BIFF_ASCII:
	  printf ("BIFF CodePage .......: ASCII\n");
	  break;
      case FREEXL_BIFF_CP437:
	  printf ("BIFF CodePage .......: CP437 [OEM United States]\n");
	  break;
      case FREEXL_BIFF_CP720:
	  printf ("BIFF CodePage .......: CP720 [Arabic (DOS)]\n");
	  break;
      case FREEXL_BIFF_CP737:
	  printf ("BIFF CodePage .......: CP737 [Greek (DOS)]\n");
	  break;
      case FREEXL_BIFF_CP775:
	  printf ("BIFF CodePage .......: CP775 [Baltic (DOS)]\n");
	  break;
      case FREEXL_BIFF_CP850:
	  printf ("BIFF CodePage .......: CP850 [Western European (DOS)]\n");
	  break;
      case FREEXL_BIFF_CP852:
	  printf ("BIFF CodePage .......: CP852 [Central European (DOS)]\n");
	  break;
      case FREEXL_BIFF_CP855:
	  printf ("BIFF CodePage .......: CP855 [OEM Cyrillic]\n");
	  break;
      case FREEXL_BIFF_CP857:
	  printf ("BIFF CodePage .......: CP857 [Turkish (DOS)]\n");
	  break;
      case FREEXL_BIFF_CP858:
	  printf ("BIFF CodePage .......: CP858 [OEM Multilingual Latin I]\n");
	  break;
      case FREEXL_BIFF_CP860:
	  printf ("BIFF CodePage .......: CP860 [Portuguese (DOS)]\n");
	  break;
      case FREEXL_BIFF_CP861:
	  printf ("BIFF CodePage .......: CP861 [Icelandic (DOS)]\n");
	  break;
      case FREEXL_BIFF_CP862:
	  printf ("BIFF CodePage .......: CP862 [Hebrew (DOS)]\n");
	  break;
      case FREEXL_BIFF_CP863:
	  printf ("BIFF CodePage .......: CP863 [French Canadian (DOS)]\n");
	  break;
      case FREEXL_BIFF_CP864:
	  printf ("BIFF CodePage .......: CP864 [Arabic (864)]\n");
	  break;
      case FREEXL_BIFF_CP865:
	  printf ("BIFF CodePage .......: CP865 [Nordic (DOS)]\n");
	  break;
      case FREEXL_BIFF_CP866:
	  printf ("BIFF CodePage .......: CP866 [Cyrillic (DOS)]\n");
	  break;
      case FREEXL_BIFF_CP869:
	  printf ("BIFF CodePage .......: CP869 [Greek, Modern (DOS)]\n");
	  break;
      case FREEXL_BIFF_CP874:
	  printf ("BIFF CodePage .......: CP874 [Thai (Windows)]\n");
	  break;
      case FREEXL_BIFF_CP932:
	  printf ("BIFF CodePage .......: CP932 [Japanese (Shift-JIS)]\n");
	  break;
      case FREEXL_BIFF_CP936:
	  printf
	      ("BIFF CodePage .......: CP936 [Chinese Simplified (GB2312)]\n");
	  break;
      case FREEXL_BIFF_CP949:
	  printf ("BIFF CodePage .......: CP949 [Korean]\n");
	  break;
      case FREEXL_BIFF_CP950:
	  printf
	      ("BIFF CodePage .......: CP950 [Chinese Traditional (Big5)]\n");
	  break;
      case FREEXL_BIFF_UTF16LE:
	  printf ("BIFF CodePage .......: UTF-16LE [Unicode]\n");
	  break;
      case FREEXL_BIFF_CP1250:
	  printf ("BIFF CodePage .......: CP1250 [Windows Central Europe]\n");
	  break;
      case FREEXL_BIFF_CP1251:
	  printf ("BIFF CodePage .......: CP1251 [Windows Cyrillic]\n");
	  break;
      case FREEXL_BIFF_CP1252:
	  printf ("BIFF CodePage .......: CP1252 [Windows Latin 1]\n");
	  break;
      case FREEXL_BIFF_CP1253:
	  printf ("BIFF CodePage .......: CP1253 [Windows Greek]\n");
	  break;
      case FREEXL_BIFF_CP1254:
	  printf ("BIFF CodePage .......: CP1254 [Windows Turkish]\n");
	  break;
      case FREEXL_BIFF_CP1255:
	  printf ("BIFF CodePage .......: CP1255 [Windows Hebrew]\n");
	  break;
      case FREEXL_BIFF_CP1256:
	  printf ("BIFF CodePage .......: CP1256 [Windows Arabic]\n");
	  break;
      case FREEXL_BIFF_CP1257:
	  printf ("BIFF CodePage .......: CP1257 [Windows Baltic]\n");
	  break;
      case FREEXL_BIFF_CP1258:
	  printf ("BIFF CodePage .......: CP1258 [Windows Vietnamese]\n");
	  break;
      case FREEXL_BIFF_CP1361:
	  printf ("BIFF CodePage .......: CP1361 [Korean (Johab)]\n");
	  break;
      case FREEXL_BIFF_MACROMAN:
	  printf ("BIFF CodePage .......: MacRoman\n");
	  break;
      case FREEXL_UNKNOWN:
	  printf ("BIFF CodePage .......: UNKNOWN\n");
	  break;
      };

/* BIFF Worksheet entries */
    ret = freexl_get_info (handle, FREEXL_BIFF_SHEET_COUNT, &worksheet_count);
    if (ret != FREEXL_OK)
      {
	  fprintf (stderr, "GET-INFO [FREEXL_BIFF_SHEET_COUNT] Error: %d\n",
		   ret);
	  goto stop;
      }
    printf ("BIFF Worksheets .....: %u\n", worksheet_count);

    if (biff_v8)
      {
	  /* BIFF SST entries */
	  ret = freexl_get_info (handle, FREEXL_BIFF_STRING_COUNT, &sst_count);
	  if (ret != FREEXL_OK)
	    {
		fprintf (stderr,
			 "GET-INFO [FREEXL_BIFF_STRING_COUNT] Error: %d\n",
			 ret);
		goto stop;
	    }
	  printf ("BIFF SST entries ....: %u\n", sst_count);
      }

/* BIFF Format entries */
    ret = freexl_get_info (handle, FREEXL_BIFF_FORMAT_COUNT, &format_count);
    if (ret != FREEXL_OK)
      {
	  fprintf (stderr, "GET-INFO [FREEXL_BIFF_FORMAT_COUNT] Error: %d\n",
		   ret);
	  goto stop;
      }
    printf ("BIFF Formats ........: %u\n", format_count);

/* BIFF XF entries */
    ret = freexl_get_info (handle, FREEXL_BIFF_XF_COUNT, &xf_count);
    if (ret != FREEXL_OK)
      {
	  fprintf (stderr, "GET-INFO [FREEXL_BIFF_XF_COUNT] Error: %d\n", ret);
	  goto stop;
      }
    printf ("BIFF eXtendedFormats : %u\n", xf_count);

    printf
	("\nWorksheets:\n=========================================================\n");
    for (idx = 0; idx < worksheet_count; idx++)
      {
	  /* printing BIFF Worksheets entries */
	  unsigned short active;
	  unsigned int rows;
	  unsigned short columns;

	  ret = freexl_get_worksheet_name (handle, idx, &utf8_string);
	  if (ret != FREEXL_OK)
	    {
		fprintf (stderr, "GET-WORKSHEET-NAME Error: %d\n", ret);
		goto stop;
	    }
	  if (utf8_string == NULL)
	      printf ("%3u] NULL (unnamed)\n", idx);
	  else
	      printf ("%3u] %s\n", idx, utf8_string);
	  ret = freexl_select_active_worksheet (handle, idx);
	  if (ret != FREEXL_OK)
	    {
		fprintf (stderr, "SELECT-ACTIVE_WORKSHEET Error: %d\n", ret);
		goto stop;
	    }
	  ret = freexl_get_active_worksheet (handle, &active);
	  if (ret != FREEXL_OK)
	    {
		fprintf (stderr, "GET-ACTIVE_WORKSHEET Error: %d\n", ret);
		goto stop;
	    }
	  printf
	      ("\tok, Worksheet successfully selected: currently active: %u\n",
	       active);
	  ret = freexl_worksheet_dimensions (handle, &rows, &columns);
	  if (ret != FREEXL_OK)
	    {
		fprintf (stderr, "WORKSHEET-DIMENSIONS Error: %d\n", ret);
		goto stop;
	    }
	  printf ("\t%u Rows X %u Columns\n\n", rows, columns);
      }

    if (!verbose)
	goto stop;

    if (biff_v8)
      {
	  /* printing BIFF SST entries */
	  printf
	      ("\nSST [Shared String Table]:\n=========================================================\n");
	  for (idx = 0; idx < sst_count; idx++)
	    {
		ret = freexl_get_SST_string (handle, idx, &utf8_string);
		if (ret != FREEXL_OK)
		  {
		      fprintf (stderr, "GET-SST-STRING Error: %d\n", ret);
		      goto stop;
		  }
		if (utf8_string == NULL)
		    printf ("%8u] NULL (empty string)\n", idx);
		else
		    printf ("%8u] %s\n", idx, utf8_string);
	    }
      }

    printf
	("\nFAT entries [File Allocation Table]:\n=========================================================\n");
    for (idx = 0; idx < fat_count; idx++)
      {
	  /* printing each FAT entry */
	  ret = freexl_get_FAT_entry (handle, idx, &next_sector);
	  if (ret != FREEXL_OK)
	    {
		fprintf (stderr, "GET-FAT-ENTRY Error: %d\n", ret);
		goto stop;
	    }
	  if (next_sector == 0xffffffff)
	      printf ("%8u -> 0xffffffff FREESECT\n", idx);
	  else if (next_sector == 0xfffffffe)
	      printf ("%8u -> 0xfffffffe ENDOFCHAIN\n", idx);
	  else if (next_sector == 0xfffffffd)
	      printf ("%8u -> 0xfffffffe FATSECT\n", idx);
	  else if (next_sector == 0xfffffffc)
	      printf ("%8u -> 0xfffffffe DIFSECT\n", idx);
	  else
	      printf ("%8u -> %8u\n", idx, next_sector);
      }

  stop:
/* closing the .XLS file [Workbook] */
    ret = freexl_close (handle);
    if (ret != FREEXL_OK)
      {
	  fprintf (stderr, "CLOSE ERROR: %d\n", ret);
	  return -1;
      }

    return 0;
}
