/* $Id: autodocs.c,v 1.4 1994/09/22 16:23:49 vertex Exp $ */

/**
|||	AUTODOC PUBLIC spg/math/absvec3_f16
|||	AbsVec3_F16 - Computes the absolute value of a vector of 16.16 values.
|||
|||	  Synopsis
|||
|||	    frac16 AbsVec3_F16( vec3f16 vec )
|||
|||	  Description
|||
|||	    This function computes the absolute value of a vector of 16.16 values.
|||
|||	  Arguments
|||
|||	    vec                          The vector whose absolute value to compute.
|||
|||	  Return Value
|||
|||	    The function returns the absolute value (a 16.16 fraction).
|||
|||	  Implementation
|||
|||	    SWI implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  Caveats
|||
|||	    This function does not detect overflow conditions and will return
|||	    unpredictable results in case of overflow.
|||
|||	  See Also
|||
|||	    AbsVec4_F16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/absvec4_f16
|||	AbsVec4_F16 - Computes the absolute value of a vector of 16.16 values.
|||
|||	  Synopsis
|||
|||	    frac16 AbsVec4_F16( vec4f16 vec )
|||
|||	  Description
|||
|||	    This function computes the absolute value of a vector of 16.16 values.
|||
|||	  Arguments
|||
|||	    vec                          The vector whose absolute value to compute.
|||
|||	  Return Value
|||
|||	    The function returns the absolute value (a 16.16 fraction).
|||
|||	  Implementation
|||
|||	    SWI implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  Caveats
|||
|||	    This function does not detect overflow conditions and will return
|||	    unpredictable results in case of overflow.
|||
|||	  See Also
|||
|||	    AbsVec3_F16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/add32
|||	Add32 - Adds two 32-bit integer quantities together.
|||
|||	  Synopsis
|||
|||	    int32 Add32( int32 x, int32 y )
|||
|||	  Description
|||
|||	    This macro adds two 32-bit numbers together and returns the result.  The
|||	    macro is included for completeness.
|||
|||	    The macro is actually defined as simple addition of its arguments, and
|||	    does not check for or enforce any type cast requirements.
|||
|||	  Arguments
|||
|||	    x, y                         32-bit integers.
|||
|||	  Return Value
|||
|||	    The macro returns the sum of its arguments.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    AddF16(), AddF14(), AddF30()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/add64
|||	Add64 - Adds two 64-bit integers together.
|||
|||	  Synopsis
|||
|||	    void Add64( int64 *r, int64 *a1, int64 *a2 )
|||
|||	  Description
|||
|||	    This function adds two 64-bit integers together and returns the result.
|||	    The value deposited in r is the sum of the arguments.  This function is
|||	    actually the same function as AddF32().
|||
|||	  Arguments
|||
|||	    r                            A pointer to a 64-bit integer structure to
|||	                                 store the result.
|||
|||	    a1, a2                       Pointers to 64-bit integer addends.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    AddF32(), AddF60()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/addf14
|||	AddF14 - Adds two 2.14 format fixed point fractions together.
|||
|||	  Synopsis
|||
|||	    frac14 AddF14( frac14 x, frac14 y )
|||
|||	  Description
|||
|||	    This macro adds two 2.14 format fixed point fractions together and returns
|||	    the result.  The macro is included for completeness.
|||
|||	    The macro is actually defined as simple addition of its arguments, and
|||	    does not check for or enforce any type cast requirements.
|||
|||	  Arguments
|||
|||	    x, y                         2.14 format fixed point fractions.
|||
|||	  Return Value
|||
|||	    The macro returns the sum of the arguments.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    Add32(), AddF16(), AddF30()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/addf16
|||	AddF16 - Adds two 16.16 format fixed point fractions together.
|||
|||	  Synopsis
|||
|||	    frac16 AddF16( frac16 x, frac16 y )
|||
|||	  Description
|||
|||	    This macro adds two 16.16 format fixed point fractions together and
|||	    returns the result.  The macro is included for completeness.
|||
|||	    The macro is actually defined as simple addition of its arguments, and
|||	    does not check for or enforce any type cast requirements.
|||
|||	  Arguments
|||
|||	    x, y                         16.16 format fixed point fractions.
|||
|||	  Return Value
|||
|||	    The function returns the sum of the arguments.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    Add32(), AddF14(), AddF30()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/addf30
|||	AddF30 - Adds two 2.30-format fixed-point fractions together.
|||
|||	  Synopsis
|||
|||	    frac30 AddF30( frac30 x, frac30 y )
|||
|||	  Description
|||
|||	    This macro adds two 2.30 format fixed-point fractions together and returns
|||	    the result.  The macro is included for completeness.
|||
|||	    The macro is actually defined as the simple addition of its arguments.  It
|||	    does not check for or enforce any type cast requirements.
|||
|||	  Arguments
|||
|||	    x, y                         2.30-format fixed-point fractions.
|||
|||	  Return Value
|||
|||	    The macro returns the sum of its arguments.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    Add32(), AddF16(), AddF14()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/addf32
|||	AddF32 - Adds two 32.32-format fixed-point fractions together.
|||
|||	  Synopsis
|||
|||	    void AddF32( frac32 *r, frac32 *a1, frac32 *a2 )
|||
|||	  Description
|||
|||	    This function adds two 32.3- format fixed-point fractions together and
|||	    deposits the result in the location pointed to by the r argument.  This
|||	    function is actually the same function as Add64().
|||
|||	  Arguments
|||
|||	    r                            A pointer to a 32.32-fraction structure to
|||	                                 store the result.
|||
|||	    a1, a2                       Pointers to 32.32-format fixed-point
|||	                                 fraction addends.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    Add64(), AddF60()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/addf60
|||	AddF60 - Adds two 4.60-format fixed-point fractions together.
|||
|||	  Synopsis
|||
|||	    void AddF60( frac60 *r, frac60 *a1, frac60 *a2 )
|||
|||	  Description
|||
|||	    This function adds two 4.60-format fixed-point fractions together and
|||	    deposits the result in the location pointed to by the r argument.
|||
|||	  Arguments
|||
|||	    r                            A pointer to a 4.60-fraction structure to
|||	                                 store the result.
|||
|||	    a1, a2                       Pointers to 4.60-format fixed-point fraction
|||	                                 addends.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    Add64(), AddF32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/atan2f16
|||	Atan2F16 - Computes the arctangent of a ratio.
|||
|||	  Synopsis
|||
|||	    frac16 Atan2F16( frac16 x, frac16 y )
|||
|||	  Description
|||
|||	    This function computes the arctangent of the ratio y/x.
|||
|||	    The result assumes 256.0 units in the circle (or 16,777,216 units if used
|||	    as an integer).  A correct 16.16 result is returned if the arguments are
|||	    int32, frac30 or frac14, as long as both arguments are the same type.
|||
|||	  Arguments
|||
|||	    x, y                         16.16 format fractions.
|||
|||	  Return Value
|||
|||	    The function returns the arctangent of the ratio of the arguments.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/closemathfolio
|||	CloseMathFolio - Closes the math folio and resets the mathbase global
|||	                 variable to zero.
|||
|||	  Synopsis
|||
|||	    Err CloseMathFolio(void)
|||
|||	  Description
|||
|||	    CloseMathFolio attempts to clsoe the math folio structure.  If successful,
|||	    it also sets _Mathfolio to zero and returns zero.  On failure a negative
|||	    error is returend.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in operamath.lib V22.
|||
|||	  See Also
|||
|||	    OpenMathFolio()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/compares64
|||	CompareS64 - Compares two signed 64-bit integer quantities.
|||
|||	  Synopsis
|||
|||	    int32 CompareS64( int64 *s1, int64 *s2 )
|||
|||	  Description
|||
|||	    This function compares two signed 64-bit integers.  This function is
|||	    actually the same function as CompareSF32() and CompareSF60().
|||
|||	  Arguments
|||
|||	    s1, s2                       Pointers to signed 64-bit integers.
|||
|||	  Return Value
|||
|||	    The result is positive if s1 > s2, negative if s1 < s2 and zero if
|||         s1 == s2.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    CompareSF32(), CompareU64(), CompareUF32(), CompareUF60(), CompareSF60()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/comparesf32
|||	CompareSF32 - Compares two signed 32.32-format fractions.
|||
|||	  Synopsis
|||
|||	    int32 CompareSF32( frac32 *s1, frac32 *s2 )
|||
|||	  Description
|||
|||	    This function compares two signed 32.32-format fractions.  This function
|||	    is actually the same function as CompareS64() and CompareSF60().
|||
|||	  Arguments
|||
|||	    s1, s2                       Pointers to signed 32.32 fractions.
|||
|||	  Return Value
|||
|||	    The result is positive if s1 > s2, negative if s1 < s2 and zero if
|||         s1 == s2.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    CompareS64(), CompareU64(), CompareUF32(), CompareUF60(), CompareSF60()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/comparesf60
|||	CompareSF60 - Compares two signed 4.60-format fractions.
|||
|||	  Synopsis
|||
|||	    int32 CompareSF60( frac60 *s1, frac60 *s2 )
|||
|||	  Description
|||
|||	    This function compares two signed 4.60-format fractions.  This function
|||	    is actually the same function as CompareS64() and CompareSF32().
|||
|||	  Arguments
|||
|||	    s1, s2                       Pointers to signed 4.60 fractions.
|||
|||	  Return Value
|||
|||	    The result is positive if s1 > s2, negative if s1 < s2 and zero if
|||         s1 == s2.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    CompareS64(), CompareU64(), CompareUF32(), CompareUF60(), CompareSF32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/compareu64
|||	CompareU64 - Compares two unsigned 64-bit integer quantities.
|||
|||	  Synopsis
|||
|||	    int32 CompareU64( uint64 *s1, uint64 *s2 )
|||
|||	  Description
|||
|||	    This function compares two unsigned 64-bit integers.  This function is
|||	    actually the same function as CompareUF32() and CompareUF60().
|||
|||	  Arguments
|||
|||	    s1, s2                       Pointers to unsigned 64-bit integers.
|||
|||	  Return Value
|||
|||	    The function returns 1 if s1 > s2, zero if s1 == s2, or -1 if s1 < s2.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    CompareSF32(), CompareS64(), CompareUF32(), CompareSF60(), CompareUF60()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/compareuf32
|||	CompareUF32 - Compares two unsigned 32.32-format fractions.
|||
|||	  Synopsis
|||
|||	    int32 CompareUF32( ufrac32 *s1, ufrac32 *s2 )
|||
|||	  Description
|||
|||	    This function compares two unsigned 32.32-format fractions.  This
|||	    function is actually the same function as CompareU64() and
|||	    CompareUF60().
|||
|||	  Arguments
|||
|||	    s1, s2                       Pointers to unsigned 32.32 fractions.
|||
|||	  Return Value
|||
|||	    The function returns 1 if s1 > s2, zero if s1 == s2, or -1 if s1 < s2.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    CompareS64(), CompareSF32(), CompareU64(), CompareUF60(), CompareSF60()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/compareuf60
|||	CompareUF60 - Compares two unsigned 4.60-format fractions.
|||
|||	  Synopsis
|||
|||	    int32 CompareUF60( ufrac60 *s1, ufrac60 *s2 )
|||
|||	  Description
|||
|||	    This function compares two unsigned 4.60-format fractions.  This
|||	    function is actually the same function as CompareU64() and
|||	    CompareUF32().
|||
|||	  Arguments
|||
|||	    s1, s2                       Pointers to unsigned 4.60 fractions.
|||
|||	  Return Value
|||
|||	    The function returns 1 if s1 > s2, zero if s1 == s2, or -1 if s1 < s2.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    CompareS64(), CompareSF32(), CompareU64(), CompareUF32(), CompareSF60()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/convert32_f16
|||	Convert32_F16 - Converts a 32-bit integer to a 16.16 fraction.
|||
|||	  Synopsis
|||
|||	    frac16 Convert32_F16( int32 x )
|||
|||	  Description
|||
|||	    This macro converts a 32-bit integer into a 16.16 fraction.
|||
|||	  Arguments
|||
|||	    x                            The 32-bit integer to be converted.
|||
|||	  Return Value
|||
|||	    The function returns the 16.16 result of the conversion.  The fractional
|||	    16 bits of the result are zero, and the upper 16 bits of the argument are
|||	    lost.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    ConvertF14_F16(), ConvertF16_F14(), ConvertF16_30(), ConvertF30_F16(),
|||	    Convert32_F32(), ConvertF16_32(), ConvertF32_F16(), ConvertS32_64(),
|||	    ConvertSF16_F32(), ConvertU32_64(), ConvertUF16_F32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/convert32_f32
|||	Convert32_F32 - Converts a 32-bit integer to a 32.32 fraction.
|||
|||	  Synopsis
|||
|||	    void Convert32_F32( frac32 *d, int32 x )
|||
|||	  Description
|||
|||	    This macro converts a 32-bit integer to a 32.32 fraction.  The fractional
|||	    portion of the result is zero.  The 32.32 result of the conversion is
|||	    deposited in the location pointed to by the dest argument.
|||
|||	  Arguments
|||
|||	    d                            A pointer to a 32.32 structure to store the
|||	                                 result.
|||
|||	    x                            A 32-bit integer to be converted.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    ConvertF14_F16(), ConvertF16_F14(), ConvertF16_30(), ConvertF30_F16(),
|||	    Convert32_F16(), ConvertF16_32(), ConvertF32_F16(), ConvertS32_64(),
|||	    ConvertSF16_F32(), ConvertU32_64(), ConvertUF16_F32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/convertf14_f16
|||	ConvertF14_F16 - Converts a 2.14 fraction to a 16.16 fraction.
|||
|||	  Synopsis
|||
|||	    frac16 ConvertF14_F16( frac14 x )
|||
|||	  Description
|||
|||	    This macro converts a 2.14 fraction to a 16.16 fraction.
|||
|||	  Arguments
|||
|||	    x                            The 2.14 fraction to be converted.
|||
|||	  Return Value
|||
|||	    The function returns the 16.16 fraction result of the conversion.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    ConvertF16_F14(), ConvertF16_F30(), ConvertF16_32(), ConvertF30_F16(),
|||	    Convert32_F16(), Convert32_F32(), Convert32_F16(), ConvertS32_64(),
|||	    ConvertSF16_F32(), ConvertU32_64(), ConvertUF16_F32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/convertf16_32
|||	ConvertF16_32 - Converts a 16.16 fraction to a 32-bit integer.
|||
|||	  Synopsis
|||
|||	    int32 ConvertF16_32( frac16 x )
|||
|||	  Description
|||
|||	    This macro converts a 16.16 fraction to a 32-bit integer.  The upper 16
|||	    bits are zero or are filled with the sign bit of the argument.  The
|||	    fraction bits of the argument are lost.
|||
|||	  Arguments
|||
|||	    x                            The 16.16 fraction to be converted.
|||
|||	  Return Value
|||
|||	    The function returns the 32-bit integer result of the conversion.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    ConvertF14_F16(), ConvertF16_F14(), ConvertF16_30(), ConvertF30_F16(),
|||	    Convert32_F16(), Convert32_F32(), Convert32_F16(), ConvertS32_64(),
|||	    ConvertSF16_F32(), ConvertU32_64(), ConvertUF16_F32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/convertf16_f14
|||	ConvertF16_F14 - Converts a 16.16 fraction to a 2.14 fraction.
|||
|||	  Synopsis
|||
|||	    frac14 ConvertF16_F14( frac16 x )
|||
|||	  Description
|||
|||	    This macro converts a 16.16 fraction to a 2.14 fraction, using the upper
|||	    16 bits of a 32-bit quantity.
|||
|||	  Arguments
|||
|||	    x                            The 16.16 fraction to be converted.
|||
|||	  Return Value
|||
|||	    The function returns the 2.14 fraction result of the conversion.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    ConvertF14_F16(), ConvertF16_F30(), ConvertF16_32(), ConvertF30_F16(),
|||	    Convert32_F16(), Convert32_F32(), Convert32_F16(), ConvertS32_64(),
|||	    ConvertSF16_F32(), ConvertU32_64(), ConvertUF16_F32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/convertf16_f30
|||	ConvertF16_F30 - Converts a 16.16 fraction to a 2.30 fraction.
|||
|||	  Synopsis
|||
|||	    frac30 ConvertF16_F30( frac16 x )
|||
|||	  Description
|||
|||	    This macro converts a 16.16 fraction to a 2.30 fraction.
|||
|||	  Arguments
|||
|||	    x                            The 16.16 fraction to be converted.
|||
|||	  Return Value
|||
|||	    The function returns the 2.30 fraction result of the conversion.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    ConvertF14_F16(), ConvertF16_F14(), ConvertF16_32(), ConvertF30_F16(),
|||	    Convert32_F16(), Convert32_F32(), Convert32_F16(), ConvertS32_64(),
|||	    ConvertSF16_F32(), ConvertU32_64(), ConvertUF16_F32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/convertf30_f16
|||	ConvertF30_F16 - Converts a 2.30 fraction to a 16.16 fraction.
|||
|||	  Synopsis
|||
|||	    frac16 ConvertF30_F16( frac30 x )
|||
|||	  Description
|||
|||	    This macro converts a 2.30 fraction to a 16.16 fraction.  The upper 14
|||	    bits will be filled with the sign bit of the argument.
|||
|||	  Arguments
|||
|||	    x                            A 2.30 fraction to be converted.
|||
|||	  Return Value
|||
|||	    The function returns the 16.16 fraction result of the conversion.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    ConvertF14_F16(), ConvertF16_F14(), ConvertF16_30(), ConvertF16_32,
|||	    Convert32_F16(), Convert32_F32(), Convert32_F16(), ConvertS32_64(),
|||	    ConvertSF16_F32(), ConvertU32_64(), ConvertUF16_F32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/convertf32_f16
|||	ConvertF32_F16 - Converts a 32.32 fraction to a 16.16 fraction.
|||
|||	  Synopsis
|||
|||	    frac16 ConvertF32_F16( frac32 *x )
|||
|||	  Description
|||
|||	    This macro converts a 32.32 fraction to a 16.16 fraction.  The result will
|||	    be correct only if the integer part of the argument can be expressed in 16
|||	    bits.  Only the most significant 16 bits of the fractional part of the
|||	    argument are preserved.
|||
|||	  Arguments
|||
|||	    x                            A pointer to a 32.32 fraction to be
|||	                                 converted.
|||
|||	  Return Value
|||
|||	    The function returns the 16.16 fraction result of the conversion.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    ConvertF14_F16(), ConvertF16_F14(), ConvertF16_30(), ConvertF30_F16(),
|||	    Convert32_F16(), Convert32_F32(), Convert32_F16(), ConvertS32_64(),
|||	    ConvertSF16_F32(), ConvertU32_64(), ConvertUF16_F32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/converts32_64
|||	ConvertS32_64 - Converts a 32-bit integer to a 64-bit integer.
|||
|||	  Synopsis
|||
|||	    void ConvertS32_64( int64 *d, int32 x )
|||
|||	  Description
|||
|||	    This macro converts a 32-bit integer to a 64-bit integer.  The upper 32
|||	    bits of the result are the sign bit of the argument.  The 64-bit integer
|||	    result is deposited in the location pointed to by the d argument.
|||
|||	  Arguments
|||
|||	    d                            A pointer to a 64-bit integer structure to
|||	                                 store the result.
|||
|||	    x                            A 32-bit integer to be converted.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    ConvertF14_F16(), ConvertF16_F14(), ConvertF16_30(), ConvertF30_F16(),
|||	    ConvertF16_32(), Convert32_F32(), ConvertF32_F16(), Convert32_F16(),
|||	    ConvertSF16_F32(), ConvertU32_64(), ConvertUF16_F32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/convertsf16_f32
|||	ConvertSF16_F32 - Converts a 16.16 fraction to a 32.32 fraction.
|||
|||	  Synopsis
|||
|||	    void ConvertSF16_F32( frac32 *d, frac16 x )
|||
|||	  Description
|||
|||	    This macro converts a 16.16 fraction to a 32.32 fraction.  The upper 16
|||	    bits of the result are the sign bit of the argument, the least 16 bits of
|||	    the result are zero.  The 32.32 fraction result is deposited in the
|||	    location pointed to by the d argument.
|||
|||	  Arguments
|||
|||	    d                            A pointer to a 32.32 fraction structure to
|||	                                 store the result.
|||
|||	    x                            A 16.16 fraction to be converted.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    ConvertF14_F16(), ConvertF16_F14(), ConvertF16_30(), ConvertF30_F16(),
|||	    ConvertF16_32(), Convert32_F32(), ConvertF32_F16(), ConvertS32_64(),
|||	    Convert32_F16(), ConvertU32_64(), ConvertUF16_F32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/convertu32_64
|||	ConvertU32_64 - Converts an unsigned 32-bit integer to an unsigned 64-bit
|||	                integer.
|||
|||	  Synopsis
|||
|||	    void ConvertU32_64( uint64 *d, uint32 x )
|||
|||	  Description
|||
|||	    This macro converts an unsigned 32-bit integer to an unsigned 64-bit
|||	    integer.  The upper 32 bits of the result are zero.  The 64-bit unsigned
|||	    integer result is deposited in the location pointed to by the d argument.
|||
|||	  Arguments
|||
|||	    d                            A pointer to a unsigned 64-bit integer
|||	                                 structure to store the result.
|||
|||	    x                            The unsigned 32-bit integer to be converted.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    ConvertF14_F16(), ConvertF16_F14(), ConvertF16_30(), ConvertF30_F16(),
|||	    ConvertF16_32(), Convert32_F32(), ConvertF32_F16(), Convert32_F16(),
|||	    ConvertSF16_F32(), ConvertUF16_F32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/convertuf16_f32
|||	ConvertUF16_F32 - Converts an unsigned 16.16 fraction to an unsigned 32.32
|||	                  fraction.
|||
|||	  Synopsis
|||
|||	    void ConvertUF16_F32( frac32 *d, ufrac16 x )
|||
|||	  Description
|||
|||	    This macro converts an unsigned 16.16 fraction to a 32.32 fraction.  The
|||	    upper 16 bits of the result and the least 16 bits of the result are zero.
|||	    The 32.32 fraction result is deposited in the location pointed to by the d
|||	    argument.
|||
|||	  Arguments
|||
|||	    d                            A pointer to a 32.32 fraction to store the
|||	                                 result.
|||
|||	    x                            An unsigned 16.16 fraction to be converted.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    ConvertF14_F16(), ConvertF16_F14(), ConvertF16_30(), ConvertF30_F16(),
|||	    ConvertF16_32(), Convert32_F32(), ConvertF32_F16(), ConvertS32_64(),
|||	    Convert32_F16(), ConvertU32_64()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/cosf16
|||	CosF16 - Computes the 16.16 operamath cosine of a 16.16 fraction angle.
|||
|||	  Synopsis
|||
|||	    frac16 CosF16( frac16 x )
|||
|||	  Description
|||
|||	    This function returns the 16.16 operamath cosine of a 16.16 fraction
|||	    angle.  In operamath coordinates, there are 256.0 units in a circle.
|||
|||	  Arguments
|||
|||	    x                            A 16.16 fraction describing the angle of the
|||	                                 circle.
|||
|||	  Return Value
|||
|||	    The value returned is the cosine of the input angle.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    SinF16(), CosF32(), SinF32(), CosF30(), SinF30()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/cosf30
|||	CosF30 - Computes the operamath 2.30 cosine of a 16.16 fraction.
|||
|||	  Synopsis
|||
|||	    frac30 CosF30( frac16 x )
|||
|||	  Description
|||
|||	    This function returns the operamath 2.30 cosine of a 16.16 fraction.  In
|||	    operamath coordinates, there are 256.0 units in a circle.
|||
|||	  Arguments
|||
|||	    x                            A 16.16 fraction describing the angle of the
|||	                                 circle.
|||
|||	  Return Value
|||
|||	    The function returns the 2.30 cosine of the input angle.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    SinF16(), CosF16(), SinF32(), CosF32(), SinF30()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/cosf32
|||	CosF32 - Computes the operamath 32.32 cosine of a 16.16 fraction.
|||
|||	  Synopsis
|||
|||	    frac32 CosF32( frac32 *c, frac16 x )
|||
|||	  Description
|||
|||	    This macro returns the operamath 32.32 cosine of a 16.16 fraction.  In
|||	    operamath coordinates, there are 256.0 units in a circle.
|||
|||	    The macro actually calls the SinF32() function with one quarter circle
|||	    added to the input value.
|||
|||	  Arguments
|||
|||	    c                            A pointer to a 32.32 fraction containing the
|||	                                 result.
|||
|||	    x                            A 16.16 fraction describing the angle of the
|||	                                 circle.
|||
|||	  Return Value
|||
|||	    The macro returns the cosine of the input angle.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    SinF16(), CosF16(), SinF32(), CosF30(), SinF30()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/cross3_f16
|||	Cross3_F16 - Computes the cross product of two vectors of 16.16 values.
|||
|||	  Synopsis
|||
|||	    void Cross3_F16( vec3f16 *dest, vec3f16 v1, vec3f16 v2 )
|||
|||	  Description
|||
|||	    This function multiplies two 3-coordinate vectors of 16.16 values
|||	    together and deposits the cross product in the location pointed to by
|||	    dest.
|||
|||	  Arguments
|||
|||	    dest                         A pointer to a destination vector to store
|||	                                 the resulting cross product.
|||
|||	    v1, v2                       3-coordinate vector multiplicands.
|||
|||	  Implementation
|||
|||	    SWI implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/divrems32
|||	DivRemS32 - Computes the quotient and remainder of a 32-bit division.
|||
|||	  Synopsis
|||
|||	    int32 DivRemS32( int32 *rem, int32 d1, int32 d2 )
|||
|||	  Description
|||
|||	    This function divides one 32-bit integer by another and returns the
|||	    quotient and remainder.
|||
|||	    This function calls the standard C function for division.  If you need
|||	    only the quotient or only the remainder, use the standard C notation for
|||	    division instead.
|||
|||	  Arguments
|||
|||	    rem                          A pointer to a 32-bit integer to store the
|||	                                 remainder.
|||
|||	    d1                           The dividend.
|||
|||	    d2                           The divisor.
|||
|||	  Return Value
|||
|||	    The function returns the quotient of the division.  The remainder is
|||	    deposited in the location pointed to by the rem argument.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  Caveats
|||
|||	    The function does not detect overflows.
|||
|||	  See Also
|||
|||	    DivRemU32(), DivS64(), DivU64(), DivSF16(), DivUF16(), DivRemUF16(),
|||	    DivRemSF16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/divremsf16
|||	DivRemSF16 - Computes the quotient and remainder of a 16.16 division.
|||
|||	  Synopsis
|||
|||	    frac16 DivRemSF16( frac16 *rem, frac16 d1, frac16 d2 )
|||
|||	  Description
|||
|||	    This function divides one signed 16.16 fraction by another and returns
|||	    the quotient and remainder.  This function returns a correct result if
|||	    the arguments are int32 or frac30, as long as both d1 and d2 are the same
|||	    type.
|||
|||	    The remainder is not a 16.16 fraction; instead, it is a signed fraction in
|||	    0.32 format.  The most significant bit of the remainder is a sign bit, and
|||	    this must be extended if the remainder is to be used in subsequent
|||	    calculations.  An overflow value is denoted by maximum positive return in
|||	    both values.
|||
|||	  Arguments
|||
|||	    rem                          A pointer to the remainder of the division,
|||	                                 a signed fraction in 0.32 format.
|||
|||	    d1                           The dividend.
|||
|||	    d2                           The divisor.
|||
|||	  Return Value
|||
|||	    The function returns the quotient of the division.  The remainder is
|||	    deposited in the location pointed to by the rem argument.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    DivRemU32(), DivRemS32(), DivU64(), DivSF16(), DivUF16(), DivRemUF16(),
|||	    DivS64()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/divremu32
|||	DivRemU32 - Computes the quotient and remainder of a 32-bit division.
|||
|||	  Synopsis
|||
|||	    uint32 DivRemU32( uint32 *rem, uint32 d1, uint32 d2 )
|||
|||	  Description
|||
|||	    This function divides one unsigned 32-bit integer by another and returns
|||	    the quotient and remainder.
|||
|||	    This function calls the standard C function for division.  If only the
|||	    quotient or only the remainder is desired, the standard C notation for
|||	    divide should be used.
|||
|||	  Arguments
|||
|||	    rem                          A pointer to a unsigned 32-bit integer to
|||	                                 store the remainder.
|||
|||	    d1                           The dividend.
|||
|||	    d2                           The divisor.
|||
|||	  Return Value
|||
|||	    The function returns the quotient of the division.  The remainder is
|||	    deposited in the location pointed to by the rem argument.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  Caveats
|||
|||	    The function does not detect overflows.
|||
|||	  See Also
|||
|||	    DivRemSF16(), DivRemS32(), DivU64(), DivSF16(), DivUF16(), DivRemUF16(),
|||	    DivS64()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/divremuf16
|||	DivRemUF16 - Computes the quotient and remainder of a 16.16 division.
|||
|||	  Synopsis
|||
|||	    ufrac16 DivRemUF16( ufrac16 *rem, ufrac16 d1, ufrac16 d2 )
|||
|||	  Description
|||
|||	    This function divides one unsigned 16.16 fraction by another and returns
|||	    the quotient and remainder.  This function returns a correct 16.16 result
|||	    if the arguments are uint32 or ufrac30, as long as both arguments, d1 and
|||	    d2, are the same type.
|||
|||	    The remainder is not a 16.16 fraction; instead, it is an unsigned fraction
|||	    in 0.32 format.   An overflow condition is signaled by maximum return in
|||	    both values.
|||
|||	  Arguments
|||
|||	    rem                          A pointer to a unsigned fraction in 0.32
|||	                                 format to store the remainder.
|||
|||	    d1                           The dividend.
|||
|||	    d2                           The divisor.
|||
|||	  Return Value
|||
|||	    The function returns the quotient of the division.  The remainder is
|||	    deposited in the location pointed to by the rem argument.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  Caveats
|||
|||	    The function does not detect overflows.
|||
|||	  See Also
|||
|||	    DivRemSF16(), DivRemS32(), DivRemU32(), DivSF16(), DivUF16(), DivU64(),
|||	    DivS64()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/divs64
|||	DivS64 - Computes the quotient and remainder of a 64-bit division.
|||
|||	  Synopsis
|||
|||	    int64 *DivS64( int64 *q, int64 *r, int64 *d1, int64 *d2 )
|||
|||	  Description
|||
|||	    This function divides one 64-bit integer by another and returns the
|||	    quotient and remainder.
|||
|||	  Arguments
|||
|||	    q                            A pointer to a 64-bit integer to store the
|||	                                 quotient.
|||
|||	    r                            A pointer to a 64-bit integer to store the
|||	                                 remainder.
|||
|||	    d1                           A pointer to a dividend.
|||
|||	    d2                           A pointer to a divisor.
|||
|||	  Return Value
|||
|||	    The function returns a pointer to q.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  Caveats
|||
|||	    The function does not detect overflows.
|||
|||	  See Also
|||
|||	    DivRemU32(), DivRemS32(), DivU64(), DivSF16(), DivUF16(), DivRemUF16(),
|||	    DivRemSF16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/divsf16
|||	DivSF16 - Computes the quotient of a 16.16 division.
|||
|||	  Synopsis
|||
|||	    frac16 DivSF16( frac16 d1, frac16 d2 )
|||
|||	  Description
|||
|||	    This function divides one 16.16 fraction by another and returns the
|||	    quotient.
|||
|||	    This function also returns a correct result if the arguments are int32 or
|||	    frac30, as long as both d1 and d2 are the same type.
|||
|||	  Arguments
|||
|||	    d1                           The dividend.
|||
|||	    d2                           The divisor.
|||
|||	  Return Value
|||
|||	    The function returns the quotient of the division.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  Caveats
|||
|||	    The function does not detect overflows.
|||
|||	  See Also
|||
|||	    DivRemU32(), DivRemS32(), DivU64(), DivRemSF16(), DivUF16(), DivRemUF16(),
|||	    DivS64()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/divu64
|||	DivU64 - Computes the quotient and remainder of a 64-bit division.
|||
|||	  Synopsis
|||
|||	    int64 *DivU64( uint64 *q, uint64 *r, uint64 *d1, uint64 *d2 )
|||
|||	  Description
|||
|||	    This function divides one unsigned 64-bit integer by another and returns
|||	    the quotient and remainder.
|||
|||	  Arguments
|||
|||	    q                            A pointer to a 64-bit integer to store the
|||	                                 quotient.
|||
|||	    r                            A pointer to a 64-bit integer to store the
|||	                                 remainder.
|||
|||	    d1                           A pointer to the dividend.
|||
|||	    d2                           A pointer to the divisor.
|||
|||	  Return Value
|||
|||	    The function returns a pointer to q.  The quotient is deposited in the
|||	    location pointed to by q.  The remainder is deposited in the location
|||	    pointed to by r.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  Caveats
|||
|||	    The function does not detect overflows.
|||
|||	  See Also
|||
|||	    DivRemSF16(), DivRemS32(), DivRemU32(), DivSF16(), DivUF16(),
|||	    DivRemUF16(), DivS64()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/divuf16
|||	DivUF16 - Computes the quotient of a 16.16 division.
|||
|||	  Synopsis
|||
|||	    ufrac16 DivUF16( ufrac16 d1, ufrac16 d2 )
|||
|||	  Description
|||
|||	    This function divides one unsigned 16.16 fraction by another and returns
|||	    the quotient.  This function returns a correct 16.16 result if the
|||	    arguments are uint32 or ufrac30, as long as both arguments, d1 and d2, are
|||	    the same type.
|||
|||	  Arguments
|||
|||	    d1                           The dividend.
|||
|||	    d2                           The divisor.
|||
|||	  Return Value
|||
|||	    The function returns the quotient of the division.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  Caveats
|||
|||	    The function does not detect overflows.
|||
|||	  See Also
|||
|||	    DivRemSF16(), DivRemS32(), DivRemU32(), DivSF16(), DivRemUF16(), DivU64(),
|||	    DivS64()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/dot3_f16
|||	Dot3_F16 - Multiplies two vectors of 16.16 values.
|||
|||	  Synopsis
|||
|||	    frac16 Dot3_F16( vec3f16 v1, vec3f16 v2 )
|||
|||	  Description
|||
|||	    This function multiplies two 3 coordinate vectors of 16.16 values
|||	    together and returns the dot product.
|||
|||	  Arguments
|||
|||	    v1, v2                       3-coordinate vector multiplicands.
|||
|||	  Return Value
|||
|||	    The function returns the dot product of the two vectors.
|||
|||	  Implementation
|||
|||	    SWI implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    Dot4_F16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/dot4_f16
|||	Dot4_F16 - Multiplies two vectors of 16.16 values.
|||
|||	  Synopsis
|||
|||	    frac16 Dot4_F16( vec4f16 v1, vec4f16 v2 )
|||
|||	  Description
|||
|||	    This function multiplies two 4-coordinate vectors of 16.16 values
|||	    together and returns the dot product.
|||
|||	  Arguments
|||
|||	    v1, v2                       4-coordinate vector multiplicands.
|||
|||	  Return Value
|||
|||	    The function returns the dot product of the two vectors.
|||
|||	  Implementation
|||
|||	    SWI implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    Dot3_F16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/mul32
|||	Mul32 - Multiplies two 32-bit integers.
|||
|||	  Synopsis
|||
|||	    int32 Mul32( int32 x, int32 y )
|||
|||	  Description
|||
|||	    This macro multiplies two 32-bit integers together and returns the
|||	    product.  The macro is only included for completeness.
|||
|||	  Arguments
|||
|||	    x, y                         The multiplicands.
|||
|||	  Return Value
|||
|||	    The function returns the product of the two arguments.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    Mul64(), MulS32_64(), MulU32_64(), MulF14(), MulSF16(), MulSF30(),
|||	    MulUF16(), MulSF16_F32(), MulUF16_F32(), MulSF30_F60(), MulUF30_F60()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/mul64
|||	Mul64 - Multiplies two 64-bit integers.
|||
|||	  Synopsis
|||
|||	    void Mul64( int64 *p, int64 *m1, int64 *m2 )
|||
|||	  Description
|||
|||	    This function multiplies two 64-bit integers together and returns the
|||	    product.  An overflow condition is not detected.  The 64-bit integer
|||	    result is deposited in the location pointed to by the p argument.
|||
|||	  Arguments
|||
|||	    p                            A pointer to a the location to store the
|||	                                 64-bit integer result.
|||
|||	    m1, m2                       Pointers to the locations of the two 64-bit
|||	                                 integer multiplicands.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  Caveats
|||
|||	    The function does not detect overflows.
|||
|||	  See Also
|||
|||	    Mul32(), MulS32_64(), MulU32_64(), MulF14(), MulSF16(), MulSF30(),
|||	    MulUF16(), MulSF16_F32(), MulUF16_F32(), MulSF30_F60(), MulUF30_F60()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/mulf14
|||	MulF14 - Multiplies two 2.14 fractions.
|||
|||	  Synopsis
|||
|||	    frac14 MulF14( frac14 x, frac14 y )
|||
|||	  Description
|||
|||	    This macro multiplies two 2.14 fractions together and returns the product.
|||
|||	  Arguments
|||
|||	    x, y                         The multiplicands.
|||
|||	  Return Value
|||
|||	    The function returns the product of the two arguments.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  Caveats
|||
|||	    The function does not detect overflows.
|||
|||	  See Also
|||
|||	    Mul32(), Mul64(), MulS32_64(), MulU32_64(), MulSF16(), MulUF16(),
|||	    MulSF30(), MulSF16_F32(), MulUF16_F32(), MulSF30_F60(), MulUF30_F60()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/mulmanyf16
|||	MulManyF16 - Multiplies an array of 16.16 values by another array of 16.16
|||	             values.
|||
|||	  Synopsis
|||
|||	    void MulManyF16( frac16 *dest, frac16 *src1, frac16 *src2,
|||	    int32 count )
|||
|||	  Description
|||
|||	    This function multiplies an array of 16.16 fractions by another array of
|||	    16.16 fractions.  Every element of the first array is multiplied by the
|||	    corresponding element in the other array, and the results are deposited in
|||	    the destination array.
|||
|||	  Arguments
|||
|||	    dest                         A pointer to the destination array to store
|||	                                 the results.
|||
|||	    src1                         Pointer to source array of 16.16 fractions
|||	                                 to be multiplied.
|||
|||	    src2                         Pointer to source array of 16.16 fractions
|||	                                 to be multiplied.
|||
|||	    count                        Number of vectors for the multiplication.
|||
|||	  Implementation
|||
|||	    SWI implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    MulScalarF16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/mulmanyvec3mat33divz_f16
|||	MulManyVec3Mat33DivZ_F16 - Multiplies a 3x3 matrix of 16.16 values by 1 or
|||	                           more 3-coordinate vectors of 16.16 values, then
|||	                           multiplies x and y elements of the result vector
|||	                           by ratio of n/z.
|||
|||	  Synopsis
|||
|||	    void MulManyVec3Mat33DivZ_F16( mmv3m33d *s )
|||
|||	  Description
|||
|||	    This function multiplies a 3x3 matrix of 16.16 fractions by one or more
|||	    3-coordinate vectors of 16.16 values.  It then multiplies the x and y
|||	    elements of the result vector {x, y, z} by the ratio of n/z, and deposits
|||	    the result vector {x*n/z, y*n/z, z} in the product vector dest.  This
|||	    function can be used to perform the final projection transformation on
|||	    points just before rendering.
|||
|||	    This function uses the structure mmv3m33d.  Here is its definition:
|||
|||	      // structure to pass arguments to MulManyVec3Mat33DivZ_F16 function
|||	      typedef struct mmv3m3dd{vec3f16*dest;
|||	      vec3f16*src;mat33f16*mat;
|||	      frac16n;
|||	      uint32count;
|||	      }  mmv3m33d;
|||
|||	  Arguments
|||
|||	    s->dest                      A pointer to the destination vector to store
|||	                                 results.
|||
|||	    s->vec                       A pointer to the vector to multiply.
|||
|||	    s->mat                       A pointer to the 3x3 matrix of 16.16
|||	                                 fractions to multiply.
|||
|||	    s->n                         Scale factor to be multiplied by x and y
|||	                                 projected results.
|||
|||	    s->count                     Number of vectors for the multiplication.
|||
|||	  Implementation
|||
|||	    SWI implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    MulManyVec3Mat33_F16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/mulmanyvec3mat33_f16
|||	MulManyVec3Mat33_F16 - Multiplies one or more vectors by a 3x3 matrix of
|||	                       16.16 values.
|||
|||	  Synopsis
|||
|||	    void MulManyVec3Mat33_F16( vec3f16 *dest, vec3f16 *src,
|||	    mat33f16 mat, int32 count )
|||
|||	  Description
|||
|||	    This function multiplies an array of one or more vectors by a 3x3 matrix
|||	    of 16.16 fractions.  The results of the products are deposited in the
|||	    array of vectors pointed to by the dest argument.
|||
|||	  Arguments
|||
|||	    dest                         A pointer to the array of destination
|||	                                 vectors to store the results.
|||
|||	    src                          A pointer to the array of source vectors to
|||	                                 be multiplied with the matrix.
|||
|||	    mat                          A 3x3 matrix of 16.16 fractions.
|||
|||	    count                        The number of vectors for the
|||	                                 multiplication.
|||
|||	  Implementation
|||
|||	    SWI implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    MulManyVec4Mat44_F16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/mulmanyvec4mat44_f16
|||	MulManyVec4Mat44_F16 - Multiplies one or more vectors by a 4x4 matrix of
|||	                       16.16 values.
|||
|||	  Synopsis
|||
|||	    void MulManyVec4Mat44_F16( vec4f16 *dest, vec4f16 *src,
|||	    mat44f16 mat, int32 count )
|||
|||	  Description
|||
|||	    This function multiplies an array of one or more vectors by a 3x3 matrix
|||	    of 16.16 fractions.  The results of the products are deposited in the
|||	    array of vectors pointed to by the dest argument.
|||
|||	  Arguments
|||
|||	    dest                         A pointer to the array of destination
|||	                                 vectors to store the results.
|||
|||	    src                          A pointer to the array of source vectors to
|||	                                 be multiplied with the matrix.
|||
|||	    mat                          A 4x4 matrix of 16.16 fractions.
|||
|||	    count                        The number of vectors for the
|||	                                 multiplication.
|||
|||	  Implementation
|||
|||	    SWI implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    MulManyVec3Mat33_F16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/mulmat33mat33_f16
|||	MulMat33Mat33_F16 - Computes the product of two 3x3 matrices of 16.16
|||	                    values.
|||
|||	  Synopsis
|||
|||	    void MulMat33Mat33_F16( mat33f16 dest, mat33f16 src1,
|||	    mat33f16 src2 )
|||
|||	  Description
|||
|||	    This function multiplies two 3x3 matrices of 16.16 fractions together.
|||	    The results of the product are deposited in the location for the matrix
|||	    dest.  Note that incorrect results may occur if the destination matrix is
|||	    the same matrix as one of the source matrices.
|||
|||	  Arguments
|||
|||	    dest                         A pointer to the destination matrix to store
|||	                                 the results.
|||
|||	    src1, src2                   The source matrices to be multiplied.
|||
|||	  Implementation
|||
|||	    SWI implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    MulVec3Mat33_F16(), MulVec4Mat44_F16(), MulMat44Mat44_F16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/mulmat44mat44_f16
|||	MulMat44Mat44_F16 - Computes the product of two 4x4 matrices of 16.16
|||	                    values.
|||
|||	  Synopsis
|||
|||	    void MulMat44Mat44_F16( mat44f16 dest, mat44f16 src1,
|||	    mat44f16 src2 )
|||
|||	  Description
|||
|||	    This function multiplies two 4x4 matrices of 16.16 fractions together.
|||	    The results of the product are deposited in the matrix dest.  Note that
|||	    incorrect results may occur if the destination matrix is the same matrix
|||	    as one of the source matrices.
|||
|||	  Arguments
|||
|||	    dest                         The destination matrix to store the results.
|||
|||	    src1, src2                   The source matrices to be multiplied
|||	                                 together.
|||
|||	  Implementation
|||
|||	    SWI implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    MulVec3Mat33_F16(), MulVec4Mat44_F16(), MulMat33Mat33_F16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/mulobjectmat33_f16
|||	MulObjectMat33_F16 - Multiplies a matrix within an object structure by an
|||	                     external 3x3 matrix of 16.16 values.
|||
|||	  Synopsis
|||
|||	    void MulObjectMat33_F16( void *objectlist[],
|||	    ObjOffset2 *offsetstruct, mat33f16 mat, int32 count )
|||
|||	  Description
|||
|||	    This function multiplies a matrix within object structures by an external
|||	    3x3 matrix of 16.16 fractions, and repeats over a number of objects.  The
|||	    results of the product are deposited in the destination matrix in each
|||	    object structure pointed to by the objectlist array.
|||
|||	    The object structure argument defines offsets within objects to the
|||	    elements to be manipulated.  The definition for ObjOffset2 is as follows:
|||
|||	    typedef struct ObjOffset2 {
|||	       int32 oo2_DestMatOffset;
|||	       int32 oo2_SrcMatOffset;
|||	       } ObjOffset2;
|||
|||	    * oo2_DestMatOffset is the offset (in bytes) from the beginning of an object
|||	      structure to where to write the result of the matrix multiply.
|||
|||	    * oo2_SrcMatOffset is the offset (in bytes) from the beginning of an object
|||	      structure to the location of the matrix to be multiplied.
|||
|||	  Arguments
|||
|||	    objectlist                   An array of pointers to object structures to
|||	                                 modify.
|||
|||	    offsetstruct                 A pointer to the source object structure
|||	                                 that defines offsets within object to the
|||	                                 elements to be multiplied.
|||
|||	    mat                          A 3x3 matrix of 16.16 fractions to be
|||	                                 multiplied.
|||
|||	    count                        The number of vectors for the
|||	                                 multiplication.
|||
|||	  Implementation
|||
|||	    SWI implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    MulObjectMat44_F16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/mulobjectmat44_f16
|||	MulObjectMat44_F16 - Multiplies a matrix within an object structure by an
|||	                     external 4x4 matrix of 16.16 values.
|||
|||	  Synopsis
|||
|||	    void MulObjectMat44_F16( void *objectlist[],
|||	    ObjOffset2 *offsetstruct, mat44f16 mat, int32 count )
|||
|||	  Description
|||
|||	    This function multiplies a matrix within object structures by an external
|||	    4x4 matrix of 16.16 fractions, and repeats over a number of objects.  The
|||	    results of the product are deposited in the destination matrix in each
|||	    object structure pointed to by the objectlist array.
|||
|||	    The object structure argument defines offsets within objects to the
|||	    elements to be manipulated.  The definition for ObjOffset2 is as follows:
|||
|||	    typedef struct ObjOffset2 {
|||	       int32 oo2_DestMatOffset;
|||	       int32 oo2_SrcMatOffset;
|||	       } ObjOffset2;
|||
|||	    * oo2_DestMatOffset is the offset (in bytes) from the beginning of an object
|||	     structure to where to write the result of the matrix multiply.
|||
|||	    * oo2_SrcMatOffset is the offset (in bytes) from the beginning of an object
|||	     structure to the location of the matrix to be multiplied.
|||
|||	  Arguments
|||
|||	    objectlist                   An array of pointers to object structures to
|||	                                 modify.
|||
|||	    offsetstruct                 A pointer to the source object structure
|||	                                 that defines offsets within object to the
|||	                                 elements to be multiplied.
|||
|||	    mat                          A 4x4 matrix of 16.16 fractions to be
|||	                                 multiplied.
|||
|||	    count                        The number of vectors for the
|||	                                 multiplication.
|||
|||	  Implementation
|||
|||	    SWI implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    MulObjectMat33_F16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/mulobjectvec3mat33_f16
|||	MulObjectVec3Mat33_F16 - Multiplies many vectors within an object
|||	                         structure by a 3x3 matrix of 16.16 values.
|||
|||	  Synopsis
|||
|||	    void MulObjectVec3Mat33_F16( void *objectlist[],
|||	    ObjOffset1 *offsetstruct, int32 count )
|||
|||	  Description
|||
|||	    This function multiplies one or more vectors within object structures by
|||	    a 3x3 matrix of 16.16 fractions also within that structure, and repeats
|||	    over a number of objects.  The results of the product are deposited in the
|||	    destination arrays pointed to by object structures pointed to by the
|||	    objectlist array.
|||
|||	    The object structure argument defines offsets within objects to the
|||	    elements to be manipulated.  The object structure has a pointer to an
|||	    array of points associated with the object, a pointer to an array of
|||	    transformed points, a count of the number of points, and an orientation
|||	    matrix.  The definition for ObjOffset1 is as follows:
|||
|||	    typedef struct ObjOffset1 {
|||	       int32 oo1_DestArrayPtrOffset;
|||	       int32 oo1_SrcArrayPtrOffset;
|||	       int32 oo1_MatOffset;
|||	       int32 oo1_CountOffset;
|||	       } ObjOffset1;
|||
|||	    * oo1_DestArrayPtrOffset is the offset (in bytes) from the beginning of an
|||	      object structure to the location of the pointer to an array of vectors to
|||	      fill with the results of the multiplies.
|||
|||	    * oo1_SrcArrayPtrOffset is the offset (in bytes) from the beginning of an
|||	      object structure to the location of the pointer to an array of vectors to
|||	      be multiplied.
|||
|||	    * oo1_MatOffset is the offset (in bytes) from the beginning of an object
|||	      structure to the location of the matrix to be multiplied.
|||
|||	   * oo1_CountOffset is the offset (in bytes) from the beginning of an object
|||	     structure to the location of the count of the number of vectors to be
|||	     multiplied.
|||
|||	  Arguments
|||
|||	    objectlist                   An array of pointers to object structures to
|||	                                 modify.
|||
|||	    offsetstruct                 A pointer to the source object structure
|||	                                 that defines offsets within object to the
|||	                                 elements to be multiplied.
|||
|||	    count                        The number of vectors for the
|||	                                 multiplication.
|||
|||	  Implementation
|||
|||	    SWI implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    MulObjectVec4Mat44_F16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/mulobjectvec4mat44_f16
|||	MulObjectVec4Mat44_F16 - Multiplies many vectors within an object
|||	                         structure by a 4x4 matrix of 16.16 values.
|||
|||	  Synopsis
|||
|||	    void MulObjectVec4Mat44_F16( void *objectlist[],
|||	    ObjOffset1 *offsetstruct, int32 count )
|||
|||	  Description
|||
|||	    This function multiplies one or more vectors within object structures by
|||	    a 3x3 matrix of 16.16 fractions also within that structure, and repeats
|||	    over a number of objects.  The results of the product are deposited in the
|||	    destination arrays pointed to by object structures pointed to by the
|||	    objectlist array.
|||
|||	    The object structure argument defines offsets within objects to the
|||	    elements to be manipulated.  The object structure has a pointer to an
|||	    array of points associated with the object, a pointer to an array of
|||	    transformed points, a count of the number of points, and an orientation
|||	    matrix.  The definition for ObjOffset1 is as follows:
|||
|||	    typedef struct ObjOffset1 {
|||	         int32 oo1_DestArrayPtrOffset;
|||	         int32 oo1_SrcArrayPtrOffset;
|||	         int32 oo1_MatOffset;
|||	         int32 oo1_CountOffset;
|||	         } ObjOffset1;
|||
|||	    * oo1_DestArrayPtrOffset is the offset (in bytes) from the beginning of an
|||	      object structure to the location of the pointer to an array of vectors to
|||	      fill with the results of the multiplies.
|||
|||	    * oo1_SrcArrayPtrOffset is the offset (in bytes) from the beginning of an
|||	      object structure to the location of the pointer to an array of vectors to
|||	      be multiplied.
|||
|||	    * oo1_MatOffset is the offset (in bytes) from the beginning of an object
|||	      structure to the location of the matrix to be multiplied.
|||
|||	    * oo1_CountOffset is the offset (in bytes) from the beginning of an object
|||	      structure to the location of the count of the number of vectors to be
|||	      multiplied.
|||
|||	  Arguments
|||
|||	    objectlist                   An array of pointers to object structures to
|||	                                 modify.
|||
|||	    offsetstruct                 A pointer to the source object structure
|||	                                 that defines offsets within object to the
|||	                                 elements to be multiplied.
|||
|||	    count                        The number of vectors for the
|||	                                 multiplication.
|||
|||	  Implementation
|||
|||	    SWI implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    MulObjectVec3Mat33_F16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/muls32_64
|||	MulS32_64 - Multiplies two 32-bit integers and returns a 64-bit result.
|||
|||	  Synopsis
|||
|||	    void MulS32_64( int64 *prod, int32 m1, int32 m2 )
|||
|||	  Description
|||
|||	    This function multiplies two signed 32-bit integers together and deposits
|||	    the 64-bit product in the location pointed to by the prod argument.
|||
|||	  Arguments
|||
|||	    prod                         A pointer to the location to store the
|||	                                 64-bit result.
|||
|||	    m1, m2                       Two 32-bit multiplicands.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    Mul32(), Mul64(), MulSF16(), MulU32_64(), MulF14(), MulSF30(), MulUF16(),
|||	    MulSF16_F32(), MulUF16_F32(), MulSF30_F60(), MulUF30_F60()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/mulscalarf16
|||	MulScalarF16 - Multiplies a 16.16 scalar by an array of 16.16 values.
|||
|||	  Synopsis
|||
|||	    void MulScalarF16( frac16 *dest, frac16 *src, frac16 scalar,
|||	    int32 count )
|||
|||	  Description
|||
|||	    This function multiplies a 16.16 scalar by an array of 16.16 fractions.
|||	    Every element of the array of frac16 values is multiplied by the same
|||	    value, and the results are deposited in the destination array.
|||
|||	  Arguments
|||
|||	    dest                         A pointer to the destination array to store
|||	                                 the results.
|||
|||	    src                          A pointer to the source array of 16.16
|||	                                 fractions to be multiplied.
|||
|||	    scalar                       A 16.16 scalar.
|||
|||	    count                        The number of elements in the vector.
|||
|||	  Implementation
|||
|||	    SWI implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    MulScalarF16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/mulsf16
|||	MulSF16 - Multiplies two signed 16.16 fractions.
|||
|||	  Synopsis
|||
|||	    frac16 MulSF16( frac16 m1, frac16 m2 )
|||
|||	  Description
|||
|||	    This function multiplies two signed 16.16 fractions together and returns
|||	    the 16.16 resulting product.  The lower bits of the result are truncated.
|||
|||	  Arguments
|||
|||	    m1, m2                       The multiplicands.
|||
|||	  Return Value
|||
|||	    The function returns the product of the two arguments.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  Caveats
|||
|||	    The function does not detect overflows.
|||
|||	  See Also
|||
|||	    Mul32(), Mul64(), MulS32_64(), MulU32_64(), MulF14(), MulSF30(),
|||	    MulUF16(), MulSF16_F32(), MulUF16_F32(), MulSF30_F60(), MulUF30_F60()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/mulsf16_f32
|||	MulSF16_F32 - Multiplies two signed 16.16 numbers and returns a 32.32
|||	              result.
|||
|||	  Synopsis
|||
|||	    void MulSF16_F32( frac32 *prod, frac16 m1, frac16 m2 )
|||
|||	  Description
|||
|||	    This function multiplies two signed 16.16 numbers together and deposits
|||	    the 32.32 result in the location pointed to by the prod argument.
|||
|||	  Arguments
|||
|||	    prod                         A pointer to the location to store the
|||	                                 32-bit result.
|||
|||	    m1, m2                       Two 16.16 multiplicands.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    Mul32(), Mul64(), MulSF16(), MulU32_64(), MulF14(), MulSF30(), MulUF16(),
|||	    MulS32_64(), MulUF16_F32(), MulSF30_F60(), MulUF30_F60()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/mulsf30
|||	MulSF30 - Multiplies two 2.30 fractions.
|||
|||	  Synopsis
|||
|||	    frac30 MulSF30( frac30 m1, frac30 m2 )
|||
|||	  Description
|||
|||	    This function multiplies two 2.30 fractions together and returns the 2.30
|||	    resulting product.  The lower bits of the result are truncated.
|||
|||	  Arguments
|||
|||	    m1, m2                       The multiplicands.
|||
|||	  Return Value
|||
|||	    The function returns the product of the two arguments.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  Caveats
|||
|||	    The function does not detect overflows.
|||
|||	  See Also
|||
|||	    Mul32(), Mul64(), MulS32_64(), MulU32_64(), MulF14(), MulSF16(),
|||	    MulUF16(), MulSF16_F32(), MulUF16_F32(), MulSF30_F60(), MulUF30_F60()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/mulsf30_f60
|||	MulSF30_F60 - Multiplies two signed 2.30 numbers and returns a 4.60
|||	              result.
|||
|||	  Synopsis
|||
|||	    void MulSF30_F60( frac60 *prod, frac30 m1, frac30 m2 )
|||
|||	  Description
|||
|||	    This function multiplies two signed 2.30 numbers together and deposits
|||	    the 4.60 result in the location pointed to by the prod argument.
|||
|||	  Arguments
|||
|||	    prod                         A pointer to the location to store the 4.60
|||	                                 result.
|||
|||	    m1, m2                       Two 2.30 multiplicands.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    Mul32(), Mul64(), MulSF16(), MulU32_64(), MulF14(), MulSF30(), MulUF16(),
|||	    MulS32_64(), MulUF16_F32(), MulSF16_F32(), MulUF30_F60()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/mulu32_64
|||	MulU32_64 - Multiplies two unsigned 32-bit integers and returns a 64-bit
|||	            result.
|||
|||	  Synopsis
|||
|||	    void MulU32_64( uint64 *prod, uint32 m1, uint32 m2 )
|||
|||	  Description
|||
|||	    This function multiplies two unsigned 32-bit integers together and
|||	    deposits the unsigned 64-bit result in the location pointed to by the prod
|||	    argument.
|||
|||	  Arguments
|||
|||	    prod                         A pointer to the location to store the
|||	                                 64-bit result.
|||
|||	    m1, m2                       Two unsigned 32-bit multiplicands.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    Mul32(), Mul64(), MulSF16(), MulS32_64(), MulF14(), MulSF30(), MulUF16(),
|||	    MulSF16_F32(), MulUF16_F32(), MulSF30_F60(), MulUF30_F60()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/muluf16
|||	MulUF16 - Multiplies two unsigned 16.16 fractions.
|||
|||	  Synopsis
|||
|||	    ufrac16 MulUF16( ufrac16 m1, ufrac16 m2 )
|||
|||	  Description
|||
|||	    This function multiplies two unsigned 16.16 fractions together and
|||	    returns the 16.16 resulting product.  The lower bits of the result are
|||	    truncated.
|||
|||	  Arguments
|||
|||	    m1, m2                       The multiplicands.
|||
|||	  Return Value
|||
|||	    The function returns the product of the two arguments.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  Caveats
|||
|||	    The function does not detect overflows.
|||
|||	  See Also
|||
|||	    Mul32(), Mul64(), MulS32_64(), MulU32_64(), MulF14(), MulSF30(),
|||	    MulSF16(), MulSF16_F32(), MulUF16_F32(), MulSF30_F60(), MulUF30_F60()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/muluf16_f32
|||	MulUF16_F32 - Multiplies two unsigned 16.16 numbers and returns a 32.32
|||	              result.
|||
|||	  Synopsis
|||
|||	    void MulUF16_F32( ufrac32 *prod, ufrac16 m1, ufrac16 m2 )
|||
|||	  Description
|||
|||	    This function multiplies two unsigned 16.16 numbers together and deposits
|||	    the unsigned 32.32 result in the location pointed to by the prod argument.
|||
|||	  Arguments
|||
|||	    prod                         A pointer to the location to store the
|||	                                 result.
|||
|||	    m1, m2                       Two unsigned 16.16 multiplicands.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    Mul32(), Mul64(), MulSF16(), MulU32_64(), MulF14(), MulSF30(), MulUF16(),
|||	    MulS32_64(), MulSF16_F32(), MulSF30_F60(), MulUF30_F60()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/muluf30_f60
|||	MulUF30_F60 - Multiplies two unsigned 2.30 numbers and returns a 4.60
|||	              result.
|||
|||	  Synopsis
|||
|||	    void MulUF30_F60( ufrac60 *prod, ufrac30 m1, ufrac30 m2 )
|||
|||	  Description
|||
|||	    This function multiplies two unsigned 2.30 numbers together and deposits
|||	    the unsigned 4.60 result in the location pointed to by the prod argument.
|||
|||	  Arguments
|||
|||	    prod                         A pointer to the location to store the
|||	                                 unsigned 4.60 result.
|||
|||	    m1, m2                       Two unsigned 2.30 multiplicands.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    Mul32(), Mul64(), MulSF16(), MulU32_64(), MulF14(), MulSF30(), MulUF16(),
|||	    MulS32_64(), MulUF16_F32(), MulSF16_F32(), MulSF30_F60()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/mulvec3mat33divz_f16
|||	MulVec3Mat33DivZ_F16 - Computes the product of a 3x3 matrix and a vector,
|||	                       then multiply the x, y elements of the result by
|||	                       the ratio n/z.
|||
|||	  Synopsis
|||
|||	    void MulVec3Mat33DivZ_F16( vec3f16 dest, vec3f16 vec,
|||	    mat33f16 mat, frac16 n )
|||
|||	  Description
|||
|||	    This function multiplies a 3x3 matrix of 16.16 fractions by a
|||	    3-coordinate vector of 16.16 values, then multiplies the x and y elements
|||	    of the result vector {x, y, z} by the ratio n/z, and deposits the result
|||	    vector {x*n/z, y*n/z, z} in the product vector dest.  This function can
|||	    be used to perform the final projection transformation on points just
|||	    before rendering.
|||
|||	  Arguments
|||
|||	    dest                         The destination vector to store the results.
|||
|||	    vec                          The vector to multiply.
|||
|||	    mat                          The matrix to multiply.
|||
|||	    n                            The scale factor to be multiplied by x and y
|||	                                 projected results.
|||
|||	  Implementation
|||
|||	    SWI implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    MulVec3Mat33_F16(), MulManyVec3Mat33DivZ_F16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/mulvec3mat33_f16
|||	MulVec3Mat33_F16 - Computes the product of a 3x3 matrix and a vector.
|||
|||	  Synopsis
|||
|||	    void MulVec3Mat33_F16( vec3f16 dest, vec3f16 vec, mat33f16
|||	    mat )
|||
|||	  Description
|||
|||	    This function multiplies a 3x3 matrix of 16.16 fractions by a 3
|||	    coordinate vector of 16.16 values and deposits the results of the product
|||	    in the vector dest.
|||
|||	  Arguments
|||
|||	    dest                         The destination vector to store the results.
|||
|||	    vec                          The vector to multiply.
|||
|||	    mat                          The matrix to multiply.
|||
|||	  Implementation
|||
|||	    SWI implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    MulVec4Mat44_F16(), MulMat33Mat33_F16(), MulMat44Mat44_F16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/mulvec4mat44_f16
|||	MulVec4Mat44_F16 - Computes the product of a 4x4 matrix and a vector.
|||
|||	  Synopsis
|||
|||	    void MulVec4Mat44_F16( vec4f16 dest, vec4f16 vec, mat44f16
|||	    mat )
|||
|||	  Description
|||
|||	    This function multiplies a 4x4 matrix of 16.16 fractions by a
|||	    4-coordinate vector of 16.16 values and deposits the results of the
|||	    product in the vector dest.
|||
|||	  Arguments
|||
|||	    dest                         The destination vector to store the results.
|||
|||	    vec                          The vector to multiply.
|||
|||	    mat                          The matrix to multiply.
|||
|||	  Implementation
|||
|||	    SWI implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    MulVec3Mat33_F16(), MulMat33Mat33_F16(), MulMat44Mat44_F16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/neg32
|||	Neg32 - Computes the two's complement of a 32-bit integer.
|||
|||	  Synopsis
|||
|||	    int32 Neg32( int32 x )
|||
|||	  Description
|||
|||	    This macro returns the two's complement of a 32-bit integer.
|||
|||	  Arguments
|||
|||	    x                            A 32-bit integer for which to get the
|||	                                 two's complement.
|||
|||	  Return Value
|||
|||	    The function returns the two's complement of x.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    NegF16(), NegF30(), NegF14(), Neg64(), NegF32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/neg64
|||	Neg64 - Computes the two's complement of a 64-bit integer.
|||
|||	  Synopsis
|||
|||	    void Neg64( int64 *dest, int64 *src )
|||
|||	  Description
|||
|||	    This function computes the two's complement of a 64-bit integer.  The
|||	    two's complement of the location pointed to by src is deposited in the
|||	    location pointed to by dest.
|||
|||	  Arguments
|||
|||	    dest                         A pointer to the location to store the
|||	                                 64-bit integer result.
|||
|||	    src                          A pointer to the location of the 64-bit
|||	                                 integer to be acted upon.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    NegF16(), NegF14(), Neg32(), NegF30(), NegF32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/negf14
|||	NegF14 - Computes the two's complement of a 2.14 fraction.
|||
|||	  Synopsis
|||
|||	    frac14 NegF14( frac14 x )
|||
|||	  Description
|||
|||	    This macro returns the two's complement of a 2.14 fraction.
|||
|||	  Arguments
|||
|||	    x                            A 2.14 fraction for which the two's
|||	                                 complement is desired.
|||
|||	  Return Value
|||
|||	    The function returns the two's complement of x.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    NegF16(), NegF30(), Neg32(), Neg64(), NegF32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/negf16
|||	NegF16 - Computes the two's complement of a 16.16 fraction.
|||
|||	  Synopsis
|||
|||	    frac16 NegF16( frac16 x )
|||
|||	  Description
|||
|||	    This macro returns the two's complement of a 16.16 fraction.
|||
|||	  Arguments
|||
|||	    x                            A 16.16 fraction for which the two's
|||	                                 complement is desired.
|||
|||	  Return Value
|||
|||	    The function returns the two's complement of x.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    NegF14(), NegF30(), Neg32(), Neg64(), NegF32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/negf30
|||	NegF30 - Computes the twos complement of a 2.30 fraction.
|||
|||	  Synopsis
|||
|||	    frac30 NegF30( frac30 x )
|||
|||	  Description
|||
|||	    This function returns the two's complement of a 2.30 fraction.
|||
|||	  Arguments
|||
|||	    x                            A 2.30 fraction for which the two's
|||	                                 complement is desired.
|||
|||	  Return Value
|||
|||	    The function returns the two's complement of x.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    NegF16(), NegF14(), Neg32(), Neg64(), NegF32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/negf32
|||	NegF32 - Computes the two's complement of a 32.32 fraction.
|||
|||	  Synopsis
|||
|||	    void NegF32( frac32 *dest, frac32 *src )
|||
|||	  Description
|||
|||	    This function returns the two's complement of a 32.32 fraction.
|||
|||	  Arguments
|||
|||	    dest                         A pointer to the location to store the
|||	                                 frac32 result.
|||
|||	    src                          A pointer to the location of the 32.32
|||	                                 fraction to be acted upon.
|||
|||	  Return Value
|||
|||	    The function returns the two's complement of x.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    NegF16(), NegF14(), Neg32(), Neg64(), NegF30()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/negf60
|||	NegF60 - Computes the two's complement of a 4.60 fraction.
|||
|||	  Synopsis
|||
|||	    void NegF60( frac60 *dest, frac60 *src )
|||
|||	  Description
|||
|||	    This function returns the two's complement of a 4.60 fraction.
|||
|||	  Arguments
|||
|||	    dest                         A pointer to the location to store the 4.60
|||	                                 fraction result.
|||
|||	    src                          A pointer to the location of the 4.60
|||	                                 fraction to be acted upon.
|||
|||	  Return Value
|||
|||	    The function returns the two's complement of x.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    NegF16(), NegF14(), Neg32(), Neg64(), NegF30(), NegF32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/not32
|||	Not32 - Computes one's complement of a 32-bit integer.
|||
|||	  Synopsis
|||
|||	    int32 Not32( int32 x )
|||
|||	  Description
|||
|||	    This macro returns the one's complement of a 32-bit integer.
|||
|||	  Arguments
|||
|||	    x                            A 32-bit integer for which the one's
|||	                                 complement is desired.
|||
|||	  Return Value
|||
|||	    The function returns the one's complement of x.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    NotF16(), NotF30(), NotF14(), Not64(), NotF32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/not64
|||	Not64 - Computes one's complement of a 64-bit integer.
|||
|||	  Synopsis
|||
|||	    void Not64( int64 *dest, int64 *src )
|||
|||	  Description
|||
|||	    This function computes the one's complement of a 64-bit integer in
|||	    the location pointed to by src.  The result is deposited in the location
|||	    pointed to by dest.
|||
|||	  Arguments
|||
|||	    dest                         A pointer to the location to store the
|||	                                 64-bit integer result.
|||
|||	    src                          A pointer to the location of the 64-bit
|||	                                 integer to be acted upon.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    NotF16(), NotF14(), Not32(), NotF30(), NotF32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/notf14
|||	NotF14 - Computes one's complement of a 2.14 fraction.
|||
|||	  Synopsis
|||
|||	    frac14 NotF14( frac14 x )
|||
|||	  Description
|||
|||	    This macro returns the one's complement of a 2.14 fraction.
|||
|||	  Arguments
|||
|||	    x                            A 2.14 fraction for which the one's
|||	                                 complement is desired.
|||
|||	  Return Value
|||
|||	    The function returns the one's complement of x.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    NotF16(), NotF30(), Not32(), Not64(), NotF32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/notf16
|||	NotF16 - Computes one's complement of a 16.16 fraction.
|||
|||	  Synopsis
|||
|||	    frac16 NotF16( frac16 x )
|||
|||	  Description
|||
|||	    This macro returns the one's complement of a 16.16 fraction.
|||
|||	  Arguments
|||
|||	    x                            A 16.16 fraction for which the one's
|||	                                 complement is desired.
|||
|||	  Return Value
|||
|||	    The function returns the one's complement of x.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    NotF14(), NotF30(), Not32(), Not64(), NotF32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/notf30
|||	NotF30 - Computes one's complement of a 2.30 fraction.
|||
|||	  Synopsis
|||
|||	    frac30 NotF30( frac30 x )
|||
|||	  Description
|||
|||	    This macro returns the one's complement of a 2.30 fraction.
|||
|||	  Arguments
|||
|||	    x                            A 2.30 fraction for which the one's
|||	                                 complement is desired.
|||
|||	  Return Value
|||
|||	    The function returns the one's complement of x.
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    NotF16(), NotF14(), Not32(), Not64(), NotF32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/notf32
|||	NotF32 - Computes one's complement of a 32.32 number.
|||
|||	  Synopsis
|||
|||	    void NotF32( frac32 *dest, frac32 *src )
|||
|||	  Description
|||
|||	    This function computes the one's complement of a 32.32 number in the
|||	    location pointed to by src.  The result is deposited in the location
|||	    pointed to by dest.
|||
|||	  Arguments
|||
|||	    dest                         A pointer to the location to store the 32.32
|||	                                 number result.
|||
|||	    src                          A pointer to the location of the 32.32
|||	                                 number to be acted upon.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    NotF16(), NotF14(), Not32(), NotF30(), NotF60(), Not64()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/notf60
|||	NotF60 - Computes one's complement of a 4.60 number.
|||
|||	  Synopsis
|||
|||	    void NotF60( frac60 *dest, frac60 *src )
|||
|||	  Description
|||
|||	    This function computes the one's complement of a 4.60 number in the
|||	    location pointed to by src.  The result is deposited in the location
|||	    pointed to by dest.
|||
|||	  Arguments
|||
|||	    dest                         A pointer to the location to store the 4.60
|||	                                 number result.
|||
|||	    src                          A pointer to the location of the 4.60 number
|||	                                 to be acted upon.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    NotF16(), NotF14(), Not32(), NotF30(), Not64(), NotF32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/openmathfolio
|||	OpenMathFolio - Opens the math folio and set _MathBase global variable.
|||
|||	  Synopsis
|||
|||	    int32 OpenMathFolio( void )
|||
|||	  Description
|||
|||	    This function attempts to open the math folio structure.  If it
|||	    successfully locates the math folio, it sets the global variable _MathBase
|||	    to point to the math folio structure.  If it fails to open the folio, it
|||	    returns a negative number that can be printed with the PrintfSysError
|||	     function.
|||
|||	    Programs must call OpenMathFolio before attempting to use any of the math
|||	    folio functions.
|||
|||	  Return Value
|||
|||	    If successful, the function sets the global variable _MathBase to point
|||	    to the math folio structure.  If unsuccessful, it returns a negative
|||	    number.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in operamath.lib V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/recipsf16
|||	RecipSF16 - Computes the reciprocal of a signed 16.16 number.
|||
|||	  Synopsis
|||
|||	    frac16 RecipSF16( frac16 d )
|||
|||	  Description
|||
|||	    This function computes the reciprocal of a signed 16.16 number and
|||	    returns the 16.16 result.  An overflow is signaled by all bits set in the
|||	    return value.
|||
|||	  Arguments
|||
|||	    d                            16.16 format fractions.
|||
|||	  Return Value
|||
|||	    The function returns the reciprocal number.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    RecipUF16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/recipuf16
|||	RecipUF16 - Computes the reciprocal of an unsigned 16.16 number.
|||
|||	  Synopsis
|||
|||	    ufrac16 RecipUF16( ufrac16 d )
|||
|||	  Description
|||
|||	    This function computes the reciprocal of an unsigned 16.16 number and
|||	    returns the 16.16 result.  An overflow is signaled by all bits set in the
|||	    return value.
|||
|||	  Arguments
|||
|||	    d                            16.16 format fractions.
|||
|||	  Return Value
|||
|||	    The function returns the reciprocal number.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    RecipSF16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/sinf16
|||	SinF16 - Computes the 16.16 sine of a 16.16 fraction angle.
|||
|||	  Synopsis
|||
|||	    frac16 SinF16( frac16 x )
|||
|||	  Description
|||
|||	    This function returns the 16.16 sine of a 16.16 fraction angle.  In
|||	    operamath coordinates, there are 256.0 units in a circle.
|||
|||	  Arguments
|||
|||	    x                            A 16.16 fraction describing the angle of the
|||	                                 circle.
|||
|||	  Return Value
|||
|||	    The value returned is the sine of the input angle.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    CosF16(), CosF32(), SinF32(), CosF30(), SinF30()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/sinf30
|||	SinF30 - Computes the operamath 2.30 sine of a 16.16 fraction.
|||
|||	  Synopsis
|||
|||	    frac30 SinF30( frac16 x )
|||
|||	  Description
|||
|||	    This function returns the operamath 2.30 sine of a 16.16 fraction.  In
|||	    operamath coordinates, there are 256.0 units in a circle.
|||
|||	  Arguments
|||
|||	    x                            A 16.16 fraction describing the angle of the
|||	                                 circle.
|||
|||	  Return Value
|||
|||	    The function returns the 2.30 sine of the input angle.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    SinF16(), CosF16(), SinF32(), CosF32(), CosF30()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/sinf32
|||	SinF32 - Computes the operamath 32.32 sine of a 16.16 fraction.
|||
|||	  Synopsis
|||
|||	    void SinF32( frac32 *dest, frac16 x )
|||
|||	  Description
|||
|||	    This function returns the operamath 32.32 sine of a 16.16 fraction.  In
|||	    operamath coordinates, there are 256.0 units in a circle.
|||
|||	  Arguments
|||
|||	    dest                         A pointer to a 32.32 fraction containing the
|||	                                 result.
|||
|||	    x                            A 16.16 fraction describing the angle of the
|||	                                 circle.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    SinF16(), CosF16(), CosF32(), CosF30(), SinF30()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/sqrt32
|||	Sqrt32 - Computes square root of an unsigned 32-bit number.
|||
|||	  Synopsis
|||
|||	    uint32 Sqrt32( uint32 x )
|||
|||	  Description
|||
|||	    This function computes the positive square root of an unsigned 32-bit
|||	    number.
|||
|||	  Arguments
|||
|||	    x                            The number for which the square root is
|||	                                 desired.
|||
|||	  Return Value
|||
|||	    The function returns the square root of x.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    SqrtF16(), Sqrt64_32(), SqrtF32_F16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/sqrt64_32
|||	Sqrt64_32 - Computes the 32-bit square root of an unsigned 64-bit integer.
|||
|||	  Synopsis
|||
|||	    uint32 Sqrt64_32( uint64 *x )
|||
|||	  Description
|||
|||	    This function computes the 32-bit square root of an unsigned 64-bit
|||	    integer.
|||
|||	  Arguments
|||
|||	    x                            A pointer to the 64  bit number for which
|||	                                 the square root is desired.
|||
|||	  Return Value
|||
|||	    The function returns the 32-bit square root of x.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    Sqrt32(), SqrtF32_F16(), SqrtF32_F16(), SqrtF60_F30()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/sqrtf16
|||	SqrtF16 - Computes the square root of an unsigned 16.16 number.
|||
|||	  Synopsis
|||
|||	    ufrac16 SqrtF16( ufrac16 x )
|||
|||	  Description
|||
|||	    This function computes the positive square root of an unsigned 16.16
|||	    number.
|||
|||	  Arguments
|||
|||	    x                            The number for which the square root is
|||	                                 desired.
|||
|||	  Return Value
|||
|||	    The function returns the square root of x.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    Sqrt32(), Sqrt64_32(), SqrtF32_F16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/sqrtf32_f16
|||	SqrtF32_F16 - Computes the square root of a 32.32 fraction.
|||
|||	  Synopsis
|||
|||	    ufrac16 SqrtF32_F16( ufrac32 *x )
|||
|||	  Description
|||
|||	    This function computes the 16.16 fraction square root of a 32.32
|||	    fraction.
|||
|||	  Arguments
|||
|||	    x                            A pointer to the 32.32 fraction for which
|||	                                 the square root is desired.
|||
|||	  Return Value
|||
|||	    The function returns the 16.16 fraction square root of x.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    Sqrt32(), Sqrt64_32(), SqrtF60_F30()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/sqrtf60_f30
|||	SqrtF60_F30 - Computes the square root of a 4.60 fraction as a 2.30
|||	              fraction.
|||
|||	  Synopsis
|||
|||	    frac30 SqrtF60_F30( frac60 *x )
|||
|||	  Description
|||
|||	    This function computes the 2.30 fraction square root of a 4.60 fraction.
|||
|||	  Arguments
|||
|||	    x                            A pointer to the 4.60 fraction for which the
|||	                                 square root is desired.
|||
|||	  Return Value
|||
|||	    The function returns the 2.30 fraction square root of x.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    Sqrt32(), Sqrt64_32(), SqrtF32_F16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/square64
|||	Square64 - Squares a 64-bit integer.
|||
|||	  Synopsis
|||
|||	    void Square64( uint64 *p, int64 *m )
|||
|||	  Description
|||
|||	    This function computes the square of a 64-bit integer in the location
|||	    pointed to by m.  The result is deposited in the location pointed to by p.
|||
|||	  Arguments
|||
|||	    p                            A pointer to the result of the square
|||	                                 operation.
|||
|||	    m                            A pointer to the location of the 64-bit
|||	                                 integer to be squared.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  Caveats
|||
|||	    The function does not detect overflows.
|||
|||	  See Also
|||
|||	    SquareUF16(), SquareSF16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/squaresf16
|||	SquareSF16 - Squares a signed 16.16 fraction.
|||
|||	  Synopsis
|||
|||	    ufrac16 SquareSF16( frac16 m )
|||
|||	  Description
|||
|||	    This function computes the square of a 16.16 number.  The lower bits of
|||	    the result are truncated.
|||
|||	  Arguments
|||
|||	    m                            The number to be squared.
|||
|||	  Return Value
|||
|||	    The function returns the square of m.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  Caveats
|||
|||	    The function does not detect overflows.
|||
|||	  See Also
|||
|||	    SquareUF16(), Square64()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/squareuf16
|||	SquareUF16 - Squares an unsigned 16.16 fraction.
|||
|||	  Synopsis
|||
|||	    ufrac16 SquareUF16( ufrac16 m )
|||
|||	  Description
|||
|||	    This function computes the square of an unsigned 16.16 number.  The lower
|||	    bits of the result are truncated.
|||
|||	  Arguments
|||
|||	    m                            The number to be squared.
|||
|||	  Return Value
|||
|||	    The function returns the square of m.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  Caveats
|||
|||	    The function does not detect overflows.
|||
|||	  See Also
|||
|||	    SquareSF16(), Square64()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/sub32
|||	Sub32 - Subtracts a 32-bit integer from another.
|||
|||	  Synopsis
|||
|||	    int32 Sub32( int32 x, int32 y )
|||
|||	  Description
|||
|||	    This macro returns the difference between two 32-bit integers.
|||
|||	  Arguments
|||
|||	    x, y                         Two 32-bit integers for the subtraction.  y
|||	                                 is subtracted from x.
|||
|||	  Return Value
|||
|||	    The macro returns the value (x - y).
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    SubF32(), SubF60(), SubF16(), SubF30(), SubF14(), Sub64()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/sub64
|||	Sub64 - Subtracts a 64-bit integer from another.
|||
|||	  Synopsis
|||
|||	    void Sub64( int64 *r, int64 *s1, int64 *s2 )
|||
|||	  Description
|||
|||	    This function subtracts one 64-bit integer from another and deposits the
|||	    64-bit result in the location pointed to by r.
|||
|||	  Arguments
|||
|||	    r                            A pointer to the 64-bit result of the
|||	                                 subtraction.
|||
|||	    s1, s2                       Pointers to the two 64-bit numbers for the
|||	                                 subtraction.  s2 is subtracted from s1.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    SubF32(), SubF60(), SubF16(), SubF30(), SubF14(), Sub32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/subf14
|||	SubF14 - Subtracts a 2.14 fraction from another.
|||
|||	  Synopsis
|||
|||	    frac14 SubF14( frac14 x, frac14 y )
|||
|||	  Description
|||
|||	    This macro returns the difference between two 2.14 fractions.
|||
|||	  Arguments
|||
|||	    x, y                         Two 2.14 fractions for the subtraction.  y
|||	                                 is subtracted from x.
|||
|||	  Return Value
|||
|||	    The macro returns the the value (x - y).
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    SubF32(), SubF60(), Sub32(), SubF30(), SubF16(), Sub64()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/subf16
|||	SubF16 - Subtracts a 16.16 fraction from another.
|||
|||	  Synopsis
|||
|||	    frac16 SubF16( frac16 x, frac16 y )
|||
|||	  Description
|||
|||	    This macro returns the difference between two 16.16 fractions.
|||
|||	  Arguments
|||
|||	    x, y                         Two 16.16 fractions for the subtraction.  y
|||	                                 is subtracted from x.
|||
|||	  Return Value
|||
|||	    The macro returns the value (x - y).
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    SubF32(), SubF60(), Sub32(), SubF30(), SubF14(), Sub64()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/subf30
|||	SubF30 - Subtracts a 2.30 fraction from another.
|||
|||	  Synopsis
|||
|||	    frac30 SubF30( frac30 x, frac30 y )
|||
|||	  Description
|||
|||	    This macro returns the difference between two 2.30 fractions.
|||
|||	  Arguments
|||
|||	    x, y                         Two 2.30 fractions for the subtraction.  y
|||	                                 is subtracted from x.
|||
|||	  Return Value
|||
|||	    The macro returns the value (x - y).
|||
|||	  Implementation
|||
|||	    Macro implemented in operamath.h V20.
|||
|||	  Associated Files
|||
|||	    operamath.h
|||
|||	  See Also
|||
|||	    SubF32(), SubF60(), Sub32(), SubF16(), SubF14(), Sub64()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/subf32
|||	SubF32 - Subtracts a 32.32 fraction from another.
|||
|||	  Synopsis
|||
|||	    void SubF32( frac32 *r, frac32 *s1, frac32 *s2 )
|||
|||	  Description
|||
|||	    This function subtracts two 32.32 fractions and deposits the result in
|||	    the location pointed to by r.
|||
|||	  Arguments
|||
|||	    r                            A pointer to the 32.32 fraction result of
|||	                                 the subtraction.
|||
|||	    s1, s2                       Pointers to the two 32.32 fractions for the
|||	                                 subtraction.  s2 is subtracted from s1.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    Sub64(), SubF60(), SubF16(), SubF30(), SubF14(), Sub32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/subf60
|||	SubF60 - Subtracts a 4.60 fraction from another.
|||
|||	  Synopsis
|||
|||	    void SubF60( frac60 *r, frac60 *s1, frac60 *s2 )
|||
|||	  Description
|||
|||	    This function subtracts two 4.60 fractions and deposits the result in the
|||	    location pointed to by r.
|||
|||	  Arguments
|||
|||	    r                            A pointer to the 4.60 fraction result of the
|||	                                 subtraction.
|||
|||	    s1, s2                       Pointers to the two 4.60 fractions for the
|||	                                 subtraction.  s2 is subtracted from s1.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    Sub64(), SubF32(), SubF16(), SubF30(), SubF14(), Sub32()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/transpose33_f16
|||	Transpose33_F16 - Transposes a 3x3 matrix of 16.16 values.
|||
|||	  Synopsis
|||
|||	    void Transpose33_F16( mat33f16 dest, mat33f16 src )
|||
|||	  Description
|||
|||	    This function transposes a 3-by-3 matrix of 16.16 values and deposits the
|||	    result in the matrix dest.
|||
|||	  Arguments
|||
|||	    dest                         Pointer to destination matrix to store the
|||	                                 result.
|||
|||	    src                          The matrix to transpose.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    Transpose44_F16()
|||
**/

/**
|||	AUTODOC PUBLIC spg/math/transpose44_f16
|||	Transpose44_F16 - Transposes a 4x4 matrix of 16.16 values.
|||
|||	  Synopsis
|||
|||	    void Transpose44_F16( mat44f16 dest, mat44f16 src )
|||
|||	  Description
|||
|||	    This function transposes a 4-by-4 matrix of 16.16 values and deposits the
|||	    result in the matrix dest.
|||
|||	  Arguments
|||
|||	    dest                         Pointer to destination matrix to store the
|||	                                 result.
|||
|||	    src                          The matrix to transpose.
|||
|||	  Implementation
|||
|||	    Folio call implemented in operamath V20.
|||
|||	  Associated Files
|||
|||	    operamath.h, operamath.lib
|||
|||	  See Also
|||
|||	    Transpose33_F16()
|||
**/

/* keep the compiler happy... */
extern int foo;
