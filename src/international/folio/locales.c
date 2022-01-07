/* $Id: locales.c,v 1.14 1994/12/22 20:13:42 vertex Exp $ */

/* #define TRACING */

#include "types.h"
#include "folio.h"
#include "item.h"
#include "mem.h"
#include "sysinfo.h"
#include "string.h"
#include "filestream.h"
#include "filestreamfunctions.h"
#include "super.h"
#include "international_folio.h"
#include "englishdriver.h"
#include "externalcode.h"
#include "usermodeserver.h"
#include "countrydb.h"
#include "locales.h"


/****************************************************************************/


Item defaultLocaleItem = -1;


/****************************************************************************/


#define ALLOC        (0xff)
#define INLINE(size) (size)
#define END_DATA     (0)


/* This is a data table to drive the country file parser. This
 * data is also used when freeing a Locale structure, to determine where
 * pointers are located within the structure in order to free the memory
 * pointed by them.
 *
 * INLINE(X) says that there are X 32-bit words of data to be loaded
 * sequentially.
 *
 * ALLOC says that the following byte in the file indicates the number
 * of 32-bit words of memory to allocate. The data to copy into this memory
 * follows the length byte.
 *
 * END_DATA tells the parser to stop...
 */

static const uint8 PackedLocaleData[] =
{
    ALLOC,       /* loc_Dialects                                     */
    INLINE(6),   /* loc_Country                                      */
                 /* loc_GMTOffset                                    */
                 /* loc_MeasuringSystem                              */
                 /* loc_CalendarType                                 */
                 /* loc_DrivingType                                  */

                 /* loc_Numbers.ns_PosGroups                         */
    ALLOC,       /* loc_Numbers.ns_PosGroupSep                       */
    ALLOC,       /* loc_Numbers.ns_PosRadix                          */
    INLINE(1),   /* loc_Numbers.ns_PosFractionalGroups               */
    ALLOC,       /* loc_Numbers.ns_PosFractionalGroupSep             */
    ALLOC,       /* loc_Numbers.ns_PosFormat                         */
    INLINE(3),   /* loc_Numbers.ns_PosMinFractionalDigits            */
                 /* loc_Numbers,ns_PosMaxFractionalDigits            */
                 /* loc_Numbers.ns_NegGroups                         */
    ALLOC,       /* loc_Numbers.ns_NegGroupSep                       */
    ALLOC,       /* loc_Numbers.ns_NegRadix                          */
    INLINE(1),   /* loc_Numbers.ns_NegFractionalGroups               */
    ALLOC,       /* loc_Numbers.ns_NegFractionalGroupSep             */
    ALLOC,       /* loc_Numbers.ns_NegFormat                         */
    INLINE(2),   /* loc_Numbers.ns_NegMinFractionalDigits            */
                 /* loc_Numbers.ns_NegMaxFractionalDigits            */
    ALLOC,       /* loc_Numbers.ns_Zero                              */
    INLINE(1),   /* loc_Numbers.ns_Flags                             */

    INLINE(1),   /* loc_Currency.ns_PosGroups                        */
    ALLOC,       /* loc_Currency.ns_PosGroupSep                      */
    ALLOC,       /* loc_Currency.ns_PosRadix                         */
    INLINE(1),   /* loc_Currency.ns_PosFractionalGroups              */
    ALLOC,       /* loc_Currency.ns_PosFractionalGroupSep            */
    ALLOC,       /* loc_Currency.ns_PosFormat                        */
    INLINE(3),   /* loc_Currency.ns_PosMinFractionalDigits           */
                 /* loc_Currency,ns_PosMaxFractionalDigits           */
                 /* loc_Currency.ns_NegGroups                        */
    ALLOC,       /* loc_Currency.ns_NegGroupSep                      */
    ALLOC,       /* loc_Currency.ns_NegRadix                         */
    INLINE(1),   /* loc_Currency.ns_NegFractionalGroups              */
    ALLOC,       /* loc_Currency.ns_NegFractionalGroupSep            */
    ALLOC,       /* loc_Currency.ns_NegFormat                        */
    INLINE(2),   /* loc_Currency.ns_NegMinFractionalDigits           */
                 /* loc_Currency.ns_NegMaxFractionalDigits           */
    ALLOC,       /* loc_Currency.ns_Zero                             */
    INLINE(1),   /* loc_Currency.ns_Flags                            */

    INLINE(1),   /* loc_SmallCurrency.ns_PosGroups                   */
    ALLOC,       /* loc_SmallCurrency.ns_PosGroupSep                 */
    ALLOC,       /* loc_SmallCurrency.ns_PosRadix                    */
    INLINE(1),   /* loc_SmallCurrency.ns_PosFractionalGroups         */
    ALLOC,       /* loc_SmallCurrency.ns_PosFractionalGroupSep       */
    ALLOC,       /* loc_SmallCurrency.ns_PosFormat                   */
    INLINE(3),   /* loc_SmallCurrency.ns_PosMinFractionalDigits      */
                 /* loc_SmallCurrency,ns_PosMaxFractionalDigits      */
                 /* loc_SmallCurrency.ns_NegGroups                   */
    ALLOC,       /* loc_SmallCurrency.ns_NegGroupSep                 */
    ALLOC,       /* loc_SmallCurrency.ns_NegRadix                    */
    INLINE(1),   /* loc_SmallCurrency.ns_NegFractionalGroups         */
    ALLOC,       /* loc_SmallCurrency.ns_NegFractionalGroupSep       */
    ALLOC,       /* loc_SmallCurrency.ns_NegFormat                   */
    INLINE(2),   /* loc_SmallCurrency.ns_NegMinFractionalDigits      */
                 /* loc_SmallCurrency.ns_NegMaxFractionalDigits      */
    ALLOC,       /* loc_SmallCurrency.ns_Zero                        */
    INLINE(1),   /* loc_SmallCurrency.ns_Flags                       */

    ALLOC,       /* loc_Date                                         */
    ALLOC,       /* loc_ShortDate                                    */
    ALLOC,       /* loc_Time                                         */
    ALLOC,       /* loc_ShortTime                                    */

    END_DATA
};


