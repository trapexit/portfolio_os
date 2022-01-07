/* $Id: intltest.h,v 1.4 1994/05/09 22:46:51 vertex Exp $ */

#ifndef __INTLTEST_H
#define __INTLTEST_H


/*****************************************************************************/


#include "types.h"
#include "debug.h"
#include "intl.h"


/*****************************************************************************/


extern bool verbose;


/*****************************************************************************/


#define CHECKRESULT(function,test,goal,result) {if (result != goal)\
                                               {\
                                                   printf("INTLTEST: %s, test %d, expected %d ($%x), got %d ($%x)\n",function,test,goal,goal,result,result);\
                                                   printf("          ");\
                                                   PrintfSysErr(result);\
                                               }}

#define CHECKRESULTSTR(function,test,goal,result) {if (!CompareUniCodeASCII(result,goal))\
                                                  {\
                                                      printf("INTLTEST: %s, test %d, expected '%s', got '",function,test,goal);\
                                                      PrintUniCode(result);\
                                                      printf("'\n");\
                                                  }}


/*****************************************************************************/


void UniCodeToASCII(unichar *uni, char *asc);
void ASCIIToUniCode(char *asc, unichar *uni);
void PrintUniCode(unichar *uni);
bool CompareUniCodeASCII(unichar *uni, char *asc);


/*****************************************************************************/


#endif /* __INTLTEST_H */
