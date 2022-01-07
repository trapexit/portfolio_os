#ifndef __VARARGS_GLUE_H
#define __VARARGS_GLUE_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  $Id: varargs_glue.h,v 1.3 1994/10/10 23:34:03 peabody Exp $
**
**  VarArgs glue building and support macros.
**  Internal: not for release to developers (yet).
**
**  These macros are used to build varargs glue functions for functions that
**  take TagArg pointer arguments. This permits a client to build a TagArg
**  directly in the function call rather than as a seperate array, which has
**  several advantages. Without varargs a client must build an array of
**  TagArgs explicitly and fill it in:
**
**      Err startins (Item instrument, int32 note, int32 amplitude)
**      {
**          TagArg tags[] = {
**              { AF_TAG_PITCH },
**              { AF_TAG_AMPLITUDE },
**              TAG_END
**          };
**
**          tags[0].ta_Arg = (TagData)note;
**          tags[1].ta_Arg = (TagData)amplitude;
**
**          return StartInstrument (instrument, tags);
**      }
**
**  This is cumbersome and error-prone when the array has to be filled out at
**  run time:
**      . easy to mismatch tag ID with tag data
**      . easy to overrun the array
**      . easy to misuse a TagArg array by passing it to multiple functions
**
**  With varargs glue functions, a client can build the TagArg array right in
**  the function call:
**
**      Err startins (Item instrument, int32 note, int32 amplitude)
**      {
**          return StartInstrumentVA (instrument,
**                                    AF_TAG_PITCH,     note,
**                                    AF_TAG_AMPLITUDE, amplitude,
**                                    TAG_END);
**      }
**
**  This has these advantages:
**      . easier to maintain code
**          . easier to read
**          . really hard to screw up the tag id/data alignment
**      . directly supports in-line evaluation of tags (data and ids)
**      . doesn't require as much casting of the data values
**
**  These macros offer a common, convenient method of creating the glue
**  functions that can include any code required to get a compiler to do the
**  right thing when translating a variable argument list into an array.
**
**  These macros create the necessary glue functions, but the prototypes for the
**  glue functions must be added manually to the appropriate include files.
**
**  Varargs functions should have the same name as the non-varargs function
**  plus "VA" (e.g. StartInstrument() - takes a TagArg pointer,
**  StartInstrumentVA() takes a varargs list).
**
******************************************************************************/


#ifndef __FOLIO_H
  #include "folio.h"            /* GetFolioFunc() */
#endif

#ifndef __STDARG_H
  #include "stdarg.h"           /* va_list stuff */
#endif

#ifndef __TYPES_H
  #include "types.h"
#endif


/* -------------------- varargs glue support */

    /* name of first tag arg */
#define VAGLUE_FIRST_TAG_PARAM  vaglue_tag1

    /* varargs function declaration string - first TagArg + varargs */
#define VAGLUE_VA_TAGS          uint32 VAGLUE_FIRST_TAG_PARAM, ...

    /* argument to pass to non-varargs function - pointer to first TagArg */
#define VAGLUE_TAG_POINTER      (TagArg *)&VAGLUE_FIRST_TAG_PARAM

/*
    Macro to create a varargs glue function that returns a value.

    For example:

        VAGLUE_FUNC (Err,
            StartInstrumentVA (Item Instrument, VAGLUE_VA_TAGS),
            StartInstrument   (Instrument, VAGLUE_TAG_POINTER) )

    creates a function named StartInstrumentVA that returs an Err and takes an
    instrument item and a variable length argument list containing tag args. This
    function passes the address of the varargs tag list to StartInstrument().
*/
#define VAGLUE_FUNC(_rettype,_decl,_call)       \
    _rettype _decl                              \
    {                                           \
        _rettype result;                        \
        va_list ap;                             \
                                                \
        va_start (ap, VAGLUE_FIRST_TAG_PARAM);  \
                                                \
        result = _call;                         \
                                                \
        va_end (ap);                            \
                                                \
        return result;                          \
    }

/*
    Macro to create a varargs glue function that does not return a value.

    For example:

        VAGLUE_VOID_FUNC (
            FooFuncVA (int32 footype, char *fooname, VAGLUE_VA_TAGS),
            FooFunc   (footype, fooname, VAGLUE_TAG_POINTER) )

    creates a function whose prototype would be:

        void FooFuncVA (int32 footype, char *fooname, uint32 tag1, ...);

    that calls FooFunc() with the address of the tag list beginning at
    tag1.
*/
#define VAGLUE_VOID_FUNC(_decl,_call)           \
    void _decl                                  \
    {                                           \
        va_list ap;                             \
                                                \
        va_start (ap, VAGLUE_FIRST_TAG_PARAM);  \
                                                \
        _call;                                  \
                                                \
        va_end (ap);                            \
    }