/*****************************************************************************/


typedef struct UserModeData
{
    Locale        *umd_Locale;
    LanguageCodes  umd_DefaultLanguage;
    LanguageCodes  umd_DesiredLanguage;
    CountryCodes   umd_DefaultCountry;
    CountryCodes   umd_DesiredCountry;
    char          *umd_FileName;
} UserModeData;


/*****************************************************************************/


/* Free all the memory associated with a Locale structure. This runs under
 * the context of the user mode server thread.
 */
static Err UnloadCountryInfo(Locale *loc)
{
uint32    i;
void    **ptr;
uint8     type;

    TRACE(("UNLOADCOUNTRYINFO: entering with loc = $%lx\n",loc));

    UnbindExternalCode(loc);

    i   = 0;
    ptr = (void **)&loc->loc_Dialects;
    while (TRUE)
    {
        type = PackedLocaleData[i];
        if (type == END_DATA)
            break;

        if (type == ALLOC)
        {
            FreeMem(*ptr,-1);
            ptr = &ptr[1];
        }
        else /* if (type == INLINE(x)) */
        {
            ptr = &ptr[type];
        }
        i++;
    }

    TRACE(("UNLOADCOUNTRYINFO: exiting\n"));

    return (0);
}


/*****************************************************************************/


/* This routine loads in the country information needed by a Locale structure.
 * The information is taken from a standard database file.
 *
 * This runs under the context of the user mode server thread, since it
 * needs to be in user mode in order to call the file system's IO routines
 */
