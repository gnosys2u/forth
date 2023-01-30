BITS 32
%include "core.inc"

SECTION .text


;========================================

; extern void CallDLLRoutine( DLLRoutine function, long argCount, ulong flags, void *core );

    entry CallDLLRoutine
;_CallDLLRoutine:             ; PROC near C public uses ebx esi edx ecx edi ebp,
;	funcAddr:PTR,
;	argCount:DWORD,
;	flags:DWORD,
;	core:PTR
	push ebp
	mov	ebp,esp
	push ebx
	push ecx
	push edx
	push esi
	push edi
	push ebp
	
	mov	eax, [ebp + 8]    	; eax = funcAddr
	mov	edi, [ebp + 12]		; edi = argCount
	mov	esi, [ebp + 16]		; esi = flags
	mov	ebp, [ebp + 20]		; ebp -> ForthCore
	mov	edx, [ebp + FCore.SPtr]
	mov	ecx, edi
CallDLL1:
	sub	ecx, 1
	jl	CallDLL2
	mov	ebx, [edx]
	add	edx, 4
	push	ebx
	jmp CallDLL1
CallDLL2:
	; all args have been moved from parameter stack to PC stack
	mov	[ebp + FCore.SPtr], edx
	
	call	eax
	
	; handle void return flag
	mov	ecx, esi
	and	esi, 0001h
	jnz CallDLL4
			
	mov	ebx, [ebp + FCore.SPtr]
	sub	ebx, 4
	
	; push high part of result if 64-bit return flag set
	mov	esi, ecx
	and	esi, 0002h
	jz CallDLL3
	mov	[ebx], edx		; return high part of result on parameter stack
	sub	ebx, 4
	
CallDLL3:
	; push low part of result
	mov	[ebx], eax
	mov	[ebp + FCore.SPtr], ebx
	
CallDLL4:
	; cleanup PC stack
	mov	esi, ecx
	and	esi, 0004h	; stdcall calling convention flag
	jnz CallDLL5
	mov	ebx, edi
	sal	ebx, 2
	add	esp, ebx
CallDLL5:
	pop ebp
	pop	edi
	pop	esi
	pop edx
	pop ecx
	pop ebx
	leave
	ret


; extern void NativeAction( CoreState *pCore, ulong opVal );
;-----------------------------------------------
;
; inner interpreter entry point for ops defined in assembler
;
        entry NativeAction
;_NativeAction:        ; PROC near C public uses ebx esi edx ecx edi ebp,
;	core:PTR,
;	opVal:DWORD
    
	push ebp
	mov	ebp,esp
	push ebx
	push ecx
	push edx
	push esi
	push edi
	push ebp
	mov	eax,[ebp + 12]		; eax = opVal
	mov	ebp,[ebp + 8]		; ebp -> ForthCore

	call	native1

	pop ebp
	pop	edi
	pop	esi
	pop edx
	pop ecx
	pop ebx
	leave
	ret

native1:
	mov	esi, [ebp + FCore.IPtr]
	mov	edx, [ebp + FCore.SPtr]
	mov	ecx, [ebp + FCore.ops]
	mov	edi, nativeActionExit
	mov	eax, [ecx + eax*4]
	jmp	eax
nativeActionExit:
	mov	[ebp + FCore.IPtr], esi
	mov	[ebp + FCore.SPtr], edx
	ret
	
;_TEXT	ENDS
;END
