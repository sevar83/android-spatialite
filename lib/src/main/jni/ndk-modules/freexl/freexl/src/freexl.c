/* 
/ freexl.c
/
/ FreeXL implementation
/
/ version  1.0, 2011 July 26
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
#include <math.h>

#if defined(__MINGW32__) || defined(_WIN32)
#define LIBICONV_STATIC
#include <iconv.h>
#define LIBCHARSET_STATIC
#ifdef _MSC_VER
/* <localcharset.h> isn't supported on OSGeo4W */
/* applying a tricky workaround to fix this issue */
extern const char *locale_charset (void);
#else /* sane Windows - not OSGeo4W */
#include <localcharset.h>
#endif /* end localcharset */
#else /* not WINDOWS */
#if defined(__APPLE__) || defined(__ANDROID__)
#include <iconv.h>
#include <localcharset.h>
#else /* neither Mac OsX nor Android */
#include <iconv.h>
#include <langinfo.h>
#endif
#endif

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#include "freexl.h"
#include "freexl_internals.h"

#if defined(_WIN32) && !defined(__MINGW32__)
/* MSVC compiler doesn't support lround() at all */
static double
round (double num)
{
    double integer = ceil (num);
    if (num > 0)
	return integer - num > 0.5 ? integer - 1.0 : integer;
    return integer - num >= 0.5 ? integer - 1.0 : integer;
}

static long
lround (double num)
{
    long integer = (long) round (num);
    return integer;
}
#endif

static void
swap16 (biff_word16 * word)
{
/* Endianness: swapping a 16 bit word */
    unsigned char save = word->bytes[0];
    word->bytes[0] = word->bytes[1];
    word->bytes[1] = save;
}

static void
swap32 (biff_word32 * word)
{
/* Endianness: swapping a 32 bit word */
    unsigned char save0 = word->bytes[0];
    unsigned char save1 = word->bytes[1];
    unsigned char save2 = word->bytes[2];
    word->bytes[0] = word->bytes[3];
    word->bytes[1] = save2;
    word->bytes[2] = save1;
    word->bytes[3] = save0;
}

static void
swap_float (biff_float * word)
{
/* Endianness: swapping a 64 bit float */
    unsigned char save0 = word->bytes[0];
    unsigned char save1 = word->bytes[1];
    unsigned char save2 = word->bytes[2];
    unsigned char save3 = word->bytes[3];
    unsigned char save4 = word->bytes[4];
    unsigned char save5 = word->bytes[5];
    unsigned char save6 = word->bytes[6];
    word->bytes[0] = word->bytes[7];
    word->bytes[1] = save6;
    word->bytes[2] = save5;
    word->bytes[3] = save4;
    word->bytes[4] = save3;
    word->bytes[5] = save2;
    word->bytes[6] = save1;
    word->bytes[7] = save0;
}

static int
biff_set_utf8_converter (biff_workbook * workbook)
{
/* attempting to set up a charset converter to UTF-8 */
    iconv_t cvt = (iconv_t) (-1);
    if (workbook->utf8_converter)
	iconv_close (workbook->utf8_converter);
    workbook->utf8_converter = NULL;
    switch (workbook->biff_code_page)
      {
      case 0x016F:
	  cvt = iconv_open ("UTF-8", "ASCII");
	  break;
      case 0x01B5:
	  cvt = iconv_open ("UTF-8", "CP437");
	  break;
      case 0x02D0:
	  cvt = iconv_open ("UTF-8", "CP720");
	  break;
      case 0x02E1:
	  cvt = iconv_open ("UTF-8", "CP737");
	  break;
      case 0x0307:
	  cvt = iconv_open ("UTF-8", "CP775");
	  break;
      case 0x0352:
	  cvt = iconv_open ("UTF-8", "CP850");
	  break;
      case 0x0354:
	  cvt = iconv_open ("UTF-8", "CP852");
	  break;
      case 0x0357:
	  cvt = iconv_open ("UTF-8", "CP855");
	  break;
      case 0x0359:
	  cvt = iconv_open ("UTF-8", "CP857");
	  break;
      case 0x035A:
	  cvt = iconv_open ("UTF-8", "CP858");
	  break;
      case 0x035C:
	  cvt = iconv_open ("UTF-8", "CP860");
	  break;
      case 0x035D:
	  cvt = iconv_open ("UTF-8", "CP861");
	  break;
      case 0x035E:
	  cvt = iconv_open ("UTF-8", "CP862");
	  break;
      case 0x035F:
	  cvt = iconv_open ("UTF-8", "CP863");
	  break;
      case 0x0360:
	  cvt = iconv_open ("UTF-8", "CP864");
	  break;
      case 0x0361:
	  cvt = iconv_open ("UTF-8", "CP865");
	  break;
      case 0x0362:
	  cvt = iconv_open ("UTF-8", "CP866");
	  break;
      case 0x0365:
	  cvt = iconv_open ("UTF-8", "CP869");
	  break;
      case 0x036A:
	  cvt = iconv_open ("UTF-8", "CP874");
	  break;
      case 0x03A4:
	  cvt = iconv_open ("UTF-8", "CP932");
	  break;
      case 0x03A8:
	  cvt = iconv_open ("UTF-8", "CP936");
	  break;
      case 0x03B5:
	  cvt = iconv_open ("UTF-8", "CP949");
	  break;
      case 0x03B6:
	  cvt = iconv_open ("UTF-8", "CP950");
	  break;
      case 0x04B0:
	  cvt = iconv_open ("UTF-8", "UTF-16LE");
	  break;
      case 0x04E2:
	  cvt = iconv_open ("UTF-8", "CP1250");
	  break;
      case 0x04E3:
	  cvt = iconv_open ("UTF-8", "CP1251");
	  break;
      case 0x04E4:
      case 0x8001:
	  cvt = iconv_open ("UTF-8", "CP1252");
	  break;
      case 0x04E5:
	  cvt = iconv_open ("UTF-8", "CP1253");
	  break;
      case 0x04E6:
	  cvt = iconv_open ("UTF-8", "CP1254");
	  break;
      case 0x04E7:
	  cvt = iconv_open ("UTF-8", "CP1255");
	  break;
      case 0x04E8:
	  cvt = iconv_open ("UTF-8", "CP1256");
	  break;
      case 0x04E9:
	  cvt = iconv_open ("UTF-8", "CP1257");
	  break;
      case 0x04EA:
	  cvt = iconv_open ("UTF-8", "CP1258");
	  break;
      case 0x0551:
	  cvt = iconv_open ("UTF-8", "CP1361");
	  break;
      case 0x2710:
      case 0x8000:
	  cvt = iconv_open ("UTF-8", "MacRoman");
	  break;
      };
    if (cvt == (iconv_t) (-1))
	return 0;
    workbook->utf8_converter = cvt;
    return 1;
}

static char *
convert_to_utf8 (iconv_t converter, const char *buf, int buflen, int *err)
{
/* converting a string to UTF8 */
    char *utf8buf = 0;
#if !defined(__MINGW32__) && defined(_WIN32)
    const char *pBuf;
#else
    char *pBuf;
#endif
    size_t len;
    size_t utf8len;
    int maxlen = buflen * 4;
    char *pUtf8buf;
    *err = FREEXL_OK;
    if (!converter)
      {
	  *err = FREEXL_UNSUPPORTED_CHARSET;
	  return NULL;
      }
    utf8buf = malloc (maxlen);
    len = buflen;
    utf8len = maxlen;
    pBuf = (char *) buf;
    pUtf8buf = utf8buf;
    if (iconv (converter, &pBuf, &len, &pUtf8buf, &utf8len) == (size_t) (-1))
      {
	  free (utf8buf);
	  *err = FREEXL_INVALID_CHARACTER;
	  return NULL;
      }
    utf8buf[maxlen - utf8len] = '\0';
    return utf8buf;
}

static void
get_unicode_params (unsigned char *addr, int swap, unsigned int *start_offset,
		    int *real_utf16, unsigned int *extra_skip)
{
/* retrieving Unicode string params */
    biff_word16 word16;
    biff_word32 word32;
    int skip_1 = 0;
    int skip_2 = 0;
    unsigned char *p_string = addr;
    unsigned char mask = *p_string;

/*
 * a bitwise mask
 * 0x01 - the string is 'real' UTF16LE
 *        otherwise any high-order ZEROes are suppressed
 *        ['stripped' UTF16]
 *
 * 0x04 - an extra 32-bit field is present 
 * 0x08 - another extra 16-bit field is present
 *        (such extra (optional) fields are intended by MS  
 *        for text formatting purposes: we'll ignore them
 *        at all, simply adjusting any offset as required)
 */
    p_string++;
    if ((mask & 0x01) == 0x01)
	*real_utf16 = 1;
    else
	*real_utf16 = 0;
    if ((mask & 0x04) == 0x04)
      {
	  /* optional field: 32-bits */
	  memcpy (word32.bytes, p_string, 2);
	  if (swap)
	      swap32 (&word32);
	  skip_1 = word32.value;
	  p_string += 4;
      }
    if ((mask & 0x08) == 0x08)
      {
	  /* optional field 16-bits */
	  memcpy (word16.bytes, p_string, 2);
	  if (swap)
	      swap16 (&word16);
	  skip_2 = word16.value;
	  p_string += 2;
      }
    *start_offset = p_string - addr;
    *extra_skip = skip_1 + (skip_2 * 4);
}

static int
parse_unicode_string (iconv_t converter, unsigned short characters,
		      int real_utf16, unsigned char *unicode_string,
		      char **utf8_string)
{
/* attemping to convert an Unicode string into UTF-8 */
    unsigned int len = characters * 2;
    char *string;
    int err;
    unsigned char *p_string = unicode_string;

    string = malloc (len);
    if (!real_utf16)
      {
	  /* 'stripped' UTF-16: requires padding */
	  unsigned int i;
	  for (i = 0; i < characters; i++)
	    {
		*(string + (i * 2)) = *p_string;
		p_string++;
		*(string + ((i * 2) + 1)) = 0x00;
	    }
      }
    else
      {
	  /* already encoded as UTF-16 */
	  memcpy (string, p_string, len);
      }
/* converting text to UTF-8 */
    *utf8_string = convert_to_utf8 (converter, string, len, &err);
    free (string);
    if (err)
	return 0;
    return 1;
}

static int
decode_rk_integer (unsigned char *bytes, int *value, int swap)
{
/* attempting to decode an RK value as an INT-32 */
    biff_word32 word32;
    unsigned char mask = bytes[0];
    if ((mask & 0x02) == 0x02 && (mask & 0x01) == 0x00)
      {
	  /* ok, this RK value is an INT-32 */
	  memcpy (word32.bytes, bytes, 4);
	  if (swap)
	      swap32 (&word32);
	  *value = word32.signed_value >> 2;	/* right shift: 2 bits */
	  return 1;
      }
    return 0;
}

static int
decode_rk_float (unsigned char *bytes, double *value, int swap)
{
/* attempting to decode an RK value as a DOUBLE-FLOAT */
    biff_word32 word32;
    biff_float word_float;
    int int_value;
    double dbl_value;
    int div_by_100 = 0;
    unsigned char mask = bytes[0];
    if ((mask & 0x02) == 0x02 && (mask & 0x01) == 0x01)
      {
	  /* ok, this RK value is an INT-32 (divided by 100) */
	  memcpy (word32.bytes, bytes, 4);
	  if (swap)
	      swap32 (&word32);
	  int_value = word32.signed_value >> 2;	/* right shift: 2 bits */
	  *value = (double) int_value / 100.0;
	  return 1;
      }
    if ((mask & 0x02) == 0x00)
      {
	  /* ok, this RK value is a FLOAT */
	  if ((mask & 0x01) == 0x01)
	      div_by_100 = 1;
	  memcpy (word32.bytes, bytes, 4);
	  if (swap)
	      swap32 (&word32);
	  int_value = word32.value;
	  int_value &= 0xfffffffc;
	  word32.value = int_value;
	  memset (word_float.bytes, '\0', 8);
	  if (swap)
	      memcpy (word_float.bytes, word32.bytes, 4);
	  else
	      memcpy (word_float.bytes + 4, word32.bytes, 4);
	  dbl_value = word_float.value;
	  if (div_by_100)
	      dbl_value /= 100.0;
	  *value = dbl_value;
	  return 1;
      }
    return 0;
}

static int
check_xf_datetime (biff_workbook * workbook, unsigned short xf_index,
		   int *is_date, int *is_datetime, int *is_time)
{
/* testing for DATE/DATETIME/TIME formats */
    unsigned short idx;
    unsigned short format_index;
    if (xf_index >= workbook->biff_xf_next_index)
	return 0;
    format_index = workbook->biff_xf_array[xf_index];
    for (idx = 0; idx < workbook->max_format_index; idx++)
      {
	  biff_format *format = workbook->format_array + idx;
	  if (format->format_index == format_index)
	    {
		*is_date = format->is_date;
		*is_datetime = format->is_datetime;
		*is_time = format->is_time;
		return 1;
	    }
      }
    *is_date = 0;
    *is_datetime = 0;
    *is_time = 0;
    return 1;
}

static int
check_xf_datetime_58 (biff_workbook * workbook, unsigned short xf_index,
		      int *is_date, int *is_datetime, int *is_time)
{
/* 
/ testing for DATE/DATETIME/TIME formats 
/ BIFF5 and BIFF8 versions
*/
    unsigned short format_index;
    if (xf_index >= workbook->biff_xf_next_index)
	return 0;
    format_index = workbook->biff_xf_array[xf_index];
    switch (format_index)
      {
      case 14:
      case 15:
      case 16:
      case 17:
	  /* BIFF5/BIFF8 built-in DATE formats */
	  *is_date = 1;
	  *is_datetime = 0;
	  *is_time = 0;
	  return 1;
      case 18:
      case 19:
      case 20:
      case 21:
      case 45:
      case 46:
      case 47:
	  /* BIFF5/BIFF8 built-in TIME formats */
	  *is_date = 0;
	  *is_datetime = 0;
	  *is_time = 1;
	  return 1;
      case 22:
	  /* BIFF5/BIFF8 built-in DATETIME formats */
	  *is_date = 0;
	  *is_datetime = 1;
	  *is_time = 0;
	  return 1;
      default:
	  break;
      };
    return check_xf_datetime (workbook, xf_index, is_date, is_datetime,
			      is_time);
}

static void
compute_time (int *hh, int *mm, int *ss, double percent)
{
/* computing an Excel time */
    int hours;
    int mins;
    int secs;
    double day_seconds = 24 * 60 * 60;
    day_seconds *= percent;
    secs = lround (day_seconds);
    hours = secs / 3600;
    secs -= hours * 3600;
    mins = secs / 60;
    secs -= mins * 60;
    *hh = hours;
    *mm = mins;
    *ss = secs;
}

static void
compute_date (int *year, int *month, int *day, int count)
{
/* coumputing an Excel date */
    int i;
    int yy = *year;
    int mm = *month;
    int dd = *day;
    for (i = 1; i < count; i++)
      {
	  int last_day_of_month;
	  switch (mm)
	    {
	    case 2:
		if ((yy % 4) == 0)
		  {
		      /* February, leap year */
		      last_day_of_month = 29;
		  }
		else
		    last_day_of_month = 28;
		break;
	    case 4:
	    case 6:
	    case 9:
	    case 11:
		last_day_of_month = 30;
		break;
	    default:
		last_day_of_month = 31;
		break;
	    };
	  if (dd == last_day_of_month)
	    {
		if (mm == 12)
		  {
		      mm = 1;
		      yy += 1;
		  }
		else
		    mm += 1;
		dd = 1;
	    }
	  else
	      dd += 1;
      }
    *year = yy;
    *month = mm;
    *day = dd;
}

static int
set_date_int_value (biff_workbook * workbook, unsigned int row,
		    unsigned short col, unsigned short mode, int num)
{
/* setting a DATE value to some cell */
    biff_cell_value *p_cell;
    char *string;
    char buf[64];
    unsigned int len;
    int yy;
    int mm;
    int dd;
    int count = num;

    if (workbook->active_sheet == NULL)
	return FREEXL_ILLEGAL_CELL_ROW_COL;
    if (workbook->active_sheet->cell_values == NULL)
	return FREEXL_ILLEGAL_CELL_ROW_COL;
    if (row >= workbook->active_sheet->rows
	|| col >= workbook->active_sheet->columns)
	return FREEXL_ILLEGAL_CELL_ROW_COL;

    if (mode)
      {
	  yy = 1904;
	  mm = 1;
	  dd = 2;
      }
    else
      {
	  yy = 1900;
	  mm = 1;
	  dd = 1;
      }
    compute_date (&yy, &mm, &dd, count);
    sprintf (buf, "%04d-%02d-%02d", yy, mm, dd);
    len = strlen (buf);
    string = malloc (len + 1);
    if (!string)
	return FREEXL_INSUFFICIENT_MEMORY;
    strcpy (string, buf);

    p_cell =
	workbook->active_sheet->cell_values +
	(row * workbook->active_sheet->columns) + col;
    p_cell->type = FREEXL_CELL_DATE;
    p_cell->value.text_value = string;
    return FREEXL_OK;
}

