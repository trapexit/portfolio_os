# $Id: GNUmakefile,v 1.27 1994/10/29 20:02:57 vertex Exp $
# GNUmakefile for Portfolio Scripts
#
# Copyright (c) 1992, 1993, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all

DOCS=			3DOBoot.doc

BOOTSCRIPTS=		3DOBoot.anvil.scr \
			3DOBoot.opera.scr \
			3DOBoot.operauk.scr

pop.here: ${BOOTSCRIPTS}

3DOBoot.anvil.scr:	3DOBoot.scr.m4
	${RM} $@; echo -n 'define(ANVIL,1)define(TWOMEG,1)' | cat - 3DOBoot.scr.m4 | m4 > $@

3DOBoot.opera.scr:	3DOBoot.scr.m4
	${RM} $@; echo -n 'define(OPERA,1)define(TWOMEG,1)' | cat - 3DOBoot.scr.m4 | m4 > $@

3DOBoot.operauk.scr:	3DOBoot.scr.m4
	${RM} $@; echo -n 'define(OPERAUK,1)define(TWOMEG,1)' | cat - 3DOBoot.scr.m4 | m4 > $@

# We *can* generate a creative boot script,
# but it is not needed (as per CR 2715).

3DOBoot.creative.scr:	3DOBoot.scr.m4
	${RM} $@; echo -n 'define(CREATIVE,1)define(TWOMEG,1)' | cat - 3DOBoot.scr.m4 | m4 > $@

RCSFILES :=		3DOBoot.scr.m4 \
			AppStartup AppStartup.rom AppStartup.debug \
			NoCD startopera \
			dir.layout \
			access.sh all.sh \
			audio.sh cdboot.sh cddipir.sh copyright.sh \
			dipir.sh directories.sh \
			filesystem.dev.sh filesystem.sh graphics.sh \
			includes.sh input.dev.sh input.sh kernel.sh \
			lib3DO.sh libs.sh mac.sh markem.sh misc.sh \
			misc1.sh operator.sh release.sh rom.sh \
			scripts.update shell.cd.sh shell.sh \
			tag.sh taginc.sh  make_rom_tag.sh \
			convert_to_pc.sed MV \
			new_includes new_libs new_remote \
			genwhat.sh drop.sh cdrtt.s.master \
			clean_i_files.pl

include GNUmakefile.master
