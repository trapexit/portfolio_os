#ifndef __DEVICE_H
#define __DEVICE_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  $Id: device.h,v 1.20 1994/12/09 21:22:41 vertex Exp $
**
**  Kernel device management definitions
**
******************************************************************************/


#include "types.h"
#include "item.h"
#include "nodes.h"
#include "list.h"
#include "kernelnodes.h"

#ifdef EXTERNAL_RELEASE

typedef struct Device
{
        ItemNode       dev;
        struct Driver *dev_Driver;
        int32          dev_OpenCnt;
        uint32         dev_Private0[13];
        uint8          dev_MaxUnitNum;  /* maximum allowed unit for this device */
        uint8          dev_Private1[3];
        uint32         dev_Private2[3];
} Device;

#else /* EXTERNAL_RELEASE */

/* The definition of this structure must not be embedded in other
 * public structures defined by the OS because it's size might need to
 * change in the future.
 */
typedef struct DeviceExtension
{
    uint32  de_StructSize;   /* size of this structure       */
    void   *de_DemandLoad;   /* private use by demand-loader */
} DeviceExtension;


typedef struct Device
{
	ItemNode	dev;
	struct Driver *dev_Driver;
	int32	dev_OpenCnt;
				/* device dependent initialization */
	int32	(*dev_CreateIOReq)(struct IOReq *iorP);
	int32	(*dev_DeleteIOReq)(struct IOReq *iorP);
	int32	(*dev_Open)(struct Device *devP);	/* called when Opened */
	void	(*dev_Close)(struct Device *devP);	/* called when Closed */
	int32	dev_IOReqSize;	/* size of ioreqs for this device */
	List	dev_IOReqs;	/* ioreqs created for this device */
	uint8	dev_MaxUnitNum;	/* maximum allowed unit for this device */
	uint8	dev_Reserved[3];
	int32	(*dev_Init)(struct Device *devP);	/* called at creation */
	int32	(*dev_DeleteDev)(struct Device *devP);	/* called before deleting this dev */
	DeviceExtension	*dev_Extension;
} Device;

enum device_tags
{
	CREATEDEVICE_TAG_DRVR = TAG_ITEM_LAST+1,
	CREATEDEVICE_TAG_CRIO,		/* createIO routine */
	CREATEDEVICE_TAG_DLIO,		/* deleteIO routine */
	CREATEDEVICE_TAG_OPEN,		/* open routine */
	CREATEDEVICE_TAG_CLOSE,		/* close routine */
	CREATEDEVICE_TAG_IOREQSZ,	/* optional request size */
	CREATEDEVICE_TAG_INIT		/* InitRoutine to call */
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern Item CreateDevice(const char *name, uint8 pri, Item driver);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#define DeleteDevice(x)	DeleteItem(x)
#endif /* EXTERNAL_RELEASE */

/* easy def for opening device without regard to version */
#define FindDevice(n)		FindNamedItem(MKNODEID(KERNELNODE,DEVICENODE),(n))
#define FindAndOpenDevice(n)	FindAndOpenNamedItem(MKNODEID(KERNELNODE,DEVICENODE),(n))
#define OpenNamedDevice(n,a)	FindAndOpenNamedItem(MKNODEID(KERNELNODE,DEVICENODE),(n))
#define CloseNamedDevice(dev)	CloseItem(dev)


/*****************************************************************************/


#endif	/* __DEVICE_H */
