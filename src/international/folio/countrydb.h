/* $Id: countrydb.h,v 1.1 1994/12/08 00:15:53 vertex Exp $ */

#ifndef __COUNTRYDB_H
#define __COUNTRYDB_H


/****************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __INTL_H
#include "intl.h"
#endif


/****************************************************************************/


typedef struct FormHdr
{
    uint32 ID;
    uint32 Size;
    uint32 FormType;
} FormHdr;

typedef struct ChunkHdr
{
    uint32 ID;
    uint32 Size;
} ChunkHdr;

typedef struct CountryEntry
{
    CountryCodes ce_Country;
    uint32       ce_SeekOffset;
} CountryEntry;


#define ID_FORM 0x464f524d
#define ID_PREF 0x50524546
#define ID_INTL 0x494e544c
#define ID_CTRY 0x43545259

#define IFF_ROUND(x) ((x & 1) ? (x+1) : x)


/****************************************************************************/


#endif /* __COUNTRYDB_H */
