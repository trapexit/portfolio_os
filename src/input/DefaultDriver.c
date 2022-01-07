/*****

$Id: DefaultDriver.c,v 1.11 1994/05/10 00:09:18 dplatt Exp $

$Log: DefaultDriver.c,v $
 * Revision 1.11  1994/05/10  00:09:18  dplatt
 * Zero out all private data variables, to prevent a crash if a pod
 * is unplugged and then plugged back in again.
 *
 * Revision 1.10  1994/03/19  01:15:58  dplatt
 * Don't bother trying to load a driverlet for device 0... this is an
 * illegal/reserved ID, and devices which send it are typically just
 * short-circuits on the bus.
 *
 * Revision 1.9  1993/12/16  00:02:36  dplatt
 * Driverlets loaded from files don't complete login until 0x80 bit in
 * first byte of real data goes to zero.  This is bogus... that bit
 * is only meaningful (to the uploader) for ROMful pods.
 *
 * Revision 1.8  1993/09/03  19:12:57  dplatt
 * XOR the ROM checksums with a magic number.  This prevents older
 * (buggy, crash-prone) versions of the Event Broker from trying to
 * execute the code from the ROM - they will see an immediate checksum
 * mismatch and abandon the download.
 *
 * Revision 1.7  1993/08/31  22:04:52  dplatt
 * Add RCS signature validation
 *

*****/

/*
  Copyright The 3DO Company Inc., 1993
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  DefaultDriver.c - Control Port driverlet code for devices which
                    don't have a built-in driverlet.  Includes ROM
                    bootstrap code.
*/

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

#include "super.h"

#include "event.h"
#include "hardware.h" 
#include "controlport.h"

#include "event.h"
#include "poddriver.h"
#include "podrom.h"

#include "filestream.h"
#include "filestreamfunctions.h"

#ifdef ARMC
#include "stdio.h"
#else
#include <stdlib.h>
#endif

extern int32 debugFlag;
extern List sharedDrivers;
extern List podDrivers;
extern int32 driverletInitiatedChange;

/* #define DEBUG */

#ifdef DEBUG
# define DBUG(x)  printf x
#else
# define DBUG(x) /* x */
#endif

#ifdef DEBUG2
# define DBUG2(x)  printf x
#else
# define DBUG2(x) /* x */
#endif

#ifdef PRODUCTION
# define DBUG0(x) /* x */
# define qprintf(x) /* x */
#else
# define DBUG0(x) printf x
# define qprintf(x) printf x
#endif

#include "strings.h"

extern uint32 SaveGlue, RestoreGlue;

/*
  Default driverlet.  Supports raw-bits read and write for devices of
  unknown nature.  Also supports ROM download and the spawning of new
  driverlets.

  Private data assignments:  0 is the pointer to the PodROM structure.
  2 is the ROM-loading phase.  3 is the pointer to the image being
  loaded.  4 is the byte-count of the ROM data area.  5 is the current
  offset being probed.  6 is for the output bits.  7 is for the flags
  to be assigned to the driver once it is built.
*/

#define DD_PodROM          0
#define DD_JoinDriver      1
#define DD_LoadPhase       2
#define DD_ImageAddress    3
#define DD_ROMSize         4
#define DD_ROMOffset       5
#define DD_OutputBits      6
#define DD_DriverFlags     7

/* 
  The OutputBits private data entry has the ROM load commands in the
  LSB. The output routine is responsible for making sure that these bits
  actually end up in the MSB of the real device output register.
*/

enum LoadPhase 
{
  InitNewPod = 0,
  WaitForQuiet = 1,
  LoadHeader = 2,
  LoadImage = 3,
  BootNewDriverlet = 4,
  JoinExistingDriverlet = 5,
  Cleanup = 6,
  Moribund = 7
};

static int32 Checksum (uint8 *what, int32 howMany)
{
  int32 ck;
  ck = 0;
  DBUG(("Checksum %d bytes\n", howMany));
  while (--howMany >= 0) {
#ifdef NOTDEF
    if (*what != 0) { DBUG0(("%x ", *what)); }
#endif
    ck += *what++;
  }
  return ck ^ PODROM_CHECKSUM_MAGIC_BITS;
}

#ifdef OWNHEXING

