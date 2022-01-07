#ifndef __DRIVER_H
#define __DRIVER_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  $Id: driver.h,v 1.25 1994/11/01 19:04:42 bungee Exp $
**
**  Kernel driver management definitions
**
******************************************************************************/


#include "types.h"
#include "nodes.h"
#include "msgport.h"

#ifndef	IOReq_typedef
#define	IOReq_typedef
typedef struct IOReq IOReq;
#endif

#ifdef EXTERNAL_RELEASE

typedef struct Driver
{
	ItemNode drv;
	uint32   drv_Private;
	int32    drv_OpenCnt;
} Driver;

#else /* EXTERNAL_RELEASE */

/* The definition of this structure must not be embedded in other
 * public structures defined by the OS because it's size might need to
 * change in the future.
 */
typedef struct Driver
{
	ItemNode drv;
	MsgPort	*drv_MsgPort;
	int32	drv_OpenCnt;
	void	(*drv_AbortIO)(struct IOReq *iorP);
	int32	(*drv_DispatchIO)(struct IOReq *iorP);
	Item	(*drv_Init)(struct Driver *drvP); /* change to device init */
	int32	(**drv_CmdTable)(struct IOReq *iorP);
	uint8	drv_MaxCommands;
	uint8	drv_pad[3];
	void   *drv_DemandLoad;
} Driver;

#endif /* EXTERNAL_RELEASE */

#ifndef EXTERNAL_RELEASE
enum driver_tags
{
	CREATEDRIVER_TAG_ABORTIO = TAG_ITEM_LAST+1,
	CREATEDRIVER_TAG_MAXCMDS,
	CREATEDRIVER_TAG_CMDTABLE,
	CREATEDRIVER_TAG_MSGPORT,
	CREATEDRIVER_TAG_INIT,
	CREATEDRIVER_TAG_DISPATCH
};
#endif


typedef struct DeviceStatus
{
        uint8   ds_DriverIdentity;
	uint8   ds_DriverStatusVersion;
	uint8	ds_FamilyCode;
	uint8   ds_headerPad;
	uint32  ds_MaximumStatusSize;
	uint32  ds_DeviceBlockSize;
	uint32  ds_DeviceBlockCount;
	uint32  ds_DeviceFlagWord;
	uint32	ds_DeviceUsageFlags;
	uint32  ds_DeviceLastErrorCode;
	uint32	ds_DeviceMediaChangeCntr;
	uint32  ds_Reserved;
} DeviceStatus;

/*
  Values for ds_DriverIdentity
*/

#define DI_OTHER	       0x00

#define DI_XBUS                0x01
#define DI_TIMER               0x02
#define DI_RAM                 0x03
#define DI_CONTROLPORT         0x04
#define DI_FILE                0x05
#define DI_MEI_CDROM           0x06
#define DI_SPORT               0x07
#define DI_HOLLYWOOD           0x08
#define DI_LCCD_CDROM          0x09


/*
  Values for ds_FamilyCode.  This byte field contains the family
  identifier for this device.
*/

#define DS_DEVTYPE_OTHER       0

#define DS_DEVTYPE_CDROM       1
#define DS_DEVTYPE_HARDDISK    2
#define DS_DEVTYPE_FILE        3
#define DS_DEVTYPE_TAPE        4
#define DS_DEVTYPE_MODEM       5
#define DS_DEVTYPE_NETWORK     6
#define DS_DEVTYPE_CODEC       7


/*
  Values for ds_DeviceUsageFlags.  This field contains device
  characteristic/usage information.  (NOTE: ds_DeviceFlagWord
  is for driver specific flag usage.)
*/

#define DS_USAGE_FILESYSTEM    0x80000000
#define DS_USAGE_PRIVACCESS    0x40000000
#define DS_USAGE_READONLY      0x20000000
#define DS_USAGE_OFFLINE       0x10000000

#define DS_USAGE_FAST          0x00000002
#define DS_USAGE_SLOW          0x00000001

#ifndef EXTERNAL_RELEASE
typedef struct RamDeviceStatus
{
	DeviceStatus ramdev_ds;
	uint32		ramdev_DeviceAddress;	/* physical start of ramdisk */
} RamDeviceStatus;

typedef struct XBusDeviceStatus
{
	DeviceStatus	xbus_ds;
	uint32		xbus_ManuIdNum;
	uint32		xbus_ManuDevNum;
	uint32		xbus_ManuRevNum;
	uint32		xbus_ManuFlags;
	uint8		xbus_Flags;
	uint8		reserved[3];
	uint32          xbus_XferSpeed;
	uint32          xbus_BurstSize;
	uint32          xbus_HogTime;
} XBusDeviceStatus;

/* xbus_Flags */
#define	XBUS_OLDSTAT	1	/* status returned without prepending cmd byte */
#define XBUS_OLDMEI	2	/* ancient pre-ces poll register */
#define XBUS_ROMDRIVER	4	/* driver downloaded from device */
#define XBUS_DEVDEAD	0x80	/* this device is not active */

/* ManuFlags */
#define DEVHAS_DRIVER	0x500	/* Driver downloaded from Device rom */

#ifdef __cplusplus
extern "C" {
#endif

extern Item CreateDriver(const char *name, uint8 pri, Item (*init)());

/* CD-style ECC correction for drivers that need it. */
extern int32 SectorECC(uint8 *buffer);

#ifdef	__cplusplus
}
#endif	/* __cplusplus */

#define DeleteDriver(x)	DeleteItem(x)
#define FindDriver(n)   FindNamedItem(MKNODEID(KERNELNODE,DRIVERNODE),(n))
#endif /* EXTERNAL_RELEASE */


/*****************************************************************************/


#endif	/* __DRIVER_H */
