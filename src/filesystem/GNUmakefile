# $Id: GNUmakefile,v 1.72 1994/12/02 05:19:21 jhw Exp $
#
# NOTE: When you add targets to be built, use ${setrev} to run modbin
# if you have a "target.rev" file in RCS. Be sure to "ci" your
# "target.rev" file before pushing the ${setrev} command back into RCS.
#
# $Log: GNUmakefile,v $
# Revision 1.72  1994/12/02  05:19:21  jhw
# Make fs-barf for network OS.  This is so cable builds don't clobber the
# Opera fs which does not include BARF.
#
# Revision 1.71  1994/10/18  23:27:03  shawn
# moved some utilities into LOCAL, useres shouldn't care
# for these.
#
# Revision 1.70  1994/10/11  19:12:51  limes
# eliminate unnecessary ${modbin} calls
#
# Revision 1.69  1994/10/10  23:13:14  limes
# set modulename in fs_MODNAME, and use fs_MODBIN for other args
#
# Revision 1.68  1994/10/06  17:14:49  vertex
# No longer copies example code, this is taken care of by the example
# build of these programs.
#
# Revision 1.67  1994/10/04  02:20:48  vertex
# Added type.c to the example source code.
# Added EXAMPLE_CLASS=filesystem, to get the example code copied to the
# right directory.
#
# Revision 1.66  1994/09/29  20:40:22  martin
# I give up.  Moved back to 1.61.
#
# Revision 1.61  1994/09/22  23:20:29  vertex
# allocate is now LOCALS= instead of PGMS=.
# alocate.c is now in EXAMPLE_SRCS=
#
# Revision 1.60  1994/09/22  16:11:34  vertex
# Added LOCALS to RCSFILES.
#
# Revision 1.59  1994/09/21  23:59:02  vertex
# No longer builds chknvram, launch, or hello.
# Dumpfile is now built locally, but not copied anywhere.
# These changes are per Dave's and Shawn's comments.
#
# Revision 1.58  1994/09/16  22:53:34  vertex
# No longer builds the run command. As Dave points out, it is "utterly
# obsolete"
#
# Revision 1.57  1994/09/13  17:17:15  vertex
# Added autodocs.c
#
# Revision 1.56  1994/08/01  22:48:29  martin
# added BARF option
#
# Revision 1.55  1994/07/13  18:19:21  vertex
# Re-added graphics.lib, since lmadm uses that
#
# Revision 1.53  1994/07/07  22:01:40  vertex
# Changed path to link libs
#
# Revision 1.52  1994/07/07  19:59:40  vertex
# Moved the building of filesystem.lib to a subdirectory
#
# Revision 1.51  1994/06/27  23:06:40  limes
# Add "unformat" utility, which just fills NVRAM with 0xFFFFFFFF
# mainly for testing of the automagic reformatting during boot.
#
# Revision 1.50  1994/06/21  23:04:20  limes
# "ls.c" is also example code ...
#
# Revision 1.49  1994/06/21  22:43:46  limes
# "Walker.c" is example code, included in our documentation, and
# the latest version should be exported onto the disk.
#
# Revision 1.48  1994/06/15  18:48:33  limes
# Get libtcl.a from /opera/archive: someone tucked a newer
# version in /usr/local/lib which unfortunately causes our
# build of "layout" to fail.
#
# Revision 1.47  1994/06/14  22:48:50  limes
# Add "fscheck" and "lmdf" to AUTODEPEND; sort the list.
#
# Revision 1.46  1994/06/14  22:09:15  limes
# Rules for "fscheck" and "lmdf" were set up to set the revisions from
# .rev files but no .rev files had been created. Added comments to this
# file warning that .rev files need to be created (also, created
# fscheck.rev and lmdf.rev) ... this has happened before, in this file.
#
# Revision 1.45  1994/06/10  20:49:29  dplatt
# Build both the ROM and bootable-volume filesystems with support
# for cache.  Actual decision as to presence of cache is now made
# in Sherry.
#
# Revision 1.44  1994/06/07  03:58:51  limes
# expand DEV/ROM into BOOT/DEV/ROM and make it work right
#
# Revision 1.43  1994/06/03  18:07:32  shawn
# Added fscheck.
#
# Revision 1.42  1994/05/25  22:38:24  vertex
# Try try again...
#
# Revision 1.41  1994/05/25  22:27:46  vertex
# Removed bogus first byte that got in there somehow
#
# Revision 1.40  1994/05/25  22:10:35  vertex
# Moved filefolioglue.h to LOCAL_INCS to it won't get exported
#
# Revision 1.39  1994/05/17  20:55:11  shawn
# Added link.c.
#
# Revision 1.38  1994/05/12  02:20:28  sdas
# modbin now used to also stamp the name FIleSystem for filesystem image
#
# Revision 1.37  1994/05/11  21:48:50  limes
# Need to install "UTIL_INCS" for "installtherest" as well as "therest".
# (Actually, we need to think about this ... what if .top points to,
# say, /opera/fz1jfc2?)
#
# Revision 1.36  1994/04/22  20:04:11  shawn
# Added lmdf.
#
# Revision 1.35  1994/03/29  06:10:42  local
# update dependencies
#
# Revision 1.34  1994/03/29  02:29:52  limes
# switch to separate autodepend files
#
# Revision 1.33  1994/03/28  19:27:59  dplatt
# Add ExecuteAsSubroutine.s to the source kit
#
#
# GNUmakefile for the Portfolio File System folio, library, and
# related exectuables.
#
# Copyright (c) 1992, 1993, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all

