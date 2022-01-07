/*****

$Id: filesystemdefs.c,v 1.2 1993/03/16 06:36:37 dplatt Exp $

$Log: filesystemdefs.c,v $
 * Revision 1.2  1993/03/16  06:36:37  dplatt
 * Functional Freeze release
 *
 * Revision 1.1  1992/09/11  00:42:32  dplatt
 * Initial revision
 *

 *****/

/*
  Copyright New Technologies Group, 1991.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  Kernel data definitions for the filesystem and I/O driver kit
*/

#include "types.h"
#include "item.h"
#include "mem.h"
#include "nodes.h"
#include "debug.h"
#include "list.h"
#include "device.h"
#include "driver.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "io.h"
#include "filesystem.h"
#include "filesystemdefs.h"
#include "discdata.h"

FileFolio *fileFolio;

