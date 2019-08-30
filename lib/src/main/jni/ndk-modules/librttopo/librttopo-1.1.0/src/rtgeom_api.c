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
 * Copyright 2001-2006 Refractions Research Inc.
 *
 **********************************************************************/

#include "rttopo_config.h"
#include "librttopo_geom_internal.h"
#include "rtgeom_log.h"

#include <stdio.h>
#include <errno.h>
#include <assert.h>

/*
 * Lower this to reduce integrity checks
 */
#define PARANOIA_LEVEL 1


const char *
rtgeom_version()
{
  static char *ptr = NULL;
  static char buf[256];
  if ( ! ptr )
  {
    ptr = buf;
    snprintf(ptr, 256, LIBRTGEOM_VERSION);
  }

  return ptr;
}


/**********************************************************************
 * BOX routines
 *
 * returns the float thats very close to the input, but <=
 *  handles the funny differences in float4 and float8 reps.
 **********************************************************************/

typedef union
{
  float value;
  uint32_t word;
} ieee_float_shape_type;

#define GET_FLOAT_WORD(i,d)      \
  do {          \
    ieee_float_shape_type gf_u;  \
    gf_u.value = (d);    \
    (i) = gf_u.word;    \
  } while (0)


#define SET_FLOAT_WORD(d,i)      \
  do {          \
    ieee_float_shape_type sf_u;  \
    sf_u.word = (i);    \
    (d) = sf_u.value;    \
  } while (0)


/*
 * Returns the next smaller or next larger float
 * from x (in direction of y).
 */
static float
nextafterf_custom(const RTCTX *ctx, float x, float y)
{
  int hx,hy,ix,iy;

  GET_FLOAT_WORD(hx,x);
  GET_FLOAT_WORD(hy,y);
  ix = hx&0x7fffffff;             /* |x| */
  iy = hy&0x7fffffff;             /* |y| */

  if ((ix>0x7f800000) ||   /* x is nan */
          (iy>0x7f800000))     /* y is nan */
    return x+y;
  if (x==y) return y;              /* x=y, return y */
  if (ix==0)
  {
    /* x == 0 */
    SET_FLOAT_WORD(x,(hy&0x80000000)|1);/* return +-minsubnormal */
    y = x*x;
    if (y==x) return y;
    else return x;   /* raise underflow flag */
  }
  if (hx>=0)
  {
    /* x > 0 */
    if (hx>hy)
    {
      /* x > y, x -= ulp */
      hx -= 1;
    }
    else
    {
      /* x < y, x += ulp */
      hx += 1;
    }
  }
  else
  {
    /* x < 0 */
    if (hy>=0||hx>hy)
    {
      /* x < y, x -= ulp */
      hx -= 1;
    }
    else
    {
      /* x > y, x += ulp */
      hx += 1;
    }
  }
  hy = hx&0x7f800000;
  if (hy>=0x7f800000) return x+x;  /* overflow  */
  if (hy<0x00800000)
  {
    /* underflow */
    y = x*x;
    if (y!=x)
    {
      /* raise underflow flag */
      SET_FLOAT_WORD(y,hx);
      return y;
    }
  }
  SET_FLOAT_WORD(x,hx);
  return x;
}


float next_float_down(const RTCTX *ctx, double d)
{
  float result  = d;

  if ( ((double) result) <=d)
    return result;

  return nextafterf_custom(ctx, result, result - 1000000);

}

/*
 * Returns the float thats very close to the input, but >=.
 * handles the funny differences in float4 and float8 reps.
 */
float
next_float_up(const RTCTX *ctx, double d)
{
  float result  = d;

  if ( ((double) result) >=d)
    return result;

  return nextafterf_custom(ctx, result, result + 1000000);
}


/*
 * Returns the double thats very close to the input, but <.
 * handles the funny differences in float4 and float8 reps.
 */
double
next_double_down(const RTCTX *ctx, float d)
{
  double result  = d;

  if ( result < d)
    return result;

  return nextafterf_custom(ctx, result, result - 1000000);
}

/*
 * Returns the double thats very close to the input, but >
 * handles the funny differences in float4 and float8 reps.
 */
double
next_double_up(const RTCTX *ctx, float d)
{
  double result  = d;

  if ( result > d)
    return result;

  return nextafterf_custom(ctx, result, result + 1000000);
}


/************************************************************************
 * RTPOINTARRAY support functions
 *
 * TODO: should be moved to ptarray.c probably
 *
 ************************************************************************/

