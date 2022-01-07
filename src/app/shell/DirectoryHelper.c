/*
** 930818 PLB Added flags to GetNextSaverDeviceName so we can filter out
**            READONLY volumes.
** 930819 PLB Prevent array overflow if too many files encountered.
**            Make flag checking more clear.
*/
#include "access.h"
#include "doaccess.h"
#include "FilesView.h"
#include "DirectoryHelper.h"

#include "filesystem.h"
#include "directory.h"
#include "directoryfunctions.h"

int32 GetFileNames(FILESVIEW * filesview)
{
  Directory *dir;
  DirectoryEntry de;
  int32 err;
//  char *filenames[FILESVIEW_MAX_NUM_FILES]; /* 930819 Just use array in struct to save stack. */
  int32 entry;
  int32 i;

  err = ERR_GENERIC;
  filesview->filenum = filesview->start_visible_fileindex = filesview->currentfileindex = 0;

  if (filesview->pathname[0] == '\0'){
	return ERR_NO_SAVER;
  }

  dir = OpenDirectoryPath(filesview->pathname);
  if (!dir) return err;

	entry = 0;
	while (entry < FILESVIEW_MAX_NUM_FILES)  /* 930819 Don't fill past array!!!! */
	{
		err = ReadDirectory(dir, &de);
		if (err < 0){
		  err = SUCCESS;  // expects err at the end of directory listing
		  break;
		}
		if(!(de.de_Flags & (FILE_IS_DIRECTORY|FILE_IS_FOR_FILESYSTEM)))
		{
			filesview->filenames[entry] = (char *) malloc( MAX_FILENAME_LEN + 1); /* 1 extra 930819 */
			if (!filesview->filenames[entry])
			{
				for(i=0; i<entry-1; i++) free(filesview->filenames[i]);  /* 930819 */
				err = ERR_SERIOUS_MEM_ERR;
				goto CLEANUP;
			}
			strncpy(filesview->filenames[entry], de.de_FileName, MAX_FILENAME_LEN);
// make sure string is null terminated
			filesview->filenames[entry][MAX_FILENAME_LEN-1]  = '\0';
			entry++;
		}
	}

	filesview->filenum = entry;

CLEANUP:
  CloseDirectory(dir);
  return err;
}

/* 930818 Added Flags */
int32 GetNextSaverDeviceName(char *fromsaver, char *nextsaver, uint32 FlagsYes, uint32 FlagsNo)
{

  Directory *dir;
  DirectoryEntry de;
  int32 err;
  char firstsaver[FILESYSTEM_MAX_NAME_LEN];
  int32 entry;
  Boolean ret_next_saver;

  err = ERR_GENERIC;

  firstsaver[0] = '\0';

  dir = OpenDirectoryPath("/");
  if (!dir) return err;

  DIAGNOSTIC("Opened root file system");

  ret_next_saver = (strcmp(fromsaver, "") == 0) ? TRUE : FALSE;

  entry = 0;
  while (1)
  {
    err = ReadDirectory(dir, &de);
	if (err < 0)  /* Could be end of directory. */
	{
	  if (ret_next_saver)
	    strcpy(nextsaver, "/");
	    strcat(nextsaver, firstsaver);
	  	break;
	}

	DIAGNOSTIC(de.de_FileName);

	// еее Dave Platt said no other flags are needed
	// еее remove flag checking to make this more demonstratable
// PLB, Put back flag checking so saver doesn't use READONLY volumes
// If all of the FlagsYes match AND none of the FlagsNo match...
    if (((de.de_Flags & FlagsYes) == FlagsYes) && ( (de.de_Flags & FlagsNo) == 0) )
    {
		entry++;

		DIAGNOSTIC("After Flags Check");

		if (ret_next_saver){
		  strcpy(nextsaver, "/");
		  strcat(nextsaver, de.de_FileName);
		  DIAGNOSTIC(nextsaver);
		  break;
		}

		if (entry == 1){
			//еее prepare for wrap around
			strcpy(firstsaver, de.de_FileName);
		}

		//еее skip the added "/"
		if (strcmp(&fromsaver[1], de.de_FileName) == 0)
			ret_next_saver = TRUE;
    }
    else
    {
		DIAGNOSTIC("GetNextSaverDeviceName: skipped filesystem");
    }

  }
  CloseDirectory(dir);
  DIAGNOSTIC("CloseDirectory");

  if ( (entry == 0) || (!ret_next_saver) ){
  	nextsaver[0] = '\0';
	return ERR_NO_SAVER;
  }

  return SUCCESS;
}
