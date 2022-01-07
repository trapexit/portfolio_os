#ifndef __SIMPLEMATH_H
#define __SIMPLEMATH_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  $Id: simplemath.h,v 1.2 1994/10/06 22:02:45 vertex Exp $
**
**  Public header file for the simplemath folio.
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif


/*****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif


/* folio management */
Err OpenSimpleMathFolio(void);
Err CloseSimpleMathFolio(void);

/* main functions */
int32 AddNumber(int32 num1, int32 num2);
int32 SubNumber(int32 num1, int32 num2);

/* convenience routines */
int32 Add3Numbers(int32 num1, int32 num2, int32 num3);
int32 Add4Numbers(int32 num1, int32 num2, int32 num3, int32 num4);


#ifdef __cplusplus
}
#endif


/****************************************************************************/


/* user function offsets */
#define ADDNUMBER -1
#define SUBNUMBER -2


/*****************************************************************************/


#endif /* __SIMPLEMATH_H */
