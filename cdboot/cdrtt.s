
	AREA |ASMCODE|, CODE, READONLY
;       hack in a romtag table
;
RT_ENTRIES	EQU 4
RT_SIZE		EQU RT_ENTRIES*32
;
rt_table
;	dipir boot strap
	DCB     &0f                  ; subsystem
	DCB	&0d                  ; type
	DCB	&02                  ; version
	DCB	&00                  ; revision
	DCD	&00000000            ; flags etc.
	DCD     1	             ; offset (blocks)
	DCD     DIPIRSIZE            ; size
	DCD     0,0,0,0

;	new os
	DCB     &0f                  ; subsystem
	DCB	&07                  ; type
	DCB	OSVER                ; version
	DCB	OSREV                ; revision
	DCD	&00000000            ; flags etc.
	DCD     5                    ; offset (blocks)
	DCD     CDBOOTSIZE           ; size
	DCD     0,0,0,0

;	new misc
	DCB     &0f                  ; subsystem
	DCB	&10                  ; type
	DCB	&00                  ; version
	DCB	&10                  ; revision
	DCD	&00000000            ; flags etc.
	DCD     69                   ; offset (blocks)
	DCD     MISCSIZE             ; size
	DCD     0,0,0,0

;	app banner screen
	DCB     &0f                  ; subsystem
	DCB	&14                  ; type
	DCB	&00                  ; version
	DCB	&00                  ; revision
	DCD	&00000000            ; flags etc.
	DCD     226                  ; offset (blocks)
	DCD     BANNERSIZE           ; size
	DCD     0,0,0,0

;	dev permission bits
	DCB     &0f                  ; subsystem
	DCB	&17                  ; type
	DCB	&00                  ; version
	DCB	&00                  ; revision
	DCD	&00000000            ; flags etc.
	DCD     &0                   ; offset (blocks)
	DCD     &0                   ; size
	DCD     &0ffffffff           ; permitted devices (all)
	DCD	0,0,0


	DCD     0,0,0,0,0,0,0,0      ; end
myend

	END
