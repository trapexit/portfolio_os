# $Id: GNUmakefile,v 1.13 1994/08/16 21:29:52 vertex Exp $
# GNUmakefile for the Kernel Library
#
# Copyright (c) 1992, 1993, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all

DOCS=			KernelLibNotes.doc

BLIBS=			kernel.blib
DLIBS=			kernel.dlib
RLIBS=			kernel.rlib

SWST=			${NOTHING}
NOWARN=			${NOTHING}

LOCAL_INCS=		createNamedItem.h

KERNELLIB_SSRC=		superdo.s  supervectors.s
KERNELLIB_CSRC=		CreateItem.c createFIRQ.c createHLInt.c createIOReq.c \
			finditem.c createSmallMsg.c \
			createBufferedMsg.c createMsgPort.c createSema4.c \
			createMsg.c createNamedItem.c usermodeserver.c

kernel.blib_OBJS=	${KERNELLIB_SSRC:%.s=%.bo} ${KERNELLIB_CSRC:%.c=%.bo}
kernel.blib:		${kernel.blib_OBJS}
	${linklib}

kernel.dlib_OBJS=	${KERNELLIB_SSRC:%.s=%.do} ${KERNELLIB_CSRC:%.c=%.do}
kernel.dlib:		${kernel.dlib_OBJS}
	${linklib}

kernel.rlib_OBJS=	${KERNELLIB_SSRC:%.s=%.ro} ${KERNELLIB_CSRC:%.c=%.ro}
kernel.rlib:		${kernel.rlib_OBJS}
	${linklib}

RCSFILES=		${KERNELLIB_SSRC} ${KERNELLIB_CSRC}

AUTODEPEND=		${kernel.dlib_OBJS} ${kernel.rlib_OBJS}

DEPFILE=	.autodepends
include GNUmakefile.master