static Item LoadCountryInfo(UserModeData *umd)
{
Stream  *stream;
uint32  *dest;
void    *ptr;
uint8    type;
uint8    allocSize;
uint8    i;
Item     result;
FormHdr  form;
ChunkHdr chunk;
Locale   *loc;
bool     stop;
CountryEntry ce;

    TRACE(("LOADCOUNTRYINFO: entering with umd = $%lx\n",umd));

    loc = (Locale *)AllocMem(sizeof(Locale),MEMTYPE_ANY);
    if (!loc)
        return INTL_ERR_NOMEM;

    *loc              = *umd->umd_Locale;
    loc->loc_Language = umd->umd_DesiredLanguage;
    loc->loc_Country  = umd->umd_DesiredCountry;

    TRACE(("LOADCOUNTRYINFO: looking for country %d, language $%x\n",loc->loc_Country,loc->loc_Language));

    /* Given a country code, look in our database file for the relevant
     * country information. If we can't find it, revert back to the machine's
     * default and try again. If that still doesn't work, revert back to
     * US and try again.
     */

    result = INTL_ERR_CANTFINDCOUNTRY;

    stream = OpenDiskStream("$folios/International/CountryDatabase",0);
    if (stream)
    {
        TRACE(("LOADCOUNTRYINFO: opened the country database\n"));

        if (ReadDiskStream(stream,(char *)&form,sizeof(form)) == sizeof(form))
        {
            if ((form.ID == ID_FORM) && (form.FormType == ID_INTL))
            {
                TRACE(("LOADCOUNTRYINFO: found INTL form\n"));

                stop = FALSE;
                while (TRUE)
                {
                    if (ReadDiskStream(stream,(char *)&chunk,sizeof(chunk)) != sizeof(chunk))
                        break;

                    if (chunk.ID == ID_CTRY)
                    {
                        TRACE(("LOADCOUNTRYINFO: found CTRY chunk\n"));

                        while (!stop)
                        {
                            /* read a country entry */
                            if (ReadDiskStream(stream,(char *)&ce,sizeof(ce)) != sizeof(ce))
                            {
                                stop = TRUE;
                                break;
                            }

                            TRACE(("LOADCOUNTRYINFO: found country entry $%d\n",ce.ce_Country));

                            if (ce.ce_Country == 0)
                            {
                                /* we've reached the end of the table... */

                                if (loc->loc_Country != umd->umd_DefaultCountry)
                                {
                                    /* Restart the scan. This time we're looking for
                                     * the machine's default country.
                                     */
                                    loc->loc_Country = umd->umd_DefaultCountry;
                                    SeekDiskStream(stream,sizeof(form)+sizeof(chunk),SEEK_SET);
                                    continue;
                                }
                                else if (loc->loc_Country != INTL_CNTRY_UNITED_STATES)
                                {
                                    loc->loc_Country = INTL_CNTRY_UNITED_STATES;
                                    SeekDiskStream(stream,sizeof(form)+sizeof(chunk),SEEK_SET);
                                    continue;
                                }

                                stop = TRUE;
                                break;
                            }

                            if (ce.ce_Country == loc->loc_Country)
                            {
                                TRACE(("LOADCOUNTRYINFO: found needed country\n"));

                                if (SeekDiskStream(stream,ce.ce_SeekOffset,SEEK_CUR) < 0)
                                {
                                    stop = TRUE;
                                    break;
                                }

                                i    = 0;
                                dest = (uint32 *)&loc->loc_Dialects;
                                while (TRUE)
                                {
                                    type = PackedLocaleData[i];
                                    if (type == END_DATA)
                                    {
                                        result = 0; /* it worked! */
                                        break;
                                    }

                                    if (type == ALLOC)
                                    {
                                        if (ReadDiskStream(stream,&allocSize,1) != 1)
                                        {
                                            break;
                                        }

                                        if (allocSize)
                                        {
                                            ptr = AllocMem((uint32)allocSize * 4,MEMTYPE_TRACKSIZE);
                                            if (!ptr)
                                            {
                                                result = INTL_ERR_NOMEM;
                                                break;
                                            }

                                            if (ReadDiskStream(stream,(char *)ptr,(uint32)allocSize * 4) < (uint32)allocSize * 4)
                                            {
                                                break;
                                            }
                                        }
                                        else
                                        {
                                            ptr = NULL;
                                        }

                                        *dest++ = (uint32)ptr;
                                    }
                                    else /* if (type == INLINE(x)) */
                                    {
                                        if (ReadDiskStream(stream,(char *)dest,(uint32)type * 4) != (uint32)type * 4)
                                        {
                                            break;
                                        }

                                        dest = &dest[type];
                                    }

                                    i++;
                                }

                                TRACE(("LOADCOUNTRYINFO: done loading country info\n"));
                                stop = TRUE;
                            }
                        }
                    }

                    if (stop)
                        break;

                    if (SeekDiskStream(stream,IFF_ROUND(chunk.Size),SEEK_CUR) < 0)
                        break;
                }
            }
        }
        CloseDiskStream(stream);
    }

    TRACE(("LOADCOUNTRYINFO: after country info stuff, result $%x\n",result));

    if (result >= 0)
    {
        /* Given a language code, try to load in the appropriate language driver.
         * If we can't find it, revert back to the machine's default. If that
         * still won't work, revert back to English.
         */

        TRACE(("LOADCOUNTRYINFO: loading language driver\n"));
        result = BindExternalCode(loc);
        if (result < 0)
        {
            TRACE(("LOADCOUNTRYINFO: no language driver, result = %d\n",result));

            /* We couldn't load the needed driver. Try to load the driver for the
             * machine's default language.
             */

            if (loc->loc_Language != umd->umd_DefaultLanguage)
            {
                TRACE(("LOADCOUNTRYINFO: attempting to load SysInfo-derived language driver\n"));
                TRACE(("                 (lang = $%08x)\n",umd->umd_DefaultLanguage));

                loc->loc_Language = umd->umd_DefaultLanguage;

                result = BindExternalCode(loc);
                if (result < 0)
                {
                    /* Darn, this language driver can't be found either. Give up, and
                     * use the built-in English language driver.
                     */

                    TRACE(("LOADCOUNTRYINFO: Could not find SysInfo-derived language driver, using English\n"));

                    loc->loc_Language = INTL_LANG_ENGLISH;
                    result = BindExternalCode(loc);
                }
            }
        }
    }

    if (result >= 0)
    {
        SuperBCopy((int32 *)loc,(int32 *)umd->umd_Locale,sizeof(Locale));
        result = loc->loc.n_Item;
    }
    else
    {
        UnloadCountryInfo(loc);
    }

    FreeMem(loc,sizeof(Locale));

    TRACE(("LOADCOUNTRYINFO: exiting with $%lx\n",result));
    TRACE(("                 (lang = $%08x)\n",umd->umd_Locale->loc_Language));

    return (result);
}


