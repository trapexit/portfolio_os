/* $Id: MySysErr.c,v 1.5 1994/07/26 21:08:34 dplatt Exp $ */

/* for now we do not worry about a particular memory type */
/* Like chip memory or public memory */

extern void Superkprintf(const char *fmt, ...);
#include "types.h"

#include "nodes.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "strings.h"

#include "operror.h"

void
MySysErr(i)
Item i;
{
#ifndef ROMBUILD
  char errorBuffer[256];
  if (KernelBase->kb_CPUFlags & KB_NODBGR) {
    return;
  }
  GetSysErr(errorBuffer, sizeof errorBuffer, i);
  Superkprintf("%s\n", errorBuffer);
#endif
}
