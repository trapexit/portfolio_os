/******************************************************************************
**
**  $Id: music_internal.h,v 1.2 1994/09/22 00:01:46 peabody Exp $
**
**  music.lib common internal include file (not for release).
**
**  By: Bill Barton
**
**  Copyright (c) 1994, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  940921 WJB  Created.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#ifndef __MUSIC_INTERNAL_H
#define __MUSIC_INTERNAL_H

#pragma force_top_level
#pragma include_only_once


/* -------------------- music.lib version string */

/*
    Macro to use to cause music.lib version to be included only in .dlibs
*/
#ifdef DEVELOPMENT
  #define PULL_MUSICLIB_VERSION musiclib_version()
#else
  #define PULL_MUSICLIB_VERSION
#endif

/*
    noop function to call in key music.lib functions to causes library
    version to be pulled into executable.
*/
const char *musiclib_version (void);


/* -------------------- package identification strings */

/*
    Define a package ID string. Put this in some key module for a given
    package. This also magically pulls in the library version string.
*/
#ifdef DEVELOPMENT
  #define DEFINE_MUSICLIB_PACKAGE_ID(package) \
    const char *musiclib_##package##_id(void) \
    {                                         \
        PULL_MUSICLIB_VERSION;                \
        return "@(#) music.lib/"#package;     \
    }
#else
  #define DEFINE_MUSICLIB_PACKAGE_ID(package)
#endif

/*
    Pull in the package ID. Put this inside key functions of each package.
*/
#ifdef DEVELOPMENT
  #define PULL_MUSICLIB_PACKAGE_ID(package)                 \
    do {                                                    \
        extern const char *musiclib_##package##_id (void);  \
        musiclib_##package##_id();                          \
    } while(0)
#else
  #define PULL_MUSICLIB_PACKAGE_ID(package)
#endif

#endif /* __MUSIC_INTERNAL_H */
