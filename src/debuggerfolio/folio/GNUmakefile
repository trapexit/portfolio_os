# $Id: GNUmakefile,v 1.6 1994/10/11 18:51:44 limes Exp $
# GNUmakefile for the Debugger folio.
#
# Copyright (c) 1994, The 3DO Company, Inc.
# All Rights Reserved.
# This document is proprietary and confidential

default: all


###############################################################################
# Describe our files
#
LOCAL_INCS = debuggerfolio.h debuggerfolio_private.h foliolist.h\
	     hostcomms.h debuggerlowmem.h switrace.h debuggerfolioequates.i

DATE       = builddate

DHDW       = debugger.privfolio.dev


###############################################################################
# Stuff for the folio
#
STARTUPCODE = subroutinestartup.o

debugger_SSRC       = debuggerfolioasm.s
debugger_CSRC       = debuggerfolio.c debuggeruser.c switrace.c
debugger_MODBIN     = -pri 190 -type 4

debugger.dev_OBJS   = ${debugger_CSRC:%.c=%.do} ${debugger_SSRC:%.s=%.do}
debugger.dev_LIBS   = filesystem.lib kernel.lib clib.lib

debugger.rom_OBJS   = ${debugger_CSRC:%.c=%.ro} ${debugger_SSRC:%.s=%.ro}
debugger.rom_LIBS   = kernel.lib clib.lib

debugger.dev: ${debugger.dev_OBJS}
		   ${linkdev}
		   ${stripaif}
		   ${compress}
		   ${setrev}
		   ${setpriv}

debugger.rom: ${debugger.rom_OBJS}
		   ${linkrom}
		   ${compress}
		   ${setrev}
		   ${setpriv}

debugger.privfolio.dev: debugger.dev
		        ${RM} debugger.privfolio.dev
		        ${CP} debugger.dev debugger.privfolio.dev

debugger.privfolio.rom: debugger.rom
		        ${RM} debugger.privfolio.rom
		        ${CP} debugger.rom debugger.privfolio.rom


###############################################################################
#
#
RCSFILES   = ${debugger_CSRC} ${debugger_SSRC}

AUTODEPEND = ${debugger.dev_OBJS} ${debugger.rom_OBJS}

DEPFILE    = .autodepends


###############################################################################
# Get our default stuff...
#
include GNUmakefile.master
