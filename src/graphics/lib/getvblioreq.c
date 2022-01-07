/* $Id: getvblioreq.c,v 1.2 1994/09/14 23:56:57 vertex Exp $ */

#include "types.h"
#include "device.h"
#include "io.h"
#include "kernel.h"
#include "graphics.h"


/*****************************************************************************/


Item
GetVBLIOReq (void)
{
  Item TimerDeviceItem;
  TagArg targs[2];

  TimerDeviceItem = FindDevice("timer");
  if (TimerDeviceItem<0) {
    return TimerDeviceItem;
  }
  if (IsItemOpened(KernelBase->kb_CurrentTask->t.n_Item,TimerDeviceItem)) {
    TimerDeviceItem = OpenItem (TimerDeviceItem, 0);
    if (TimerDeviceItem<0) {
      return TimerDeviceItem;
    }
  }

  targs[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
  targs[0].ta_Arg = (void *)TimerDeviceItem;
  targs[1].ta_Tag = TAG_END;

  return CreateItem(MKNODEID(KERNELNODE,IOREQNODE), targs);
}
