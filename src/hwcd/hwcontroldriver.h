#ifndef __HWCONTROLDRIVER_H
#define __HWCONTROLDRIVER_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  $Id: hwcontroldriver.h,v 1.4 1994/09/10 02:07:43 vertex Exp $
**
**  HW control driver interface definitions
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __ITEM_H
#include "item.h"
#endif

#ifndef __DEVICE_H
#include "device.h"
#endif

#ifndef __DRIVER_H
#include "driver.h"
#endif

#ifndef __IO_H
#include "io.h"
#endif

#ifndef __OPERROR_H
#include "operror.h"
#endif


#define HWCONTROL_DRIVER_NAME   "HWControlDriver"
#define HWCONTROL_DEVICE_NAME   "HWControlDevice0"

/* Driver-specific commands */
#define CMD_INTERLACE    3
#define CMD_NONINTERLACE 4


/*****************************************************************************/


#endif /* __HWCONTROLDRIVER_H */
