/* $Id: tables.h,v 1.2 1994/11/04 17:56:22 vertex Exp $ */

#ifndef __TABLES_H
#define __TABLES_H


/****************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif


/****************************************************************************/


typedef struct SJIS_UniCodeTable
{
    uint16 SJISCode;
    uint16 UniCode;
} SJIS_UniCodeTable;

typedef	struct RomajiTableIndex
{
    char  alpha;
    int16 n;
} RomajiTableIndex, *RomajiTableIndexPtr, **RomajiTableIndexHandle;

typedef struct RomajiTableRec
{
    char *romaji;
    char *halfKana;
    char *fullKana;
    char *hiragana;
} RomajiTableRec, *RomajiTablePtr, **RomajiTableHandle;


/*****************************************************************************/


extern SJIS_UniCodeTable uniCodeTable[];
extern char              asciiTable[];
extern int16             gRomajiTableIndex[];
extern RomajiTableRec    gRomajiTable[];


/*****************************************************************************/


#endif /* __TABLES_H */
