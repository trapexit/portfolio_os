# $Id: GNUmakefile,v 1.16 1994/11/29 17:43:17 vertex Exp $
# GNUmakefile for the International folio language drivers.
#
# Copyright (c) 1994, The 3DO Company, Inc.
# All Rights Reserved.
# This document is proprietary and confidential

default: all


###############################################################################
# Describe our files
#
INCS_HERE      = ja.h

DATE           = builddate

LOCAL_INCLUDES = ../folio

DPGMS          = fr.language.dev \
		 it.language.dev \
		 nl.language.dev \
		 de.language.dev \
		 pt.language.dev \
		 da.language.dev \
		 ja.language.dev \
		 es.language.dev

RPGMS          = fr.language.rom \
		 it.language.rom \
		 nl.language.rom \
		 de.language.rom \
		 pt.language.rom \
		 da.language.rom \
		 ja.language.rom \
		 es.language.rom


###############################################################################
# Stuff for the French language driver
#
fr.language_CSRC        = fr.c
fr.language_STARTUPCODE = subroutinestartup.o

fr.language.dev_OBJS    = ${fr.language_CSRC:%.c=%.do}
fr.language.dev_MODBIN  = -pri 20 -stack 1024
fr.language.dev_LIBS    = filesystem.lib kernel.lib clib.lib

fr.language.rom_OBJS    = ${fr.language_CSRC:%.c=%.ro}
fr.language.rom_MODBIN  = -pri 20 -stack 1024
fr.language.rom_LIBS    = filesystem.lib kernel.lib clib.lib

fr.language.dev: ${fr.language.dev_OBJS}
		 ${linkdev}
		 ${compress}
		 ${setrev}

fr.language.rom: ${fr.language.rom_OBJS}
		 ${linkrom}
		 ${compress}
		 ${setrev}


###############################################################################
# Stuff for the Italian language driver
#
it.language_CSRC        = it.c
it.language_STARTUPCODE = subroutinestartup.o

it.language.dev_OBJS    = ${it.language_CSRC:%.c=%.do}
it.language.dev_MODBIN  = -pri 20 -stack 1024
it.language.dev_LIBS    = filesystem.lib kernel.lib clib.lib

it.language.rom_OBJS    = ${it.language_CSRC:%.c=%.ro}
it.language.rom_MODBIN  = -pri 20 -stack 1024
it.language.rom_LIBS    = filesystem.lib kernel.lib clib.lib

it.language.dev: ${it.language.dev_OBJS}
		 ${linkdev}
	         ${compress}
		 ${setrev}

it.language.rom: ${it.language.rom_OBJS}
		 ${linkrom}
		 ${compress}
		 ${setrev}


###############################################################################
# Stuff for the Dutch language driver
#
nl.language_CSRC        = nl.c
nl.language_STARTUPCODE = subroutinestartup.o

nl.language.dev_OBJS    = ${nl.language_CSRC:%.c=%.do}
nl.language.dev_MODBIN  = -pri 20 -stack 1024
nl.language.dev_LIBS    = filesystem.lib kernel.lib clib.lib

nl.language.rom_OBJS    = ${nl.language_CSRC:%.c=%.ro}
nl.language.rom_MODBIN  = -pri 20 -stack 1024
nl.language.rom_LIBS    = filesystem.lib kernel.lib clib.lib

nl.language.dev: ${nl.language.dev_OBJS}
		 ${linkdev}
		 ${compress}
		 ${setrev}

nl.language.rom: ${nl.language.rom_OBJS}
		 ${linkrom}
		 ${compress}
		 ${setrev}


###############################################################################
# Stuff for the German language driver
#
de.language_CSRC        = de.c
de.language_STARTUPCODE = subroutinestartup.o

de.language.dev_OBJS    = ${de.language_CSRC:%.c=%.do}
de.language.dev_MODBIN  = -pri 20 -stack 1024
de.language.dev_LIBS    = filesystem.lib kernel.lib clib.lib

de.language.rom_OBJS    = ${de.language_CSRC:%.c=%.ro}
de.language.rom_MODBIN  = -pri 20 -stack 1024
de.language.rom_LIBS    = filesystem.lib kernel.lib clib.lib

de.language.dev: ${de.language.dev_OBJS}
		 ${linkdev}
		 ${compress}
		 ${setrev}

de.language.rom: ${de.language.rom_OBJS}
		 ${linkrom}
		 ${compress}
		 ${setrev}


###############################################################################
# Stuff for the Portuguese language driver
#
pt.language_CSRC        = pt.c
pt.language_STARTUPCODE = subroutinestartup.o