/*****************************************************************************/


/* This routine loads in the prefs information from a given file.
 * The information includes a language code and a country code.
 *
 * This runs under the context of the user mode server thread, since it
 * needs to be in user mode in order to call the file system's IO routines
 */
static Item LoadPrefsInfo(UserModeData *umd)
{
Stream      *stream;
FormHdr      form;
ChunkHdr     chunk;
uint32       temp;
UserModeData local;

    TRACE(("LOADPREFSINFO: entering with umd = $%x\n",umd));

    local = *umd;

    stream = OpenDiskStream(umd->umd_FileName,0);
    if (stream)
    {
        TRACE(("LOADPREFSINFO: found '%s' config file\n",umd->umd_FileName));

        if (ReadDiskStream(stream,(char *)&form,sizeof(form)) == sizeof(form))
        {
            if ((form.ID == ID_FORM) && (form.FormType == ID_PREF))
            {
                while (TRUE)
                {
                    if (ReadDiskStream(stream,(char *)&chunk,sizeof(chunk)) != sizeof(chunk))
                        break;

                    if (chunk.ID == ID_INTL)
                    {
                        /* read the language code */
                        if (ReadDiskStream(stream,(char *)&temp,sizeof(temp)) != sizeof(temp))
                            break;
                        local.umd_DesiredLanguage = (LanguageCodes)temp;

                        /* read the country code */
                        if (ReadDiskStream(stream,(char *)&temp,sizeof(temp)) != sizeof(temp))
                            break;
                        local.umd_DesiredCountry = (CountryCodes)temp;

                        break;
                    }

                    if (SeekDiskStream(stream,IFF_ROUND(chunk.Size),SEEK_CUR) < 0)
                        break;
                }
            }
        }
        CloseDiskStream(stream);
    }

    SuperBCopy((int32 *)&local,(int32 *)umd,sizeof(local));

    TRACE(("LOADPREFSINFO: exiting\n"));

    return (0);
}


/*****************************************************************************/


Item OpenLocaleItem(Locale *loc, TagArg *args)
{
    SUPERTRACE(("OPENLOCALEITEM: entering with loc = $%lx, args = %lx\n",loc,args));

    if (args)
        return (INTL_ERR_BADTAG);

    loc->loc_OpenCount++;

    SUPERTRACE(("OPENLOCALEITEM: exiting with $%lx\n",loc->loc.n_Item));

    return (loc->loc.n_Item);
}


/****************************************************************************/


Err CloseLocaleItem(Locale *loc)
{
    SUPERTRACE(("CLOSELOCALEITEM: entering with $%lx\n",loc));

    loc->loc_OpenCount--;

    SUPERTRACE(("CLOSELOCALEITEM: exiting with 0\n"));

    return (0);
}


/*****************************************************************************/


