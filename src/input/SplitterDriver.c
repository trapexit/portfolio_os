/*****

$Id: SplitterDriver.c,v 1.2 1993/06/16 00:49:25 dplatt Exp $

$Log: SplitterDriver.c,v $
 * Revision 1.2  1993/06/16  00:49:25  dplatt
 * Fix ROMloader, improve splitter support, reduce footprint
 *
 * Revision 1.1  1993/06/14  21:17:44  dplatt
 * Dragon beta
 *

*****/

/*
  Copyright The 3DO Company Inc., 1993
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  SplitterDriver.c - Control Port driverlet code for 3DO Splitters.

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

#include "event.h"
#include "hardware.h" 
#include "controlport.h"

#include "event.h"
#include "poddriver.h"

#ifdef ARMC
#include "stdio.h"
#else
#include <stdlib.h>
#endif

extern int32 debugFlag;

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

#define DBUG0(x) if (debugFlag) { printf x ; }

#include "strings.h"

/*
  3DO Splitter driverlet.

  Private data assignments:

  0 is the state machine.
  1 is the output data (32 bits worth).

*/

enum SplitterState {
  SplitterUncertain =   0, /***** must be zero! *****/
  SplitterCooldown =    1,
  SplitterConfiguring = 2,
  SplitterConfigured =  3
};

#define LATCHBIT 0x01000000

Err SplitterDriver(PodInterface *interfaceStruct)
{
  Pod *pod, *successor;
  uint32 rawBits;
  uint32 bitsIn, bitsOut;
  uint32 misconfig, unstable;
  uint8 *base;
  pod = interfaceStruct->pi_Pod;
  DBUG(("Splitter driver, function %d\n", interfaceStruct->pi_Command));
  switch (interfaceStruct->pi_Command) {
  case PD_InitDriver:
    DBUG0(("Splitter driverlet init\n"));
    break;
  case PD_InitPod:
    DBUG0(("Splitter pod init\n"));
    memset(pod->pod_PrivateData, 0, sizeof pod->pod_PrivateData);
    pod->pod_Flags = 0 /* POD_IsSplitter */;
    break;
  case PD_ParsePodInput:
    DBUG(("Splitter parse data\n"));
    base = interfaceStruct->pi_ControlPortBuffers->
	mb_Segment[MB_INPUT_SEGMENT].bs_SegmentBase + pod->pod_InputByteOffset;
    rawBits = ((((uint32) base[1] << 8) + (uint32) base[2]) << 8) + (uint32) base[3];
    switch (pod->pod_PrivateData[0]) {
    case SplitterUncertain:
      DBUG0(("Uncertain, sending zeroes\n"));
      pod->pod_PrivateData[1] = LATCHBIT;
      pod->pod_PrivateData[0] = SplitterCooldown;
      pod->pod_Blipvert = TRUE;
      break;
    case SplitterCooldown:
      DBUG0(("Cooldown, sending zeroes\n"));
      pod->pod_PrivateData[1] = LATCHBIT;
      if (rawBits == 0) {
	pod->pod_PrivateData[0] = SplitterConfiguring;
      }
      pod->pod_Blipvert = TRUE;
      break;
    case SplitterConfiguring:
      DBUG0(("Configuring\n"));
      pod->pod_Blipvert = TRUE;
      pod->pod_PrivateData[1] = LATCHBIT;
      if (rawBits != 0) {
	pod->pod_PrivateData[0] = SplitterCooldown;
	break;
      }
      bitsIn = bitsOut = 0;
      successor = (Pod *) NextNode(pod);
      misconfig = FALSE;
      unstable = FALSE;
      while (NextNode(successor) != NULL) { /* scan to end of list */
	DBUG0(("Port 1 type 0x%x\n", successor->pod_Type));
	if (successor->pod_Type == 0xFE /* air */ ||
	    successor->pod_Type == 0x56 /* splitter 1 */ ||
	    successor->pod_Type == 0x57 /* splitter 2 */) {
	  misconfig = TRUE;
	  DBUG0(("Misconfigured!"));
	  break;
	}
	if (pod->pod_LoginLogoutPhase != POD_Online) {
	  DBUG0(("Not yet online\n"));
	  unstable = TRUE;
	  break;
	}
	bitsIn += successor->pod_BitsIn;
	bitsOut += successor->pod_BitsOut;
	successor = (Pod *) NextNode(successor);
      }
      if (misconfig) {
	pod->pod_PrivateData[1] = 0;
	pod->pod_PrivateData[0] = SplitterUncertain;
	break;
      }
      if (unstable) {
	pod->pod_PrivateData[1] = 0;
	break;
      }
      DBUG0(("Port 1 bits-in %d bits-out %d\n", bitsIn, bitsOut));
      bitsIn += 8; /* account for air frame */
      bitsOut += pod->pod_BitsOut;
      pod->pod_PrivateData[1] = ((bitsIn / 8) << 11) |
	(bitsOut / 2) | LATCHBIT;
      DBUG0(("Write output bits 0x%x\n", pod->pod_PrivateData[1]));
      pod->pod_PrivateData[0] = SplitterConfigured;
      break;
    case SplitterConfigured:
      DBUG(("Configured!\n"));
      if (rawBits == 0) {
	pod->pod_Blipvert = TRUE;
      } else if (rawBits != (pod->pod_PrivateData[1] & 0x00FFFFFF)) {
	DBUG0(("Bad data readback (0x%x != 0x%x, reconfiguring!\n",
	       rawBits, pod->pod_PrivateData[1]));
	pod->pod_PrivateData[1] = LATCHBIT;
	pod->pod_PrivateData[0] = SplitterUncertain;
	pod->pod_Blipvert = TRUE;
      }
      break;
    }
    break;
  case PD_AppendEventFrames:
    break;
  case PD_ProcessCommand:
    interfaceStruct->pi_CommandOutLen = 0;
    break;
  case PD_ConstructPodOutput:
    DBUG(("Build output for a Splitter: 0x%x\n", pod->pod_PrivateData[1]));
    (*interfaceStruct->pi_PackBits)(pod->pod_PrivateData[1], 
				    32,
				    FALSE,
				    interfaceStruct->pi_ControlPortBuffers,
				    MB_OUTPUT_SEGMENT);
    break;
  default:
    break;
  }
  return 0;
}

Err EndOfSplitterDriver(PodInterface *interfaceStruct)
{
  return 0;
}
