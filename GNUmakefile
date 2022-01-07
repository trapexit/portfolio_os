# $Id: GNUmakefile,v 1.283.1.20 1995/03/07 00:56:23 stan Exp $
# GNUmakefile for all of Portfolio
#
# Copyright (c) 1992, 1993, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential
#
# ------------------------------------------------------------------------
# The GNUMakefile FAQ has been moved to the Web.
#
# Unlike builds in individual components,
# standard builds from the top use "install"
# to install components in the build tree.
# This is so we use our own libraries.

default:		install

# BDIRS: directories that contain RCS links, have GNUmakefiles,
# and in which we will trigger targets.

BDIRS=			scripts \
			src/includes \
			src/libs/funcs \
			src/libs/c \
			src/libs/cpluslib \
			src/libs/kernellib \
			src/libs/lib3DO \
			src/libs/obsoletelib3do \
			src/libs/memdebug \
			src/libs/operamath \
			src/libs/swi \
			src/kernel \
			src/misc \
			src/audio \
			src/filesystem \
			src/graphics \
			src/jstring \
			src/international \
			src/debuggerfolio \
			src/compression \
			src/input \
			src/hwcd \
			src/app/oper \
			src/app/shell \
			src/app/other \
			src/drivers/nsio \
			src/drivers/fmv \
			src/dipir \
			src/examples \
			html

# The main tree is not orthogonal between the RCS sequence and
# the build tree, so we can not use the standard subdirectory
# tree build sequence.
NONSTANDARD_TREE=	true

# be sure to delete autodoc.list if you add an autodoc
# block to a file that did not previously have one.

autodoc.list:
	${MAKE} RCSCHECK_DISABLE=true -s -k -r doclist			| \
	 grep -v '^gmake\['						| \
	 grep -v '^--'							| \
	 sed "s;${HERE}/;;"						| \
	 sort > $@

autodoc.doc: autodoc.list
	adx `cat autodoc.list` > autodoc.doc

sources.list:
	${MAKE} RCSCHECK_DISABLE=true -s -k -r srclist			| \
	 grep -v '^gmake\['						| \
	 grep -v '^--'							| \
	 sed 's;'${HERE}'/;;'						| \
	 sort -u > $@

sources.wc: sources.list
	wc `cat $^ | grep -v /.autodepends` | sort -nr > $@

sources.locked: sources.list
	/usr/local/bin/rcslocks -d `/bin/pwd` `cat $^`			| \
	sed 's;'`/bin/pwd`/';;'						| \
	sort > $@

# ========================================================================

#
# The on-line documentation viewable with Mosaic or any other html viewer.
#
#  Right now this seems to live in Robert Laws home directory (yuck!)
#

on_line.doc:
	@echo "... Copying On-line HTML documentation directory ..."
	@echo "    ... checking for macstuff ..." ;
	@if	[ -d release ] ; \
	then	echo "       ... OK ..." ; \
	else	echo "ERROR:  please make  macstuff first" ; \
		exit 1; \
	fi
	@echo "    ... copying files ..." ;
	@cp -pr ${ARCHIVEDIR}/online02.15.95	release
	@echo "    ... Moving target directory ..." ;
	@${MV}	release/online02.15.95		release/OnLineDoc
	@echo "... Done on_line.doc ..."


# ========================================================================

cdboot/sherry: bootablecd/hardware/sherry
	${copyit}

cdboot/misc: runtime/hardware/misc
	${copyit}

cdboot/operator: runtime/hardware/operator
	${copyit}

cdboot/fs: bootablecd/programs/fs
	${copyit}

cdboot/cdrtt.s: scripts/cdrtt.s.master
	${copyit}

cdboot/make_rom_tag.sh: scripts/make_rom_tag.sh
	${copyit}

cdboot/cdboot: cdboot/sherry cdboot/operator cdboot/fs
	@cd cdboot; \
	if	/usr/local/armbin/makeboot.95.03.06	0x18000 sherry  \
	                                        0x28000 operator \
	                                        0x38000 fs \
			> cdboot 2> cdboot.log ; \
	then	echo "... makeboot OK ..."; \
	else	echo "### makeboot FAILED, see cdboot/cdboot.log for details ###"; \
		${RM} cdboot; exit 1; \
	fi

cdboot/misc_code: cdboot/misc
	@cd cdboot; \
	if	/usr/local/armbin/makeboot.95.03.06	0x14000 misc  \
			> misc_code 2> misc_code.log ; \
	then	echo "... makeboot (misc_code) OK ..."; \
	else	echo "### makeboot FAILED, see cdboot/misc_code.log for details ###"; \
		${RM} misc_code; exit 1; \
	fi

