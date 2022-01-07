# $Id: GNUmakefile,v 1.22 1994/09/21 23:15:51 peabody Exp $
# musiclib/GNUmakefile - makefile for music.lib
#
# Copyright (c) 1992, 1993, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all


# --------------------- includes

# These files are private and under RCS control here.
LOCAL_INCS=		music_internal.h soundplayer_internal.h

# These files are exported by this package and are under RCS in this directory.
INCS_HERE=

# These files are exported by this package but are under RCS control in "includes".
INCS_DOWN=		cobj.h flex_stream.h handy_tools.h iff_fs_tools.h \
			juggler.h midifile.h music.h musicerror.h score.h sound3d.h \
			soundfile.h soundplayer.h soundspooler.h


# --------------------- .tree

.tree: .includes


# --------------------- compilation control

# extra includes path directories to scan (inserted before INCS_TARG)
LOCAL_INCLUDES=		../audiofolio/includes

# put each function in its own code area
ZO=-zo

user_handy_tools.bo:	handy_tools.c
	${CC} -DUSERONLY ${CFLAGSB} -c -o $@ $<

.dep.user_handy_tools.bo: handy_tools.c
	${CC} -DUSERONLY $(CFLAGSB) -c $< -o ${@:.dep.%=%} -M > $@

user_handy_tools.do:	handy_tools.c
	${CC} -DUSERONLY ${CFLAGSD} -c -o $@ $<

.dep.user_handy_tools.do: handy_tools.c
	${CC} -DUSERONLY $(CFLAGSD) -c $< -o ${@:.dep.%=%} -M > $@

user_handy_tools.ro:	handy_tools.c
	${CC} -DUSERONLY ${CFLAGSR} -c -o $@ $<

.dep.user_handy_tools.ro: handy_tools.c
	${CC} -DUSERONLY $(CFLAGSR) -c $< -o ${@:.dep.%=%} -M > $@


# --------------------- libs

LIB_DATE=		builddate

BLIBS=			music.blib
DLIBS=			music.dlib
RLIBS=			music.rlib

music.lib_CSRC=		juggler.c \
			score_mfloader.c \
			score_pimaploader.c \
			score.c \
			midifile.c \
			selectsampleplayer.c \
			soundplayer.c \
			soundplayer_file.c \
			soundplayer_sample.c \
			soundfile.c \
			soundspooler.c \
			sound3d.c \
			iff_fs_tools.c \
			flex_stream.c \
			errors.c
# !!! handy_tools.c currently linked into both audiofolio and musiclib - will eventually be split into 2 parts
music.lib_XSRC=		handy_tools.c

music.blib_OBJS=	${music.lib_CSRC:%.c=%.bo} user_handy_tools.bo
music.blib:		${music.blib_OBJS}
	${linklib}

music.dlib_OBJS=	${music.lib_CSRC:%.c=%.do} user_handy_tools.do
music.dlib:		${music.dlib_OBJS}
	${linklib}

music.rlib_OBJS=	${music.lib_CSRC:%.c=%.ro} user_handy_tools.ro
music.rlib:		${music.rlib_OBJS}
	${linklib}


# --------------------- docs (!!! this might go away)

musiclib.doc:		${music.lib_CSRC}
	${ADX} $^ >$@


# --------------------- rcs files and dependencies

RCSFILES=		${music.lib_CSRC} ${music.lib_XSRC}

AUTODEPEND=		${music.blib_OBJS} ${music.dlib_OBJS} ${music.rlib_OBJS}


# --------------------- tail

DEPFILE=	.autodepends
include GNUmakefile.master
