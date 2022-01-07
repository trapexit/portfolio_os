# $Id: GNUmakefile,v 1.28 1994/10/11 19:12:38 limes Exp $
# tests/GNUmakefile - makefile for audio test programs (local only).
#
# Copyright (c) 1992, 1993, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all


# --------------------- includes

# These files are private, and under RCS control here
LOCAL_INCS=

# These files are exported by this package and are under RCS in this directory.
INCS_HERE=

# These files are exported by this package but are under RCS control in "includes".
INCS_DOWN=


# --------------------- compilation control

# extra includes path directories to scan (inserted before INCS_TARG)
# !!! only need ../examples for ta_freqresp.arm and test3d.arm
LOCAL_INCLUDES=		../examples \
			../musiclib/includes \
			../audiofolio/includes

ifdef MEMDEBUG
  CCDEBUG=		-DMEMDEBUG
endif


# --------------------- linkage control

ifdef MEMDEBUG
  MEMDEBUG_LIB=		memdebug.lib
endif

# global link libraries (used by everything, linked after all other LIBS)
LINK_LIBS=		../musiclib/music.dlib \
			../audiolib/audio.dlib \
			graphics.lib input.lib \
			filesystem.lib operamath.lib \
			$(MEMDEBUG_LIB) \
			clib.lib

# global MBFLAGS (used by everything, <target>_MODBIN can take precedence over these)
MBFLAGS=                -stack 6000

# .c -> .arm link libs (link order is <target>_LIBS, ARMBIN_LINK_LIBS, LINK_LIBS).
ARMBIN_LINK_LIBS=

# .c -> .arm modbin flags (take precedence over MBFLAGS, but not <target>_MODBIN)
ARMBIN_MODBIN=

#DATE=			BuildDate


# --------------------- programs

# Programs to build (locals only)
LOCALS=			${SIMPLE_LOCALS} ${COMPLEX_LOCALS}


# Simple programs (thost with just 1 source code module)

SIMPLE_LOCALS= \
			cd2mac.arm \
			leftright.arm \
			supportadpcm.arm \
			ta_define.arm \
			ta_dualsaw.arm \
			ta_freqresp.arm \
			ta_polyphony.arm \
			ta_printins.arm \
			testaudio.arm \
			test3d.arm \
			tg_lines.arm

SIMPLE_LOCAL_SRCS=	${SIMPLE_LOCALS:%.arm=%.c}
SIMPLE_LOCAL_OBJS=	${SIMPLE_LOCALS:%.arm=%.o}

# Extra stuff required by some simple programs
ta_freqresp.arm_LIBS=	../examples/audiodemo.dlib
test3d.arm_LIBS=	../examples/audiodemo.dlib
testaudio.arm_LIBS=	../examples/audiodemo.dlib


# Complex programs (those that require more than 1 module)

COMPLEX_LOCALS= \
			testgravel.arm

COMPLEX_LOCAL_SRCS=	${foreach prog,${COMPLEX_LOCALS},${${prog}_CSRC} ${${prog}_INCS} ${${prog}_EXTRAS}}
COMPLEX_LOCAL_OBJS=	${foreach prog,${COMPLEX_LOCALS},${${prog}_OBJS}}

testgravel.arm_INCS=	gravel.h
testgravel.arm_CSRC=	testgravel.c gravel.c
testgravel.arm_OBJS=	${testgravel.arm_CSRC:%.c=%.o}
testgravel.arm:		${testgravel.arm_OBJS}
	${linkdev}

MFLoopTest.arm_INCS=	JoyPad.h
MFLoopTest.arm_CSRC=	MFLoopTest.c JoyPad.c
MFLoopTest.arm_OBJS=	${MFLoopTest.arm_CSRC:%.c=%.o}
MFLoopTest.arm:		${MFLoopTest.arm_OBJS}
	${linkdev}

#test_torture.arm_INCS=	torture_sound.h
#test_torture.arm_CSRC=	test_torture.c torture_sound.c
#test_torture.arm_OBJS=	${test_torture.arm_CSRC:%.c=%.o}
#test_torture.arm_MORE=	availmem.o
#test_torture.arm:	${test_torture.arm_OBJS} ${test_torture.arm_MORE}
#	${linkdev}

ta_setfolio.arm_CSRC=	ta_setfolio.c
ta_setfolio.arm_OBJS=	${ta_setfolio.arm_CSRC:%.c=%.o}
ta_setfolio.arm:	${ta_setfolio.arm_OBJS}
	${linkdev}
	${setpriv}


# --------------------- rcs files and dependencies

RCSFILES= 		${SIMPLE_LOCAL_SRCS} ${COMPLEX_LOCAL_SRCS}

AUTODEPEND=		${SIMPLE_LOCAL_OBJS} ${COMPLEX_LOCAL_OBJS}


# --------------------- tail

DEPFILE=	.autodepends
include GNUmakefile.master
