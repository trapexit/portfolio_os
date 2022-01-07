# $Id: GNUmakefile,v 1.7 1994/10/10 19:13:08 peabody Exp $
# audiolib/GNUmakefile - makefile for audio.lib
#
# Copyright (c) 1994, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all


# --------------------- includes

# These files are private and under RCS control here.
LOCAL_INCS=

# These files are exported by this package and are under RCS in this directory.
INCS_HERE=

# These files are exported by this package but are under RCS control in "includes".
INCS_DOWN=


# --------------------- compilation control

# extra includes path directories to scan (inserted before INCS_TARG)
LOCAL_INCLUDES=		../musiclib/includes \
			../audiofolio/includes \
			../audiofolio

# put each function in its own code area
ZO=-zo


# --------------------- libs

LIB_DATE= 		builddate

BLIBS=			audio.blib
DLIBS=			audio.dlib
RLIBS=			audio.rlib

audio.lib_CSRC=		audio_lib.c

audio.blib_OBJS=	${audio.lib_CSRC:%.c=%.bo}
audio.blib:		${audio.blib_OBJS}
	${linklib}

audio.dlib_OBJS=	${audio.lib_CSRC:%.c=%.do}
audio.dlib:		${audio.dlib_OBJS}
	${linklib}

audio.rlib_OBJS=	${audio.lib_CSRC:%.c=%.ro}
audio.rlib:		${audio.rlib_OBJS}
	${linklib}


# --------------------- rcs files and dependencies

RCSFILES=		${audio.lib_CSRC}

AUTODEPEND=		${audio.blib_OBJS} ${audio.dlib_OBJS} ${audio.rlib_OBJS}


# --------------------- tail

DEPFILE=		.autodepends

include GNUmakefile.master
