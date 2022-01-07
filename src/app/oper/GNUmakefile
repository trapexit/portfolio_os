# $Id: GNUmakefile,v 1.30 1994/12/01 23:02:16 jhw Exp $
# GNUmakefile for the Operator
#
# Copyright (c) 1992, 1993, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all

# Yes, we have a datestamp file.
DATE=			BuildDate

# Everyone gets -DARMC,
# we add nothing more for development,
# and add -DROMBUILD and -DPRODUCTIONfor runtime.
# XXX- PowerPC guys probably do not want -DARMC turned on.
CDEFS=			-DARMC
CDDEFS=			${NOTHING}
CRDEFS=			-DROMBUILD -DPRODUCTION

# Pretty standard link libraries.
LINK_LIBS=		kernel.lib filesystem.lib clib.lib

# The magical "uppercase" -MAP is used here.
MAP=			-MAP

# We turn warnings back on; they are useful and not overwhelming.
NOWARN=			${NOTHING}

# Only used for "dense-pack" demand loaded eepromtask, not
# yet fully debugged. Included here for future use.
# Do not delete unless demand-loading project has been cancelled.
#LCRT0_SSRC=            ../../libs/c/cdevstartup.s

# These headers are local to this directory and are private to this package.
LOCAL_INCS=		lccddriver.h

# ========================================================================

DHDW=			operator.dev

operator_MODBIN=	-pri 200 -stack 2048 -MaxUsecs 15000
operator_MODNAME=	Operator

operator_SSRC=		softreset.s read4bytes.s kernelcall.s sportxfer.s
operator_CSRC=		operator.c ramdevice.c \
			timerdevice.c xbusdevice.c \
			duckandcover.c sportdev.c eeprom.c lomac.c \
			cdrom.c mac.c math64.c lccddriver.c CDROMDriver.c \
			ControlPort.c putc.c fmv.c

operator.dev_OBJS=	${operator_SSRC:%.s=%.o} ${operator_CSRC:%.c=%.do}
operator.dev_LIBS=	graphics.lib
operator.dev:		${operator.dev_OBJS}
	${linkdev}
	${compress}
	${setrev}

RHDW=			operator.rom

operator.rom_OBJS=	${operator_SSRC:%.s=%.o} ${operator_CSRC:%.c=%.ro}
operator.rom_LIBS=	graphics.lib
operator.rom:		${operator.rom_OBJS}
	${linkrom}
	${compress}
	${setrev}


# ==========

LOCALS =		sporttest

sporttest_CSRC =	sporttest.c
sporttest_OBJS =	${sporttest_CSRC:%.c=%.o}
sporttest_MODBIN =	-stack 1024
sporttest:	${sporttest_OBJS}
	${linkdev}


# ==========

DLOCALS=		eepromtask.dev

# eepromtask developed as proof-of-concept for demand-loaded
# device drivers. Project still in progress.
# Note there is no ROM version because I've never tested one.
# Special info to use eepromtask:
# - build operator normally (no eeprom)
# - build eepromtask.dev
# - copy eepromtask.dev to $DRIVERS, change name to "eeprom"
# - make sure you have a leaky or extrarich dipir
# - use eeprom utility normally - myfind will launch eepromtask
#   from $DRIVERS if it can't find the eeprom device already
#   loaded.
# - (Note - eeprom task or driver and eeprom utility are different things.
#    eeprom utility is in src/app/other)

eepromtask.do:	eeprom.c
	${CC} -DEEPROMTASK ${CFLAGSD} -c -o $@ $<

.dep.eepromtask.do:	eeprom.c
	${CC} -DEEPROMTASK ${CFLAGSD} -c $< -o $*.do -M > $@


eepromtask.dev_OBJS=	eepromtask.do

# for device task node - "dense-pack"
#eepromtask.dev_MODBIN=		-stack 512 -type 15 -subsys 1
eepromtask.dev_MODBIN=		-stack 1024 -type 5 -subsys 1
eepromtask.dev:		${eepromtask.dev_OBJS}
	${linkdev}
	${setpriv}

# ==========

RCSFILES=		${operator_SSRC} ${operator_CSRC} ${eepromtask_CSRC} ${sporttest_CSRC}

AUTODEPEND=		${operator.dev_OBJS} ${operator.rom_OBJS} \
			${eepromtask.dev_OBJS} ${sporttest_OBJS}

DEPFILE=	.autodepends
include GNUmakefile.master
