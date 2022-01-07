# $Id: GNUmakefile,v 1.66 1994/09/26 18:29:52 peabody Exp $
# ./GNUmakefile - Top level audio makefile.
#
# Copyright (c) 1992, 1993, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all

.PHONY:			installaiff

installtherest:		installaiff


# --------------------- build directories
# (Practically nothing builds here; it is all in the subdirectories)

BDIRS=			audiofolio \
			audiolib \
			musiclib \
			examples \
			tests \
			dsp


# --------------------- docs
# (Well, we _do_ have some documents :-))

DOCS=			AudioRelease.txt
LOCAL_DOCS=		audio.notes


# --------------------- other files (nothing built by these)

OTHERFILES=		commenttemplate


# --------------------- AIFF file installation

# target directory for AIFF files
AIFF_TARG=		${RELEASEDIR}/remote/audio/aiff

installaiff:
	@${MD} ${AIFF_TARG}
	${RM} ${AIFF_TARG}/sinewave.aiff; ${CP} ${ARCHIVEDIR}/sinewave.aiff.93.06.15 ${AIFF_TARG}/sinewave.aiff


# --------------------- rcs files and dependencies

RCSFILES=		${LOCAL_DOCS}


# --------------------- tail

DEPFILE=		.autodepends
include GNUmakefile.master