static void ConstructDriverletPath(char path[32], int32 type)
{
  uint32 residue, digits, index;
  static char hex[17] = "0123456789ABCDEF";
  memset(path, '\0', 32);
  strcpy(path, "$DRIVERS/CPORT");
  residue = (uint32) type;
  digits = 0;
  do {
    residue = residue >> 4;
    digits ++;
  } while (residue != 0 && digits <= 8);
  residue = (uint32) type;
  index = 14; /* strlen of $DRIVERS/CPORT */
  do {
    digits --;
    path[index+digits] = hex[residue & 0x0F];
    residue = residue >> 4;
  } while (digits > 0);
  strcat(path, ".ROM");
}

#endif

Err DefaultDriver(PodInterface *interfaceStruct)
{
  Pod *pod;
  uint8 *base, *nextByte;
  uint8 streamByte;
  int32 inBits;
  uint8 jjj, kkkk;
  PodROM *podROM, *image;
  Stream *stream;
  int32 inBytes, packetBytes, dataBytes, toMove, offset, romAddress, hit;
  int32 cksum, i, missedBy;
  int32 imageSize;
  int32 wipeout;
  uint32 outpacket;
  uint32 blEntry;
  int32 romful;
  int32 RSAkey;
  typedef Err (*vector)();
  vector initializationLoc;
  PodInterface pi;
  PodDriver *newDriver;
  char driverPath[32];
  stream = NULL;
  wipeout = FALSE;
  pod = interfaceStruct->pi_Pod;
  switch (interfaceStruct->pi_Command) {
  case PD_InitDriver:
    DBUG(("Default driverlet init\n"));
    break;
  case PD_InitPod:
  case PD_ReconnectPod:
    DBUG(("Default driver pod init\n"));
    podROM = (PodROM *) malloc(sizeof (PodROM));
    memset(pod->pod_PrivateData, 0, sizeof pod->pod_PrivateData);
    pod->pod_PrivateData[DD_PodROM] = (uint32) podROM;
    if (podROM) {
      pod->pod_PrivateData[DD_LoadPhase] = InitNewPod;
    } else {
      pod->pod_PrivateData[DD_LoadPhase] = Cleanup;
    }
    pod->pod_Flags = 0;
    pod->pod_Blipvert = TRUE;
    break;
  case PD_ParsePodInput:
    base = interfaceStruct->pi_ControlPortBuffers->
      mb_Segment[MB_INPUT_SEGMENT].bs_SegmentBase + pod->pod_InputByteOffset;
    switch (pod->pod_PrivateData[DD_LoadPhase]) {
    case InitNewPod:
      if (pod->pod_Type == 0x00) { /* bogus ID, probable hardware failure */
	pod->pod_PrivateData[DD_LoadPhase] = Moribund;
	break;
      } else if (((pod->pod_Type & 0xC0) == 0x00 || pod->pod_Type > 0xFF) &&
	  (base[1] & 0x80)) {
	romful = TRUE;
      } else {
	romful = FALSE;
      }
      if (romful) {
	DBUG(("ROMful device, starting upload\n"));
	pod->pod_PrivateData[DD_LoadPhase] = WaitForQuiet;
	pod->pod_PrivateData[DD_ROMOffset] = 0;
	pod->pod_PrivateData[DD_OutputBits] = PODROM_CompleteReset;
	pod->pod_Blipvert = TRUE;
      } else {
	DBUG(("ROMless device, looking for driver on disk\n"));
	pod->pod_PrivateData[DD_LoadPhase] = Cleanup;
	if (LookupItem(FILEFOLIO) == NULL) {
	  DBUG(("Filesystem is not up, cannot load\n"));
	} else {
#ifdef OWNHEXING
	  ConstructDriverletPath(driverPath, pod->pod_Type);
#else
	  sprintf(driverPath, "$DRIVERS/CPORT%x.ROM", pod->pod_Type);
#endif
	  DBUG(("Driver path is %s\n", driverPath));
	  stream = OpenDiskStream(driverPath, 0);
	  if (stream) {
	    DBUG(("Opened driver file!\n"));
	    podROM = (PodROM *) pod->pod_PrivateData[DD_PodROM];
	    i = ReadDiskStream(stream, (char *) podROM, sizeof (PodROM));
	    if (i == sizeof (PodROM)) {
	      DBUG(("Got driver\n"));
	      goto ChecksumHeader;
	    } else {
	      DBUG(("Expected %d bytes, got %d\n", sizeof (PodROM), i));
	    }
	  } else {
	    DBUG(("No driver file\n"));
	  }
	}
      }
break;
    case WaitForQuiet:
    case LoadHeader:
    case LoadImage:
    case BootNewDriverlet:
    case JoinExistingDriverlet:
      pod->pod_Blipvert = TRUE;
      podROM = (PodROM *) pod->pod_PrivateData[DD_PodROM];
      streamByte = base[1];
      if (((pod->pod_Type & 0xC0) == 0x00 || pod->pod_Type > 0xFF) &&
	  (base[1] & 0x80)) {
	romful = TRUE;
      } else {
	romful = FALSE;
      }
#ifndef PRODUCTION
      DBUG(("Chew "));
      for (i = 0; i < 10; i++) {
	DBUG(("0x%x ", base[i]));
      }
      DBUG(("\n"));
#endif
      if (pod->pod_Type > 0xFF) {
	offset = 3;
      } else {
	offset = 2;
      }
      jjj = (streamByte >> 4) & 0x07;
      kkkk = streamByte & 0x0F;
      if (jjj == 0x7) {
	offset ++;
      }
      if (kkkk == 0x0F) {
	offset ++;
      }
      inBits = pod->pod_BitsIn;
      inBytes = inBits / 8;
      packetBytes = inBytes - offset;
      DBUG(("Uploader: %d bytes total, %d in upload packet\n", inBytes, packetBytes));
      if (!romful || packetBytes < 4 || (base[offset] & 0x80) == 0) {
	DBUG(("Short or non-upload packet\n"));
	switch (pod->pod_PrivateData[DD_LoadPhase]) {
	case WaitForQuiet:
	  pod->pod_PrivateData[DD_LoadPhase] = LoadHeader;
	  pod->pod_PrivateData[DD_OutputBits] = PODROM_UploadIncr;
	  DBUG(("Switching to upload/increment\n"));
	  break;
	case LoadHeader:
	case LoadImage:
	  pod->pod_PrivateData[DD_OutputBits] = PODROM_UploadIncr;
	  DBUG(("Awaiting a real data packet\n"));
	  break;
	case BootNewDriverlet:
	  DBUG(("Booting driverlet\n"));
	  image = (PodROM *) pod->pod_PrivateData[DD_ImageAddress];
	  newDriver = (PodDriver *) malloc(sizeof (PodDriver));
	  if (!newDriver) {
	    DBUG(("Could not malloc new driver\n"));
	    wipeout = TRUE;
	    break;
	  }
	  free((char *) pod->pod_PrivateData[DD_PodROM]);
	  pod->pod_PrivateData[DD_PodROM] = 0;
	  memset(newDriver, 0, sizeof (PodDriver));
	  newDriver->pd_FamilyCode = image->pr_Header.prh_FamilyCode;
	  newDriver->pd_FamilyVersion = image->pr_Header.prh_FamilyVersion;
	  newDriver->pd_DriverArea = image;
	  newDriver->pd_DriverAreaSize = pod->pod_PrivateData[DD_ROMSize];
	  newDriver->pd_Flags = pod->pod_PrivateData[DD_DriverFlags];
	  newDriver->pd_UseCount = 1;
	  initializationLoc = (vector) &image->pr_aif;
	  DBUG(("NO PRISONERS!!!!!!\n"));
	  blEntry = image->pr_aif.aif_blEntry;
	  image->pr_aif.aif_blEntry = RestoreGlue;
	  image->pr_aif.aif_blDecompress = SaveGlue;
	  (void) (*initializationLoc)();
	  DBUG(("Hey, I survived initialization!\n"));
	  image->pr_aif.aif_blEntry = blEntry;
	  initializationLoc = 
	    (vector) (((char *) &image->pr_aif.aif_blEntry) +
		      ((blEntry & 0x00FFFFFF) * 4) + 8);
	  DBUG(("Initialization location is 0x%x\n", initializationLoc));
	  DBUG(("Branch is to 0x%x\n", blEntry));
	  DBUG(("KernelBase is 0x%x\n", KernelBase));
	  DBUG(("OK, dudes, here we go...\n"));
	  memcpy(&pi, interfaceStruct, sizeof pi);
	  pi.pi_Command = PD_InitDriver;
	  newDriver->pd_DriverEntry = initializationLoc;
	  /* the following calls should be error-checked */
	  (void) (*initializationLoc)(&pi, KernelBase);
	  DBUG(("Driver initialized!\n"));
	  pi.pi_Command = PD_InitPod;
	  (void) (*initializationLoc)(&pi, KernelBase);
	  DBUG(("Pod initialized!\n"));
	  pod->pod_Driver = newDriver;
	  if (newDriver->pd_Flags & PD_LOADED_FROM_FILE) {
	    newDriver->pd_Flags |= PD_SHARED;
	    AddHead(&podDrivers, (Node *) newDriver);
	  } else if (newDriver->pd_FamilyCode != 0) {
	    newDriver->pd_Flags |= PD_SHARED;
	    AddTail(&sharedDrivers, (Node *) newDriver);
	  }
	  driverletInitiatedChange = TRUE;
	  break;
	case JoinExistingDriverlet:
	  DBUG(("Join-new-driver:  here we go!\n"));
	  free((char *) pod->pod_PrivateData[DD_PodROM]);
	  pod->pod_PrivateData[DD_PodROM] = 0;
	  newDriver = (PodDriver *) pod->pod_PrivateData[DD_JoinDriver];
	  memcpy(&pi, interfaceStruct, sizeof pi);
	  pi.pi_Command = PD_InitPod;
	  (void) (*newDriver->pd_DriverEntry)(&pi, KernelBase);
	  DBUG(("Pod initialized!\n"));
	  pod->pod_Driver = newDriver;
	  driverletInitiatedChange = TRUE;
	  break;
	}
	break;
      }
      if (pod->pod_PrivateData[DD_LoadPhase] == WaitForQuiet ||
	  pod->pod_PrivateData[DD_LoadPhase] == BootNewDriverlet ||
	  pod->pod_PrivateData[DD_LoadPhase] == JoinExistingDriverlet) {
	pod->pod_PrivateData[DD_OutputBits] = PODROM_NoUpload;
	break;
      }
      dataBytes = packetBytes - 3;
      romAddress = ((uint32) base[offset+1] << 8) + base[offset+2];
      DBUG(("Uploader: %d bytes for offset 0x%x\n", dataBytes, romAddress));
      if (romAddress % dataBytes != 0) {
	DBUG(("Counter out of sync, start over\n"));
	pod->pod_PrivateData[DD_OutputBits] = 0;
	pod->pod_PrivateData[DD_ROMOffset] = 0;
	break;
      }
      missedBy = romAddress - pod->pod_PrivateData[DD_ROMOffset];
      if (missedBy != 0) {
#ifdef NOTDEF
	if (pod->pod_PrivateData[DD_LoadPhase] == LoadImage) {
	  DBUG(("Image: %d bytes for offset 0x%x, missed by %d\n", dataBytes, romAddress, missedBy));
	}
#endif
	DBUG(("Missed by 0x%x;  ", missedBy));
	if (missedBy > 0 && missedBy < 0x1000) {
	  DBUG(("Uploader: %d bytes for offset 0x%x\n", dataBytes, romAddress));
	  DBUG(("try indexing downwards\n"));
	  pod->pod_PrivateData[DD_OutputBits] = PODROM_UploadDecr;
	} else if (missedBy < 0 || (missedBy >= 0xE000 & missedBy <= 0xFFFF)) {
	  DBUG(("try indexing upwards\n"));
	  pod->pod_PrivateData[DD_OutputBits] = PODROM_UploadIncr;
	} else {
	  DBUG(("really out of sync, start over\n"));
	  pod->pod_PrivateData[DD_OutputBits] = 0;
	  pod->pod_PrivateData[DD_ROMOffset] = PODROM_NoUpload;
	}
	break;
      }
      DBUG(("Take %d bytes, offset 0x%x\n", dataBytes, romAddress));
#ifndef PRODUCTION
      DBUG(("Chew "));
      for (i = 0; i < 10; i++) {
	DBUG(("0x%x ", base[i]));
      }
      DBUG(("\n"));
#endif
      nextByte = base + offset + 3;
      switch (pod->pod_PrivateData[DD_LoadPhase]) {
      case LoadHeader:
	toMove = sizeof (PodROM) - romAddress;
	if (toMove > dataBytes) {
	  toMove = dataBytes;
	}
	if (toMove > 0) {
	  DBUG(("Move %d bytes to offset 0x%x\n", toMove, romAddress));
	  DBUG(("Private-data ROM offset is 0x%x\n", pod->pod_PrivateData[DD_ROMOffset]));
	  memcpy(romAddress + (char *) podROM, nextByte, toMove);
	  pod->pod_PrivateData[DD_ROMOffset] += toMove;
	  DBUG(("Private-data ROM offset is now 0x%x\n", pod->pod_PrivateData[DD_ROMOffset]));
	  if (pod->pod_PrivateData[DD_ROMOffset] >= sizeof (PodROM)) {
	    pod->pod_PrivateData[DD_OutputBits] = 0;
	  ChecksumHeader:
	    pod->pod_PrivateData[DD_LoadPhase] = Cleanup;
	    DBUG(("Got the whole header, ought to verify it here\n"));
	    cksum =
	      Checksum (sizeof podROM->pr_Header.prh_HeaderChecksum + (uint8 *) podROM,
			sizeof (PodROM) - sizeof podROM->pr_Header.prh_HeaderChecksum);
	    DBUG(("Reported header checksum 0x%x, calculated 0x%x\n",
		   podROM->pr_Header.prh_HeaderChecksum, cksum));
	    if (cksum != podROM->pr_Header.prh_HeaderChecksum) {
	      wipeout = TRUE;
	      qprintf(("Bad ROM header checksum, URP!\n"));
	    } else if (podROM->pr_Header.prh_TotalByteCount <= sizeof (PodROM) ||
		podROM->pr_Header.prh_TotalByteCount > 0x10000) {
	      DBUG(("Bad byte count (0x%x), URP!\n", podROM->pr_Header.prh_TotalByteCount));
	    } else {
	      DBUG(("This ROM header looks OK\n"));
	      if (podROM->pr_Header.prh_FamilyCode != 0) {
		newDriver = (PodDriver *) FirstNode(&sharedDrivers);
		hit = FALSE;
		while (IsNode(&sharedDrivers, newDriver)) {
		  if (newDriver->pd_FamilyCode == podROM->pr_Header.prh_FamilyCode &&
		      newDriver->pd_FamilyVersion >= podROM->pr_Header.prh_FamilyVersion) {
		    hit = TRUE;
		    break;
		  }
		  newDriver = (PodDriver *) NextNode(newDriver);
		}
		if (hit) {
		  DBUG(("Using an existing shared driverlet\n"));
		  pod->pod_PrivateData[DD_JoinDriver] = (uint32) newDriver;
		  pod->pod_PrivateData[DD_LoadPhase] = JoinExistingDriverlet;
		  pod->pod_PrivateData[DD_OutputBits] = PODROM_NoUpload;
		  break;
		}
	      }
	      imageSize = podROM->pr_aif.aif_ImageROsize +
		podROM->pr_aif.aif_ImageRWsize +
		  podROM->pr_aif.aif_ZeroInitSize;
	      if (imageSize < podROM->pr_Header.prh_TotalByteCount) {
		imageSize = podROM->pr_Header.prh_TotalByteCount;
	      }
	      image = (PodROM *) malloc(imageSize);
	      if (!image) {
		DBUG(("Could not allocate image area of %d bytes\n", imageSize));
		break;
	      }
	      DBUG(("Image buffer 0x%x is %d bytes\n", image, imageSize));
	      memset(image, 0, imageSize);
	      pod->pod_PrivateData[DD_ImageAddress] = (int32) image;
	      pod->pod_PrivateData[DD_ROMSize] = imageSize;
	      memcpy(image, podROM, sizeof (PodROM));
	      if (stream) {
		DBUG(("Read image from disk\n"));
		i = ReadDiskStream(stream,
				   sizeof (PodROM) + (char *) image,
				   imageSize - sizeof (PodROM));
		/*
		   We could check the number of bytes read against the
		   actual file size, or against the minimum expected size
		   from the AIF header, but what the hell... the checksum
		   should catch bad driver files, I hope.
		*/
		goto ChecksumImage;
		break;
	      }
	      pod->pod_PrivateData[DD_LoadPhase] = LoadImage;
	      if (pod->pod_PrivateData[DD_ROMOffset] != sizeof (PodROM)) {
		pod->pod_PrivateData[DD_ROMOffset] = 0;
		pod->pod_PrivateData[DD_OutputBits] = PODROM_UploadDecr;
		DBUG(("Restart from zero\n"));
	      } else {
		pod->pod_PrivateData[DD_OutputBits] = PODROM_UploadIncr;
		DBUG(("Resume from here\n"));
	      }
	      DBUG(("Start image upload!\n"));
	    }
	  }
	}
	break;
      case LoadImage:
	image = (PodROM *) pod->pod_PrivateData[DD_ImageAddress];
	toMove = pod->pod_PrivateData[DD_ROMSize] - romAddress;
	if (toMove > dataBytes) {
	  toMove = dataBytes;
	}
	if (toMove > 0) {
	  DBUG(("Move %d bytes to offset 0x%x\n", toMove, romAddress));
	  DBUG(("Private-data ROM offset is 0x%x\n", pod->pod_PrivateData[DD_ROMOffset]));
	  memcpy(romAddress + (char *) image, nextByte, toMove);
	  pod->pod_PrivateData[DD_ROMOffset] += toMove;
	  DBUG(("Private-data ROM offset is now 0x%x\n", pod->pod_PrivateData[DD_ROMOffset]));
	  if (pod->pod_PrivateData[DD_ROMOffset] >= podROM->pr_Header.prh_TotalByteCount) {
	  ChecksumImage:
	    pod->pod_PrivateData[DD_LoadPhase] = Cleanup;
	    pod->pod_PrivateData[DD_OutputBits] = PODROM_NoUpload;
	    DBUG(("Got total image\n"));
	    cksum =
	      Checksum ((uint8 *) &image->pr_aif,
			image->pr_Header.prh_TotalByteCount - sizeof image->pr_Header);
	    DBUG(("Reported image checksum 0x%x, calculated 0x%x\n",
		   image->pr_Header.prh_ImageAreaChecksum, cksum));
	    if (cksum == image->pr_Header.prh_ImageAreaChecksum) {
	      DBUG(("Image checksum matches!\n"));
	      DBUG(("RSA-checking %d bytes at 0x%x\n",
		     image->pr_Header.prh_TotalByteCount -
		     sizeof image->pr_Header,
		     &image->pr_aif));
	      RSAkey =  RSACheck((uchar *) &image->pr_aif,
				 image->pr_Header.prh_TotalByteCount -
				 sizeof image->pr_Header);
	      DBUG(("RSA validation returned key %d\n", RSAkey));
	      if (RSAkey != 1 /* 3DO key */) {
		qprintf(("Driverlet AIF is not properly signed!\n"));
		wipeout = TRUE;
	      } else {
		pod->pod_PrivateData[DD_LoadPhase] = BootNewDriverlet;
		if (stream) {
		  pod->pod_PrivateData[DD_DriverFlags] =
		    PD_LOADED_FROM_FILE | PD_LOADED_INTO_RAM;
		} else {
		  pod->pod_PrivateData[DD_DriverFlags] =
		    PD_LOADED_FROM_ROM | PD_LOADED_INTO_RAM;
		}
	      }
	    } else {
	      qprintf(("Image checksum mismatch!\n"));
	      wipeout = TRUE;
	    }
	  }
	}
      }
      break;
    case Cleanup:
      wipeout = TRUE;
      break;
    case Moribund:
      break;
    }
    break;
  case PD_ProcessCommand:
    interfaceStruct->pi_CommandOutLen = 0;
    break;
  case PD_ConstructPodOutput:
    /* this won't work if there are more than 32 bits out - fix it! */
    outpacket = pod->pod_PrivateData[DD_OutputBits] << (pod->pod_BitsOut - 8);
    DBUG(("Output 0x%x\n", outpacket));
    (*interfaceStruct->pi_PackBits)
      (outpacket,
       pod->pod_BitsOut,
       FALSE,
       interfaceStruct->pi_ControlPortBuffers,
       MB_OUTPUT_SEGMENT);
    break;
  case PD_TeardownPod:
    wipeout = TRUE;
    break;
  default:
    break;
  }
  if (stream) {
    CloseDiskStream(stream);
  }
  if (wipeout) {
    pod->pod_PrivateData[DD_LoadPhase] = Moribund;
    if (pod->pod_PrivateData[DD_PodROM]) {
      DBUG(("Freeing pod private data at 0x%X\n", pod->pod_PrivateData[DD_PodROM]));
      free((char *) pod->pod_PrivateData[DD_PodROM]);
      pod->pod_PrivateData[DD_PodROM] = 0;
      DBUG(("Freed.\n"));
    }
    if (pod->pod_PrivateData[DD_ImageAddress]) {
      free((char *) pod->pod_PrivateData[DD_ImageAddress]);
      pod->pod_PrivateData[DD_ImageAddress] = 0;
    }
  }  
  return 0;
}

