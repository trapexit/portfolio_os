#ifndef __VARARGS_H
#define __VARARGS_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  $Id: varargs.h,v 1.7 1994/09/10 01:35:19 vertex Exp $
**
**  Standard C varargs definitions
**
******************************************************************************/


typedef char *va_list;
#define va_arg(list,mode) ((mode *)(list += sizeof(mode)))[-1]
/*#define va_start(list,va_list) list = (char *)&va_list*/
#define va_start(list,va_list) list = (char *)&(va_list) + sizeof(va_list)
#define va_end(list)


/*****************************************************************************/


#endif /* __VARARGS_H */
