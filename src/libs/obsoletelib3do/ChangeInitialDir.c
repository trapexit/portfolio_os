
/******************************************************************************
**
**  $Id: ChangeInitialDir.c,v 1.3 1994/11/01 03:49:01 vertex Exp $
**
**  Lib3DO routine to change default directory.
**
**  Prior to OS v1.0 this routine was required.  Nowadays there probably
**  isn't much need for it.
**
**  ChangeInitialDirectory adjusts your working directory if your current
**  working directory is null or if 'always' is true.  Changing your initial
**  directory is useful if your are debugging your application using the
**  debug command and you have set the data and source directory from debugger.
**
******************************************************************************/

#include "init3do.h"
#include "filefunctions.h"
#include "filesystem.h"

Err ChangeInitialDirectory( char *firstChoice, char *secondChoice, Boolean always )
{
	char DirectoryName[FILESYSTEM_MAX_PATH_LEN];
	Item targetDir;

	GetDirectory( DirectoryName, 255 );

	if ( ( DirectoryName[0] == 0 ) || always ) {
		if ( ( targetDir = ChangeDirectory( firstChoice ? firstChoice : "$boot" ) ) < 0 )
			if ( ( targetDir = ChangeDirectory( secondChoice ? secondChoice : "$boot" ) ) < 0 )
				return targetDir;
		}
	return 0;
}
