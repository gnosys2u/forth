
DEFAULT REL
BITS 64

%include "core64.inc"

SECTION .text


;========================================

; extern void CallDLLRoutine( DLLRoutine function, long argCount, ulong flags, void *core );

    entry CallDLLRoutine
	; rcx - dll routine address
	; rdx - arg count
	; r8 - flags
	; r9 - pCore
    ; rax, rcx, rdx, r8, r9, r10, r11, xmm0-xmm5 are volatile and can be stomped.

    push r12        ; forth core state ptr
    push r13        ; system stack ptr
    push r14        ; forth stack ptr
    push r15
    push rsi
    push rdi
    push rbx
	; stack should be 16-byte aligned at this point

	mov rax, rdx		            ; rax = arg count
	mov r10, rcx		            ; r10 = routine address
    mov rdi, r8                     ; rdi = flags
    
    mov r12, r9                     ; r12 -> CoreState
	mov	r14, [r12 + FCore.SPtr]

    ; TOS: argN..arg1
	mov r13, rsp                    ; r13 is saved system stack pointer

    ; if there are less than 5 arguments, none will be passed on system stack
    cmp rax, 5
    jl .calldll1
    ; if number of args is odd, do a dummy push to fix stack alignment
    or rax, rax
    jpo .calldll1
    push rax

.calldll1:
    lea r11, [r14 + rax*8]
    mov rsi, r11                    ; rsi is forth stack with all DLL args removed
    dec rax
    jl .calldll9
    sub r11, 8                      ; r11 -> arg1

    ; arg1
    mov rcx, [r11]
    movq xmm0, rcx
    dec rax
    jl .calldll9

    ; arg2
    mov rdx, [r11 - 8]
    movq xmm1, rdx
    dec rax
    jl .calldll9

    ; arg3
    mov r8, [r11 - 16]
    movq xmm2, r8
    dec rax
    jl .calldll9

    ; arg4
    mov r9, [r11 - 24]
    movq xmm3, r9
    dec rax
    jl .calldll9


    ; push args 5..N on system stack
    mov r11, r14     ; r11 -> argN
.calldllLoop:
    mov rbx, [r11]
    push rbx
    add r11, 8
    dec rax
    jge .calldllLoop

.calldll9:
    ; all args have been fetched

    sub rsp, 32         ; shadow space
    call r10            ; call DLL entry point
    mov rsp, r13
    mov r14, rsi       ; remove DLL function args from TOS
    
	; handle void return flag
	and	rdi, 0001h
	jnz CallDLL4
			
    sub r14, 8
    mov [r14], rax
	
CallDLL4:
    mov [r12 + FCore.SPtr], r14

    ; TODO: handle stdcall?
	;and	rdi, 0004h	; stdcall calling convention flag
	;jnz CallDLL5
	;mov	ebx, edi
	;sal	ebx, 2
	;add	esp, ebx
;CallDLL5:
    pop rbx
    pop rdi
    pop rsi
    pop r15
    pop r14
    pop r13
    pop r12
	ret


; extern void NativeAction( CoreState *pCore, ulong opVal );
;-----------------------------------------------
;
; inner interpreter entry point for ops defined in assembler
;
    entry NativeAction
%ifdef LINUX
	push rcx
	push rdx
	mov	rcx, rdi
	mov rdx, rsi
%endif
	; rcx - pCore
	; rdx - opVal

	mov rax, [rcx + FCore.numOps]
    cmp rdx, rax
    jle .nativeAction1
	mov	rax, kForthErrorBadOpcode
	mov	[rcx + FCore.state], rax
%ifdef LINUX
	pop	rdx
	pop	rcx
%endif
    ret

.nativeAction1:
    push rcore
    push rpsp
    push rrp
    push rfp
    push rip
	; stack should be 16-byte aligned at this point

    mov rcore, rcx                        ; rcore -> CoreState
	mov	rpsp, [rcore + FCore.SPtr]
	mov rrp, [rcore + FCore.RPtr]
	mov	rfp, [rcore + FCore.FPtr]
	mov	rip, [rcore + FCore.IPtr]
	mov roptab, [rcore + FCore.ops]
	mov racttab, [rcore + FCore.optypeAction]
	mov rnext, nativeActionExit

	mov rax, [roptab + rdx*8]
    jmp rax

nativeActionExit:
    mov	[rcore + FCore.IPtr], rip
	mov	[rcore + FCore.SPtr], rpsp
	mov	[rcore + FCore.RPtr], rrp
	mov	[rcore + FCore.FPtr], rfp

	pop rip
	pop	rfp
	pop	rrp
	pop rpsp
	pop rcore
%ifdef LINUX
	pop	rdx
	pop	rcx
%endif
	ret

;_TEXT	ENDS
;END
