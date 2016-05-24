/******************************************************************************
 * $Id: jniproj.c 2095 2011-09-02 08:44:02Z desruisseaux $
 *
 * Project:  PROJ.4
 * Purpose:  Java/JNI wrappers for PROJ.4 API.
 * Author:   Martin Desruisseaux
 *
 *****************************************************************************/

/*!
 * \file jniproj.c
 *
 * \brief
 * Functions used by the Java Native Interface (JNI) wrappers of Proj.4
 *
 * \author Martin Desruisseaux
 * \date   August 2011
 */

#include "proj_config.h"

#ifdef JNI_ENABLED

#include <math.h>
#include <string.h>

#define PJ_LIB__
#include "projects.h"
#include "org_proj4_PJ.h"
#include "org_proj4_GridInfo.h"
#include <jni.h>

#define PJ_FIELD_NAME "ptr"
#define PJ_FIELD_TYPE "J"
#define PJ_MAX_DIMENSION 100
/* The PJ_MAX_DIMENSION value appears also in quoted strings.
   Please perform a search-and-replace if this value is changed. */

#define GRIDINFO_FIELD_GRIDNAME "gridName"
#define GRIDINFO_FIELD_FILENAME "fileName"
#define GRIDINFO_FIELD_FORMAT "format"
#define GRIDINFO_FIELD_GRIDOFFSET "gridOffset"
#define GRIDINFO_FIELD_CTABLE "ctable"
#define GRIDINFO_FIELD_NEXT "next"
#define GRIDINFO_FIELD_CHILD "child"

#define CTABLE_FIELD_ID "id"
#define CTABLE_FIELD_ORIGINLAMBDA "originLambda"
#define CTABLE_FIELD_ORIGINPHI "originPhi"
#define CTABLE_FIELD_CELLLAMBDA "cellLambda"
#define CTABLE_FIELD_CELLPHI "cellPhi"
#define CTABLE_FIELD_SIZELAMBDA "sizeLambda"
#define CTABLE_FIELD_SIZEPHI "sizePhi"

PJ_CVSID("$Id: jniproj.c 2095 2011-09-02 08:44:02Z desruisseaux $");

/*!
 * \brief
 * Assigns the Proj4 data path (in PROJ_LIB env. variable) where all init and
 * grid files are searched for.
 * This method should be called before attempting to construct a PJ instance from init or
 * grid files (like "+init=epsg:4236", "+datum=NAD83", "+ellps=clrk66", "+nadgrids=@ntv2_0.gsb", etc.)
 * For Android: Usually this directory is /data/data/<app_package_name>
 *
 * \param  env        	 - The JNI environment.
 * \param  class      	 - The class from which this method has been invoked.
 * \param  dataPath 	 - String with the absolute path where Proj4 will search for data files
 * \author Svetlozar Kostadinov <sevarbg@gmail.com>
 */
JNIEXPORT void JNICALL Java_org_proj4_PJ_setDataPath
  (JNIEnv *env, jclass class, jstring dataPath)
{
    const char *datadir_utf = (*env)->GetStringUTFChars(env, dataPath, NULL);
    if (!datadir_utf) return; /* OutOfMemoryError already thrown. */
    setenv("PROJ_LIB", datadir_utf, 1);
    (*env)->ReleaseStringUTFChars(env, dataPath, datadir_utf);
}

/*!
 * \brief
 * Assigns multiple data directories where Proj4 will search for its init and
 * grid files if it cannot find them in the directory. The whole list is searched no
 * matter the type of file (init or grid).
 * This method should be called before attempting to construct a PJ instance from
 * init files or grids (like "+init=epsg:4236", "+datum=NAD83", "+ellps=clrk66",
 * "+nadgrids=@ntv2_0.gsb", etc.)
 * For Android: Usually the search paths are located under /data/data/<app_package_name>
 *
 * \param  env        	 - The JNI environment.
 * \param  class      	 - The class from which this method has been invoked.
 * \param  paths		 - String array containing the paths in which Proj4 will search for data files.
 * \author Svetlozar Kostadinov <sevarbg@gmail.com>
 */
