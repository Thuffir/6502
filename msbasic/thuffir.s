.segment "BIOS"

; Read one character from terminal 
MONRDKEY:
		lda TERMIO	; Poll character
		cmp #$FF	; If anything received
		bne MONCOUT	; Echo it back
		rts

; Output one character to terminal
MONCOUT:
		sta TERMIO	; Output character to terminal 
		rts

; Dummy interrupt handler
EMPTYINT:
		rti

; Entry point
START:
		sei		; Disable interrupts
		cld		; Clear decimal mode
		ldx #$FF	; Initialize stack pointer
		txs
@loop:		inx		; Show header message 
		lda HEADER,x	; Get next character
		beq @done	; Until zero termination
		jsr MONCOUT	; Output character
		bne @loop	; Do nex character
@done:		jmp COLD_START	; Jump to basic start

; Interrupt and reset vectors
.segment "INTVEC"
		.word	EMPTYINT	; NMI vector
		.word	START		; Reset vector
		.word	EMPTYINT	; INT vector
