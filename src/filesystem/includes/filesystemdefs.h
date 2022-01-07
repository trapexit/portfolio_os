#ifndef __H_FILESYSTEMDEFS
#define __H_FILESYSTEMDEFS

#pragma force_top_level
#pragma include_only_once

#ifndef EXTERNAL_RELEASE

/******************************************************************************
**
**  $Id: filesystemdefs.h,v 1.5 1994/10/21 21:06:16 shawn Exp $
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __DRIVER_H
#include "driver.h"
#endif

#ifndef __H_FILESYSTEM
#include "filesystem.h"
#endif


/*****************************************************************************/


/* Data definitions for the filesystem and I/O driver kit */
extern FileFolio *fileFolio;
extern Driver *fileDriver;


/*****************************************************************************/

#endif /* EXTERNAL_RELEASE */

#endif /* __H_FILESYSTEMDEFS */
