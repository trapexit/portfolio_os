# $Id: GNUmakefile,v 1.31 1994/10/20 00:22:43 stan Exp $
# GNUmakefile for other Portfolio executables
#
# Copyright (c) 1992, 1993, The 3DO Company, Inc.
# All rights reserved.
# This document is proprietary and confidential

default:		all

MAP=			${NOTHING}

# ==========

PGMS=			copy2mac eeprom gdbug iostress \
			items organus sysload what

items_MODBIN=		-stack 4096
items_OBJS=		items.o
items_LIBS=		clib.lib
items:			${items_OBJS}
	${linkdev}

gdbug_MODBIN=		-stack 512
gdbug_OBJS=		gdbug.o
gdbug_LIBS=		clib.lib
gdbug:			${gdbug_OBJS}
	${linkdev}
	${setpriv}

eeprom_OBJS=		eeprom.o
eeprom_LIBS=		filesystem.lib clib.lib
eeprom:			${eeprom_OBJS}
	${linkdev}

sysload_MODBIN=		-stack 2048
sysload_OBJS=		sysload.o
sysload_LIBS=		audio.lib graphics.lib clib.lib
sysload:		${sysload_OBJS}
	${linkdev}

what_MODBIN=		-stack 2048
what_OBJS=		what.o
what_LIBS=		kernel.lib clib.lib
what:			${what_OBJS}
	${linkdev}
	${setpriv}

iostress_MODBIN=	-stack 1024
iostress_CSRC=	 	iostress.c
iostress_OBJS=		${iostress_CSRC:%.c=%.o} ${iostress_SSRC:%.s=%.o}
iostress_LIBS=		kernel.lib clib.lib
iostress:		${iostress_OBJS}
	${linkdev}
	${setpriv}

TESTS=			bigmalloc memtest qtest readfile

bigmalloc_CSRC=		bigmalloc.c
bigmalloc_OBJS=		${bigmalloc_CSRC:%.c=%.o}
bigmalloc_LIBS=		clib.lib
bigmalloc_MODBIN=	-pri 100 -stack 4096
bigmalloc:		${bigmalloc_OBJS}
	${linkdev}

memtest_CSRC=		memtest.c
memtest_OBJS=		${memtest_CSRC:%.c=%.o}
memtest_LIBS=		clib.lib
memtest_MODBIN=	-pri 100 -stack 4096
memtest:		${memtest_OBJS}
	${linkdev}

readfile_CSRC=		readfile.c
readfile_OBJS=		${readfile_CSRC:%.c=%.o}
readfile_LIBS=		filesystem.lib clib.lib
readfile_MODBIN=	-pri 100 -stack 4096
readfile:		${readfile_OBJS}
	${linkdev}

qtest_CSRC=		qtest.c
qtest_OBJS=		${qtest_CSRC:%.c=%.o}
qtest_LIBS=		clib.lib
qtest_MODBIN=	-pri 100 -stack 4096
qtest:		${qtest_OBJS}
	${linkdev}

organus_OBJS=		organus.o
organus_LIBS=		filesystem.lib graphics.lib input.lib clib.lib
organus_MODBIN=		-stack 4096
organus:		${organus_OBJS}
	${linkdev}

copy2mac_CSRC=		copy2mac.c
copy2mac_OBJS=		copy2mac.o
copy2mac_LIBS=		filesystem.lib graphics.lib input.lib clib.lib
copy2mac_MODBIN=	-stack 4096
copy2mac:		${copy2mac_OBJS}
	${linkdev}

# ==========

# These were available with "make sorts" and "make everything"
# but some do not build just now.

SORTS=			sort sort1 sort2 sort3 sort4 sort5 sort6 sortmain

sort_OBJS=		sort.o
sort:			${sort_OBJS}
	${linkdev}

sort1_OBJS=		sort.o sort1.o
sort1:			${sort1_OBJS}
	${linkdev}

sort2_OBJS=		sort.o sort2.o
sort2:			${sort2_OBJS}
	${linkdev}

sort3_OBJS=		sort.o sort3.o
sort3:			${sort3_OBJS}
	${linkdev}

sort4_OBJS=		sort.o sort4.o
sort4:			${sort4_OBJS}
	${linkdev}

sort5_OBJS=		sort.o sort5.o
sort5:			${sort5_OBJS}
	${linkdev}

sort6_OBJS=		sort.o sort6.o
sort6:			${sort6_OBJS}
	${linkdev}

sortmain_OBJS=		sortmain.o
sortmain:		${sortmain_OBJS}
	${linkdev}

# ==========

#	These were defined but not included in any targets,
#	and many do not build just now.

DONTCOMPILE=		example playopera plays playv \
			playy shell showgif showopera signal sleep \
			slideshow time

example_OBJS=		example.o
example:		${example_OBJS}
	${linkdev}

playopera_OBJS=		playopera.o
playopera:		${playopera_OBJS}
	${linkdev}

plays_OBJS=		plays.o
plays:			${plays_OBJS}
	${linkdev}

playv_OBJS=		playv.o
playv:			${playv_OBJS}
	${linkdev}

playy_OBJS=		playy.o
playy:			${playy_OBJS}
	${linkdev}

showgif_OBJS=		showgif.o
showgif:		${showgif_OBJS}
	${linkdev}

showopera_OBJS=		showopera.o
showopera:		${showopera_OBJS}
	${linkdev}

signal_OBJS=		signal.o
signal:			${signal_OBJS}
	${linkdev}

sleep_OBJS=		sleep.o
sleep:			${sleep_OBJS}
	${linkdev}

slideshow_OBJS=		slideshow.o
slideshow:		${slideshow_OBJS}
	${linkdev}

time_OBJS=		time.o
time:			${time_OBJS}
	${linkdev}

shell_MODBIN=		-pri 100 -stack 2048 -MaxUsecs 10000
shell_OBJS=		shell.o
shell:			${shell_OBJS}
	${linkdev}
	${compress}

# ========================================================================

RCSFILES=		${PGMS:%=%.c} ${TESTS:%=%.c}

AUTODEPEND=		${PGMS:%=%.o} ${TESTS:%=%.o}

DEPFILE=	.autodepends
include GNUmakefile.master
