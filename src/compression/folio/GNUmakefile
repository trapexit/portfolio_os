# $Id: GNUmakefile,v 1.7 1994/10/13 00:15:17 limes Exp $
# GNUmakefile for the compression folio.
#
# Copyright (c) 1994, The 3DO Company, Inc.
# All Rights Reserved.
# This document is proprietary and confidential

default: all


###############################################################################
# Describe our files
#
LOCAL_INCS = lzss.h

INCS_HERE  = compression.h

DATE       = builddate

DHDW       = compression.folio.dev
RHDW       = compression.folio.rom


###############################################################################
# Stuff for the folio
#
compression_CSRC       = compression_folio.c compress.c decompress.c \
			 autodocs.c addstring.c

compression_STARTUPCODE = subroutinestartup.o
compression_MODBIN     = -pri 20 -type 4

compression.dev_OBJS   = ${compression_CSRC:%.c=%.do}
compression.dev_LIBS   = clib.lib

compression.rom_OBJS   = ${compression_CSRC:%.c=%.ro}
compression.rom_LIBS   = clib.lib

ifndef CPU_POWERPC
# The ARM 1.61 compiler has a bug that forces us to compile this module
# without optimization. If this bug gets fixed, then this exception
# handling should be removed, and the module should be optimized. It is
# in fact a major performance bottleneck in the compression code, so
# optimizations would be quite welcomed.
#
addstring.do: addstring.c
	$(CC) ${CDEFS} ${CDDEFS} ${CCDEBUG} \
	$(ARMCFLAGS) ${FF} ${ZPS} ${APCS} ${WA} ${ZO} \
	$(INCLUDES:%=-J%) ${HWOPTS} -DDEVELOPMENT \
	-g -c $< -o $@

addstring.ro: addstring.c
	$(CC) ${CDEFS} ${CRDEFS} ${CCDEBUG} \
	$(ARMCFLAGS) ${FF} ${ZPS} ${APCS} ${WA} ${ZO} \
	$(INCLUDES:%=-J%) ${HWOPTS} -DROMBUILD -DRUNTIME -DPRODUCTION\
	-g -c $< -o $@
endif

compression.dev: ${compression.dev_OBJS}
		 ${linkdev}
		 ${compress}
		 ${setrev}

compression.rom: ${compression.rom_OBJS}
		 ${linkrom}
		 ${compress}
		 ${setrev}

compression.folio.dev: compression.dev
		       ${RM} compression.folio.dev
		       ${CP} compression.dev compression.folio.dev

compression.folio.rom: compression.rom
		       ${RM} compression.folio.rom
		       ${CP} compression.rom compression.folio.rom


###############################################################################
#
#
RCSFILES   = ${compression_CSRC}

AUTODEPEND = ${compression.dev_OBJS} ${compression.rom_OBJS}

DEPFILE    = .autodepends


###############################################################################
# Get our default stuff...
#
include GNUmakefile.master
