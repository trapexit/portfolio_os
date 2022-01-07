/* $Id: gdbug.c,v 1.3 1994/02/09 01:22:45 limes Exp $ */

/* for now we do not worry about a particular memory type */
/* Like chip memory or public memory */

#undef KERNEL

extern void kprintf(const char *fmt, ...);
extern void printf(const char *fmt, ...);

/*#define TESTSPRINTF*/
#define PRINTF	kprintf

#include "types.h"
#include "nodes.h"
#include "list.h"
#include "kernel.h"
#include "task.h"
#include "kernelnodes.h"
#include "strings.h"
#include "super.h"

extern struct KernelBase *KernelBase;

uint32 mybits[32];

char *kernelswis[] =
{
	"createitem", /* 0 */
	"wait", /* 1 */
	"signal",	/* 2 */
	"deleteitem",	/* 3 */
	"finditem",	/* 4 */
	"openitem",	/* 5 */
	"unlocksemaphore",	/* 6 */
	"locksemaphore",	/* 7 */
	"closeitem",	/* 8 */
	"yield",	/* 9 */
	"setitempri",	/* 10 */
	"illegal",	/* 11 */
	"illegal",	/* 12 */
	"allocmem",	/* 13 */
	"printf",	/* 14 */
	"illegal",	/* 15 */
	"putmsg",	/* 16 */
	"waitport",	/* 17 */
	"replymsg",	/* 18 */
	"getmsg",	/* 19 */
	"controlmem",	/* 20 */
	"allocsignal",	/* 21 */
	"freesignal",	/* 22 */
	"illegal",	/* 23 */
	"sendio",	/* 24 */
	"abortio",	/* 25 */
	"dbugtrigger",	/* 26 */
	"bcopy",	/* 27 */
	"setitemowner",	/* 16 */
};

#define funccount	(sizeof(kernelswis)/sizeof(char *))

int
main(argc,argv)
int argc;
char **argv;
{
	uint32 *tbl = KernelBase->kb.f_DebugTable;
	pd_set *mytbl = (pd_set *)mybits;
	uint32 bits;
	int on = 1;
	int argindex = 1;
	int i;

	printf("gdbug start\n");

	if (!tbl)
	{
		printf("Debuggin not compiled in\n");
		return 0;
	}
	if (argc > 3)	return 0;
	if (argc == 3)
	{
	    if (strcmp(argv[1],"off") == 0)	on  = 0;
	    argindex = 2;
	}

	/* first look through table */

	for (i = 0; i < funccount; i++)
	{
	    if (strcmp(argv[argindex],kernelswis[i]) == 0) break;
	}

	if (i == funccount) 	i = strtol(argv[argindex],0,0);
	else 	i += KernelBase->kb.f_MaxUserFunctions;

	printf("i = %ld\n",i);

	memcpy(mybits,tbl,(1+(i/32))*sizeof(uint32));

	/* need to turn bit i on in the DebugTable */

	if (on) PD_Set(i,mytbl);	/* turn it on locally */
	else	PD_Clr(i,mytbl);	/* turn it off locally */

	bits = mybits[i/NPDBITS];	/* get the uint32 */

	SuperBCopy((int32 *)&bits,(int32 *)&tbl[i/NPDBITS],sizeof(uint32));
}
