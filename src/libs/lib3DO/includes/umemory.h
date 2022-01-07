#ifndef __UMEMORY_H
#define __UMEMORY_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  $Id: umemory.h,v 1.12 1995/02/21 00:04:11 stan Exp $
**
**  Lib3DO compatibility header. Do not use in new code.
**
******************************************************************************/


#ifndef __MEM_H
#include "mem.h"
#endif

#ifdef __cplusplus
  extern "C" {
#endif

void *	Malloc(uint32 size, uint32 memtype);
void *	Free(void *ptr);

#ifdef __cplusplus
  }
#endif

#define NewPtr				Malloc
#define FreePtr				Free

#define getptrsize			GetMemTrackSize
#define MemBlockSize		GetMemTrackSize

#define InitMalloc(x)		0

#define ReportMemoryUsage()
#define DebugMemUsage()

/*****************************************************************************/


#endif /* __UMEMORY_H */
