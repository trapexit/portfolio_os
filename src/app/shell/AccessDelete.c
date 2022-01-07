/* File : AccessDelete.c */
/*
** 930818 PLB Used AAFLAGS_WRITE_ABLE flag for Save and Delete volumes.
**            This is not fully implemented.
*/

#include "access.h"
#include "doaccess.h"
#include "AccessFileGet.h"
#include "MsgStrings.h"

int32
AccessDelete(aa_args, deletefilename)
AccessArgs	*aa_args;
char		*deletefilename;
{
	FILEGETTEXTS deletelabels;
	int32 i, ret;
	
	for (i = 0; i<MAX_FILE_GET_ITEMS; i++)
		{
		switch( i )
			{
			case FILE_GET_TITLE_ITEM:
				strcpy(deletelabels.labels[i], MSG_DELETE_TITLE);
				strcat(deletelabels.labels[i], " ");
				strcat(deletelabels.labels[i], aa_args->aa_Title);
				break;
			case FILE_GET_DIRECTORY_ITEM:
				strcpy(deletelabels.labels[i], "");
				break;
			case FILE_GET_INSTR_ITEM:
				strcpy(deletelabels.labels[i], MSG_LOAD_INSTR);
				break;
			case FILE_GET_RETURN_ITEM:
				strcpy(deletelabels.labels[i], MSG_LOAD_CANCEL);
				break;
			case FILE_GET_NEXT_ITEM:
				strcpy(deletelabels.labels[i], MSG_LOAD_NEXT_SAVER);
				break;
			case FILE_GET_PROCESS_ITEM:
				strcpy(deletelabels.labels[i], MSG_DELETE_PROCESS);
				break;
			}
		}

	aa_args->aa_Flags |= AAFLAGS_WRITE_ABLE;     /* Skip READONLY volumes. */
	ret = AccessFileGet(aa_args, &deletelabels);
	
	return ret;
}

