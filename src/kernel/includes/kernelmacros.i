

;*****

; $Id: kernelmacros.i,v 1.4 1994/09/10 01:43:35 vertex Exp $

; $Log: kernelmacros.i,v $
;Revision 1.4  1994/09/10  01:43:35  vertex
;Prepared for automatic labelling and release numbering.
;
;Revision 1.3  1992/10/24  01:37:21  dale
;fix id
;

;*****/
; file: KernelMacros.i

	MACRO
	DISABLE
	mrs	r12,CPSR	; get current bits
;	orr	r12,r12,#&40	; or FIRQ disable
	orr	r12,r12,#&c0	; or FIRQ disable
	msr	CPSR_ctl,r12	; now disabled
	MEND

	END
