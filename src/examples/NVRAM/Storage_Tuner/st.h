#ifndef __ST_H
#define __ST_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  $Id: st.h,v 1.2 1994/11/18 18:04:47 vertex Exp $
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif


/*****************************************************************************/


typedef struct STParms
{
    Item    stp_ScreenGroup;
    List   *stp_MemoryLists;
} STParms;


/*****************************************************************************/


#endif /* __ST_H */
