; configuration
CONFIG_2A := 1

; CONFIG_CBM_ALL := 1

CONFIG_DATAFLG := 1
; CONFIG_EASTER_EGG := 1
; CONFIG_FILE := 1; support PRINT#, INPUT#, GET#, CMD
; CONFIG_NO_CR := 1; terminal doesn't need explicit CRs on line ends
; CONFIG_NO_LINE_EDITING := 1; support for "@", "_", BEL etc.
CONFIG_NO_READ_Y_IS_ZERO_HACK := 1
CONFIG_PEEK_SAVE_LINNUM := 1
CONFIG_SCRTCH_ORDER := 2

; zero page
ZP_START  = $00
ZP_INBUFS = 80
;ZP_START1 = $00
;ZP_START2 = $0A
;ZP_START3 = $60
;ZP_START4 = $6B

; extra/override ZP variables
USR				:= GORESTART ; XXX

; inputbuffer
;INPUTBUFFER     := $0200

; constants
SPACE_FOR_GOSUB := $3E
STACK_TOP		:= $FA
WIDTH			:= 40
WIDTH2			:= 30

RAMSTART2		:= $0200

; Character constants
CTRL_C	:=  03
CTRL_X	:=  24
BS	:=  08
DEL	:= 127

; Address of Terminal I/O
TERMIO	= $8000
