/*
 gg_sequence.c -- Gaia support for Spatialite's own Sequence
  
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
 
Portions created by the Initial Developer are Copyright (C) 2013-2015
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

#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#include <spatialite_private.h>

#ifdef _WIN32
#define strcasecmp	_stricmp
#endif /* not WIN32 */

GAIASEQ_DECLARE gaiaSequencePtr
gaiaCreateSequence (const void *p_cache, const char *seq_name)
{
/* creating a new Sequence or retrieving an already existing Sequence */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    gaiaSequencePtr seq;

    if (cache == NULL)
	return NULL;
    seq = cache->first_seq;
    while (seq != NULL)
      {
	  /* testing for an existing Sequence */
	  if (seq_name == NULL && seq->seq_name == NULL)
	      return seq;
	  if (seq_name != NULL && seq->seq_name != NULL)
	    {
		if (strcasecmp (seq_name, seq->seq_name) == 0)
		    return seq;
	    }
	  seq = seq->next;
      }

/* not already existsting; creating a new Sequence */
    seq = malloc (sizeof (gaiaSequence));
    if (seq_name == NULL)
	seq->seq_name = NULL;
    else
      {
	  int len = strlen (seq_name);
	  seq->seq_name = malloc (len + 1);
	  strcpy (seq->seq_name, seq_name);
      }
    seq->value = 0;
    seq->next = NULL;

/* inserting into the linked list */
    if (cache->first_seq == NULL)
	cache->first_seq = seq;
    if (cache->last_seq != NULL)
	cache->last_seq->next = seq;
    cache->last_seq = seq;

    return seq;
}

GAIASEQ_DECLARE gaiaSequencePtr
gaiaFindSequence (const void *p_cache, const char *seq_name)
{
/* retrieving an existing Sequence */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    gaiaSequencePtr seq;

    if (cache == NULL)
	return NULL;
    seq = cache->first_seq;
    while (seq != NULL)
      {
	  /* testing for an existing Sequence */
	  if (seq_name == NULL && seq->seq_name == NULL)
	      return seq;
	  if (seq_name != NULL && seq->seq_name != NULL)
	    {
		if (strcasecmp (seq_name, seq->seq_name) == 0)
		    return seq;
	    }
	  seq = seq->next;
      }

    return NULL;
}

GAIASEQ_DECLARE int
gaiaLastUsedSequence (const void *p_cache, int *value)
{
/* return the most recently used Sequence (if any) */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;

    if (cache == NULL)
	return 0;
    if (cache->ok_last_used_sequence == 0)
	return 0;
    *value = cache->last_used_sequence_val;
    return 1;
}

GAIASEQ_DECLARE int
gaiaSequenceNext (const void *p_cache, gaiaSequencePtr seq)
{
/* inreases the Sequence value */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;

    if (cache == NULL)
	return 0;
    if (seq == NULL)
	return 0;

    seq->value += 1;
    cache->ok_last_used_sequence = 1;
    cache->last_used_sequence_val = seq->value;
    return 1;
}

GAIASEQ_DECLARE int
gaiaResetSequence (gaiaSequencePtr seq, int value)
{
/* resetting a Sequence */
    if (seq == NULL)
	return 0;

    seq->value = abs (value);
    return 1;
}
