*\  :ts=8 bk=0
*
* supervectors.s:	Glue routines for Super#?() routines within graphics.
*
* Leo L. Schwab						9408.02
*
* $Id: supervectors.s,v 1.1 1994/08/25 23:03:12 ewhac Exp $
*/
		IMPORT	|SuperGrafDo|

****************************************************************************
* Stub routines to route to internal vectors when already in supervisor
* mode.
*
		AREA	|STUBSuperInternalGetVBLAttrs|, CODE, READONLY

		EXPORT	SuperInternalGetVBLAttrs
SuperInternalGetVBLAttrs
		mov	r12, #-1	; These really should be symbolic.
		b	SuperGrafDo


		AREA	|STUBSuperGetVBLAttrs|, CODE, READONLY

		EXPORT	SuperGetVBLAttrs
SuperGetVBLAttrs
		mov	r12, #53
		b	SuperGrafDo


		AREA	|STUBSuperSetVBLAttrs|, CODE, READONLY

		EXPORT	SuperSetVBLAttrs
SuperSetVBLAttrs
		mov	r12, #52
		b	SuperGrafDo


		END
