# $Id: GNUmakefile,v 1.64 1995/02/16 22:44:38 vertex Exp $
# GNUmakefile for the Portfolio C Library
#
# Copyright (c) 1992, 1993, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all

# ==========

DOCS=			CLibChangeLog.doc

# ==========

LOCAL_INCS=		stdlibcom.h
INCS_HERE=		clib.h

# ==========

NOWARN=			${NOTHING}

# ==========

LIB_DATE=		builddate

LIBS=			${LOBJ_SSRC:%.s=%.o} ${LOBJ_CSRC:%.c=%.o}
BLIBS=			clib.blib
DLIBS=			clib.dlib
RLIBS=			clib.rlib

# Since the object files for these are named in LIBS, they will
# be compiled and copied into place.

LOBJ_SSRC=		cstartup.s threadstartup.s subroutinestartup.s
LOBJ_CSRC=		copyright.c

# ==========

# Asm source files do not care about what kind of object we are building.
CLIB_SSRC=		allocvectors.s FindMSB.s FindLSB.s changebits.s \
			getpsr.s kbvectors.s rangerand.s samplesystemtimetv.s \
			sdiv.s sdiv10.s sdivtest.s setjmp.s swap.s \
			udiv.s udiv10.s fastfill.s w64copy.s w64zero.s

# These C source files do not care about what kind of object we are building.
CLIB_CCSRC=		addtimes.c atoi.c autosize.c availMem.c \
			bcopy.c bsearch.c bzero.c calloc.c cfree.c \
			chval.c comparetimes.c CountBits.c \
			createBufferedMsg.c createDev.c createDrvr.c \
			createFIRQ.c createIOReq.c createitemva.c \
			createMsg.c createMsgPort.c createSema4.c \
			createsizeditemva.c createSizedMsg.c \
			createSmallMsg.c createThread.c \
			createtimerio.c createUniqueMsgPort.c \
			createUniqueSema4.c ctype.c deletetimerio.c \
			doio.c dov.c dumpnode.c dumptaglist.c errno.c \
			fclose.c fgetc.c filbuf.c findandopenitem.c \
			findandopenitemva.c finditem.c finditemva.c \
			findnodefromhead.c findnodefromtail.c \
			findversioneditem.c fopen.c fputs.c fread.c \
			free.c fseek.c ftell.c fwrite.c getnodecount.c \
			getnodeposfromhead.c getnodeposfromtail.c \
			gettagarg.c insertnodeafter.c \
			insertnodebefore.c isuser.c malloc.c memchr.c \
			memcmp.c memmove.c nofpdisplay.c noop.c \
			panic.c perror.c printf.c qsort.c rand.c \
			readMsg.c realloc.c remove.c scanf.c sprintf.c \
			stdlib.c strcasecmp.c strcat.c strchr.c \
			strcmp.c strcpy.c strcspn.c strerror.c \
			strlen.c strncasecmp.c strncat.c strncmp.c \
			strncpy.c strpbrk.c strrchr.c strspn.c \
			strstr.c strtok.c strtol.c strtoul.c \
			subtimes.c syserr.c system.c tmpfile.c \
			typeofmem.c ungetc.c vector10.c vector13.c \
			vector14.c vector17.c vector19.c vector1.c \
			vector20.c vector21.c vector22.c vector2.c \
			vector38.c vector39.c vector3.c vector44.c \
			vector45.c vector4.c vector5.c vector6.c \
			vector9.c version.c vprintf.c vsprintf.c \
			waittime.c waituntil.c

# These C source files *do* care about what kind of object we are building.
CLIB_CVSRC=		prterr.c

CLIB_CSRC=		${CLIB_CCSRC} ${CLIB_CVSRC}

# ==========

clib.blib_OBJS=		${CLIB_SSRC:%.s=%.o} ${CLIB_CCSRC:%.c=%.o} \
			${CLIB_CVSRC:%.c=%.bo}
clib.blib:		${clib.blib_OBJS}
	${linklib}

clib.dlib_OBJS=		${CLIB_SSRC:%.s=%.o} ${CLIB_CCSRC:%.c=%.o} \
			${CLIB_CVSRC:%.c=%.do}
clib.dlib:		${clib.dlib_OBJS}
	${linklib}

clib.rlib_OBJS=		${CLIB_SSRC:%.s=%.o} ${CLIB_CCSRC:%.c=%.o} \
			${CLIB_CVSRC:%.c=%.ro}
clib.rlib:		${clib.rlib_OBJS}
	${linklib}

# ==========

RCSFILES=		${CLIB_SSRC} ${CLIB_CSRC} ${CLIB_HSRC} \
			${LOBJ_SSRC} ${LOBJ_CSRC}

AUTODEPEND=		${clib.blib_OBJS} ${clib.dlib_OBJS} ${clib.rlib_OBJS}

DEPFILE=	.autodepends
include GNUmakefile.master
