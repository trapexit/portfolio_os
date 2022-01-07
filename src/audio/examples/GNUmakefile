# $Id: GNUmakefile,v 1.53 1994/11/07 21:14:59 peabody Exp $
# examples/GNUmakefile - somewhat inaccurate name for few audio shell program makefile.
#
# Copyright (c) 1992-1994, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential
#
# Notes:
#
# . Define MEMDEBUG environment variable or on gmake command line to turn
#   memdebug.

default:		all

ifdef MEMDEBUG
  CCDEBUG      = -DMEMDEBUG
  MEMDEBUG_LIB = memdebug.lib
endif

CDEFS          = -DEXTERNAL_RELEASE
LINK_LIBS      = ../musiclib/music.dlib ../audiolib/audio.dlib input.lib operamath.lib \
                 graphics.lib filesystem.lib ${MEMDEBUG_LIB} clib.lib
MBFLAGS        = -stack 6000

# extra includes path directories to scan (inserted before INCS_TARG)
LOCAL_INCLUDES = ../musiclib/includes ../audiofolio/includes

# ==========

# audiodemo.dlib is a private library for a few programs and tests
# !!! using .dlib here seems like a necessary evil just to get libs to be built that should probably change,
#     that has the unfortunate sideeffect of posting this library in .top/developer/libs. This would
#     be unnecessary except that there's a couple of programs under ../tests that depend on this
#     library. This is another one of those issues that will probably get resolved after the change to teamware.
DLIBS=			audiodemo.dlib

audiodemo.lib_INCS=	audiodemo.h faders.h joytools.h graphic_tools.h
audiodemo.lib_CSRC=	faders.c graphic_tools.c joytools.c

audiodemo.dlib_OBJS=	${audiodemo.lib_CSRC:%.c=%.do}
audiodemo.dlib:		${audiodemo.dlib_OBJS}
	${linklib}

# ==========

audiomon.dev_CSRC = audiomon.c
audiomon.dev_OBJS = ${audiomon.dev_CSRC:%.c=%.o}

audiomon.dev: ${audiomon.dev_OBJS}
	${linkdev}

dspfaders.dev_CSRC = ta_faders.c
dspfaders.dev_OBJS = ${dspfaders.dev_CSRC:%.c=%.o}
dspfaders.dev_LIBS = audiodemo.dlib

dspfaders.dev: ${dspfaders.dev_OBJS}
	${linkdev}

# !!! patchdemo.dev_EXTRAS=	delay.patch eqnoise.patch filternoise.patch pwm.patch rsh.patch sample.patch sawenv.patch wind.patch patchdemo.rnotes
patchdemo.dev_CSRC = patchdemo.c pd_patch.c pd_faders.c
patchdemo.dev_INCS = patchdemo.h
patchdemo.dev_OBJS = ${patchdemo.dev_CSRC:%.c=%.o}
patchdemo.dev_LIBS = audiodemo.dlib

patchdemo.dev: ${patchdemo.dev_OBJS}
	${linkdev}

playmf.dev_CSRC = playmf.c
playmf.dev_OBJS = ${playmf.dev_CSRC:%.c=%.o}

playmf.dev: ${playmf.dev_OBJS}
	${linkdev}

# ==========

# Some old, but perhaps useful, documentation re nested variable expansion:
#
# COMPLEX_EXAMPLE_SRCS expands to include <target>_CSRC, <target>_INCS, and
# <target>_EXTRAS for each target named in COMPLEX_EXAMPLES.
#
# COMPLEX_EXAMPLE_OBJS expands to include <target>_OBJS for each target
# named in COMPLEX_EXAMPLES.
#
# The $(foreach ...) functions cause each item in COMPLEX_EXAMPLES to be
# expanded into a list of variables associated with each enty, which are in
# turn is expanded using nested variable expansion.
#
# Given:
#     COMPLEX_EXAMPLES=foo.arm bar.arm
#     foo.arm_OBJS=foo.o morefoo.o evenmorefoo.o
#     bar.arm_OBJS=bar.o
#     ${foreach prog,${COMPLEX_EXAMPLES},${${prog}_OBJS}}
#
# $(foreach) is expands to:
#     ${foo.arm_OBJS} ${bar.arm_OBJS}
#
# which in turn expands to:
#     foo.o morefoo.o evenmorefoo.o bar.o
#
# RCSFILES and AUTODEPEND depend on these, which greatly reduces the risk
# of forgetting to update RCSFILES and AUTODEPEND when changing the list
# of examples or their dependencies.

ALL_CSRC      = ${foreach prog,${DPGMS},${${prog}_CSRC}} ${audiodemo.lib_CSRC}
LOCAL_INCS    = ${foreach prog,${DPGMS},${${prog}_INCS}} ${audiodemo.lib_INCS}

DPGMS         = audiomon.dev \
		dspfaders.dev \
		patchdemo.dev \
		playmf.dev

# ========================================================================

RCSFILES   = ${ALL_CSRC}
AUTODEPEND = ${ALL_CSRC:%.c=%.o}
DEPFILE    = .autodepends

include GNUmakefile.master
