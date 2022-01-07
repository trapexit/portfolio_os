/* File : AccessLoad.c */

#include "access.h"
#include "doaccess.h"
#include "AccessFileGet.h"
#include "MsgStrings.h"

int32
AccessLoad(aa_args)
AccessArgs	*aa_args;
{
  FILEGETTEXTS	loadlabels;
  int32			i,ret;
  
  for (i = 0; i<MAX_FILE_GET_ITEMS; i++){
  	switch(i){
	case FILE_GET_TITLE_ITEM:
		strcpy(loadlabels.labels[i], MSG_LOAD_TITLE);
		strcat(loadlabels.labels[i], " ");
		strcat(loadlabels.labels[i], aa_args->aa_Title);
		break;
	case FILE_GET_DIRECTORY_ITEM:
		strcpy(loadlabels.labels[i], "");
		break;
	case FILE_GET_INSTR_ITEM:
		strcpy(loadlabels.labels[i], MSG_LOAD_INSTR);
		break;
	case FILE_GET_RETURN_ITEM:
		strcpy(loadlabels.labels[i], MSG_LOAD_CANCEL);
		break;
	case FILE_GET_NEXT_ITEM:
		strcpy(loadlabels.labels[i], MSG_LOAD_NEXT_SAVER);
		break;
	case FILE_GET_PROCESS_ITEM:
		strcpy(loadlabels.labels[i], MSG_LOAD_PROCESS);
		break;
	}
  }

  ret = AccessFileGet(aa_args, &loadlabels);
  
  return ret;
}

