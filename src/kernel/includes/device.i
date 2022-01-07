 IF	:DEF:|_DEVICE_I|
 ELSE
	GBLL	|_DEVICE_I|

;*****************************************************************************
;*
;*  $Id: device.i,v 1.10 1994/10/12 18:52:13 vertex Exp $
;*
;*  Kernel device management definitions
;*
;*****************************************************************************

	INCLUDE item.i
	INCLUDE structs.i
	INCLUDE nodes.i
	INCLUDE list.i

	BEGINSTRUCT	Device
		STRUCT	ItemNode,dev_dev
		PTR	dev_Driver
		INT32	dev_OpenCnt
		ARRAY	UINT32,dev_Private0,13
		UINT8	dev_MaxUnitNum
		ARRAY	UINT8,dev_Private1,3
		ARRAY	UINT32,dev_Private2,3
	ENDSTRUCT

 ENDIF	; |_DEVICES_I|

	END
