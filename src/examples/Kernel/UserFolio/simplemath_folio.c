
/******************************************************************************
**
**  $Id: simplemath_folio.c,v 1.3 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/userfolio
|||	userfolio - Demonstrates how to create a folio and a link library to
|||	    accompany it.
|||
|||	  Description
|||
|||	    This is an example of a simple demand-loaded folio.
|||
|||	    Folios are a convenient way to package functionality. They are loaded into
|||	    memory by the kernel when they are needed, and removed when no one is
|||	    using them. The code within folios is shared by all users of the folio.
|||
|||	    If you have some functionality that is needed by multiple modules in your
|||	    title, it is best to encapsulate the functionality in a folio. The folio
|||	    can remain in memory while different sections of the title get loaded into
|||	    memory.
|||
|||	    simplemath_folio.c is the code needed to create a simple folio, containing
|||	    two routines. AddNumber() adds two numbers together while SubNumber() does
|||	    a substraction.
|||
|||	    simplemath_lib.c is the link library that is used to interface to the
|||	    folio. It provides OpenSimpleMathFolio() and CloseSimpleMathFolio() which
|||	    let clients gain access to the folio. It provides simple stubs for
|||	    AddNumber() and SubNumber(). These stubs let a client get the right
|||	    address and jump into the folio for execution of the code. The link
|||	    library also has a few convenience routines, which are routines that add
|||	    simple functionality on top of what the folio provides.
|||
|||	    To use this folio, it must be called simplemath.folio, and be put in the
|||	    $app/folios directory.
|||
|||	  Associated Files
|||
|||	    simplemath.h, simplemath_folio.c, simplemath_lib.c
|||
**/

#include "types.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "simplemath.h"


/****************************************************************************/


/* User functions for this folio */
static void *(*FolioUserFuncs[])() =
{
    /* add new functions at the top */
    (void *(*)())SubNumber,   /* -2 */
    (void *(*)())AddNumber    /* -1 */
};
#define NUM_USERFUNCS (sizeof(FolioUserFuncs)/sizeof(void *))


/*****************************************************************************/


/* Tags used when creating the Folio */
static TagArg FolioTags[] =
{
    { TAG_ITEM_NAME,                (void *) "simplemath"},      /* name of folio              */
    { CREATEFOLIO_TAG_NUSERVECS,    (void *) NUM_USERFUNCS },    /* number of user functions   */
    { CREATEFOLIO_TAG_USERFUNCS,    (void *) FolioUserFuncs },   /* list of user functions     */
    { TAG_END,                      (void *) 0 },                /* end of tag list */
};


/****************************************************************************/


int main(int32 argc, char **argv)
{
Item result;

    if (argc != DEMANDLOAD_MAIN_CREATE)
        return 0;

    result = CreateItem(MKNODEID(KERNELNODE,FOLIONODE), FolioTags);

    return (int)result;
}


/****************************************************************************/


/* a user-callable folio function */
int32 AddNumber(int32 num1, int32 num2)
{
    return num1 + num2;
}


/****************************************************************************/


/* a user-callable folio function */
int32 SubNumber(int32 num1, int32 num2)
{
    return num1 - num2;
}