cdboot/rom_tags: cdboot/sherry cdboot/misc cdboot/operator \
		 cdboot/fs cdboot/cdrtt.s cdboot/BannerScreen \
		 cdboot/make_rom_tag.sh
	@echo "    ... cddipir ... (FROM LOCAL PARTS BIN)"; \
	cp -p bootablecd/hardware/cddipir cdboot/cddipir
	@cd cdboot; \
	if	make_rom_tag.sh	> rom_tag.log ; \
	then	echo "... ROM_TAGS OK ..."; \
	else	echo "### ROM_TAGS FAILED, see cdboot/rom_tag.log for details ###"; \
	fi

# ========================================================================

UNIXtoMAC=	/usr/local/bin/lfcr
MACtoUNIX=	/usr/local/bin/crlf
UNIFDEF=	unifdef

NST=		${shell uname -s}
FIXCOPYRSRC=	fix_copyright.c
FIXCOPYR=	./fix_copyright.${NST}

WHICHDIPIR=	nulldev

LIB3DO= release/examples/lib3do

# ========================================================================

FCCF=	-DRELEASENUMBERSTR='"'${RELEASENUMBER}'"' \
	-DRELEASENAMESTR='"'${RELEASE}'"' \
	-DRELEASEDATESTR='"'${RELEASEDATE}'"'

${FIXCOPYR}: ${FIXCOPYRSRC}
	gcc ${FCCF} $< -o $@

${FIXCOPYR}.debug: ${FIXCOPYRSRC}
	gcc ${FCCF} -g $< -o $@

fctest: ${FIXCOPYR}.debug ${FIXCOPYRSRC}
	rm -f $@.c
	cp ${FIXCOPYRSRC} $@.c
	echo where | dbx -r ./${FIXCOPYR}.debug -dv $@.c
	-diff -c ${FIXCOPYRSRC} $@.c
	find release -type f -print | xargs ${FIXCOPYR}.debug -v > fixcopyr.log 2>&1
	-@/bin/rm -f $@ $@.c ${FIXCOPYR}.debug

# ========================================================================

