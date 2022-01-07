/***************************************************************\
*								*
* Header file for loadfile function                             *
*								*
* By:  Stephen H. Landrum					*
*								*
* Last update:  29-Dec-92					*
*								*
* Copyright (c) 1992, The 3DO Company, Inc.                     *
*								*
* This program is proprietary and confidential			*
*								*
\***************************************************************/


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int32 openmacdevice (void);
int32 getfilesize (char *name);
void *loadfile (char *name, void *buffer, uint32 buffersize, uint32 memtype);

#ifdef __cplusplus
}
#endif /* __cplusplus */
