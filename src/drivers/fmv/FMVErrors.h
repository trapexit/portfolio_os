/* file: FMVErrors.h */
/* FMV error code definitions */
/* 4/8/93 George Mitsuoka */
/* The 3DO Company Copyright © 1993 */

#ifndef FMVERRORS_HEADER
#define FMVERRORS_HEADER

typedef
	enum
	{
		kNoErr = 0,
		kErrROMAccessFailed,
		kErrAllocateROMMemoryFailed,
		kErr450ROMBootCodeNotFound,
		kErrBadBootCodeAddress,
		kErr450ROMCodeNotFound,
		kErrOpenROMFileFailed,
		kErrReadROMFileFailed,
		kErrFMVCL450CommandTimeout
	}
	FMVError;
	
#endif