static int
set_datetime_int_value (biff_workbook * workbook, unsigned int row,
			unsigned short col, unsigned short mode, int num)
{
/* setting a DATETIME value to some cell */
    biff_cell_value *p_cell;
    char *string;
    char buf[64];
    unsigned int len;
    int yy;
    int mm;
    int dd;
    int count = num;

    if (workbook->active_sheet == NULL)
	return FREEXL_ILLEGAL_CELL_ROW_COL;
    if (workbook->active_sheet->cell_values == NULL)
	return FREEXL_ILLEGAL_CELL_ROW_COL;
    if (row >= workbook->active_sheet->rows
	|| col >= workbook->active_sheet->columns)
	return FREEXL_ILLEGAL_CELL_ROW_COL;

    if (mode)
      {
	  yy = 1904;
	  mm = 1;
	  dd = 2;
      }
    else
      {
	  yy = 1900;
	  mm = 1;
	  dd = 1;
      }
    compute_date (&yy, &mm, &dd, count);
    sprintf (buf, "%04d-%02d-%02d 00:00:00", yy, mm, dd);
    len = strlen (buf);
    string = malloc (len + 1);
    if (!string)
	return FREEXL_INSUFFICIENT_MEMORY;
    strcpy (string, buf);

    p_cell =
	workbook->active_sheet->cell_values +
	(row * workbook->active_sheet->columns) + col;
    p_cell->type = FREEXL_CELL_DATETIME;
    p_cell->value.text_value = string;
    return FREEXL_OK;
}

static int
set_date_double_value (biff_workbook * workbook, unsigned int row,
		       unsigned short col, unsigned short mode, double num)
{
/* setting a DATE value to some cell */
    biff_cell_value *p_cell;
    char *string;
    char buf[64];
    unsigned int len;
    int yy;
    int mm;
    int dd;
    int count = (int) floor (num);

    if (workbook->active_sheet == NULL)
	return FREEXL_ILLEGAL_CELL_ROW_COL;
    if (workbook->active_sheet->cell_values == NULL)
	return FREEXL_ILLEGAL_CELL_ROW_COL;
    if (row >= workbook->active_sheet->rows
	|| col >= workbook->active_sheet->columns)
	return FREEXL_ILLEGAL_CELL_ROW_COL;

    if (mode)
      {
	  yy = 1904;
	  mm = 1;
	  dd = 2;
      }
    else
      {
	  yy = 1900;
	  mm = 1;
	  dd = 1;
      }
    compute_date (&yy, &mm, &dd, count);
    sprintf (buf, "%04d-%02d-%02d", yy, mm, dd);
    len = strlen (buf);
    string = malloc (len + 1);
    if (!string)
	return FREEXL_INSUFFICIENT_MEMORY;
    strcpy (string, buf);

    p_cell =
	workbook->active_sheet->cell_values +
	(row * workbook->active_sheet->columns) + col;
    p_cell->type = FREEXL_CELL_DATE;
    p_cell->value.text_value = string;
    return FREEXL_OK;
}

static int
set_datetime_double_value (biff_workbook * workbook, unsigned int row,
			   unsigned short col, unsigned short mode, double num)
{
/* setting a DATETIME value to some cell */
    biff_cell_value *p_cell;
    char *string;
    char buf[64];
    unsigned int len;
    int yy;
    int mm;
    int dd;
    int h;
    int m;
    int s;
    int count = (int) floor (num);
    double percent = num - (double) count;

    if (workbook->active_sheet == NULL)
	return FREEXL_ILLEGAL_CELL_ROW_COL;
    if (workbook->active_sheet->cell_values == NULL)
	return FREEXL_ILLEGAL_CELL_ROW_COL;
    if (row >= workbook->active_sheet->rows
	|| col >= workbook->active_sheet->columns)
	return FREEXL_ILLEGAL_CELL_ROW_COL;

    if (mode)
      {
	  yy = 1904;
	  mm = 1;
	  dd = 2;
      }
    else
      {
	  yy = 1900;
	  mm = 1;
	  dd = 1;
      }
    compute_date (&yy, &mm, &dd, count);
    compute_time (&h, &m, &s, percent);
    sprintf (buf, "%04d-%02d-%02d %02d:%02d:%02d", yy, mm, dd, h, m, s);
    len = strlen (buf);
    string = malloc (len + 1);
    if (!string)
	return FREEXL_INSUFFICIENT_MEMORY;
    strcpy (string, buf);

    p_cell =
	workbook->active_sheet->cell_values +
	(row * workbook->active_sheet->columns) + col;
    p_cell->type = FREEXL_CELL_DATETIME;
    p_cell->value.text_value = string;
    return FREEXL_OK;
}

static int
set_time_double_value (biff_workbook * workbook, unsigned int row,
		       unsigned short col, double num)
{
/* setting a TIME value to some cell */
    biff_cell_value *p_cell;
    char *string;
    char buf[64];
    unsigned int len;
    int h;
    int m;
    int s;
    int count = (int) floor (num);
    double percent = num - (double) count;

    if (workbook->active_sheet == NULL)
	return FREEXL_ILLEGAL_CELL_ROW_COL;
    if (workbook->active_sheet->cell_values == NULL)
	return FREEXL_ILLEGAL_CELL_ROW_COL;
    if (row >= workbook->active_sheet->rows
	|| col >= workbook->active_sheet->columns)
	return FREEXL_ILLEGAL_CELL_ROW_COL;

    compute_time (&h, &m, &s, percent);
    sprintf (buf, "%02d:%02d:%02d", h, m, s);
    len = strlen (buf);
    string = malloc (len + 1);
    if (!string)
	return FREEXL_INSUFFICIENT_MEMORY;
    strcpy (string, buf);

    p_cell =
	workbook->active_sheet->cell_values +
	(row * workbook->active_sheet->columns) + col;
    p_cell->type = FREEXL_CELL_TIME;
    p_cell->value.text_value = string;
    return FREEXL_OK;
}

static int
set_int_value (biff_workbook * workbook, unsigned int row, unsigned short col,
	       int num)
{
/* setting an INTEGER value to some cell */
    biff_cell_value *p_cell;

    if (workbook->active_sheet == NULL)
	return FREEXL_ILLEGAL_CELL_ROW_COL;
    if (workbook->active_sheet->cell_values == NULL)
	return FREEXL_ILLEGAL_CELL_ROW_COL;
    if (row >= workbook->active_sheet->rows
	|| col >= workbook->active_sheet->columns)
	return FREEXL_ILLEGAL_CELL_ROW_COL;

    p_cell =
	workbook->active_sheet->cell_values +
	(row * workbook->active_sheet->columns) + col;
    p_cell->type = FREEXL_CELL_INT;
    p_cell->value.int_value = num;
    return FREEXL_OK;
}

static int
set_double_value (biff_workbook * workbook, unsigned int row,
		  unsigned short col, double num)
{
/* setting a DOUBLE value to some cell */
    biff_cell_value *p_cell;

    if (workbook->active_sheet == NULL)
	return FREEXL_ILLEGAL_CELL_ROW_COL;
    if (workbook->active_sheet->cell_values == NULL)
	return FREEXL_ILLEGAL_CELL_ROW_COL;
    if (row >= workbook->active_sheet->rows
	|| col >= workbook->active_sheet->columns)
	return FREEXL_ILLEGAL_CELL_ROW_COL;

    p_cell =
	workbook->active_sheet->cell_values +
	(row * workbook->active_sheet->columns) + col;
    p_cell->type = FREEXL_CELL_DOUBLE;
    p_cell->value.dbl_value = num;
    return FREEXL_OK;
}

static int
set_text_value (biff_workbook * workbook, unsigned int row, unsigned short col,
		char *text)
{
/* setting a TEXT value to some cell */
    biff_cell_value *p_cell;

    if (workbook->active_sheet == NULL)
	return FREEXL_ILLEGAL_CELL_ROW_COL;
    if (workbook->active_sheet->cell_values == NULL)
	return FREEXL_ILLEGAL_CELL_ROW_COL;
    if (row >= workbook->active_sheet->rows
	|| col >= workbook->active_sheet->columns)
	return FREEXL_ILLEGAL_CELL_ROW_COL;

    p_cell =
	workbook->active_sheet->cell_values +
	(row * workbook->active_sheet->columns) + col;
    if (!text)
      {
	  p_cell->type = FREEXL_CELL_NULL;
	  return FREEXL_OK;
      }
    p_cell->type = FREEXL_CELL_TEXT;
    p_cell->value.text_value = text;
    return FREEXL_OK;
}

static int
set_sst_value (biff_workbook * workbook, unsigned int row, unsigned short col,
	       const char *text)
{
/* setting an SST-TEXT value to some cell */
    biff_cell_value *p_cell;

    if (workbook->active_sheet == NULL)
	return FREEXL_ILLEGAL_CELL_ROW_COL;
    if (workbook->active_sheet->cell_values == NULL)
	return FREEXL_ILLEGAL_CELL_ROW_COL;
    if (row >= workbook->active_sheet->rows
	|| col >= workbook->active_sheet->columns)
	return FREEXL_ILLEGAL_CELL_ROW_COL;

    p_cell =
	workbook->active_sheet->cell_values +
	(row * workbook->active_sheet->columns) + col;
    if (!text)
      {
	  p_cell->type = FREEXL_CELL_NULL;
	  return FREEXL_OK;
      }
    p_cell->type = FREEXL_CELL_SST_TEXT;
    p_cell->value.sst_value = text;
    return FREEXL_OK;
}

static fat_chain *
alloc_fat_chain (int swap, unsigned short sector_shift,
		 unsigned int directory_start)
{
/* allocating the FAT chain */
    fat_chain *chain = malloc (sizeof (fat_chain));
    if (!chain)
	return NULL;
    chain->swap = swap;
    if (sector_shift == 12)
	chain->sector_size = 4096;
    else
	chain->sector_size = 512;
    chain->next_sector = 0;
    chain->directory_start = directory_start;
    chain->first = NULL;
    chain->last = NULL;
    chain->fat_array = NULL;
    chain->fat_array_count = 0;
    chain->miniCutOff = 0;
    chain->next_sectorMini = 0;
    chain->firstMini = NULL;
    chain->lastMini = NULL;
    chain->miniFAT_array = NULL;
    chain->miniFAT_array_count = 0;
    chain->miniFAT_len = 0;
    chain->miniStream = NULL;
    return chain;
}

static void
destroy_fat_chain (fat_chain * chain)
{
/* destroying a FAT chain */
    fat_entry *entry;
    fat_entry *entry_n;
    if (!chain)
	return;
/* destroying the main FAT */
    entry = chain->first;
    while (entry)
      {
	  entry_n = entry->next;
	  free (entry);
	  entry = entry_n;
      }
    if (chain->fat_array)
	free (chain->fat_array);
/* destroying the miniFAT */
    entry = chain->firstMini;
    while (entry)
      {
	  entry_n = entry->next;
	  free (entry);
	  entry = entry_n;
      }
    if (chain->miniFAT_array)
	free (chain->miniFAT_array);
    if (chain->miniStream)
	free (chain->miniStream);
    free (chain);
}

static unsigned int
get_worksheet_count (biff_workbook * workbook)
{
/* counting how many Worksheet are into the Workbook */
    unsigned int count = 0;
    biff_sheet *p_sheet = workbook->first_sheet;
    while (p_sheet)
      {
	  count++;
	  p_sheet = p_sheet->next;
      }
    return count;
}

static void
destroy_cell (biff_cell_value * cell)
{
/* destroying a cell */
    if (cell->type == FREEXL_CELL_TEXT || cell->type == FREEXL_CELL_DATE
	|| cell->type == FREEXL_CELL_DATETIME || cell->type == FREEXL_CELL_TIME)
      {
	  if (cell->value.text_value)
	      free (cell->value.text_value);
      }
}

static void
destroy_sheet (biff_sheet * sheet)
{
/* destroying a Sheet struct */
    unsigned int row;
    unsigned int col;
    biff_cell_value *p_cell;

    if (!sheet)
	return;
    if (sheet->utf8_name)
	free (sheet->utf8_name);
    if (sheet->cell_values)
      {
	  for (row = 0; row < sheet->rows; row++)
	    {
		/* destroying rows */
		p_cell = sheet->cell_values + (row * sheet->columns);
		for (col = 0; col < sheet->columns; col++)
		  {
		      /* destroying cells */
		      destroy_cell (p_cell);
		      p_cell++;
		  }
	    }
      }
    free (sheet->cell_values);
    free (sheet);
}

static int
allocate_cells (biff_workbook * workbook)
{
/* allocating the rows and cells for the active Worksheet */
    unsigned int row;
    unsigned int col;
    biff_cell_value *p_cell;

/* allocating the cell values array */
    workbook->active_sheet->cell_values =
	malloc (sizeof (biff_cell_value) *
		(workbook->active_sheet->rows *
		 workbook->active_sheet->columns));
    if (workbook->active_sheet->cell_values == NULL)
	return FREEXL_INSUFFICIENT_MEMORY;
    for (row = 0; row < workbook->active_sheet->rows; row++)
      {
	  /* initializing NULL cell values */
	  p_cell =
	      workbook->active_sheet->cell_values +
	      (row * workbook->active_sheet->columns);
	  for (col = 0; col < workbook->active_sheet->columns; col++)
	    {
		p_cell->type = FREEXL_CELL_NULL;
		p_cell++;
	    }
      }

    return FREEXL_OK;
}

static int
add_sheet_to_workbook (biff_workbook * workbook, unsigned int offset,
		       unsigned char visible, unsigned char type, char *name)
{
/* appending a further Sheet to the Workbook */
    biff_sheet *sheet = malloc (sizeof (biff_sheet));
    if (!sheet)
	return 0;
    sheet->start_offset = offset;
    sheet->visible = visible;
    sheet->type = type;
    sheet->utf8_name = name;
    sheet->rows = 0;
    sheet->columns = 0;
    sheet->cell_values = NULL;
    sheet->valid_dimension = 0;
    sheet->already_done = 0;
    sheet->next = NULL;

/* updating the linked list */
    if (workbook->first_sheet == NULL)
	workbook->first_sheet = sheet;
    if (workbook->last_sheet != NULL)
	workbook->last_sheet->next = sheet;
    workbook->last_sheet = sheet;
    return 1;
}

static void
destroy_workbook (biff_workbook * workbook)
{
/* destroyng the Workbook struct */
    biff_sheet *p_sheet;
    biff_sheet *p_sheet_n;
    if (workbook)
      {
	  if (workbook->xls)
	      fclose (workbook->xls);
	  if (workbook->utf8_converter)
	      iconv_close (workbook->utf8_converter);
	  if (workbook->utf16_converter)
	      iconv_close (workbook->utf16_converter);
	  if (workbook->shared_strings.utf8_strings != NULL)
	    {
		/* destroying the Shared Strings Table [SST] */
		unsigned int i;
		for (i = 0; i < workbook->shared_strings.string_count; i++)
		  {
		      char *string =
			  *(workbook->shared_strings.utf8_strings + i);
		      if (string != NULL)
			  free (string);
		  }
		free (workbook->shared_strings.utf8_strings);
	    }
	  if (workbook->shared_strings.current_utf16_buf)
	      free (workbook->shared_strings.current_utf16_buf);
	  p_sheet = workbook->first_sheet;
	  while (p_sheet)
	    {
		p_sheet_n = p_sheet->next;
		destroy_sheet (p_sheet);
		p_sheet = p_sheet_n;
	    }
	  if (workbook->fat)
	      destroy_fat_chain (workbook->fat);
	  free (workbook);
      }
}

static biff_workbook *
alloc_workbook (int magic)
{
/* allocating and initializing the Workbook struct */
    biff_workbook *workbook = malloc (sizeof (biff_workbook));
    if (!workbook)
	return NULL;
    workbook->magic1 = magic;
    workbook->magic2 = FREEXL_MAGIC_END;
    workbook->xls = NULL;
    workbook->fat = NULL;
    workbook->cfbf_version = 0;
    workbook->cfbf_sector_size = 0;
    workbook->start_sector = 0;
    workbook->size = 0;
    workbook->current_sector = 0;
    workbook->bytes_read = 0;
    workbook->current_offset = 0;
    memset (workbook->sector_buf, 0, sizeof (workbook->sector_buf));
    workbook->p_in = workbook->sector_buf;
    workbook->sector_end = 0;
    workbook->sector_ready = 0;
    workbook->ok_bof = -1;
    workbook->biff_version = 0;
    workbook->biff_max_record_size = 0;
    workbook->biff_content_type = 0;
    workbook->biff_code_page = 0;
    workbook->biff_book_code_page = 0;
    workbook->biff_date_mode = 0;
    workbook->biff_obfuscated = 0;
    workbook->utf8_converter = NULL;
    workbook->utf16_converter = NULL;
    memset (workbook->record, 0, sizeof (workbook->record));
    workbook->record_type = 0;
    workbook->prev_record_type = 0;
    workbook->record_size = 0;
    workbook->shared_strings.string_count = 0;
    workbook->shared_strings.utf8_strings = NULL;
    workbook->shared_strings.current_index = 0;
    workbook->shared_strings.current_utf16_buf = NULL;
    workbook->shared_strings.current_utf16_len = 0;
    workbook->shared_strings.current_utf16_off = 0;
    workbook->shared_strings.current_utf16_skip = 0;
    workbook->first_sheet = NULL;
    workbook->last_sheet = NULL;
    workbook->active_sheet = NULL;
    workbook->second_pass = 0;
    workbook->max_format_index = 0;
    workbook->biff_xf_next_index = 0;
    return workbook;
}

