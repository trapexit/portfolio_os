/* $Id: externalcode.c,v 1.9 1994/12/16 17:54:12 vertex Exp $ */

#include "types.h"
#include "string.h"
#include "filefunctions.h"
#include "stdio.h"
#include "kernel.h"
#include "locales.h"
#include "langdrivers.h"
#include "englishdriver.h"
#include "international_folio.h"
#include "externalcode.h"


/*****************************************************************************/


/* This function returns the function pointer stored at the given offset
 * in "driverInfo". If this structure is not large enough to hold this
 * pointer, or if the pointer is NULL, the English version of the function
 * is returned instead
 */

static void *ReturnFunc(LanguageDriverInfo *driverInfo,
                        LanguageDriverInfo *englishDriverInfo,
                        uint32 funcOffset)
{
void **result;

    result = NULL;
    if (driverInfo->ldi_StructSize >= funcOffset + sizeof(void *))
        result = (void **)((uint32)driverInfo + funcOffset);

    if (!result || !*result)
        result = (void **)((uint32)englishDriverInfo + funcOffset);

    return (*result);
}



/*****************************************************************************/


/* Load in an external language driver, or use the built-in English one
 * if appropriate. The function pointers within the Locale structure are
 * initialized to point to functions contained within the language driver.
 * If any function is not supplied by the loaded code, the built-in
 * English version is used instead.
 */

Err BindExternalCode(Locale *loc)
{
Err                 err;
LanguageDriverInfo *driverInfo;
LanguageDriverInfo *englishDriverInfo;
char                buffer[30];

    englishDriverInfo = EnglishDriverMain();

    if (loc->loc_Language == INTL_LANG_ENGLISH)
    {
        loc->loc_LoadedCode = NULL;
        driverInfo = englishDriverInfo;
    }
    else
    {
        sprintf(buffer,"$Drivers/Languages/%c%c.language",(loc->loc_Language >> 24) & 0xff,
                                                          (loc->loc_Language >> 16) & 0xff);

        err = LoadCode(buffer,&loc->loc_LoadedCode);
        if (err < 0)
            return (err);

        driverInfo = (LanguageDriverInfo *)ExecuteAsSubroutine(loc->loc_LoadedCode,0,NULL);

        if (!driverInfo)
        {
            UnloadCode(loc->loc_LoadedCode);
            loc->loc_LoadedCode = NULL;

            return (INTL_ERR_NOMEM);
        }
    }

    /* STOOPID compiler gives warnings on the following assignments even
     * though there are explicit casts to tell it that I know what I'm
     * doing. Argh.
     */

    loc->loc_CompareStrings = (COMPAREFUNC)ReturnFunc(driverInfo,englishDriverInfo,
                                                      Offset(LanguageDriverInfo *,ldi_CompareStrings));

    loc->loc_ConvertString = (CONVERTFUNC)ReturnFunc(driverInfo,englishDriverInfo,
                                                     Offset(LanguageDriverInfo *,ldi_ConvertString));

    loc->loc_GetCharAttrs = (GETATTRSFUNC)ReturnFunc(driverInfo,englishDriverInfo,
                                                     Offset(LanguageDriverInfo *,ldi_GetCharAttrs));

    loc->loc_GetDateStr = (GETDATESTRFUNC)ReturnFunc(driverInfo,englishDriverInfo,
                                                     Offset(LanguageDriverInfo *,ldi_GetDateStr));

    return (0);
}


/****************************************************************************/


void UnbindExternalCode(Locale *loc)
{
    UnloadCode(loc->loc_LoadedCode);
}


/****************************************************************************/


int32 intlCompareStrings(Item locItem, const unichar *string1, const unichar *string2)
{
Locale *loc;

#ifdef DEVELOPMENT
    /* the size can actually be longer, but checking it would cause a fault anyway.... */
    if (!IsRamAddr(string1,2))
        return (INTL_ERR_BADSOURCEBUFFER);

    /* the size can actually be longer, but checking it would cause a fault anyway.... */
    if (!IsRamAddr(string2,2))
        return (INTL_ERR_BADSOURCEBUFFER);
#endif

    /* find ourselves */
    loc = (Locale *)CheckItem(locItem,NST_INTL,INTL_LOCALE_NODE);
    if (!loc)
        return (INTL_ERR_BADITEM);

    return ((*loc->loc_CompareStrings)(string1,string2));
}


/****************************************************************************/


int32 intlConvertString(Item locItem, const unichar *string, unichar *result,
                        uint32 resultSize, uint32 flags)
{
Locale *loc;

#ifdef DEVELOPMENT
    /* the size can actually be longer, but checking it would cause a fault anyway.... */
    if (!IsRamAddr(string,2))
        return (INTL_ERR_BADSOURCEBUFFER);

    if (ValidateMem(KernelBase->kb_CurrentTask,result,resultSize) < 0)
        return (INTL_ERR_BADRESULTBUFFER);

    if (resultSize < sizeof(unichar))
        return (INTL_ERR_BUFFERTOOSMALL);

    if (flags & ~(INTL_CONVF_UPPERCASE |
                  INTL_CONVF_LOWERCASE |
                  INTL_CONVF_STRIPDIACRITICALS |
                  INTL_CONVF_HALF_WIDTH |
                  INTL_CONVF_FULL_WIDTH))
        return (INTL_ERR_BADFLAGS);
#endif

    /* find ourselves */
    loc = (Locale *)CheckItem(locItem,NST_INTL,INTL_LOCALE_NODE);
    if (!loc)
        return (INTL_ERR_BADITEM);

    return ((*loc->loc_ConvertString)(string,result,resultSize,flags));
}


/****************************************************************************/


uint32 intlGetCharAttrs(Item locItem, unichar character)
{
Locale *loc;

    /* find ourselves */
    loc = (Locale *)CheckItem(locItem,NST_INTL,INTL_LOCALE_NODE);
    if (!loc)
        return (INTL_ERR_BADITEM);

    return ((*loc->loc_GetCharAttrs)(character));
}
