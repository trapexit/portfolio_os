# $Id: GNUmakefile,v 1.54 1994/10/24 06:06:14 ewhac Exp $
# GNUmakefile for the Graphics Library, Folio, and test programs.
#
# Copyright (c) 1992, 1993, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all

# ==========

.tree: .includes

# ==========

INCS_DOWN=		graphics.h graphics.i intgraf.h intgraf.i stack.i \
			vbl.h

LOCAL_INCS=		overlay.h vdl.h

# ==========

DATE=			BuildDate

CDDEFS=			-DMODE="_MODE_developer"
CRDEFS=			-DMODE="_MODE_runtime"
NOWARN=			${NOTHING}
FZ=			${NOTHING}

# ==========

BDIRS= lib

# ==========

LINK_LIBS=		kernel.lib input.lib filesystem.lib clib.lib

DHDW=			graphics.privfolio.dev
RHDW=			graphics.privfolio.rom

OPERAMATH_DIR=		../libs/operamath
LOCAL_INCLUDES=		${OPERAMATH_DIR}includes

# ==========

gfxcommon_CSRC=	        operator.c debug.c error.c firq.c intgraf.c \
			render.c screen.c graftext.c text_deffont.c \
			createscreengroup.c vdl.c setwatchdog.c \
			kludge.c bitmap.c query.c overlay.c vblmgr.c
gfxcommon_SSRC=

operamath_CSRC=		mathfolio.c hardmath.c
operamath_SSRC=		mathfolio_asm.s hardmath_asm.s \
			divuf16.s divsf16.s \
			muluf16.s mulsf16.s \
			divs64.s muls32_64.s \
			sqrtf16.s

# ==========

graphics_CSRC=		grafboot.c ${gfxcommon_CSRC}
graphics_SSRC=          ${gfxcommon_SSRC}
graphics_STARTUPCODE=	subroutinestartup.o

graphics_MODBIN=	-pri 201 -type 4

graphics.dev_OBJS=	${graphics_SSRC:%.s=%.do} ${graphics_CSRC:%.c=%.do}
graphics.dev:		${graphics.dev_OBJS}
	${linkdev}
	${compress}
	${setrev}
	${setpriv}

graphics.rom_OBJS=	${graphics_SSRC:%.s=%.ro} ${graphics_CSRC:%.c=%.ro}
graphics.rom:		${graphics.rom_OBJS}
	${linkrom}
	${compress}
	${setrev}
	${setpriv}

graphics.privfolio.dev: graphics.dev
			${RM} graphics.privfolio.dev
			${CP} graphics.dev graphics.privfolio.dev

graphics.privfolio.rom: graphics.rom
			${RM} graphics.privfolio.rom
			${CP} graphics.rom graphics.privfolio.rom

# ==========

COMPLEX_LOCALS=		grafmath

grafmath_LCSRC=		grafmathboot.c ${gfxcommon_CSRC}
grafmath_RCSRC=		${operamath_CSRC:%=${OPERAMATH_DIR}/%}
grafmath_LSSRC=		${graphics_SSRC}
grafmath_RSSRC=		${operamath_SSRC:%=${OPERAMATH_DIR}/%}
grafmath_MODBIN=	-pri 201 -stack 1024
grafmath_LOBJS=		${grafmath_LSSRC:%.s=%.do} ${grafmath_LCSRC:%.c=%.do}
grafmath_ROBJS=		${grafmath_RSSRC:%.s=%.do} ${grafmath_RCSRC:%.c=%.do}
grafmath_OBJS=		${grafmath_LOBJS} ${grafmath_ROBJS}
grafmath:		${grafmath_OBJS}
	${linkdev}
	${compress}
	${setrev}
	${setpriv}

# ==========

LOCALS=			${SIMPLE_LOCALS} ${COMPLEX_LOCALS}

example_CSRC=		example.c
example_OBJS=		${example_CSRC:%.c=%.o}
example_LIBS=		lib/graphics.dlib
example_MODBIN=		-stack 1024
example:		${example_OBJS}
	${linkdev}

simple_CSRC=		simple.c
simple_OBJS=		${simple_CSRC:%.c=%.o}
simple_LIBS=		lib/graphics.dlib
simple:			${simple_OBJS}
	${linkdev}

readloc_CSRC=		readloc.c
readloc_OBJS=		${readloc_CSRC:%.c=%.o}
readloc:		${readloc_OBJS}
	${linkdev}

gtest_CSRC=		gtest.c
gtest_OBJS=		${gtest_CSRC:%.c=%.o}
gtest_LIBS=		lib/graphics.dlib
gtest:		${gtest_OBJS}
	${linkdev}

ovtest_CSRC=		ovtest.c
ovtest_OBJS=		${ovtest_CSRC:%.c=%.o}
ovtest_LIBS=		lib/graphics.dlib
ovtest_MODBIN=		-stack 1024
ovtest:		${ovtest_OBJS}
	${linkdev}

vbltest_CSRC=		vbltest.c
vbltest_OBJS=		${vbltest_CSRC:%.c=%.o}
vbltest_LIBS=		lib/graphics.dlib
vbltest_MODBIN=		-stack 1024
vbltest:	${vbltest_OBJS}
	${linkdev}

vdltest_CSRC=		vdltest.c
vdltest_OBJS=		${vdltest_CSRC:%.c=%.o}
vdltest_LIBS=		lib/graphics.dlib
vdltest_MODBIN=		-stack 1024
vdltest:	${vdltest_OBJS}
	${linkdev}

scrtest_CSRC=		scrtest.c
scrtest_OBJS=		${scrtest_CSRC:%.c=%.o}
scrtest_LIBS=		lib/graphics.dlib
scrtest_MODBIN=		-stack 1024
scrtest:	${scrtest_OBJS}
	${linkdev}

SIMPLE_LOCALS=		example simple gtest readloc ovtest vbltest vdltest \
			scrtest

# ==========

DOCS=			GraphicsNotes.doc

RCSFILES=		${INCS_DOWN:%=includes/%} ${INCS_HERE} \
			${graphics_CSRC} \
			${graphics_SSRC} \
			${SIMPLE_LOCALS:%=%.c} ${DOCS} \
			${grafmath_LCSRC} \
			${grafmath_LSSRC} \
			${grafmath_RCSRC} \
			${grafmath_RSSRC} \
			autodocs.c

AUTODEPEND=		${graphics.dev_OBJS} \
			${graphics.rom_OBJS} \
			${grafmath_LOBJS} \
			${example_OBJS} \
			${simple_OBJS} \
			${readloc_OBJS}

DEPFILE=	.autodepends
include GNUmakefile.master
