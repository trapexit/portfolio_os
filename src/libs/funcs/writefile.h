/***************************************************************\
*								*
* Header file for writefile function                             *
*								*
* By:  Stephen H. Landrum					*
*								*
* Last update:  4-Jan-93					*
*								*
* Copyright (c) 1993, The 3DO Company, Inc.                     *
*								*
* This program is proprietary and confidential			*
*								*
\***************************************************************/


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int32 OpenMacLink (void);
int32 WriteFile (char *filename, char *buf, uint32 count);

#ifdef __cplusplus
}
#endif /* __cplusplus */