JNIEXPORT void JNICALL Java_org_proj4_PJ_setSearchPaths(JNIEnv *env, jclass class, jobjectArray paths)
{
    int path_count = (*env)->GetArrayLength(env, paths);

    if (path_count == 0 || paths == NULL)
    {
    	pj_set_searchpath(0, NULL);
    	return;
    }

    const char** paths_ptr = malloc(path_count * sizeof(char*));
    jstring** paths_str = malloc(path_count * sizeof(jstring*));

    int i;
    for (i = 0; i < path_count; i++)
    {
    	paths_str[i] = (jstring)(*env)->GetObjectArrayElement(env, paths, i);
        paths_ptr[i] = (char*)(*env)->GetStringUTFChars(env, (char*) paths_str[i], 0);
    }

    pj_set_searchpath(path_count, paths_ptr);

    for (i = 0; i < path_count; i++)
	{
		(*env)->ReleaseStringUTFChars(env, (char*) paths_str[i], paths_ptr[i]);
	}

    free(paths_ptr);
    free(paths_str);
}

jobject constructCTable(JNIEnv *env, projCtx ctx, struct CTABLE *ct) {

	jclass clsCT = (*env)->FindClass(env, "org/proj4/GridInfo$CTable");
	if (clsCT == NULL)
		return NULL;
	jmethodID ctorCT = (*env)->GetMethodID(env, clsCT, "<init>", "()V");
	if (ctorCT == NULL)
		return NULL;
	jobject objCT = (*env)->NewObject(env, clsCT, ctorCT);
	if (objCT == NULL)
		return NULL;

	jfieldID field;

	// "id" String field assignment

	if (ct->id) {
		jstring strID = (*env)->NewStringUTF(env, ct->id);
		if (strID) {
			field = (*env)->GetFieldID(env, clsCT, CTABLE_FIELD_ID, "Ljava/lang/String;");
			if (field)
				(*env)->SetObjectField(env, objCT, field, strID);
			(*env)->DeleteLocalRef(env, strID);
		}
	}

	// "originLambda" double field assignment

	field = (*env)->GetFieldID(env, clsCT, CTABLE_FIELD_ORIGINLAMBDA, "D");
	if (field) {
		(*env)->SetDoubleField(env, objCT, field, ct->ll.lam);
	}

	// "originPhi" double field assignment

	field = (*env)->GetFieldID(env, clsCT, CTABLE_FIELD_ORIGINPHI, "D");
	if (field) {
		(*env)->SetDoubleField(env, objCT, field, ct->ll.phi);
	}

	// "cellLambda" double field assignment

	field = (*env)->GetFieldID(env, clsCT, CTABLE_FIELD_CELLLAMBDA, "D");
	if (field) {
		(*env)->SetDoubleField(env, objCT, field, ct->del.lam);
	}

	// "cellPhi" double field assignment

	field = (*env)->GetFieldID(env, clsCT, CTABLE_FIELD_CELLPHI, "D");
	if (field) {
		(*env)->SetDoubleField(env, objCT, field, ct->del.phi);
	}

	// "sizeLambda" int field assignment

	field = (*env)->GetFieldID(env, clsCT, CTABLE_FIELD_SIZELAMBDA, "I");
	if (field) {
		(*env)->SetIntField(env, objCT, field, ct->lim.lam);
	}

	// "sizePhi" int field assignment

	field = (*env)->GetFieldID(env, clsCT, CTABLE_FIELD_SIZEPHI, "I");
	if (field) {
		(*env)->SetIntField(env, objCT, field, ct->lim.phi);
	}

	return objCT;
}

