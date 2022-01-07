/* $Id: hellorom.c,v 1.2 1994/02/09 01:22:45 limes Exp $ */
#include "types.h"
#include "item.h"
#include "kernel.h"
#include "mem.h"
#include "nodes.h"
#include "debug.h"
#include "list.h"
#include "device.h"
#include "driver.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "io.h"
#include "operror.h"
#include "aif.h"

#include "stdio.h"
#include "poddriver.h"

struct KernelBase *localBase;

Err DriverletEntry(PodInterface *pi, struct KernelBase *base) {
  kprintf("Hello, world!\n");
  kprintf("Interface is 0x%x, kernelbase is 0x%x\n", pi, base);
  return 0;
}
