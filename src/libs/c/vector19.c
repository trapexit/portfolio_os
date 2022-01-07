/* $Id: vector19.c,v 1.1 1993/12/14 19:00:58 sdas Exp $ */

#include "types.h"
#include "folio.h"

extern struct KernelBase *KernelBase;

uint8
SetNodePri(Node *n, uint8 pri)
{
	uint8 oldpri;
	CALLFOLIORET(KernelBase,KBV_SETNODEPRI,(n,pri),oldpri,(uint8));
	return oldpri;
}
