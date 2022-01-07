/*
 *
 * Copyright (C) 1993 by
 * Digital Equipment Corporation, Maynard, Mass.
 *
 * This software is furnished under a license and may be used and copied
 * only  in  accordance  with  the  terms  of such  license and with the
 * inclusion of the above copyright notice. This software or  any  other
 * copies thereof may not be provided or otherwise made available to any
 * other person. No title to and ownership of  the  software  is  hereby
 * transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Digital Equipment
 * Corporation.
 *
 * Digital assumes no responsibility for the use or  reliability  of its
 * software on equipment which is not supplied by Digital.
 *
 *
 * Data Servers Business Unit - Software Engineering
 * 
 * Author: Pete Bixby 
 *
 * Initial revision: 5 Aug 1993
 *
 * Change history:
 *	13 sep 93, pcb - add function codes and change status codes
 *	17 sep 93, pcb - root handle, and file types
 *
 *
 */

#ifndef __H_BARFEXPORT
#define __H_BARFEXPORT

#ifdef EXTERNAL_RELEASE
/* cause an error if this file gets included somehow */
#include "THIS_FILE_IS_3DO-INTERNAL,_AND_IS_NOT_FOR_THE_USE_OF_DEVELOPERS"
#else /* EXTERNAL_RELEASE */

/* Define the root directory handle */

#define RF_ROOT_HANDLE 			0xFF
/* Define BARF function codes */

#define RF_START			0x0
#define RF_DIRECTORY_OPEN		0x1
#define RF_DIRECTORY_CLOSE		0x2
#define RF_DIRECTORY_READ		0x3
#define RF_FILE_OPEN			0x4
#define RF_FILE_CLOSE			0x5
#define RF_FILE_READ			0x6
#define RF_MULTI_READ			0x7
#define RF_GET_FILE_SIZE		0x8
#define RF_END				0xF

/* Define BARF status codes */
#define RF_SUCCESS 			0

#define RF_END_OF_DIRECTORY		1
#define RF_INVALID_PARENT_HANDLE     	2
#define RF_INVALID_DIRECTORY_HANDLE  	3
#define RF_INVALID_FILE_HANDLE		4
#define RF_DIRNAME_DOES_NOT_EXIST	5
#define RF_FILENAME_DOES_NOT_EXIST	6
#define RF_INVALID_BLOCK_NUMBER		7
#define RF_OPEN_LIMIT_EXCEEDED		8
#define RF_SERVER_OF_RESOURCES		9
#define RF_INVALID_DIRKEY		10
#define RF_FILE_READ_ERROR		11
#define RF_DIRECTORY_READ_ERROR		12
#define RF_FILE_OPEN_ERROR		13
#define RF_INVALID_TRANSACTION_ID	14
#define RF_INVALID_ACCESS		15
#define RF_INVALID_OPERATION		16
#define RF_END_OF_FILE			17

/* Define the BARF file types */

#define	RF_REGULAR			'F'
#define RF_DIRECTORY			'D'

#endif /* EXTERNAL_RELEASE */
#endif /* BARFEXPORT */
