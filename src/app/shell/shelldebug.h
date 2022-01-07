/* $Id: shelldebug.h,v 1.2 1994/04/25 21:39:11 vertex Exp $ */

#ifndef __SHELLDEBUG_H
#define __SHELLDEBUG_H


/****************************************************************************/


#ifndef __OPERROR_H
#include "operror.h"
#endif


/****************************************************************************/


#ifdef TRACING
#include "debug.h"
#include "super.h"
#define TRACE(x)      {kprintf("Shell/"); kprintf x;}
#define SUPERTRACE(x) {Superkprintf("Shell/"); Superkprintf x;}
#else
#define TRACE(x)
#define SUPERTRACE(x)
#endif


/****************************************************************************/


#endif /* __SHELLDEBUG_H */
