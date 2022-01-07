# $Id: GNUmakefile,v 1.7 1995/01/08 14:52:47 vertex Exp $
# GNUmakefile for the fmv device
#
# Copyright (c) 1994, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all


###############################################################################
# Describe our files
#
DATE      = BuildDate
LOCALS    = fmvvideodevice.dev fmvvideodevice.rom
PGMS      = CL45016Bit.UCode CL45024Bit.UCode CL450Boot.UCode
DPGMS     = fmvvideodevice.privdevice.dev
RPGMS     = fmvvideodevice.privdevice.rom
DOCS      =
TESTS     =
CDEFS     = -DMIABUILD=0 -DCLOSEDCAPTION=0
INCS_HERE = fmvdriver.h


###############################################################################
# Stuff for the fmv device proper
#
fmv_CSRC        = AV110.c \
	          CL450.c \
	          CL450Diags.c \
	          FMVAudioDriver.c \
	          FMVDriver.c \
	          FMVROM.c \
	          FMVVideoDriver.c \
	          Woody.c \
	          installFMVDriver.c

fmv_INCS        = AV110.h \
	          CL450.h \
	          FMV.h \
	          FMVAudioDriver.h \
	          FMVDriver.h \
	          FMVErrors.h \
	          FMVROM.h \
	          FMVUtilities.h \
	          FMVVideoDriver.h \
	          FMVVideoDriverImp.h \
	          Hollywood.h \
	          Woody.h

fmvvideodevice_MODBIN      = -pri 100 -type 15
fmvvideodevice_STARTUPCODE = subroutinestartup.o

fmvvideodevice.dev_OBJS = ${fmv_CSRC:%.c=%.do}
fmvvideodevice.dev_LIBS = audio.lib filesystem.lib kernel.lib clib.lib

fmvvideodevice.rom_OBJS = ${fmv_CSRC:%.c=%.ro}
fmvvideodevice.rom_LIBS = audio.lib filesystem.lib kernel.lib clib.lib

# This file must be compiled with debugging turned on, or the code just doesn't
# work
CL450.do: CL450.c
	$(CC) ${CDEFS} ${CDDEFS} ${CCDEBUG} \
	$(ARMCFLAGS) ${FF} ${ZPS} ${APCS} ${WA} ${ZO} \
	$(INCLUDES:%=-J%) ${HWOPTS} -DDEVELOPMENT \
	-g -c $< -o $@

CL450.ro: CL450.c
	$(CC) ${CDEFS} ${CRDEFS} ${CCDEBUG} \
	$(ARMCFLAGS) ${FF} ${ZPS} ${APCS} ${WA} ${ZO} \
	$(INCLUDES:%=-J%) ${HWOPTS} -DROMBUILD -DRUNTIME -DPRODUCTION\
	-g -c $< -o $@

fmvvideodevice.dev: ${fmvvideodevice.dev_OBJS}
	            ${linkdev}
	            ${compress}
	            ${setrev}
	            ${setpriv}

fmvvideodevice.privdevice.dev: fmvvideodevice.dev
	                       ${copyit}

fmvvideodevice.rom: ${fmvvideodevice.rom_OBJS}
	            ${linkrom}
	            ${compress}
	            ${setrev}
	            ${setpriv}

fmvvideodevice.privdevice.rom: fmvvideodevice.rom
	                       ${copyit}

CL45016Bit.UCode: /opera/archive/CL45016Bit.UCode.94.12.09
		  ${copyit}

CL45024Bit.UCode: /opera/archive/CL45024Bit.UCode.94.12.09
		  ${copyit}

CL450Boot.UCode: /opera/archive/CL450Boot.UCode.94.12.09
		  ${copyit}


###############################################################################
#
#
RCSFILES   = ${fmv_CSRC} ${fmv_INCS}
AUTODEPEND = ${fmvvideodevice.dev_OBJS} ${fmvvideodevice.rom_OBJS}
DEPFILE    = .autodepends


###############################################################################
# Get our default stuff...
#
include GNUmakefile.master
