/* $Id: ezmem_tools.h,v 1.1 1994/08/11 21:02:52 phil Exp $ */
#pragma include_only_once
#ifndef _ezmem_tools_h
#define _ezmem_tools_h

/****************************************************
** Includes for AudioFolio Memory Allocation Tools
** By:  Phil Burk
**
** Copyright (C) 1992, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
******************************************************
** 940811 PLB Stripped from handy_tools.c
******************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Prototypes */
int32 EZMemSize ( void *ptr );
int32 EZMemSetCustomVectors( void *(*AllocVector)(int32 Size, uint32 Type),
	 void (*FreeVector)(void *p, int32 Size) );
int32 SumAvailMem( List *l, uint32 Type );
void  DumpMemory( void *addr, int32 cnt);
void  EZMemFree ( void *ptr );
void *EZMemAlloc ( int32 size, int32 type );
void *zalloc( int32 NumBytes );
void *UserMemAlloc ( int32 size, int32 type );
void UserMemFree( void *p, int32 size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