static int
insert_into_fat_chain (fat_chain * chain, unsigned int sector)
{
/* inserting a sector into the FAT chain [linked list] */
    fat_entry *entry = malloc (sizeof (fat_entry));
    if (entry == NULL)
	return FREEXL_INSUFFICIENT_MEMORY;
    entry->current_sector = chain->next_sector;
    chain->next_sector += 1;
    entry->next_sector = sector;
    entry->next = NULL;
    if (chain->first == NULL)
	chain->first = entry;
    if (chain->last != NULL)
	chain->last->next = entry;
    chain->last = entry;
    return FREEXL_OK;
}

static int
insert_into_miniFAT_chain (fat_chain * chain, unsigned int sector)
{
/* inserting a sector into the miniFAT chain [linked list] */
    fat_entry *entry = malloc (sizeof (fat_entry));
    if (entry == NULL)
	return FREEXL_INSUFFICIENT_MEMORY;
    entry->current_sector = chain->next_sectorMini;
    chain->next_sectorMini += 1;
    entry->next_sector = sector;
    entry->next = NULL;
    if (chain->firstMini == NULL)
	chain->firstMini = entry;
    if (chain->lastMini != NULL)
	chain->lastMini->next = entry;
    chain->lastMini = entry;
    return FREEXL_OK;
}

static fat_entry *
get_fat_entry (fat_chain * chain, unsigned int i_sect)
{
/* attempting to retrieve a FAT item [sector] */
    if (!chain)
	return NULL;
    if (i_sect < chain->fat_array_count)
	return *(chain->fat_array + i_sect);
    return NULL;
}

static int
build_fat_arrays (fat_chain * chain)
{
/* building FAT sectors array */
    fat_entry *entry;
    int i;

    if (!chain)
	return FREEXL_NULL_ARGUMENT;
    if (chain->fat_array)
	free (chain->fat_array);
    chain->fat_array = NULL;
    chain->fat_array_count = 0;
    if (chain->miniFAT_array)
	free (chain->miniFAT_array);
    chain->miniFAT_array = NULL;
    chain->miniFAT_array_count = 0;

    entry = chain->first;
    while (entry)
      {
	  /* counting how many sectors are into the FAT list */
	  chain->fat_array_count += 1;
	  entry = entry->next;
      }
    if (chain->fat_array_count == 0)
	return FREEXL_CFBF_EMPTY_FAT_CHAIN;

/* allocating the FAT sectors array */
    chain->fat_array = malloc (sizeof (fat_entry *) * chain->fat_array_count);
    if (chain->fat_array == NULL)
	return FREEXL_INSUFFICIENT_MEMORY;

    i = 0;
    entry = chain->first;
    while (entry)
      {
	  /* populating the pointer array */
	  *(chain->fat_array + i) = entry;
	  i++;
	  entry = entry->next;
      }

    entry = chain->firstMini;
    while (entry)
      {
	  /* counting how many sectors are into the miniFAT list */
	  chain->miniFAT_array_count += 1;
	  entry = entry->next;
      }
    if (chain->miniFAT_array_count > 0)
      {
	  /* allocating the miniFAT sectors array */
	  chain->miniFAT_array =
	      malloc (sizeof (fat_entry *) * chain->miniFAT_array_count);
	  if (chain->miniFAT_array == NULL)
	      return FREEXL_INSUFFICIENT_MEMORY;

	  i = 0;
	  entry = chain->firstMini;
	  while (entry)
	    {
		/* populating the pointer array */
		*(chain->miniFAT_array + i) = entry;
		i++;
		entry = entry->next;
	    }
      }
    return FREEXL_OK;
}

static void
select_active_sheet (biff_workbook * workbook, unsigned int current_offset)
{
/* selecting the currently acrive Sheet (if any) */
    biff_sheet *p_sheet;
    p_sheet = workbook->first_sheet;
    while (p_sheet)
      {
	  if (p_sheet->start_offset == current_offset)
	    {
		/* ok, this one is the current Sheet */
		workbook->active_sheet = p_sheet;
		return;
	    }
	  p_sheet = p_sheet->next;
      }
    workbook->active_sheet = NULL;
}

static int
read_fat_sector (FILE * xls, fat_chain * chain, unsigned int sector)
{
/* reading a FAT chain sector */
    long where = (sector + 1) * chain->sector_size;
    unsigned char buf[4096];
    unsigned char *p_buf = buf;
    int i_fat;
    int max_fat;
    if (fseek (xls, where, SEEK_SET) != 0)
	return FREEXL_CFBF_SEEK_ERROR;
    if (chain->sector_size == 4096)
	max_fat = 1024;
    else
	max_fat = 128;

/* reading a FAT sector */
    if (fread (buf, 1, chain->sector_size, xls) != chain->sector_size)
	return FREEXL_CFBF_READ_ERROR;

    for (i_fat = 0; i_fat < max_fat; i_fat++)
      {
	  int ret;
	  biff_word32 fat;
	  memcpy (fat.bytes, p_buf, 4);
	  p_buf += 4;
	  if (chain->swap)
	      swap32 (&fat);
	  ret = insert_into_fat_chain (chain, fat.value);
	  if (ret != FREEXL_OK)
	      return ret;
      }
    return FREEXL_OK;
}

static int
read_difat_sectors (FILE * xls, fat_chain * chain, unsigned int sector,
		    unsigned int num_sectors)
{
/* reading a DIFAT (DoubleIndirect) chain sector */
    unsigned int next_sector = sector;
    unsigned int blocks = 0;
    long where = (sector + 1) * chain->sector_size;
    biff_word32 difat[1024];
    int i_difat;
    int max_difat;
    int end_of_chain = 0;

    if (chain->sector_size == 4096)
	max_difat = 1024;
    else
	max_difat = 128;

    while (1)
      {
	  where = (next_sector + 1) * chain->sector_size;
	  if (fseek (xls, where, SEEK_SET) != 0)
	      return FREEXL_CFBF_SEEK_ERROR;
	  /* reading a DIFAT sector */
	  if (fread (&difat, 1, chain->sector_size, xls) != chain->sector_size)
	      return FREEXL_CFBF_READ_ERROR;
	  blocks++;
	  if (chain->swap)
	    {
		for (i_difat = 0; i_difat < max_difat; i_difat++)
		    swap32 (difat + i_difat);
	    }

	  for (i_difat = 0; i_difat < max_difat; i_difat++)
	    {
		if (difat[i_difat].value == 0xFFFFFFFE)
		  {
		      end_of_chain = 1;	/* end of FAT chain */
		      break;
		  }
		if (i_difat == max_difat - 1)
		    next_sector = difat[i_difat].value;
		else
		  {
		      int ret;
		      if (difat[i_difat].value == 0xFFFFFFFF)
			  continue;	/* unused sector */
		      ret = read_fat_sector (xls, chain, difat[i_difat].value);
		      if (ret != FREEXL_OK)
			  return ret;
		  }
	    }
	  if (blocks == num_sectors)
	      break;
	  if (end_of_chain)
	      break;
      }
    if (blocks != num_sectors)
	return 0;
    return 1;
}

static int
read_miniFAT_sectors (FILE * xls, fat_chain * chain, unsigned int sector,
		      unsigned int num_sectors)
{
/* reading miniFAT chain sectors */
    long where = (sector - 1) * chain->sector_size;
    unsigned char buf[4096];
    int i_fat;
    int max_fat;
    unsigned int block = 0;

    if (chain->sector_size == 4096)
	max_fat = 1024;
    else
	max_fat = 128;

    if (fseek (xls, where, SEEK_SET) != 0)
	return FREEXL_CFBF_SEEK_ERROR;
    while (block < num_sectors)
      {
	  unsigned char *p_buf = buf;
	  block++;
	  /* reading a miniFAT sector */
	  if (fread (&buf, 1, chain->sector_size, xls) != chain->sector_size)
	      return FREEXL_CFBF_READ_ERROR;
	  for (i_fat = 0; i_fat < max_fat; i_fat++)
	    {
		int ret;
		biff_word32 fat;
		memcpy (fat.bytes, p_buf, 4);
		p_buf += 4;
		if (chain->swap)
		    swap32 (&fat);
		ret = insert_into_miniFAT_chain (chain, fat.value);
		if (ret != FREEXL_OK)
		    return ret;
	    }
      }
    return 1;
}

static fat_chain *
read_cfbf_header (biff_workbook * workbook, int swap, int *err_code)
{
/* attempting to read and check FAT header */
    cfbf_header header;
    fat_chain *chain = NULL;
    int i_fat;
    int ret;
    unsigned char *p_fat = header.fat_sector_map;

    if (fread (&header, 1, 512, workbook->xls) != 512)
      {
	  *err_code = FREEXL_CFBF_READ_ERROR;
	  return NULL;
      }
    if (swap)
      {
	  /* BIG endian arch: swap required */
	  swap16 (&(header.minor_version));
	  swap16 (&(header.major_version));
	  swap16 (&(header.byte_order));
	  swap16 (&(header.sector_shift));
	  swap16 (&(header.mini_sector_shift));
	  swap16 (&(header.reserved1));
	  swap32 (&(header.reserved2));
	  swap32 (&(header.directory_sectors));
	  swap32 (&(header.fat_sectors));
	  swap32 (&(header.directory_start));
	  swap32 (&(header.transaction_signature));
	  swap32 (&(header.mini_cutoff));
	  swap32 (&(header.mini_fat_start));
	  swap32 (&(header.mini_fat_sectors));
	  swap32 (&(header.difat_start));
	  swap32 (&(header.difat_sectors));
      }

    if (header.signature[0] == 0xd0 && header.signature[1] == 0xcf
	&& header.signature[2] == 0x11 && header.signature[3] == 0xe0
	&& header.signature[4] == 0xa1 && header.signature[5] == 0xb1
	&& header.signature[6] == 0x1a && header.signature[7] == 0xe1)
	;			/* magic signature OK */
    else
      {
	  *err_code = FREEXL_CFBF_INVALID_SIGNATURE;
	  return NULL;
      }
    if (header.sector_shift.value == 9 || header.sector_shift.value == 12)
	;			/* ok, valid sector size */
    else
      {
	  *err_code = FREEXL_CFBF_INVALID_SECTOR_SIZE;
	  return NULL;
      }

    workbook->cfbf_version = header.major_version.value;
    if (header.sector_shift.value == 9)
	workbook->cfbf_sector_size = 512;
    if (header.sector_shift.value == 12)
	workbook->cfbf_sector_size = 4096;

    chain =
	alloc_fat_chain (swap, header.sector_shift.value,
			 header.directory_start.value);
    if (!chain)
      {
	  *err_code = FREEXL_INSUFFICIENT_MEMORY;
	  return NULL;
      }
    for (i_fat = 0; i_fat < 109; i_fat++)
      {
	  /* reading FAT sectors */
	  biff_word32 fat;
	  memcpy (fat.bytes, p_fat, 4);
	  p_fat += 4;
	  if (swap)
	      swap32 (&fat);
	  if (fat.value == 0xFFFFFFFF)
	      continue;		/* unused sector */
	  ret = read_fat_sector (workbook->xls, chain, fat.value);
	  if (ret != FREEXL_OK)
	    {
		*err_code = ret;
		destroy_fat_chain (chain);
		return NULL;
	    }
      }

    if (header.difat_sectors.value > 0)
      {
	  /* reading DoubleIndirect [DIFAT] sectors */
	  if (!read_difat_sectors
	      (workbook->xls, chain, header.difat_start.value,
	       header.difat_sectors.value))
	    {
		*err_code = FREEXL_CFBF_READ_ERROR;
		destroy_fat_chain (chain);
		return NULL;
	    }
      }

    if (header.mini_fat_sectors.value > 0)
      {
	  /* there is a miniFAT requiring to be supported */
	  chain->miniCutOff = header.mini_cutoff.value;
	  if (!read_miniFAT_sectors
	      (workbook->xls, chain, header.mini_fat_start.value,
	       header.mini_fat_sectors.value))
	    {
		*err_code = FREEXL_CFBF_READ_ERROR;
		destroy_fat_chain (chain);
		return NULL;
	    }
      }

    ret = build_fat_arrays (chain);
    if (ret != FREEXL_OK)
      {
	  *err_code = ret;
	  destroy_fat_chain (chain);
	  return NULL;
      }

    *err_code = FREEXL_OK;
    return chain;
}

static int
read_mini_stream (biff_workbook * workbook, int *errcode)
{
/* loading in memory the whole ministream */
    unsigned int len = 0;
    unsigned int sector = workbook->fat->miniFAT_start;
    unsigned char buf[4096];
    unsigned char *miniStream;
    fat_entry *entry;
    int eof = 0;

    if (workbook->fat->miniStream)
	free (workbook->fat->miniStream);
    workbook->fat->miniStream = NULL;
    miniStream = malloc (workbook->fat->miniFAT_len);
    if (miniStream == NULL)
      {
	  *errcode = FREEXL_INSUFFICIENT_MEMORY;
	  return 0;
      }
    while (len < workbook->fat->miniFAT_len)
      {
	  /* reading one sector */
	  unsigned int size;
	  long where = (sector + 1) * workbook->fat->sector_size;
	  if (fseek (workbook->xls, where, SEEK_SET) != 0)
	    {
		*errcode = FREEXL_CFBF_SEEK_ERROR;
		return 0;
	    }
	  if (fread (buf, 1, workbook->fat->sector_size, workbook->xls) !=
	      workbook->fat->sector_size)
	    {
		*errcode = FREEXL_CFBF_READ_ERROR;
		return 0;
	    }
	  size = workbook->fat->sector_size;
	  if ((len + size) > workbook->fat->miniFAT_len)
	      size = workbook->fat->miniFAT_len - len;
	  memcpy (miniStream + len, buf, size);
	  len += size;
	  entry = get_fat_entry (workbook->fat, sector);
	  if (entry == NULL)
	    {
		*errcode = FREEXL_CFBF_ILLEGAL_FAT_ENTRY;
		return 0;
	    }
	  if (entry->next_sector == 0xfffffffe)
	    {
		/* EOF: end-of-chain marker found */
		eof = 1;
		break;
	    }
	  sector = entry->next_sector;
      }
    if (!eof || len != workbook->fat->miniFAT_len)
      {
	  free (miniStream);
	  *errcode = FREEXL_INVALID_MINI_STREAM;
	  return 0;
      }
    workbook->fat->miniStream = miniStream;
    return 1;
}

static const char *
find_in_SST (biff_workbook * workbook, unsigned int string_index)
{
/* retieving a string from the SST */
    if (!workbook)
	return NULL;
    if (workbook->shared_strings.utf8_strings == NULL)
	return NULL;
    if (string_index < workbook->shared_strings.string_count)
	return *(workbook->shared_strings.utf8_strings + string_index);
    return NULL;
}

