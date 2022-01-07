/* $Id: locales.h,v 1.4 1994/12/08 00:15:53 vertex Exp $ */

#ifndef __LOCALES_H
#define __LOCALES_H


/****************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __ITEM_H
#include "item.h"
#endif

#ifndef __INTL_H
#include "intl.h"
#endif


/****************************************************************************/


Item CreateLocaleItem(Locale *loc, TagArg *args);
Err DeleteLocaleItem(Locale *loc);
Item OpenLocaleItem(Locale *loc, TagArg *args);
Err CloseLocaleItem(Locale *loc);
Item FindLocaleItem(TagArg *args);


/*****************************************************************************/


#endif /* __LOCALES_H */
