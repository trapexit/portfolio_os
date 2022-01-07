/* $Id: romify.c,v 1.5 1994/02/09 01:22:45 limes Exp $ */
#define __swi(foo) /* foo */

#include "/usr/include/stdio.h"
#include "/usr/include/stdlib.h"

#define	__TYPES_H

typedef long int int32;
typedef unsigned long int uint32;
typedef char int8;
typedef unsigned char uint8;

typedef int32 Item;

#include "podrom.h"

static int32 Checksum (uint8 *what, int32 howMany)
{
  int32 ck;
  fprintf(stderr, "Checksum %d bytes: ", howMany);
  ck = 0;
  while (--howMany >= 0) {
#ifdef NOTDEF
    if (*what != 0) { fprintf(stderr, "%x ", *what); }
#endif
    ck += *what++;
  }
  ck ^= PODROM_CHECKSUM_MAGIC_BITS;
  fprintf(stderr, "0x%x\n", ck);
  return ck;
}

int main (int argc, char **argv)
{
  void *buffer;
  PodROM *pr;
  int got, put;
  int buffersize;
  buffersize = 64*1026 + sizeof (PodROM);
  buffer = (void *) malloc(buffersize);
  if (!buffer) {
    fprintf(stderr, "Could not allocate munchbuffer\n");
    return 1;
  }
  memset(buffer, 0, buffersize);
  pr = (PodROM *) buffer;
  got = fread (&pr->pr_aif, 1, 64*1024, stdin);
  if (got < sizeof (struct AIFHeader)) {
    fprintf(stderr, "Only got %d bytes, invalid AIF!\n", got);
    return 1;
  }
  pr->pr_Header.prh_TotalByteCount = got + sizeof pr->pr_Header;
  pr->pr_Header.prh_ImageAreaChecksum = Checksum((uint8 *) &pr->pr_aif, got);
  pr->pr_Header.prh_HeaderChecksum =
    Checksum(sizeof pr->pr_Header.prh_HeaderChecksum + (uint8 *) pr,
	     sizeof (PodROM) - sizeof pr->pr_Header.prh_HeaderChecksum);
  put = fwrite(pr, 1, got + sizeof pr->pr_Header, stdout);
  return 0;
}