static int
parse_SST (biff_workbook * workbook, int swap)
{
/* attempting to parse a Shared String Table */
    unsigned int i_string;
    unsigned char *p_string;
    biff_word32 n_strings;
    unsigned int required;
    unsigned int available;

    if (workbook->shared_strings.string_count == 0
	&& workbook->shared_strings.utf8_strings == NULL)
      {
	  /* main SST record [initializing] */
	  memcpy (n_strings.bytes, workbook->record + 4, 4);
	  if (swap)
	      swap32 (&n_strings);
	  p_string = workbook->record + 8;
	  workbook->shared_strings.string_count = n_strings.value;
	  workbook->shared_strings.utf8_strings =
	      malloc (sizeof (char **) * workbook->shared_strings.string_count);
	  for (i_string = 0; i_string < workbook->shared_strings.string_count;
	       i_string++)
	      *(workbook->shared_strings.utf8_strings + i_string) = NULL;
      }
    else
      {
	  /* SST-CONTINUE */
	  char *utf8_string;
	  unsigned char mask;
	  unsigned int len;
	  int utf16 = 0;
	  int err;
	  unsigned int utf16_len = workbook->shared_strings.current_utf16_len;
	  unsigned int utf16_off = workbook->shared_strings.current_utf16_off;
	  unsigned int utf16_skip = workbook->shared_strings.current_utf16_skip;
	  char *utf16_buf = workbook->shared_strings.current_utf16_buf;
	  p_string = workbook->record;

	  if (workbook->shared_strings.current_utf16_len > 0)
	    {
		/* completing the last suspended string [split between records] */
		mask = *p_string;
		p_string++;
		len = utf16_len - utf16_off;

		if ((mask & 0x01) == 0x01)
		    utf16 = 1;
		if (!utf16)
		  {
		      /* 'stripped' UTF-16: requires padding */
		      unsigned int i;
		      for (i = 0; i < len; i++)
			{
			    *(utf16_buf + (utf16_off * 2) + (i * 2)) =
				*p_string;
			    p_string++;
			    *(utf16_buf + (utf16_off * 2) + ((i * 2) + 1)) =
				0x00;
			}
		  }
		else
		  {
		      /* already encoded as UTF-16 */
		      memcpy (utf16_buf + (utf16_off * 2), p_string, len * 2);
		      p_string += len * 2;
		  }

		/* skipping extra data (if any) */
		p_string += utf16_skip;

		/* converting text to UTF-8 */
		utf8_string =
		    convert_to_utf8 (workbook->utf16_converter, utf16_buf,
				     utf16_len * 2, &err);
		if (err)
		    return FREEXL_INVALID_CHARACTER;
		*(workbook->shared_strings.utf8_strings +
		  workbook->shared_strings.current_index) = utf8_string;
		free (workbook->shared_strings.current_utf16_buf);
		workbook->shared_strings.current_utf16_buf = NULL;
		workbook->shared_strings.current_utf16_len = 0;
		workbook->shared_strings.current_utf16_off = 0;
		workbook->shared_strings.current_utf16_skip = 0;
		workbook->shared_strings.current_index += 1;
	    }
      }

    for (i_string = workbook->shared_strings.current_index;
	 i_string < workbook->shared_strings.string_count; i_string++)
      {
	  /* parsing strings */
	  char *utf8_string;
	  unsigned int len;
	  int utf16 = 0;
	  biff_word16 word16;
	  unsigned int start_offset;
	  unsigned int extra_skip;

	  if ((unsigned int) (p_string - workbook->record) >=
	      workbook->record_size)
	    {
		/* end of record */
		return FREEXL_OK;
	    }

	  memcpy (word16.bytes, p_string, 2);
	  if (swap)
	      swap16 (&word16);
	  len = word16.value;
	  p_string += 2;

	  get_unicode_params (p_string, swap, &start_offset, &utf16,
			      &extra_skip);
	  p_string += start_offset;

	  /* initializing the current UTF-16 variables */
	  workbook->shared_strings.current_utf16_skip = extra_skip;
	  workbook->shared_strings.current_utf16_off = 0;
	  workbook->shared_strings.current_utf16_len = len;
	  workbook->shared_strings.current_utf16_buf =
	      malloc (workbook->shared_strings.current_utf16_len * 2);

	  if (!utf16)
	      required = len;
	  else
	      required = len * 2;
	  available = workbook->record_size - (p_string - workbook->record);
	  if (required > available)
	    {
		/* not enough input bytes: data spanning on next CONTINUE record */
		char *utf16_buf = workbook->shared_strings.current_utf16_buf;
		if (!utf16)
		  {
		      /* 'stripped' UTF-16: requires padding */
		      unsigned int i;
		      for (i = 0; i < available; i++)
			{
			    *(utf16_buf + (i * 2)) = *p_string;
			    p_string++;
			    *(utf16_buf + ((i * 2) + 1)) = 0x00;
			}
		      workbook->shared_strings.current_utf16_off = available;
		  }
		else
		  {
		      /* already encoded as UTF-16 */
		      memcpy (utf16_buf, p_string, available);
		      workbook->shared_strings.current_utf16_off =
			  available / 2;
		  }
		return FREEXL_OK;
	    }

	  if (!parse_unicode_string
	      (workbook->utf16_converter, len, utf16, p_string, &utf8_string))
	      return FREEXL_INVALID_CHARACTER;

	  /* skipping string data */
	  if (!utf16)
	      p_string += len;
	  else
	      p_string += len * 2;
	  /* skipping extra data (if any) */
	  p_string += workbook->shared_strings.current_utf16_skip;

	  *(workbook->shared_strings.utf8_strings + i_string) = utf8_string;
	  free (workbook->shared_strings.current_utf16_buf);
	  workbook->shared_strings.current_utf16_buf = NULL;
	  workbook->shared_strings.current_utf16_len = 0;
	  workbook->shared_strings.current_utf16_off = 0;
	  workbook->shared_strings.current_utf16_skip = 0;
	  workbook->shared_strings.current_index = i_string + 1;
      }

    return FREEXL_OK;
}

static void
check_format (char *utf8_string, int *is_date, int *is_datetime, int *is_time)
{
/* attempting to identify DATE, DATETIME or TIME formats */
    int y = 0;
    int m = 0;
    int d = 0;
    int h = 0;
    int s = 0;
    unsigned int i;
    for (i = 0; i < strlen (utf8_string); i++)
      {
	  switch (utf8_string[i])
	    {
	    case 'Y':
	    case 'y':
		y++;
		break;
	    case 'M':
	    case 'm':
		m++;
		break;
	    case 'D':
	    case 'd':
		d++;
		break;
	    case 'H':
	    case 'h':
		h++;
		break;
	    case 'S':
	    case 's':
		s++;
		break;
	    }
      }
    *is_date = 0;
    *is_datetime = 0;
    *is_time = 0;
    if (y && m && d && h)
      {
	  *is_datetime = 1;
	  return;
      }
    if ((y && m) || (m && d))
      {
	  *is_date = 1;
	  return;
      }
    if ((h && m) || (m && s))
	*is_time = 1;
}

static void
add_format_to_workbook (biff_workbook * workbook, unsigned short format_index,
			int is_date, int is_datetime, int is_time)
{
/* adding a new DATE/DATETIME/TIME format to the Workbook */
    if (workbook->max_format_index < BIFF_MAX_FORMAT)
      {
	  biff_format *format =
	      workbook->format_array + workbook->max_format_index;
	  format->format_index = format_index;
	  format->is_date = is_date;
	  format->is_datetime = is_datetime;
	  format->is_time = is_time;
	  workbook->max_format_index += 1;
      }
}

static void
add_xf_to_workbook (biff_workbook * workbook, unsigned short format_index)
{
/* adding a new XF to the Workbook */
    if (workbook->biff_xf_next_index < BIFF_MAX_XF)
	workbook->biff_xf_array[workbook->biff_xf_next_index] = format_index;
    workbook->biff_xf_next_index += 1;
}

static int
legacy_emergency_dimension (biff_workbook * workbook, int swap,
			    unsigned short type, unsigned short size,
			    unsigned int *rows, unsigned short *columns)
{
/* performing a preliminary pass so to get DIMENSION */
    biff_word16 record_type;
    biff_word16 record_size;
    unsigned char buf[16];
    int first = 1;
    long where;
    long restart_off = ftell (workbook->xls);

    record_type.value = type;
    record_size.value = size;

    while (1)
      {
	  /* looping on BIFF records */
	  if (!first)
	    {
		if (fread (&buf, 1, 4, workbook->xls) != 4)
		    return 0;
		memcpy (record_type.bytes, buf, 2);
		memcpy (record_size.bytes, buf + 2, 2);
	    }
	  else
	      first = 0;
	  if (swap)
	    {
		/* BIG endian arch: swap required */
		swap16 (&record_type);
		swap16 (&record_size);
	    }

	  if (record_type.value == BIFF_EOF)
	    {
		/* EOF marker found: the current stream is terminated */
		break;
	    }

	  if (record_type.value == BIFF_INTEGER_2
	      && workbook->biff_version == FREEXL_BIFF_VER_2)
	    {
		/* INTEGER marker found */
		biff_word16 word16;

		if (fread
		    (workbook->record, 1, record_size.value,
		     workbook->xls) != record_size.value)
		    return 0;

		memcpy (word16.bytes, workbook->record, 2);
		if (swap)
		    swap16 (&word16);
		if (word16.value > *rows)
		    *rows = word16.value;
		memcpy (word16.bytes, workbook->record + 2, 2);
		if (swap)
		    swap16 (&word16);
		if (word16.value > *columns)
		    *columns = word16.value;
		continue;
	    }

	  if ((record_type.value == BIFF_NUMBER_2
	       && workbook->biff_version == FREEXL_BIFF_VER_2)
	      || (record_type.value == BIFF_NUMBER
		  && (workbook->biff_version == FREEXL_BIFF_VER_3
		      || workbook->biff_version == FREEXL_BIFF_VER_4)))
	    {
		/* NUMBER marker found */
		biff_word16 word16;

		if (fread
		    (workbook->record, 1, record_size.value,
		     workbook->xls) != record_size.value)
		    return 0;

		memcpy (word16.bytes, workbook->record, 2);
		if (swap)
		    swap16 (&word16);
		if (word16.value > *rows)
		    *rows = word16.value;
		memcpy (word16.bytes, workbook->record + 2, 2);
		if (swap)
		    swap16 (&word16);
		if (word16.value > *columns)
		    *columns = word16.value;
		continue;
	    }

	  if ((record_type.value == BIFF_BOOLERR_2
	       && workbook->biff_version == FREEXL_BIFF_VER_2)
	      || (record_type.value == BIFF_BOOLERR
		  && (workbook->biff_version == FREEXL_BIFF_VER_3
		      || workbook->biff_version == FREEXL_BIFF_VER_4)))
	    {
		/* BOOLERR marker found */
		biff_word16 word16;

		if (fread
		    (workbook->record, 1, record_size.value,
		     workbook->xls) != record_size.value)
		    return 0;

		memcpy (word16.bytes, workbook->record, 2);
		if (swap)
		    swap16 (&word16);
		if (word16.value > *rows)
		    *rows = word16.value;
		memcpy (word16.bytes, workbook->record + 2, 2);
		if (swap)
		    swap16 (&word16);
		if (word16.value > *columns)
		    *columns = word16.value;
		continue;
	    }

	  if (record_type.value == BIFF_RK
	      && (workbook->biff_version == FREEXL_BIFF_VER_3
		  || workbook->biff_version == FREEXL_BIFF_VER_4))
	    {
		/* RK marker found */
		biff_word16 word16;

		if (fread
		    (workbook->record, 1, record_size.value,
		     workbook->xls) != record_size.value)
		    return 0;

		memcpy (word16.bytes, workbook->record, 2);
		if (swap)
		    swap16 (&word16);
		if (word16.value > *rows)
		    *rows = word16.value;
		memcpy (word16.bytes, workbook->record + 2, 2);
		if (swap)
		    swap16 (&word16);
		if (word16.value > *columns)
		    *columns = word16.value;
		continue;
	    }

	  if ((record_type.value == BIFF_LABEL_2
	       && workbook->biff_version == FREEXL_BIFF_VER_2)
	      || (record_type.value == BIFF_LABEL
		  && (workbook->biff_version == FREEXL_BIFF_VER_3
		      || workbook->biff_version == FREEXL_BIFF_VER_4)))
	    {
		/* LABEL marker found */
		biff_word16 word16;

		if (fread
		    (workbook->record, 1, record_size.value,
		     workbook->xls) != record_size.value)
		    return 0;

		memcpy (word16.bytes, workbook->record, 2);
		if (swap)
		    swap16 (&word16);
		if (word16.value > *rows)
		    *rows = word16.value;
		memcpy (word16.bytes, workbook->record + 2, 2);
		if (swap)
		    swap16 (&word16);
		if (word16.value > *columns)
		    *columns = word16.value;
		continue;
	    }

	  /* skipping to next record */
	  where = record_size.value;
	  if (fseek (workbook->xls, where, SEEK_CUR) != 0)
	      return 0;
      }

/* repositioning the stream offset */
    if (fseek (workbook->xls, restart_off, SEEK_SET) != 0)
	return 0;
    return 1;
}

static int
check_legacy_undeclared_dimension (biff_workbook * workbook, int swap,
				   unsigned short type, unsigned short size)
{
/* checking (and eventually saning) missing DIMENSION */
    if (workbook->active_sheet == NULL)
      {
	  char *utf8_name;
	  unsigned int rows = 0;
	  unsigned short columns = 0;
	  if (!legacy_emergency_dimension
	      (workbook, swap, type, size, &rows, &columns))
	      return 0;
	  /* initializing the worksheet */
	  utf8_name = malloc (10);
	  strcpy (utf8_name, "Worksheet");
	  if (!add_sheet_to_workbook (workbook, 0, 0, 0, utf8_name))
	      return 0;
	  workbook->active_sheet = workbook->first_sheet;
	  if (workbook->active_sheet != NULL)
	    {
		/* setting Sheet dimensions */
		int ret;
		workbook->active_sheet->rows = rows + 1;
		workbook->active_sheet->columns = columns + 1;
		ret = allocate_cells (workbook);
		if (ret != FREEXL_OK)
		    return 0;
	    }
      }
    return 1;
}

