# $Id: GNUmakefile,v 1.29 1994/12/21 06:18:40 phil Exp $
# dsp/GNUmakefile - makefile for DSP instruments.
#
# Copyright (c) 1992, 1993, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all

.PHONY:			dsp installdsp

therest moredevs:	dsp

installtherest installmoredevs:	installdsp


# --------------------- includes

# These files are private, and under RCS control here
LOCAL_INCS=		adpcmduck.j DSPP.j dspp_map.j function_ids.j OscUpDownFP.j

# These files are exported by this package and are under RCS in this directory.
INCS_HERE=

# These files are exported by this package but are under RCS control in "includes".
INCS_DOWN=


# --------------------- forth environment

# forth interpreter and support files
FORTHBIN=		/usr/local/bin
ifndef FORTH
FORTH=			${FORTHBIN}/forth
endif
# BASEDIC=		/opera/archive/forth.unix.94.05.16.dic
BASEDIC=		/opera/archive/forth.unix.94.11.01.dic

# --------------------- DSPP assembler dictionary

DSPPASM_DIC=		dsppasm.dic
DSPPASM_DIC_SRCS=	DSPP.j ansi_fileio_mitch.fth dspp_asm.fth dspp_map.j function_ids.j

${DSPPASM_DIC}: ${DSPPASM_DIC_SRCS}
	@echo "--- building DSPP assembler dictionary."
	( \
		${DSPPASM_DIC_SRCS:%=echo "fload %";} \
		echo 'p" ${DSPPASM_DIC}" save-forth' \
	) | ${FORTH} ${BASEDIC} >/dev/null


# --------------------- DSPP instrument assembly rules

# assembler output files (always generated)
OUTPUT_DSP=	output.dsp
OUTPUT_DEPEND=	output.depend

# .dsp assembler rule
%.dsp: %.ins ${DSPPASM_DIC}
	@echo "---------------------------"
	@${RM} $@ ${OUTPUT_DSP} ${OUTPUT_DEPEND}
	${FORTH} ${DSPPASM_DIC} $*.ins
	@${RM} ${OUTPUT_DEPEND}
	${MV} ${OUTPUT_DSP} $@

# include file dependency generation rule
#     !!! Ultimately DSPP assembler will output the dependency info in a
#     separate file.
.dep.%.dsp: %.ins
	@awk '$$1=="include?" { print "$*.dsp:", $$3 }' <$*.ins >$@
#	@echo "---------------------------"
#	@${RM} ${OUTPUT_DEPEND} ${OUTPUT_DSP} $@
#	${FORTH} ${FORTHDIC} $*.ins
#	@${RM} ${OUTPUT_DSP}
#	${MV} ${OUTPUT_DEPEND} $@


# --------------------- DSP files
# adpcmduck44s.dsp removed because it is not needed and not supportable.

DSP_FILES= \
	add.dsp \
	adpcmhalfmono.dsp \
	adpcmmono.dsp \
	adpcmvarmono.dsp \
	adpcmduck22s.dsp \
	benchmark.dsp \
	dcsqxdhalfmono.dsp \
	dcsqxdhalfstereo.dsp \
	dcsqxdmono.dsp \
	dcsqxdvarmono.dsp \
	dcsqxdstereo.dsp \
	decodeadpcm.dsp \
	deemphcd.dsp \
	delay1tap.dsp \
	delaymono.dsp \
	delaystereo.dsp \
	directin.dsp \
	directout.dsp \
	envelope.dsp \
	envfollower.dsp \
	ezflix225.dsp \
	filterednoise.dsp \
	fixedmono8.dsp \
	fixedmonosample.dsp \
	fixedstereo16swap.dsp \
	fixedstereo8.dsp \
	fixedstereosample.dsp \
	halfmono8.dsp \
	halfmonosample.dsp \
	halfstereo8.dsp \
	halfstereosample.dsp \
	head.dsp \
	impulse.dsp \
	maximum.dsp \
	minimum.dsp \
	mixer12x2.dsp \
	mixer2x2.dsp \
	mixer4x2.dsp \
	mixer8x2.dsp \
	mixer8x2amp.dsp \
	monitor.dsp \
	multiply.dsp \
	noise.dsp \
	oscupdownfp.dsp \
	probe.dsp \
	pulse.dsp \
	pulser.dsp \
	pulse_lfo.dsp \
	randomhold.dsp \
	rednoise.dsp \
	romhead.dsp \
	romtail.dsp \
	sampler.dsp \
	sampler3d.dsp \
	samplerenv.dsp \
	samplermod.dsp \
	sawenv.dsp \
	sawenvsvfenv.dsp \
	sawfilterednoise.dsp \
	sawfilteredsaw.dsp \
	sawtooth.dsp \
	splitexec.dsp \
	square.dsp \
	square_lfo.dsp \
	submixer2x2.dsp \
	submixer4x2.dsp \
	submixer8x2.dsp \
	subtract.dsp \
	svfilter.dsp \
	tail.dsp \
        tapoutput.dsp \
	timesplus.dsp \
	triangle.dsp \
	triangle_lfo.dsp \
	varmono16.dsp \
	varmono8.dsp \
	varmono8_s.dsp

#	sawtooth_lfo.dsp \	@@@ to be added later

DSP_INSSRC=		${DSP_FILES:%.dsp=%.ins}

dsp:			${DSP_FILES}


# --------------------- docs (!!! this might go away)

dsp.doc:		${DSP_INSSRC}
	${ADX} $^ >$@


# --------------------- rcs files and dependencies

RCSFILES=		${DSPPASM_DIC_SRCS} \
			${DSP_INSSRC}

AUTODEPEND=		${DSP_FILES}


# --------------------- master

DEPFILE=		.autodepends
include GNUmakefile.master


# --------------------- installation
# must follow master include because it depends on some variables defined in master

DSP_TARG=		${RELEASEDIR}/remote/audio/dsp

${DSP_TARG}/%: 		%
	@${MD} ${DSP_TARG}
	${copyit}

installdsp:		dsp ${DSP_FILES:%=${DSP_TARG}/%}

