#ifndef __AUDIODEMO_H
#define __AUDIODEMO_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  $Id: audiodemo.h,v 1.5 1994/10/06 18:43:49 peabody Exp $
**
**  Phil's audiodemo.lib master include file
**
******************************************************************************/


#include "faders.h"
#include "joytools.h"
#include "graphic_tools.h"


/* -------------------- macros (!!! publish) */

#define MAX(a,b)    ((a)>(b)?(a):(b))
#define MIN(a,b)    ((a)<(b)?(a):(b))
#define ABS(x)      ((x<0)?(-(x)):(x))
#define CLIPRANGE(n,a,b) ((n)<(a) ? (a) : (n)>(b) ? (b) : (n))      /* range clipping */


/*****************************************************************************/


#endif  /* __AUDIODEMO_H */
