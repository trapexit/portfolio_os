# $Id: GNUmakefile,v 1.60 1994/11/22 23:59:36 markn Exp $
# GNUmakefile for the Portfolio Kernel
#
# Copyright (c) 1992, 1993, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all

# ==========

.tree: .includes

# ==========

DOCS=			KernelReleaseNotes.doc KernelChangeLog.doc aif.5

# ==========

LOCAL_INCS=		sherrie.i macros.i conio.h includes/internalf.h

INCS_HERE=		registers.i macros.a gfx.h Charset.a scc8530.h listmacros.h

INCS_DOWN=		aif.h bootdata.h ctype.h device.h device.i driver.h \
			folio.h folio.i hardware.h \
			interrupts.h interrupts.i io.h io.i item.h \
			item.i kernel.h kernel.i kernelnodes.h \
			kernelnodes.i kernelmacros.i list.h list.i \
			mem.h miscfunc.h mem.i mmu.h msgport.h \
			msgport.i sysinfo.h nodes.h nodes.i operror.h \
			semaphore.h semaphore.i stdarg.h rsa.h stdio.h \
			string.h strings.h tags.h task.h task.i timer.h \
			timer.i types.h varargs.h time.h time.i clio.h \
			sherryvers.h usermodeservices.h sysstate.h

# ==========

DATE=			BuildDate

APCS=			-apcs/swstackcheck

CDEFS=			-DKERNEL -DSUPER

LCRT0_SSRC=		startup.s

# RELOC=			${NOTHING}
MAP=			-MAP

# ==========

gentimezone: gentimezone.c
	gcc gentimezone.c -o gentimezone

timezone.c: gentimezone
	./gentimezone > timezone.c

# ==========

sherry_SSRC=		acs.s getpsr.s sherry.s Video6.s CopyArgsToStack.s \
			samplesystemtime.s
sherry_CSRC=		main.c mem.c io.c item.c semaphore.c sendio.c \
			drivers.c devices.c timer.c allocmem.c \
			isuser.c pagemem.c task.c folio.c ports.c \
			firq.c mmu.c exceptions.c signal.c printf.c \
			vdl.c fence.c kernelgfx.c rsa.c tags.c \
			syserr.c random.c stack.c otherdocs.c sysinfo.c \
			gcio.c olio.c ssio.c zsio.c usermodeservices.c sysstate.c
sherry_ASRC=		timezone.c

BHDW=			sherry.boot
DHDW=			sherry.dev
RHDW=			sherry.rom

# ==========

sherry_xmisc_SSRC=	misc.s
sherry_xmisc_CSRC=	miscfuncs.c

sherry_lmisc_SSRC=	memcpyset.s list.s sdiv.s sdiv10.s udiv.s udiv10.s
sherry_lmisc_CSRC=	initlist.c

# boot kernels use a separated MISC code module.

CBDEFS=			-DEXT_MISC
SBDEFS=			-PD "INT_MISC SETA 0" -PD "EXT_MISC SETA 1"

# dev kernels use a separated MISC code module.

CDDEFS=			-DEXT_MISC
SDDEFS=			-PD "INT_MISC SETA 0" -PD "EXT_MISC SETA 1"

# rom kernels link the misc functions directly.

CRDEFS=			-DINT_MISC
SRDEFS=			-PD "INT_MISC SETA 1" -PD "EXT_MISC SETA 0"

# ==========

sherry_MODBIN=		-type 4 -stack 8192 -noswi
sherry_MODNAME=		Sherry

sherry.boot_LIBS=	clib.lib
sherry.boot_OBJS=	${sherry_SSRC:%.s=%.bo} ${sherry_CSRC:%.c=%.bo} \
			${sherry_xmisc_SSRC:%.s=%.bo} ${sherry_xmisc_CSRC:%.c=%.bo} \
			${sherry_ASRC:%.c=%.bo}
sherry.boot:		${LCRT0_SSRC:%.s=%.bo} ${sherry.boot_OBJS}
	${linkboot}
	${setrev}
	${compress}
	${setpriv}

sherry.dev_LIBS=	clib.lib
sherry.dev_OBJS=	${sherry_SSRC:%.s=%.do} ${sherry_CSRC:%.c=%.do} \
			${sherry_xmisc_SSRC:%.s=%.do} ${sherry_xmisc_CSRC:%.c=%.do} \
			${sherry_ASRC:%.c=%.do}
sherry.dev:		${LCRT0_SSRC:%.s=%.do} ${sherry.dev_OBJS}
	${linkdev}
	${setrev}
	${compress}
	${setpriv}

sherry.rom_LIBS=	clib.lib
sherry.rom_OBJS=	${sherry_SSRC:%.s=%.ro} ${sherry_CSRC:%.c=%.ro} \
			${sherry_lmisc_SSRC:%.s=%.ro} ${sherry_lmisc_CSRC:%.c=%.ro} \
			${sherry_ASRC:%.c=%.ro}
sherry.rom:		${LCRT0_SSRC:%.s=%.ro} ${sherry.rom_OBJS}
	${linkrom}
	${setrev}
	${compress}
	${setpriv}

# ==========

opera.sym: .FORCE
	operasym ../../src | cat ../../symhead - ../../symtail > opera.sym

macstuff: .FORCE
	${MAKE} -C ../.. $@

.FORCE:

# ==========

RCSFILES=		${sherry_CSRC} ${sherry_SSRC} \
			${sherry_xmisc_CSRC} ${sherry_xmisc_SSRC} \
			${sherry_lmisc_CSRC} ${sherry_lmisc_SSRC} \
			${LCRT0_SSRC} gentimezone.c

AUTODEPEND=		${sherry.dev_OBJS} ${sherry.boot_OBJS} ${sherry.rom_OBJS}

DEPFILE=	.autodepends
include GNUmakefile.master

# No to startup.s, but Yes to copyright.
# And this is *after* the LCRT stuff.

LIBCRT0=		copyright.o
