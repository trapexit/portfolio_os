/* $Id: filefolio_lib.h,v 1.1 1994/07/07 18:57:28 vertex Exp $ */

#ifndef __FILEFOLIO_LIB_H
#define __FILEFOLIO_LIB_H


/*****************************************************************************/


#include "types.h"
#include "folio.h"
#include "filesystem.h"
#include "filestream.h"
#include "directory.h"
#include "filestreamfunctions.h"
#include "filefunctions.h"
#include "directoryfunctions.h"


/*****************************************************************************/


#define FolioGlue(name,func,params,args,type) \
type name params \
{ \
  int32 (* *_f)(); \
  _f =  (int32 (* *)())GetFileFolio(); \
  return (type) (*_f[func])args; \
}

#define FolioGlueVoid(name,func,params,args) \
void name params \
{ \
  int32 (* *_f)(); \
  _f =  (int32 (* *)())GetFileFolio(); \
  (*_f[func])args; \
}


/*****************************************************************************/


extern FileFolio *FileFolioBase;
extern Item       FileFolioNum;


FileFolio *GetFileFolio(void);


/*****************************************************************************/


#endif /* __FILEFOLIO_LIB_H */