Item CreateLocaleItem(Locale *loc, TagArg *args)
{
Item           result;
UserModeData   umd;
LanguageCodes  sysinfoLang;
CountryCodes   sysinfoCountry;

    SUPERTRACE(("CREATELOCALEITEM: entering with loc = $%lx, args = %lx\n",loc,args));

    if (args)
        return (INTL_ERR_BADTAG);

    loc->loc_Semaphore = SuperCreateSemaphore(NULL,0);
    if (loc->loc_Semaphore < 0)
        return (loc->loc_Semaphore);

    SuperLockSemaphore(loc->loc_Semaphore,SEM_WAIT);

    switch (SuperQuerySysInfo(SYSINFO_TAG_INTLLANGCNTRY,NULL,0))
    {
        case SYSINFO_LANGCNTRY_GERMANY   : sysinfoLang    = INTL_LANG_GERMAN;
                                           sysinfoCountry = INTL_CNTRY_GERMANY;
                                           break;

        case SYSINFO_LANGCNTRY_JAPAN     : sysinfoLang    = INTL_LANG_JAPANESE;
                                           sysinfoCountry = INTL_CNTRY_JAPAN;
                                           break;

        case SYSINFO_LANGCNTRY_SPAIN     : sysinfoLang    = INTL_LANG_SPANISH;
                                           sysinfoCountry = INTL_CNTRY_SPAIN;
                                           break;

        case SYSINFO_LANGCNTRY_ITALY     : sysinfoLang    = INTL_LANG_ITALIAN;
                                           sysinfoCountry = INTL_CNTRY_ITALY;
                                           break;

        case SYSINFO_LANGCNTRY_CHINA     : sysinfoLang    = INTL_LANG_CHINESE;
                                           sysinfoCountry = INTL_CNTRY_CHINA;
                                           break;

        case SYSINFO_LANGCNTRY_KOREA     : sysinfoLang    = INTL_LANG_KOREAN;
                                           sysinfoCountry = INTL_CNTRY_KOREA_SOUTH;
                                           break;

        case SYSINFO_LANGCNTRY_FRANCE    : sysinfoLang    = INTL_LANG_FRENCH;
                                           sysinfoCountry = INTL_CNTRY_FRANCE;
                                           break;

        case SYSINFO_LANGCNTRY_UK        : sysinfoLang    = INTL_LANG_ENGLISH;
                                           sysinfoCountry = INTL_CNTRY_UNITED_KINGDOM;
                                           break;

        case SYSINFO_LANGCNTRY_AUSTRALIA : sysinfoLang    = INTL_LANG_ENGLISH;
                                           sysinfoCountry = INTL_CNTRY_AUSTRALIA;
                                           break;

        case SYSINFO_LANGCNTRY_MEXICO    : sysinfoLang    = INTL_LANG_SPANISH;
                                           sysinfoCountry = INTL_CNTRY_MEXICO;
                                           break;

        case SYSINFO_LANGCNTRY_CANADA    : sysinfoLang    = INTL_LANG_ENGLISH;
                                           sysinfoCountry = INTL_CNTRY_CANADA;
                                           break;

        default                          : sysinfoLang    = INTL_LANG_ENGLISH;
                                           sysinfoCountry = INTL_CNTRY_UNITED_STATES;
                                           break;
    }

    umd.umd_Locale          = loc;
    umd.umd_DefaultLanguage = sysinfoLang;
    umd.umd_DesiredLanguage = sysinfoLang;
    umd.umd_DefaultCountry  = sysinfoCountry;
    umd.umd_DesiredCountry  = sysinfoCountry;
    umd.umd_FileName        = "/NVRAM/3DO Settings";

    SUPERTRACE(("CREATELOCALEITEM: calling user mode function: LoadPrefsInfo()\n"));
    CallUserModeFunc((USERMODEFUNC)LoadPrefsInfo,&umd);

    SUPERTRACE(("CREATELOCALEITEM: calling user mode function: LoadCountryInfo()\n"));
    result = CallUserModeFunc((USERMODEFUNC)LoadCountryInfo,&umd);

    if (result < 0)
        SuperDeleteSemaphore(loc->loc_Semaphore);
    else
        SuperUnlockSemaphore(loc->loc_Semaphore);

    SUPERTRACE(("CREATELOCALEITEM: exiting with $%x\n",result));

    return (result);
}


/****************************************************************************/


Err DeleteLocaleItem(Locale *loc)
{
    SUPERTRACE(("DELETELOCALEITEM: entering with loc = $%lx\n",loc));

    if (loc->loc_OpenCount)
        return -1; // !!! cantdeleteitem;

    SuperLockSemaphore(loc->loc_Semaphore,SEM_WAIT);
    CallUserModeFunc((USERMODEFUNC)UnloadCountryInfo,loc);
    SuperDeleteSemaphore(loc->loc_Semaphore);

    SUPERTRACE(("DELETELOCALEITEM: exiting with 0\n"));

    return (0);
}


/****************************************************************************/


Item FindLocaleItem(TagArg *args)
{
    SUPERTRACE(("FINDLOCALEITEM: entering\n"));

    if (args)
        return (INTL_ERR_BADTAG);

    if (defaultLocaleItem < 0)
    {
        defaultLocaleItem = SuperCreateSizedItem(MKNODEID(NST_INTL,INTL_LOCALE_NODE),NULL,sizeof(Locale));
        SuperSetItemOwner(defaultLocaleItem,serverThread);
    }

    SUPERTRACE(("FINDLOCALEITEM: exiting with defaultLocaleItem = %lx\n",defaultLocaleItem));

    return (defaultLocaleItem);
}
