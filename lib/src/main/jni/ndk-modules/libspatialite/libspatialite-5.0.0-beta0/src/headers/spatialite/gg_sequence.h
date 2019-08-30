/*
 gg_sequence.h -- Gaia support for Spatialite's own Sequence
  
 version 4.4, 2016 August 12

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
 
Portions created by the Initial Developer are Copyright (C) 2008-2015
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
 \file gg_sequence.h

 Spatialite's own Sequence
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS
/* stdio.h included for FILE objects. */
#include <stdio.h>
#ifdef DLL_EXPORT
#define GAIASEQ_DECLARE __declspec(dllexport)
#else
#define GAIASEQ_DECLARE extern
#endif
#endif

#ifndef _GG_SEQUENCE_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GG_SEQUENCE_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/** 
 Typedef for Spatialite's own Sequence
 */
    typedef struct gaia_sequence
    {
/** name of the Sequence; NULL for the generic unnamed Sequence */
	char *seq_name;
/** current value */
	int value;
/** pointer to next Sequence (linked list) */
	struct gaia_sequence *next;
    } gaiaSequence;

/**
 Typedef for Spatialite's own Sequence
 */
    typedef gaiaSequence *gaiaSequencePtr;
/**
/ Creates a new SpatiaLite's own Sequence or retrieves an already
  existing Sequence of the same name

 \param p_cache a memory pointer returned by spatialite_alloc_connection()
 \param seq_name name of the Sequence (may be NULL)
  
 \return a pointer to the Sequence or NULL on failure
  
 \sa gaiaFindSequence(), gaiaLastUsedSequence(), gaiaSequenceNext(),
 gaiaResetSequence()
  */
    GAIASEQ_DECLARE gaiaSequencePtr gaiaCreateSequence (const void *p_cache,
							const char *seq_name);

/**
/ Finds an existing SpatiaLite's own Sequence

 \param p_cache a memory pointer returned by spatialite_alloc_connection()
 \param seq_name name of the Sequence (may be NULL)
  
 \return a pointer to the Sequence or NULL on failure
  
 \sa gaiaCreateSequence(), gaiaLastUsedSequence(), gaiaSequenceNext(),
 gaiaResetSequence()
  */
    GAIASEQ_DECLARE gaiaSequencePtr gaiaFindSequence (const void *p_cache,
						      const char *seq_name);

/**
/ Finds an existing SpatiaLite's own Sequence

 \param p_cache a memory pointer returned by spatialite_alloc_connection()
 \param last_value on sucess will contain the most recently used Sequence value
  
 \return ZERO on failure, any other value on success.
  
 \sa gaiaCreateSequence(), gaiaLastUsedSequence(), gaiaSequenceNext(),
 gaiaResetSequence()
  */
    GAIASEQ_DECLARE int gaiaLastUsedSequence (const void *p_cache,
					      int *last_value);

/**
/ Increases by 1 the value of some SpatiaLite's own Sequence

 \param p_cache a memory pointer returned by spatialite_alloc_connection()
 \param sequence a memory pointer returned by gaiaFindSequence() or
 gaiaFindCreateSequence()
 \param value new value to be set for the given Sequence
  
 \return ZERO on failure, any other value on success.
  
 \sa gaiaFindSequence(), gaiaCreateSequence(), gaiaLastUsedSequence(),
 gaiaResetSequence()
   
 \note this method will reset an existing Sequence. The initial 
 value will be increased by the next call to gaiaSequenceNext()
  */
    GAIASEQ_DECLARE int gaiaSequenceNext (const void *p_cache,
					  gaiaSequencePtr sequence);

/**
/ Resets a SpatiaLite's own Sequence

 \param sequence a memory pointer returned by gaiaFindSequence() or
 gaiaCreateSequence()
 \param value new value to be set for the given Sequence
  
 \return ZERO on failure, any other value on success.
  
 \sa gaiaFindSequence(), gaiaCreateSequence(), gaiaSequenceNext(),
 gaiaLastUsedSequence()
   
 \note this method will reset an existing Sequence. The initial 
 value will be increased by the next call to gaiaSequenceNext()
 */
    GAIASEQ_DECLARE int gaiaResetSequence (gaiaSequencePtr sequence, int value);

#ifdef __cplusplus
}
#endif

#endif				/* _GG_SEQUENCE_H */
