/* $Id: bootdata.h,v 1.6 1994/11/29 18:16:45 markn Exp $ */

/*
 * This file describes the data structure passed from the boot programs
 * (for example, dipir) to sherry.
 */

#ifndef BOOTDATA_H
#define	BOOTDATA_H 1

#include "discdata.h"

typedef struct BootData
{
	uint32		bd_Version;		/* version of BootData struct */
	struct DiscLabel *bd_BootVolumeLabel;	/* Disc label of bool volume */
	uint32		bd_DevicePerms;		/* Device access permissions */
} BootData;

/* Bits in bd_DevicePerms */
#define	BOOTDATA_DEVICEPERM_AUDIOIN	0x0001	/* Audio-input */

#endif /* BOOTDATA_H */
