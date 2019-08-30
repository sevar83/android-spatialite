/**********************************************************************
 *
 * rttopo - topology library
 * http://git.osgeo.org/gitea/rttopo/librttopo
 *
 * rttopo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * rttopo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with rttopo.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 **********************************************************************/



#include "rttopo_config.h"
#include <ctype.h>

#include "librttopo_geom_internal.h"

/* Structure for the type array */
struct geomtype_struct
{
  char *typename;
  int type;
  int z;
  int m;
};

/* Type array. Note that the order of this array is important in
   that any typename in the list must *NOT* occur within an entry
   before it. Otherwise if we search for "POINT" at the top of the
   list we would also match MULTIPOINT, for example. */

struct geomtype_struct geomtype_struct_array[] =
{
  { "GEOMETRYCOLLECTIONZM", RTCOLLECTIONTYPE, 1, 1 },
  { "GEOMETRYCOLLECTIONZ", RTCOLLECTIONTYPE, 1, 0 },
  { "GEOMETRYCOLLECTIONM", RTCOLLECTIONTYPE, 0, 1 },
  { "GEOMETRYCOLLECTION", RTCOLLECTIONTYPE, 0, 0 },

  { "GEOMETRYZM", 0, 1, 1 },
  { "GEOMETRYZ", 0, 1, 0 },
  { "GEOMETRYM", 0, 0, 1 },
  { "GEOMETRY", 0, 0, 0 },

  { "POLYHEDRALSURFACEZM", RTPOLYHEDRALSURFACETYPE, 1, 1 },
  { "POLYHEDRALSURFACEZ", RTPOLYHEDRALSURFACETYPE, 1, 0 },
  { "POLYHEDRALSURFACEM", RTPOLYHEDRALSURFACETYPE, 0, 1 },
  { "POLYHEDRALSURFACE", RTPOLYHEDRALSURFACETYPE, 0, 0 },

  { "TINZM", RTTINTYPE, 1, 1 },
  { "TINZ", RTTINTYPE, 1, 0 },
  { "TINM", RTTINTYPE, 0, 1 },
  { "TIN", RTTINTYPE, 0, 0 },

  { "CIRCULARSTRINGZM", RTCIRCSTRINGTYPE, 1, 1 },
  { "CIRCULARSTRINGZ", RTCIRCSTRINGTYPE, 1, 0 },
  { "CIRCULARSTRINGM", RTCIRCSTRINGTYPE, 0, 1 },
  { "CIRCULARSTRING", RTCIRCSTRINGTYPE, 0, 0 },

  { "COMPOUNDCURVEZM", RTCOMPOUNDTYPE, 1, 1 },
  { "COMPOUNDCURVEZ", RTCOMPOUNDTYPE, 1, 0 },
  { "COMPOUNDCURVEM", RTCOMPOUNDTYPE, 0, 1 },
  { "COMPOUNDCURVE", RTCOMPOUNDTYPE, 0, 0 },

  { "CURVEPOLYGONZM", RTCURVEPOLYTYPE, 1, 1 },
  { "CURVEPOLYGONZ", RTCURVEPOLYTYPE, 1, 0 },
  { "CURVEPOLYGONM", RTCURVEPOLYTYPE, 0, 1 },
  { "CURVEPOLYGON", RTCURVEPOLYTYPE, 0, 0 },

  { "MULTICURVEZM", RTMULTICURVETYPE, 1, 1 },
  { "MULTICURVEZ", RTMULTICURVETYPE, 1, 0 },
  { "MULTICURVEM", RTMULTICURVETYPE, 0, 1 },
  { "MULTICURVE", RTMULTICURVETYPE, 0, 0 },

  { "MULTISURFACEZM", RTMULTISURFACETYPE, 1, 1 },
  { "MULTISURFACEZ", RTMULTISURFACETYPE, 1, 0 },
  { "MULTISURFACEM", RTMULTISURFACETYPE, 0, 1 },
  { "MULTISURFACE", RTMULTISURFACETYPE, 0, 0 },

  { "MULTILINESTRINGZM", RTMULTILINETYPE, 1, 1 },
  { "MULTILINESTRINGZ", RTMULTILINETYPE, 1, 0 },
  { "MULTILINESTRINGM", RTMULTILINETYPE, 0, 1 },
  { "MULTILINESTRING", RTMULTILINETYPE, 0, 0 },

  { "MULTIPOLYGONZM", RTMULTIPOLYGONTYPE, 1, 1 },
  { "MULTIPOLYGONZ", RTMULTIPOLYGONTYPE, 1, 0 },
  { "MULTIPOLYGONM", RTMULTIPOLYGONTYPE, 0, 1 },
  { "MULTIPOLYGON", RTMULTIPOLYGONTYPE, 0, 0 },

