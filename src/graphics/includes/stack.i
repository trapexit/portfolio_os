; $Id: stack.i,v 1.2 1994/02/09 02:04:35 limes Exp $
;---------------------------------------------------
;
;  Stack type macros.
;
;---------------------------------------------------

	MACRO	
$label		SWPREG	$reg1,$reg2
$label		eor	$reg1,$reg1,$reg2
		eor	$reg2,$reg1,$reg2
		eor	$reg1,$reg1,$reg2
	MEND

	MACRO
$label		ldfar	$reg,$addr
$label		ADRL	$reg,$addr
		ldr	$reg,[$reg]
	MEND

	MACRO
$label		stfar	$reg,$reg2,$addr
$label		ADRL	$reg2,$addr
		str	$reg,[$reg2]
	MEND

	MACRO
$label		ENTER	$expr
$label
		mov	ip,sp
	IF	"$expr"=""
		stmdb	sp!,{fp,ip,lr,pc}
	ELSE
		stmdb	sp!,{$expr,fp,ip,lr,pc}
	ENDIF
		sub	fp,ip,#4
	MEND

	MACRO
$label		EXIT	$expr
$label
	IF	"$expr"=""
		ldmdb	fp,{fp,sp,pc}
	ELSE
		ldmdb	fp,{$expr,fp,sp,pc}
	ENDIF
	MEND

	MACRO
$label		PUSH	$expr
		stmfd	sp!,{$expr}
	MEND

	MACRO
$label		POP	$expr
		ldmfd	sp!,{$expr}
	MEND

	END