static int
read_legacy_biff (biff_workbook * workbook, int swap)
{
/* 
 * attempting to read legacy BIFF (versions 2,3,4)
 * no CFBF: simply a BIFF stream (one only Worksheet)
 */
    long where;
    biff_word16 word16;
    biff_word16 record_type;
    biff_word16 record_size;
    unsigned char buf[16];
    unsigned short format_index = 0;

/* attempting to get the main BOF */
    rewind (workbook->xls);
    if (fread (&buf, 1, 4, workbook->xls) != 4)
	return 0;
    memcpy (record_type.bytes, buf, 2);
    memcpy (record_size.bytes, buf + 2, 2);
    if (swap)
      {
	  /* BIG endian arch: swap required */
	  swap16 (&record_type);
	  swap16 (&record_size);
      }
    switch (record_type.value)
      {
      case BIFF_BOF_2:		/* BIFF2 */
	  workbook->biff_version = FREEXL_BIFF_VER_2;
	  workbook->ok_bof = 1;
	  break;
      case BIFF_BOF_3:		/* BIFF3 */
	  workbook->biff_version = FREEXL_BIFF_VER_3;
	  workbook->ok_bof = 1;
	  break;
      case BIFF_BOF_4:		/* BIFF4 */
	  workbook->biff_version = FREEXL_BIFF_VER_4;
	  workbook->ok_bof = 1;
	  break;
      };
    if (workbook->ok_bof != 1)
	return 0;

    where = record_size.value;
    if (fseek (workbook->xls, where, SEEK_CUR) != 0)
	return 0;

    while (1)
      {
	  /* looping on BIFF records */

	  if (fread (&buf, 1, 4, workbook->xls) != 4)
	      return 0;
	  memcpy (record_type.bytes, buf, 2);
	  memcpy (record_size.bytes, buf + 2, 2);
	  if (swap)
	    {
		/* BIG endian arch: swap required */
		swap16 (&record_type);
		swap16 (&record_size);
	    }

	  if (record_type.value == BIFF_SHEETSOFFSET)
	    {
/* unsupported BIFF4W format */
		return 0;
	    }

	  if (record_type.value == BIFF_EOF)
	    {
		/* EOF marker found: the current stream is terminated */
		return 1;
	    }

	  if (record_type.value == BIFF_CODEPAGE)
	    {
		/* CODEPAGE marker found */
		if (fread
		    (workbook->record, 1, record_size.value,
		     workbook->xls) != record_size.value)
		    return 0;
		memcpy (word16.bytes, workbook->record, 2);
		if (swap)
		    swap16 (&word16);
		workbook->biff_code_page = word16.value;
		if (workbook->ok_bof == 1)
		    workbook->biff_book_code_page = word16.value;
		if (!biff_set_utf8_converter (workbook))
		    return 0;
		continue;
	    }

	  if (record_type.value == BIFF_DATEMODE)
	    {
		/* DATEMODE marker found */
		if (fread
		    (workbook->record, 1, record_size.value,
		     workbook->xls) != record_size.value)
		    return 0;
		memcpy (word16.bytes, workbook->record, 2);
		if (swap)
		    swap16 (&word16);
		workbook->biff_date_mode = word16.value;
		continue;
	    }

	  if (record_type.value == BIFF_FILEPASS)
	    {
		/* PASSWORD marker found */
		workbook->biff_obfuscated = 1;
		goto skip_to_next;
	    }

	  if ((record_type.value == BIFF_FORMAT_2
	       && (workbook->biff_version == FREEXL_BIFF_VER_2
		   || workbook->biff_version == FREEXL_BIFF_VER_3))
	      || (record_type.value == BIFF_FORMAT
		  && (workbook->biff_version == FREEXL_BIFF_VER_4)))
	    {
		/* FORMAT marker found */
		biff_word16 word16;
		char *string;
		char *utf8_string;
		unsigned int len;
		int err;
		unsigned char *p_string;
		int is_date = 0;
		int is_datetime = 0;
		int is_time = 0;
		if (fread
		    (workbook->record, 1, record_size.value,
		     workbook->xls) != record_size.value)
		    return 0;

		if (workbook->biff_version == FREEXL_BIFF_VER_2
		    || workbook->biff_version == FREEXL_BIFF_VER_3)
		  {
		      len = *workbook->record;
		      p_string = workbook->record + 1;
		      string = malloc (len);
		      memcpy (string, p_string, len);
		      /* converting text to UTF-8 */
		      utf8_string =
			  convert_to_utf8 (workbook->utf8_converter, string,
					   len, &err);
		      free (string);
		      if (err)
			  return 0;
		      check_format (utf8_string, &is_date, &is_datetime,
				    &is_time);
		      free (utf8_string);
		      if (is_date || is_datetime || is_time)
			  add_format_to_workbook (workbook, format_index,
						  is_date, is_datetime,
						  is_time);
		  }
		else
		  {
		      memcpy (word16.bytes, workbook->record + 2, 2);
		      if (swap)
			  swap16 (&word16);
		      len = word16.value;
		      p_string = workbook->record + 4;

		      len = *(workbook->record + 2);
		      p_string = workbook->record + 3;
		      string = malloc (len);
		      memcpy (string, p_string, len);
		      /* converting text to UTF-8 */
		      utf8_string =
			  convert_to_utf8 (workbook->utf8_converter, string,
					   len, &err);
		      free (string);
		      if (err)
			  return 0;
		      check_format (utf8_string, &is_date, &is_datetime,
				    &is_time);
		      free (utf8_string);
		      if (is_date || is_datetime || is_time)
			  add_format_to_workbook (workbook, format_index,
						  is_date, is_datetime,
						  is_time);
		  }
		format_index++;
		continue;
	    }

	  if ((record_type.value == BIFF_XF_2
	       && workbook->biff_version == FREEXL_BIFF_VER_2)
	      || (record_type.value == BIFF_XF_3
		  && workbook->biff_version == FREEXL_BIFF_VER_3)
	      || (record_type.value == BIFF_XF_4
		  && workbook->biff_version == FREEXL_BIFF_VER_4))
	    {
		/* XF [Extended Format] marker found */
		unsigned char format;
		unsigned short s_format;
		if (fread
		    (workbook->record, 1, record_size.value,
		     workbook->xls) != record_size.value)
		    return 0;
		switch (workbook->biff_version)
		  {
		  case FREEXL_BIFF_VER_2:
		      format = *(workbook->record + 2);
		      format &= 0x3F;
		      s_format = format;
		      break;
		  case FREEXL_BIFF_VER_3:
		  case FREEXL_BIFF_VER_4:
		      format = *(workbook->record + 1);
		      s_format = format;
		      break;
		  };
		add_xf_to_workbook (workbook, s_format);
		continue;
	    }

	  if ((record_type.value == 0x0000
	       && workbook->biff_version == FREEXL_BIFF_VER_2)
	      || (record_type.value == BIFF_DIMENSION
		  && (workbook->biff_version == FREEXL_BIFF_VER_3
		      || workbook->biff_version == FREEXL_BIFF_VER_4)))
	    {
		/* DIMENSION marker found */
		biff_word16 word16;
		unsigned int rows;
		unsigned short columns;
		char *utf8_name;
		if (fread
		    (workbook->record, 1, record_size.value,
		     workbook->xls) != record_size.value)
		    return 0;

		memcpy (word16.bytes, workbook->record + 2, 2);
		if (swap)
		    swap16 (&word16);
		rows = word16.value;
		memcpy (word16.bytes, workbook->record + 6, 2);
		if (swap)
		    swap16 (&word16);
		columns = word16.value;
		utf8_name = malloc (10);
		strcpy (utf8_name, "Worksheet");
		if (!add_sheet_to_workbook (workbook, 0, 0, 0, utf8_name))
		    return 0;
		workbook->active_sheet = workbook->first_sheet;
		if (workbook->active_sheet != NULL)
		  {
		      /* setting Sheet dimensions */
		      int ret;
		      workbook->active_sheet->rows = rows;
		      workbook->active_sheet->columns = columns;
		      ret = allocate_cells (workbook);
		      if (ret != FREEXL_OK)
			  return 0;
		  }
		continue;
	    }

	  if (record_type.value == BIFF_INTEGER_2
	      && workbook->biff_version == FREEXL_BIFF_VER_2)
	    {
		/* INTEGER marker found */
		biff_word16 word16;
		unsigned short row;
		unsigned short col;
		unsigned short xf_index = 0;
		unsigned short num;
		int is_date;
		int is_datetime;
		int is_time;
		int ret;
		unsigned char format;

		if (!check_legacy_undeclared_dimension
		    (workbook, swap, record_type.value, record_size.value))
		    return 0;

		if (fread
		    (workbook->record, 1, record_size.value,
		     workbook->xls) != record_size.value)
		    return 0;

		memcpy (word16.bytes, workbook->record, 2);
		if (swap)
		    swap16 (&word16);
		row = word16.value;
		memcpy (word16.bytes, workbook->record + 2, 2);
		if (swap)
		    swap16 (&word16);
		col = word16.value;

		format = *(workbook->record + 4);
		format &= 0x3F;
		xf_index = format;
		memcpy (word16.bytes, workbook->record + 7, 2);
		if (swap)
		    swap16 (&word16);
		num = word16.value;

		if (!check_xf_datetime
		    (workbook, xf_index, &is_date, &is_datetime, &is_time))
		  {
		      is_date = 0;
		      is_datetime = 0;
		      is_time = 0;
		  }
		if (is_date)
		    ret =
			set_date_int_value (workbook, row, col,
					    workbook->biff_date_mode, num);
		else if (is_datetime)
		    ret =
			set_datetime_int_value (workbook, row, col,
						workbook->biff_date_mode, num);
		else if (is_time)
		    ret = set_time_double_value (workbook, row, col, 0.0);
		else
		    ret = set_int_value (workbook, row, col, num);
		if (ret != FREEXL_OK)
		    return 0;
		continue;
	    }

	  if ((record_type.value == BIFF_NUMBER_2
	       && workbook->biff_version == FREEXL_BIFF_VER_2)
	      || (record_type.value == BIFF_NUMBER
		  && (workbook->biff_version == FREEXL_BIFF_VER_3
		      || workbook->biff_version == FREEXL_BIFF_VER_4)))
	    {
		/* NUMBER marker found */
		biff_word16 word16;
		biff_float word_float;
		unsigned short row;
		unsigned short col;
		unsigned short xf_index = 0;
		double num;
		int is_date;
		int is_datetime;
		int is_time;
		int ret;

		if (!check_legacy_undeclared_dimension
		    (workbook, swap, record_type.value, record_size.value))
		    return 0;

		if (fread
		    (workbook->record, 1, record_size.value,
		     workbook->xls) != record_size.value)
		    return 0;

		memcpy (word16.bytes, workbook->record, 2);
		if (swap)
		    swap16 (&word16);
		row = word16.value;
		memcpy (word16.bytes, workbook->record + 2, 2);
		if (swap)
		    swap16 (&word16);
		col = word16.value;

		if (workbook->biff_version == FREEXL_BIFF_VER_2)
		  {
		      /* BIFF2 */
		      unsigned char format = *(workbook->record + 4);
		      format &= 0x3F;
		      xf_index = format;
		      memcpy (word_float.bytes, workbook->record + 7, 8);
		      if (swap)
			  swap_float (&word_float);
		      num = word_float.value;
		  }
		else
		  {
		      /* any other sebsequent version */
		      memcpy (word16.bytes, workbook->record + 4, 2);
		      if (swap)
			  swap16 (&word16);
		      xf_index = word16.value;
		      memcpy (word_float.bytes, workbook->record + 6, 8);
		      if (swap)
			  swap_float (&word_float);
		      num = word_float.value;
		  }
		if (!check_xf_datetime
		    (workbook, xf_index, &is_date, &is_datetime, &is_time))
		  {
		      is_date = 0;
		      is_datetime = 0;
		      is_time = 0;
		  }
		if (is_date)
		    ret =
			set_date_double_value (workbook, row, col,
					       workbook->biff_date_mode, num);
		else if (is_datetime)
		    ret =
			set_datetime_double_value (workbook, row, col,
						   workbook->biff_date_mode,
						   num);
		else if (is_time)
		    ret = set_time_double_value (workbook, row, col, num);
		else
		    ret = set_double_value (workbook, row, col, num);
		if (ret != FREEXL_OK)
		    return 0;
		continue;
	    }

	  if ((record_type.value == BIFF_BOOLERR_2
	       && workbook->biff_version == FREEXL_BIFF_VER_2)
	      || (record_type.value == BIFF_BOOLERR
		  && (workbook->biff_version == FREEXL_BIFF_VER_3
		      || workbook->biff_version == FREEXL_BIFF_VER_4)))
	    {
		/* BOOLERR marker found */
		biff_word16 word16;
		unsigned short row;
		unsigned short col;
		unsigned char value;
		int ret;

		if (!check_legacy_undeclared_dimension
		    (workbook, swap, record_type.value, record_size.value))
		    return 0;

		if (fread
		    (workbook->record, 1, record_size.value,
		     workbook->xls) != record_size.value)
		    return 0;

		memcpy (word16.bytes, workbook->record, 2);
		if (swap)
		    swap16 (&word16);
		row = word16.value;
		memcpy (word16.bytes, workbook->record + 2, 2);
		if (swap)
		    swap16 (&word16);
		col = word16.value;

		if (workbook->biff_version == FREEXL_BIFF_VER_2)
		  {
		      /* BIFF2 */
		      value = *(workbook->record + 7);
		  }
		else
		  {
		      /* any other sebsequent version */
		      value = *(workbook->record + 6);
		  }
		if (value != 0)
		    value = 1;
		ret = set_int_value (workbook, row, col, value);
		if (ret != FREEXL_OK)
		    return 0;
		continue;
	    }

	  if (record_type.value == BIFF_RK
	      && (workbook->biff_version == FREEXL_BIFF_VER_3
		  || workbook->biff_version == FREEXL_BIFF_VER_4))
	    {
		/* RK marker found */
		biff_word16 word16;
		biff_word32 word32;
		unsigned short row;
		unsigned short col;
		unsigned short xf_index;
		int int_value;
		double dbl_value;
		int is_date;
		int is_datetime;
		int is_time;
		int ret;

		if (!check_legacy_undeclared_dimension
		    (workbook, swap, record_type.value, record_size.value))
		    return 0;

		if (fread
		    (workbook->record, 1, record_size.value,
		     workbook->xls) != record_size.value)
		    return 0;

		memcpy (word16.bytes, workbook->record, 2);
		if (swap)
		    swap16 (&word16);
		row = word16.value;
		memcpy (word16.bytes, workbook->record + 2, 2);
		if (swap)
		    swap16 (&word16);
		col = word16.value;
		memcpy (word16.bytes, workbook->record + 4, 2);
		if (swap)
		    swap16 (&word16);
		xf_index = word16.value;
		memcpy (word32.bytes, workbook->record + 6, 4);
		if (decode_rk_integer (word32.bytes, &int_value, swap))
		  {
		      if (!check_xf_datetime
			  (workbook, xf_index, &is_date, &is_datetime,
			   &is_time))
			{
			    is_date = 0;
			    is_datetime = 0;
			    is_time = 0;
			}
		      if (is_date)
			  ret =
			      set_date_int_value (workbook, row, col,
						  workbook->biff_date_mode,
						  int_value);
		      else if (is_datetime)
			  ret =
			      set_datetime_int_value (workbook, row, col,
						      workbook->biff_date_mode,
						      int_value);
		      else if (is_time)
			  ret = set_time_double_value (workbook, row, col, 0.0);
		      else
			  ret = set_int_value (workbook, row, col, int_value);
		      if (ret != FREEXL_OK)
			  return 0;
		  }
		else if (decode_rk_float (word32.bytes, &dbl_value, swap))
		  {
		      if (!check_xf_datetime
			  (workbook, xf_index, &is_date, &is_datetime,
			   &is_time))
			{
			    is_date = 0;
			    is_datetime = 0;
			    is_time = 0;
			}
		      if (is_date)
			  ret =
			      set_date_double_value (workbook, row, col,
						     workbook->biff_date_mode,
						     dbl_value);
		      else if (is_datetime)
			  ret =
			      set_datetime_double_value (workbook, row, col,
							 workbook->biff_date_mode,
							 dbl_value);
		      else if (is_time)
			  ret =
			      set_time_double_value (workbook, row, col,
						     dbl_value);
		      else
			  ret =
			      set_double_value (workbook, row, col, dbl_value);
		      if (ret != FREEXL_OK)
			  return 0;
		  }
		else
		    return 0;
		continue;
	    }

	  if ((record_type.value == BIFF_LABEL_2
	       && workbook->biff_version == FREEXL_BIFF_VER_2)
	      || (record_type.value == BIFF_LABEL
		  && (workbook->biff_version == FREEXL_BIFF_VER_3
		      || workbook->biff_version == FREEXL_BIFF_VER_4)))
	    {
		/* LABEL marker found */
		biff_word16 word16;
		char *string;
		char *utf8_string;
		unsigned int len;
		int err;
		unsigned short row;
		unsigned short col;
		unsigned char *p_string;
		int ret;

		if (!check_legacy_undeclared_dimension
		    (workbook, swap, record_type.value, record_size.value))
		    return 0;

		if (fread
		    (workbook->record, 1, record_size.value,
		     workbook->xls) != record_size.value)
		    return 0;

		memcpy (word16.bytes, workbook->record, 2);
		if (swap)
		    swap16 (&word16);
		row = word16.value;
		memcpy (word16.bytes, workbook->record + 2, 2);
		if (swap)
		    swap16 (&word16);
		col = word16.value;

		if (workbook->biff_version == FREEXL_BIFF_VER_2)
		  {
		      len = *(workbook->record + 7);
		      p_string = workbook->record + 8;
		  }
		else
		  {
		      memcpy (word16.bytes, workbook->record + 6, 2);
		      if (swap)
			  swap16 (&word16);
		      len = word16.value;
		      p_string = workbook->record + 8;
		  }

		string = malloc (len);
		memcpy (string, p_string, len);

		/* converting text to UTF-8 */
		utf8_string =
		    convert_to_utf8 (workbook->utf8_converter, string, len,
				     &err);
		free (string);
		if (err)
		    return 0;
		ret = set_text_value (workbook, row, col, utf8_string);
		if (ret != FREEXL_OK)
		    return 0;
		continue;
	    }

	  /* skipping to next record */
	skip_to_next:
	  where = record_size.value;
	  if (fseek (workbook->xls, where, SEEK_CUR) != 0)
	      return 0;
      }

/* saving the current record */
    workbook->record_type = record_type.value;
    workbook->record_size = record_size.value;

    return 0;
}

static int
check_already_done (biff_workbook * workbook)
{
/* checking if the currently active sheet has been already loaded */
    if (workbook->active_sheet != NULL)
      {
	  if (workbook->active_sheet->already_done)
	    {
		/* already loaded */
		return 1;
	    }
      }
    return 0;
}

static int
check_undeclared_dimension (biff_workbook * workbook, unsigned int row,
			    unsigned short col)
{
/* checking if DIMENSION isn't yet set */
    if (workbook->active_sheet != NULL)
      {
	  if (workbook->active_sheet->valid_dimension == 0)
	    {
		/* not yet set */
		if (row > workbook->active_sheet->rows)
		    workbook->active_sheet->rows = row;
		if (col > workbook->active_sheet->columns)
		    workbook->active_sheet->columns = col;
		return 1;
	    }
      }
    return 0;
}