jobject constructGridInfo(JNIEnv *env, projCtx ctx, PJ_GRIDINFO *gi) {

	jclass clsGI;
	jmethodID ctorGI;
	jfieldID field;
	jobject obj;
	jstring str;
	jobject val;
	struct CTABLE *ct;
	const char *fmt;

	// GridInfo object construction

	clsGI = (*env)->FindClass(env, "org/proj4/GridInfo");
	if (clsGI == NULL)
		return NULL;
	ctorGI = (*env)->GetMethodID(env, clsGI, "<init>", "()V");
	if (ctorGI == NULL)
		return NULL;
	obj = (*env)->NewObject(env, clsGI, ctorGI);
	if (obj == NULL)
		return NULL;

	// "gridName" String field assignment

	if (gi->gridname && strlen(gi->gridname) > 0) {
		field = (*env)->GetFieldID(env, clsGI, GRIDINFO_FIELD_GRIDNAME,
				"Ljava/lang/String;");
		if (field) {
			str = (*env)->NewStringUTF(env, gi->gridname);
			(*env)->SetObjectField(env, obj, field, str);
			(*env)->DeleteLocalRef(env, str);
		}
	}

	// "fileName" String field assignment

	if (gi->filename && strlen(gi->filename) > 0) {
		field = (*env)->GetFieldID(env, clsGI, GRIDINFO_FIELD_FILENAME,
				"Ljava/lang/String;");
		if (field) {
			str = (*env)->NewStringUTF(env, gi->filename);
			(*env)->SetObjectField(env, obj, field, str);
			(*env)->DeleteLocalRef(env, str);
		}
	}

	// "format" enum field assignment

	if (gi->format) {
		if (strncmp(gi->format, "ctable", 6) == 0) {
			fmt = "CTABLE";
		} else if (strncmp(gi->format, "ctable2", 6) == 0) {
			fmt = "CTABLE2";
		} else if (strncmp(gi->format, "ntv1", 4) == 0) {
			fmt = "NTV1";
		} else if (strncmp(gi->format, "ntv2", 4) == 0) {
			fmt = "NTV2";
		} else if (strncmp(gi->format, "gtx", 3) == 0) {
			fmt = "GTX";
		} else if (strncmp(gi->format, "missing", 7) == 0) {
			fmt = NULL;
		}

		if (fmt) {
			jclass clsFmt = (*env)->FindClass(env, "org/proj4/GridInfo$Format");
			if (clsFmt) {
				field = (*env)->GetStaticFieldID(env, clsFmt, fmt,
						"Lorg/proj4/GridInfo$Format;");
				if (field) {
					val = (*env)->GetStaticObjectField(env, clsFmt, field);
					if (val) {
						jfieldID fieldFmt = (*env)->GetFieldID(env, clsGI, GRIDINFO_FIELD_FORMAT, "Lorg/proj4/GridInfo$Format;");
						if (fieldFmt)
							(*env)->SetObjectField(env, obj, fieldFmt, val);
						(*env)->DeleteLocalRef(env, val);
					}
				}
				(*env)->DeleteLocalRef(env, clsFmt);
			}
		}
	}

	// "gridOffset" int field assignment

	field = (*env)->GetFieldID(env, clsGI, GRIDINFO_FIELD_GRIDOFFSET, "I");
	if (field) {
		(*env)->SetIntField(env, obj, field, gi->grid_offset);
	}

	// "ctable" field assignment

	if (gi->ct) {
		jfieldID fieldCT = (*env)->GetFieldID(env, clsGI, GRIDINFO_FIELD_CTABLE, "Lorg/proj4/GridInfo$CTable;");
		if (fieldCT)
		{
			jobject objCT = constructCTable(env, ctx, gi->ct);
			if (objCT)
			{
				(*env)->SetObjectField(env, obj, fieldCT, objCT);
				(*env)->DeleteLocalRef(env, objCT);
			}
		}
	}

	// "next" field recursive assignment

	if (gi->next) {
		jfieldID fieldNext = (*env)->GetFieldID(env, clsGI, GRIDINFO_FIELD_NEXT, "Lorg/proj4/GridInfo;");
		if (fieldNext)
		{
			jobject objNext = constructGridInfo(env, ctx, gi->next);
			if (objNext) {
				(*env)->SetObjectField(env, obj, fieldNext, objNext);
				(*env)->DeleteLocalRef(env, objNext);
			}
		}
	}

	// "child" field recursive assignment

	if (gi->child) {
		jfieldID fieldChild = (*env)->GetFieldID(env, clsGI, GRIDINFO_FIELD_CHILD, "Lorg/proj4/GridInfo;");
		if (fieldChild)
		{
			jobject objChild = constructGridInfo(env, ctx, gi->child);
			if (objChild) {
				(*env)->SetObjectField(env, obj, fieldChild, objChild);
				(*env)->DeleteLocalRef(env, objChild);
			}
		}
	}

	return obj;
}

/*!
 * \brief
 * Reads information from a grid file header to identify its format
 * and other metadata.
 *
 * \param  env        	 - The JNI environment.
 * \param  class      	 - The class from which this method has been invoked.
 * \param  gridname		 - The grid filename (without the path to it)
 * \return A GridInfo object containing data about the grid
 * \author Svetlozar Kostadinov <sevarbg@gmail.com>
 */
