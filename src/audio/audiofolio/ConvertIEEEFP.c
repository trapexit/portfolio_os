/* $Id: ConvertIEEEFP.c,v 1.3 1994/02/09 01:22:45 limes Exp $ */
/**********************************************************
**
** Convert IEEE floating point number to unsigned 16.16 fraction.
**
** By Steve Hayes
** Copyright 1993 3DO
**
**********************************************************/
#include "audio_internal.h"
/*#include "math.h" */
#include "stdio.h"
#include "operamath.h"

#define HUGE 0xffffffff

ufrac16
ufrac16FromIeee(char *bytes)
{
    ufrac16  f;
    int32    exp;
    uint32   hi, lo;
    
    exp = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
    hi    =    ((uint32)(bytes[2] & 0xFF) << 24)
            |    ((uint32)(bytes[3] & 0xFF) << 16)
            |    ((uint32)(bytes[4] & 0xFF) << 8)
            |    ((uint32)(bytes[5] & 0xFF));
    lo    =    ((uint32)(bytes[6] & 0xFF) << 24)
            |    ((uint32)(bytes[7] & 0xFF) << 16)
            |    ((uint32)(bytes[8] & 0xFF) << 8)
            |    ((uint32)(bytes[9] & 0xFF));

    if (exp == 0 && hi == 0 && lo == 0) {
        f = 0;
    }
    else {
        if (exp == 0x7FFF) {    /* Infinity or NaN */
            f = HUGE;
        }
        else {
            exp -= 16398;		/* Adjust exponent for 16.16 */
            if (exp > 0) {
							/* This will overflow for 16.16 so just hose the result.*/
#if 0
							f  = hi << exp;
							f  |= (lo >> (32-exp));
#else
							f = HUGE;
#endif
							}
            else if (exp < 0) f  = hi >> (-exp);
						else f = hi;
        }
    }

    if (bytes[0] & 0x80)
        return -f;
    else
        return f;
}
