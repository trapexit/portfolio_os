/* $Id: rsa.c,v 1.18 1994/02/09 01:22:45 limes Exp $ */

#include "types.h"
#include "rsa.h"
#include "aif.h"
#include "mem.h"
#include "time.h"
#include "stdio.h"
#include "internalf.h"

/* code for RSA and MD4 checking */
#include "debug.h"

#define DBUG(x)	/*printf x*/
#define DBUG1(x)	/*printf x*/

#define USE_DIPIR
#define SWAP_STACK

#ifdef SWAP_STACK
void *swapstack(void *newstack);
char *rsastack;

extern void Panic(int halt,char *);

#define RSA_STACKSIZE	6*1024	/* Yes, it really needs this much! */

void
RSAInit(void)
{
    rsastack = (char *)ALLOCMEM(RSA_STACKSIZE,MEMTYPE_FILL);
    /*printf("RSAStackBase=%lx\n",rsastack);*/
    if (!rsastack)
    {
#ifdef DEVELOPMENT
	printf("Error, could not allocate rsa stack\n");
	while (1);
#else
	Panic(1,"could not allocate rsa stack\n");
#endif
    }
    rsastack += RSA_STACKSIZE - 4;
}
#endif

int32
RSACheck(buff,buffsize)
uchar *buff;
int32 buffsize;
{
	int8 ret;
	bool (*dipir_rsa)() = (bool (*)())((char *)0x204);
#ifdef SWAP_STACK
	void *oldstack;
#endif
	DBUG(("RSACheck(%lx,%ld)\n",buff,buffsize));

#ifdef SWAP_STACK
	if (rsastack == 0)	RSAInit();
	DBUG(("rsastack=%lx\n",rsastack));

	oldstack = swapstack(rsastack);
#endif

#ifdef under
	TimeStamp(&start);
#endif

	ret = (int8)(*dipir_rsa)(buff,buffsize);
#ifdef undef
	TimeStamp(&stop);
	stop.tv_sec -= start.tv_sec;
	stop.tv_usec -= start.tv_usec;
	if (stop.tv_usec < 0)
	{
	     stop.tv_sec--;
	     stop.tv_usec += 1000000;
	}
	printf("time= %d secs, %d usecs\n",stop.tv_sec,stop.tv_usec);
#endif
#ifdef SWAP_STACK
	swapstack(oldstack);
#endif
	DBUG(("RSACheck results = %ld\n",ret));
	return ret;
}