  { "MULTIPOINTZM", RTMULTIPOINTTYPE, 1, 1 },
  { "MULTIPOINTZ", RTMULTIPOINTTYPE, 1, 0 },
  { "MULTIPOINTM", RTMULTIPOINTTYPE, 0, 1 },
  { "MULTIPOINT", RTMULTIPOINTTYPE, 0, 0 },

  { "LINESTRINGZM", RTLINETYPE, 1, 1 },
  { "LINESTRINGZ", RTLINETYPE, 1, 0 },
  { "LINESTRINGM", RTLINETYPE, 0, 1 },
  { "LINESTRING", RTLINETYPE, 0, 0 },

  { "TRIANGLEZM", RTTRIANGLETYPE, 1, 1 },
  { "TRIANGLEZ", RTTRIANGLETYPE, 1, 0 },
  { "TRIANGLEM", RTTRIANGLETYPE, 0, 1 },
  { "TRIANGLE", RTTRIANGLETYPE, 0, 0 },

  { "POLYGONZM", RTPOLYGONTYPE, 1, 1 },
  { "POLYGONZ", RTPOLYGONTYPE, 1, 0 },
  { "POLYGONM", RTPOLYGONTYPE, 0, 1 },
  { "POLYGON", RTPOLYGONTYPE, 0, 0 },

  { "POINTZM", RTPOINTTYPE, 1, 1 },
  { "POINTZ", RTPOINTTYPE, 1, 0 },
  { "POINTM", RTPOINTTYPE, 0, 1 },
  { "POINT", RTPOINTTYPE, 0, 0 }

};
#define GEOMTYPE_STRUCT_ARRAY_LEN (sizeof geomtype_struct_array/sizeof(struct geomtype_struct))

/*
* We use a very simple upper case mapper here, because the system toupper() function
* is locale dependent and may have trouble mapping lower case strings to the upper
* case ones we expect (see, the "Turkisk I", http://www.i18nguy.com/unicode/turkish-i18n.html)
* We could also count on PgSQL sending us *lower* case inputs, as it seems to do that
* regardless of the case the user provides for the type arguments.
*/
const char dumb_upper_map[128] = "................................................0123456789.......ABCDEFGHIJKLMNOPQRSTUVWXYZ......ABCDEFGHIJKLMNOPQRSTUVWXYZ.....";

static char dump_toupper(const RTCTX *ctx, int in)
{
  if ( in < 0 || in > 127 )
    return '.';
  return dumb_upper_map[in];
}

uint8_t gflags(const RTCTX *ctx, int hasz, int hasm, int geodetic)
{
  uint8_t flags = 0;
  if ( hasz )
    RTFLAGS_SET_Z(flags, 1);
  if ( hasm )
    RTFLAGS_SET_M(flags, 1);
  if ( geodetic )
    RTFLAGS_SET_GEODETIC(flags, 1);
  return flags;
}

/**
* Calculate type integer and dimensional flags from string input.
* Case insensitive, and insensitive to spaces at front and back.
* Type == 0 in the case of the string "GEOMETRY" or "GEOGRAPHY".
* Return RT_SUCCESS for success.
*/
int geometry_type_from_string(const RTCTX *ctx, const char *str, uint8_t *type, int *z, int *m)
{
  char *tmpstr;
  int tmpstartpos, tmpendpos;
  int i;

  assert(str);
  assert(type);
  assert(z);
  assert(m);

  /* Initialize. */
  *type = 0;
  *z = 0;
  *m = 0;

  /* Locate any leading/trailing spaces */
  tmpstartpos = 0;
  for (i = 0; i < strlen(str); i++)
  {
    if (str[i] != ' ')
    {
      tmpstartpos = i;
      break;
    }
  }

  tmpendpos = strlen(str) - 1;
  for (i = strlen(str) - 1; i >= 0; i--)
  {
    if (str[i] != ' ')
    {
      tmpendpos = i;
      break;
    }
  }

  /* Copy and convert to upper case for comparison */
  tmpstr = rtalloc(ctx, tmpendpos - tmpstartpos + 2);
  for (i = tmpstartpos; i <= tmpendpos; i++)
    tmpstr[i - tmpstartpos] = dump_toupper(ctx, str[i]);

  /* Add NULL to terminate */
  tmpstr[i - tmpstartpos] = '\0';

  /* Now check for the type */
  for (i = 0; i < GEOMTYPE_STRUCT_ARRAY_LEN; i++)
  {
    if (!strcmp(tmpstr, geomtype_struct_array[i].typename))
    {
      *type = geomtype_struct_array[i].type;
      *z = geomtype_struct_array[i].z;
      *m = geomtype_struct_array[i].m;

      rtfree(ctx, tmpstr);

      return RT_SUCCESS;
    }

  }

  rtfree(ctx, tmpstr);

  return RT_FAILURE;
}




