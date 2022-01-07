/* $Id: realloc.c,v 1.1 1994/04/04 23:03:50 vertex Exp $ */
/* file: malloc.c */

#include "types.h"
#include "stdlib.h"
#include "string.h"

void *
realloc(void *oldBlock, size_t newSize)
{
void   *newBlock;
size_t  oldSize;

    oldSize = 0;  /* keep the silly compiler from complaining */
    if (oldBlock)
    {
        oldSize = *(uint32 *)((uint32)oldBlock - sizeof(uint32));
    }
    else if (newSize == 0)
    {
        return NULL;
    }

    newBlock = malloc(newSize);
    if (newBlock && oldBlock)
        memcpy(newBlock,oldBlock,newSize < oldSize ? newSize : oldSize);

    /* should this be freed when the new allocation failed? */
    free(oldBlock);

    return (newBlock);
}