JNIEXPORT jobject JNICALL Java_org_proj4_PJ_readGridInfo(JNIEnv *env,
		jclass class, jstring gridname) {

	if (!gridname) {
		jclass npe = (*env)->FindClass(env, "java/lang/NullPointerException");
		if (npe)
			(*env)->ThrowNew(env, npe, "gridname cannot be null");
		return NULL;
	}

	const char *szGridname = (*env)->GetStringUTFChars(env, gridname, NULL);
	if (!szGridname) return NULL;

	// Load a grid file and try to read the info from its header
	projCtx ctx = pj_get_default_ctx();
	PJ_GRIDINFO *gi = pj_gridinfo_init(ctx, szGridname);
	(*env)->ReleaseStringUTFChars(env, gridname, szGridname);
	if (!gi || !gi->ct) {
		jclass pjex = (*env)->FindClass(env, "org/proj4/PJException");
		if (pjex) (*env)->ThrowNew(env, pjex, pj_strerrno(ctx->last_errno));
		return NULL;
	}

	jobject objGI = constructGridInfo(env, ctx, gi);
	pj_gridinfo_free(ctx, gi);

	return objGI;
}

/*!
 * \brief
 * Returns Proj4 last error code. It is stored in pj_errno global variable.
 *
 * \return the code of the last error in Proj4 (number from -1 to -49) as defined in pj_strerrno.c
 * A zero is returned if no error.
 * \author Svetlozar Kostadinov <sevarbg@gmail.com>
 */
JNIEXPORT jint JNICALL Java_org_proj4_PJ_getGlobalLastErrorCode(JNIEnv *env, jclass class) {
	return (jint) pj_errno;
}

/*!
 * \brief
 * Returns Proj4 last error string. It is given by function pj_strerrno(int err)
 *
 * \return the string of the last error in Proj4 (number from -1 to -49) as defined in pj_strerrno.c.
 * If no error (error code 0) is defined a NULL string is returned.
 * \author Svetlozar Kostadinov <sevarbg@gmail.com>
 */
JNIEXPORT jstring JNICALL Java_org_proj4_PJ_getGlobalLastErrorString(JNIEnv *env, jclass class) {
	const char *strerr = pj_strerrno(pj_errno);
	return (strerr) ? (*env)->NewStringUTF(env, strerr) : NULL;
}

/*!
 * \brief
 * Internal method returning the address of the PJ structure wrapped by the given Java object.
 * This function looks for a field named "ptr" and of type "long" (Java signature "J") in the
 * given object.
 *
 * \param  env    - The JNI environment.
 * \param  object - The Java object wrapping the PJ structure (not allowed to be NULL).
 * \return The address of the PJ structure, or NULL if the operation fails (for example
 *         because the "ptr" field was not found).
 */
PJ *getPJ(JNIEnv *env, jobject object)
{
    jfieldID id = (*env)->GetFieldID(env, (*env)->GetObjectClass(env, object), PJ_FIELD_NAME, PJ_FIELD_TYPE);
    return (id) ? (PJ*) (*env)->GetLongField(env, object, id) : NULL;
}

/*!
 * \brief
 * Returns the Proj4 release number.
 *
 * \param  env   - The JNI environment.
 * \param  class - The class from which this method has been invoked.
 * \return The Proj4 release number, or NULL.
 */
JNIEXPORT jstring JNICALL Java_org_proj4_PJ_getVersion
  (JNIEnv *env, jclass class)
{
    const char *desc = pj_get_release();
    return (desc) ? (*env)->NewStringUTF(env, desc) : NULL;
}

/*!
 * \brief
 * Allocates a new PJ structure from a definition string.
 *
 * \param  env        - The JNI environment.
 * \param  class      - The class from which this method has been invoked.
 * \param  definition - The string definition to be given to Proj4.
 * \return The address of the new PJ structure, or 0 in case of failure.
 */
JNIEXPORT jlong JNICALL Java_org_proj4_PJ_allocatePJ
  (JNIEnv *env, jclass class, jstring definition)
{
    const char *def_utf = (*env)->GetStringUTFChars(env, definition, NULL);
    if (!def_utf) return 0; /* OutOfMemoryError already thrown. */
    PJ *pj = pj_init_plus(def_utf);
    (*env)->ReleaseStringUTFChars(env, definition, def_utf);
    return (jlong) pj;
}

/*!
 * \brief
 * Allocates a new geographic PJ structure from an existing one.
 *
 * \param  env       - The JNI environment.
 * \param  class     - The class from which this method has been invoked.
 * \param  projected - The PJ object from which to derive a new one.
 * \return The address of the new PJ structure, or 0 in case of failure.
 */
