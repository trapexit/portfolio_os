# $Id: GNUmakefile,v 1.73.1.1 1994/12/21 21:43:02 markn Exp $

# GNUmakefile for dipir.
#
# Copyright (c) 1992, 1993, 1994 The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

# To make a dipir, you must specify a target architecture and a configuration.
# These can be specified by setting DIPIR_ARCH and DIPIR_CONFIG in the
# environment.  If they are not set, the default is to build all production
# (non-development) configs for all architectures.

# Resulting objects and binaries are put in a 
# subdirectory $DIPIR_ARCH/$DIPIR_CONFIG.

# we do not want ${MODBIN} automagicly run for all our links.
# This means we have to ${MODBIN} manually where we want it ...
automodbin=		; true

CCOPTIMIZE=		-Ospace
MAKE_PAKHDR=		./make_pakhdr.sh
AIFSIZE=		./aifsize.sh

############################################################################
# Possible target architectures and configurations.
ARCHS=			opera operauk anvil lccd creative cable
CONFIGS=		enc    unenc    null    demo    exhst \
			encdev unencdev nulldev demodev exhstdev

# Default target architectures and configurations.
DEFAULT_ARCHS=		opera operauk anvil creative cable lccd
DEFAULT_CONFIGS=	enc unenc nulldev 

# List of drivers and their prefixes.
DIPIR_DRIVERS=		meidev.drv \
			fmvdev.drv \
			mei563dev.drv \
			lccddev.drv \
			depotdev.drv

meidev_PREFIX=          MEICD
fmvdev_PREFIX=          FMV
mei563dev_PREFIX=       MEICD563
lccddev_PREFIX=         LCCD
depotdev_PREFIX=	DEPOT

############################################################################
# Our private include files ...
LOCAL_INCS=		bsafe2.h global.h atypes.h programmer.h \
			dipir.h md5.h stack.i md5.i dipir_rev.h \
			meidev_rev.h \
			fmvdev_rev.h \
			mei563dev_rev.h \
			lccddev.h lccddev_rev.h \
			depot.h depotdev_rev.h

############################################################################
# Compiler flags associated with each config.
CONFIG_enc=		-DENCRYPT -DROMBUILD 
CONFIG_encdev=		-DENCRYPT
CONFIG_unenc=		-DENG_KEY -DDEMO_KEY -DLEAKY_DIPIR -DROMBUILD 
CONFIG_unencdev=	-DENG_KEY -DDEMO_KEY -DLEAKY_DIPIR 
CONFIG_demo=		-DENG_KEY -DDEMO_KEY -DROMBUILD 
CONFIG_demodev=		-DENG_KEY -DDEMO_KEY 
CONFIG_null=		-DENG_KEY -DDEMO_KEY -DLEAKY_DIPIR -DNULL_DIPIR -DROMBUILD 
CONFIG_nulldev=		-DENG_KEY -DDEMO_KEY -DLEAKY_DIPIR -DNULL_DIPIR 
CONFIG_exhst=		-DDIGEST_WHOLE_DISC -DENCRYPT -DROMBUILD 
CONFIG_exhstdev=	-DDIGEST_WHOLE_DISC -DENCRYPT

# Compiler flags associated with each target architecture.
ARCH_opera=		-DMEICD_DRIVER
ARCH_operauk=		-DMEICD_DRIVER -DFZ1_UK 
ARCH_anvil=		-DANVIL -DMEICD_DRIVER -DLCCD_DRIVER -DSWECC
# lccd ARCH is temporary; for debugging only
ARCH_lccd=		-DANVIL -DMEICD_DRIVER -DLCCD_DRIVER -DSWECC -DALLOW_NONZERO_DEVICE
ARCH_creative=		-DCREATIVE -DMEICD_DRIVER -DMEICD563_DRIVER -DMEICD563 
ARCH_cable=		-DCABLE -DMEICD_DRIVER -DNO_DIPIR_FMV -DBOOT_DEVICE_UNIT_NUMBER=0x81 

