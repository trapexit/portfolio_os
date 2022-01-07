/* File : DirectoryHelper.h */

#ifndef __DIRECTORYHELPER_H
#define	__DIRECTORYHELPER_H

#include "FilesView.h"

int32 GetFileNames(FILESVIEW *filename);
int32 GetNextSaverDeviceName(char *fromsaver, char *nextsaver, uint32 FlagsYes, uint32 FlagsNo);

#endif
