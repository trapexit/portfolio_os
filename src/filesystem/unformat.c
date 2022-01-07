/*****

$Id: unformat.c,v 1.2 1994/08/29 18:25:17 sdas Exp $

$Log: unformat.c,v $
 * Revision 1.2  1994/08/29  18:25:17  sdas
 * CallBackSuper() now takes three parameters.
 *
 * Revision 1.1  1994/06/27  23:06:02  limes
 * Initial revision
 *
 *****/

#include "types.h"
#include "super.h"
#include "operror.h"

/* unformat: scrog the nvram in an opera system. */

extern void	FillMemoryWithWord(uint32 *ptr, uint32 size, uint32 pat);

void cback (void)
{
    FillMemoryWithWord((uint32 *)0x3140000, 0x20000, 0xFFFFFFFF);
    while(1);
}

int main(void)
{
    int	err;
    err = CallBackSuper((Err (*)())cback, 0, 0, 0);
    if (err < 0)
	PrintError(0, "CallBackSuper", "cback", err);
    return 0;
}
