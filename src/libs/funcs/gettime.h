/***************************************************************\
*								*
* Header file for gettime function                              *
*								*
* By:  Stephen H. Landrum					*
*								*
* Last update:  12-Mar-92					*
*								*
* Copyright (c) 1993, The 3DO Company                           *
*								*
* This program is proprietary and confidential			*
*								*
\***************************************************************/


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern Item timerior;
extern IOReq *timerioreq;

void opentimer (void);
void gettime (uint32 timebuffer[2]);

#ifdef __cplusplus
}
#endif /* __cplusplus */

