# $Id: GNUmakefile,v 1.2 1994/11/10 01:23:10 limes Exp $
# GNUmakefile for the ns device
#
# Copyright (c) 1994, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all


###############################################################################
# Describe our files
#
DATE=			BuildDate
LOCALS=			ns.dev ns.rom
DPGMS=			ns.privdevice.dev
RPGMS=			ns.privdevice.rom
DOCS=			nsNotes.doc


###############################################################################
# Stuff for the ns device proper
#
ns_CSRC=		nsio.c
ns_INCS=		nsio.h
ns_MODBIN=		-pri 100 -type 15
ns_STARTUPCODE=		subroutinestartup.o

ns.dev_OBJS=		${ns_CSRC:%.c=%.do}
ns.dev_LIBS=		kernel.lib clib.lib

ns.rom_OBJS=		${ns_CSRC:%.c=%.ro}
ns.rom_LIBS=		kernel.lib clib.lib

ns.dev: ${ns.dev_OBJS}
	${linkdev}
	${compress}
	${setrev}
	${setpriv}

ns.privdevice.dev: ns.dev
	${copyit}

ns.rom: ${ns.rom_OBJS}
	${linkrom}
	${compress}
	${setrev}
	${setpriv}

ns.privdevice.rom: ns.rom
	${copyit}

###############################################################################
#
#
RCSFILES=		${ns_CSRC} ${ns_INCS}
AUTODEPEND=		${ns.dev_OBJS} ${ns.rom_OBJS}
DEPFILE=		.autodepends


###############################################################################
# Get our default stuff...
#
include GNUmakefile.master
