# $Id: GNUmakefile,v 1.40 1994/10/17 22:37:56 vertex Exp $
# GNUmakefile for Opera Event Broker
#
# Copyright (c) 1992, 1993, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all

# ==========

.tree: .includes

# ==========

INCS_DOWN=		event.h controlport.h poddriver.h podrom.h graphics.h

# ==========

CDEFS=			-DCONTROLPORT -DARMC
CRDEFS=			-ff
MAP=			-MAP

# ==========

LIB_DATE=		libbuilddate

BLIBS=			input.blib
DLIBS=			input.dlib
RLIBS=			input.rlib

INPUTLIB_CSRC=		EventUtility.c

input.blib_OBJS=	${INPUTLIB_CSRC:%.c=%.bo}
input.blib:		${input.blib_OBJS}
	${linklib}

input.dlib_OBJS=	${INPUTLIB_CSRC:%.c=%.do}
input.dlib:		${input.dlib_OBJS}
	${linklib}

input.rlib_OBJS=	${INPUTLIB_CSRC:%.c=%.ro}
input.rlib:		${input.rlib_OBJS}
	${linklib}

# ==========

DATE=			BuildDate

LINK_DLIBS=		input.dlib
LINK_RLIBS=		input.rlib
LINK_LIBS=		filesystem.lib clib.lib kernel.lib

# ==========

DPGMS=			eventbroker.dev
RPGMS=			eventbroker.rom

eventbroker_SSRC=	RegisterGlue.s
eventbroker_CSRC=	EventBroker.c ControlPadDriver.c DefaultDriver.c \
			SplitterDriver.c DummyPutc.c autodocs.c
eventbroker_DCSRC=	KeyboardDriver.c

eventbroker_LIBCRT0=	threadstartup.o copyright.o

eventbroker.dev_OBJS=	${eventbroker_SSRC:%.s=%.do} \
			${eventbroker_CSRC:%.c=%.do} \
			${eventbroker_DSRC:%.c=%.do}
eventbroker.dev_MODBIN=	-stack 1200 -pri 199
eventbroker.dev:	${eventbroker.dev_OBJS}
	${linkdev}
	${compress}
	${setrev}

eventbroker.rom_OBJS=	${eventbroker_SSRC:%.s=%.ro} \
			${eventbroker_CSRC:%.c=%.ro}
eventbroker.rom_MODBIN=	-stack 1200 -pri 199
eventbroker.rom:	${eventbroker.rom_OBJS}
	${linkrom}
	${compress}
	${setrev}

# ==========

PGMS=			${GENERIC} ${ROMS}

GENERIC=		lookie luckie cpdump focus generic maus

lookie_CSRC=		lookie.c
lookie_OBJS=		${lookie_CSRC:%.c=%.o}
lookie_LIBCRT0=		cstartup.o copyright.o
lookie_MODBIN=		-stack 2048
lookie:			${lookie_OBJS}
	${linkdev}

luckie_CSRC=		luckie.c
luckie_OBJS=		${luckie_CSRC:%.c=%.o}
luckie_LIBCRT0=		cstartup.o copyright.o
luckie_MODBIN=		-stack 2048
luckie:			${luckie_OBJS}
	${linkdev}

cpdump_CSRC=		cpdump.c
cpdump_OBJS=		${cpdump_CSRC:%.c=%.o}
cpdump_LIBCRT0=		cstartup.o copyright.o
cpdump_MODBIN=		-stack 2048
cpdump:			${cpdump_OBJS}
	${linkdev}

focus_CSRC=		focus.c
focus_OBJS=		${focus_CSRC:%.c=%.o}
focus_LIBCRT0=		cstartup.o copyright.o
focus_MODBIN=		-stack 2048
focus:			${focus_OBJS}
	${linkdev}

generic_CSRC=		generic.c
generic_OBJS=		${generic_CSRC:%.c=%.o}
generic_LIBCRT0=	cstartup.o copyright.o
generic_MODBIN=		-stack 2048
generic:		${generic_OBJS}
	${linkdev}