JNIEXPORT jlong JNICALL Java_org_proj4_PJ_allocateGeoPJ
  (JNIEnv *env, jclass class, jobject projected)
{
    PJ *pj = getPJ(env, projected);
    return (pj) ? (jlong) pj_latlong_from_proj(pj) : 0;
}

/*!
 * \brief
 * Returns the definition string.
 *
 * \param  env    - The JNI environment.
 * \param  object - The Java object wrapping the PJ structure (not allowed to be NULL).
 * \return The definition string.
 */
JNIEXPORT jstring JNICALL Java_org_proj4_PJ_getDefinition
  (JNIEnv *env, jobject object)
{
    PJ *pj = getPJ(env, object);
    if (pj) {
        char *desc = pj_get_def(pj, 0);
        if (desc) {
            jstring str = (*env)->NewStringUTF(env, desc);
            pj_dalloc(desc);
            return str;
        }
    }
    return NULL;
}

/*!
 * \brief
 * Returns the description associated to the PJ structure.
 *
 * \param  env    - The JNI environment.
 * \param  object - The Java object wrapping the PJ structure (not allowed to be NULL).
 * \return The description associated to the PJ structure.
 */
JNIEXPORT jstring JNICALL Java_org_proj4_PJ_toString
  (JNIEnv *env, jobject object)
{
    PJ *pj = getPJ(env, object);
    if (pj) {
        const char *desc = pj->descr;
        if (desc) {
            return (*env)->NewStringUTF(env, desc);
        }
    }
    return NULL;
}

/*!
 * \brief
 * Returns the CRS type as one of the PJ.Type enum: GEOGRAPHIC, GEOCENTRIC or PROJECTED.
 * This function should never return NULL, unless class or fields have been renamed in
 * such a way that we can not find anymore the expected enum values.
 *
 * \param  env    - The JNI environment.
 * \param  object - The Java object wrapping the PJ structure (not allowed to be NULL).
 * \return The CRS type as one of thevoid MyJNIFunction(JNIEnv *env, jobject object, jobjectarray stringArray) {

    int stringCount = GetArrayLength(env, stringArray);

    for (int i=0; i<stringCount; i++) {
        jstring string = (jstring) GetObjectArrayElement(env, stringArray, i);
        const char *rawString = GetStringUTFChars(env, string, 0);
        // Don't forget to call `ReleaseStringUTFChars` when you're done.
    }
} PJ.Type enum.
 */
JNIEXPORT jobject JNICALL Java_org_proj4_PJ_getType
  (JNIEnv *env, jobject object)
{
    PJ *pj = getPJ(env, object);
    if (pj) {
        const char *type;
        if (pj_is_latlong(pj)) {
            type = "GEOGRAPHIC";
        } else if (pj_is_geocent(pj)) {
            type = "GEOCENTRIC";
        } else {
            type = "PROJECTED";
        }
        jclass c = (*env)->FindClass(env, "org/proj4/PJ$Type");
        if (c) {
            jfieldID id = (*env)->GetStaticFieldID(env, c, type, "Lorg/proj4/PJ$Type;");
            if (id) {
                return (*env)->GetStaticObjectField(env, c, id);
            }
        }
    }
    return NULL;
}

/*!
 * \brief
 * Returns the semi-major axis length.
 *
 * \param  env    - The JNI environment.
 * \param  object - The Java object wrapping the PJ structure (not allowed to be NULL).
 * \return The semi-major axis length.
 */
JNIEXPORT jdouble JNICALL Java_org_proj4_PJ_getSemiMajorAxis
  (JNIEnv *env, jobject object)
{
    PJ *pj = getPJ(env, object);
    return pj ? pj->a_orig : NAN;
}

/*!
 * \brief
 * Computes the semi-minor axis length from the semi-major axis length and the eccentricity
 * squared.
 *
 * \param  env    - The JNI environment.
 * \param  object - The Java object wrapping the PJ structure (not allowed to be NULL).
 * \return The semi-minor axis length.
 */
JNIEXPORT jdouble JNICALL Java_org_proj4_PJ_getSemiMinorAxis
  (JNIEnv *env, jobject object)
{
    PJ *pj = getPJ(env, object);
    if (!pj) return NAN;
    double a = pj->a_orig;
    return sqrt(a*a * (1.0 - pj->es_orig));
}

