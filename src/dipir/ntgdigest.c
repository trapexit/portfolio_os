/*	$Id: ntgdigest.c,v 1.2 1994/03/09 21:30:22 deborah Exp $
**
**	code for RSA and MD5 checking
**
**	3DO Confidential -- Contains 3DO Trade Secrets -- internal use only
*/
#include "types.h"
#include "rsa.h"
#include "global.h"
#include "bsafe2.h"
#include "aif.h"
#include "dipir.h"

#define	X(x)	;	/* Disabled debugging */

typedef unsigned int uint;
#define PROTO(x)	x

/*
 * We have two contexts to take care of. Dipir can happen in the
 * middle of the kernels rsa check, so they must be kept separate.
 */
#define DIPIR_CONTEXT	0

extern unsigned int DIGEST[4];

/* helper routines */

extern MD5_CTX	digest_context;

/* used by dipir routines */
void
InitDigest(MD5_CTX *d)
{
   if (d == 0)	d = &digest_context;
   MD5Init(d);
}

void
UpdateDigest(MD5_CTX *d, unsigned char *input, int inputLen)
{
#ifdef undef
    puts("UD(");puthex((int)input);puts(",");puthex(inputLen);puts(")");
#endif
    if (d == 0) d = &digest_context;
    MD5Update(d,input,inputLen);
}

void
FinalDigest(MD5_CTX *d, unsigned char *result)
{
    if (d == 0)
    {
	d = &digest_context;
    	result = (unsigned char *)DIGEST;
    }
    MD5Final(result,d);
}

/* DIPIR ONLY ROUTINES */
void DipirInitDigest(void)
{
    InitDigest(DIPIR_CONTEXT);
}
void DipirUpdateDigest(unsigned char *input, int len)
{
    UpdateDigest(DIPIR_CONTEXT, input, len);
}
void DipirFinalDigest(void)
{
    FinalDigest(DIPIR_CONTEXT, 0);
}