macstuff: cdboot/cdboot cdboot/misc_code cdboot/rom_tags ${FIXCOPYR}
	@echo "... Building Mac release directory ..."
	@if	[ -d release ]; \
	then	echo "    ... cleaning out old stuff ..." ; \
		${RM} -r		release ; \
	fi
	@${MD}				release
	@echo "    ... libs ..."
	@cp -pr developer/libs		release
	@echo "    ... examples ..."
	@cp -pr examples		release
	@echo "    ... remote ..."
	@cp -pr remote			release
	@echo "    ... testprograms ..."
	@cp -pr testprograms		release
	@echo "    ... utils ..."
	@cp -pr utils			release
	@${MD}				release/utils/takeme
	@echo "... Building MinimalTest subdirectory ..."
	@echo "    ... Supra directory ..."
	@${MD}		release/MinimalTest
	@cp -p developer/hardware/dipir.*	release/MinimalTest
	@cp -p developer/programs/fs		release/MinimalTest
	@cp -p developer/hardware/operator	release/MinimalTest
	@cp -p developer/hardware/sherry	release/MinimalTest
	@cp -p developer/hardware/misc		release/MinimalTest
	@echo "    ... Remote directory ..."
	@${MD}		release/MinimalTest/remote
	@cp -p src/graphics/example		release/MinimalTest/remote/example.ope
	@echo "    ... System directory ..."
	@${MD}		release/MinimalTest/remote/System
	@cp -pr remote/audio	release/MinimalTest/remote/System/Audio
	@echo "    ... empty directories ..."
	@${MD}		release/MinimalTest/remote/System/Daemons
	@echo "" > release/MinimalTest/remote/System/Daemons/junk
	@${MD}		release/MinimalTest/remote/System/Devices
	@${MD}		release/MinimalTest/remote/System/Devices/FMV
	@${MD}		release/MinimalTest/remote/System/Drivers
	@${MD}		release/MinimalTest/remote/System/Drivers/Languages
	@${MD}		release/MinimalTest/remote/System/Drivers/Tuners
	@echo "    ... Folio directory ..."
	@${MD}		release/MinimalTest/remote/System/Folios
	@${MD}		release/MinimalTest/remote/System/Folios/International
	@cp -p developer/hardware/*.privfolio		release/MinimalTest/remote/System/Folios
	@cp -p developer/hardware/*.folio		release/MinimalTest/remote/System/Folios
	@${MD}		release/MinimalTest/remote/System/Graphics
	@${MD}		release/MinimalTest/remote/System/Graphics/Fonts
	@echo "    ... Programs directory ..."
	@cp -pr developer/programs		release/MinimalTest/remote/System/Programs
	@${RM} release/MinimalTest/remote/System/Programs/eventbroker
	@${RM} release/MinimalTest/remote/System/Programs/shell
	@${RM} release/MinimalTest/remote/System/Programs/fs
	@${RM} release/MinimalTest/remote/System/Programs/CountryDatabase
	@${RM} release/MinimalTest/remote/System/Programs/*.UCode
	@${MD}		release/MinimalTest/remote/System/Scripts
	@cp -p scripts/startopera	release/MinimalTest/remote/System/Scripts
	@cp -p scripts/3DOBoot*scr	release/MinimalTest
	@cp -p scripts/AppStartup.debug	release/MinimalTest/remote/AppStartup
	@chmod 0666 release/MinimalTest/remote/AppStartup
	@echo "    ... Tasks directory ..."
	@${MD}		release/MinimalTest/remote/System/Tasks
	@cp -p developer/programs/eventbroker	release/MinimalTest/remote/System/Tasks
	@cp -p developer/programs/shell		release/MinimalTest/remote/System/Tasks
	@cp -p developer/programs/CountryDatabase release/MinimalTest/remote/System/Folios/International
	@cp -p developer/programs/*.UCode	 release/MinimalTest/remote/System/Devices/FMV
	@echo "... Converting Doc Files to MAC format ..."
	@cp -pr doc release
	@echo "... Copying Includes to release ..."
	@${MD}		release/includes
	@cp -pr includes/* release/includes
	@echo "... Removing non-shipping Includes and Libs ..."
	@${RM} -rf release/includes/RCS
	@${RM} -rf release/includes/GNUmakefile
	@${RM} -rf release/includes/GNUmakefile.master
	@${RM} -rf release/includes/.top
	@chmod 0777 release/includes/*.[hi]
	@${MD}		release/ShipNot
	@mv -f	release/includes/aif.h \
		release/includes/barf.h \
		release/includes/barfexport.h \
		release/includes/bootdata.h \
		release/includes/Charset.a \
		release/includes/clib.h \
		release/includes/clio.h \
		release/includes/controlpad.h \
		release/includes/controlport.h \
		release/includes/effectshandler.h \
		release/includes/filesystemdefs.h \
		release/includes/fmvdriver.h \
		release/includes/gettime.h \
		release/includes/getvideoinfo.h \
		release/includes/gfx.h \
		release/includes/interrupts.h \
		release/includes/interrupts.i \
		release/includes/inthard.h \
		release/includes/inthard.i \
		release/includes/intgraf.h \
		release/includes/intgraf.i \
		release/includes/hwcontroldriver.h \
		release/includes/ja.h \
		release/includes/kernelmacros.i \
		release/includes/listmacros.h \
		release/includes/loadfile.h \
		release/includes/loopstereosoundfile.h \
		release/includes/macros.a \
		release/includes/madam.h \
		release/includes/manuids.h \
		release/includes/mathfolio.h \
		release/includes/miscfunc.h \
		release/includes/mmu.h \
		release/includes/poddriver.h \
		release/includes/podrom.h \
		release/includes/rom.h \
		release/includes/rsa.h \
		release/includes/scanf.h \
		release/includes/scc8530.h \
		release/includes/sherryvers.h \
		release/includes/stack.i \
		release/includes/super.h \
		release/includes/sysinfo.h \
		release/includes/sysstate.h \
		release/includes/tcl.h \
		release/includes/timer.h \
		release/includes/timer.i \
		release/includes/usermodeservices.h \
		release/includes/varargs_glue.h \
		release/includes/vbl.h \
		release/includes/vdlutil.h \
		release/includes/writefile.h \
		release/libs/audiodemo.lib \
		release/libs/debugger.lib \
		release/libs/exampleslib.lib \
		release/libs/kernel.lib \
		release/ShipNot
	@echo "... Examples cleanup ..."
	@chmod a+w release/examples/*/*.[hcs]
	@chmod a+w release/examples/*/*/*.[hcs]
	@chmod a+w release/examples/*/*/*/*.[hcs]
	@${RM} -fr release/examples/C++
	@${RM} -fr release/examples/Kernel/UserFolio
	@echo "... Utilities ..."
	@cp -p cdboot/cdboot		release/utils
	@cp -p cdboot/misc_code		release/utils
	@cp -p cdboot/BannerScreen	release/utils
	@${RM} release/utils/layout
	@${RM} -f release/utils/filesystem.h
	@cp -p release/includes/filesystem.h release/utils
	@${RM} -f release/utils/discdata.h
	@cp -p release/includes/discdata.h release/utils
	@echo "... Creating Takeme for CD ..."
	@echo "... Copying Takeme for CD ..."
	@cp -pr release/utils/../MinimalTest/remote/System	release/utils/takeme/System
	@echo "... Copying AppStartup for CD ..."
	@cp -p scripts/AppStartup	release/utils/takeme/AppStartup
	@chmod 0666 release/utils/takeme/AppStartup
	@echo "... Creating Kernel directory for CD ..."
	@${MD}	release/utils/takeme/System/Kernel
	@echo "... Copying shell for CD ..."
	@cp -p release/utils/../../runtime/programs/shell	release/utils/takeme/System/Tasks
	@echo "... Copying folios for CD ..."
	@cp -p release/utils/../../runtime/hardware/audio.privfolio	release/utils/takeme/System/Folios
	@cp -p release/utils/../../runtime/hardware/graphics.privfolio	release/utils/takeme/System/Folios
	@cp -p release/utils/../../runtime/hardware/international.privfolio	release/utils/takeme/System/Folios
	@cp -p release/utils/../../runtime/hardware/operamath.privfolio	release/utils/takeme/System/Folios
	@echo "... Creating #rom_tags for CD ..."
	@echo "1 226" > release/utils/takeme/\#rom_tags
	@echo "... Creating #boot_code for CD ..."
	@echo  "2" > release/utils/takeme/System/Kernel/\#boot_code
	@echo "... Creating #os_code for CD ..."
	@echo "6" > release/utils/takeme/System/Kernel/\#os_code
	@echo "... Creating os_code for CD ..."
	@cp	/home/slab/stan/macstuff/SIGNATURES release/utils/takeme/signatures
	@mv release/utils/cdboot release/utils/takeme/System/Kernel/os_code
	@echo "... Creating #misc_code for CD ..."
	@echo "70" > release/utils/takeme/System/Kernel/\#misc_code
	@mv release/utils/misc_code release/utils/takeme/System/Kernel/misc_code
	@echo "... Creating boot_code for CD ..."
	@cp -p cdboot/boot_code release/utils/takeme/System/Kernel/boot_code
	@cp -p cdboot/rom_tags  release/utils/takeme/rom_tags
	@echo "... Creating #BannerScreen for CD ..."
	@echo "227" > release/utils/takeme/\#BannerScreen
	@mv release/utils/BannerScreen release/utils/takeme/BannerScreen
	@echo "... Creating other stuff for CD ..."
	@mv release/utils release/cdrommaster
	@mv release/cdrommaster/takeme/System/Programs/*.rom release/cdrommaster/takeme/System/Drivers
	@mv release/MinimalTest/remote/System/Programs/*.rom release/MinimalTest/remote/System/Drivers
	@mv release/cdrommaster/takeme/System/Programs/*.language release/cdrommaster/takeme/System/Drivers/Languages
	@mv release/MinimalTest/remote/System/Programs/*.language release/MinimalTest/remote/System/Drivers/Languages
	@mv release/cdrommaster/takeme/System/Programs/*.tuner release/cdrommaster/takeme/System/Drivers/Tuners
	@mv release/MinimalTest/remote/System/Programs/*.tuner release/MinimalTest/remote/System/Drivers/Tuners
	@mv release/cdrommaster/takeme/System/Programs/*.privdevice release/cdrommaster/takeme/System/Devices
	@mv release/MinimalTest/remote/System/Programs/*.privdevice release/MinimalTest/remote/System/Devices
	@echo "... Nuking non-distributed files from CD ..."
	@${RM} -f release/cdrommaster/takeme/System/Programs/audiomon
	@${RM} -f release/cdrommaster/takeme/System/Programs/bangon
	@${RM} -f release/cdrommaster/takeme/System/Programs/cdrom
	@${RM} -f release/cdrommaster/takeme/System/Programs/certify
	@${RM} -f release/cdrommaster/takeme/System/Programs/copy2mac
	@${RM} -f release/cdrommaster/takeme/System/Programs/cpdump
	@${RM} -f release/cdrommaster/takeme/System/Programs/dspfaders
	@${RM} -f release/cdrommaster/takeme/System/Programs/focus
	@${RM} -f release/cdrommaster/takeme/System/Programs/generic
	@${RM} -f release/cdrommaster/takeme/System/Programs/intl
	@${RM} -f release/cdrommaster/takeme/System/Programs/iostress
	@${RM} -f release/cdrommaster/takeme/System/Programs/items
	@${RM} -f release/cdrommaster/takeme/System/Programs/lookie
	@${RM} -f release/cdrommaster/takeme/System/Programs/luckie
	@${RM} -f release/cdrommaster/takeme/System/Programs/maus
	@${RM} -f release/cdrommaster/takeme/System/Programs/organus
	@${RM} -f release/cdrommaster/takeme/System/Programs/patchdemo
	@${RM} -f release/cdrommaster/takeme/System/Programs/playmf
	@${RM} -f release/cdrommaster/takeme/System/Programs/syncstress
	@${RM} -f release/cdrommaster/takeme/System/Programs/sysload
#
# fix for CR #3943
#
	@${RM} -f release/cdrommaster/takeme/System/Programs/InstallHWControlDriver
	@${RM} -f release/cdrommaster/takeme/System/Programs/InstallHWControlDevice
	@${RM} -f release/cdrommaster/takeme/System/Programs/ns.privdevice
	@${RM} -f release/cdrommaster/takeme/System/Programs/StorageDriver
	@${RM} -f release/cdrommaster/takeme/System/Programs/fs-barf
	@${RM} -f release/MinimalTest/dipir.cable
	@${RM} -f release/MinimalTest/dipir.creative
	@${RM} -f release/MinimalTest/dipir.lccd
	@${RM} -f release/MinimalTest/remote/System/Programs/InstallHWControlDriver
	@${RM} -f release/MinimalTest/remote/System/Programs/InstallHWControlDevice
	@${RM} -f release/MinimalTest/remote/System/Programs/ns.privdevice
	@${RM} -f release/MinimalTest/remote/System/Programs/StorageDriver
	@${RM} -f release/MinimalTest/remote/System/Programs/fs-barf
#
# Get Raw driverlets & anything else that needs encrypting,
#  then Remove test driverlets
#
	@${MD}	release/encrypt_me
	@cp -p src/input/*.raw			release/encrypt_me
	@cp -p src/hwcd/InstallHWControlDriver	release/encrypt_me
	@cp -p src/drivers/fmv/fmvvideodevice.rom	release/encrypt_me
	@cp -p src/drivers/nsio/ns.rom		release/encrypt_me
	@cp -p src/audio/audiofolio/audio.rom	release/encrypt_me/audio
	@cp -p src/international/folio/international.rom	release/encrypt_me/international
	@cp -p src/dipir/creative/enc/cddipir	release/encrypt_me
	@cp -p src/dipir/creative/enc/cddipir_dd release/encrypt_me
	@cp -p src/filesystem/dismount		release/encrypt_me
	@cp -p src/filesystem/format		release/encrypt_me
	@cp -p src/filesystem/fs.rom		release/encrypt_me
	@cp -p src/app/other/gdbug		release/encrypt_me
	@cp -p src/app/other/iostress		release/encrypt_me
	@cp -p src/graphics/graphics.rom	release/encrypt_me
	@cp -p src/libs/operamath/operamath.rom	release/encrypt_me
	@cp -p src/filesystem/lmadm		release/encrypt_me
	@cp -p release/cdrommaster/takeme/System/Kernel/misc_code release/encrypt_me
	@cp -p src/filesystem/mount		release/encrypt_me
	@cp -p src/app/oper/operator.rom	release/encrypt_me
	@cp -p release/cdrommaster/takeme/System/Kernel/os_code release/encrypt_me
	@cp -p src/kernel/sherry.rom		release/encrypt_me
	@cp -p src/kernel/sherry.boot		release/encrypt_me
	@cp -p cdboot/BannerScreen		release/encrypt_me
	@${RM} -f release/MinimalTest/remote/System/Drivers/blink.rom
	@${RM} -f release/MinimalTest/remote/System/Drivers/hello.rom
	@${RM} -f release/cdrommaster/takeme/System/Drivers/blink.rom
	@${RM} -f release/cdrommaster/takeme/System/Drivers/hello.rom
	@${RM} -f release/cdrommaster/takeme/System/Programs/unformat
#
# More fixes to cdrommaster
#
	@cp -p src/drivers/fmv/fmvvideodevice.rom	release/cdrommaster/takeme/System/Devices/fmvvideodevice.privdevice
	@cp -p src/drivers/nsio/ns.rom	release/cdrommaster/takeme/System/Devices/ns.privdevice
#
# Copy out the Font file(s) & other archive files
#
	@touch release/cdrommaster/MakeBannerScreen
	@cp -p ${ARCHIVEDIR}/MakeBannerScreen.95.01.23 release/cdrommaster/.resource/MakeBannerScreen
	@cp -p ${ARCHIVEDIR}/BogusTitle.imag.95.01.16 release/cdrommaster/BogusTitle.imag
	@cp -p ${ARCHIVEDIR}/HypotheticalTitle.imag.95.01.16 release/cdrommaster/HypotheticalTitle.imag
	@cp -p ${ARCHIVEDIR}/codec.lib.94.11.18 release/libs/codec.lib
	@cp -p ${ARCHIVEDIR}/Kanji16.4.94.01.04 release/MinimalTest/remote/System/Graphics/Fonts/Kanji16.4
	@cp -p ${ARCHIVEDIR}/Kanji16.4.94.01.04 release/cdrommaster/takeme/System/Graphics/Fonts/Kanji16.4
	@chmod 0666 release/MinimalTest/remote/System/Graphics/Fonts/Kanji16.4
	@chmod 0666 release/cdrommaster/takeme/System/Graphics/Fonts/Kanji16.4
	@if [ -d src/debuggerfolio ]; \
	 then	echo "   ... debuggerfolio ... (FROM LOCAL BUILD TREE)"; \
		cp -p src/debuggerfolio/folio/debugger.dev release/MinimalTest/remote/System/Folios/debugger.privfolio; \
	 else	echo "   ... debuggerfolio ... (FROM ARCHIVE)"; \
		cp -p ${ARCHIVEDIR}/debuggerfolio.94.06.27 release/MinimalTest/remote/System/Folios/debuggerfolio; \
	 fi
#
#  Remove for Babs
#
	@${MV} release/MinimalTest/remote/System/Programs/iostress release/ShipNot
#
# Remove any private information from the header files
#
	@echo "... Removing private sections of include files ..."
	@for h in release/includes/*.h ; \
	do\
		${RM} $$h.tmp;\
		${MV} $$h $$h.tmp;\
		${UNIFDEF} -DEXTERNAL_RELEASE $$h.tmp >$$h;\
		${RM} $$h.tmp;\
	done
	@.top/scripts/clean_i_files.pl release/includes/*.i
#
# !!! should do a grep here to verify that there are no
#     EXTERNAL_RELEASE references remaining in the includes.
#
# And last, we need to insert copyright messages into all text files
# with $Id strings, and convert all our text file from UNIX format to
# MAC format (turn LF into CR).
#
	@echo "... Copyright message substitutions ..."
	@find release -type f -print | xargs ${FIXCOPYR} -v > fixcopyr.log 2>&1
	@echo "... UNIX to MAC conversions ..."
	@find release -type f -print | xargs ${UNIXtoMAC} > unix2mac.log
	@echo "... Done ..."

# ========================================================================

pcstuff:
	@echo "... Building actual PC release subdirectory ..."
	@if	[ -d release.pc ]; \
	then	echo "    ... cleaning out old stuff ..." ; \
		${RM} -r		release.pc ; \
	fi
	@${MD}				release.pc
	@echo "    ... copying [MAC] includes & libs  ..."
#	@if	[ -d release/includes]; \
#	then	echo "   ... includes ..." ; \
#	else	echo "   ERROR   -   Must make macstuff first" ; \
#		exit 1 ; \
#	fi
#	@if	[\! -d release/libs]; \
#	then	echo "   ... libs ..." ; \
#	else	echo "   ERROR   -   No libs directory???" ; \
#		exit 1 ; \
#	fi
	@(cd release ; tar -cf - includes libs) | (cd release.pc ; tar -xf -)
	@echo "    ... Converting to UNIX format ..."
	@chmod +w release.pc/includes/*
	@(cd release.pc/includes ; ${MACtoUNIX} * > /dev/null)
	@echo "    ... Converting to 8.3 filenames ..."
	@for d in release.pc/includes/* ; \
	do	sed -f scripts/convert_to_pc.sed $$d > /tmp/foo ; \
		mv /tmp/foo $$d ; \
	done
	@echo "    ... Renaming include files ..."
	@sed s/\^/MV\ / scripts/new_includes > /tmp/foo
	@chmod +x /tmp/foo
	@cp scripts/MV release.pc/includes
	@(cd release.pc/includes ; /tmp/foo > /dev/null)
	@rm -f release.pc/includes/MV
	@echo "    ... Renaming library files ..."
	@sed s/\^/MV\ / scripts/new_libs > /tmp/foo
	@chmod +x /tmp/foo
	@cp scripts/MV release.pc/libs
	@(cd release.pc/libs ; /tmp/foo > /dev/null)
	@rm -f release.pc/libs/MV
	@echo "    ... Copying MinimalTest ..."
	@(cd release ; tar -cf - MinimalTest) | (cd release.pc ; tar -xf -)
	@echo "    ... Converting MinimalTest to 8.3 filenames ..."
	@(cd release.pc ; ../scripts/new_remote)
	@echo "... Done ..."

# ========================================================================

ROMCOREPATH=	/opera/romcore/bin

romcore: .FORCE
	@${MD} romcore
ifdef MARK_REL
	cd romcore; PATH=${ROMCOREPATH}:$$PATH getromcore -l
endif
ifndef MARK_REL
ifdef BUILD_REL
	cd romcore; PATH=${ROMCOREPATH}:$$PATH getromcore -r ${BUILD_REL}
endif
	cd romcore; PATH=${ROMCOREPATH}:$$PATH getromcore -l
endif
	cd romcore; PATH=${ROMCOREPATH}:$$PATH cook unenc.setup
ifdef BUILD_REL
	cd romcore; PATH=${ROMCOREPATH}:$$PATH cook rom.recipe -DANVIL -DLCCD -DROM_RELEASE=${BUILD_REL}
else
	cd romcore; PATH=${ROMCOREPATH}:$$PATH cook rom.recipe -DANVIL -DLCCD -DROM_RELEASE=$USER
endif

# ========================================================================

testrom:	.FORCE
	@${MD} testrom
	cd testrom ; ln -s /opera/RCS/romcore	RCS
	cd testrom ; ${CO} unenc.setup
	cd testrom ; ${CO} core.defines
	cd testrom ; ${CO} rom.recipe
	cd testrom ; ${CO} takeme.blueprint
ifdef BUILD_REL
	cd testrom ; PATH=${ROMCOREPATH}:$$PATH cook unenc.setup -DOS_BUILD=/opera/${BUILD_REL}
	cd testrom ; PATH=${ROMCOREPATH}:$$PATH cook rom.recipe -DANVIL -DLCCD -DROM_RELEASE=${BUILD_REL} -DOS_BUILD=/opera/${BUILD_REL}
else
	cd testrom ; PATH=${ROMCOREPATH}:$$PATH cook unenc.setup -DOS_BUILD=${PWD}
	cd testrom ; PATH=${ROMCOREPATH}:$$PATH cook rom.recipe -DANVIL -DLCCD -DROM_RELEASE=${USER} -DOS_BUILD=${PWD}
endif


# ========================================================================

RCSFILES=	symhead symtail BuildDate.c builddate.c ${FIXCOPYRSRC}

# ========================================================================

include GNUmakefile.master

# ========================================================================
#	This has to be after we include GNUmakefile.master,
#	so ${ARCHIVEDIR} gets set up.

cdboot/BannerScreen: ${ARCHIVEDIR}/BannerScreen.95.01.16
	${copyit}

# ========================================================================
# .tree.here		construct the build/install tree.
# must be *after* GNUmakefile.master, so we can see ${MARK_REL}.

.tree.here:
	@echo '--- building .tree in '`pwd`' ---'
	@${MD}		\
			bootablecd \
			bootablecd/hardware \
			bootablecd/libs \
			bootablecd/programs \
			cdboot \
			developer \
			developer/hardware \
			developer/libs \
			developer/programs \
			doc \
			examples \
			examples/Lib3DO \
			examples/Lib3DO/AnimUtils \
			examples/Lib3DO/CelUtils \
			examples/Lib3DO/DisplayUtils \
			examples/Lib3DO/IOUtils \
			examples/Lib3DO/MiscUtils \
			examples/Lib3DO/TextLib \
			examples/Lib3DO/TimerUtils \
			includes \
			remote \
			runtime \
			runtime/hardware \
			runtime/libs \
			runtime/programs \
			testprograms \
			utils
	@${CP} -r /opera/archive/examples.V24/* examples
	@${RM}						.top
	@${LN} .					.top
	@for d in src src/app src/libs src/drivers ${BDIRS} ; \
	do	${MD}					$$d ; \
		${RM}					$$d/.top ; \
		${LN} ../.top				$$d/.top ; \
		${RM}					$$d/GNUmakefile.master ; \
		${LN} .top/GNUmakefile.master		$$d/GNUmakefile.master ; \
	done
	@${RM}						scripts/RCS
	@${LN} .top/RCS/misc/scripts			scripts/RCS
	@${RM}						src/app/oper/RCS
	@${LN} .top/RCS/src/kernel/app/oper		src/app/oper/RCS
	@${RM}						src/app/other/RCS
	@${LN} .top/RCS/src/kernel/app/other		src/app/other/RCS
	@${RM}						src/app/shell/RCS
	@${LN} .top/RCS/src/kernel/app/shell		src/app/shell/RCS
	@${RM}						src/audio/RCS
	@${LN} .top/RCS/src/audio			src/audio/RCS
	@${RM}						src/dipir/RCS
	@${LN} .top/RCS/src/kernel/dipir		src/dipir/RCS
	@${RM}						src/drivers/nsio/RCS
	@${LN} .top/RCS/src/drivers/nsio		src/drivers/nsio/RCS
	@${RM}						src/drivers/fmv/RCS
	@${LN} .top/RCS/src/drivers/fmv			src/drivers/fmv/RCS
	@${RM}						src/filesystem/RCS
	@${LN} .top/RCS/src/filesystem			src/filesystem/RCS
	@${RM}						src/graphics/RCS
	@${LN} .top/RCS/src/graphics			src/graphics/RCS
	@${RM}						src/jstring/RCS
	@${LN} .top/RCS/src/jstring			src/jstring/RCS
	@${RM}						src/international/RCS
	@${LN} .top/RCS/src/international		src/international/RCS
	@${RM}						src/debuggerfolio/RCS
	@${LN} .top/RCS/src/debuggerfolio		src/debuggerfolio/RCS
	@${RM}						src/compression/RCS
	@${LN} .top/RCS/src/compression			src/compression/RCS
	@${RM}						src/includes/RCS
	@${LN} .top/RCS/includes			src/includes/RCS
	@${RM}						src/input/RCS
	@${LN} .top/RCS/src/input			src/input/RCS
	@${RM}						src/kernel/RCS
	@${LN} .top/RCS/src/kernel/kernel		src/kernel/RCS
	@${RM}						src/libs/c/RCS
	@${LN} .top/RCS/src/kernel/lib			src/libs/c/RCS
	@${RM}						src/libs/funcs/RCS
	@${LN} .top/RCS/src/funcs			src/libs/funcs/RCS
	@${RM}						src/libs/kernellib/RCS
	@${LN} .top/RCS/src/kernel/kernellib		src/libs/kernellib/RCS
	@${RM}						src/libs/lib3DO/RCS
	@${LN} .top/RCS/src/lib3DO			src/libs/lib3DO/RCS
	@${RM}						src/libs/obsoletelib3do/RCS
	@${LN} .top/RCS/src/libs/obsoletelib3do		src/libs/obsoletelib3do/RCS
	@${RM}						src/libs/memdebug/RCS
	@${LN} .top/RCS/src/libs/memdebug		src/libs/memdebug/RCS
	@${RM}						src/libs/cpluslib/RCS
	@${LN} .top/RCS/src/libs/cpluslib		src/libs/cpluslib/RCS
	@${RM}						src/libs/operamath/RCS
	@${LN} .top/RCS/src/operamath			src/libs/operamath/RCS
	@${RM}						src/libs/swi/RCS
	@${LN} .top/RCS/src/kernel/swi			src/libs/swi/RCS
	@${RM}						src/misc/RCS
	@${LN} .top/RCS/src/kernel/misc			src/misc/RCS
	@${RM}						src/hwcd/RCS
	@${LN} .top/RCS/src/hwcd			src/hwcd/RCS
	@${RM}						src/examples/RCS
	@${LN} .top/RCS/src/examples			src/examples/RCS
	@${RM}						html/RCS
	@${LN} .top/RCS/misc/html			html/RCS
ifneq (0, ${words ${MARK_REL}})
	@echo marking makefiles with ${MARK_REL}
	-@${SETMARK}		GNUmakefile GNUmakefile.master
	-@${SETMARK}		${BDIRS:%=%/GNUmakefile}
	-@${SETMARK}		${BDIRS:%=%/.autodepends}		>/dev/null 2>&1
endif
	-@${CO}			${BDIRS:%=%/GNUmakefile}
	-@${CO}			${BDIRS:%=%/.autodepends}		>/dev/null 2>&1
	@touch $@

opera.sym: symhead symtail .FORCE
	operasym src | cat symhead - symtail > opera.sym

.FORCE:
