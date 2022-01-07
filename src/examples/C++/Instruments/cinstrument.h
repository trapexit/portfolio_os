#ifndef _CINSTRUMENT_H_
#define _CINSTRUMENT_H_

/******************************************************************************
**
**  $Id: cinstrument.h,v 1.3 1994/11/23 23:51:23 vertex Exp $
**
**  Simple instrument class.
**
******************************************************************************/

#include "portfolio.h"
#include "utils3do.h"

class CInstrument
{
	public:
		CInstrument (char *pSndFileName);
		CInstrument (char *pSndFileName, long rate, long vol);
		~CInstrument ( void );

		long GetChannel( void );
		long GetRate( void );
		long GetVolume( void );
		Boolean IsPaused( void );
		Boolean IsPlaying( void );
		Boolean IsValid( void );
		void Pause( void );
		void Play( void );
		void Reset( void );
		void Resume( void );
		void SetChannel(long chn);
		void SetRate(long rate);
		void SetVolume(long vol);
		void Stop( void );

	private:
		Item fSndItem;
		TagArg fSndTag[3];     // 0 - rate, 1 - ampl
		long fChannel;
		Boolean fIsPlaying;
		Boolean fIsPaused;
};

inline long CInstrument::GetChannel( void )
{
	return fChannel;
}

inline long CInstrument::GetRate( void )
{
	return (long)fSndTag[0].ta_Arg;
}

inline long CInstrument::GetVolume( void )
{
	return (long)fSndTag[1].ta_Arg;
}

inline Boolean CInstrument::IsValid( void )
{
	return fSndItem != 0L;
}

inline Boolean CInstrument::IsPaused( void )
{
	return (fIsPaused != 0L);
}

inline Boolean CInstrument::IsPlaying( void )
{
	return (fIsPlaying != 0L && fSndItem);
}

#endif
