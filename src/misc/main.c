/* $Id: main.c,v 1.2 1994/02/09 01:22:45 limes Exp $ */
#include "types.h"
#include "list.h"
#include "string.h"

#ifdef undef
extern void *memcpy(void *, void *, int);
extern void *memset(void *, int , int);
#endif

extern void __rt_sdiv(void);	/* dummy routines */
extern void __rt_sdiv10(void);	/* dummy routines */
extern void __rt_udiv(void);	/* dummy routines */
extern void __rt_udiv10(void);	/* dummy routines */

void *(*miscfuncs[])() =
{
	(void *(*)())memcpy,	/* 14 */
	(void *(*)())memset,	/* 13 */
	(void *(*)())RemNode,
	(void *(*)())InsertNodeFromTail,	/* 5 */
	(void *(*)())AddTail,
	(void *(*)())RemTail,
	(void *(*)())AddHead,
	(void *(*)())RemHead,	/* 1 */
	(void *(*)())InsertNodeFromHead,	/* 5 */
	(void *(*)())InitList,	/* 9 */
	(void *(*)())__rt_sdiv,	/* 9 */
	(void *(*)())__rt_udiv,	/* 9 */
	(void *(*)())__rt_sdiv10,	/* 9 */
	(void *(*)())__rt_udiv10,	/* 9 */
};

int
main()
{
    return (int)miscfuncs;
}