/*!
 * \brief
 * Returns the eccentricity squared.
 *
 * \param  env    - The JNI environment.
 * \param  object - The Java object wrapping the PJ structure (not allowed to be NULL).
 * \return The eccentricity.
 */
JNIEXPORT jdouble JNICALL Java_org_proj4_PJ_getEccentricitySquared
  (JNIEnv *env, jobject object)
{
    PJ *pj = getPJ(env, object);
    return pj ? pj->es_orig : NAN;
}

/*!
 * \brief
 * Returns an array of character indicating the direction of each axis.
 *
 * \param  env    - The JNI environment.
 * \param  object - The Java object wrapping the PJ structure (not allowed to be NULL).
 * \return The axis directions.
 */
JNIEXPORT jcharArray JNICALL Java_org_proj4_PJ_getAxisDirections
  (JNIEnv *env, jobject object)
{
    PJ *pj = getPJ(env, object);
    if (pj) {
        int length = strlen(pj->axis);
        jcharArray array = (*env)->NewCharArray(env, length);
        if (array) {
            jchar* axis = (*env)->GetCharArrayElements(env, array, NULL);
            if (axis) {
                /* Don't use memcp because the type may not be the same. */
                int i;
                for (i=0; i<length; i++) {
                    axis[i] = pj->axis[i];
                }
                (*env)->ReleaseCharArrayElements(env, array, axis, 0);
            }
            return array;
        }
    }
    return NULL;
}

/*!
 * \brief
 * Longitude of the prime meridian measured from the Greenwich meridian, positive eastward.
 *
 * \param env    - The JNI environment.
 * \param object - The Java object wrapping the PJ structure (not allowed to be NULL).
 * \return The prime meridian longitude, in degrees.
 */
JNIEXPORT jdouble JNICALL Java_org_proj4_PJ_getGreenwichLongitude
  (JNIEnv *env, jobject object)
{
    PJ *pj = getPJ(env, object);
    return (pj) ? (pj->from_greenwich)*(180/M_PI) : NAN;
}

/*!
 * \brief
 * Returns the conversion factor from linear units to metres.
 *
 * \param env      - The JNI environment.
 * \param object   - The Java object wrapping the PJ structure (not allowed to be NULL).
 * \param vertical - JNI_FALSE for horizontal axes, or JNI_TRUE for the vertical axis.
 * \return The conversion factor to metres.
 */
JNIEXPORT jdouble JNICALL Java_org_proj4_PJ_getLinearUnitToMetre
  (JNIEnv *env, jobject object, jboolean vertical)
{
    PJ *pj = getPJ(env, object);
    if (pj) {
        return (vertical) ? pj->vto_meter : pj->to_meter;
    }
    return NAN;
}

/*!
 * \brief
 * Converts input values from degrees to radians before coordinate operation, or the output
 * values from radians to degrees after the coordinate operation.
 *
 * \param pj        - The Proj.4 PJ structure.
 * \param data      - The coordinate array to transform.
 * \param numPts    - Number of points to transform.
 * \param dimension - Dimension of points in the coordinate array.
 * \param factor    - The scale factor to apply: M_PI/180 for inputs or 180/M_PI for outputs.
 */
void convertAngularOrdinates(PJ *pj, double* data, jint numPts, int dimension, double factor) {
    int dimToSkip;
    if (pj_is_latlong(pj)) {
        /* Convert only the 2 first ordinates and skip all the other dimensions. */
        dimToSkip = dimension - 2;
    } else if (pj_is_geocent(pj)) {
        /* Convert only the 3 first ordinates and skip all the other dimensions. */
        dimToSkip = dimension - 3;
    } else {
        /* Not a geographic or geocentric CRS: nothing to convert. */
        return;
    }
    double *stop = data + dimension*numPts;
    if (dimToSkip > 0) {
        while (data != stop) {
            (*data++) *= factor;
            (*data++) *= factor;
            data += dimToSkip;
        }
    } else {
        while (data != stop) {
            (*data++) *= factor;
        }
    }
}

/*!
 * \brief
 * Transforms in-place the coordinates in the given array.
 *
 * \param env         - The JNI environment.
 * \param object      - The Java object wrapping the PJ structure (not allowed to be NULL).
 * \param target      - The target CRS.
 * \param dimension   - The dimension of each coordinate value. Must be equals or greater than 2.
 * \param coordinates - The coordinates to transform, as a sequence of (x,y,<z>,...) tuples.
 * \param offset      - Offset of the first coordinate in the given array.
 * \param numPts      - Number of points to transform.
 */