maus_CSRC=		maus.c
maus_OBJS=		${maus_CSRC:%.c=%.o}
maus_LIBCRT0=		cstartup.o copyright.o
maus_MODBIN=		-stack 2048
maus:			${maus_OBJS}
	${linkdev}

# ==========

romify=			./romify < $@ > $@.rr; mv -f $@.rr $@

ROMS=			hello.rom blink.rom \
			cport1.rom cport4D.rom \
			cport49.rom cport41.rom

GLUE_SSRC=		DriverletGlue.s
GLUE_OBJS=		${GLUE_SSRC:%.s=%.o}

hello.rom_CSRC=		hellorom.c
hello.rom_OBJS=		${GLUE_OBJS} ${hello.rom_CSRC:%.c=%.o}
hello.rom:		${hello.rom_OBJS}
	${linkdev}
	${signit}
	${romify}

blink.rom_CSRC=		blinkrom.c
blink.rom_OBJS=		${GLUE_OBJS} ${blink.rom_CSRC:%.c=%.o}
blink.rom:		${blink.rom_OBJS}
	${linkdev}
	${signit}
	${romify}

cport1.rom_CSRC=	StickDriver.c
cport1.rom_OBJS=	${GLUE_OBJS} ${cport1.rom_CSRC:%.c=%.o}
cport1.rom:		${cport1.rom_OBJS}
	${linkdev}
	${signit}
	${romify}

cport49.rom_CSRC=	MouseDriver.c
cport49.rom_OBJS=	${GLUE_OBJS} ${cport49.rom_CSRC:%.c=%.o}
cport49.rom:		${cport49.rom_OBJS}
	${linkdev}
	${signit}
	${romify}

cport41.rom_CSRC=	GlassesDriver.c
cport41.rom_OBJS=	${GLUE_OBJS} ${cport41.rom_CSRC:%.c=%.o}
cport41.rom:		${cport41.rom_OBJS}
	${linkdev}
	${signit}
	${romify}

cport4D.rom_CSRC=	LightGunRom.c
cport4D.rom_OBJS=	${GLUE_OBJS} ${cport4D.rom_CSRC:%.c=%.o}
cport4D.rom:		${cport4D.rom_OBJS}
	${linkdev}
	${signit}
	${romify}

${ROMS}: romify

romify:			romify.c
	@gcc ${INCLUDES:%=-I%} -o romify romify.c

# ==========

zap-locals:
	@/bin/rm -f romify

clean cleanest: zap-locals

RCSFILES=	${INPUTLIB_CSRC} \
		${eventbroker_SSRC} ${eventbroker_CSRC} \
		${eventbroker_DCSRC} ${lookie_CSRC} \
		${luckie_CSRC} ${cpdump_CSRC} ${focus_CSRC} \
		${generic_CSRC} ${maus_CSRC} ${hello.rom_CSRC} \
		${blink.rom_CSRC} ${cport4D.rom_CSRC} \
		${cport1.rom_CSRC} \
		${cport41.rom_CSRC} \
		${cport49.rom_CSRC} \
		${GLUE_SSRC} romify.c

AUTODEPEND= \
		${input.lib_OBJS} \
		${eventbroker.dev_OBJS} \
		${eventbroker.rom_OBJS} \
		${lookie_OBJS} \
		${luckie_OBJS} \
		${cpdump_OBJS} \
		${focus_OBJS} \
		${generic_OBJS} \
		${maus_OBJS} \
		${GLUE_OBJS} \
		${hello.rom_OBJS} \
		${blink.rom_OBJS} \
		${cport4D.rom_OBJS} \
		${cport49.rom_OBJS} \
		${cport41.rom_OBJS} \
		${cport1.rom_OBJS}

DEPFILE=	.autodepends
include GNUmakefile.master

# Ugly Overrides Go Here.

# Driverlets do not use LIBCRT0 support,
# but other stuff does.

LIBCRT0=		${${basename $@}_LIBCRT0}

