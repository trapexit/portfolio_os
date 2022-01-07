#ifndef _CSOUND_H_
#define _CSOUND_H_

/******************************************************************************
**
**  $Id: csound.h,v 1.2 1994/11/23 23:45:04 vertex Exp $
**
**  Simple sound class.
**
******************************************************************************/

#include "music.h"
#include "soundfile.h"

#define NUMBLOCKS 		16
#define BLOCKSIZE 		2048
#define BUFSIZE 		(NUMBLOCKS * BLOCKSIZE)

class CSound
{
	public:
		CSound (char *pSndFileName, long vol = 0x1000);
		~CSound (void);

		long GetVolume(void);
		void Loop(long nTimes);
		void Play(void);
		void SetVolume(long vol);
		void Stop(void);
		void Rewind(void);

	private:
		SoundFilePlayer  *fSndFile;
		long fVolume;
};

#endif