# Compiler flags associated with swappable drivers.
DRVFLAGS=		-DROMBUILD

echoconfig=	@echo "linking ${DIPIR_CONFIG} ${DIPIR_ARCH} $@"

# Everyone gets -DKERNEL -DSUPER and the config flags.
CDEFS=			-DKERNEL -DSUPER ${DIPIR_DEBUG} \
			${CONFIG_${DIPIR_CONFIG}} ${ARCH_${DIPIR_ARCH}}

CDDEFS=			${NOTHING}
SDDEFS=			${NOTHING}

############################################################################
# Miscellaneous definitions.

# no stack checking in dipir
SWST=			${NOTHING}
APCS=			-apcs/noswstackcheck
ZPS=			-zps1
WA=			-Wa

# Some links use -MAP; some do not (actually, they all do now).
MAP=			${$@_MAP}

# Some links use -R; some do not (actually, they all do now).
RELOC=			${$@_RELOC}

# Everyone gets a shot at clib.
# warning:  any calls with hidden SWIs won't work in the dipir environment
LINK_LIBS=		clib.lib

ifdef	DIPIR_CONFIG
ifdef	DIPIR_ARCH
HDW=			${DIPIR_ARCH}/${DIPIR_CONFIG}/dipir \
			${DIPIR_ARCH}/${DIPIR_CONFIG}/driver.pak