static int
parse_biff_record (biff_workbook * workbook, int swap)
{
/* 
 * attempting to parse a BIFF record 
 * please note well: BIFF5 and BIFF8 versions only
 *
 * oldest BIFF2, BIFF3 and BIFF4 are processed separatedly
 * by read_legacy_biff() function
 *
 */
    biff_word16 word16;
    unsigned int base_offset = workbook->current_offset;

    workbook->current_offset += workbook->record_size + 4;

    if (workbook->record_type == BIFF_CONTINUE)
      {
	  /* CONTINUE marker found: restoring the previous record type */
	  if (workbook->prev_record_type == BIFF_SST)
	    {
		/* continuing: SST [Shared String Table] */
		if (workbook->second_pass)
		    return FREEXL_OK;
		return parse_SST (workbook, swap);
	    }
	  return FREEXL_OK;
      }

    workbook->prev_record_type = workbook->record_type;
    if (workbook->ok_bof == -1)
      {
	  /* 
	   * the first record is expected to be of BOF type 
	   * and contains Version related information 
	   */
	  switch (workbook->record_type)
	    {
	    case BIFF_BOF:	/* BIFF5 or BIFF8 */
		memcpy (word16.bytes, workbook->record, 2);
		if (swap)
		    swap16 (&word16);
		if (word16.value == 0x0500)
		    workbook->biff_version = FREEXL_BIFF_VER_5;
		else if (word16.value == 0x0600)
		    workbook->biff_version = FREEXL_BIFF_VER_8;
		else
		  {
		      /* unknown, probably wrong or corrupted */
		      workbook->ok_bof = 0;
		      return FREEXL_BIFF_INVALID_BOF;
		  }
		workbook->ok_bof = 1;
		break;
	    default:
		workbook->ok_bof = 0;
		return FREEXL_BIFF_INVALID_BOF;
	    };
	  if (workbook->biff_version == FREEXL_BIFF_VER_8)
	      workbook->biff_max_record_size = 8224;
	  else
	      workbook->biff_max_record_size = 2080;
	  memcpy (word16.bytes, workbook->record + 2, 2);
	  if (swap)
	      swap16 (&word16);
	  workbook->biff_content_type = word16.value;
	  workbook->biff_code_page = 0;
	  return FREEXL_OK;
      }
    if (workbook->ok_bof == 0)
      {
	  /* we are expecting to find some BOF record here (not the main one) */
	  switch (workbook->record_type)
	    {
	    case BIFF_BOF:	/* BIFF5 or BIFF8 */
		workbook->ok_bof = 1;
		memcpy (word16.bytes, workbook->record + 2, 2);
		if (swap)
		    swap16 (&word16);
		workbook->biff_content_type = word16.value;
		workbook->biff_code_page = 0;
		break;
	    default:
		workbook->ok_bof = 0;
		return FREEXL_BIFF_INVALID_BOF;
	    };
	  select_active_sheet (workbook, base_offset);
	  return FREEXL_OK;
      }

    if (workbook->record_type == BIFF_EOF)
      {
	  /* EOF marker found: the current stream is terminated */
	  workbook->ok_bof = 0;
	  workbook->biff_content_type = 0;
	  workbook->biff_code_page = 0;
	  return FREEXL_OK;
      }

    if (workbook->record_type == BIFF_SST)
      {
	  /* SST [Shared String Table] marker found */
	  if (workbook->second_pass)
	      return FREEXL_OK;
	  return parse_SST (workbook, swap);
      }

    if (workbook->record_type == BIFF_CODEPAGE)
      {
	  /* CODEPAGE marker found */
	  memcpy (word16.bytes, workbook->record, 2);
	  if (swap)
	      swap16 (&word16);
	  workbook->biff_code_page = word16.value;
	  if (workbook->ok_bof == 1)
	      workbook->biff_book_code_page = word16.value;
	  if (!biff_set_utf8_converter (workbook))
	      return FREEXL_UNSUPPORTED_CHARSET;
	  return FREEXL_OK;
      }

    if (workbook->record_type == BIFF_DATEMODE)
      {
	  /* DATEMODE marker found */
	  memcpy (word16.bytes, workbook->record, 2);
	  if (swap)
	      swap16 (&word16);
	  workbook->biff_date_mode = word16.value;
	  return FREEXL_OK;
      }

    if (workbook->record_type == BIFF_FILEPASS)
      {
	  /* PASSWORD marker found */
	  workbook->biff_obfuscated = 1;
	  return FREEXL_OK;
      }

    if (workbook->record_type == BIFF_SHEET)
      {
	  /* SHEET marker found */
	  char *utf8_name;
	  char name[4096];
	  int err;
	  int len;
	  biff_word32 offset;

	  if (workbook->second_pass)
	    {
		if (workbook->active_sheet == NULL)
		    workbook->active_sheet = workbook->first_sheet;
		else
		    workbook->active_sheet = workbook->active_sheet->next;
		return FREEXL_OK;
	    }

	  memcpy (offset.bytes, workbook->record, 4);
	  if (swap)
	      swap32 (&offset);
	  len = workbook->record[6];
	  if (workbook->biff_version == FREEXL_BIFF_VER_5)
	    {
		/* BIFF5: codepage text */
		memcpy (name, workbook->record + 7, len);
		utf8_name =
		    convert_to_utf8 (workbook->utf8_converter, name, len, &err);
		if (err)
		    return FREEXL_INVALID_CHARACTER;
	    }
	  else
	    {
		/* BIFF8: Unicode text */
		if (workbook->record[7] == 0x00)
		  {
		      /* 'stripped' UTF-16: requires padding */
		      int i;
		      for (i = 0; i < len; i++)
			{
			    name[i * 2] = workbook->record[8 + i];
			    name[(i * 2) + 1] = 0x00;
			}
		      len *= 2;
		  }
		else
		  {
		      /* already encoded as UTF-16 */
		      len *= 2;
		      memcpy (name, workbook->record + 8, len);
		  }
		utf8_name =
		    convert_to_utf8 (workbook->utf16_converter, name, len,
				     &err);
		if (err)
		    return FREEXL_INVALID_CHARACTER;
	    }
	  if (!add_sheet_to_workbook
	      (workbook, offset.value, workbook->record[4], workbook->record[5],
	       utf8_name))
	      return FREEXL_INSUFFICIENT_MEMORY;
	  return FREEXL_OK;
      }

    if (workbook->record_type == BIFF_DIMENSION
	&& (workbook->biff_version == FREEXL_BIFF_VER_5
	    || workbook->biff_version == FREEXL_BIFF_VER_8))
      {
	  /* DIMENSION marker found */
	  biff_word16 word16;
	  biff_word32 word32;
	  unsigned int rows;
	  unsigned short columns;

	  if (workbook->second_pass)
	      return FREEXL_OK;
	  if (workbook->biff_version == FREEXL_BIFF_VER_8)
	    {
		/* BIFF8: 32-bit row index */
		memcpy (word32.bytes, workbook->record + 4, 4);
		if (swap)
		    swap32 (&word32);
		rows = word32.value;
		memcpy (word16.bytes, workbook->record + 10, 2);
		if (swap)
		    swap16 (&word16);
		columns = word16.value;
	    }
	  else
	    {
		/* any previous version: 16-bit row index */
		memcpy (word16.bytes, workbook->record + 2, 2);
		if (swap)
		    swap16 (&word16);
		rows = word16.value;
		memcpy (word16.bytes, workbook->record + 6, 2);
		if (swap)
		    swap16 (&word16);
		columns = word16.value;
	    }
	  if (workbook->active_sheet != NULL)
	    {
		/* setting Sheet dimensions */
		int ret;
		workbook->active_sheet->rows = rows;
		workbook->active_sheet->columns = columns;
		ret = allocate_cells (workbook);
		if (ret != FREEXL_OK)
		    return ret;
		workbook->active_sheet->valid_dimension = 1;
	    }
	  return FREEXL_OK;
      }

    if (workbook->magic1 == FREEXL_MAGIC_INFO)
      {
	  /* when open in INFO mode we can safely ignore any other */
	  return FREEXL_OK;
      }

    if (workbook->record_type == BIFF_FORMAT
	&& (workbook->biff_version == FREEXL_BIFF_VER_5
	    || workbook->biff_version == FREEXL_BIFF_VER_8))
      {
	  /* FORMAT marker found */
	  biff_word16 word16;
	  char *string;
	  char *utf8_string;
	  unsigned int len;
	  int err;
	  unsigned short format_index;
	  unsigned char *p_string;
	  int is_date = 0;
	  int is_datetime = 0;
	  int is_time = 0;

	  if (workbook->second_pass)
	      return FREEXL_OK;
	  if (workbook->biff_version == FREEXL_BIFF_VER_5)
	    {
		/* CODEPAGE string */
		memcpy (word16.bytes, workbook->record, 2);
		if (swap)
		    swap16 (&word16);
		format_index = word16.value;
		len = *(workbook->record + 3);
		p_string = workbook->record + 3;
		string = malloc (len);
		memcpy (string, p_string, len);

		/* converting text to UTF-8 */
		utf8_string =
		    convert_to_utf8 (workbook->utf8_converter, string, len,
				     &err);
		free (string);
		if (err)
		    return FREEXL_INVALID_CHARACTER;
		check_format (utf8_string, &is_date, &is_datetime, &is_time);
		free (utf8_string);
		if (is_date || is_datetime || is_time)
		    add_format_to_workbook (workbook, format_index, is_date,
					    is_datetime, is_time);
	    }
	  if (workbook->biff_version == FREEXL_BIFF_VER_8)
	    {
		/* please note: this always is UTF-16 */
		int utf16 = 0;
		unsigned int start_offset;
		unsigned int extra_skip;
		memcpy (word16.bytes, workbook->record, 2);
		if (swap)
		    swap16 (&word16);
		format_index = word16.value;
		memcpy (word16.bytes, workbook->record + 2, 2);
		if (swap)
		    swap16 (&word16);
		len = word16.value;
		p_string = workbook->record + 4;
		get_unicode_params (p_string, swap, &start_offset, &utf16,
				    &extra_skip);
		p_string += start_offset;
		if (!parse_unicode_string
		    (workbook->utf16_converter, len, utf16, p_string,
		     &utf8_string))
		    return FREEXL_INVALID_CHARACTER;
		check_format (utf8_string, &is_date, &is_datetime, &is_time);
		free (utf8_string);
		if (is_date || is_datetime || is_time)
		    add_format_to_workbook (workbook, format_index, is_date,
					    is_datetime, is_time);
	    }
	  return FREEXL_OK;
      }

    if (workbook->record_type == BIFF_XF
	&& (workbook->biff_version == FREEXL_BIFF_VER_5
	    || workbook->biff_version == FREEXL_BIFF_VER_8))
      {
	  /* XF [Extended Format] marker found */
	  unsigned short s_format;
	  biff_word16 word16;
	  if (workbook->second_pass)
	      return FREEXL_OK;
	  switch (workbook->biff_version)
	    {
	    case FREEXL_BIFF_VER_5:
	    case FREEXL_BIFF_VER_8:
		memcpy (word16.bytes, workbook->record + 2, 2);
		if (swap)
		    swap16 (&word16);
		s_format = word16.value;
		break;
	    };
	  add_xf_to_workbook (workbook, s_format);
	  return FREEXL_OK;
      }

    if ((workbook->record_type == BIFF_NUMBER
	 && (workbook->biff_version == FREEXL_BIFF_VER_5
	     || workbook->biff_version == FREEXL_BIFF_VER_8)))
      {
	  /* NUMBER marker found */
	  biff_word16 word16;
	  biff_float word_float;
	  unsigned short row;
	  unsigned short col;
	  unsigned short xf_index = 0;
	  double num;
	  int is_date;
	  int is_datetime;
	  int is_time;
	  int ret;

	  if (check_already_done (workbook))
	      return FREEXL_OK;

	  memcpy (word16.bytes, workbook->record, 2);
	  if (swap)
	      swap16 (&word16);
	  row = word16.value;
	  memcpy (word16.bytes, workbook->record + 2, 2);
	  if (swap)
	      swap16 (&word16);
	  col = word16.value;

	  if (check_undeclared_dimension (workbook, row, col))
	      return FREEXL_OK;

	  memcpy (word16.bytes, workbook->record + 4, 2);
	  if (swap)
	      swap16 (&word16);
	  xf_index = word16.value;
	  memcpy (word_float.bytes, workbook->record + 6, 8);
	  if (swap)
	      swap_float (&word_float);
	  num = word_float.value;

	  if (!check_xf_datetime_58
	      (workbook, xf_index, &is_date, &is_datetime, &is_time))
	    {
		is_date = 0;
		is_datetime = 0;
		is_time = 0;
	    }
	  if (is_date)
	      ret =
		  set_date_double_value (workbook, row, col,
					 workbook->biff_date_mode, num);
	  else if (is_datetime)
	      ret =
		  set_datetime_double_value (workbook, row, col,
					     workbook->biff_date_mode, num);
	  else if (is_time)
	      ret = set_time_double_value (workbook, row, col, num);
	  else
	      ret = set_double_value (workbook, row, col, num);
	  if (ret != FREEXL_OK)
	      return ret;
	  return FREEXL_OK;
      }

    if ((workbook->record_type == BIFF_BOOLERR
	 && (workbook->biff_version == FREEXL_BIFF_VER_5
	     || workbook->biff_version == FREEXL_BIFF_VER_8)))
      {
	  /* BOOLERR marker found */
	  biff_word16 word16;
	  unsigned short row;
	  unsigned short col;
	  unsigned char value;
	  int ret;

	  if (check_already_done (workbook))
	      return FREEXL_OK;

	  memcpy (word16.bytes, workbook->record, 2);
	  if (swap)
	      swap16 (&word16);
	  row = word16.value;
	  memcpy (word16.bytes, workbook->record + 2, 2);
	  if (swap)
	      swap16 (&word16);
	  col = word16.value;

	  if (check_undeclared_dimension (workbook, row, col))
	      return FREEXL_OK;

	  value = *(workbook->record + 6);
	  if (value != 0)
	      value = 1;
	  ret = set_int_value (workbook, row, col, value);
	  if (ret != FREEXL_OK)
	      return ret;
	  return FREEXL_OK;
      }

    if (workbook->record_type == BIFF_RK
	&& (workbook->biff_version == FREEXL_BIFF_VER_5
	    || workbook->biff_version == FREEXL_BIFF_VER_8))
      {
	  /* RK marker found */
	  biff_word16 word16;
	  biff_word32 word32;
	  unsigned short row;
	  unsigned short col;
	  unsigned short xf_index;
	  int int_value;
	  double dbl_value;
	  int is_date;
	  int is_datetime;
	  int is_time;
	  int ret;

	  if (check_already_done (workbook))
	      return FREEXL_OK;

	  memcpy (word16.bytes, workbook->record, 2);
	  if (swap)
	      swap16 (&word16);
	  row = word16.value;
	  memcpy (word16.bytes, workbook->record + 2, 2);
	  if (swap)
	      swap16 (&word16);
	  col = word16.value;

	  if (check_undeclared_dimension (workbook, row, col))
	      return FREEXL_OK;

	  memcpy (word16.bytes, workbook->record + 4, 2);
	  if (swap)
	      swap16 (&word16);
	  xf_index = word16.value;
	  memcpy (word32.bytes, workbook->record + 6, 4);
	  if (decode_rk_integer (word32.bytes, &int_value, swap))
	    {
		if (!check_xf_datetime_58
		    (workbook, xf_index, &is_date, &is_datetime, &is_time))
		  {
		      is_date = 0;
		      is_datetime = 0;
		      is_time = 0;
		  }
		if (is_date)
		    ret =
			set_date_int_value (workbook, row, col,
					    workbook->biff_date_mode,
					    int_value);
		else if (is_datetime)
		    ret =
			set_datetime_int_value (workbook, row, col,
						workbook->biff_date_mode,
						int_value);
		else if (is_time)
		    ret = set_time_double_value (workbook, row, col, 0.0);
		else
		    ret = set_int_value (workbook, row, col, int_value);
		if (ret != FREEXL_OK)
		    return ret;
	    }
	  else if (decode_rk_float (word32.bytes, &dbl_value, swap))
	    {
		if (!check_xf_datetime_58
		    (workbook, xf_index, &is_date, &is_datetime, &is_time))
		  {
		      is_date = 0;
		      is_datetime = 0;
		      is_time = 0;
		  }
		if (is_date)
		    ret =
			set_date_double_value (workbook, row, col,
					       workbook->biff_date_mode,
					       dbl_value);
		else if (is_datetime)
		    ret =
			set_datetime_double_value (workbook, row, col,
						   workbook->biff_date_mode,
						   dbl_value);
		else if (is_time)
		    ret = set_time_double_value (workbook, row, col, dbl_value);
		else
		    ret = set_double_value (workbook, row, col, dbl_value);
		if (ret != FREEXL_OK)
		    return ret;
	    }
	  else
	      return FREEXL_ILLEGAL_RK_VALUE;
	  return FREEXL_OK;
      }

    if (workbook->record_type == BIFF_MULRK
	&& (workbook->biff_version == FREEXL_BIFF_VER_5
	    || workbook->biff_version == FREEXL_BIFF_VER_8))
      {
	  /* MULRK marker found */
	  biff_word16 word16;
	  biff_word32 word32;
	  unsigned short row;
	  unsigned short col;
	  unsigned short xf_index;
	  unsigned int off = 4;
	  int int_value;
	  double dbl_value;
	  int is_date;
	  int is_datetime;
	  int is_time;
	  int ret;

	  if (check_already_done (workbook))
	      return FREEXL_OK;

	  memcpy (word16.bytes, workbook->record, 2);
	  if (swap)
	      swap16 (&word16);
	  row = word16.value;
	  memcpy (word16.bytes, workbook->record + 2, 2);
	  if (swap)
	      swap16 (&word16);
	  col = word16.value;

	  if (check_undeclared_dimension (workbook, row, col))
	      return FREEXL_OK;

	  while ((off + 6) < workbook->record_size)
	    {
		/* fetching one cell value */
		memcpy (word16.bytes, workbook->record + off, 2);
		if (swap)
		    swap16 (&word16);
		xf_index = word16.value;
		memcpy (word32.bytes, workbook->record + off + 2, 4);
		if (decode_rk_integer (word32.bytes, &int_value, swap))
		  {
		      if (!check_xf_datetime_58
			  (workbook, xf_index, &is_date, &is_datetime,
			   &is_time))
			{
			    is_date = 0;
			    is_datetime = 0;
			    is_time = 0;
			}
		      if (is_date)
			  ret =
			      set_date_int_value (workbook, row, col,
						  workbook->biff_date_mode,
						  int_value);
		      else if (is_datetime)
			  ret =
			      set_datetime_int_value (workbook, row, col,
						      workbook->biff_date_mode,
						      int_value);
		      else if (is_time)
			  ret = set_time_double_value (workbook, row, col, 0.0);
		      else
			  ret = set_int_value (workbook, row, col, int_value);
		      if (ret != FREEXL_OK)
			  return ret;
		  }
		else if (decode_rk_float (word32.bytes, &dbl_value, swap))
		  {
		      if (!check_xf_datetime_58
			  (workbook, xf_index, &is_date, &is_datetime,
			   &is_time))
			{
			    is_date = 0;
			    is_datetime = 0;
			    is_time = 0;
			}
		      if (is_date)
			  ret =
			      set_date_double_value (workbook, row, col,
						     workbook->biff_date_mode,
						     dbl_value);
		      else if (is_datetime)
			  ret =
			      set_datetime_double_value (workbook, row, col,
							 workbook->biff_date_mode,
							 dbl_value);
		      else if (is_time)
			  ret =
			      set_time_double_value (workbook, row, col,
						     dbl_value);
		      else
			  ret =
			      set_double_value (workbook, row, col, dbl_value);
		      if (ret != FREEXL_OK)
			  return ret;
		  }
		else
		    return FREEXL_ILLEGAL_MULRK_VALUE;
		off += 6;
		col++;
	    }
	  return FREEXL_OK;
      }

    if (workbook->record_type == BIFF_LABEL
	&& (workbook->biff_version == FREEXL_BIFF_VER_5
	    || workbook->biff_version == FREEXL_BIFF_VER_8))
      {
	  /* LABEL marker found */
	  biff_word16 word16;
	  char *string;
	  char *utf8_string;
	  unsigned int len;
	  int err;
	  unsigned short row;
	  unsigned short col;
	  unsigned char *p_string;
	  int ret;

	  if (check_already_done (workbook))
	      return FREEXL_OK;

	  memcpy (word16.bytes, workbook->record, 2);
	  if (swap)
	      swap16 (&word16);
	  row = word16.value;
	  memcpy (word16.bytes, workbook->record + 2, 2);
	  if (swap)
	      swap16 (&word16);
	  col = word16.value;

	  if (check_undeclared_dimension (workbook, row, col))
	      return FREEXL_OK;

	  memcpy (word16.bytes, workbook->record + 6, 2);
	  if (swap)
	      swap16 (&word16);
	  len = word16.value;
	  p_string = workbook->record + 8;

	  if (workbook->biff_version == FREEXL_BIFF_VER_5)
	    {
		/* CODEPAGE string */
		string = malloc (len);
		memcpy (string, p_string, len);

		/* converting text to UTF-8 */
		utf8_string =
		    convert_to_utf8 (workbook->utf8_converter, string, len,
				     &err);
		free (string);
		if (err)
		    return FREEXL_INVALID_CHARACTER;
	    }
	  else
	    {
		/* please note: this always is UTF-16 [BIFF8] */
		int utf16 = 0;
		unsigned int start_offset;
		unsigned int extra_skip;
		get_unicode_params (p_string, swap, &start_offset, &utf16,
				    &extra_skip);
		p_string += start_offset;
		if (!parse_unicode_string
		    (workbook->utf16_converter, len, utf16, p_string,
		     &utf8_string))
		    return FREEXL_INVALID_CHARACTER;
	    }
	  ret = set_text_value (workbook, row, col, utf8_string);
	  if (ret != FREEXL_OK)
	      return ret;
	  return FREEXL_OK;
      }

    if (workbook->record_type == BIFF_LABEL_SST
	&& workbook->biff_version == FREEXL_BIFF_VER_8)
      {
	  /* LABELSST marker found */
	  biff_word16 word16;
	  biff_word32 word32;
	  unsigned short row;
	  unsigned short col;
	  unsigned int string_index;
	  const char *utf8_string;
	  int ret;

	  if (check_already_done (workbook))
	      return FREEXL_OK;

	  memcpy (word16.bytes, workbook->record, 2);
	  if (swap)
	      swap16 (&word16);
	  row = word16.value;
	  memcpy (word16.bytes, workbook->record + 2, 2);
	  if (swap)
	      swap16 (&word16);
	  col = word16.value;

	  if (check_undeclared_dimension (workbook, row, col))
	      return FREEXL_OK;

	  memcpy (word32.bytes, workbook->record + 6, 4);
	  if (swap)
	      swap32 (&word32);
	  string_index = word32.value;
	  utf8_string = find_in_SST (workbook, string_index);
	  if (!utf8_string)
	      return FREEXL_BIFF_ILLEGAL_SST_INDEX;
	  ret = set_sst_value (workbook, row, col, utf8_string);
	  if (ret != FREEXL_OK)
	      return ret;
	  return FREEXL_OK;
      }

    return FREEXL_OK;
}

