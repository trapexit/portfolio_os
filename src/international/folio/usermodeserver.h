/* $Id: usermodeserver.h,v 1.3 1994/12/08 00:15:53 vertex Exp $ */

#ifndef __USERMODESERVER_H
#define __USERMODESERVER_H


/****************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif


/****************************************************************************/


typedef Err (* USERMODEFUNC)(void *);

extern Item serverThread;


/****************************************************************************/


Err CreateUserModeServer(void);
void DeleteUserModeServer(void);

Err CallUserModeFunc(USERMODEFUNC func, void *data);


/*****************************************************************************/


#endif /* __USERMODESERVER_H */