/* -------------------- Folio glue building macros */
/*
    This is a set of macros that can be used to build folio glue functions.

    These 4 are used to build user function glue:

        FOLIOGLUE_FUNC(_name,_folio,_funcnum,_params,_args,_rettype)
        FOLIOGLUE_VA_FUNC(_name,_folio,_funcnum,_params,_args,_rettype)
        FOLIOGLUE_VOID_FUNC(_name,_folio,_funcnum,_params,_args)
        FOLIOGLUE_VOID_VA_FUNC(_name,_folio,_funcnum,_params,_args)

    These 4 are used to build glue functions to provide supervisor access to
    SWI functions (calls a SWI function as a function):

        FOLIOGLUE_SUPER_FUNC(_name,_folio,_swinum,_params,_args,_rettype)
        FOLIOGLUE_SUPER_VA_FUNC(_name,_folio,_swinum,_params,_args,_rettype)
        FOLIOGLUE_SUPER_VOID_FUNC(_name,_folio,_swinum,_params,_args)
        FOLIOGLUE_SUPER_VOID_VA_FUNC(_name,_folio,_swinum,_params,_args)

    These 4 are used to build glue to call a function pointer (the common code
    used to construct the above macro sets).

        FOLIOGLUE_GLUE(_name,_funcptr,_params,_args,_rettype)
        FOLIOGLUE_VA_GLUE(_name,_funcptr,_params,_args,_rettype)
        FOLIOGLUE_VOID_GLUE(_name,_funcptr,_params,_args)
        FOLIOGLUE_VOID_VA_GLUE(_name,_funcptr,_params,_args)

    In each macro set, the suffixes mean:

        <none>          Construct a fixed-args glue function for a
                        folio function that returns a value.

        _VA             Construct a varargs glue function for a folio
                        function that returns a value.

        _VOID           Construct a fixed-args glue function for a
                        folio function that does not return a value.

        _VOID_VA        Construct a varargs glue function for a folio
                        function that does not return a value.

    In all cases the args are as follows:

        _name           Name of glue function to create.

        _folio          Pointer to folio base for folio containing function to call

        _funcnum        User folio function # (-f_MaxUserFunctions..-1) of function to call

        _swinum         SWI function # (0..f_MaxSWIFunctions-1) to emulate a call to

        _funcptr        Address of function to call

        _params         Formal parameter list for glue function

        _args           Parameters to be passed to folio function

        _rettype        Return type of glue function (e.g. Err, Item, etc.)


    @@@ this file is perhaps not the most correct location for these given that
        only half of 'em are varargs things, but sticking these in folio.h didn't
        seem to make a great deal of sense.  Perhaps a folio_glue.h or glue.h
        include instead?
*/

    /* folio function glue builders */
#define FOLIOGLUE_FUNC(_name,_folio,_funcnum,_params,_args,_rettype) \
    FOLIOGLUE_GLUE (_name, GetFolioFunc(_folio,_funcnum), _params, _args, _rettype)

#define FOLIOGLUE_VA_FUNC(_name,_folio,_funcnum,_params,_args,_rettype) \
    FOLIOGLUE_VA_GLUE (_name, GetFolioFunc(_folio,_funcnum), _params, _args, _rettype)

#define FOLIOGLUE_VOID_FUNC(_name,_folio,_funcnum,_params,_args) \
    FOLIOGLUE_VOID_GLUE (_name, GetFolioFunc(_folio,_funcnum), _params, _args)

#define FOLIOGLUE_VOID_VA_FUNC(_name,_folio,_funcnum,_params,_args) \
    FOLIOGLUE_VOID_VA_GLUE (_name, GetFolioFunc(_folio,_funcnum), _params, _args)


    /* super vector (swi emulation) glue builders */
#define FOLIOGLUE_SUPER_FUNC(_name,_folio,_swinum,_params,_args,_rettype) \
    FOLIOGLUE_GLUE (_name, GetFolioSWIFunc(_folio,_swinum), _params, _args, _rettype)

#define FOLIOGLUE_SUPER_VA_FUNC(_name,_folio,_swinum,_params,_args,_rettype) \
    FOLIOGLUE_VA_GLUE (_name, GetFolioSWIFunc(_folio,_swinum), _params, _args, _rettype)

#define FOLIOGLUE_SUPER_VOID_FUNC(_name,_folio,_swinum,_params,_args) \
    FOLIOGLUE_VOID_GLUE (_name, GetFolioSWIFunc(_folio,_swinum), _params, _args)

#define FOLIOGLUE_SUPER_VOID_VA_FUNC(_name,_folio,_swinum,_params,_args) \
    FOLIOGLUE_VOID_VA_GLUE (_name, GetFolioSWIFunc(_folio,_swinum), _params, _args)


    /* general glue functions for building glue function to call _funcptr */
#define FOLIOGLUE_GLUE(_name,_funcptr,_params,_args,_rettype) \
    _rettype _name _params \
    { \
        return (_rettype) (*_funcptr) _args; \
    }

#define FOLIOGLUE_VA_GLUE(_name,_funcptr,_params,_args,_rettype) \
    VAGLUE_FUNC (_rettype, _name _params, (_rettype)(*_funcptr) _args)

#define FOLIOGLUE_VOID_GLUE(_name,_funcptr,_params,_args) \
    void _name _params \
    { \
        (*_funcptr) _args; \
    }

#define FOLIOGLUE_VOID_VA_GLUE(_name,_funcptr,_params,_args) \
    VAGLUE_VOID_FUNC (_name _params, (*_funcptr) _args)


#endif /* __VARARGS_GLUE_H */
