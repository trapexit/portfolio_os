/* $Id: intl.c,v 1.4 1994/12/21 23:26:19 vertex Exp $ */

/**
|||	AUTODOC PUBLIC tpg/shell/intl
|||	intl - get/set the international settings of the machine.
|||
|||	  Format
|||
|||	    intl [-language <2 letter ISO-639 language code>]
|||	         [-country <ISO-3166 numeric-3 country code>]
|||
|||	  Description
|||
|||	    This command lets you display the current country and language
|||	    as reported by the international folio. The information
|||	    displayed corresponds to the country codes and language codes
|||	    listed in the intl.h include file.
|||
|||	    By using the -language option, you can adjust the machine's
|||	    language code. This is useful in order to test a title's
|||	    multilingual support. Changing the language will affect how
|||	    many of the international folio's routines behave.
|||
|||	    By using the -country option, you can adjust the machine's
|||	    country code. Changing the language code will affect the
|||	    information presented by the international folio in the Locale
|||	    structure.
|||
|||	  Arguments
|||
|||	    -language <language code>   Change the machine's language code.
|||	                                The language code is a two letter ASCII
|||	                                code, as listed in the ISO-639
|||	                                standard. Many of these codes appear
|||	                                in the intl.h include file. For
|||	                                example, the code for English is "en",
|||	                                while the one for Japanese is "ja".
|||
|||	                                If you ask for a language that the
|||	                                international folio doesn't support,
|||	                                defaults will be used instead.
|||
|||	    -country <country code>     Change the machine's country code.
|||	                                The language code is a three digit
|||	                                numeric code, as listed in the ISO-3166
|||	                                standard. Many of these codes appear
|||	                                in the intl.h include file.
|||
|||	  Implementation
|||
|||	    Command implemented in V24.
|||
|||	  Location
|||
|||	    $c/intl
**/

#include "types.h"
#include "item.h"
#include "mem.h"
#include "nodes.h"
#include "debug.h"
#include "list.h"
#include "device.h"
#include "driver.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "io.h"
#include "filesystem.h"
#include "filefunctions.h"
#include "filestream.h"
#include "filestreamfunctions.h"
#include "operror.h"
#include "string.h"
#include "stdio.h"
#include "countrydb.h"
#include "intl.h"


/*****************************************************************************/


#define Error(x,err) {printf(x); PrintfSysErr(err);}
#define FILENAME     "/NVRAM/3DO Settings"


/*****************************************************************************/


#define NOLANGUAGE ((LanguageCodes)-1)
#define NOCOUNTRY  ((CountryCodes)-1)

#define ERR_BADFORMAT 1
#define ERR_READERROR 2



/*****************************************************************************/


typedef struct IntlChunkHdr
{
    ChunkHdr Chunk;
    char     Language[4];
    uint32   Country;
} IntlChunkHdr;

typedef struct IntlFormHdr
{
    FormHdr      Form;
    IntlChunkHdr Chunk;
} IntlFormHdr;

#define ID_FORM 0x464f524d
#define ID_PREF 0x50524546
#define ID_INTL 0x494e544c

#define IFF_ROUND(x) ((x & 1) ? (x+1) : x)


/*****************************************************************************/


