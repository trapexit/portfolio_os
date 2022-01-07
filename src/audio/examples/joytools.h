#ifndef __JOYTOOLS_H
#define __JOYTOOLS_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  $Id: joytools.h,v 1.4 1994/10/06 18:43:49 peabody Exp $
**
**  audiodemo.lib joypad stuff (really obsolete)
**
******************************************************************************/


#include <types.h>


int32 InitJoypad( void );
int32 ReadJoypad( uint32 *Buttons );
int32 TermJoypad( void );
int32 WaitJoypad( uint32 Mask, uint32 *Buttons );


/*****************************************************************************/


#endif  /* __JOYTOOLS_H */
