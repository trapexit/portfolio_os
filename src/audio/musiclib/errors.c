/* $Id: errors.c,v 1.12 1994/10/07 21:09:39 peabody Exp $ */
/****************************************************************************
**
**  Music library error text.
**
**  By: Bill Barton
**
**  Copyright (c) 1992-1994, 3DO Company.
**  This program is proprietary and confidential.
**
**---------------------------------------------------------------------------
**
**  History:
**
**  940526 WJB  Created.
**  940527 WJB  Filled in error text.
**  940527 WJB  Replaced music.h with musicerror.h.
**  940527 WJB  Removed temp ER_MUSICLIB define.
**  940819 WJB  Added autodocs.
**  940906 WJB  Added text for ML_ERR_BAD_SAMPLE_ALIGNMENT.
**  940906 WJB  Added ML_ERR_BAD_SAMPLE_ADDRESS.
**  940915 WJB  Added ML_ERR_INCOMPATIBLE_SOUND thru ML_ERR_NAME_NOT_FOUND.
**  941007 WJB  Added ML_ERR_BUFFER_TOO_SMALL.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
****************************************************************************/

#ifdef DEVELOPMENT

#include <item.h>
#include <kernelnodes.h>
#include <musicerror.h>

#include "music_internal.h"     /* version stuff */


/* -------------------- error table */

static const char *Errors[] =
{
    /* no error                      */ "no error",
    /* ML_ERR_NOT_IFF_FORM           */ "Illegal IFF format",
    /* ML_ERR_BAD_FILE_NAME          */ "Could not open file",
    /* ML_ERR_NOT_OPEN               */ "Stream or file not open",
    /* ML_ERR_BAD_SEEK               */ "Bad seek mode or offset",
    /* ML_ERR_END_OF_FILE            */ "Unexpected end of file",
    /* ML_ERR_UNSUPPORTED_SAMPLE     */ "Sample format has no instrument to play it",
    /* ML_ERR_BAD_FORMAT             */ "File format is incorrect",
    /* ML_ERR_BAD_USERDATA           */ "UserData in MIDI Parser bad",
    /* ML_ERR_OUT_OF_RANGE           */ "Input parameter out of range",
    /* ML_ERR_NO_TEMPLATE            */ "No Template mapped to that Program number",
    /* ML_ERR_NO_NOTES               */ "No voices could be allocated",
    /* ML_ERR_BAD_ARG                */ "Invalid or conflicting argument(s) supplied to function",
    /* ML_ERR_BAD_SAMPLE_ALIGNMENT   */ "Sample not properly DMA aligned",
    /* ML_ERR_BAD_SAMPLE_ADDRESS     */ "Invalid sample address or length",
    /* ML_ERR_INCOMPATIBLE_SOUND     */ "Sound is incompatible with player",
    /* ML_ERR_CORRUPT_DATA           */ "Some data has become corrupt",
    /* ML_ERR_INVALID_MARKER         */ "Inappropriate marker for operation",
    /* ML_ERR_INVALID_FILE_TYPE      */ "inappropriate file type",
    /* ML_ERR_DUPLICATE_NAME         */ "Named thing already exists",
    /* ML_ERR_NAME_NOT_FOUND         */ "Named thing not found",
    /* ML_ERR_BUFFER_TOO_SMALL       */ "A (sound) buffer is too small",
};
#define MAX_ERROR_LEN 56            /* @@@ see usage below */


/* -------------------- Installation function */

 /**
 |||	AUTODOC PRIVATE mpg/musiclib/installmusicliberrors
 |||	InstallMusicLibErrors - Install error text for music.lib errors.
 |||
 |||	  Synopsis
 |||
 |||	    Err InstallMusicLibErrors (void)
 |||
 |||	  Description
 |||
 |||	    This function installs the error text for music.lib error codes such
 |||	    that the text for these error codes can be read with GetSysErr().
 |||
 |||	    This function is called by the shell during system startup in
 |||	    developer mode, but not runtime mode.
 |||
 |||	  Arguments
 |||
 |||	    None
 |||
 |||	  Return Value
 |||
 |||	    Nonnegative on success, negative error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V22.
 |||
 |||	  Associated Files
 |||
 |||	    music.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    GetSysErr(), PrintfSysErr()
 |||
 **/
Err InstallMusicLibErrors (void)
{
    static TagArg ErrorTags[] =
    {
        { TAG_ITEM_NAME,      (void *)"MusicLib" },
        { ERRTEXT_TAG_OBJID,  (void *)(ER_LINKLIB << ERR_IDSIZE | ER_MUSICLIB) },
        { ERRTEXT_TAG_MAXERR, (void *)(sizeof Errors / sizeof Errors[0]) },
        { ERRTEXT_TAG_TABLE,  (void *)Errors },
        { ERRTEXT_TAG_MAXSTR, (void *)MAX_ERROR_LEN },  /* @@@ apparently the kernel ignores this - besides it could just go ahead and compute it if it really needs to. */
        { TAG_END }
    };

    PULL_MUSICLIB_VERSION;

    return CreateItem (MKNODEID (KERNELNODE,ERRORTEXTNODE), ErrorTags);
}

#else /* !defined(DEVELOPEMENT) */

long _ml_installerrors_pad;           /* !!! shouldn't need this.  silly armcc bug */

#endif
