######################################### Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.## This material contains confidential information that is the property of The 3DO Company.## Any unauthorized duplication, disclosure or use is prohibited.## $Id: bounce.make,v 1.8 1995/01/20 04:50:51 ceckhaus Exp $########################################   File:       bounce.make#   Target:     bounce#   Sources:    bounce.c bounce_sound.c#######################################	Symbol definitions#####################################App				= bounceDebugFlag		= 1ObjectDir		= :Objects:SourceDir		= :AppsDir			= :Apps_Data:ExamplesLibDir	= {3DOFolder}Examples:ExamplesLib:CC				= armccLINK			= armlinkWorkingDisk		=######################################	Default compiler options#####################################COptions			= -fa -zps0 -za1 -i "{ExamplesLibDir}"CDebugOptions		= -g -d DEBUG={DebugFlag}LOptions			= -aif -r -b 0x00LStackSize			= 4096LDebugOptions		= -dModbinDebugOptions	= -debug######################################	Object files#####################################OBJECTS			=	{ObjectDir}bounce.c.o �					{ObjectDir}bounce_sound.c.o �					"{3DOLibs}"cstartup.oLIBS			=	�					"{ExamplesLibDir}ExamplesLib.lib"  �					"{3DOLibs}Lib3DO.lib" �					"{3DOLibs}audio.lib" �					"{3DOLibs}music.lib" �					"{3DOLibs}operamath.lib" �					"{3DOLibs}filesystem.lib" �					"{3DOLibs}graphics.lib" �					"{3DOLibs}input.lib" �					"{3DOLibs}clib.lib"######################################	Default build rules#####################################All				�	{App}{ObjectDir}		�	:.c.o	�	.c	{CC} -i "{3DOIncludes}" {COptions} {CDebugOptions} -o {TargDir}{Default}.c.o {DepDir}{Default}.c######################################	Target build rules#####################################{App} � {App}.make {OBJECTS}	{LINK} {LOptions} {LDebugOptions} �		{OBJECTS} �		{LIBS} �		-o "{WorkingDisk}"{Targ}	SetFile "{WorkingDisk}"{Targ} -c 'EaDJ' -t 'PROJ'	modbin "{WorkingDisk}"{Targ} -stack {LStackSize} {ModbinDebugOptions}	stripaif "{WorkingDisk}"{Targ} -o {Targ} -s {Targ}.sym	if not `exists "{AppsDir}"`		newFolder "{AppsDir}"	end	move -y {Targ} "{AppsDir}"	move -y {Targ}.sym "{AppsDir}"######################################	Additional Target Dependencies#####################################{ObjectDir}bounce.c.o		�	{App}.make bounce.h bounce_sound.h{ObjectDir}bounce_sound.c.o	� 	{App}.make bounce_sound.h