static int
read_cfbf_sector (biff_workbook * workbook, unsigned char *buf)
{
/* attempting to read a physical sector from the CFBF stream */
    long where = (workbook->current_sector + 1) * workbook->fat->sector_size;
    if (fseek (workbook->xls, where, SEEK_SET) != 0)
	return FREEXL_CFBF_SEEK_ERROR;
    if (fread (buf, 1, workbook->fat->sector_size, workbook->xls) !=
	workbook->fat->sector_size)
	return FREEXL_CFBF_READ_ERROR;
    return FREEXL_OK;
}

static int
read_cfbf_next_sector (biff_workbook * workbook, int *errcode)
{
/* attempting to read the next sector from the CFBF stream */
    int ret;
    fat_entry *entry = get_fat_entry (workbook->fat, workbook->current_sector);
    if (entry == NULL)
      {
	  *errcode = FREEXL_CFBF_ILLEGAL_FAT_ENTRY;
	  return 0;
      }
    if (entry->next_sector == 0xfffffffe)
      {
	  /* EOF: end-of-chain marker found */
	  *errcode = FREEXL_OK;
	  return -1;
      }
    workbook->current_sector = entry->next_sector;
    if (workbook->sector_end > workbook->fat->sector_size)
      {
	  /* shifting back the current sector buffer */
	  memcpy (workbook->sector_buf,
		  workbook->sector_buf + workbook->fat->sector_size,
		  workbook->fat->sector_size);
	  workbook->p_in -= workbook->fat->sector_size;
      }
/* reading into the second half of the sector buffer */
    ret =
	read_cfbf_sector (workbook,
			  workbook->sector_buf + workbook->fat->sector_size);
    if (ret != FREEXL_OK)
      {
	  *errcode = ret;
	  return 0;
      }
    workbook->bytes_read += workbook->fat->sector_size;
    if (workbook->bytes_read > workbook->size)
      {
	  /* incomplete last sector */
	  workbook->sector_end =
	      (workbook->fat->sector_size * 2) - (workbook->bytes_read -
						  workbook->size);
      }
    else
	workbook->sector_end = (workbook->fat->sector_size * 2);
    *errcode = FREEXL_OK;
    return 1;
}

static int
read_biff_next_record (biff_workbook * workbook, int swap, int *errcode)
{
/* 
 * attempting to read the next BIFF record
 * from the Workbook stream
 */
    biff_word16 record_type;
    biff_word16 record_size;
    int ret;

    if (workbook->sector_ready == 0)
      {
	  /* first access: loading the first stream sector */
	  ret = read_cfbf_sector (workbook, workbook->sector_buf);
	  if (ret != FREEXL_OK)
	    {
		*errcode = ret;
		return 0;
	    }
	  workbook->current_sector = workbook->start_sector;
	  workbook->bytes_read += workbook->fat->sector_size;
	  if (workbook->bytes_read > workbook->size)
	    {
		/* incomplete last sector */
		workbook->sector_end =
		    workbook->fat->sector_size - (workbook->bytes_read -
						  workbook->size);
	    }
	  else
	      workbook->sector_end = workbook->fat->sector_size;
	  workbook->p_in = workbook->sector_buf;
	  workbook->sector_ready = 1;
      }

/* 
 * four bytes are now expected:
 * USHORT record-type
 * USHORT record-size
 */
    if ((workbook->p_in - workbook->sector_buf) + 4 > workbook->sector_end)
      {
	  /* reading next sector */
	  ret = read_cfbf_next_sector (workbook, errcode);
	  if (ret == -1)
	      return -1;	/* EOF found */
	  if (ret == 0)
	      return 0;
      }
/* fetching record-type and record-size */
    memcpy (record_type.bytes, workbook->p_in, 2);
    workbook->p_in += 2;
    memcpy (record_size.bytes, workbook->p_in, 2);
    workbook->p_in += 2;
    if (swap)
      {
	  /* BIG endian arch: swap required */
	  swap16 (&record_type);
	  swap16 (&record_size);
      }
/* 
/ Sandro 2011-09-04
/ apparently a record-type 0x0000 and a record-size 0
/ seems to be an alternative way to mark EOF
*/
    if (record_type.value == 0x0000 && record_size.value == 0)
	return -1;

/* saving the current record */
    workbook->record_type = record_type.value;
    workbook->record_size = record_size.value;

    if (((workbook->p_in + workbook->record_size) - workbook->sector_buf) >
	workbook->sector_end)
      {
	  /* the current record spans on the following sector(s) */
	  unsigned int already_done;
	  unsigned int chunk =
	      workbook->sector_end - (workbook->p_in - workbook->sector_buf);
	  memcpy (workbook->record, workbook->p_in, chunk);
	  workbook->p_in += chunk;
	  already_done = chunk;

	  while (already_done < workbook->record_size)
	    {
		/* reading a further sector */
		ret = read_cfbf_next_sector (workbook, errcode);
		if (ret == -1)
		    return -1;	/* EOF found */
		if (ret == 0)
		    return 0;
		chunk = workbook->record_size - already_done;
		if (chunk <= workbook->fat->sector_size)
		  {
		      /* ok, finished: whole record reassembled */
		      memcpy (workbook->record + already_done, workbook->p_in,
			      chunk);
		      workbook->p_in += chunk;
		      goto record_done;
		  }
		/* record still spanning on the following sector */
		memcpy (workbook->record + already_done, workbook->p_in,
			workbook->fat->sector_size);
		workbook->p_in += workbook->fat->sector_size;
		already_done += workbook->fat->sector_size;
	    }
      }
    else
      {
	  /* the record is fully contained into the current sector */
	  memcpy (workbook->record, workbook->p_in, workbook->record_size);
	  workbook->p_in += record_size.value;
      }
  record_done:
    ret = parse_biff_record (workbook, swap);
    if (ret != FREEXL_OK)
	return 0;
    *errcode = FREEXL_OK;
    return 1;
}

static int
read_mini_biff_next_record (biff_workbook * workbook, int swap, int *errcode)
{
/* 
 * attempting to read the next BIFF record
 * from the Workbook MINI-stream
 */
    biff_word16 record_type;
    biff_word16 record_size;
    int ret;

/* 
 * four bytes are now expected:
 * USHORT record-type
 * USHORT record-size
 */
    if ((workbook->p_in - workbook->fat->miniStream) + 4 > (int) workbook->size)
	return -1;		/* EOF found */

/* fetching record-type and record-size */
    memcpy (record_type.bytes, workbook->p_in, 2);
    workbook->p_in += 2;
    memcpy (record_size.bytes, workbook->p_in, 2);
    workbook->p_in += 2;
    if (swap)
      {
	  /* BIG endian arch: swap required */
	  swap16 (&record_type);
	  swap16 (&record_size);
      }
/* saving the current record */
    workbook->record_type = record_type.value;
    workbook->record_size = record_size.value;

    memcpy (workbook->record, workbook->p_in, workbook->record_size);
    workbook->p_in += record_size.value;

    ret = parse_biff_record (workbook, swap);
    if (ret != FREEXL_OK)
	return 0;
    *errcode = FREEXL_OK;
    return 1;
}

static int
parse_dir_entry (void *block, int swap, iconv_t utf16_utf8_converter,
		 unsigned int *workbook, unsigned int *workbook_len,
		 unsigned int *miniFAT_start, unsigned int *miniFAT_len,
		 int *rootEntry)
{
/* parsing a Directory entry */
    char *name;
    int err;
    cfbf_dir_entry *entry = (cfbf_dir_entry *) block;
    if (swap)
      {
	  /* BIG endian arch: swap required */
	  swap16 (&(entry->name_size));
	  swap32 (&(entry->previous));
	  swap32 (&(entry->next));
	  swap32 (&(entry->child));
	  swap32 (&(entry->timestamp_1));
	  swap32 (&(entry->timestamp_2));
	  swap32 (&(entry->timestamp_3));
	  swap32 (&(entry->timestamp_4));
	  swap32 (&(entry->start_sector));
	  swap32 (&(entry->extra_size));
	  swap32 (&(entry->size));
      }

    name =
	convert_to_utf8 (utf16_utf8_converter, entry->name,
			 entry->name_size.value, &err);
    if (err)
	return FREEXL_INVALID_CHARACTER;

    if (strcmp (name, "Root Entry") == 0)
      {
	  *miniFAT_start = entry->start_sector.value;
	  *miniFAT_len = entry->size.value;
	  *rootEntry = 1;
      }
    else
	*rootEntry = 0;

    if (strcmp (name, "Workbook") == 0 || strcmp (name, "Book") == 0)
      {
	  *workbook = entry->start_sector.value;
	  *workbook_len = entry->size.value;
      }
    free (name);
    return FREEXL_OK;
}

static int
get_workbook_stream (biff_workbook * workbook)
{
/* attempting to locate the Workbook into the main FAT directory */
    long where;
    unsigned int sector = workbook->fat->directory_start;
    unsigned char dir_block[4096];
    int max_entries;
    int i_entry;
    unsigned char *p_entry;
    unsigned int workbook_start;
    unsigned int workbook_len;
    unsigned int miniFAT_start;
    unsigned int miniFAT_len;
    int rootEntry;
    int ret;

    if (workbook->fat->sector_size == 4096)
	max_entries = 32;
    else
	max_entries = 4;

    where = (sector + 1) * workbook->fat->sector_size;
    if (fseek (workbook->xls, where, SEEK_SET) != 0)
	return FREEXL_CFBF_SEEK_ERROR;
/* reading a FAT Directory block [sector] */
    if (fread (dir_block, 1, workbook->fat->sector_size, workbook->xls) !=
	workbook->fat->sector_size)
	return FREEXL_CFBF_READ_ERROR;
    workbook_start = 0xFFFFFFFF;
    for (i_entry = 0; i_entry < max_entries; i_entry++)
      {
	  /* scanning dir entries until Workbook found */
	  p_entry = dir_block + (i_entry * 128);
	  ret =
	      parse_dir_entry (p_entry, workbook->fat->swap,
			       workbook->utf16_converter, &workbook_start,
			       &workbook_len, &miniFAT_start, &miniFAT_len,
			       &rootEntry);
	  if (ret != FREEXL_OK)
	      return ret;
	  if (rootEntry)
	    {
		/* ok, Root Entry found */
		workbook->fat->miniFAT_start = miniFAT_start;
		workbook->fat->miniFAT_len = miniFAT_len;
	    }
	  if (workbook_start != 0xFFFFFFFF)
	    {
		/* ok, Workbook found */
		workbook->start_sector = workbook_start;
		workbook->size = workbook_len;
		workbook->current_sector = workbook_start;
		return FREEXL_OK;
	    }
      }
    return FREEXL_BIFF_WORKBOOK_NOT_FOUND;
}

static void *
create_utf16_utf8_converter (void)
{
/* creating the UTF16/UTF8 converter and returning on opaque reference to it */
    iconv_t cvt = iconv_open ("UTF-8", "UTF-16LE");
    if (cvt == (iconv_t) (-1))
	return NULL;
    return cvt;
}

static int
check_little_endian_arch ()
{
/* checking if target CPU is a little-endian one */
    biff_word32 word32;
    word32.value = 1;
    if (word32.bytes[0] == 0)
	return 1;
    return 0;
}

static int
common_open (const char *path, const void **xls_handle, int magic)
{
/* opening and initializing the Workbook */
    biff_workbook *workbook;
    biff_sheet *p_sheet;
    fat_chain *chain = NULL;
    int errcode;
    int ret;
    int swap = check_little_endian_arch ();

    *xls_handle = NULL;
/* allocating the Workbook struct */
    workbook = alloc_workbook (magic);
    if (!workbook)
	return FREEXL_INSUFFICIENT_MEMORY;
    *xls_handle = workbook;

    workbook->xls = fopen (path, "rb");
    if (workbook->xls == NULL)
	return FREEXL_FILE_NOT_FOUND;

/*
 * the XLS file is internally structured as a FAT-like
 * file-system (Compound File Binary Format, CFBF) 
 * so we'll start by parsing the FAT
 */
    chain = read_cfbf_header (workbook, swap, &errcode);
    if (!chain)
      {
	  /* it's not a CFBF file: testing older BIFF-(2,3,4) formats */
	  if (read_legacy_biff (workbook, swap))
	      return FREEXL_OK;
	  goto stop;
      }

/* transferring FAT chain ownership */
    workbook->fat = chain;
    chain = NULL;

/* creating the UTF16/UTF8 converter */
    workbook->utf16_converter = create_utf16_utf8_converter ();
    if (workbook->utf16_converter == NULL)
      {
	  errcode = FREEXL_UNSUPPORTED_CHARSET;
	  goto stop;
      }

/* we'll now retrieve the FAT main Directory */
    ret = get_workbook_stream (workbook);
    if (ret != FREEXL_OK)
      {
	  errcode = ret;
	  goto stop;
      }

/* we'll now parse the Workbook */
    if (workbook->size <= workbook->fat->miniCutOff)
      {
	  /* mini-stream stored in miniFAT */
	  int ret = read_mini_stream (workbook, &errcode);
	  if (!ret)
	      goto stop;
	  workbook->p_in = workbook->fat->miniStream;
	  while (1)
	    {
		ret = read_mini_biff_next_record (workbook, swap, &errcode);
		if (ret == -1)
		    break;	/* EOF */
		if (ret == 0)
		    goto stop;
	    }
      }
    else
      {
	  /* normal stream */
	  while (1)
	    {
		int ret = read_biff_next_record (workbook, swap, &errcode);
		if (ret == -1)
		    break;	/* EOF */
		if (ret == 0)
		    goto stop;
	    }
      }

    p_sheet = workbook->first_sheet;
    while (p_sheet)
      {
	  if (p_sheet->valid_dimension == 0)
	    {
		/* setting Sheet dimensions */
		int ret;
		p_sheet->rows += 1;
		p_sheet->columns += 1;
		ret = allocate_cells (workbook);
		if (ret != FREEXL_OK)
		    return ret;
		p_sheet->valid_dimension = 1;
		workbook->second_pass = 1;
	    }
	  else
	      p_sheet->already_done = 1;
	  p_sheet = p_sheet->next;
      }

    if (workbook->second_pass)
      {
	  /* attempting to fetch cell values performing a second pass */
	  workbook->active_sheet = NULL;
	  workbook->start_sector = 0;
	  workbook->size = 0;
	  workbook->current_sector = 0;
	  workbook->bytes_read = 0;
	  workbook->current_offset = 0;
	  workbook->p_in = workbook->sector_buf;
	  workbook->sector_end = 0;
	  workbook->sector_ready = 0;
	  workbook->ok_bof = -1;

	  ret = get_workbook_stream (workbook);
	  if (ret != FREEXL_OK)
	    {
		errcode = ret;
		goto stop;
	    }

/* we'll now parse the Workbook */
	  if (workbook->size <= workbook->fat->miniCutOff)
	    {
		/* mini-stream stored in miniFAT */
		int ret = read_mini_stream (workbook, &errcode);
		if (!ret)
		    goto stop;
		workbook->p_in = workbook->fat->miniStream;
		while (1)
		  {
		      ret =
			  read_mini_biff_next_record (workbook, swap, &errcode);
		      if (ret == -1)
			  break;	/* EOF */
		      if (ret == 0)
			  goto stop;
		  }
	    }
	  else
	    {
		/* normal stream */
		while (1)
		  {
		      int ret =
			  read_biff_next_record (workbook, swap, &errcode);
		      if (ret == -1)
			  break;	/* EOF */
		      if (ret == 0)
			  goto stop;
		  }
	    }
      }

    return FREEXL_OK;

  stop:
    if (chain)
	destroy_fat_chain (chain);
    if (workbook)
	destroy_workbook (workbook);
    *xls_handle = NULL;
    return errcode;
}

