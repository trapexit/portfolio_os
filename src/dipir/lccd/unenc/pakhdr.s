; DO NOT EDIT THIS FILE
; This file was generated by ./make_pakhdr.sh on Thu Mar  2 20:43:51 PST 1995.
; This is the header for a dipir driver package.

 AREA |ASMCODE|, CODE, READONLY
 DCD &925fbcd0 ; Magic number for PACK file (PACK_MAGIC)
 DCD 6 ; number of drivers
; -- lccd/unenc/icddipir --
 DCW &1000 ; manufacturer id
 DCW &F000 ; manufacturer device number
 DCD 80 ; offset of start of driver
 DCD 6320 ; size of driver
; -- meidev.drv --
 DCW &0010 ; manufacturer id
 DCW &0001 ; manufacturer device number
 DCD 6400 ; offset of start of driver
 DCD 2092 ; size of driver
; -- fmvdev.drv --
 DCW &1000 ; manufacturer id
 DCW &0020 ; manufacturer device number
 DCD 8492 ; offset of start of driver
 DCD 1032 ; size of driver
; -- mei563dev.drv --
 DCW &0010 ; manufacturer id
 DCW &0563 ; manufacturer device number
 DCD 9524 ; offset of start of driver
 DCD 2036 ; size of driver
; -- lccddev.drv --
 DCW &1000 ; manufacturer id
 DCW &0050 ; manufacturer device number
 DCD 11560 ; offset of start of driver
 DCD 7104 ; size of driver
; -- depotdev.drv --
 DCW &1000 ; manufacturer id
 DCW &0030 ; manufacturer device number
 DCD 18664 ; offset of start of driver
 DCD 1712 ; size of driver
 END