pt.language.dev_OBJS    = ${pt.language_CSRC:%.c=%.do}
pt.language.dev_MODBIN  = -pri 20 -stack 1024
pt.language.dev_LIBS    = filesystem.lib kernel.lib clib.lib

pt.language.rom_OBJS    = ${pt.language_CSRC:%.c=%.ro}
pt.language.rom_MODBIN  = -pri 20 -stack 1024
pt.language.rom_LIBS    = filesystem.lib kernel.lib clib.lib

pt.language.dev: ${pt.language.dev_OBJS}
		 ${linkdev}
		 ${compress}
		 ${setrev}

pt.language.rom: ${pt.language.rom_OBJS}
		 ${linkrom}
		 ${compress}
		 ${setrev}


###############################################################################
# Stuff for the Danish language driver
#
da.language_CSRC        = da.c
da.language_STARTUPCODE = subroutinestartup.o

da.language.dev_OBJS    = ${da.language_CSRC:%.c=%.do}
da.language.dev_MODBIN  = -pri 20 -stack 1024
da.language.dev_LIBS    = filesystem.lib kernel.lib clib.lib

da.language.rom_OBJS    = ${da.language_CSRC:%.c=%.ro}
da.language.rom_MODBIN  = -pri 20 -stack 1024
da.language.rom_LIBS    = filesystem.lib kernel.lib clib.lib

da.language.dev: ${da.language.dev_OBJS}
		 ${linkdev}
		 ${compress}
		 ${setrev}

da.language.rom: ${da.language.rom_OBJS}
		 ${linkrom}
		 ${compress}
		 ${setrev}


###############################################################################
# Stuff for the Japanese language driver
#
ja.language_CSRC        = ja.c
ja.language_STARTUPCODE = subroutinestartup.o

ja.language.dev_OBJS    = ${ja.language_CSRC:%.c=%.do}
ja.language.dev_MODBIN  = -pri 20 -stack 1024
ja.language.dev_LIBS    = ../lib/international.dlib filesystem.lib kernel.lib clib.lib

ja.language.rom_OBJS    = ${ja.language_CSRC:%.c=%.ro}
ja.language.rom_MODBIN  = -pri 20 -stack 1024
ja.language.rom_LIBS    = ../lib/international.rlib filesystem.lib kernel.lib clib.lib

ja.language.dev: ${ja.language.dev_OBJS}
		 ${linkdev}
		 ${compress}
		 ${setrev}

ja.language.rom: ${ja.language.rom_OBJS}
		 ${linkrom}
		 ${compress}
		 ${setrev}


###############################################################################
# Stuff for the Spanish language driver
#
es.language_CSRC        = es.c
es.language_STARTUPCODE = subroutinestartup.o

es.language.dev_OBJS    = ${es.language_CSRC:%.c=%.do}
es.language.dev_MODBIN  = -pri 20 -stack 1024
es.language.dev_LIBS    = filesystem.lib kernel.lib clib.lib

es.language.rom_OBJS    = ${es.language_CSRC:%.c=%.ro}
es.language.rom_MODBIN  = -pri 20 -stack 1024
es.language.rom_LIBS    = filesystem.lib kernel.lib clib.lib

es.language.dev: ${es.language.dev_OBJS}
		 ${linkdev}
		 ${compress}
		 ${setrev}

es.language.rom: ${es.language.rom_OBJS}
		 ${linkrom}
		 ${compress}
		 ${setrev}


###############################################################################
#
#
RCSFILES   = ${fr.language_CSRC} \
	     ${it.language_CSRC} \
	     ${nl.language_CSRC} \
	     ${de.language_CSRC} \
	     ${pt.language_CSRC} \
	     ${da.language_CSRC} \
	     ${ja.language_CSRC} \
	     ${es.language_CSRC}

AUTODEPEND = ${fr.language.dev_OBJS} ${fr.language.rom_OBJS} \
	     ${it.language.dev_OBJS} ${it.language.rom_OBJS} \
	     ${nl.language.dev_OBJS} ${nl.language.rom_OBJS} \
	     ${de.language.dev_OBJS} ${de.language.rom_OBJS} \
	     ${pt.language.dev_OBJS} ${pt.language.rom_OBJS} \
	     ${da.language.dev_OBJS} ${da.language.rom_OBJS} \
	     ${ja.language.dev_OBJS} ${ja.language.rom_OBJS} \
	     ${es.language.dev_OBJS} ${es.language.rom_OBJS}

DEPFILE    = .autodepends


###############################################################################
# Get our default stuff...
#
include GNUmakefile.master