/*
 * Copies a point from the point array into the parameter point
 * will set point's z=NO_Z_VALUE if pa is 2d
 * will set point's m=NO_M_VALUE if pa is 3d or 2d
 *
 * NOTE: point is a real POINT3D *not* a pointer
 */
RTPOINT4D
rt_getPoint4d(const RTCTX *ctx, const RTPOINTARRAY *pa, int n)
{
  RTPOINT4D result;
  rt_getPoint4d_p(ctx, pa, n, &result);
  return result;
}

/*
 * Copies a point from the point array into the parameter point
 * will set point's z=NO_Z_VALUE  if pa is 2d
 * will set point's m=NO_M_VALUE  if pa is 3d or 2d
 *
 * NOTE: this will modify the point4d pointed to by 'point'.
 *
 * @return 0 on error, 1 on success
 */
int
rt_getPoint4d_p(const RTCTX *ctx, const RTPOINTARRAY *pa, int n, RTPOINT4D *op)
{
  uint8_t *ptr;
  int zmflag;

#if PARANOIA_LEVEL > 0
  if ( ! pa ) rterror(ctx, "rt_getPoint4d_p: NULL pointarray");

  if ( (n<0) || (n>=pa->npoints))
  {
    rterror(ctx, "rt_getPoint4d_p: point offset out of range");
    return 0;
  }
#endif

  RTDEBUG(ctx, 4, "rt_getPoint4d_p called.");

  /* Get a pointer to nth point offset and zmflag */
  ptr=rt_getPoint_internal(ctx, pa, n);
  zmflag=RTFLAGS_GET_ZM(pa->flags);

  RTDEBUGF(ctx, 4, "ptr %p, zmflag %d", ptr, zmflag);

  switch (zmflag)
  {
  case 0: /* 2d  */
    memcpy(op, ptr, sizeof(RTPOINT2D));
    op->m=NO_M_VALUE;
    op->z=NO_Z_VALUE;
    break;

  case 3: /* ZM */
    memcpy(op, ptr, sizeof(RTPOINT4D));
    break;

  case 2: /* Z */
    memcpy(op, ptr, sizeof(RTPOINT3DZ));
    op->m=NO_M_VALUE;
    break;

  case 1: /* M */
    memcpy(op, ptr, sizeof(RTPOINT3DM));
    op->m=op->z; /* we use Z as temporary storage */
    op->z=NO_Z_VALUE;
    break;

  default:
    rterror(ctx, "Unknown ZM flag ??");
    return 0;
  }
  return 1;

}



/*
 * Copy a point from the point array into the parameter point
 * will set point's z=NO_Z_VALUE if pa is 2d
 * NOTE: point is a real RTPOINT3DZ *not* a pointer
 */
RTPOINT3DZ
rt_getPoint3dz(const RTCTX *ctx, const RTPOINTARRAY *pa, int n)
{
  RTPOINT3DZ result;
  rt_getPoint3dz_p(ctx, pa, n, &result);
  return result;
}

/*
 * Copy a point from the point array into the parameter point
 * will set point's z=NO_Z_VALUE if pa is 2d
 *
 * NOTE: point is a real RTPOINT3DZ *not* a pointer
 */
RTPOINT3DM
rt_getPoint3dm(const RTCTX *ctx, const RTPOINTARRAY *pa, int n)
{
  RTPOINT3DM result;
  rt_getPoint3dm_p(ctx, pa, n, &result);
  return result;
}

/*
 * Copy a point from the point array into the parameter point
 * will set point's z=NO_Z_VALUE if pa is 2d
 *
 * NOTE: this will modify the point3dz pointed to by 'point'.
 */
int
rt_getPoint3dz_p(const RTCTX *ctx, const RTPOINTARRAY *pa, int n, RTPOINT3DZ *op)
{
  uint8_t *ptr;

#if PARANOIA_LEVEL > 0
  if ( ! pa ) return 0;

  if ( (n<0) || (n>=pa->npoints))
  {
    RTDEBUGF(ctx, 4, "%d out of numpoint range (%d)", n, pa->npoints);
    return 0; /*error */
  }
#endif

  RTDEBUGF(ctx, 2, "rt_getPoint3dz_p called on array of %d-dimensions / %u pts",
           RTFLAGS_NDIMS(pa->flags), pa->npoints);

  /* Get a pointer to nth point offset */
  ptr=rt_getPoint_internal(ctx, pa, n);

  /*
   * if input RTPOINTARRAY has the Z, it is artays
   * at third position so make a single copy
   */
  if ( RTFLAGS_GET_Z(pa->flags) )
  {
    memcpy(op, ptr, sizeof(RTPOINT3DZ));
  }

  /*
   * Otherwise copy the 2d part and initialize
   * Z to NO_Z_VALUE
   */
  else
  {
    memcpy(op, ptr, sizeof(RTPOINT2D));
    op->z=NO_Z_VALUE;
  }

  return 1;

}

