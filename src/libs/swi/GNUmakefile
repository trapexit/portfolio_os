# $Id: GNUmakefile,v 1.7 1994/03/29 06:10:42 local Exp $
# GNUmakefile for the SWI library
#
# Copyright (c) 1992, 1993, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all

SWST=			${NOTHING}
NOWARN=			${NOTHING}

LIBS=			swi.lib

swi.lib_SSRC=		SendIO.s DeleteItem.s OpenItem.s kprintf.s Superbcopy.s
swi.lib_OBJS=		${swi.lib_SSRC:%.s=%.o}
swi.lib:		${swi.lib_OBJS}
	${linklib}

RCSFILES=		${swi.lib_SSRC}

AUTODEPEND=		${swi.lib_OBJS}

DEPFILE=	.autodepends
include GNUmakefile.master
