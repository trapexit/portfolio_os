# $Id: GNUmakefile,v 1.29 1994/12/14 23:40:26 phil Exp $
# audiofolio/GNUmakefile - makefile for audiofolio
#
# Copyright (c) 1992, 1993, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential
#
# 940407 PLB removed availmem.o and test_hack.h from build
# 940811 PLB split table_alloc.c and ezmem_tools.c from handy_tools.c

default:		all


# --------------------- includes

# These files are private and under RCS control here.
LOCAL_INCS=		audio_internal.h audio_structs.h \
			dspp.h dspp_touch.h dspp_addresses.h \
                        touch_hardware.h ezmem_tools.h table_alloc.h

# These files are exported by this package and are under RCS in this directory.
INCS_HERE=

# These files are exported by this package but are under RCS control in "includes".
INCS_DOWN=		audio.h


# --------------------- .tree

.tree: .includes


# --------------------- compilation control

# extra includes path directories to scan (inserted before INCS_TARG)
LOCAL_INCLUDES=		../musiclib/includes


# --------------------- linkage control

# global link libraries (used by everything, linked after all other LIBS)
LINK_LIBS=		filesystem.lib operamath.lib kernel.lib clib.lib

# global MBFLAGS (used by everything, <target>_MODBIN can take precedence over these)
MBFLAGS=

DATE=			BuildDate


# --------------------- programs

DHDW=			audio.privfolio.dev
RHDW=			audio.privfolio.rom

# This is here to cause library dependencies to be generated correctly in .autodepends
# It does however have the annoying side effect of putting audio.rom into the devs list (e.g. quickdevs).
LOCALS=			audio.dev audio.rom

AVAILMEM=

audio_CSRC=		audio_folio.c audio_instr.c audio_knob.c \
			audio_samples.c audio_timer.c audio_probe.c \
			audio_envelopes.c audio_attachments.c \
			audio_tuning.c audio_misc.c audio_errors.c $(AVAILMEM) \
			dspp_instr.c dspp_loader.c dspp_knobs.c dspp_touch.c \
			dspp_duck.c dspp_touch_anvil.c dspp_touch_bulldog.c \
			ConvertIEEEFP.c ezmem_tools.c table_alloc.c

audio_STARTUPCODE=	subroutinestartup.o

audio_MODBIN=		-pri 190 -stack 5000 -type 4

audio.dev_OBJS=		${audio_CSRC:%.c=%.do}
audio.dev_LIBS=		../musiclib/music.dlib
audio.dev_MODBIN=	${audio_MODBIN}
audio.dev:		${audio.dev_OBJS}
	${linkdev}
	${stripaif}
	${compress}
	${setrev}
	${setpriv}

audio.rom_OBJS=		${audio_CSRC:%.c=%.ro}
audio.rom_LIBS=		../musiclib/music.rlib
audio.rom_MODBIN=	${audio_MODBIN}
audio.rom:		${audio.rom_OBJS}
	${linkrom}
	${stripaif}
	${compress}
	${setrev}
	${setpriv}

audio.privfolio.dev: audio.dev
		     ${RM} audio.privfolio.dev
		     ${CP} audio.dev audio.privfolio.dev

audio.privfolio.rom: audio.rom
		     ${RM} audio.privfolio.rom
		     ${CP} audio.rom audio.privfolio.rom

# --------------------- docs (!!! this might go away)

audio.doc:		${audio_CSRC}
	${ADX} $^ >$@


# --------------------- rcs files and dependencies

RCSFILES=		${audio_CSRC}

AUTODEPEND=		${audio.dev_OBJS} ${audio.rom_OBJS}


# --------------------- tail

DEPFILE=	.autodepends
include GNUmakefile.master