/*
 * Copy a point from the point array into the parameter point
 * will set point's m=NO_Z_VALUE if pa has no M
 *
 * NOTE: this will modify the point3dm pointed to by 'point'.
 */
int
rt_getPoint3dm_p(const RTCTX *ctx, const RTPOINTARRAY *pa, int n, RTPOINT3DM *op)
{
  uint8_t *ptr;
  int zmflag;

#if PARANOIA_LEVEL > 0
  if ( ! pa ) return 0;

  if ( (n<0) || (n>=pa->npoints))
  {
    rterror(ctx, "%d out of numpoint range (%d)", n, pa->npoints);
    return 0; /*error */
  }
#endif

  RTDEBUGF(ctx, 2, "rt_getPoint3dm_p(ctx, %d) called on array of %d-dimensions / %u pts",
           n, RTFLAGS_NDIMS(pa->flags), pa->npoints);


  /* Get a pointer to nth point offset and zmflag */
  ptr=rt_getPoint_internal(ctx, pa, n);
  zmflag=RTFLAGS_GET_ZM(pa->flags);

  /*
   * if input RTPOINTARRAY has the M and NO Z,
   * we can issue a single memcpy
   */
  if ( zmflag == 1 )
  {
    memcpy(op, ptr, sizeof(RTPOINT3DM));
    return 1;
  }

  /*
   * Otherwise copy the 2d part and
   * initialize M to NO_M_VALUE
   */
  memcpy(op, ptr, sizeof(RTPOINT2D));

  /*
   * Then, if input has Z skip it and
   * copy next double, otherwise initialize
   * M to NO_M_VALUE
   */
  if ( zmflag == 3 )
  {
    ptr+=sizeof(RTPOINT3DZ);
    memcpy(&(op->m), ptr, sizeof(double));
  }
  else
  {
    op->m=NO_M_VALUE;
  }

  return 1;
}


/*
 * Copy a point from the point array into the parameter point
 * z value (if present) is not returned.
 *
 * NOTE: point is a real RTPOINT2D *not* a pointer
 */
RTPOINT2D
rt_getPoint2d(const RTCTX *ctx, const RTPOINTARRAY *pa, int n)
{
  const RTPOINT2D *result;
  result = rt_getPoint2d_cp(ctx, pa, n);
  return *result;
}

/*
 * Copy a point from the point array into the parameter point
 * z value (if present) is not returned.
 *
 * NOTE: this will modify the point2d pointed to by 'point'.
 */
int
rt_getPoint2d_p(const RTCTX *ctx, const RTPOINTARRAY *pa, int n, RTPOINT2D *point)
{
#if PARANOIA_LEVEL > 0
  if ( ! pa ) return 0;

  if ( (n<0) || (n>=pa->npoints))
  {
    rterror(ctx, "rt_getPoint2d_p: point offset out of range");
    return 0; /*error */
  }
#endif

  /* this does x,y */
  memcpy(point, rt_getPoint_internal(ctx, pa, n), sizeof(RTPOINT2D));
  return 1;
}

/**
* Returns a pointer into the RTPOINTARRAY serialized_ptlist,
* suitable for reading from. This is very high performance
* and declared const because you aren't allowed to muck with the
* values, only read them.
*/
const RTPOINT2D*
rt_getPoint2d_cp(const RTCTX *ctx, const RTPOINTARRAY *pa, int n)
{
  if ( ! pa ) return 0;

  if ( (n<0) || (n>=pa->npoints))
  {
    rterror(ctx, "rt_getPoint2D_const_p: point offset out of range");
    return 0; /*error */
  }

  return (const RTPOINT2D*)rt_getPoint_internal(ctx, pa, n);
}

