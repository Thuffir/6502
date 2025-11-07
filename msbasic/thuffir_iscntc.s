.segment "CODE"
ISCNTC:
		jsr MONRDKEY
		cmp #CTRL_C	; CTRL-C key stops execution
		beq @stop
		cmp #CTRL_X	; So does CTRL-X
@stop:  
;!!! runs into "STOP"