# ==========

.tree: .includes

# ==========

# These files are exported by this package, and
# are under RCS control in the includes directory.

LOCAL_INCS=

INCS_DOWN=		cdrom.h directory.h directoryfunctions.h \
			discdata.h filefunctions.h \
			filestream.h filestreamfunctions.h \
			filesystem.h filesystemdefs.h tcl.h \
			barf.h barfexport.h

# ==========

BDIRS = lib

# ==========

# GNUmakefile for the Opera filesystem

DOCS=			filesystem.doc

#
#
# **************
# Feature support definitions
#
# Uncomment this line (or override from command line)
# to get more debugging in the development version.
# DEBUGOPT=		-DDEBUG
#
# Use this definition during software development
CDDEFS=			-DAUTOMOUNT -DDOCACHE -DMACFS $(DEBUGOPT)
#
# Use this definition for bootable-CD release
CBDEFS=			-DAUTOMOUNT -DDOCACHE
#
# Use this definition for ROM release.  NB that cache is now included!
CRDEFS=			-DAUTOMOUNT -DDOCACHE

# And, everyone gets these options:
CDEFS=			-DARMC -DDODAEMON -DSCANMOUNT

NOWARN=			${NOTHING}
MAP=			-MAP

# ==========
# These rules are derived from rules in .top/GNUmakefile.master, and
# commit an abomination designed to keep cable builds (which require
# BARF) from conflict with opera builds (which require the absence of
# BARF) in the same build.

%.cbo	: %.c ;	$(CC) $(CFLAGSB) -DBARFFS -c $< -o $@
%.cdo	: %.c ;	$(CC) $(CFLAGSD) -DBARFFS -c $< -o $@
%.cro	: %.c ;	$(CC) $(CFLAGSR) -DBARFFS -c $< -o $@

%.cbo	: %.s ;	$(AS) $(SFLAGSB) $(SFLAGS_BARFFS)   $< -o $@
%.cdo	: %.s ;	$(AS) $(SFLAGSD) $(SFLAGS_BARFFS)   $< -o $@
%.cro	: %.s ;	$(AS) $(SFLAGSR) $(SFLAGS_BARFFS)   $< -o $@

.dep.%.cbo	: %.c ; $(CC) $(CFLAGSB) -DBARFFS -c $< -o $*.cbo -M > $@
.dep.%.cdo	: %.c ; $(CC) $(CFLAGSD) -DBARFFS -c $< -o $*.cdo -M > $@
.dep.%.cro	: %.c ; $(CC) $(CFLAGSR) -DBARFFS -c $< -o $*.cro -M > $@

.dep.%.cbo	: %.s ; $(AS) $(SFLAGSB) $(SFLAGS_BARFFS)    $< -o $*.cbo -D $@.raw >/dev/null 2>&1 && \
			awk '{ for (f=2; f<=NF; ++f) printf "%s %s\n", $$1, $$f; }' < $@.raw > $@ && \
			${RM} $@.raw

.dep.%.cdo	: %.s ; $(AS) $(SFLAGSD) $(SFLAGS_BARFFS)    $< -o $*.cdo  -D $@.raw >/dev/null 2>&1 && \
			awk '{ for (f=2; f<=NF; ++f) printf "%s %s\n", $$1, $$f; }' < $@.raw > $@ && \
			${RM} $@.raw

.dep.%.cro	: %.s ; $(AS) $(SFLAGSR) $(SFLAGS_BARFFS)    $< -o $*.cro -D $@.raw >/dev/null 2>&1 && \
			awk '{ for (f=2; f<=NF; ++f) printf "%s %s\n", $$1, $$f; }' < $@.raw > $@ && \
			${RM} $@.raw

# ==========

DATE=			BuildDate

LINK_DLIBS=		lib/filesystem.dlib
LINK_BLIBS=		lib/filesystem.blib
LINK_RLIBS=		lib/filesystem.rlib

