
/******************************************************************************
**
**  $Id: simplemath_lib.c,v 1.2 1994/10/06 22:02:45 vertex Exp $
**
**  The simplemath link library. This contains stubs to interface to the
**  folio, as well as convenience routines which provide additional services
**  around the core folio functions.
**
******************************************************************************/

#include "types.h"
#include "nodes.h"
#include "folio.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "simplemath.h"


/****************************************************************************/


Folio *SimpleMathBase;


/****************************************************************************/


Err OpenSimpleMathFolio(void)
{
Item it;

    it = FindAndOpenFolio("simplemath");
    if (it >= 0)
        SimpleMathBase = (Folio *)LookupItem(it);

    return (int)it;
}


/****************************************************************************/


Err CloseSimpleMathFolio(void)
{
    return CloseItem(SimpleMathBase->fn.n_Item);
}


/****************************************************************************/


/* stub to call AddNumber() function in the folio */
int32 AddNumber(int32 num1, int32 num2)
{
int32 result;

    CallFolioRet(SimpleMathBase, ADDNUMBER, (num1, num2), result, (int32));

    return result;
}


/****************************************************************************/


/* stub to call AddNumber() function in the folio */
int32 SubNumber(int32 num1, int32 num2)
{
int32 result;

    CallFolioRet(SimpleMathBase, SUBNUMBER, (num1, num2), result, (int32));

    return result;
}


/****************************************************************************/


/* convenience routine */
int32 Add3Numbers(int32 num1, int32 num2, int32 num3)
{
int32 result;

    result = AddNumber(num1, num2);
    return AddNumber(result, num3);
}


/****************************************************************************/


/* convenience routine */
int32 Add4Numbers(int32 num1, int32 num2, int32 num3, int32 num4)
{
int32 result;

    result = AddNumber(num1, num2);
    result = AddNumber(result, num3);
    return AddNumber(result, num4);
}