static Err SaveFile(char *name, void *data, uint32 dataSize)
{
Item      fileItem;
Item      fileIOItem;
IOInfo    fileInfo;
void     *ioBuffer;
Err       result;
uint32    ioBufferSize;
uint32    numBlocks;
uint32    blockSize;
OpenFile *file;

    /* get rid of the file */
    DeleteFile(name);

    result = CreateFile(name);
    if (result >= 0)
    {
        fileItem = OpenDiskFile(name);
        if (fileItem >= 0)
        {
            file         = (OpenFile *) LookupItem(fileItem);
            blockSize    = (int32) file->ofi_File->fi_BlockSize;
            numBlocks    = (dataSize + blockSize - 1) / blockSize;
            ioBufferSize = numBlocks * blockSize;

            ioBuffer = AllocMem(ioBufferSize, MEMTYPE_DMA | MEMTYPE_FILL);
            if (ioBuffer)
            {
                memcpy(ioBuffer,data,dataSize);

                fileIOItem = CreateIOReq(NULL, 0, fileItem, 0);
                if (fileIOItem >= 0)
                {
                    memset(&fileInfo, 0, sizeof(fileInfo));
                    fileInfo.ioi_Command         = FILECMD_ALLOCBLOCKS;
                    fileInfo.ioi_Recv.iob_Buffer = NULL;
                    fileInfo.ioi_Recv.iob_Len    = 0;
                    fileInfo.ioi_Offset          = numBlocks;
                    result = DoIO(fileIOItem, &fileInfo);

                    if (result >= 0)
                    {
                        fileInfo.ioi_Command         = CMD_WRITE;
                        fileInfo.ioi_Send.iob_Buffer = ioBuffer;
                        fileInfo.ioi_Send.iob_Len    = ioBufferSize;
                        fileInfo.ioi_Offset          = 0;
                        result = DoIO(fileIOItem, &fileInfo);

                        if (result >= 0)
                        {
                            fileInfo.ioi_Command         = FILECMD_SETEOF;
                            fileInfo.ioi_Send.iob_Buffer = NULL;
                            fileInfo.ioi_Send.iob_Len    = 0;
                            fileInfo.ioi_Offset          = dataSize;
                            result = DoIO(fileIOItem, &fileInfo);
                        }
                    }
                    DeleteIOReq(fileIOItem);
                }
                else
                {
                    result = fileIOItem;
                }
                FreeMem(ioBuffer, ioBufferSize);
            }
            else
            {
                result = NOMEM;
            }
            CloseDiskFile(fileItem);
        }
        else
        {
            result = fileItem;
        }

        /* don't leave a potentially corrupt file around... */
        if (result < 0)
            DeleteFile(name);
    }

    return (result);
}


/*****************************************************************************/


static Err SetLanguageCountry(LanguageCodes lang, CountryCodes country)
{
void         *buffer;
Err           result;
FormHdr      *form;
ChunkHdr     *chunk;
IntlChunkHdr *intl;
IntlFormHdr  *intlForm;
uint32        formData;
Stream       *stream;
uint32        fileSize;

    stream = OpenDiskStream(FILENAME,0);
    if (stream)
    {
        /* The prefs file already exists. Update its contents */

        fileSize = stream->st_FileLength;
        if (fileSize >= sizeof(FormHdr))
        {
            /* we allocate enough room so we can extend the data if we need to */
            buffer = malloc(fileSize + sizeof(IntlChunkHdr));
            if (buffer)
            {
                if (ReadDiskStream(stream,(char *)buffer,fileSize) == fileSize)
                {
                    CloseDiskStream(stream);
                    stream = NULL;

                    form = (FormHdr *)buffer;

                    if ((form->ID == ID_FORM) && (form->FormType == ID_PREF))
                    {
                        formData = form->Size;

                        chunk = (ChunkHdr *)((uint32)buffer + sizeof(FormHdr));
                        while (TRUE)
                        {
                            if (formData < sizeof(ChunkHdr))
                            {
                                /* chunk not found, add it... */
                                form->Size += sizeof(IntlChunkHdr);
                                intl = (IntlChunkHdr *)chunk;
                                intl->Chunk.ID    = ID_INTL;
                                intl->Chunk.Size  = 8;

                                if (lang == NOLANGUAGE)
                                    lang = INTL_LANG_ENGLISH;

                                if (country == NOCOUNTRY)
                                    country = INTL_CNTRY_UNITED_STATES;

                                intl->Language[0] = (char)(((uint32)lang >> 24) & 0xff);
                                intl->Language[1] = (char)(((uint32)lang >> 16) & 0xff);
                                intl->Language[2] = (char)(((uint32)lang >> 8) & 0xff);
                                intl->Language[3] = (char)(((uint32)lang >> 0) & 0xff);
                                intl->Country     = country;
                                result = SaveFile(FILENAME,form,form->Size + sizeof(FormHdr));
                                break;
                            }

                            if (chunk->Size > formData)
                            {
                                result = ERR_BADFORMAT;
                                break;
                            }

                            if (chunk->ID == ID_INTL)
                            {
                                intl = (IntlChunkHdr *)chunk;

                                if (lang != NOLANGUAGE)
                                {
                                    intl->Language[0] = (char)(((uint32)lang >> 24) & 0xff);
                                    intl->Language[1] = (char)(((uint32)lang >> 16) & 0xff);
                                    intl->Language[2] = (char)(((uint32)lang >> 8) & 0xff);
                                    intl->Language[3] = (char)(((uint32)lang >> 0) & 0xff);
                                }

                                if (country != NOCOUNTRY)
                                {
                                    intl->Country     = country;
                                }

                                result = SaveFile(FILENAME,form,form->Size + sizeof(FormHdr));
                                break;
                            }

                            formData -= chunk->Size + sizeof(ChunkHdr);
                            chunk     = (ChunkHdr *)((uint32)chunk + sizeof(ChunkHdr) + IFF_ROUND(chunk->Size));
                        }
                    }
                    else
                    {
                        /* doesn't have an IFF header */
                        result = ERR_BADFORMAT;
                    }
                }
                else
                {
                    /* couldn't read the file into memory */
                    result = ERR_READERROR;
                }
                free(buffer);
            }
            else
            {
                result = NOMEM;
            }
        }
        else
        {
            /* file not big enough for IFF header */
            result = ERR_BADFORMAT;
        }

        if (stream)
            CloseDiskStream(stream);

        return result;
    }

    /* The prefs file didn't previously exist, so create a whole new
     * one with just enough data in it for our needs.
     */

    intlForm = (IntlFormHdr *)malloc(sizeof(IntlFormHdr));
    if (intlForm)
    {
        if (lang == NOLANGUAGE)
            lang = INTL_LANG_ENGLISH;

        if (country == NOCOUNTRY)
            country = INTL_CNTRY_UNITED_STATES;

        intlForm->Form.ID           = ID_FORM;
        intlForm->Form.Size         = sizeof(IntlChunkHdr);
        intlForm->Form.FormType     = ID_PREF;
        intlForm->Chunk.Chunk.ID    = ID_INTL;
        intlForm->Chunk.Chunk.Size  = 8;
        intlForm->Chunk.Language[0] = (char)(((uint32)lang >> 24) & 0xff);
        intlForm->Chunk.Language[1] = (char)(((uint32)lang >> 16) & 0xff);
        intlForm->Chunk.Language[2] = (char)(((uint32)lang >> 8) & 0xff);
        intlForm->Chunk.Language[3] = (char)(((uint32)lang >> 0) & 0xff);
        intlForm->Chunk.Country     = country;
        result = SaveFile(FILENAME,intlForm,sizeof(IntlFormHdr));

        free(intlForm);
    }
    else
    {
        result = NOMEM;
    }

    return result;
}


