#ifdef EXTERNAL_RELEASE
#error The file sysstate.h is not for external release
#endif

#ifndef __SYSSTATE_H
#define __SYSSTATE_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  $Id: sysstate.h,v 1.2 1994/11/15 02:22:35 vertex Exp $
**
**  Get/set system state values
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif


/*****************************************************************************/


#define SYSSTATE_TAG_BOOTDISCTYPE 0x10001

/* for use with the SYSSTATE_TAG_BOOTDISKTYPE tag */
typedef enum BootDiscTypes
{
   BDT_NotSet,
   BDT_NoDisc,
   BDT_Unknown,
   BDT_3DO,
   BDT_Audio,
   BDT_Photo,
   BDT_Video,
   BDT_Naviken
} BootDiscTypes;


/*****************************************************************************/


extern Err GetSystemState(uint32 tag, void *info, size_t infosize);
extern Err __swi(KERNELSWI+43) SetSystemState(uint32 tag, void *info, size_t infosize);


/*****************************************************************************/


#endif /* __SYSSTATE_H */
