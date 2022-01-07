#ifndef _CAPPLICATION_H_
#define _CAPPLICATION_H_

/******************************************************************************
**
**  $Id: capplication.h,v 1.3 1994/11/24 00:02:34 vertex Exp $
**
**  Simple application class.
**
******************************************************************************/

#define		kControlAll	( ControlStart | ControlX | ControlUp | ControlDown | ControlLeft | ControlRight | ControlA | ControlB | ControlC )

class CApplication
{
	public:
		CApplication ( void );
		~CApplication ( void );

		virtual void Run( void );
		virtual void Stop( void );

	protected:
		virtual uint32 DoControlPad( uint32 continuousMask );

		virtual void OnStart ( void ) {};
		virtual void OnX ( void ) {};

		virtual void OnUp ( void ) {};
		virtual void OnDown ( void ) {};
		virtual void OnLeft ( void ) {};
		virtual void OnRight ( void ) {};

		virtual void OnA ( void ) {};
		virtual void OnB ( void ) {};
		virtual void OnC ( void ) {};

	private:
		long 	fContinue;
};

#endif
