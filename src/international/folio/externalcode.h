/* $Id: externalcode.h,v 1.2 1994/12/08 00:15:53 vertex Exp $ */

#ifndef __EXTERNALCODE_H
#define __EXTERNALCODE_H


/****************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __LOCALES_H
#include "locales.h"
#endif


/****************************************************************************/


Err BindExternalCode(Locale *loc);
void UnbindExternalCode(Locale *loc);


/*****************************************************************************/


#endif /* __EXTERNALCODE_H */
