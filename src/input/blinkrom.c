/* $Id: blinkrom.c,v 1.2 1994/02/09 01:22:45 limes Exp $ */
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

#define DBUG0(x) kprintf x
#define DBUG(x) /* x */

Err DriverletEntry(PodInterface *pi, struct KernelBase *base) {
  Pod *pod;
  pod = pi->pi_Pod;
  switch (pi->pi_Command) {
  case PD_InitDriver:
    DBUG0(("Blink driver init\n"));
    break;
  case PD_InitPod:
    DBUG0(("Blink pod init\n"));
    pod->pod_PrivateData[0] = 0x1;
    pod->pod_PrivateData[1] = 10;
    pod->pod_PrivateData[2] = 0;
    pod->pod_Flags = 0;
    break;
  case PD_ParsePodInput:
    DBUG(("Blink device parse\n"));
    pod->pod_Blipvert = TRUE;
    pod->pod_EventsReady[0] = 0;
    break;
  case PD_AppendEventFrames:
    break;
  case PD_ConstructPodOutput:
    DBUG(("Blink device outgen 0x%x 0x%x 0x%x\n",
	  pod->pod_PrivateData[0],
	  pod->pod_PrivateData[1],
	  pod->pod_PrivateData[2]));
    (*pi->pi_PackBits)(pod->pod_PrivateData[0],
		       8,
		       FALSE,
		       pi->pi_ControlPortBuffers,
		       MB_OUTPUT_SEGMENT);
    DBUG(("Output packed\n"));
    if (pod->pod_PrivateData[1]-- == 0) {
      pod->pod_PrivateData[1] = 10;
      if (pod->pod_PrivateData[2]) {
	pod->pod_PrivateData[0] >>= 1;
	if (pod->pod_PrivateData[0] == 0x1) {
	  pod->pod_PrivateData[2] = 0;
	}
      } else {
	pod->pod_PrivateData[0] <<= 1;
	if (pod->pod_PrivateData[0] == 0x80) {
	  pod->pod_PrivateData[2] = 1;
	}
      }
    }
    break;
  default:
    break;
  }
  return 0;
}