LINK_LIBS=		kernel.lib graphics.lib input.lib clib.lib

# ==========

BPGMS=			fs.boot fs-barf.boot
DPGMS=			fs.dev fs-barf.dev
RPGMS=			fs.rom fs-barf.rom

fs_MODNAME=		FileSystem
fs_MODBIN=		-stack 1024 -pri 210

fs_SSRC=		FileDaemonStartup.s ExecuteSubroutine.s
fs_CSRC=		FileFolio.c FileDriver.c MountFileSystem.c \
			filesystemdefs.c OpenClose.c FileDaemon.c \
			FileFolioMain.c MacFileSystem.c MySysErr.c \
			LinkedMemFileSystem.c FileStream.c \
			DirectoryAccess.c LoadFile.c BarfFileSystem.c \
			autodocs.c

fs-barf_MODNAME=	FileSystem
fs-barf_MODBIN=		-stack 1024 -pri 210

fs.boot_OBJS=		${fs_SSRC:%.s=%.bo} ${fs_CSRC:%.c=%.bo}
fs.boot:		${fs.boot_OBJS}
	${linkboot}
	${compress}
	${setrev}

fs.dev_OBJS=		${fs_SSRC:%.s=%.do} ${fs_CSRC:%.c=%.do}
fs.dev:			${fs.dev_OBJS}
	${linkdev}
	${compress}
	${setrev}

fs.rom_OBJS=		${fs_SSRC:%.s=%.ro} ${fs_CSRC:%.c=%.ro}
fs.rom:			${fs.rom_OBJS}
	${linkrom}
	${compress}
	${setrev}

fs-barf.boot_OBJS=	${fs_SSRC:%.s=%.cbo} ${fs_CSRC:%.c=%.cbo}
fs-barf.boot:		${fs-barf.boot_OBJS}
	${linkboot}
	${compress}
	${setrev}

fs-barf.dev_OBJS=	${fs_SSRC:%.s=%.cdo} ${fs_CSRC:%.c=%.cdo}
fs-barf.dev:		${fs-barf.dev_OBJS}
	${linkdev}
	${compress}
	${setrev}

fs-barf.rom_OBJS=	${fs_SSRC:%.s=%.cro} ${fs_CSRC:%.c=%.cro}
fs-barf.rom:		${fs-barf.rom_OBJS}
	${linkrom}
	${compress}
	${setrev}

# ==========

PGMS=			ls Walker type bangon certify cdrom fscheck \
			lmadm lmdump delete copy dismount \
			format mount

LOCALS=			dumpfile allocate lmdf lmfs mkfile unformat

ls_CSRC=		ls.c
ls_OBJS=		${ls_CSRC:%.c=%.o}
ls_MODBIN=		-stack 2048
ls:			${ls_OBJS}
	${linkdev}
	${setrev}

Walker_CSRC=		Walker.c
Walker_OBJS=		${Walker_CSRC:%.c=%.o}
Walker_MODBIN=		-stack 2048
Walker:			${Walker_OBJS}
	${linkdev}

type_CSRC=		type.c
type_OBJS=		${type_CSRC:%.c=%.o}
type_MODBIN=		-stack 2048
type:			${type_OBJS}
	${linkdev}

bangon_CSRC=		bangon.c
bangon_OBJS=		${bangon_CSRC:%.c=%.o}
bangon_MODBIN=		-stack 2048
bangon:			${bangon_OBJS}
	${linkdev}

certify_CSRC=		certify.c
certify_OBJS=		${certify_CSRC:%.c=%.o}
certify_MODBIN=		-stack 2048
certify:		${certify_OBJS}
	${linkdev}

cdrom_CSRC=		cdrom.c
cdrom_OBJS=		${cdrom_CSRC:%.c=%.o}
cdrom_MODBIN=		-stack 2048
cdrom:			${cdrom_OBJS}
	${linkdev}

fscheck_CSRC=		fscheck.c
fscheck_OBJS=		${fscheck_CSRC:%.c=%.o}
fscheck_MODBIN=		-stack 2048
fscheck:		${fscheck_OBJS}
	${linkdev}
	${setrev}

lmadm_CSRC=		lmadm.c
lmadm_OBJS=		${lmadm_CSRC:%.c=%.o}
lmadm_MODBIN=		-stack 2048
lmadm:			${lmadm_OBJS}
	${linkdev}
	${setrev}
	${setpriv}

lmdf_CSRC=		lmdf.c
lmdf_OBJS=		${lmdf_CSRC:%.c=%.o}
lmdf_MODBIN=		-stack 2048
lmdf:			${lmdf_OBJS}
	${linkdev}
	${setrev}

link_CSRC=		link.c
link_OBJS=		${link_CSRC:%.c=%.o}
link_MODBIN=		-stack 2048
link:			${link_OBJS}
	${linkdev}
	${setrev}