LOCALS=			${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir \
			${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir_dd \
			${DIPIR_ARCH}/${DIPIR_CONFIG}/fmvdipir \
			${DIPIR_ARCH}/${DIPIR_CONFIG}/depotdipir
endif
endif

############################################################################
# Implicit rules for building object files (in subdirectories).

${DIPIR_ARCH}/${DIPIR_CONFIG}/%.o: %.s
	${AS} ${SFLAGS}    $< -o $@
${DIPIR_ARCH}/${DIPIR_CONFIG}/%.o: %.c
	${CC} ${CFLAGS} -c $< -o $@
${DIPIR_ARCH}/${DIPIR_CONFIG}/%.splo: %.c
	${CC} ${CFLAGS} -DAPPSPLASH -c $< -o $@
${DIPIR_ARCH}/${DIPIR_CONFIG}/%.digo: %.c
	${CC} ${CFLAGS} -DAPPDIGEST -c $< -o $@
${DIPIR_ARCH}/${DIPIR_CONFIG}/%.dso: %.c
	${CC} ${CFLAGS} -DAPPDIGEST -DAPPSPLASH -c $< -o $@
%.drvo: %.c
	${CC} ${CFLAGS} ${DRVFLAGS} -c \
	    -D${$*_PREFIX}_DRIVER -D${$*_PREFIX}_InitDisc=InitDisc $< -o $@
%.s: %.id drvhdr.s
	cat $*.id drvhdr.s >$*.s

linkdriver= \
	${LD} -AIF -R -S $@.sym -o $@ \
		${@:.drv=}h.o ${${@:.drv=}_OBJS} ${DOLINK_DLIBS}; \
	${MODBIN} -noswi $@; \
	${setpriv}

############################################################################
# just building does a "production" build
default: production

############################################################################
# attach us to the rest of the build system for nightly builds

installlibs.here: libs.here

installtherest.here: installtherest.dipirs

installtherest.dipirs: production
	${MAKE} copydipirs

############################################################################
# Make targets

production:
	@if [ -z "${DIPIR_ARCH}" ]; then ARCH_LIST="${DEFAULT_ARCHS}"; \
		else ARCH_LIST="${DIPIR_ARCH}"; fi; \
	if [ -z "${DIPIR_CONFIG}" ]; then CONFIG_LIST="${DEFAULT_CONFIGS}"; \
		else CONFIG_LIST="${DIPIR_CONFIG}"; fi; \
	for arch in $$ARCH_LIST; do \
	  for config in $$CONFIG_LIST; do \
	    echo "===== building $$arch $$config =====" ; \
	    ${MAKE} DIPIR_ARCH=$$arch DIPIR_CONFIG=$$config all ; \
	done; done


# A driver package (driver.pak) consists of a pak header followed by
# the individual drivers.
# Source for a swappable driver consists of driver.c and driverh.id files.
# The .id file is concatenated with the standard driver header drvhdr.s
# and builds a driverh.o file, which will be the header of the driver.drv file.
# The driver.c file is compiled into a driver.o, and then the
# driver.o and driverh.o are linked into a driver.drv binary.
# All the driver.drv files are collected and make_pakhdr creates a
# pak header (Table Of Contents) for the entire package, pakhdr.s.
# This is assembled and linked into pakhdr, to which is appended all the
# driver.drv files to create the finished driver.pak.
PACK_CONTENTS= ${DIPIR_ARCH}/${DIPIR_CONFIG}/icddipir ${DIPIR_DRIVERS}

${DIPIR_ARCH}/${DIPIR_CONFIG}/pakhdr.s: ${PACK_CONTENTS}
	rm -f ${DIPIR_ARCH}/${DIPIR_CONFIG}/pakhdr.s
	${MAKE_PAKHDR} -f ${DIPIR_ARCH}/${DIPIR_CONFIG}/pakhdr.s ${PACK_CONTENTS}
${DIPIR_ARCH}/${DIPIR_CONFIG}/pakhdr: ${DIPIR_ARCH}/${DIPIR_CONFIG}/pakhdr.o
	${LD} -BIN -o ${DIPIR_ARCH}/${DIPIR_CONFIG}/pakhdr \
		${DIPIR_ARCH}/${DIPIR_CONFIG}/pakhdr.o
${DIPIR_ARCH}/${DIPIR_CONFIG}/driver.pak: \
		${DIPIR_ARCH}/${DIPIR_CONFIG}/pakhdr ${PACK_CONTENTS}
	cat ${DIPIR_ARCH}/${DIPIR_CONFIG}/pakhdr ${PACK_CONTENTS} \
		>${DIPIR_ARCH}/${DIPIR_CONFIG}/driver.pak


rtt_OBJS=		${rtt_SSRC:%.s=%.o}
rtt_LTYPE=		-BIN
rtt:		${rtt_OBJS}
	${echoconfig}
	${linkdev}

# Make subdirectories.
dirs:
	if [ "${DIPIR_ARCH}/${DIPIR_CONFIG}" != "/" ]; then \
		${MD} ${DIPIR_ARCH}/${DIPIR_CONFIG}; fi
	
############################################################################
# dipir: The dipir that sits in the ROM
dipir_SSRC=		dipir.s md5s.s rangerand.s rsa.s read4bytes.s ecc32.s
dipir_CSRC=		rsadipir.c t_rsa.c ntgdigest.c rand.c \
			dipirutils.c romsysinfo.c display.c
dipir_XSRC=		cdipir.c 
${DIPIR_ARCH}/${DIPIR_CONFIG}/dipir_OBJS= \
	${dipir_SSRC:%.s=${DIPIR_ARCH}/${DIPIR_CONFIG}/%.o} \
	${dipir_CSRC:%.c=${DIPIR_ARCH}/${DIPIR_CONFIG}/%.o} \
	${dipir_XSRC:%.c=${DIPIR_ARCH}/${DIPIR_CONFIG}/%.o} 
${DIPIR_ARCH}/${DIPIR_CONFIG}/dipir_LIBS=	
${DIPIR_ARCH}/${DIPIR_CONFIG}/dipir_BASE=	0x200
${DIPIR_ARCH}/${DIPIR_CONFIG}/dipir_LTYPE=	-BIN
${DIPIR_ARCH}/${DIPIR_CONFIG}/dipir_MAP=	
${DIPIR_ARCH}/${DIPIR_CONFIG}/dipir: ${${DIPIR_ARCH}/${DIPIR_CONFIG}/dipir_OBJS}
	${echoconfig}
	${linkdev}

############################################################################
# cddipir: The dipir that sits on a title CD.
cddipir_SSRC=		cddipir.s rangerand.s 
cddipir_CSRC=		dipirutils.c rand.c
cddipir_XSRC=		cdromdipir.c appdigest.c display.c
${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir_OBJS= \
	${cddipir_SSRC:%.s=${DIPIR_ARCH}/${DIPIR_CONFIG}/%.o} \
	${cddipir_CSRC:%.c=${DIPIR_ARCH}/${DIPIR_CONFIG}/%.o} \
	${cddipir_XSRC:%.c=${DIPIR_ARCH}/${DIPIR_CONFIG}/%.splo} 
${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir_LIBS=	
${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir_BASE=	0x0
${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir_LTYPE=	-AIF
${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir_RELOC=	-R
${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir_MAP=	
${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir: ${${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir_OBJS}
	${echoconfig}
	${linkdev}
	@sz=`${AIFSIZE} ${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir`; \
	if [ $$sz -gt 8128 ]; then \
	 echo "ERROR: ${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir size $$sz > 8128"; \
	 exit 1; \
	else \
	 echo "${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir size $$sz ok"; \
	fi

############################################################################
# cddipir_dd: The dipir that sits on a data-disc CD.
cddipir_dd_SSRC=	cddipir.s rangerand.s 
cddipir_dd_CSRC=	dipirutils.c rand.c
cddipir_dd_XSRC=	cdromdipir.c appdigest.c
${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir_dd_OBJS= \
	${cddipir_dd_SSRC:%.s=${DIPIR_ARCH}/${DIPIR_CONFIG}/%.o} \
	${cddipir_dd_CSRC:%.c=${DIPIR_ARCH}/${DIPIR_CONFIG}/%.o} \
	${cddipir_dd_XSRC:%.c=${DIPIR_ARCH}/${DIPIR_CONFIG}/%.digo} 
${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir_dd_LIBS=	
${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir_dd_BASE=	0x0
${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir_dd_LTYPE=	-AIF
${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir_dd_RELOC=	-R
${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir_dd_MAP=	
${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir_dd: ${${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir_dd_OBJS}
	${echoconfig}
	${linkdev}
	@sz=`${AIFSIZE} ${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir_dd`; \
	if [ $$sz -gt 8128 ]; then \
	 echo "ERROR: ${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir_dd size $$sz > 8128"; \
	 exit 1; \
	else \
	 echo "${DIPIR_ARCH}/${DIPIR_CONFIG}/cddipir_dd size $$sz ok"; \
	fi

############################################################################
# icddipir: The version of cddipir that is in the ROM (in driver.pak).
icddipir_SSRC=		cddipir.s rangerand.s 
icddipir_CSRC=		dipirutils.c rand.c
icddipir_XSRC=		cdromdipir.c appdigest.c display.c
${DIPIR_ARCH}/${DIPIR_CONFIG}/icddipir_OBJS= \
	${icddipir_SSRC:%.s=${DIPIR_ARCH}/${DIPIR_CONFIG}/%.o} \
	${icddipir_CSRC:%.c=${DIPIR_ARCH}/${DIPIR_CONFIG}/%.o} \
	${icddipir_XSRC:%.c=${DIPIR_ARCH}/${DIPIR_CONFIG}/%.dso} 
${DIPIR_ARCH}/${DIPIR_CONFIG}/icddipir_LIBS=	
${DIPIR_ARCH}/${DIPIR_CONFIG}/icddipir_BASE=	0x0
${DIPIR_ARCH}/${DIPIR_CONFIG}/icddipir_LTYPE=	-AIF
${DIPIR_ARCH}/${DIPIR_CONFIG}/icddipir_RELOC=	-R
${DIPIR_ARCH}/${DIPIR_CONFIG}/icddipir_MAP=	
${DIPIR_ARCH}/${DIPIR_CONFIG}/icddipir: ${${DIPIR_ARCH}/${DIPIR_CONFIG}/icddipir_OBJS}
	${echoconfig}
	${linkdev}

############################################################################
# fmvdipir: The dipir that sits on the FMV card
fmvdipir_SSRC=		cddipir.s
fmvdipir_CSRC=		dipirutils.c
fmvdipir_XSRC=		fmvdipir.c display.c
${DIPIR_ARCH}/${DIPIR_CONFIG}/fmvdipir_OBJS= \
	${fmvdipir_SSRC:%.s=${DIPIR_ARCH}/${DIPIR_CONFIG}/%.o} \
	${fmvdipir_CSRC:%.c=${DIPIR_ARCH}/${DIPIR_CONFIG}/%.o} \
	${fmvdipir_XSRC:%.c=${DIPIR_ARCH}/${DIPIR_CONFIG}/%.o} 
${DIPIR_ARCH}/${DIPIR_CONFIG}/fmvdipir_RELOC=	-R
${DIPIR_ARCH}/${DIPIR_CONFIG}/fmvdipir_BASE=	0x0
${DIPIR_ARCH}/${DIPIR_CONFIG}/fmvdipir_MAP=	
${DIPIR_ARCH}/${DIPIR_CONFIG}/fmvdipir_LTYPE=	-AIF
${DIPIR_ARCH}/${DIPIR_CONFIG}/fmvdipir: ${${DIPIR_ARCH}/${DIPIR_CONFIG}/fmvdipir_OBJS}
	${echoconfig}
	${linkdev}

############################################################################
# depotdipir: The dipir that sits on a Depot ROM.
depotdipir_SSRC=	cddipir.s
depotdipir_CSRC=	dipirutils.c
depotdipir_XSRC=	depotdipir.c display.c
${DIPIR_ARCH}/${DIPIR_CONFIG}/depotdipir_OBJS= \
	${depotdipir_SSRC:%.s=${DIPIR_ARCH}/${DIPIR_CONFIG}/%.o} \
	${depotdipir_CSRC:%.c=${DIPIR_ARCH}/${DIPIR_CONFIG}/%.o} \
	${depotdipir_XSRC:%.c=${DIPIR_ARCH}/${DIPIR_CONFIG}/%.o} 
${DIPIR_ARCH}/${DIPIR_CONFIG}/depotdipir_RELOC=	-R
${DIPIR_ARCH}/${DIPIR_CONFIG}/depotdipir_BASE=	0x0
${DIPIR_ARCH}/${DIPIR_CONFIG}/depotdipir_MAP=	
${DIPIR_ARCH}/${DIPIR_CONFIG}/depotdipir_LTYPE=	-AIF
${DIPIR_ARCH}/${DIPIR_CONFIG}/depotdipir: ${${DIPIR_ARCH}/${DIPIR_CONFIG}/depotdipir_OBJS}
	${echoconfig}
	${linkdev}

############################################################################
# drvdipir: The dipir that sits on a downloadable driver rom
drvdipir_SSRC=		cddipir.s
drvdipir_CSRC=		devdipir.c dipirutils.c
${DIPIR_ARCH}/${DIPIR_CONFIG}/drvdipir_OBJS= \
	${drvdipir_SSRC:%.s=${DIPIR_ARCH}/${DIPIR_CONFIG}/%.o} \
	${drvdipir_CSRC:%.c=${DIPIR_ARCH}/${DIPIR_CONFIG}/%.o} 
${DIPIR_ARCH}/${DIPIR_CONFIG}/drvdipir_RELOC=	-R
${DIPIR_ARCH}/${DIPIR_CONFIG}/drvdipir_BASE=	0x0
${DIPIR_ARCH}/${DIPIR_CONFIG}/drvdipir_LTYPE=	-AIF
${DIPIR_ARCH}/${DIPIR_CONFIG}/drvdipir: ${${DIPIR_ARCH}/${DIPIR_CONFIG}/drvdipir_OBJS}
	${echoconfig}
	${linkdev}
	@driverrsapriv drvdipir drvdipir.priv nopriv
	@rm drvdipir
	mv drvdipir.priv drvdipir


############################################################################
# Rules for building dipir device drivers.
############################################################################

############################################################################
# meidev: The driver for the MEI-560 CD drive.
meidev_SSRC=		
meidev_CSRC=		meidev.c
meidev_OBJS=		${meidev_SSRC:%.s=%.drvo} ${meidev_CSRC:%.c=%.drvo}
meidev.drv: meidevh.o ${meidev_OBJS}
	${linkdriver}

############################################################################
# fmvdev: The driver for the DVM (FMV) card.
fmvdev_SSRC=		
fmvdev_CSRC=		fmvdev.c
fmvdev_OBJS=		${fmvdev_SSRC:%.s=%.drvo} ${fmvdev_CSRC:%.c=%.drvo}
fmvdev.drv: fmvdevh.o ${fmvdev_OBJS}
	${linkdriver}

############################################################################
# depotdev: The driver for the Depot/Visa.
depotdev_SSRC=		
depotdev_CSRC=		depotdev.c
depotdev_OBJS=		${depotdev_SSRC:%.s=%.drvo} ${depotdev_CSRC:%.c=%.drvo}
depotdev.drv: depotdevh.o ${depotdev_OBJS}
	${linkdriver}

############################################################################
# mei563dev: The driver for the MEI-563 CD drive.
mei563dev_SSRC=		
mei563dev_CSRC=		mei563dev.c
mei563dev_OBJS=		${mei563dev_SSRC:%.s=%.drvo} ${mei563dev_CSRC:%.c=%.drvo}
mei563dev.drv: mei563devh.o ${mei563dev_OBJS}
	${linkdriver}

############################################################################
# lccddev.drv: The driver for the Low-Cost CD drive.
lccddev_SSRC=		
lccddev_CSRC=		lccddev.c
lccddev_OBJS=		${lccddev_SSRC:%.s=%.drvo} ${lccddev_CSRC:%.c=%.drvo}
lccddev.drv: lccddevh.o ${lccddev_OBJS}
	${linkdriver}

############################################################################
# Other targets (cleanup, dependencies, etc.)

drivers_CSRC=	${DIPIR_DRIVERS:%.drv=%.c}
drivers_SSRC=	drvhdr.s ${DIPIR_DRIVERS:%.drv=%h.id}
drivers_MSRC=	make_pakhdr.sh aifsize.sh


RCSFILES=	${rtt_SSRC} \
		${fmvdipir_SSRC} ${fmvdipir_CSRC} ${fmvdipir_XSRC} \
		${cddipir_SSRC} ${cddipir_CSRC} ${cddipir_XSRC} \
		${dipir_SSRC} ${dipir_CSRC} ${dipir_XSRC} \
		${depotdipir_SSRC} ${depotdipir_CSRC} ${depotdipir_XSRC} \
		${drvdipir_SSRC} ${drvdipir_CSRC} \
		${drivers_SSRC} ${drivers_CSRC} ${drivers_MSRC}

AUTODEPEND=	${rtt_OBJS} \
		${fmvdipir_OBJS} \
		${cddipir_OBJS} \
		${dipir_OBJS} \
		${drvdipir_OBJS}

DEPFILE=	.autodepends

include GNUmakefile.master

# Ugly Overrides go here.
LIBCRT0=		copyright.o

clean:
	/bin/rm -rf ${ARCHS} rtt *.drv *.drvo

cleanest: clean

############################################################################
# Dipir copying rules
#	Must go after inclusion of GNUmakefile.master,
#	as the copying build rules need variables defined there.

DEVSTN_CONFIG=	nulldev
BOOTCD_CONFIG=	unenc
BOOTCD_ARCH=	opera

${HDW_DTARG}/dipir.%: %/${DEVSTN_CONFIG}/dipir
	${copyit}

${HDW_BTARG}/cddipir: ${BOOTCD_ARCH}/${BOOTCD_CONFIG}/cddipir
	${copyit}

copydipirs: ${HDW_BTARG}/cddipir ${DEFAULT_ARCHS:%=${HDW_DTARG}/dipir.%}
	/bin/ls -l ${HDW_BTARG}/cddipir ${DEFAULT_ARCHS:%=${HDW_DTARG}/dipir.%}

.tree.here: dirs
