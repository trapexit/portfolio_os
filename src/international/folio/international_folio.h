/* $Id: international_folio.h,v 1.4 1994/12/22 20:13:42 vertex Exp $ */

#ifndef __INTERNATIONAL_FOLIO_H
#define __INTERNATIONAL_FOLIO_H


/****************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __NODES_H
#include "nodes.h"
#endif

#ifndef __FOLIO_H
#include "folio.h"
#endif

#ifndef __INTL_H
#include "intl.h"
#endif


/****************************************************************************/


#ifdef TRACING
#include "stdio.h"
#include "super.h"
#define TRACE(x)      printf x
#define SUPERTRACE(x) Superkprintf x
#else
#define TRACE(x)
#define SUPERTRACE(x)
#endif


/****************************************************************************/


typedef struct InternationalFolio
{
    Folio fb_Folio;
} InternationalFolio;


/****************************************************************************/


/* these are defined here temporarily, until they appear in a sanctioned
 * kernel include file
 */
#define ValidateMem(a,b,c) SuperValidateMem(a,b,c)
#define IsRamAddr(a,b) SuperIsRamAddr(a,b)
#include "super.h"


/****************************************************************************/


extern Item defaultLocaleItem;


/****************************************************************************/


#endif /* __INTERNATIONAL_FOLIO_H */