FREEXL_DECLARE int
freexl_open (const char *path, const void **xls_handle)
{
/* opening and initializing the Workbook */
    return common_open (path, xls_handle, FREEXL_MAGIC_START);
}

FREEXL_DECLARE int
freexl_open_info (const char *path, const void **xls_handle)
{
/* opening and initializing the Workbook (only for Info) */
    return common_open (path, xls_handle, FREEXL_MAGIC_INFO);
}

FREEXL_DECLARE int
freexl_close (const void *xls_handle)
{
/* attempting to destroy the Workbook */
    biff_workbook *workbook = (biff_workbook *) xls_handle;
    if (!workbook)
	return FREEXL_NULL_HANDLE;
    if ((workbook->magic1 == FREEXL_MAGIC_INFO
	 || workbook->magic1 == FREEXL_MAGIC_START)
	&& workbook->magic2 == FREEXL_MAGIC_END)
	;
    else
	return FREEXL_INVALID_HANDLE;

/* destroying the workbook */
    destroy_workbook (workbook);

    return FREEXL_OK;
}

FREEXL_DECLARE int
freexl_get_info (const void *xls_handle, unsigned short what,
		 unsigned int *info)
{
/* attempting to retrieve some info */
    biff_workbook *workbook = (biff_workbook *) xls_handle;
    if (!workbook)
	return FREEXL_NULL_HANDLE;
    if (!info)
	return FREEXL_NULL_ARGUMENT;
    if ((workbook->magic1 == FREEXL_MAGIC_INFO
	 || workbook->magic1 == FREEXL_MAGIC_START)
	&& workbook->magic2 == FREEXL_MAGIC_END)
	;
    else
	return FREEXL_INVALID_HANDLE;

    switch (what)
      {
      case FREEXL_CFBF_VERSION:
	  *info = FREEXL_UNKNOWN;
	  if (workbook->cfbf_version == 3)
	      *info = FREEXL_CFBF_VER_3;
	  if (workbook->cfbf_version == 4)
	      *info = FREEXL_CFBF_VER_4;
	  return FREEXL_OK;
      case FREEXL_CFBF_SECTOR_SIZE:
	  *info = FREEXL_UNKNOWN;
	  if (workbook->cfbf_sector_size == 512)
	      *info = FREEXL_CFBF_SECTOR_512;
	  if (workbook->cfbf_sector_size == 4096)
	      *info = FREEXL_CFBF_SECTOR_4096;
	  return FREEXL_OK;
      case FREEXL_CFBF_FAT_COUNT:
	  if (workbook->fat != NULL)
	      *info = workbook->fat->fat_array_count;
	  else
	      *info = 0;
	  return FREEXL_OK;
      case FREEXL_BIFF_MAX_RECSIZE:
	  *info = FREEXL_UNKNOWN;
	  if (workbook->biff_max_record_size == 2080)
	      *info = FREEXL_BIFF_MAX_RECSZ_2080;
	  if (workbook->biff_max_record_size == 8224)
	      *info = FREEXL_BIFF_MAX_RECSZ_8224;
	  return FREEXL_OK;
      case FREEXL_BIFF_DATEMODE:
	  *info = FREEXL_UNKNOWN;
	  if (workbook->biff_date_mode == 0)
	      *info = FREEXL_BIFF_DATEMODE_1900;
	  if (workbook->biff_date_mode == 1)
	      *info = FREEXL_BIFF_DATEMODE_1904;
	  return FREEXL_OK;
      case FREEXL_BIFF_CODEPAGE:
	  switch (workbook->biff_book_code_page)
	    {
	    case 0x016F:
		*info = FREEXL_BIFF_ASCII;
		break;
	    case 0x01B5:
		*info = FREEXL_BIFF_CP437;
		break;
	    case 0x02D0:
		*info = FREEXL_BIFF_CP720;
		break;
	    case 0x02E1:
		*info = FREEXL_BIFF_CP737;
		break;
	    case 0x0307:
		*info = FREEXL_BIFF_CP775;
		break;
	    case 0x0352:
		*info = FREEXL_BIFF_CP850;
		break;
	    case 0x0354:
		*info = FREEXL_BIFF_CP852;
		break;
	    case 0x0357:
		*info = FREEXL_BIFF_CP855;
		break;
	    case 0x0359:
		*info = FREEXL_BIFF_CP857;
		break;
	    case 0x035A:
		*info = FREEXL_BIFF_CP858;
		break;
	    case 0x035C:
		*info = FREEXL_BIFF_CP860;
		break;
	    case 0x035D:
		*info = FREEXL_BIFF_CP861;
		break;
	    case 0x035E:
		*info = FREEXL_BIFF_CP862;
		break;
	    case 0x035F:
		*info = FREEXL_BIFF_CP863;
		break;
	    case 0x0360:
		*info = FREEXL_BIFF_CP864;
		break;
	    case 0x0361:
		*info = FREEXL_BIFF_CP865;
		break;
	    case 0x0362:
		*info = FREEXL_BIFF_CP866;
		break;
	    case 0x0365:
		*info = FREEXL_BIFF_CP869;
		break;
	    case 0x036A:
		*info = FREEXL_BIFF_CP874;
		break;
	    case 0x03A4:
		*info = FREEXL_BIFF_CP932;
		break;
	    case 0x03A8:
		*info = FREEXL_BIFF_CP936;
		break;
	    case 0x03B5:
		*info = FREEXL_BIFF_CP949;
		break;
	    case 0x03B6:
		*info = FREEXL_BIFF_CP950;
		break;
	    case 0x04B0:
		*info = FREEXL_BIFF_UTF16LE;
		break;
	    case 0x04E2:
		*info = FREEXL_BIFF_CP1250;
		break;
	    case 0x04E3:
		*info = FREEXL_BIFF_CP1251;
		break;
	    case 0x04E4:
	    case 0x8001:
		*info = FREEXL_BIFF_CP1252;
		break;
	    case 0x04E5:
		*info = FREEXL_BIFF_CP1253;
		break;
	    case 0x04E6:
		*info = FREEXL_BIFF_CP1254;
		break;
	    case 0x04E7:
		*info = FREEXL_BIFF_CP1255;
		break;
	    case 0x04E8:
		*info = FREEXL_BIFF_CP1256;
		break;
	    case 0x04E9:
		*info = FREEXL_BIFF_CP1257;
		break;
	    case 0x04EA:
		*info = FREEXL_BIFF_CP1258;
		break;
	    case 0x0551:
		*info = FREEXL_BIFF_CP1361;
		break;
	    case 0x2710:
	    case 0x8000:
		*info = FREEXL_BIFF_MACROMAN;
		break;
	    default:
		*info = FREEXL_UNKNOWN;
		break;
	    };
	  return FREEXL_OK;
      case FREEXL_BIFF_VERSION:
	  *info = FREEXL_UNKNOWN;
	  if (workbook->biff_version == 2)
	      *info = FREEXL_BIFF_VER_2;
	  if (workbook->biff_version == 3)
	      *info = FREEXL_BIFF_VER_3;
	  if (workbook->biff_version == 4)
	      *info = FREEXL_BIFF_VER_4;
	  if (workbook->biff_version == 5)
	      *info = FREEXL_BIFF_VER_5;
	  if (workbook->biff_version == 8)
	      *info = FREEXL_BIFF_VER_8;
	  return FREEXL_OK;
      case FREEXL_BIFF_STRING_COUNT:
	  *info = workbook->shared_strings.string_count;
	  return FREEXL_OK;
      case FREEXL_BIFF_SHEET_COUNT:
	  *info = get_worksheet_count (workbook);
	  return FREEXL_OK;
      case FREEXL_BIFF_FORMAT_COUNT:
	  *info = workbook->max_format_index;
	  return FREEXL_OK;
      case FREEXL_BIFF_XF_COUNT:
	  *info = workbook->biff_xf_next_index;
	  return FREEXL_OK;
      case FREEXL_BIFF_PASSWORD:
	  *info = FREEXL_UNKNOWN;
	  if (workbook->biff_obfuscated == 0)
	      *info = FREEXL_BIFF_PLAIN;
	  else if (workbook->biff_obfuscated == 0)
	      *info = FREEXL_BIFF_OBFUSCATED;
	  else
	      *info = workbook->biff_xf_next_index;
	  return FREEXL_OK;
      };

    return FREEXL_INVALID_INFO_ARG;
}

FREEXL_DECLARE int
freexl_get_FAT_entry (const void *xls_handle, unsigned int sector_index,
		      unsigned int *next_sector_index)
{
/* attempting to retrieve some FAT entry [by index] */
    fat_entry *entry;
    biff_workbook *workbook = (biff_workbook *) xls_handle;
    if (!workbook)
	return FREEXL_NULL_HANDLE;
    if (!next_sector_index)
	return FREEXL_NULL_ARGUMENT;
    if ((workbook->magic1 == FREEXL_MAGIC_INFO
	 || workbook->magic1 == FREEXL_MAGIC_START)
	&& workbook->magic2 == FREEXL_MAGIC_END)
	;
    else
	return FREEXL_INVALID_HANDLE;

    if (workbook->fat == NULL)
	return FREEXL_CFBF_EMPTY_FAT_CHAIN;

    entry = get_fat_entry (workbook->fat, sector_index);
    if (entry == NULL)
	return FREEXL_CFBF_ILLEGAL_FAT_ENTRY;
    *next_sector_index = entry->next_sector;

    return FREEXL_OK;

}

FREEXL_DECLARE int
freexl_get_worksheet_name (const void *xls_handle,
			   unsigned short worksheet_index, const char **string)
{
/* attempting to retrieve some Worksheet name [by index] */
    unsigned int count = 0;
    biff_sheet *worksheet;
    biff_workbook *workbook = (biff_workbook *) xls_handle;
    if (!workbook)
	return FREEXL_NULL_HANDLE;
    if (!string)
	return FREEXL_NULL_ARGUMENT;
    if ((workbook->magic1 == FREEXL_MAGIC_INFO
	 || workbook->magic1 == FREEXL_MAGIC_START)
	&& workbook->magic2 == FREEXL_MAGIC_END)
	;
    else
	return FREEXL_INVALID_HANDLE;

    worksheet = workbook->first_sheet;
    while (worksheet)
      {
	  if (count == worksheet_index)
	    {
		*string = worksheet->utf8_name;
		return FREEXL_OK;
	    }
	  count++;
	  worksheet = worksheet->next;
      }
    return FREEXL_BIFF_ILLEGAL_SHEET_INDEX;
}

FREEXL_DECLARE int
freexl_select_active_worksheet (const void *xls_handle,
				unsigned short worksheet_index)
{
/* selecting the currently active worksheet [by index] */
    unsigned int count = 0;
    biff_sheet *worksheet;
    biff_workbook *workbook = (biff_workbook *) xls_handle;
    if (!workbook)
	return FREEXL_NULL_HANDLE;
    if ((workbook->magic1 == FREEXL_MAGIC_INFO
	 || workbook->magic1 == FREEXL_MAGIC_START)
	&& workbook->magic2 == FREEXL_MAGIC_END)
	;
    else
	return FREEXL_INVALID_HANDLE;

    worksheet = workbook->first_sheet;
    while (worksheet)
      {
	  if (count == worksheet_index)
	    {
		workbook->active_sheet = worksheet;
		return FREEXL_OK;
	    }
	  count++;
	  worksheet = worksheet->next;
      }
    return FREEXL_BIFF_ILLEGAL_SHEET_INDEX;
}

FREEXL_DECLARE int
freexl_get_active_worksheet (const void *xls_handle,
			     unsigned short *worksheet_index)
{
/* retrieving the currently active worksheet index */
    unsigned int count = 0;
    biff_sheet *worksheet;
    biff_workbook *workbook = (biff_workbook *) xls_handle;
    if (!workbook)
	return FREEXL_NULL_HANDLE;
    if (!worksheet_index)
	return FREEXL_NULL_ARGUMENT;
    if ((workbook->magic1 == FREEXL_MAGIC_INFO
	 || workbook->magic1 == FREEXL_MAGIC_START)
	&& workbook->magic2 == FREEXL_MAGIC_END)
	;
    else
	return FREEXL_INVALID_HANDLE;

    worksheet = workbook->first_sheet;
    while (worksheet)
      {
	  if (workbook->active_sheet == worksheet)
	    {
		*worksheet_index = count;
		return FREEXL_OK;
	    }
	  count++;
	  worksheet = worksheet->next;
      }
    return FREEXL_BIFF_UNSELECTED_SHEET;
}

FREEXL_DECLARE int
freexl_worksheet_dimensions (const void *xls_handle, unsigned int *rows,
			     unsigned short *columns)
{
/* dimensions: currently selected Worksheet */
    biff_workbook *workbook = (biff_workbook *) xls_handle;
    if (!workbook)
	return FREEXL_NULL_HANDLE;
    if (!rows)
	return FREEXL_NULL_ARGUMENT;
    if (!columns)
	return FREEXL_NULL_ARGUMENT;
    if ((workbook->magic1 == FREEXL_MAGIC_INFO
	 || workbook->magic1 == FREEXL_MAGIC_START)
	&& workbook->magic2 == FREEXL_MAGIC_END)
	;
    else
	return FREEXL_INVALID_HANDLE;

    if (workbook->active_sheet == NULL)
	return FREEXL_BIFF_UNSELECTED_SHEET;

    *rows = workbook->active_sheet->rows;
    *columns = workbook->active_sheet->columns;
    return FREEXL_OK;
}

FREEXL_DECLARE int
freexl_get_SST_string (const void *xls_handle, unsigned short string_index,
		       const char **string)
{
/* attempting to retrieve some SST entry [by index] */
    biff_workbook *workbook = (biff_workbook *) xls_handle;
    if (!workbook)
	return FREEXL_NULL_HANDLE;
    if (!string)
	return FREEXL_NULL_ARGUMENT;
    if (workbook->magic1 == FREEXL_MAGIC_START
	&& workbook->magic2 == FREEXL_MAGIC_END)
	;
    else
	return FREEXL_INVALID_HANDLE;

    *string = NULL;
    if (workbook->shared_strings.utf8_strings == NULL)
	return FREEXL_BIFF_INVALID_SST;
    if (string_index < workbook->shared_strings.string_count)
      {
	  *string = *(workbook->shared_strings.utf8_strings + string_index);
	  return FREEXL_OK;
      }
    return FREEXL_BIFF_ILLEGAL_SST_INDEX;
}

FREEXL_DECLARE int
freexl_get_cell_value (const void *xls_handle, unsigned int row,
		       unsigned short column, FreeXL_CellValue * val)
{
/* attempting to fetch a cell value */
    biff_cell_value *p_cell;
    biff_workbook *workbook = (biff_workbook *) xls_handle;
    if (!workbook)
	return FREEXL_NULL_HANDLE;
    if (!val)
	return FREEXL_NULL_ARGUMENT;
    if (workbook->magic1 == FREEXL_MAGIC_START
	&& workbook->magic2 == FREEXL_MAGIC_END)
	;
    else
	return FREEXL_INVALID_HANDLE;

    if (row >= workbook->active_sheet->rows
	|| column >= workbook->active_sheet->columns)
	return FREEXL_ILLEGAL_CELL_ROW_COL;
    if (workbook->active_sheet->cell_values == NULL)
	return FREEXL_ILLEGAL_CELL_ROW_COL;

    p_cell =
	workbook->active_sheet->cell_values +
	(row * workbook->active_sheet->columns) + column;
/* 
/ kindly contributed by Brad Hards: 2011-09-03
/ this function now return the Cell Value using the
/ FreeXL_CellValue multi-type container 
*/
    val->type = p_cell->type;
    switch (p_cell->type)
      {
      case FREEXL_CELL_INT:
	  val->value.int_value = p_cell->value.int_value;
	  break;
      case FREEXL_CELL_DOUBLE:
	  val->value.double_value = p_cell->value.dbl_value;
	  break;
      case FREEXL_CELL_DATE:
      case FREEXL_CELL_DATETIME:
      case FREEXL_CELL_TIME:
      case FREEXL_CELL_TEXT:
	  val->value.text_value = p_cell->value.text_value;
	  break;
      case FREEXL_CELL_SST_TEXT:
	  val->value.text_value = p_cell->value.sst_value;
	  break;
      };

    return FREEXL_OK;
}