/*****************************************************************************/


static uint32 ConvertNum(char *str)
{
    if (*str == '$')
    {
        str++;
        return strtoul(str,0,16);
    }

    return strtoul(str,0,0);
}


/*****************************************************************************/


int main(int argc, char **argv)
{
Err           err;
int           parm;
LanguageCodes language;
CountryCodes  country;
Item          locItem;
Locale       *loc;

    language = NOLANGUAGE;
    country  = NOCOUNTRY;

    for (parm = 1; parm < argc; parm++)
    {
        if ((strcmp("-help",argv[parm]) == 0)
         || (strcmp("-?",argv[parm]) == 0)
         || (strcmp("help",argv[parm]) == 0)
         || (strcmp("?",argv[parm]) == 0))
        {
            printf("intl: get/set the international settings of the machine\n");
            printf("  -language <2 letter ISO-639 language code>\n");
            printf("  -country <ISO-3166 numeric-3 country code>\n");
            return (0);
        }

        if (strcmp("-language",argv[parm]) == 0)
        {
            parm++;
            language = (LanguageCodes)(((uint32)argv[parm][0] << 24) | ((uint32)argv[parm][1] << 16));
        }

        if (strcmp("-country",argv[parm]) == 0)
        {
            parm++;
            country = (CountryCodes)ConvertNum(argv[parm]);
        }
    }

    if ((language == NOLANGUAGE) && (country == NOCOUNTRY))
    {
        err = intlOpenFolio();
        if (err >= 0)
        {
            locItem = intlOpenLocale(NULL);
            if (locItem >= 0)
            {
                loc = (Locale *)LookupItem(locItem);

                printf("The international folio reports: Language '%.2s', Country %d\n",&loc->loc_Language,loc->loc_Country);
                intlCloseLocale(locItem);
            }
            else
            {
                Error("Unable to open Locale item: ",locItem);
            }
            intlCloseFolio();
        }
        else
        {
            Error("Unable to open the international folio: ",err);
        }
    }
    else
    {
        err = SetLanguageCountry(language,country);
        if (err < 0)
        {
            Error("Unable to set language or country: ",err);
        }
        else if (err > 0)
        {
            printf("Unable to set language or country: ");
            switch (err)
            {
                case ERR_BADFORMAT: printf("bad file format\n"); break;
                case ERR_READERROR: printf("read error\n"); break;
            }
        }
    }

    return 0;
}
