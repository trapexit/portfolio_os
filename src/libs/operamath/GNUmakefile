# $Id: GNUmakefile,v 1.30 1994/10/13 00:20:05 limes Exp $
# GNUmakefile for the opera math folio, library, and test programs.
#
# Copyright (c) 1992, 1993, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all

# ==========

.tree: .includes

# ==========

INCS_DOWN=		operamath.h operamath.i macros.i mathfolio.h

LOCAL_INCS=		ThreeD.h

# ==========

DATE=			BuildDate

CBDEFS=			-DMODE="\"runtime\""
CDDEFS=			-DMODE="\"developer\""
CRDEFS=			-DMODE="\"runtime\""

NOWARN=			${NOTHING}

# ==========

LIB_DATE=		libbuilddate

BLIBS=			operamath.blib
DLIBS=			operamath.dlib
RLIBS=			operamath.rlib

operamath_lib_CSRC=	atan2f16.c bigsintable.c operamath.c sinf32.c

operamath_lib_SSRC=	add64.s cmps64.s cmpu64.s divs32.s divs64.s \
			divu32.s divu64.s mul64.s muls32_64.s \
			mulsf30.s mulu32_64.s neg64.s not64.s sinf16.s \
			sinf30.s softmath.s sqrt32.s sqrt64_32.s \
			sqrtf16.s square64.s squaresf16.s squareuf16.s \
			sub64.s

operamath.blib_OBJS=	${operamath_lib_CSRC:%.c=%.bo} ${operamath_lib_SSRC:%.s=%.bo}
operamath.blib:		${operamath.blib_OBJS}
	${linklib}

operamath.dlib_OBJS=	${operamath_lib_CSRC:%.c=%.do} ${operamath_lib_SSRC:%.s=%.do}
operamath.dlib:		${operamath.dlib_OBJS}
	${linklib}

operamath.rlib_OBJS=	${operamath_lib_CSRC:%.c=%.ro} ${operamath_lib_SSRC:%.s=%.ro}
operamath.rlib:		${operamath.rlib_OBJS}
	${linklib}

operamath_STARTUPCODE=	subroutinestartup.o
operamath_CSRC=		mathboot.c mathfolio.c hardmath.c autodocs.c
operamath_SSRC=		mathfolio_asm.s hardmath_asm.s divuf16.s divsf16.s \
			muluf16.s mulsf16.s divs64.s muls32_64.s \
			sqrtf16.s

# ==========

DHDW=			operamath.privfolio.dev
RHDW=			operamath.privfolio.rom

operamath_MODBIN=	-pri 201 -type 4

operamath.dev_LIBS=	kernel.lib clib.lib
operamath.dev_OBJS=	${operamath_SSRC:%.s=%.do} ${operamath_CSRC:%.c=%.do}
operamath.dev:		${operamath.dev_OBJS}
	${linkdev}
	${compress}
	${setrev}
	${setpriv}

operamath.rom_LIBS=	kernel.lib clib.lib
operamath.rom_OBJS=	${operamath_SSRC:%.s=%.ro} ${operamath_CSRC:%.c=%.ro}
operamath.rom:		${operamath.rom_OBJS}
	${linkrom}
	${compress}
	${setrev}
	${setpriv}

operamath.privfolio.dev: operamath.dev
			 ${RM} operamath.privfolio.dev
			 ${CP} operamath.dev operamath.privfolio.dev

operamath.privfolio.rom: operamath.rom
			 ${RM} operamath.privfolio.rom
			 ${CP} operamath.rom operamath.privfolio.rom

# ==========

LOCALS=			testmath cube mattest testmat

testmath_CSRC=		testmath.c
testmath_SSRC=		testmath_asm.s
testmath_LIBS=		operamath.dlib clib.lib
testmath_OBJS=		${testmath_CSRC:%.c=%.o} ${testmath_SSRC:%.s=%.o}
testmath:		${testmath_OBJS}
	${linkdev}

cube_CSRC=		cube.c ThreeD.c
cube_LIBS=		../funcs/loadfile.o operamath.dlib filesystem.lib graphics.lib input.lib clib.lib
cube_OBJS=		${cube_CSRC:%.c=%.o}
cube:			${cube_OBJS}
	${linkdev}

mattest_CSRC=		mattest.c mat3mulf16.c
mattest_LIBS=		../funcs/loadfile.o ../funcs/gettime.o operamath.dlib \
			filesystem.lib graphics.lib input.lib clib.lib
mattest_OBJS=		${mattest_CSRC:%.c=%.o} ${mattest_SSRC:%.s=%.o}
mattest:		${mattest_OBJS}
	${linkdev}

testmat_CSRC=		testmat.c
testmat_LIBS=		operamath.dlib filesystem.lib graphics.lib input.lib clib.lib
testmat_OBJS=		${testmat_CSRC:%.c=%.o} ${testmat_SSRC:%.s=%.o}
testmat:		${testmat_OBJS}
	${linkdev}

# ==========

DOCS=			OperamathRelease.doc

# ==========

RCSFILES=		${operamath_lib_CSRC} \
			${operamath_lib_SSRC} \
			${operamath_CSRC} \
			${operamath_SSRC} \
			${testmath_CSRC} \
			${testmath_SSRC} \
			${cube_CSRC} \
			${mattest_CSRC} \
			${testmat_CSRC}

AUTODEPEND= \
	${operamath.dlib_OBJS} \
	${operamath.rlib_OBJS} \
	${operamath.dev_OBJS} \
	${operamath.rom_OBJS} \
	${testmath_OBJS} \
	${cube_OBJS} \
	${mattest_OBJS} \
	${testmat_OBJS}

DEPFILE=	.autodepends

include GNUmakefile.master
