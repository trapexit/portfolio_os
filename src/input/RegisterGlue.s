	;; $Id: RegisterGlue.s,v 1.3 1994/02/09 01:27:17 limes Exp $
;	c initial startup code for tasks

	AREA	|ASMCODE|, CODE, READONLY
;	AREA	|C$$code|, CODE, READONLY

	EXPORT	SaveGlue
	EXPORT	RestoreGlue

SaveGlue
	stmfd	sp!,{r0-r12,r14}

RestoreGlue
	ldmfd	sp!,{r0-r12,pc}

	END
