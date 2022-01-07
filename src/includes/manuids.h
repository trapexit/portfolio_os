/* $Id: manuids.h,v 1.2 1994/11/10 19:01:11 markn Exp $ */

/*
  File:  manuids.h

  Contains:  Manufacturer ids and device ids for devices

  Written by:  Tim Wiegman and Mark Nudelman

  Copyright:  (c) 1994 by The 3DO Company, all rights reserved.

  Change History (most recent first):

	 <2>	  11/4/94	markn	more devices
  	 <1>	  9/13/94	tlw	first checked in

  To Do:

  Note:

  Each entry in this file must begin with the fixed string MANU_ID_
  to identify manufacturer ids and MANU_DV_ to identify manufacturer devs.
*/

/************************************************************************/
/* Manufacturers */

#define	MANU_CO_3DO			0x1000	/* The 3DO Company */
#define	MANU_CO_SA			0x000E	/* Scientific Atlanta */
#define	MANU_CO_MKE			0x0010	/* For all MKE CD drives */
#define	MANU_CO_MEI			0x0020	/* MEI */
#define MANU_CO_UNKNOWN			0xFFFF	/* Unknown/unspecified */


/************************************************************************/
/* Devices */

/* MKE CD-560 (CD drive in original MEI FZ-1) */
#define	MANU_ID_MEICD			MANU_CO_MKE
#define	MANU_DV_MEICD			0x0001

/* 3DO EtherNicky network interface */
#define	MANU_ID_ETHERNICKY		MANU_CO_3DO
#define	MANU_DV_ETHERNICKY		0x0010

/* 3DO Digital Video Module */
#define	MANU_ID_FMV			MANU_CO_3DO
#define	MANU_DV_FMV			0x0020

/* 3DO-designed Low-cost CD drive */
#define	MANU_ID_LCCD   			MANU_CO_3DO
#define	MANU_DV_LCCD			0x0050

/* MEI "Nicky-like" generic interface */
#define	MANU_ID_GDD			MANU_CO_MEI
#define	MANU_DV_GDD			0x0070

/* 3DO Norton (IDE interface) */
#define	MANU_ID_NORTON			MANU_CO_3DO
#define	MANU_DV_NORTON			0x0080

/* 3DO Bridget interface */
#define	MANU_ID_BRIDGIT			MANU_CO_3DO
#define	MANU_DV_BRIDGIT			0x0100

/* MEI CD-563 (Creative Labs Soundblaster CD drive) */
#define	MANU_ID_MEICD563		MANU_CO_MKE
#define	MANU_DV_MEICD563		0x0563

/* MEI Storage Expander (no card slot) */
#define	MANU_ID_STOREX			MANU_CO_MEI
#define	MANU_DV_STOREX			0x0600

/* 3DO Nicky network interface board */
#define	MANU_ID_NICKY			MANU_CO_3DO
#define	MANU_DV_NICKY			0x1000

/* Scientific Atlanta Nicky */
#define	MANU_ID_SA_NICKY		MANU_CO_SA
#define	MANU_DV_SA_NICKY		0x1000

/************************************************************************/
/* Software pseudo-devices */

/* CD-dipir */
#define	MANU_ID_CDDIPIR			MANU_CO_3DO
#define	MANU_DV_CDDIPIR			0xF000

/************************************************************************/
/* Special devices */

/* Generic Depot device */
/* No one should be explicitly checking for this ID! */
#define	MANU_ID_DEPOT			MANU_CO_UNKNOWN
#define	MANU_DV_DEPOT			0x2
