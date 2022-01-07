/* $Id: getvramioreq.c,v 1.2 1994/09/14 23:56:57 vertex Exp $ */

#include "types.h"
#include "device.h"
#include "io.h"
#include "kernel.h"
#include "graphics.h"


/*****************************************************************************/


Item
GetVRAMIOReq (void)
{
  Item SPORTDevice;
  TagArg targs[2];

  SPORTDevice = FindDevice("SPORT");
  if (SPORTDevice<0) {
    return SPORTDevice;
  }

  if (IsItemOpened(KernelBase->kb_CurrentTask->t.n_Item, SPORTDevice)) {
    SPORTDevice = OpenItem (SPORTDevice, 0);
    if (SPORTDevice<0) {
      return SPORTDevice;
    }
  }

  targs[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
  targs[0].ta_Arg = (void *)SPORTDevice;
  targs[1].ta_Tag = TAG_END;

  return CreateItem(MKNODEID(KERNELNODE,IOREQNODE), targs);
}