lmdebug_CSRC=		lmdebug.c
lmdebug_OBJS=		${lmdebug_CSRC:%.c=%.o}
lmdebug_MODBIN=		-stack 2048
lmdebug:		${lmdebug_OBJS}
	${linkdev}

lmdump_CSRC=		lmdump.c
lmdump_OBJS=		${lmdump_CSRC:%.c=%.o}
lmdump_MODBIN=		-stack 2048
lmdump:			${lmdump_OBJS}
	${linkdev}
	${setrev}

lmfs_CSRC=		lmfs.c
lmfs_OBJS=		${lmfs_CSRC:%.c=%.o}
lmfs_MODBIN=		-stack 2048
lmfs:			${lmfs_OBJS}
	${linkdev}
	${setrev}
	${setpriv}

mkfile_CSRC=		mkfile.c
mkfile_OBJS=		${mkfile_CSRC:%.c=%.o}
mkfile_MODBIN=		-stack 2048
mkfile:			${mkfile_OBJS}
	${linkdev}

delete_CSRC=		delete.c
delete_OBJS=		${delete_CSRC:%.c=%.o}
delete_MODBIN=		-stack 2048
delete:			${delete_OBJS}
	${linkdev}

allocate_CSRC=		allocate.c
allocate_OBJS=		${allocate_CSRC:%.c=%.o}
allocate_MODBIN=	-stack 2048
allocate:		${allocate_OBJS}
	${linkdev}

copy_CSRC=		copy.c
copy_OBJS=		${copy_CSRC:%.c=%.o}
copy_MODBIN=		-stack 2048
copy:			${copy_OBJS}
	${linkdev}

dumpfile_CSRC=		dumpfile.c
dumpfile_OBJS=		${dumpfile_CSRC:%.c=%.o}
dumpfile_MODBIN=	-stack 2048
dumpfile:		${dumpfile_OBJS}
	${linkdev}

dismount_CSRC=		dismount.c
dismount_OBJS=		${dismount_CSRC:%.c=%.o}
dismount_MODBIN=	-stack 2048
dismount:		${dismount_OBJS}
	${linkdev}
	${setpriv}

format_CSRC=		format.c
format_OBJS=		${format_CSRC:%.c=%.o}
format_MODBIN=		-stack 2048
format:			${format_OBJS}
	${linkdev}
	${setpriv}

unformat_CSRC=		unformat.c
unformat_OBJS=		${unformat_CSRC:%.c=%.o}
unformat_MODBIN=	-stack 2048
unformat:		${unformat_OBJS}
	${linkdev}
	${setpriv}

mount_CSRC=		mount.c
mount_OBJS=		${mount_CSRC:%.c=%.o}
mount_MODBIN=		-stack 2048
mount:			${mount_OBJS}
	${linkdev}
	${setpriv}

TCLS=			layout.tcl cdrom.tcl ramdisk.tcl romdisk.tcl
UTILS=			layout ${TCLS}
UTIL_INCS=		discdata.h filesystem.h

libtcl.a:		/opera/archive/libtcl.a
	${copyit}
	ranlib libtcl.a

layout:			layout.c libtcl.a
	gcc layout.c -I/usr/include -Iincludes -g -DSUNDEBUG libtcl.a -o layout

RCSFILES=		${TCLS} \
			${PGMS:%=%.c} ${fs_SSRC} ${fs_CSRC} \
			${LOCALS:%=%.c} \
			${UTIL_INCS:%=includes/%} \
			layout.c ${DOCS}

AUTODEPEND= \
		${Walker_OBJS} \
		${allocate_OBJS} \
		${bangon_OBJS} \
		${cdrom_OBJS} \
		${certify_OBJS} \
		${copy_OBJS} \
		${delete_OBJS} \
		${dismount_OBJS} \
		${dumpfile_OBJS} \
		${format_OBJS} \
		${unformat_OBJS} \
		${fs.boot_OBJS} \
		${fs.dev_OBJS} \
		${fs.rom_OBJS} \
		${fs-barf.boot_OBJS} \
		${fs-barf.dev_OBJS} \
		${fs-barf.rom_OBJS} \
		${fscheck_OBJS} \
		${lmadm_OBJS} \
		${lmdf_OBJS} \
		${lmdump_OBJS} \
		${lmfs_OBJS} \
		${ls_OBJS} \
		${mkfile_OBJS} \
		${mount_OBJS} \
		${type_OBJS}

DEPFILE=	.autodepends
include GNUmakefile.master

ifdef CPU_ARM
SFLAGS_BARFFS= -PD "BARFFS SETA 1"
else
SFLAGS_BARFFS= -DBARFFS=1
endif
