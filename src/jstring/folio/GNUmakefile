# $Id: GNUmakefile,v 1.8 1994/10/13 00:19:44 limes Exp $
# GNUmakefile for the JString folio.
#
# Copyright (c) 1994, The 3DO Company, Inc.
# All Rights Reserved.
# This document is proprietary and confidential

default: all


###############################################################################
# Describe our files
#
INCS_HERE  = jstring.h

LOCAL_INCS = tables.h

DATE       = builddate

DHDW       = jstring.folio.dev
RHDW       = jstring.folio.rom


###############################################################################
# Stuff for the folio
#
jstring_CSRC       = jstring_folio.c jconversions.c tables.c autodocs.c
jstring_STARTUPCODE = subroutinestartup.o
jstring_MODBIN     = -pri 20 -type 4

jstring.dev_OBJS   = ${jstring_CSRC:%.c=%.do}
jstring.dev_LIBS   = clib.lib

jstring.rom_OBJS   = ${jstring_CSRC:%.c=%.ro}
jstring.rom_LIBS   = clib.lib

jstring.dev: ${jstring.dev_OBJS}
	     ${linkdev}
	     ${compress}
	     ${setrev}

jstring.rom: ${jstring.rom_OBJS}
	     ${linkrom}
	     ${compress}
	     ${setrev}

jstring.folio.dev: jstring.dev
		   ${RM} jstring.folio.dev
		   ${CP} jstring.dev jstring.folio.dev

jstring.folio.rom: jstring.rom
		   ${RM} jstring.folio.rom
		   ${CP} jstring.rom jstring.folio.rom


###############################################################################
#
#
RCSFILES   = ${jstring_CSRC}

AUTODEPEND = ${jstring.dev_OBJS} ${jstring.rom_OBJS}

DEPFILE    = .autodepends


###############################################################################
# Get our default stuff...
#
include GNUmakefile.master