JNIEXPORT void JNICALL Java_org_proj4_PJ_transform
  (JNIEnv *env, jobject object, jobject target, jint dimension, jdoubleArray coordinates, jint offset, jint numPts)
{
    if (!target || !coordinates) {
        jclass c = (*env)->FindClass(env, "java/lang/NullPointerException");
        if (c) (*env)->ThrowNew(env, c, "The target CRS and the coordinates array can not be null.");
        return;
    }
    if (dimension < 2 || dimension > PJ_MAX_DIMENSION) { /* Arbitrary upper value for catching potential misuse. */
        jclass c = (*env)->FindClass(env, "java/lang/IllegalArgumentException");
        if (c) (*env)->ThrowNew(env, c, "Illegal dimension. Must be in the [2-100] range.");
        return;
    }
    if ((offset < 0) || (numPts < 0) || (offset + dimension*numPts) > (*env)->GetArrayLength(env, coordinates)) {
        jclass c = (*env)->FindClass(env, "java/lang/ArrayIndexOutOfBoundsException");
        if (c) (*env)->ThrowNew(env, c, "Illegal offset or illegal number of points.");
        return;
    }
    PJ *src_pj = getPJ(env, object);
    PJ *dst_pj = getPJ(env, target);
    if (src_pj && dst_pj) {
        /* Using GetPrimitiveArrayCritical/ReleasePrimitiveArrayCritical rather than
           GetDoubleArrayElements/ReleaseDoubleArrayElements increase the chances that
           the JVM returns direct reference to its internal array without copying data.
           However we must promise to run the "critical" code fast, to not make any
           system call that may wait for the JVM and to not invoke any other JNI method. */
        double *data = (*env)->GetPrimitiveArrayCritical(env, coordinates, NULL);
        if (data) {
            double *x = data + offset;
            double *y = x + 1;
            double *z = (dimension >= 3) ? y+1 : NULL;
            convertAngularOrdinates(src_pj, x, numPts, dimension, M_PI/180);
            int err = pj_transform(src_pj, dst_pj, numPts, dimension, x, y, z);
            convertAngularOrdinates(dst_pj, x, numPts, dimension, 180/M_PI);
            (*env)->ReleasePrimitiveArrayCritical(env, coordinates, data, 0);
            if (err) {
                jclass c = (*env)->FindClass(env, "org/proj4/PJException");
                if (c) (*env)->ThrowNew(env, c, pj_strerrno(err));
            }
        }
    }
}

/*!
 * \brief
 * Returns a description of the last error that occurred, or NULL if none.
 *
 * \param  env    - The JNI environment.
 * \param  object - The Java object wrapping the PJ structure (not allowed to be NULL).
 * \return The last error, or NULL.
 */
JNIEXPORT jstring JNICALL Java_org_proj4_PJ_getLastError
  (JNIEnv *env, jobject object)
{
    PJ *pj = getPJ(env, object);
    if (pj) {
        int err = pj_ctx_get_errno(pj->ctx);
        if (err) {
            return (*env)->NewStringUTF(env, pj_strerrno(err));
        }
    }
    return NULL;
}

/*!
 * \brief
 * Deallocate the PJ structure. This method is invoked by the garbage collector exactly once.
 * This method will also set the Java "ptr" final field to 0 as a safety. In theory we are not
 * supposed to change the value of a final field. But no Java code should use this field, and
 * the PJ object is being garbage collected anyway. We set the field to 0 as a safety in case
 * some user invoked the finalize() method explicitely despite our warning in the Javadoc to
 * never do such thing.
 *
 * \param env    - The JNI environment.
 * \param object - The Java object wrapping the PJ structure (not allowed to be NULL).
 */
JNIEXPORT void JNICALL Java_org_proj4_PJ_finalize
  (JNIEnv *env, jobject object)
{
    jfieldID id = (*env)->GetFieldID(env, (*env)->GetObjectClass(env, object), PJ_FIELD_NAME, PJ_FIELD_TYPE);
    if (id) {
        PJ *pj = (PJ*) (*env)->GetLongField(env, object, id);
        if (pj) {
            (*env)->SetLongField(env, object, id, (jlong) 0);
            pj_free(pj);
        }
    }
}

#endif