const RTPOINT3DZ*
rt_getPoint3dz_cp(const RTCTX *ctx, const RTPOINTARRAY *pa, int n)
{
  if ( ! pa ) return 0;

  if ( ! RTFLAGS_GET_Z(pa->flags) )
  {
    rterror(ctx, "rt_getPoint3dz_cp: no Z coordinates in point array");
    return 0; /*error */
  }

  if ( (n<0) || (n>=pa->npoints))
  {
    rterror(ctx, "rt_getPoint3dz_cp: point offset out of range");
    return 0; /*error */
  }

  return (const RTPOINT3DZ*)rt_getPoint_internal(ctx, pa, n);
}


const RTPOINT4D*
rt_getPoint4d_cp(const RTCTX *ctx, const RTPOINTARRAY *pa, int n)
{
  if ( ! pa ) return 0;

  if ( ! (RTFLAGS_GET_Z(pa->flags) && RTFLAGS_GET_Z(pa->flags)) )
  {
    rterror(ctx, "rt_getPoint3dz_cp: no Z and M coordinates in point array");
    return 0; /*error */
  }

  if ( (n<0) || (n>=pa->npoints))
  {
    rterror(ctx, "rt_getPoint3dz_cp: point offset out of range");
    return 0; /*error */
  }

  return (const RTPOINT4D*)rt_getPoint_internal(ctx, pa, n);
}



/*
 * set point N to the given value
 * NOTE that the pointarray can be of any
 * dimension, the appropriate ordinate values
 * will be extracted from it
 *
 */
void
ptarray_set_point4d(const RTCTX *ctx, RTPOINTARRAY *pa, int n, const RTPOINT4D *p4d)
{
  uint8_t *ptr;
  assert(n >= 0 && n < pa->npoints);
  ptr=rt_getPoint_internal(ctx, pa, n);
  switch ( RTFLAGS_GET_ZM(pa->flags) )
  {
  case 3:
    memcpy(ptr, p4d, sizeof(RTPOINT4D));
    break;
  case 2:
    memcpy(ptr, p4d, sizeof(RTPOINT3DZ));
    break;
  case 1:
    memcpy(ptr, p4d, sizeof(RTPOINT2D));
    ptr+=sizeof(RTPOINT2D);
    memcpy(ptr, &(p4d->m), sizeof(double));
    break;
  case 0:
    memcpy(ptr, p4d, sizeof(RTPOINT2D));
    break;
  }
}




/*****************************************************************************
 * Basic sub-geometry types
 *****************************************************************************/

/* handle missaligned uint32_t32 data */
uint32_t
rt_get_uint32_t(const RTCTX *ctx, const uint8_t *loc)
{
  uint32_t result;

  memcpy(&result, loc, sizeof(uint32_t));
  return result;
}

/* handle missaligned signed int32_t data */
int32_t
rt_get_int32_t(const RTCTX *ctx, const uint8_t *loc)
{
  int32_t result;

  memcpy(&result,loc, sizeof(int32_t));
  return result;
}


/************************************************
 * debugging routines
 ************************************************/

void printBOX3D(const RTCTX *ctx, BOX3D *box)
{
  rtnotice(ctx, "BOX3D: %g %g, %g %g", box->xmin, box->ymin,
           box->xmax, box->ymax);
}

void printPA(const RTCTX *ctx, RTPOINTARRAY *pa)
{
  int t;
  RTPOINT4D pt;
  char *mflag;


  if ( RTFLAGS_GET_M(pa->flags) ) mflag = "M";
  else mflag = "";

  rtnotice(ctx, "      RTPOINTARRAY%s{", mflag);
  rtnotice(ctx, "                 ndims=%i,   ptsize=%i",
           RTFLAGS_NDIMS(pa->flags), ptarray_point_size(ctx, pa));
  rtnotice(ctx, "                 npoints = %i", pa->npoints);

  for (t =0; t<pa->npoints; t++)
  {
    rt_getPoint4d_p(ctx, pa, t, &pt);
    if (RTFLAGS_NDIMS(pa->flags) == 2)
    {
      rtnotice(ctx, "                    %i : %lf,%lf",t,pt.x,pt.y);
    }
    if (RTFLAGS_NDIMS(pa->flags) == 3)
    {
      rtnotice(ctx, "                    %i : %lf,%lf,%lf",t,pt.x,pt.y,pt.z);
    }
    if (RTFLAGS_NDIMS(pa->flags) == 4)
    {
      rtnotice(ctx, "                    %i : %lf,%lf,%lf,%lf",t,pt.x,pt.y,pt.z,pt.m);
    }
  }

  rtnotice(ctx, "      }");
}


/**
 * Given a string with at least 2 chars in it, convert them to
 * a byte value.  No error checking done!
 */
