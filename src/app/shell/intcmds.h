/* $Id: intcmds.h,v 1.5 1994/08/12 20:45:45 vertex Exp $ */

#ifndef __INTCMDS_H
#define __INTCMDS_H


/****************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif


/****************************************************************************/


#ifdef DEVELOPMENT
#define VERBOSE(x) {if (verbose) kprintf x;}
#else
#define VERBOSE(x)
#endif


extern bool fromROM;
extern bool default_bg;
extern bool verbose;


bool ExecBuiltIn(char *command, char *cmdLine);
uint32 ConvertNum(char *str);


/****************************************************************************/


#endif /* __INTCMDS_H */
