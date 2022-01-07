# $Id: GNUmakefile,v 1.13 1994/10/11 18:51:35 limes Exp $
# GNUmakefile for the Portfolio Misc Code
#
# Copyright (c) 1992, 1993, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all

# ==========

CDEFS=			-DKERNEL -DSUPER
APCS=			-apcs/swstackcheck
MAP=			-MAP

# ==========

HDW=			misc

misc_SSRC=		startup.s memcpyset.s list.s sdiv.s sdiv10.s \
			udiv.s udiv10.s
misc_CSRC=		main.c initlist.c

misc_OBJS=		$(misc_SSRC:%.s=%.o) $(misc_CSRC:%.c=%.o)
misc_MODNAME=		Misc
#misc_MODBIN=		-priv
misc: ${misc_OBJS} fixmisc
	${linkdev}
	${compress}
	./fixmisc

fixmisc: fixmisc.c
	gcc fixmisc.c -o fixmisc

# ==========

RCSFILES=		${misc_CSRC} ${misc_SSRC} fixmisc.c

AUTODEPEND=		${misc_OBJS}

DEPFILE=	.autodepends
include GNUmakefile.master

# Ugly Overrides go here.

# No LIBCRT0 support in the misc code.
LIBCRT0=		${NOTHING}