uint8_t
parse_hex(const RTCTX *ctx, char *str)
{
  /* do this a little brute force to make it faster */

  uint8_t    result_high = 0;
  uint8_t    result_low = 0;

  switch (str[0])
  {
  case '0' :
    result_high = 0;
    break;
  case '1' :
    result_high = 1;
    break;
  case '2' :
    result_high = 2;
    break;
  case '3' :
    result_high = 3;
    break;
  case '4' :
    result_high = 4;
    break;
  case '5' :
    result_high = 5;
    break;
  case '6' :
    result_high = 6;
    break;
  case '7' :
    result_high = 7;
    break;
  case '8' :
    result_high = 8;
    break;
  case '9' :
    result_high = 9;
    break;
  case 'A' :
  case 'a' :
    result_high = 10;
    break;
  case 'B' :
  case 'b' :
    result_high = 11;
    break;
  case 'C' :
  case 'c' :
    result_high = 12;
    break;
  case 'D' :
  case 'd' :
    result_high = 13;
    break;
  case 'E' :
  case 'e' :
    result_high = 14;
    break;
  case 'F' :
  case 'f' :
    result_high = 15;
    break;
  }
  switch (str[1])
  {
  case '0' :
    result_low = 0;
    break;
  case '1' :
    result_low = 1;
    break;
  case '2' :
    result_low = 2;
    break;
  case '3' :
    result_low = 3;
    break;
  case '4' :
    result_low = 4;
    break;
  case '5' :
    result_low = 5;
    break;
  case '6' :
    result_low = 6;
    break;
  case '7' :
    result_low = 7;
    break;
  case '8' :
    result_low = 8;
    break;
  case '9' :
    result_low = 9;
    break;
  case 'A' :
  case 'a' :
    result_low = 10;
    break;
  case 'B' :
  case 'b' :
    result_low = 11;
    break;
  case 'C' :
  case 'c' :
    result_low = 12;
    break;
  case 'D' :
  case 'd' :
    result_low = 13;
    break;
  case 'E' :
  case 'e' :
    result_low = 14;
    break;
  case 'F' :
  case 'f' :
    result_low = 15;
    break;
  }
  return (uint8_t) ((result_high<<4) + result_low);
}


/**
 * Given one byte, populate result with two byte representing
 * the hex number.
 *
 * Ie. deparse_hex(ctx,  255, mystr)
 *    -> mystr[0] = 'F' and mystr[1] = 'F'
 *
 * No error checking done
 */
void
deparse_hex(const RTCTX *ctx, uint8_t str, char *result)
{
  int  input_high;
  int  input_low;
  static char outchr[]=
  {
    "0123456789ABCDEF"
  };

  input_high = (str>>4);
  input_low = (str & 0x0F);

  result[0] = outchr[input_high];
  result[1] = outchr[input_low];

}


/**
 * Find interpolation point I
 * between point A and point B
 * so that the len(AI) == len(AB)*F
 * and I falls on AB segment.
 *
 * Example:
 *
 *   F=0.5  :    A----I----B
 *   F=1    :    A---------B==I
 *   F=0    : A==I---------B
 *   F=.2   :    A-I-------B
 */
void
interpolate_point4d(const RTCTX *ctx, RTPOINT4D *A, RTPOINT4D *B, RTPOINT4D *I, double F)
{
#if PARANOIA_LEVEL > 0
  double absF=fabs(F);
  if ( absF < 0 || absF > 1 )
  {
    rterror(ctx, "interpolate_point4d: invalid F (%g)", F);
  }
#endif
  I->x=A->x+((B->x-A->x)*F);
  I->y=A->y+((B->y-A->y)*F);
  I->z=A->z+((B->z-A->z)*F);
  I->m=A->m+((B->m-A->m)*F);
}


int _rtgeom_interrupt_requested = 0;
void
rtgeom_request_interrupt(const RTCTX *ctx) {
  _rtgeom_interrupt_requested = 1;
}
void
rtgeom_cancel_interrupt(const RTCTX *ctx) {
  _rtgeom_interrupt_requested = 0;
}

rtinterrupt_callback *_rtgeom_interrupt_callback = 0;
rtinterrupt_callback *
rtgeom_register_interrupt_callback(const RTCTX *ctx, rtinterrupt_callback *cb) {
  rtinterrupt_callback *old = _rtgeom_interrupt_callback;
  _rtgeom_interrupt_callback = cb;
  return old;
}
