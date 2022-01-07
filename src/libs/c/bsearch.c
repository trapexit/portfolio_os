/* $Id: bsearch.c,v 1.3 1994/02/09 01:22:45 limes Exp $ */
#include "types.h"


void *bsearch(const void *key, const void *base,
              size_t nmemb, size_t size,
              int (*compar)(const void *, const void *))
{
/* Binary search for given key in a sorted array (starting at base).     */
/* Comparisons are performed using the given function, and the array has */
/* nmemb items in it, each of size bytes.                                */
    int midn, c;
    void *midp;
    for (;;)
    {   if (nmemb==0) return NULL;      /* not found at all.             */
        else if (nmemb==1)
/* If there is only one item left in the array it is the only one that   */
/* needs to be looked at.                                                */
        {   if (compar(key, base)==0) return (void *)base;
            else return NULL;
        }
        midn = (int)nmemb>>1;           /* index of middle item          */
/* I have to cast bast to (char *) here so that the addition will be     */
/* performed using natural machine arithmetic.                           */
        midp = (char *)base + midn * size;
        c = compar(key, midp);
        if (c==0) return midp;          /* item found (by accident)      */
        else if (c<0) nmemb = midn;     /* key is to left of midpoint    */
        else                            /* key is to the right           */
        {   base = (char *)midp + size; /* exclude the midpoint          */
            nmemb = nmemb - midn - 1;
        }
        continue;
    }
}
