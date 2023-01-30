
DEFAULT REL
BITS 64

%include "core64.inc"

; Windows:
; first four non-FP args are in rcx, rdx, r8, r9, rest are on stack
; floating point args are passed in xmm0 - xmm3
;
; func1(int a, int b, int c, int d, int e);
; a in RCX, b in RDX, c in R8, d in R9, e pushed on stack
;
; func2(int a, double b, int c, float d);
; a in RCX, b in XMM1, c in R8, d in XMM3
;
; return value in rax
; 
; rax, rcx, rdx, r8, r9, r10, r11, xmm0-xmm5 are volatile and can be stomped by function calls.
; rbx, rbp, rdi, rsi, rsp, r12, r13, r14, r15, xmm6-xmm15 are non-volatile and must be saved/restored.
;
; Linux:
; first 6 non-FP args are in rdi, rsi, rdx, rcx, r8, r9, rest are on stack
; floating point args are passed in xmm0 - xmm3
;
; func1(int a, int b, int c, int d, int e);
; a in RDI, b in RSI, c in RDX, d in RCX, e in R8
;
; func2(int a, double b, int c, float d);
; a in RDI, b in XMM0, c in RSI, d in XMM1
;
; return value in rax
; 
; rax, rcx, rdx, r8, r9, r10, r11, r12, r13, r14, r15 are volatile and can be stomped by function calls.
; rbx, rbp, rdi, rsi, rsp, r12, r13, r14, r15, xmm6-xmm15 are non-volatile and must be saved/restored.
;
;		win		linux		forth
; rax	  retval
; rbx
; rcx	arg0	arg3
; rdx	arg1	arg2
; rsi			arg1		rip
; rdi			arg0		rnext
; rsp
; rbp
; r8	arg2	arg4		opcode in optype action routines
; r9	arg3	arg5		roptab
; r10	v		v			rnumops
; r11	v		v			racttab
; r12						rcore
; r13						rfp
; r14						rsp
; r15 						rrp
;
; amd64 register usage:
;	rsi			rip     IP
;	rdi 		rnext   inner interp PC (constant)
;	R9			roptab  ops table (volatile on external calls)
;	R10			rnumops number of ops (volatile on external calls)
;	R11			racttab actionType table (volatile on external calls)
;	R12			rcore   core ptr
;	R13			rfp     FP
;	R14			rpsp    SP (forth param stack)
;	R15			rrp     RP
;
;   R8          opcode in optype action routines

;
; ARM register usage:
;	R4			core ptr
;	R5			IP
;	R6			SP (forth param stack)
;	R7			RP
;	R8			FP
;	R9			ops table
;	R10			number of ops
;	R11			actionType table
;
; x86 register usage:
;	EDX		SP
;	ESI		IP
;	EDI		inner interp PC (constant)
;	EBP		core ptr (constant)
;


EXTERN CallDLLRoutine

SECTION .text

; register usage in a forthOp:
;
;	EAX		free
;	EBX		free
;	ECX		free
;	ESI		IP
;	RDI		inner interp PC (constant)
;	R9		ops table (volatile on external calls)
;	R10		number of ops (volatile on external calls)
;	R11		actionType table (volatile on external calls)
;   R12		core ptr (constant)
;   R13     FP frame pointer
;	R14		SP paramater stack pointer
;   R15     RP return stack pointer

; when in a opType routine:
;	AL		8-bit opType
;	RBX		full 32-bit opcode (need to mask off top 8 bits)

; remember when calling extern cdecl functions:
; 1) they are free to stomp EAX, EBX, ECX and EDX
; 2) they are free to modify their input params on stack

; if you need more than EAX, EBX and ECX in a routine, save ESI/IP & EDX/SP in FCore at start with these instructions:
;	mov	[rcore + FCore.IPtr], rip
;	mov	[rcore + FCore.SPtr], rpsp
; jump to interpFuncReenter at end - interpFuncReenter will restore ESI, EDX, and EDI and go back to inner loop

; Linux & OSX requires that the system stack be on a 16-byte boundary before you call any system routine
; In general in the inner interpreter code the system stack is kept at 16-byte boundary, so to
; determine how much need to offset the system stack to maintain OSX stack alignment, you count
; the number of pushes done, including both arguments to the called routine and any extra registers
; you have saved on the stack, and use this formula:
; offset = 4 * (3 - (numPushesDone mod 4))

; these are the trace flags that force the external trace output to be
; called each time an instruction is executed
traceDebugFlags EQU	kLogProfiler + kLogStack + kLogInnerInterpreter

; extra stack space allocated around external calls
kShadowSpace	EQU	32

; opcode types which include a varop specifier have it in bits 20-23
VAROP_HIMASK    EQU     00F00000h
VAROP_LOMASK    EQU     000FFFFFh
VAROP_SHIFT     EQU     20

;-----------------------------------------------
;
; unaryDoubleFunc is used for dsin, dsqrt, dceil, ...
;
%macro unaryDoubleFunc 2
%ifdef LINUX
GLOBAL %1
EXTERN %2
%1:
%else
GLOBAL %1
EXTERN %2
%1:
%endif
	movsd xmm0, QWORD[rpsp]
%ifdef LINUX
	push rip
	push rnext
%endif
	sub rsp, kShadowSpace			; shadow space
	call	%2
	add rsp, kShadowSpace
%ifdef LINUX
	pop rnext
	pop rip
%endif
    movsd QWORD[rpsp], xmm0
	jmp	restoreNext
%endmacro
	
;-----------------------------------------------
;
; unaryFloatFunc is used for fsin, fsqrt, fceil, ...
;
%macro unaryFloatFunc 2
%ifdef LINUX
GLOBAL %1
EXTERN %2
%1:
%else
GLOBAL %1
EXTERN %2
%1:
%endif
	movss xmm0, DWORD[rpsp]
%ifdef LINUX
	push rip
	push rnext
%endif
	sub rsp, kShadowSpace			; shadow space
	call	%2
	add rsp, kShadowSpace
%ifdef LINUX
	pop rnext
	pop rip
%endif
    movss DWORD[rpsp], xmm0
	jmp	restoreNext
%endmacro
	
;========================================
;  safe exception handler
;.safeseh SEH_handler

;SEH_handler   PROC
;	ret

;SEH_handler   ENDP

;-----------------------------------------------
;
; extOpType is used to handle optypes which are only defined in C++
;
;	r8 holds the opcode
;
entry extOpType
	; get the C routine to handle this optype from optypeAction table in FCore
	mov	rax, r8
	shr	rax, 24							; rax is 8-bit optype
	mov	rcx, [rcore + FCore.optypeAction]
	mov	rax, [rcx + rax*8]				; rax is C routine to dispatch to
	and	r8, 00FFFFFFh
    mov rcx, rcore  ; 1st param to C routine
    mov rdx, r8     ; 2nd param to C routine
    ; stack is already 16-byte aligned
	sub rsp, kShadowSpace			; shadow space
	call	rax
	add rsp, kShadowSpace
	; NOTE: we can't just jump to interpFuncReenter, since that will replace rnext & break single stepping
	mov	roptab, [rcore + FCore.ops]
	mov	rnumops, [rcore + FCore.numOps]
	mov	racttab, [rcore + FCore.optypeAction]
	mov	rax, [rcore + FCore.state]
	or	rax, rax
	jnz	extOpType1	; if something went wrong
	jmp	rnext			; if everything is ok
	
; NOTE: Feb. 14 '07 - doing the right thing here - restoring IP & SP and jumping to
; the interpreter loop exit point - causes an access violation exception ?why?
	;mov	rip, [rcore + FCore.IPtr]
	;mov	rpsp, [rcore + FCore.SPtr]
	;jmp	interpFuncExit	; if something went wrong
	
extOpType1:
; TODO!
	ret

;-----------------------------------------------
;
; InitAsmTables - initializes first part of optable, where op positions are referenced by constants
;
; extern void InitAsmTables( CoreState *pCore );
entry InitAsmTables

	; rcx -> ForthCore struct (rdi in Linux)
	
	; setup normal (non-debug) inner interpreter re-entry point
%ifdef LINUX
	push	rcx
	push	rdx
	push	r8
	mov	rcx, rdi
%endif
	mov	rdx, interpLoopDebug
	mov	[rcx + FCore.innerLoop], rdx
	mov	rdx, interpLoopExecuteEntry
	mov	[rcx + FCore.innerExecute], rdx
    ; copy opTypesTable
	mov rax, [rcx + FCore.optypeAction]
	mov rdx, opTypesTable
    mov rcx, 256
.initAsmTables1:
    mov r8, [rdx]
    mov [rax], r8
    add rdx, 8
    add rax, 8
    dec rcx
    jnz .initAsmTables1
%ifdef LINUX
	pop	r8
	pop	rdx
	pop	rcx
%endif
	ret

;-----------------------------------------------
;
; single step a thread
;
; extern OpResult InterpretOneOpFast( CoreState *pCore, forthop op );
entry InterpretOneOpFast
    ; rcx is pCore	(rdi in Linux)
    ; rdx is op		(rsi in Linux)
    
	push rbx
    push rdi
    push rsi
    push r12
    push r13
    push r14
    push r15
%ifdef LINUX
	push rcx
	push rdx
	mov	rcx, rdi
	mov rdx, rsi
%endif
	; stack should be 16-byte aligned at this point
    mov rcore, rcx                        ; rcore -> CoreState
	mov	rpsp, [rcore + FCore.SPtr]
	mov rrp, [rcore + FCore.RPtr]
	mov	rfp, [rcore + FCore.FPtr]
	mov	rip, [rcore + FCore.IPtr]
	mov roptab, [rcore + FCore.ops]
	mov rnumops, [rcore + FCore.numOps]
	mov racttab, [rcore + FCore.optypeAction]
	mov rnext, InterpretOneOpFastExit
	; TODO: should we reset state to OK before every step?
	mov	rax, kResultOk
	mov	[rcore + FCore.state], rax
	; instead of jumping directly to the inner loop, do a call so that
	; error exits which do a return instead of branching to inner loop will work
    ; interpLoopExecuteEntry expects opcode in rbx
    mov rbx, rdx
	sub rsp, kShadowSpace			; shadow space
	call	interpLoopExecuteEntry
	jmp	InterpretOneOpFastExit2

InterpretOneOpFastExit:		; this is exit for state == OK - discard the unused return address from call above
	add	rsp, 8
InterpretOneOpFastExit2:	; this is exit for state != OK
	add rsp, kShadowSpace
	mov	[rcore + FCore.SPtr], rpsp
	mov	[rcore + FCore.RPtr], rrp
	mov	[rcore + FCore.FPtr], rfp
	mov	[rcore + FCore.IPtr], rip
	mov	rax, [rcore + FCore.state]

%ifdef LINUX
	pop rdx
	pop rcx
%endif
    pop r15
    pop r14
    pop r13
    pop r12
    pop rsi
    pop rdi
    pop rbx
	ret

;-----------------------------------------------
;
; inner interpreter C entry point
;
; extern OpResult InnerInterpreterFast( CoreState *pCore );
entry InnerInterpreterFast
	push rbx
    push rdi
    push rsi
    push r12
    push r13
    push r14
    push r15
	; stack should be 16-byte aligned at this point
    
%ifdef LINUX
    mov rcore, rdi                        ; rcore -> CoreState
%else
    mov rcore, rcx                        ; rcore -> CoreState
%endif
	call	interpFunc

	mov	[rcore + FCore.SPtr], rpsp
	mov	[rcore + FCore.RPtr], rrp
	mov	[rcore + FCore.FPtr], rfp
	mov	[rcore + FCore.IPtr], rip
	mov	rax, [rcore + FCore.state]

    pop r15
    pop r14
    pop r13
    pop r12
    pop rsi
    pop rdi
    pop rbx
	ret

;-----------------------------------------------
;
; inner interpreter
;   caller has saved all non-volatile registers
;   rcore is set by caller
;	jump to interpFuncReenter if you need to reload IP, SP, interpLoop
entry interpFunc
	push rbp			; 16-byte align stack
	mov rbp, rsp
entry interpFuncReenter
	mov	rpsp, [rcore + FCore.SPtr]
	mov rrp, [rcore + FCore.RPtr]
	mov	rfp, [rcore + FCore.FPtr]
	mov	rip, [rcore + FCore.IPtr]
	mov roptab, [rcore + FCore.ops]
	mov rnumops, [rcore + FCore.numOps]
	mov racttab, [rcore + FCore.optypeAction]
	mov	rax, kResultOk
	mov	[rcore + FCore.state], rax
    
	mov	rax, [rcore + FCore.traceFlags]
	and	rax, traceDebugFlags
	jz .interpFunc1
    ; setup inner interp entry points for debugging mode
	mov	rbx, traceLoopDebug
	mov	[rcore + FCore.innerLoop], rbx
	mov rbx, traceLoopExecuteEntry
	mov	[rcore + FCore.innerExecute], rbx
	jmp	.interpFunc2
.interpFunc1:
    ; setup inner interp entry points for regular mode
	mov rbx, interpLoopDebug
	mov	[rcore + FCore.innerLoop], rbx
	mov rbx, interpLoopExecuteEntry
	mov	[rcore + FCore.innerExecute], rbx
.interpFunc2:
	mov	rip, [rcore + FCore.IPtr]
	mov	rpsp, [rcore + FCore.SPtr]
	;mov	rnext, interpLoopDebug
	mov	rnext, [rcore + FCore.innerLoop]
	jmp	rnext

entry interpLoopDebug
	cmp rsp, rbp
	jz .iil2
	mov rbx, [rcore + FCore.scratch]
.iil2:
	; while debugging, store IP,SP in corestate shadow copies after every instruction
	;   so crash stacktrace will be more accurate (off by only one instruction)
	mov	[rcore + FCore.IPtr], rip
	mov	[rcore + FCore.SPtr], rpsp
	mov	[rcore + FCore.RPtr], rrp
	mov	[rcore + FCore.FPtr], rfp
entry interpLoop
	mov	ebx, [rip]		; rbx is opcode
	mov	[rcore + FCore.scratch], rbx
	add	rip, 4			; advance IP
	; interpLoopExecuteEntry is entry for executeBop/methodWithThis/methodWithTos - expects opcode in rbx
interpLoopExecuteEntry:
	cmp	rbx, rnumops
	jae	notNative
	mov	rcx, [roptab + rbx*8]
	jmp	rcx

entry traceLoopDebug
	; while debugging, store IP/SP/RP/FP in corestate shadow copies after every instruction
	;   so crash stacktrace will be more accurate (off by only one instruction)
	mov	[rcore + FCore.IPtr], rip
	mov	[rcore + FCore.SPtr], rpsp
	mov	[rcore + FCore.RPtr], rrp
	mov	[rcore + FCore.FPtr], rfp
	mov	ebx, [rip]		; rbx is opcode
	mov	rax, rip		; rax is the IP for trace
	jmp	traceLoopDebug2

	; traceLoopExecuteEntry is entry for executeBop/methodWithThis/methodWithTos - expects opcode in rbx
traceLoopExecuteEntry:
	xor	rax, rax		; put null in trace IP for indirect execution (op isn't at IP)
	sub	rip, 4			; actual IP was already advanced by execute/method op, don't double advance it
traceLoopDebug2:
%ifdef LINUX
	push rip
	push rnext
	mov	rdi, rcore
	mov	rsi, rax
	mov rdx, rbx
%else
    mov rcx, rcore      ; 1st param - core
    mov rdx, rax        ; 2nd param - IP
    mov r8, rbx         ; 3rd param - opcode (used if IP param is null)
%endif
	sub rsp, kShadowSpace			; shadow space
	xcall traceOp
	add rsp, kShadowSpace
%ifdef LINUX
	pop rnext
	pop rip
%endif
	mov roptab, [rcore + FCore.ops]
	mov rnumops, [rcore + FCore.numOps]
	mov racttab, [rcore + FCore.optypeAction]
	add	rip, 4			; advance IP
	jmp interpLoopExecuteEntry

interpFuncExit:
	pop rbp
	mov	[rcore + FCore.state], rax
	mov	[rcore + FCore.IPtr], rip
	mov	[rcore + FCore.SPtr], rpsp
	mov	[rcore + FCore.RPtr], rrp
	mov	[rcore + FCore.FPtr], rfp
	ret

badOptype:
	mov	rax, kForthErrorBadOpcodeType
	jmp	interpFuncErrorExit

badVarOperation:
	mov	rax, kForthErrorBadVarOperation
	jmp	interpFuncErrorExit
	
badOpcode:
	mov	rax, kForthErrorBadOpcode

	
interpFuncErrorExit:
	; error exit point
	; rax is error code
	mov	[rcore + FCore.error], rax
	mov	rax, kResultError
	jmp	interpFuncExit
	
interpFuncFatalErrorExit:
	; fatal error exit point
	; rax is error code
	mov	[rcore + FCore.error], rax
	mov	rax, kResultFatalError
	jmp	interpFuncExit
	
; op (in rbx) is not defined in assembler, dispatch through optype table
notNative:
	mov	rax, rbx			; leave full opcode in rbx
	shr	rax, 24				; rax is 8-bit optype
	mov	rax, [racttab + rax*8]
	jmp	rax

nativeImmediate:
	and	rbx, 0x00FFFFFF
	cmp	rbx, rnumops
	jae	badOpcode
	mov	rcx, [roptab + rbx*8]
	jmp	rcx

; jump here to reload volatile registers and continue
restoreNext:
	mov roptab, [rcore + FCore.ops]
	mov rnumops, [rcore + FCore.numOps]
	mov racttab, [rcore + FCore.optypeAction]
%ifdef LINUX
;	mov	rnext, [rcore + FCore.innerLoop]	; rnext/rdi in linux is arg0 for calls
;	mov rip, [rcore + FCore.IPtr]			; rip/rsi in linux is arg1 for calls
%endif
    jmp rnext
    
; externalBuiltin is invoked when a builtin op which is outside of range of table is invoked
externalBuiltin:
	; it should be impossible to get here now
	jmp	badOpcode
	
;-----------------------------------------------
;
; combos which push an immediate literal, then execute a builtin op defined in assembler
;
entry nativeU32Type
    xor rax, rax
    mov	eax, DWORD[rip]
    add rip, 4
    sub rpsp, 8
    mov [rpsp], rax
	and	rbx, 0x00FFFFFF
	cmp	rbx, rnumops
	jae badOpcode
	mov	rcx, [roptab + rbx*8]
	jmp	rcx

entry nativeS32Type
    movsx	rax, DWORD[rip]
    add rip, 4
    sub rpsp, 8
    mov [rpsp], rax
	and	rbx, 0x00FFFFFF
	cmp	rbx, rnumops
	jae badOpcode
	mov	rcx, [roptab + rbx*8]
	jmp	rcx

entry native64Type
    mov	rax, [rip]
    add rip, 8
    sub rpsp, 8
    mov [rpsp], rax
	and	rbx, 0x00FFFFFF
	cmp	rbx, rnumops
	jae badOpcode
	mov	rcx, [roptab + rbx*8]
	jmp	rcx

;-----------------------------------------------
;
; cCodeType is used by "builtin" ops which are only defined in C++
;
;	rbx holds the opcode
;
entry cCodeType
	and	rbx, 00FFFFFFh
	; dispatch to C version if valid
	cmp	rbx, rnumops
	jae	badOpcode
	; rbx holds the opcode which was just dispatched, use its low 24-bits as index into builtinOps table of ops defined in C/C++
	mov	rax, [roptab + rbx*8]				; rax is C routine to dispatch to
	; save current IP and SP	
	mov	[rcore + FCore.IPtr], rip
	mov	[rcore + FCore.SPtr], rpsp
	mov	[rcore + FCore.RPtr], rrp
	mov	[rcore + FCore.FPtr], rfp
%ifdef LINUX
	; need to save/restore rnext/rdi since interpretOneOpFast passes in the register, but doesn't
	;  change the copy in rcore.innerLoop, since that would break tracing
	push rnext
    mov rdi, rcore      ; 1st param - core
	sub rsp, kShadowSpace + 8			; shadow space
	call	rax
	add rsp, kShadowSpace + 8
	pop rnext
%else
    mov rcx, rcore      ; 1st param - core
	sub rsp, kShadowSpace			; shadow space
	call	rax
	add rsp, kShadowSpace
%endif
	; load IP and SP from core, in case C routine modified them
	; NOTE: we can't just jump to interpFuncReenter, since that will replace rnext & break single stepping
	mov	rpsp, [rcore + FCore.SPtr]
	mov rrp, [rcore + FCore.RPtr]
	mov	rfp, [rcore + FCore.FPtr]
	mov	rip, [rcore + FCore.IPtr]
	mov	rax, [rcore + FCore.state]
	mov roptab, [rcore + FCore.ops]
	mov rnumops, [rcore + FCore.numOps]
	mov racttab, [rcore + FCore.optypeAction]
	or	rax, rax
	jnz	interpFuncExit		; if something went wrong (should this be interpFuncErrorExit?)
	jmp	rnext					; if everything is ok
	
; NOTE: Feb. 14 '07 - doing the right thing here - restoring IP & SP and jumping to
; the interpreter loop exit point - causes an access violation exception ?why?
	;mov	rip, [rcore + FCore.IPtr]
	;mov	rpsp, [rcore + FCore.SPtr]
	;jmp	interpFuncExit	; if something went wrong
	
;-----------------------------------------------
;
; combos which push an immediate literal, then execute a builtin op defined in C++
;
entry cCodeU32Type
    xor rax, rax
    mov	eax, DWORD[rip]
    add rip, 4
    sub rpsp, 8
    mov [rpsp], rax
    jmp cCodeType

entry cCodeS32Type
    movsx	rax, WORD[rip]
    add rip, 4
    sub rpsp, 8
    mov [rpsp], rax
    jmp cCodeType

entry cCode64Type
    mov	rax, [rip]
    add rip, 8
    sub rpsp, 8
    mov [rpsp], rax
    jmp cCodeType

;-----------------------------------------------
;
; user-defined ops (forth words defined with colon)
;
entry userDefType
	; get low-24 bits of opcode & check validity
	and	rbx, 00FFFFFFh
	cmp	rbx, rnumops
	jge	badUserDef
	; push IP on rstack
	sub	rrp, 8
	mov	[rrp], rip
	; get new IP
	mov	rip, [roptab + rbx*8]
	jmp	rnext

badUserDef:
	mov	rax, kForthErrorBadOpcode
	jmp	interpFuncErrorExit

;-----------------------------------------------
;
; combos which push an immediate literal, then execute a user-defined op
;
entry userDefU32Type
    xor rax, rax
    mov	eax, DWORD[rip]
    add rip, 4
    sub rpsp, 8
    mov [rpsp], rax
    jmp userDefType

entry userDefS32Type
    movsx	rax, WORD[rip]
    add rip, 4
    sub rpsp, 8
    mov [rpsp], rax
    jmp userDefType

entry userDef64Type
    mov	rax, [rip]
    add rip, 8
    sub rpsp, 8
    mov [rpsp], rax
    jmp userDefType

;-----------------------------------------------
;
; user-defined ops (forth words defined with colon)
;
entry relativeDefType
	; rbx is offset (in longs) from dictionary base of user definition
	and	rbx, 00FFFFFFh
	sal	rbx, 2
	mov	rax, [rcore + FCore.DictionaryPtr]
	add	rbx, [rax + MemorySection.pBase]
	cmp	rbx, [rax + MemorySection.pCurrent]
	jge	badUserDef
	; push IP on rstack
	sub	rrp, 8
	mov	[rrp], rip
	mov	rip, rbx
	jmp	rnext

;-----------------------------------------------
;
; unconditional branch ops
;
entry branchType
	; get low-24 bits of opcode
	mov	rax, rbx
	and	rax, 00800000h
	jnz	branchBack
	; branch forward
	and	rbx, 00FFFFFFh
	sal	rbx, 2
	add	rip, rbx
	jmp	rnext

branchBack:
	mov rax, 0xFFFFFFFFFF000000
	or	rbx, rax
	sal	rbx, 2
	add	rip, rbx
	jmp	rnext

;-----------------------------------------------
;
; branch-on-zero ops
;
entry branchZType
	mov	rax, [rpsp]
	add	rpsp, 8
	or	rax, rax
	jz	branchType		; branch taken
	jmp	rnext	; branch not taken

;-----------------------------------------------
;
; branch-on-notzero ops
;
entry branchNZType
	mov	rax, [rpsp]
	add	rpsp, 8
	or	rax, rax
	jnz	branchType		; branch taken
	jmp	rnext	; branch not taken

;-----------------------------------------------
;
; case branch ops
;
entry caseBranchTType
    ; TOS: this_case_value case_selector
	mov	rax, [rpsp]		; rax is this_case_value
	add	rpsp, 8
	cmp	rax, [rpsp]
	jnz	caseMismatch
	; case did match - branch to case body
	and	rbx, 00FFFFFFh
	sal	rbx, 2
	add	rip, rbx
	add	rpsp, 8
caseMismatch:
	jmp	rnext

entry caseBranchFType
    ; TOS: this_case_value case_selector
	mov	rax, [rpsp]		; rax is this_case_value
	add	rpsp, 8
	cmp	rax, [rpsp]
	jz	caseMatched
	; case didn't match - branch to next case test
	and	rbx, 00FFFFFFh
	sal	rbx, 2
	add	rip, rbx
	jmp	rnext

caseMatched:
	add	rpsp, 8
	jmp	rnext
	
;-----------------------------------------------
;
; branch around block ops
;
entry pushBranchType
	sub	rpsp, 8			; push IP (pointer to block)
	mov	[rpsp], rip
	and	rbx, 00FFFFFFh	; branch around block
	sal	rbx, 2
	add	rip, rbx
	jmp	rnext

;-----------------------------------------------
;
; relative data block ops
;
entry relativeDataType
	; rbx is offset from dictionary base of user definition
	and	rbx, 00FFFFFFh
	sal	rbx, 2
	mov	rax, [rcore + FCore.DictionaryPtr]
	add	rbx, [rax + MemorySection.pBase]
	cmp	rbx, [rax + MemorySection.pCurrent]
	jge	badUserDef
	; push address of data on pstack
	sub	rpsp, 8
	mov	[rpsp], rbx
	jmp	rnext

;-----------------------------------------------
;
; relative data ops
;
entry relativeDefBranchType
	; push relativeDef opcode for immediately following anonymous definition (IP in rip points to it)
	; compute offset from dictionary base to anonymous def
	mov	rax, [rcore + FCore.DictionaryPtr]
	mov	rcx, [rax + MemorySection.pBase]
	mov	rax, rip
	sub	rax, rcx
	sar	rax, 2
	; stick the optype in top 8 bits
	mov	rcx, kOpRelativeDef << 24
	or	rax, rcx
	sub	rpsp, 8
	mov	[rpsp], rax
	; advance IP past anonymous definition
	and	rbx, 00FFFFFFh	; branch around block
	sal	rbx, 2
	add	rip, rbx
	jmp	rnext


;-----------------------------------------------
;
; 24-bit constant ops
;
entry constantType
	; get low-24 bits of opcode
	mov	rax, rbx
	sub	rpsp, 8
	and	rax,00800000h
	jnz	constantNegative
	; positive constant
	and	rbx,00FFFFFFh
	mov	[rpsp], rbx
	jmp	rnext

constantNegative:
	or	rbx, 0FFFFFFFFFF000000h
	mov	[rpsp], rbx
	jmp	rnext

;-----------------------------------------------
;
; 24-bit offset ops
;
entry offsetType
	; get low-24 bits of opcode
	mov	rax, rbx
	and	rax, 00800000h
	jnz	offsetNegative
	; positive constant
	and	rbx, 00FFFFFFh
	add	[rpsp], rbx
	jmp	rnext

offsetNegative:
	or	rbx, 0FFFFFFFFFF000000h
	add	[rpsp], rbx
	jmp	rnext

;-----------------------------------------------
;
; 24-bit offset fetch ops
;
entry offsetFetchType
	; get low-24 bits of opcode
	mov	rax, rbx
	and	rax, 00800000h
	jnz	offsetFetchNegative
	; positive constant
	and	rbx, 00FFFFFFh
	add	rbx, [rpsp]
	mov	rax, [rbx]
	mov	[rpsp], rax
	jmp	rnext

offsetFetchNegative:
	or	rbx, 0FFFFFFFFFF000000h
	add	rbx, [rpsp]
	mov	rax, [rbx]
	mov	[rpsp], rax
	jmp	rnext

;-----------------------------------------------
;
; array offset ops
;
entry arrayOffsetType
	; get low-24 bits of opcode
	and	rbx, 00FFFFFFh		; rbx is size of one element
	; TOS is array base, tos-1 is index
    ; TODO: this probably isn't right for 64-bit
	imul	rbx, [rpsp+8]	; multiply index by element size
	add	rbx, [rpsp]			; add array base addr
	add	rpsp, 8
	mov	[rpsp], rbx
	jmp	rnext

;-----------------------------------------------
;
; local struct array ops
;
entry localStructArrayType
   ; bits 0..11 are padded struct length in bytes, bits 12..23 are frame offset in longs
   ; multiply struct length by TOS, add in (negative) frame offset, and put result on TOS
	mov	rax, 00000FFFh
	and	rax, rbx                ; rax is padded struct length in bytes
	imul	rax, [rpsp]              ; multiply index * length
	add	rax, [rcore + FCore.FPtr]
	and	rbx, 00FFF000h
	sar	rbx, 10							; rbx = frame offset in bytes of struct[0]
	sub	rax, rbx						; rax -> struct[N]
	mov	[rpsp], rax
	jmp	rnext

;-----------------------------------------------
;
; string constant ops
;
entry constantStringType
	; IP points to beginning of string
	; low 24-bits of rbx is string len in longs
	sub	rpsp, 8
	mov	[rpsp], rip		; push string ptr
	; get low-24 bits of opcode
	and	rbx, 00FFFFFFh
	shl	rbx, 2
	; advance IP past string
	add	rip, rbx
	jmp	rnext

;-----------------------------------------------
;
; local stack frame allocation ops
;
entry allocLocalsType
	; rpush old FP
	sub	rrp, 8
	mov	[rrp], rfp
	; set FP = RP, points at old FP
	mov	rfp, rrp
	mov	[rcore + FCore.FPtr], rfp
	; allocate amount of storage specified by low 24-bits of op on rstack
	and	rbx, 00FFFFFFh
	sal	rbx, 3
	sub	rrp, rbx
	; clear out allocated storage
	mov rcx, rrp
	xor rax, rax
alt1:
	mov [rcx], rax
	add rcx, 8
	sub rbx, 8
	jnz alt1
	jmp	rnext

;-----------------------------------------------
;
; local string init ops
;
entry initLocalStringType
   ; bits 0..11 are string length in bytes, bits 12..23 are frame offset in cells
   ; init the current & max length fields of a local string
   ; TODO: is frame offset in longs or cells?
	mov	rax, 00FFF000h
	and	rax, rbx
	sar	rax, 9							; rax = frame offset in bytes
	mov	rcx, rfp
	sub	rcx, rax						; rcx -> max length field
	and	rbx, 00000FFFh					; rbx = max length
	mov	[rcx], ebx						; set max length (32-bits)
	xor	rax, rax
	mov	[rcx + 4], eax					; set current length (32-bits) to 0
	mov	[rcx + 5], al					; add terminating null
	jmp	rnext

;-----------------------------------------------
;
; local reference ops
;
entry localRefType
	; push local reference - rbx is frame offset in cells
    ; TODO: should offset be in bytes instead?
	mov	rax, rfp
	and	rbx, 00FFFFFFh
	sal	rbx, 3
	sub	rax, rbx
	sub	rpsp, 8
	mov	[rpsp], rax
	jmp	rnext
	
;-----------------------------------------------
;
; member reference ops
;
entry memberRefType
	; push member reference - rbx is member offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, 00FFFFFFh
	add	rax, rbx
	sub	rpsp, 8
	mov	[rpsp], rax
	jmp	rnext
	
;-----------------------------------------------
;
; byte ops
;
entry localByteType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz localByteType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
localByteType1:
	; get ptr to byte var into rax
	mov	rax, rfp
	and	rbx, VAROP_LOMASK
	sal	rbx, 3
	sub	rax, rbx
	; see if it is a fetch
byteEntry:
	mov	rbx, [rcore + FCore.varMode]
	or	rbx, rbx
	jnz	localByte1
	; fetch local byte
localByteFetch:
	sub	rpsp, 8
	movsx	rbx, BYTE[rax]
	mov	[rpsp], rbx
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localByte1:
	cmp	rbx, kVarGetDec
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, localByteActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx

localByteActionTable:
	DQ	localByteFetch
	DQ	localByteFetch
	DQ	localRef
	DQ	localByteStore
	DQ	localBytePlusStore
	DQ	localByteMinusStore
    DQ  localByteClear
    DQ  localBytePlus
    DQ  localByteInc
    DQ  localByteMinus
    DQ  localByteDec
    DQ  localByteIncGet
    DQ  localByteDecGet
    DQ  localByteGetInc
    DQ  localByteGetDec

entry localUByteType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz localUByteType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
localUByteType1:
	; get ptr to byte var into rax
	mov	rax, rfp
	and	rbx, VAROP_LOMASK
	sal	rbx, 3
	sub	rax, rbx
	; see if it is a fetch
ubyteEntry:
	mov	rbx, [rcore + FCore.varMode]
	or	rbx, rbx
	jnz	localUByte1
	; fetch local unsigned byte
localUByteFetch:
	sub	rpsp, 8
	xor	rbx, rbx
	mov	bl, [rax]
	mov	[rpsp], rbx
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localUByte1:
	cmp	rbx, kVarGetDec
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, localUByteActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx

localUByteActionTable:
	DQ	localUByteFetch
	DQ	localUByteFetch
	DQ	localRef
	DQ	localByteStore
	DQ	localBytePlusStore
	DQ	localByteMinusStore
    DQ  localByteClear
    DQ  localUBytePlus
    DQ  localByteInc
    DQ  localUByteMinus
    DQ  localByteDec
    DQ  localUByteIncGet
    DQ  localUByteDecGet
    DQ  localUByteGetInc
    DQ  localUByteGetDec

localRef:
	sub	rpsp, 8
	mov	[rpsp], rax
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext
	
localByteStore:
	mov	rbx, [rpsp]
	mov	[rax], bl
	add	rpsp, 8
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localBytePlusStore:
	xor	rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
	mov	bl, [rax]
	add	rbx, [rpsp]
	mov	[rax], bl
	add	rpsp, 8
	jmp	rnext

localByteMinusStore:
	xor	rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
	mov	bl, [rax]
	sub	rbx, [rpsp]
	mov	[rax], bl
	add	rpsp, 8
	jmp	rnext

localByteClear:
	xor	rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
	mov	[rax], bl
	jmp	rnext

localBytePlus:
	movsx	rbx, BYTE[rax]
	add	rbx, [rpsp]
	mov	[rpsp], rbx
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localByteInc:
    inc BYTE[rax]
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localByteMinus:
	movsx	rbx, BYTE[rax]
    mov rax, [rpsp]
	sub	rax, rbx
	mov	[rpsp], rax
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localByteDec:
    dec BYTE[rax]
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localByteIncGet:
    inc BYTE[rax]
	sub	rpsp, 8
	movsx	rbx, BYTE[rax]
	mov	[rpsp], rbx
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localByteDecGet:
    dec BYTE[rax]
	sub	rpsp, 8
	movsx	rbx, BYTE[rax]
	mov	[rpsp], rbx
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localByteGetInc:
	sub	rpsp, 8
	movsx	rbx, BYTE[rax]
    inc BYTE[rax]
	mov	[rpsp], rbx
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localByteGetDec:
	sub	rpsp, 8
	movsx	rbx, BYTE[rax]
    dec BYTE[rax]
	mov	[rpsp], rbx
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localUBytePlus:
    xor rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
    mov bl, BYTE[rax]
	add	rbx, [rpsp]
	mov	[rpsp], rbx
	jmp	rnext

localUByteMinus:
    xor rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
    mov bl, BYTE[rax]
    mov rax, [rpsp]
	sub	rax, rbx
	mov	[rpsp], rax
	jmp	rnext

localUByteIncGet:
    inc BYTE[rax]
    xor rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
    mov bl, BYTE[rax]
	sub	rpsp, 8
	mov	[rpsp], rbx
	jmp	rnext

localUByteDecGet:
    dec BYTE[rax]
    xor rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
    mov bl, BYTE[rax]
	sub	rpsp, 8
	mov	[rpsp], rbx
	jmp	rnext

localUByteGetInc:
	sub	rpsp, 8
    xor rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
    mov bl, BYTE[rax]
    inc BYTE[rax]
	mov	[rpsp], rbx
	jmp	rnext

localUByteGetDec:
	sub	rpsp, 8
    xor rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
    mov bl, BYTE[rax]
    dec BYTE[rax]
	mov	[rpsp], rbx
	jmp	rnext

entry fieldByteType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz fieldByteType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
fieldByteType1:
	; get ptr to byte var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	byteEntry

entry fieldUByteType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz fieldUByteType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
fieldUByteType1:
	; get ptr to byte var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	ubyteEntry

entry memberByteType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz memberByteType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
memberByteType1:
	; get ptr to byte var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	byteEntry

entry memberUByteType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz memberUByteType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
memberUByteType1:
	; get ptr to byte var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	ubyteEntry

entry localByteArrayType
	; get ptr to byte var into rax
	mov	rax, rfp
	and	rbx, 00FFFFFFh
	sal	rbx, 3
	sub	rax, rbx
	add	rax, [rpsp]		; add in array index on TOS
	add	rpsp, 8
	jmp	byteEntry

entry localUByteArrayType
	; get ptr to byte var into rax
	mov	rax, rfp
	and	rbx, 00FFFFFFh
	sal	rbx, 3
	sub	rax, rbx
	add	rax, [rpsp]		; add in array index on TOS
	add	rpsp, 8
	jmp	ubyteEntry

entry fieldByteArrayType
	; get ptr to byte var into rax
	; TOS is struct base ptr, NOS is index
	; rbx is field offset in bytes
	mov	rax, [rpsp]
	add	rax, [rpsp + 8]
	add	rpsp, 16
	and	rbx, 00FFFFFFh
	add	rax, rbx
	jmp	byteEntry

entry fieldUByteArrayType
	; get ptr to byte var into rax
	; TOS is struct base ptr, NOS is index
	; rbx is field offset in bytes
	mov	rax, [rpsp]
	add	rax, [rpsp + 8]
	add	rpsp, 16
	and	rbx, 00FFFFFFh
	add	rax, rbx
	jmp	ubyteEntry

entry memberByteArrayType
	; get ptr to byte var into rax
	; this data ptr is base ptr, TOS is index
	; rbx is field offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	add	rax, [rpsp]
	add	rpsp, 8
	and	rbx, 00FFFFFFh
	add	rax, rbx
	jmp	byteEntry

entry memberUByteArrayType
	; get ptr to byte var into rax
	; this data ptr is base ptr, TOS is index
	; rbx is field offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	add	rax, [rpsp]
	add	rpsp, 8
	and	rbx, 00FFFFFFh
	add	rax, rbx
	jmp	ubyteEntry

;-----------------------------------------------
;
; short ops
;
entry localShortType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz localShortType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
localShortType1:
	; get ptr to short var into rax
	mov	rax, rfp
	and	rbx, VAROP_LOMASK
	sal	rbx, 3
	sub	rax, rbx
	; see if it is a fetch
shortEntry:
	mov	rbx, [rcore + FCore.varMode]
	or	rbx, rbx
	jnz	localShort1
	; fetch local short
localShortFetch:
	sub	rpsp, 8
	movsx	rbx, WORD[rax]
	mov	[rpsp], rbx
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localShort1:
	cmp	rbx, kVarGetDec
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, localShortActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx

localShortActionTable:
	DQ	localShortFetch
	DQ	localShortFetch
	DQ	localRef
	DQ	localShortStore
	DQ	localShortPlusStore
	DQ	localShortMinusStore
    DQ  localShortClear
    DQ  localShortPlus
    DQ  localShortInc
    DQ  localShortMinus
    DQ  localShortDec
    DQ  localShortIncGet
    DQ  localShortDecGet
    DQ  localShortGetInc
    DQ  localShortGetDec

entry localUShortType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz localUShortType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
localUShortType1:
	; get ptr to short var into rax
	mov	rax, rfp
	and	rbx, VAROP_LOMASK
	sal	rbx, 3
	sub	rax, rbx
	; see if it is a fetch
ushortEntry:
	mov	rbx, [rcore + FCore.varMode]
	or	rbx, rbx
	jnz	localUShort1
	; fetch local unsigned short
localUShortFetch:
	sub	rpsp, 8
	movsx	rbx, WORD[rax]
	xor	rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
	mov	bx, [rax]
	mov	[rpsp], rbx
	jmp	rnext

localUShort1:
	cmp	rbx, kVarGetDec
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, localUShortActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx

localUShortActionTable:
	DQ	localUShortFetch
	DQ	localUShortFetch
	DQ	localRef
	DQ	localShortStore
	DQ	localShortPlusStore
	DQ	localShortMinusStore
    DQ  localShortClear
    DQ  localUShortPlus
    DQ  localShortInc
    DQ  localUShortMinus
    DQ  localShortDec
    DQ  localUShortIncGet
    DQ  localUShortDecGet
    DQ  localUShortGetInc
    DQ  localUShortGetDec

localShortStore:
	mov	rbx, [rpsp]
	mov	[rax], bx
	add	rpsp, 8
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localShortPlusStore:
	movsx	rbx, WORD[rax]
	add	rbx, [rpsp]
	mov	[rax], bx
	add	rpsp, 8
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localShortMinusStore:
	movsx	rbx, WORD[rax]
	sub	rbx, [rpsp]
	mov	[rax], bx
	add	rpsp, 8
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localShortClear:
	xor	rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
	mov	[rax], bx
	jmp	rnext

localShortPlus:
	movsx	rbx, WORD[rax]
	add	rbx, [rpsp]
	mov	[rpsp], rbx
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localShortInc:
    inc WORD[rax]
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localShortMinus:
	movsx	rbx, WORD[rax]
    mov rax, [rpsp]
	sub	rax, rbx
	mov	[rpsp], rax
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localShortDec:
    dec WORD[rax]
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localShortIncGet:
    inc WORD[rax]
	sub	rpsp, 8
	movsx	rbx, WORD[rax]
	mov	[rpsp], rbx
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localShortDecGet:
    dec WORD[rax]
	sub	rpsp, 8
	movsx	rbx, WORD[rax]
	mov	[rpsp], rbx
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localShortGetInc:
	sub	rpsp, 8
	movsx	rbx, WORD[rax]
    inc WORD[rax]
	mov	[rpsp], rbx
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localShortGetDec:
	sub	rpsp, 8
	movsx	rbx, WORD[rax]
    dec WORD[rax]
	mov	[rpsp], rbx
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localUShortPlus:
    xor rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
    mov bx, WORD[rax]
	add	rbx, [rpsp]
	mov	[rpsp], rbx
	jmp	rnext

localUShortMinus:
    xor rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
    mov bx, WORD[rax]
    mov rax, [rpsp]
	sub	rax, rbx
	mov	[rpsp], rax
	jmp	rnext

localUShortIncGet:
    inc WORD[rax]
    xor rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
    mov bx, WORD[rax]
	sub	rpsp, 8
	mov	[rpsp], rbx
	jmp	rnext

localUShortDecGet:
    dec WORD[rax]
    xor rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
    mov bx, WORD[rax]
	sub	rpsp, 8
	mov	[rpsp], rbx
	jmp	rnext

localUShortGetInc:
	sub	rpsp, 8
    xor rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
    mov bx, WORD[rax]
    inc WORD[rax]
	mov	[rpsp], rbx
	jmp	rnext

localUShortGetDec:
	sub	rpsp, 8
    xor rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
    mov bx, WORD[rax]
    dec WORD[rax]
	mov	[rpsp], rbx
	jmp	rnext

entry fieldShortType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz fieldShortType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
fieldShortType1:
	; get ptr to short var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	shortEntry

entry fieldUShortType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz fieldUShortType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
fieldUShortType1:
	; get ptr to unsigned short var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	ushortEntry

entry memberShortType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz memberShortType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
memberShortType1:
	; get ptr to short var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	shortEntry

entry memberUShortType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz memberUShortType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
memberUShortType1:
	; get ptr to unsigned short var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	ushortEntry

entry localShortArrayType
	; get ptr to int var into rax
	mov	rax, rfp
	and	rbx, 00FFFFFFh	; rbx is frame offset in cells
	sal	rbx, 3
	sub	rax, rbx
	mov	rbx, [rpsp]		; add in array index on TOS
	add	rpsp, 8
	sal	rbx, 1
	add	rax, rbx
	jmp	shortEntry

entry localUShortArrayType
	; get ptr to int var into rax
	mov	rax, rfp
	and	rbx, 00FFFFFFh	; rbx is frame offset in cells
	sal	rbx, 3
	sub	rax, rbx
	mov	rbx, [rpsp]		; add in array index on TOS
	add	rpsp, 8
	sal	rbx, 1
	add	rax, rbx
	jmp	ushortEntry

entry fieldShortArrayType
	; get ptr to short var into rax
	; TOS is struct base ptr, NOS is index
	; rbx is field offset in bytes
	mov	rax, [rpsp+4]	; rax = index
	sal	rax, 1
	add	rax, [rpsp]		; add in struct base ptr
	add	rpsp, 8
	and	rbx, 00FFFFFFh
	add	rax, rbx		; add in field offset
	jmp	shortEntry

entry fieldUShortArrayType
	; get ptr to short var into rax
	; TOS is struct base ptr, NOS is index
	; rbx is field offset in bytes
	mov	rax, [rpsp+4]	; rax = index
	sal	rax, 1
	add	rax, [rpsp]		; add in struct base ptr
	add	rpsp, 8
	and	rbx, 00FFFFFFh
	add	rax, rbx		; add in field offset
	jmp	ushortEntry

entry memberShortArrayType
	; get ptr to short var into rax
	; this data ptr is base ptr, TOS is index
	; rbx is field offset in bytes
	mov	rax, [rpsp]	; rax = index
	sal	rax, 1
	add	rax, [rcore + FCore.TPtr]
	add	rpsp, 8
	and	rbx, 00FFFFFFh
	add	rax, rbx		; add in field offset
	jmp	shortEntry

entry memberUShortArrayType
	; get ptr to short var into rax
	; this data ptr is base ptr, TOS is index
	; rbx is field offset in bytes
	mov	rax, [rpsp]	; rax = index
	sal	rax, 1
	add	rax, [rcore + FCore.TPtr]
	add	rpsp, 8
	and	rbx, 00FFFFFFh
	add	rax, rbx		; add in field offset
	jmp	ushortEntry


;-----------------------------------------------
;
; int ops
;
entry localIntType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz localIntType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
localIntType1:
	; get ptr to int var into rax
	mov	rax, rfp
	and	rbx, VAROP_LOMASK
	sal	rbx, 3
	sub	rax, rbx
	; see if it is a fetch
intEntry:
	mov	rbx, [rcore + FCore.varMode]
	or	rbx, rbx
	jnz	localInt1
	; fetch local int
localIntFetch:
	sub	rpsp, 8
	movsx	rbx, DWORD[rax]
	mov	[rpsp], rbx
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localInt1:
	cmp	rbx, kVarGetDec
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, localIntActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx

localIntActionTable:
	DQ	localIntFetch
	DQ	localIntFetch
	DQ	localRef
	DQ	localIntStore
	DQ	localIntPlusStore
	DQ	localIntMinusStore
    DQ  localIntClear
    DQ  localIntPlus
    DQ  localIntInc
    DQ  localIntMinus
    DQ  localIntDec
    DQ  localIntIncGet
    DQ  localIntDecGet
    DQ  localIntGetInc
    DQ  localIntGetDec

entry localUIntType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz localUIntType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
localUIntType1:
	; get ptr to int var into rax
	mov	rax, rfp
	and	rbx, VAROP_LOMASK
	sal	rbx, 3
	sub	rax, rbx
	; see if it is a fetch
uintEntry:
	mov	rbx, [rcore + FCore.varMode]
	or	rbx, rbx
	jnz	localUInt1
	; fetch local uint
localUIntFetch:
	sub	rpsp, 8
	xor	rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
	mov	ebx, DWORD[rax]
	mov	[rpsp], rbx
	jmp	rnext

localUInt1:
	cmp	rbx, kVarGetDec
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, localUIntActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx

localUIntActionTable:
	DQ	localUIntFetch
	DQ	localUIntFetch
	DQ	localRef
	DQ	localIntStore
	DQ	localIntPlusStore
	DQ	localIntMinusStore
    DQ  localIntClear
    DQ  localUIntPlus
    DQ  localIntInc
    DQ  localUIntMinus
    DQ  localIntDec
    DQ  localUIntIncGet
    DQ  localUIntDecGet
    DQ  localUIntGetInc
    DQ  localUIntGetDec

localIntStore:
	mov	ebx, [rpsp]
	mov	[rax], ebx
	add	rpsp, 8
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localIntPlusStore:
	mov	rbx, [rax]
	add	rbx, [rpsp]
	mov	[rax], rbx
	add	rpsp, 8
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localIntMinusStore:
	mov	rbx, [rax]
	sub	rbx, [rpsp]
	mov	[rax], rbx
	add	rpsp, 8
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localIntClear:
	xor	rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
	mov	[rax], ebx
	jmp	rnext

localIntPlus:
	movsx	rbx, DWORD[rax]
	add	rbx, [rpsp]
	mov	[rpsp], rbx
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localIntInc:
    inc DWORD[rax]
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localIntMinus:
	movsx	rbx, DWORD[rax]
    mov rax, [rpsp]
	sub	rax, rbx
	mov	[rpsp], rax
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localIntDec:
    dec DWORD[rax]
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localIntIncGet:
    inc DWORD[rax]
	sub	rpsp, 8
	movsx	rbx, DWORD[rax]
	mov	[rpsp], rbx
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localIntDecGet:
    dec DWORD[rax]
	sub	rpsp, 8
	movsx	rbx, DWORD[rax]
	mov	[rpsp], rbx
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localIntGetInc:
	sub	rpsp, 8
	movsx	rbx, DWORD[rax]
    inc DWORD[rax]
	mov	[rpsp], rbx
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localIntGetDec:
	sub	rpsp, 8
	movsx	rbx, DWORD[rax]
    dec DWORD[rax]
	mov	[rpsp], rbx
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localUIntPlus:
    xor rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
    mov ebx, DWORD[rax]
	add	rbx, [rpsp]
	mov	[rpsp], rbx
	jmp	rnext

localUIntMinus:
    xor rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
    mov ebx, DWORD[rax]
    mov rax, [rpsp]
	sub	rax, rbx
	mov	[rpsp], rax
	jmp	rnext

localUIntIncGet:
    inc DWORD[rax]
    xor rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
    mov ebx, DWORD[rax]
	sub	rpsp, 8
	mov	[rpsp], rbx
	jmp	rnext

localUIntDecGet:
    dec DWORD[rax]
    xor rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
    mov ebx, DWORD[rax]
	sub	rpsp, 8
	mov	[rpsp], rbx
	jmp	rnext

localUIntGetInc:
	sub	rpsp, 8
    xor rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
    mov ebx, DWORD[rax]
    inc DWORD[rax]
	mov	[rpsp], rbx
	jmp	rnext

localUIntGetDec:
	sub	rpsp, 8
    xor rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
    mov ebx, DWORD[rax]
    dec DWORD[rax]
	mov	[rpsp], rbx
	jmp	rnext

entry fieldIntType
	; get ptr to int var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz fieldIntType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
fieldIntType1:
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	intEntry

entry fieldUIntType
	; get ptr to uint var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz fieldUIntType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
fieldUIntType1:
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	uintEntry

entry memberIntType
	; get ptr to int var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz memberIntType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
memberIntType1:
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	intEntry

entry memberUIntType
	; get ptr to int var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz memberUIntType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
memberUIntType1:
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	uintEntry

entry localIntArrayType
	; get ptr to int var into rax
	mov	rax, rfp
	and	rbx, 00FFFFFFh
	sub	rbx, [rpsp]		; add in array index on TOS
	add	rpsp, 8
	sal	rbx, 3
	sub	rax, rbx
	jmp	intEntry

entry localUIntArrayType
	; get ptr to int var into rax
	mov	rax, rfp
	and	rbx, 00FFFFFFh
	sub	rbx, [rpsp]		; add in array index on TOS
	add	rpsp, 8
	sal	rbx, 3
	sub	rax, rbx
	jmp	uintEntry

entry fieldIntArrayType
	; get ptr to int var into rax
	; TOS is struct base ptr, NOS is index
	; rbx is field offset in bytes
	mov	rax, [rpsp+4]	; rax = index
	sal	rax, 2
	add	rax, [rpsp]		; add in struct base ptr
	add	rpsp, 8
	and	rbx, 00FFFFFFh
	add	rax, rbx		; add in field offset
	jmp	intEntry

entry fieldUIntArrayType
	; get ptr to int var into rax
	; TOS is struct base ptr, NOS is index
	; rbx is field offset in bytes
	mov	rax, [rpsp+4]	; rax = index
	sal	rax, 2
	add	rax, [rpsp]		; add in struct base ptr
	add	rpsp, 8
	and	rbx, 00FFFFFFh
	add	rax, rbx		; add in field offset
	jmp	uintEntry

entry memberIntArrayType
	; get ptr to short var into rax
	; this data ptr is base ptr, TOS is index
	; rbx is field offset in bytes
	mov	rax, [rpsp]	; rax = index
	sal	rax, 2
	add	rax, [rcore + FCore.TPtr]
	add	rpsp, 8
	and	rbx, 00FFFFFFh
	add	rax, rbx		; add in field offset
	jmp	intEntry

entry memberUIntArrayType
	; get ptr to short var into rax
	; this data ptr is base ptr, TOS is index
	; rbx is field offset in bytes
	mov	rax, [rpsp]	; rax = index
	sal	rax, 2
	add	rax, [rcore + FCore.TPtr]
	add	rpsp, 8
	and	rbx, 00FFFFFFh
	add	rax, rbx		; add in field offset
	jmp	uintEntry

;-----------------------------------------------
;
; float ops
;
entry localFloatType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz localFloatType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
localFloatType1:
	; get ptr to float var into rax
	mov	rax, rfp
	and	rbx, VAROP_LOMASK
	sal	rbx, 3
	sub	rax, rbx
	; see if it is a fetch
floatEntry:
	mov	rbx, [rcore + FCore.varMode]
	or	rbx, rbx
	jnz	localFloat1
	; fetch local float
localFloatFetch:
	sub	rpsp, 8
    xor rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
	mov	ebx, [rax]
	mov	[rpsp], rbx
	jmp	rnext

localFloatStore:
	mov	rbx, [rpsp]
	mov	[rax], ebx
	add	rpsp, 8
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localFloatPlusStore:
	movss xmm0, DWORD[rax]
    addss xmm0, DWORD[rpsp]
    movss DWORD[rax], xmm0
	add	rpsp, 8
	; set var operation back to default
	xor	rbx, rbx
	mov	[rcore + FCore.varMode], rbx
	jmp	rnext

localFloatMinusStore:
	movss xmm0, DWORD[rax]
    subss xmm0, DWORD[rpsp]
    movss DWORD[rax], xmm0
	add	rpsp, 8
	; set var operation back to default
	xor	rbx, rbx
	mov	[rcore + FCore.varMode], rbx
	jmp	rnext

localFloatPlus:
	movss xmm0, DWORD[rax]
    addss xmm0, DWORD[rpsp]
    movss DWORD[rpsp], xmm0
	; set var operation back to default
	xor	rbx, rbx
	mov	[rcore + FCore.varMode], rbx
	jmp	rnext

localFloatMinus:
	movss xmm0, DWORD[rpsp]
    subss xmm0, DWORD[rax]
    movss DWORD[rpsp], xmm0
	; set var operation back to default
	xor	rbx, rbx
	mov	[rcore + FCore.varMode], rbx
	jmp	rnext

localFloatActionTable:
	DQ	localFloatFetch
	DQ	localFloatFetch
	DQ	localRef
	DQ	localFloatStore
	DQ	localFloatPlusStore
	DQ	localFloatMinusStore
    DQ  localIntClear
    DQ  localFloatPlus
    DQ  badVarOperation      ; this is varop Inc, which doesn't exist for float and double
    DQ  localFloatMinus

localFloat1:
	cmp	rbx, kVarGetDec
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, localFloatActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx

entry fieldFloatType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz fieldFloatType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
fieldFloatType1:
	; get ptr to float var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	floatEntry

entry memberFloatType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz memberFloatType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
memberFloatType1:
	; get ptr to float var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	floatEntry

entry localFloatArrayType
	; get ptr to float var into rax
	mov	rax, rfp
	and	rbx, 00FFFFFFh
	sub	rbx, [rpsp]		; add in array index on TOS
	add	rpsp, 8
	sal	rbx, 3
	sub	rax, rbx
	jmp	floatEntry

entry fieldFloatArrayType
	; get ptr to float var into rax
	; TOS is struct base ptr, NOS is index
	; rbx is field offset in bytes
	mov	rax, [rpsp+4]	; rax = index
	sal	rax, 2
	add	rax, [rpsp]		; add in struct base ptr
	add	rpsp, 8
	and	rbx, 00FFFFFFh
	add	rax, rbx		; add in field offset
	jmp	floatEntry

entry memberFloatArrayType
	; get ptr to short var into rax
	; this data ptr is base ptr, TOS is index
	; rbx is field offset in bytes
	mov	rax, [rpsp]	; rax = index
	sal	rax, 2
	add	rax, [rcore + FCore.TPtr]
	add	rpsp, 8
	and	rbx, 00FFFFFFh
	add	rax, rbx		; add in field offset
	jmp	floatEntry
	
;-----------------------------------------------
;
; double ops
;
entry localDoubleType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz localDoubleType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
localDoubleType1:
	; get ptr to double var into rax
	mov	rax, rfp
	and	rbx, VAROP_LOMASK
	sal	rbx, 3
	sub	rax, rbx
	; see if it is a fetch
doubleEntry:
	mov	rbx, [rcore + FCore.varMode]
	or	rbx, rbx
	jnz	localDouble1
	; fetch local double
localDoubleFetch:
	sub	rpsp, 8
	mov	rbx, [rax]
	mov	[rpsp], rbx
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localDoubleStore:
	mov	rbx, [rpsp]
	mov	[rax], rbx
	add	rpsp, 8
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localDoublePlusStore:
	movsd   xmm0, QWORD[rax]
    addsd   xmm0, QWORD[rpsp]
    movsd QWORD[rax], xmm0
	add	rpsp, 8
	; set var operation back to default
	xor	rbx, rbx
	mov	[rcore + FCore.varMode], rbx
	jmp	rnext

localDoubleMinusStore:
	movsd xmm0, QWORD[rax]
    subsd xmm0, [rpsp]
    movsd QWORD[rax], xmm0
	add	rpsp, 8
	; set var operation back to default
	xor	rbx, rbx
	mov	[rcore + FCore.varMode], rbx
	jmp	rnext

localDoublePlus:
	movsd   xmm0, QWORD[rax]
    addsd   xmm0, QWORD[rpsp]
    movsd QWORD[rpsp], xmm0
	; set var operation back to default
	xor	rbx, rbx
	mov	[rcore + FCore.varMode], rbx
	jmp	rnext

localDoubleMinus:
	movsd xmm0, QWORD[rpsp]
    subsd xmm0, [rax]
    movsd QWORD[rpsp], xmm0
	; set var operation back to default
	xor	rbx, rbx
	mov	[rcore + FCore.varMode], rbx
	jmp	rnext

localDoubleActionTable:
	DQ	localDoubleFetch
	DQ	localDoubleFetch
	DQ	localRef
	DQ	localDoubleStore
	DQ	localDoublePlusStore
	DQ	localDoubleMinusStore
    DQ  localLongClear
	DQ	localDoublePlus
    DQ  badVarOperation     ; this is varop Inc, which doesn't exist for float and double
	DQ	localDoubleMinus

localDouble1:
	cmp	rbx, kVarGetDec
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, localDoubleActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx

entry fieldDoubleType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz fieldDoubleType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
fieldDoubleType1:
	; get ptr to double var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	doubleEntry

entry memberDoubleType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz memberDoubleType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
memberDoubleType1:
	; get ptr to double var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	doubleEntry

entry localDoubleArrayType
	; get ptr to double var into rax
	mov	rax, rfp
	and	rbx, 00FFFFFFh
	sal	rbx, 3
	sub	rax, rbx
	mov	rbx, [rpsp]		; add in array index on TOS
	add	rpsp, 8
	sal	rbx, 3
	add	rax, rbx
	jmp doubleEntry

entry fieldDoubleArrayType
	; get ptr to double var into rax
	; TOS is struct base ptr, NOS is index
	; rbx is field offset in bytes
	mov	rax, [rpsp+4]	; rax = index
	sal	rax, 3
	add	rax, [rpsp]		; add in struct base ptr
	add	rpsp, 8
	and	rbx, 00FFFFFFh
	add	rax, rbx		; add in field offset
	jmp	doubleEntry

entry memberDoubleArrayType
	; get ptr to short var into rax
	; this data ptr is base ptr, TOS is index
	; rbx is field offset in bytes
	mov	rax, [rpsp]	; rax = index
	sal	rax, 3
	add	rax, [rcore + FCore.TPtr]
	add	rpsp, 8
	and	rbx, 00FFFFFFh
	add	rax, rbx		; add in field offset
	jmp	doubleEntry
	
;-----------------------------------------------
;
; string ops
;
GLOBAL localStringType, stringEntry, localStringFetch, localStringStore, localStringAppend, localStringClear
entry localStringType
	mov	rax, rbx
	; see if opcode specifies a varop
	and	rax, VAROP_HIMASK
	jz localStringType1
	; store opcode varop in core.varMode
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
localStringType1:
	; get ptr to string var into rax
	mov	rax, rfp
	and	rbx, VAROP_LOMASK
	sal	rbx, 3
	sub	rax, rbx
	; see if it is a fetch
stringEntry:
%ifdef LINUX
	mov	[rcore + FCore.IPtr], rip		; save IP as it will get stomped in xcalls in linux
%endif
	mov	rbx, [rcore + FCore.varMode]
	or	rbx, rbx
	jnz	localString1
	; fetch local string
localStringFetch:
	sub	rpsp, 8
	add	rax, 8		; skip maxLen & currentLen fields
	mov	[rpsp], rax
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localString1:
	cmp	rbx, kVarClear
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, localStringActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx
	
; ref on a string returns the address of maxLen field, not the string chars
localStringRef:
	sub	rpsp, 8
	mov	[rpsp], rax
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext
	
localStringStore:
	; rax -> dest string maxLen field
	; TOS is src string addr
    mov rbx, rax            ; strlen will stomp rax
%ifdef LINUX
	push rip
	push rnext
	mov	rdi, [rpsp]			; rdi/rnext -> chars of src string
%else
	mov	rcx, [rpsp]			; rcx -> chars of src string
%endif
	sub rsp, kShadowSpace
	xcall	strlen
	add rsp, kShadowSpace
	; rax is src string length
	; rbx -> dest string maxLen field
    ; TOS -> src string
	; figure how many chars to copy
	cmp	eax, [rbx]
	jle lsStore1
    mov rax, rbx
lsStore1:
	; set current length field
	mov	[rbx + 4], eax
	
	; setup params for memcpy further down
%ifdef LINUX
    lea rdi, [rbx + 8]      ; 1st param - dest pointer
    mov rsi, [rpsp]         ; 2nd param - src pointer
    mov rdx, rax            ; 3rd param - num chars to copy
    mov rbx, rdi
%else
    lea rcx, [rbx + 8]      ; 1st param - dest pointer
    mov rdx, [rpsp]         ; 2nd param - src pointer
    mov r8, rax             ; 3rd param - num chars to copy
    mov rbx, rcx
%endif
    add rpsp, 8

    add rbx, rax            ; rbx -> end of dest string
	; add the terminating null
	xor	rax, rax
	mov	[rbx], al
	; set var operation back to default
	mov	[rcore + FCore.varMode], rax

	sub rsp, kShadowSpace			; shadow space
	xcall	memcpy
	add rsp, kShadowSpace

%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext

localStringAppend:
	; rax -> dest string maxLen field
	; TOS is src string addr
    mov rbx, rax            ; strlen will stomp rax
%ifdef LINUX
	push rip
	push rnext
	mov	rdi, [rpsp]			; rdi/rnext -> chars of src string
%else
	mov	rcx, [rpsp]			; rcx -> chars of src string
%endif
	sub rsp, kShadowSpace			; shadow space
	xcall	strlen
	add rsp, kShadowSpace
	; rax is src string length
	; rbx -> dest string maxLen field
    ; TOS -> src string
	; figure how many chars to copy
    mov r8d, DWORD[rbx]
    mov ecx, DWORD[rbx + 4]
    sub r8d, ecx       ; r8d is max bytes to copy (space left at end of string)
	cmp	eax, r8d
	jg lsAppend1
    mov r8, rax
lsAppend1:
    ; r8 is #bytes to copy
    ; rcx is current length
    mov rdx, rcx                ; rdx = old current length 
    
	; set current length field
    add rcx, r8                 ; rcx = new current length
    mov DWORD[rbx + 4], ecx
	
	; do the copy
; Windows: first four non-FP args are in rcx, rdx, r8, r9, rest are on stack
; Linux: first 6 non-FP args are in rdi, rsi, rdx, rcx, r8, r9, rest are on stack
%ifdef LINUX
    lea rdi, [rbx + 8]          ; 1st param - dest pointer
    add rdi, rdx                ;   at end of current dest string
    mov rsi, [rpsp]             ; 2nd param - src pointer
    mov rdx, r8                 ; 3rd param - num chars to copy
    mov rbx, rdi                ; save dest pointer in non-volatile register
%else
    lea rcx, [rbx + 8]          ; 1st param - dest pointer
    add rcx, rdx                ;   at end of current dest string
    mov rdx, [rpsp]             ; 2nd param - src pointer
    ; 3rd param - num chars to copy - already in r8
    mov rbx, rcx                ; save dest pointer in non-volatile register
%endif
    add rpsp, 8
    add rbx, r8                 ; rbx -> new end of dest string (for stuffing null after memcpy)
	sub rsp, kShadowSpace		; shadow space
	xcall	memcpy
	add rsp, kShadowSpace
    
	; add the terminating null
	xor	rax, rax
	mov	[rbx], al
		
	; set var operation back to default
	mov	[rcore + FCore.varMode], rax
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext

localStringClear:
	; rax -> dest string maxLen field
    add rax, 4              ; skip maxLen field
    xor rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
    mov DWORD[rax], ebx          ; set curLen = 0
    add rax, 4
    mov BYTE[rax], bl           ; and set first byte to terminating nul
	jmp	rnext

localStringActionTable:
	DQ	localStringFetch
	DQ	localStringFetch
	DQ	localStringRef
	DQ	localStringStore
	DQ	localStringAppend
	DQ	badVarOperation
	DQ	localStringClear

entry fieldStringType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz fieldStringType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
fieldStringType1:
	; get ptr to byte var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	stringEntry

entry memberStringType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz memberStringType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
memberStringType1:
	; get ptr to byte var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	stringEntry

entry localStringArrayType
	; get ptr to int var into rax
	mov	rax, rfp
	and	rbx, 00FFFFFFh
	sal	rbx, 3
	sub	rax, rbx		; rax -> maxLen field of string[0]
	mov	ebx, [rax]
	sar	rbx, 2
	add	rbx, 3			; rbx is element length in longs
	imul	rbx, [rpsp]	; mult index * element length
	add	rpsp, 8
	sal	rbx, 2			; rbx is offset in bytes
	add	rax, rbx
	jmp stringEntry

entry fieldStringArrayType
	; get ptr to string var into rax
	; TOS is struct base ptr, NOS is index
	; rbx is field offset in bytes
	and	rbx, 00FFFFFFh
	add	rbx, [rpsp]		; rbx -> maxLen field of string[0]
	mov	eax, [rbx]		; rax = maxLen
	sar	rax, 2
	add	rax, 3			; rax is element length in longs
	imul	rax, [rpsp+8]	; mult index * element length
	sal	rax, 2
	add	rax, rbx		; rax -> maxLen field of string[N]
	add	rpsp, 16
	jmp	stringEntry

entry memberStringArrayType
	; get ptr to string var into rax
	; this data ptr is base ptr, TOS is index
	; rbx is field offset in bytes
	and	rbx, 00FFFFFFh
	add	rbx, [rcore + FCore.TPtr]	; rbx -> maxLen field of string[0]
	mov	eax, [rbx]		; rax = maxLen
	sar	rax, 2
	add	rax, 3			; rax is element length in longs
	imul	rax, [rpsp]	; mult index * element length
	sal	rax, 2
	add	rax, rbx		; rax -> maxLen field of string[N]
	add	rpsp, 8
	jmp	stringEntry

;-----------------------------------------------
;
; op ops
;
entry localOpType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz localOpType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
localOpType1:
	; get ptr to op var into rbx
	mov	rax, rfp
	and	rbx, VAROP_LOMASK
	sal	rbx, 3
	sub	rax, rbx
	; see if it is a fetch (execute for ops)
opEntry:
	mov	rbx, [rcore + FCore.varMode]
	or	rbx, rbx
	jnz	localOp1
	; execute local op
localOpExecute:
    xor rbx, rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
	mov	ebx, [rax]
	mov	rax, [rcore + FCore.innerExecute]
	jmp rax

localOpActionTable:
	DQ	localOpExecute
	DQ	localUIntFetch
	DQ	localRef
	DQ	localIntStore

localOp1:
	cmp	rbx, kVarSet
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, localOpActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx

entry fieldOpType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz fieldOpType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
fieldOpType1:
	; get ptr to op var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	opEntry

entry memberOpType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz memberOpType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
memberOpType1:
	; get ptr to op var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	opEntry

entry localOpArrayType
	; get ptr to op var into rax
	mov	rax, rfp
	and	rbx, 00FFFFFFh
	sub	rbx, [rpsp]		; add in array index on TOS
	add	rpsp, 8
	sal	rbx, 3
	sub	rax, rbx
	jmp	opEntry

entry fieldOpArrayType
	; get ptr to op var into rax
	; TOS is struct base ptr, NOS is index
	; rbx is field offset in bytes
	mov	rax, [rpsp + 8]	; rax = index
	sal	rax, 2
	add	rax, [rpsp]		; add in struct base ptr
	add	rpsp, 16
	and	rbx, 00FFFFFFh
	add	rax, rbx		; add in field offset
	jmp	opEntry

entry memberOpArrayType
	; get ptr to short var into rax
	; this data ptr is base ptr, TOS is index
	; rbx is field offset in bytes
	mov	rax, [rpsp]	; rax = index
	sal	rax, 2
	add	rax, [rcore + FCore.TPtr]
	add	rpsp, 8
	and	rbx, 00FFFFFFh
	add	rax, rbx		; add in field offset
	jmp	opEntry
	
;-----------------------------------------------
;
; long (int64) ops
;
entry localLongType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz localLongType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
localLongType1:
	; get ptr to long var into rax
	mov	rax, rfp
	and	rbx, VAROP_LOMASK
	sal	rbx, 3
	sub	rax, rbx
	; see if it is a fetch
longEntry:
	mov	rbx, [rcore + FCore.varMode]
	or	rbx, rbx
	jnz	localLong1
	; fetch local long
entry localLongFetch
	sub	rpsp, 8
	mov	rbx, [rax]
	mov	[rpsp], rbx
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax	; set var operation back to default
	jmp	rnext

localLong1:
	cmp	rbx, kVarGetDec
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, localLongActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx

entry localLongStore
	mov	rbx, [rpsp]
	mov	[rax], rbx
	add	rpsp, 8
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax	; set var operation back to default
	jmp	rnext

entry localLongPlusStore
	mov	rbx, [rax]
	add	rbx, [rpsp]
	add	rpsp, 8
	mov	[rax], rbx
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax	; set var operation back to default
	jmp	rnext

entry localLongMinusStore
	mov	rbx, [rax]
	sub	rbx, [rpsp]
	add	rpsp, 8
	mov	[rax], rbx
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax	; set var operation back to default
	jmp	rnext

entry localLongClear
	xor	rbx, rbx
	mov	[rax], rbx
	mov	[rcore + FCore.varMode], rbx	; set var operation back to default
	jmp	rnext

localLongPlus:
	mov	rbx, QWORD[rax]
	add	rbx, [rpsp]
	mov	[rpsp], rbx
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax	; set var operation back to default
	jmp	rnext

localLongInc:
    inc QWORD[rax]
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax	; set var operation back to default
	jmp	rnext

localLongMinus:
	mov	rbx, QWORD[rax]
    mov rax, [rpsp]
	sub	rax, rbx
	mov	[rpsp], rax
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax	; set var operation back to default
	jmp	rnext

localLongDec:
    dec QWORD[rax]
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax	; set var operation back to default
	jmp	rnext

localLongIncGet:
    inc QWORD[rax]
	sub	rpsp, 8
	mov	rbx, QWORD[rax]
	mov	[rpsp], rbx
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax	; set var operation back to default
	jmp	rnext

localLongDecGet:
    dec QWORD[rax]
	sub	rpsp, 8
	mov	rbx, QWORD[rax]
	mov	[rpsp], rbx
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax	; set var operation back to default
	jmp	rnext

entry localLongGetInc
	sub	rpsp, 8
	mov	rbx, QWORD[rax]
	mov	[rpsp], rbx
    inc QWORD[rax]
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax	; set var operation back to default
	jmp	rnext

entry localLongGetDec
	sub	rpsp, 8
	mov	rbx, QWORD[rax]
	mov	[rpsp], rbx
    dec QWORD[rax]
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax	; set var operation back to default
	jmp	rnext

localLongActionTable:
	DQ	localLongFetch
	DQ	localLongFetch
	DQ	localRef
	DQ	localLongStore
	DQ	localLongPlusStore
	DQ	localLongMinusStore
    DQ  localLongClear
    DQ  localLongPlus
    DQ  localLongInc
    DQ  localLongMinus
    DQ  localLongDec
    DQ  localLongIncGet
    DQ  localLongDecGet
    DQ  localLongGetInc
    DQ  localLongGetDec

entry fieldLongType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz fieldLongType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
fieldLongType1:
	; get ptr to double var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	longEntry

entry memberLongType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz memberLongType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
memberLongType1:
	; get ptr to double var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	longEntry

entry localLongArrayType
	; get ptr to double var into rax
	mov	rax, rfp
	and	rbx, 00FFFFFFh
	sal	rbx, 3
	sub	rax, rbx
	mov	rbx, [rpsp]		; add in array index on TOS
	add	rpsp, 8
	sal	rbx, 3
	add	rax, rbx
	jmp longEntry

entry fieldLongArrayType
	; get ptr to double var into rax
	; TOS is struct base ptr, NOS is index
	; rbx is field offset in bytes
	mov	rax, [rpsp + 8]	; rax = index
	sal	rax, 3
	add	rax, [rpsp]		; add in struct base ptr
	add	rpsp, 16
	and	rbx, 00FFFFFFh
	add	rax, rbx		; add in field offset
	jmp	longEntry

entry memberLongArrayType
	; get ptr to short var into rax
	; this data ptr is base ptr, TOS is index
	; rbx is field offset in bytes
	mov	rax, [rpsp]	; rax = index
	sal	rax, 3
	add	rax, [rcore + FCore.TPtr]
	add	rpsp, 8
	and	rbx, 00FFFFFFh
	add	rax, rbx		; add in field offset
	jmp	longEntry
	
;-----------------------------------------------
;
; object ops
;
entry localObjectType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz localObjectType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
localObjectType1:
	; get ptr to Object var into rax
	mov	rax, rfp
	and	rbx, VAROP_LOMASK
	sal	rbx, 3
	sub	rax, rbx
	; see if it is a fetch
objectEntry:
	mov	rbx, [rcore + FCore.varMode]
	or	rbx, rbx
	jnz	localObject1
	; fetch local Object
localObjectFetch:
	sub	rpsp, 8
	mov	rbx, [rax]
	mov	[rpsp], rbx
	; set var operation back to default
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localObject1:
	cmp	rbx, kVarClear
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, localObjectActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx

entry localObjectStore
	; TOS is new object, rax points to destination/old object
	xor	rbx, rbx			; set var operation back to default/fetch
los0:
	mov	[rcore + FCore.varMode], rbx
	mov rbx, [rax]		; rbx = olbObj
	mov rcx, rax		; rcx -> destinationVar
	mov rax, [rpsp]		; rax = newObj
	add	rpsp, 8
	mov	[rcx], rax		; destinationVar = newObj
	cmp rax, rbx
	jz losx				; objects are same, don't change refcount
	; handle newObj refcount
	or rax, rax
	jz los1				; if newObj is null, don't try to increment refcount

	; newObj isn't null, increment its refcount
%ifdef ATOMIC_REFCOUNTS
	mov	rcx, 1
	lock xadd QWORD[rax + Object.refCount], rcx
%else
	inc QWORD[rax + Object.refCount]
%endif

	; handle oldObj refcount
los1:
	or rbx, rbx
	jz losx				; if oldObj is null, don't try to decrement refcount

	; oldObj  isn't null, decrement its refcount
%ifdef ATOMIC_REFCOUNTS
	mov	rcx, -1
	lock xadd QWORD[rbx + Object.refCount], rcx
	cmp	rcx, 1
	jz los3
%else
	dec QWORD[rbx + Object.refCount]
	jz los3
%endif

losx:
	jmp	rnext

	; destinationVar held last reference to oldObj, invoke olbObj.delete method
	; rax = newObj
	; rbx = oldObj
los3:
	; push this ptr on return stack
    sub rrp, 8
	mov	rax, [rcore + FCore.TPtr]
	mov	[rrp], rax
	
	; set this to oldObj
	mov	[rcore + FCore.TPtr], rbx
	mov	rbx, [rbx]	; rbx = oldObj methods pointer
	mov	ebx, [rbx]	; rbx = oldObj method 0 (delete)
	
	; execute the delete method opcode which is in rbx
	mov	rax, [rcore + FCore.innerExecute]
	jmp rax

localObjectClear:
	; rax points to destination/old object
	; for now, do the clear operation by pushing null on TOS then doing a regular object store
	; later on optimize it since we know source is a null
	xor	rbx, rbx
	sub	rpsp, 8
	mov [rpsp], rbx
	jmp	los0

; store object on TOS in variable pointed to by rax
; do not adjust reference counts of old object or new object
localObjectStoreNoRef:
	; TOS is new object, rax points to destination/old object
	xor	rbx, rbx			; set var operation back to default/fetch
	mov	[rcore + FCore.varMode], rbx
	mov	rbx, [rpsp]
	mov	[rax], rbx
	add	rpsp, 8
	jmp	rnext

; clear object reference, leave on TOS
localObjectUnref:
	; leave object on TOS
	sub	rpsp, 8
	mov	rbx, [rax]
	mov	[rpsp], rbx
	; if object var is already null, do nothing else
	or	rbx, rbx
	jz	lou2
	; clear object var
	mov	rcx, rax		; rcx -> object var
	xor	rax, rax
	mov	[rcx], rax
	; set var operation back to default
	mov	[rcore + FCore.varMode], rax
	; get object refcount, see if it is already 0
%ifdef ATOMIC_REFCOUNTS
	mov	rcx, -1
	lock xadd QWORD[rbx + Object.refCount], rcx
	or rcx, rcx
	; ugh, this leaves count as negative
	jz	louNegativeCountError
%else
	mov	rax, [rbx + Object.refCount]
	or	rax, rax
	jz	louNegativeCountError
	; decrement object refcount
	sub	rax, 1
	mov	[rbx + Object.refCount], rax
%endif

lou2:
	jmp	rnext

louNegativeCountError:
	; report refcount negative error
	mov	rax, kForthErrorBadReferenceCount
	jmp	interpFuncErrorExit
	
localObjectActionTable:
	DQ	localObjectFetch
	DQ	localObjectFetch
	DQ	localRef
	DQ	localObjectStore
	DQ	localObjectStoreNoRef
	DQ	localObjectUnref
	DQ	localObjectClear

entry fieldObjectType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz fieldObjectType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
fieldObjectType1:
	; get ptr to Object var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	objectEntry

entry memberObjectType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, VAROP_HIMASK
	jz memberObjectType1
	shr	rax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], rax
memberObjectType1:
	; get ptr to Object var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, VAROP_LOMASK
	add	rax, rbx
	jmp	objectEntry

entry localObjectArrayType
	; get ptr to Object var into rax
	mov	rax, rfp
	and	rbx, 00FFFFFFh
	sal	rbx, 3
	sub	rax, rbx
	mov	rbx, [rpsp]		; add in array index on TOS
	add	rpsp, 8
	sal	rbx, 3
	add	rax, rbx
	jmp objectEntry

entry fieldObjectArrayType
	; get ptr to Object var into rax
	; TOS is struct base ptr, NOS is index
	; rbx is field offset in bytes
	mov	rax, [rpsp+4]	; rax = index
	sal	rax, 3
	add	rax, [rpsp]		; add in struct base ptr
	add	rpsp, 8
	and	rbx, 00FFFFFFh
	add	rax, rbx		; add in field offset
	jmp	objectEntry

entry memberObjectArrayType
	; get ptr to short var into rax
	; this data ptr is base ptr, TOS is index
	; rbx is field offset in bytes
	mov	rax, [rpsp]	; rax = index
	sal	rax, 3
	add	rax, [rcore + FCore.TPtr]
	add	rpsp, 8
	and	rbx, 00FFFFFFh
	add	rax, rbx		; add in field offset
	jmp	objectEntry
	
;-----------------------------------------------
;
; method invocation ops
;

; invoke a method on object currently referenced by this ptr pair
entry methodWithThisType
	; rbx is method number
	; push this on return stack
	mov	rax, [rcore + FCore.TPtr]
	or	rax, rax
	jnz methodThis1
	mov	rax, kForthErrorBadObject
	jmp	interpFuncErrorExit
methodThis1:
	sub	rrp, 8
	mov	[rrp], rax
	
	and	rbx, 00FFFFFFh
	sal	rbx, 2
	add	rbx, [rax]
	mov	ebx, [rbx]	; rbx = method opcode
	mov	rax, [rcore + FCore.innerExecute]
	jmp rax
	
; invoke a method on an object referenced by ptr pair on TOS
entry methodWithTOSType
	; TOS is object
	; rbx is method number

	; set this ptr from TOS	
	mov	rax, [rpsp]
	add	rpsp, 8
	or	rax, rax
	jnz methodTos1
	mov	rax, kForthErrorBadObject
	jmp	interpFuncErrorExit
methodTos1:
	; push current this on return stack
	mov	rcx, [rcore + FCore.TPtr]
	sub	rrp, 8
	mov	[rrp], rcx
	mov	[rcore + FCore.TPtr], rax
	and	rbx, 00FFFFFFh
	sal	rbx, 2
	add	rbx, [rax]
	mov	ebx, [rbx]	; ebx = method opcode
	mov	rax, [rcore + FCore.innerExecute]
	jmp rax
	
; invoke a method on an object in a local variable
entry methodWithLocalObjectType
	; rbx: bits 0..11 are method index, bits 12..23 are frame offset in longs

	; push current this on return stack
	mov	rcx, [rcore + FCore.TPtr]
	sub	rrp, 8
	mov	[rrp], rcx

	; set this ptr from local object selected by rbx bits 12..23
    mov rcx, rbx
	and	rcx, 00FFF000h
	shr	rcx, 9
    mov rax, rfp
	sub	rax, rcx        ; rax points to local var holding object
	mov	rcx, [rax]      ; rcx is object pointer
	or	rcx, rcx
	jnz methodLocal1
	mov	rax, kForthErrorBadObject
	jmp	interpFuncErrorExit
methodLocal1:
	mov	[rcore + FCore.TPtr], rcx

    mov rax, [rcx]              ; eax points to methods table
    and rbx, 00000FFFh
    mov ebx, [rax + rbx * 4]    ; ebx is method opcode

	mov	rax, [rcore + FCore.innerExecute]
	jmp rax

; invoke a method on an object in a member variable
entry methodWithMemberObjectType
	; rbx: bits 0..11 are method index, bits 12..23 are object offset in longs

	; push current this on return stack
	sub	rrp, 8
	mov	rax, [rcore + FCore.TPtr]
	mov	[rrp], rax

	; set this ptr from member object selected by ebx bits 12..23
    mov rcx, rbx
	and	rcx, 00FFF000h
	shr	rcx, 12
    mov rax, [rcore + FCore.TPtr]
	mov	rcx, [rax + rcx]      ; rcx is object pointer
	or	rcx, rcx
	jnz methodMember1
	mov	rax, kForthErrorBadObject
	jmp	interpFuncErrorExit
methodMember1:
	mov	[rcore + FCore.TPtr], rcx

    mov rax, [rcx]              ; eax points to methods table
    and rbx, 00000FFFh
    mov ebx, [rax + rbx * 4]    ; ebx is method opcode

	mov	rax, [rcore + FCore.innerExecute]
	jmp rax

;-----------------------------------------------
;
; member string init ops
;
entry memberStringInitType
   ; bits 0..11 are string length in bytes, bits 12..23 are member offset in longs
   ; init the current & max length fields of a member string
	mov	rax, 00FFF000h
	and	rax, rbx
	sar	rax, 10							; rax = member offset in bytes
	mov	rcx, [rcore + FCore.TPtr]
	add	rcx, rax						; rcx -> max length field
	and	rbx, 00000FFFh					; rbx = max length
	mov	[rcx], rbx						; set max length
	xor	rax, rax
	mov	[rcx+4], rax					; set current length to 0
	mov	[rcx+8], al						; add terminating null
	jmp	rnext

;-----------------------------------------------
;
; builtinOps code
;

;-----------------------------------------------
;
; doByteOp is compiled as the first op in global byte vars
; the byte data field is immediately after this op
;
entry doByteBop
	; get ptr to byte var into rax
	mov	rax, rip
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	byteEntry

;-----------------------------------------------
;
; doUByteOp is compiled as the first op in global unsigned byte vars
; the byte data field is immediately after this op
;
entry doUByteBop
	; get ptr to byte var into rax
	mov	rax, rip
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	ubyteEntry

;-----------------------------------------------
;
; doByteArrayOp is compiled as the first op in global byte arrays
; the data array is immediately after this op
;
entry doByteArrayBop
	; get ptr to byte var into rax
	mov	rax, rip
	add	rax, [rpsp]
	add	rpsp, 8
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	byteEntry

entry doUByteArrayBop
	; get ptr to byte var into rax
	mov	rax, rip
	add	rax, [rpsp]
	add	rpsp, 8
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	ubyteEntry

;-----------------------------------------------
;
; doShortOp is compiled as the first op in global short vars
; the short data field is immediately after this op
;
entry doShortBop
	; get ptr to short var into rax
	mov	rax, rip
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	shortEntry

;-----------------------------------------------
;
; doUShortOp is compiled as the first op in global unsigned short vars
; the short data field is immediately after this op
;
entry doUShortBop
	; get ptr to short var into rax
	mov	rax, rip
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	ushortEntry

;-----------------------------------------------
;
; doShortArrayOp is compiled as the first op in global short arrays
; the data array is immediately after this op
;
entry doShortArrayBop
	; get ptr to short var into rax
	mov	rax, rip
	mov	rbx, [rpsp]		; rbx = array index
	add	rpsp, 8
	sal	rbx, 1
	add	rax, rbx	
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	shortEntry

entry doUShortArrayBop
	; get ptr to short var into rax
	mov	rax, rip
	mov	rbx, [rpsp]		; rbx = array index
	add	rpsp, 8
	sal	rbx, 1
	add	rax, rbx	
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	ushortEntry

;-----------------------------------------------
;
; doIntOp is compiled as the first op in global int vars
; the int data field is immediately after this op
;
entry doIntBop
	; get ptr to int var into rax
	mov	rax, rip
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	intEntry

;-----------------------------------------------
;
; doUIntOp is compiled as the first op in global uint vars
; the uint data field is immediately after this op
;
entry doUIntBop
	; get ptr to int var into rax
	mov	rax, rip
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	uintEntry

;-----------------------------------------------
;
; doIntArrayOp is compiled as the first op in global int arrays
; the data array is immediately after this op
;
entry doIntArrayBop
	; get ptr to int var into rax
	mov	rax, rip
	mov	rbx, [rpsp]		; rbx = array index
	add	rpsp, 8
	sal	rbx, 2
	add	rax, rbx	
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	intEntry

;-----------------------------------------------
;
; doUIntArrayOp is compiled as the first op in global uint arrays
; the data array is immediately after this op
;
entry doUIntArrayBop
	; get ptr to uint var into rax
	mov	rax, rip
	mov	rbx, [rpsp]		; rbx = array index
	add	rpsp, 8
	sal	rbx, 2
	add	rax, rbx	
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	uintEntry

;-----------------------------------------------
;
; doFloatOp is compiled as the first op in global float vars
; the float data field is immediately after this op
;
entry doFloatBop
	; get ptr to float var into rax
	mov	rax, rip
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	floatEntry

;-----------------------------------------------
;
; doFloatArrayOp is compiled as the first op in global float arrays
; the data array is immediately after this op
;
entry doFloatArrayBop
	; get ptr to float var into rax
	mov	rax, rip
	mov	rbx, [rpsp]		; rbx = array index
	add	rpsp, 8
	sal	rbx, 2
	add	rax, rbx	
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	floatEntry

;-----------------------------------------------
;
; doDoubleOp is compiled as the first op in global double vars
; the data field is immediately after this op
;
entry doDoubleBop
	; get ptr to double var into rax
	mov	rax, rip
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	doubleEntry

;-----------------------------------------------
;
; doDoubleArrayOp is compiled as the first op in global double arrays
; the data array is immediately after this op
;
entry doDoubleArrayBop
	; get ptr to double var into rax
	mov	rax, rip
	mov	rbx, [rpsp]		; rbx = array index
	add	rpsp, 8
	sal	rbx, 3
	add	rax, rbx	
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	doubleEntry

;-----------------------------------------------
;
; doStringOp is compiled as the first op in global string vars
; the data field is immediately after this op
;
entry doStringBop
	; get ptr to string var into rax
	mov	rax, rip
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	stringEntry

;-----------------------------------------------
;
; doStringArrayOp is compiled as the first op in global string arrays
; the data array is immediately after this op
;
entry doStringArrayBop
	; get ptr to string var into rax
	mov	rax, rip		; rax -> maxLen field of string[0]
	xor rbx, rbx
	mov	ebx, DWORD[rax]		; rbx = maxLen
	sar	rbx, 2
	add	rbx, 3			; rbx is element length in longs
	imul rbx, [rpsp]	; mult index * element length
	add	rpsp, 8
	sal	rbx, 2			; rbx is offset in bytes
	add	rax, rbx
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp stringEntry

;-----------------------------------------------
;
; doOpOp is compiled as the first op in global op vars
; the op data field is immediately after this op
;
entry doOpBop
	; get ptr to int var into rax
	mov	rax, rip
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	opEntry

;-----------------------------------------------
;
; doOpArrayOp is compiled as the first op in global op arrays
; the data array is immediately after this op
;
entry doOpArrayBop
	; get ptr to op var into rax
	mov	rax, rip
	mov	rbx, [rpsp]		; rbx = array index
	add	rpsp, 8
	sal	rbx, 2
	add	rax, rbx	
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	opEntry

;-----------------------------------------------
;
; doLongOp is compiled as the first op in global int64 vars
; the data field is immediately after this op
;
entry doLongBop
	; get ptr to double var into rax
	mov	rax, rip
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	longEntry

;-----------------------------------------------
;
; doLongArrayOp is compiled as the first op in global int64 arrays
; the data array is immediately after this op
;
entry doLongArrayBop
	; get ptr to double var into rax
	mov	rax, rip
	mov	rbx, [rpsp]		; rbx = array index
	add	rpsp, 8
	sal	rbx, 3
	add	rax, rbx	
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	longEntry

;-----------------------------------------------
;
; doObjectOp is compiled as the first op in global Object vars
; the data field is immediately after this op
;
entry doObjectBop
	; get ptr to Object var into rax
	mov	rax, rip
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	objectEntry

;-----------------------------------------------
;
; doObjectArrayOp is compiled as the first op in global Object arrays
; the data array is immediately after this op
;
entry doObjectArrayBop
	; get ptr to Object var into rax
	mov	rax, rip
	mov	rbx, [rpsp]		; rbx = array index
	add	rpsp, 8
	sal	rbx, 3
	add	rax, rbx	
	; pop rstack
	mov	rip, [rrp]
	add	rrp, 8
	jmp	objectEntry

;========================================

entry initStringBop
	;	TOS: len strPtr
	mov	rbx, [rpsp+8]	; rbx -> first char of string
	xor	rax, rax
	mov	[rbx-4], eax	; set current length = 0
	mov	[rbx], al		; set first char to terminating null
	mov	rax, [rpsp]		; rax == string length
	mov	[rbx-8], eax	; set maximum length field
	add	rpsp, 16
	jmp	rnext

;========================================

entry strFixupBop
	mov	rax, [rpsp]
	add	rpsp, 8
	mov rcx, rax
	xor	rbx, rbx
	; stuff a nul at end of string storage - there should already be one there or earlier
	add	rcx, [rax-8]
	mov	[rcx], bl
	mov rcx, rax
strFixupBop1:
	mov	bl, [rax]
	test	bl, 255
	jz	strFixupBop2
	add	rax, 1
	jmp	strFixupBop1

strFixupBop2:
	sub	rax, rcx
	mov	rbx, [rcx-8]
	cmp	rbx, rax
	jge	strFixupBop3
	; characters have been written past string storage end
	mov	rax, kForthErrorStringOverflow
	jmp	interpFuncErrorExit

strFixupBop3:
	mov	[rcx-4], rax
	jmp	rnext

;========================================

entry doneBop
	mov	rax, kResultDone
	jmp	interpFuncExit

;========================================

entry yieldBop
	mov	rax, kResultYield
	jmp	interpFuncExit

;========================================

entry abortBop
	mov	rax, kForthErrorAbort
	jmp	interpFuncFatalErrorExit

;========================================

entry noopBop
	jmp	rnext

;========================================
	
entry plusBop
	mov	rax, [rpsp]
	add	rpsp, 8
	add	rax, [rpsp]
	mov	[rpsp], rax
	jmp	rnext

;========================================
	
entry minusBop
	mov	rax, [rpsp]
	add	rpsp, 8
	mov	rbx, [rpsp]
	sub	rbx, rax
	mov	[rpsp], rbx
	jmp	rnext

;========================================

entry timesBop
	mov	rax, [rpsp]
	add	rpsp, 8
	imul	rax, [rpsp]
	mov	[rpsp], rax
	jmp	rnext

;========================================
	
entry times2Bop
	mov	rax, [rpsp]
	add	rax, rax
	mov	[rpsp], rax
	jmp	rnext

;========================================
	
entry times4Bop
	mov	rax, [rpsp]
	sal	rax, 2
	mov	[rpsp], rax
	jmp	rnext

;========================================
	
entry times8Bop
	mov	rax, [rpsp]
	sal	rax, 3
	mov	[rpsp], rax
	jmp	rnext

;========================================
	
entry divideBop
	; idiv takes 128-bit numerator in rdx:rax
	mov	rax, [rpsp+8]	; get numerator
	cqo					; convert into 128-bit in rdx:rax
	idiv	QWORD[rpsp]		; rax is quotient, rdx is remainder
	add	rpsp, 8
	mov	[rpsp], rax
	jmp	rnext

;========================================

entry divide2Bop
	mov	rax, [rpsp]
	sar	rax, 1
	mov	[rpsp], rax
	jmp	rnext
	
;========================================

entry divide4Bop
	mov	rax, [rpsp]
	sar	rax, 2
	mov	[rpsp], rax
	jmp	rnext
	
;========================================

entry divide8Bop
	mov	rax, [rpsp]
	sar	rax, 3
	mov	[rpsp], rax
	jmp	rnext
	
;========================================
	
entry divmodBop
	; idiv takes 128-bit numerator in rdx:rax
	mov	rax, [rpsp+8]	; get numerator
	cqo					; convert into 128-bit in rdx:rax
	idiv	QWORD[rpsp]		; rax is quotient, rdx is remainder
	mov	[rpsp+8], rdx
	mov	[rpsp], rax
	jmp	rnext
	
;========================================
	
entry modBop
	; idiv takes 128-bit numerator in rdx:rax
	mov	rax, [rpsp+8]	; get numerator
	cqo					; convert into 128-bit in rdx:rax
	idiv	QWORD[rpsp]		; rax is quotient, rdx is remainder
	add	rpsp, 8
	mov	[rpsp], rdx
	jmp	rnext
	
;========================================
	
entry negateBop
	mov	rax, [rpsp]
	neg	rax
	mov	[rpsp], rax
	jmp	rnext
	
;========================================
	
entry fplusBop
	movss xmm0, DWORD[rpsp]
	add	rpsp,8
    addss xmm0, DWORD[rpsp]
    movd eax, xmm0
    mov [rpsp], rax
	jmp	rnext
	
;========================================
	
entry fminusBop
	movss xmm0, DWORD[rpsp+8]
    movss xmm1, DWORD[rpsp]
    subss xmm0, xmm1
    movd eax, xmm0
	add	rpsp,8
    mov [rpsp], rax
	jmp	rnext
	
;========================================
	
entry ftimesBop
	movss xmm0, DWORD[rpsp]
	add	rpsp,8
    mulss xmm0, DWORD[rpsp]
    movd eax, xmm0
    mov [rpsp], rax
	jmp	rnext
	
;========================================
	
entry fdivideBop
	movss xmm0, DWORD[rpsp+8]
    divss xmm0, DWORD[rpsp]
    add rpsp, 8
    movd eax, xmm0
    mov [rpsp], rax
	jmp	rnext
	
;========================================
	
entry faddBlockBop
	; TOS: num pDst pSrcB pSrcA
	mov	rax, [rpsp+24]
	mov	rbx, [rpsp+16]
	mov	rcx, [rpsp]
	mov	rdx, [rpsp+8]
.faddBlockBop1:
	movss xmm0, DWORD[rax]
    addss xmm0, DWORD[rbx]
    movss DWORD[rdx], xmm0
	add rax, 4
	add rbx, 4
	add	rdx, 4
	sub rcx, 1
	jnz .faddBlockBop1
	add	rpsp, 32
	jmp	rnext
	
;========================================
	
entry fsubBlockBop
	; TOS: num pDst pSrcB pSrcA
	mov	rax, [rpsp+24]
	mov	rbx, [rpsp+16]
	mov	rcx, [rpsp]
	mov	rdx, [rpsp+8]
.fsubBlockBop1:
	movss xmm0, DWORD[rax]
    subss xmm0, DWORD[rbx]
    movss DWORD[rdx], xmm0
	add rax, 4
	add rbx, 4
	add	rdx, 4
	sub rcx, 1
	jnz .fsubBlockBop1
	add	rpsp, 32
	jmp	rnext
	
;========================================
	
entry fmulBlockBop
	; TOS: num pDst pSrcB pSrcA
	mov	rax, [rpsp+24]
	mov	rbx, [rpsp+16]
	mov	rcx, [rpsp]
	mov	rdx, [rpsp+8]
.fmulBlockBop1:
	movss xmm0, DWORD[rax]
    mulss xmm0, DWORD[rbx]
    movss DWORD[rdx], xmm0
	add rax, 4
	add rbx, 4
	add	rdx, 4
	sub rcx, 1
	jnz .fmulBlockBop1
	add	rpsp, 32
	jmp	rnext
	
;========================================
	
entry fdivBlockBop
	; TOS: num pDst pSrcB pSrcA
	mov	rax, [rpsp+24]
	mov	rbx, [rpsp+16]
	mov	rcx, [rpsp]
	mov	rdx, [rpsp+8]
.fdivBlockBop1:
	movss xmm0, DWORD[rax]
    divss xmm0, DWORD[rbx]
    movss DWORD[rdx], xmm0
	add rax, 4
	add rbx, 4
	add	rdx, 4
	sub rcx, 1
	jnz .fdivBlockBop1
	add	rpsp, 32
	jmp	rnext
	
;========================================
	
entry fscaleBlockBop
	; TOS: num scale pDst pSrc
	mov	rax, [rpsp+24]
	mov	rbx, [rpsp+16]
	mov	rcx, [rpsp]
	movss xmm0, DWORD[rpsp+8]
.fscaleBlockBop1:
    movss xmm1, DWORD[rax]
    mulss xmm1, xmm0
    movss DWORD[rbx], xmm1
	add rax,4
	add rbx,4
	sub rcx,1
	jnz .fscaleBlockBop1
	add	rpsp, 32
	jmp	rnext
	
;========================================
	
entry foffsetBlockBop
	; TOS: num offset pDst pSrc
	mov	rax, [rpsp+24]
	mov	rbx, [rpsp+16]
	mov	rcx, [rpsp]
	movss xmm0, DWORD[rpsp+8]
.foffsetBlockBop1:
    movss xmm1, DWORD[rax]
    addss xmm1, xmm0
    movss DWORD[rbx], xmm1
	add rax,4
	add rbx,4
	sub rcx,1
	jnz .foffsetBlockBop1
	add	rpsp, 32
	jmp	rnext
	
;========================================
	
entry fmixBlockBop
	; TOS: num scale pDst pSrc
	mov	rax, [rpsp+24]
	mov	rbx, [rpsp+16]
	mov	rcx, [rpsp]
	movss xmm0, DWORD[rpsp+8]
.fmixBlockBop1:
    movss xmm1, DWORD[rax]
    mulss xmm1, xmm0
    addss xmm1, DWORD[rbx]
    movss DWORD[rbx], xmm1
	add rax,4
	add rbx,4
	sub rcx,1
	jnz .fmixBlockBop1
	add	rpsp, 32
	jmp	rnext
	
;========================================
	
entry daddBlockBop
	; TOS: num pDst pSrcB pSrcA
	mov	rax, [rpsp+24]
	mov	rbx, [rpsp+16]
	mov	rcx, [rpsp]
	mov	rdx, [rpsp+8]
.daddBlockBop1:
	movsd xmm0, QWORD[rax]
    addsd xmm0, QWORD[rbx]
    movsd QWORD[rdx], xmm0
	add rax, 8
	add rbx, 8
	add	rdx, 8
	sub rcx, 1
	jnz .daddBlockBop1
	add	rpsp, 32
	jmp	rnext
	
;========================================
	
entry dsubBlockBop
	; TOS: num pDst pSrcB pSrcA
	mov	rax, [rpsp+24]
	mov	rbx, [rpsp+16]
	mov	rcx, [rpsp]
	mov	rdx, [rpsp+8]
.dsubBlockBop1:
	movsd xmm0, QWORD[rax]
    subsd xmm0, [rbx]
    movsd QWORD[rdx], xmm0
	add rax, 8
	add rbx, 8
	add	rdx, 8
	sub rcx, 1
	jnz .dsubBlockBop1
	add	rpsp, 32
	jmp	rnext
	
;========================================
	
entry dmulBlockBop
	; TOS: num pDst pSrcB pSrcA
	mov	rax, [rpsp+24]
	mov	rbx, [rpsp+16]
	mov	rcx, [rpsp]
	mov	rdx, [rpsp+8]
.dmulBlockBop1:
	movsd xmm0, QWORD[rax]
    mulsd xmm0, [rbx]
    movsd QWORD[rdx], xmm0
	add rax, 8
	add rbx, 8
	add	rdx, 8
	sub rcx, 1
	jnz .dmulBlockBop1
	add	rpsp, 32
	jmp	rnext
	
;========================================
	
entry ddivBlockBop
	; TOS: num pDst pSrcB pSrcA
	mov	rax, [rpsp+24]
	mov	rbx, [rpsp+16]
	mov	rcx, [rpsp]
	mov	rdx, [rpsp+8]
.ddivBlockBop1:
	movsd xmm0, QWORD[rax]
    divsd xmm0, [rbx]
    movsd QWORD[rdx], xmm0
	add rax, 8
	add rbx, 8
	add	rdx, 8
	sub rcx, 1
	jnz .ddivBlockBop1
	add	rpsp, 32
	jmp	rnext
	
;========================================
	
entry dscaleBlockBop
	; TOS: num scale pDst pSrc
	mov	rax, [rpsp+24]
	mov	rbx, [rpsp+16]
	mov	rcx, [rpsp]
	movsd	xmm0, QWORD[rpsp+8]
.dscaleBlockBop1:
    movsd xmm1, QWORD[rax]
    mulsd xmm1, xmm0
    movsd QWORD[rbx], xmm1
	add rax,8
	add rbx,8
	sub rcx,1
	jnz .dscaleBlockBop1
	add	rpsp, 32
	jmp	rnext
	
;========================================
	
entry doffsetBlockBop
	; TOS: num offset pDst pSrc
	mov	rax, [rpsp+24]
	mov	rbx, [rpsp+16]
	mov	rcx, [rpsp]
	movsd xmm0, QWORD[rpsp+8]
.doffsetBlockBop1:
    movsd xmm1, QWORD[rax]
    addsd xmm1, xmm0
    movsd QWORD[rbx], xmm1
	add rax,8
	add rbx,8
	sub rcx,1
	jnz .doffsetBlockBop1
	add	rpsp, 32
	jmp	rnext
	
;========================================
	
entry dmixBlockBop
	; TOS: num scale pDst pSrc
	mov	rax, [rpsp+24]
	mov	rbx, [rpsp+16]
	mov	rcx, [rpsp]
	movsd xmm0, QWORD[rpsp+8]
.dmixBlockBop1:
    movsd xmm1, QWORD[rax]
    mulsd xmm1, xmm0
    addsd xmm1, QWORD[rbx]
    movsd QWORD[rbx], xmm1
	add rax,8
	add rbx,8
	sub rcx,1
	jnz .dmixBlockBop1
	add	rpsp, 32
	jmp	rnext
	
;========================================
	
entry fEquals0Bop
    xor rax, rax
    movd xmm0, eax
	jmp	fEqualsBop1
	
entry fEqualsBop
    movss xmm0, DWORD[rpsp]
	add	rpsp, 8
    xor rax, rax
fEqualsBop1:
    movss xmm1, DWORD[rpsp]
    comiss xmm0, xmm1
	jnz	fEqualsBop2
	dec	rax
fEqualsBop2:
	mov	[rpsp], rax
	jmp	rnext
	
;========================================
	
entry fNotEquals0Bop
    xor rax, rax
    movd xmm0, eax
	jmp	fNotEqualsBop1
	
entry fNotEqualsBop
    movss xmm0, DWORD[rpsp]
	add	rpsp, 8
    xor rax, rax
fNotEqualsBop1:
    movss xmm1, DWORD[rpsp]
    comiss xmm0, xmm1
	jz	fNotEqualsBop2
	dec	rax
fNotEqualsBop2:
	mov	[rpsp], rax
	jmp	rnext
	
;========================================
	
entry fGreaterThan0Bop
    xor rax, rax
    movd xmm1, eax
	jmp	fGreaterThanBop1
	
entry fGreaterThanBop
    movss xmm1, DWORD[rpsp]
	add	rpsp, 8
    xor rax, rax
fGreaterThanBop1:
    movss xmm0, DWORD[rpsp]
    comiss xmm0, xmm1
	jbe fGreaterThanBop2
	dec	rax
fGreaterThanBop2:
	mov	[rpsp], rax
	jmp	rnext
	
;========================================
	
entry fGreaterEquals0Bop
    xor rax, rax
    movd xmm1, eax
	jmp	fGreaterEqualsBop1
	
entry fGreaterEqualsBop
    movss xmm1, DWORD[rpsp]
	add	rpsp, 8
    xor rax, rax
fGreaterEqualsBop1:
    movss xmm0, DWORD[rpsp]
    comiss xmm0, xmm1
	jb fGreaterEqualsBop2
	dec	rax
fGreaterEqualsBop2:
	mov	[rpsp], rax
	jmp	rnext
    
	
;========================================
	
entry fLessThan0Bop
    xor rax, rax
    movd xmm1, eax
	jmp	fLessThanBop1
	
entry fLessThanBop
    movss xmm1, DWORD[rpsp]
	add	rpsp, 8
    xor rax, rax
fLessThanBop1:
    movss xmm0, DWORD[rpsp]
    comiss xmm0, xmm1
	jae	fLessThanBop2
	dec	rax
fLessThanBop2:
	mov	[rpsp], rax
	jmp	rnext
    
	
;========================================
	
entry fLessEquals0Bop
    xor rax, rax
    movd xmm1, eax
	jmp	fLessEqualsBop1
	
entry fLessEqualsBop
    movss xmm1, DWORD[rpsp]
	add	rpsp, 8
    xor rax, rax
fLessEqualsBop1:
    movss xmm0, DWORD[rpsp]
    comiss xmm0, xmm1
	ja	fLessEqualsBop2
	dec	rax
fLessEqualsBop2:
	mov	[rpsp], rax
	jmp	rnext
    
	
;========================================
	
entry fWithinBop
	movss xmm0, DWORD[rpsp + 8]       ; low bound
	movss xmm1, DWORD[rpsp + 16]      ; check value
	xor	rbx, rbx
    comiss xmm0, xmm1
	ja fWithinBop2
    movss xmm0, DWORD[rpsp]           ; hi bound
    comiss xmm0, xmm1
	jb fWithinBop2
	dec	rbx
fWithinBop2:
	add	rpsp, 16
	mov	[rpsp], rbx
	jmp	rnext
	
;========================================
	
entry fMinBop
    movss xmm0, DWORD[rpsp]
	add	rpsp, 8
    comiss xmm0, DWORD[rpsp]
    jae fMinBop2
    movss DWORD[rpsp], xmm0
fMinBop2:
	jmp	rnext
	
;========================================
	
entry fMaxBop
    movss xmm0, DWORD[rpsp]
	add	rpsp, 8
    comiss xmm0, DWORD[rpsp]
    jbe fMaxBop2
    movss DWORD[rpsp], xmm0
fMaxBop2:
	jmp	rnext
	
;========================================
	
entry dcmpBop
    movsd xmm0, QWORD[rpsp]
	add	rpsp, 8
    movsd xmm1, QWORD[rpsp]
	xor	rbx, rbx
    comiss xmm0, xmm1
	jz	dcmpBop3
	jg	dcmpBop2
	add	rbx, 2
dcmpBop2:
	dec	rbx
dcmpBop3:
	mov	[rpsp], rbx
	jmp	rnext

;========================================
	
entry fcmpBop
    movss xmm0, DWORD[rpsp]
	add	rpsp, 8
    movss xmm1, DWORD[rpsp]
	xor	rbx, rbx
    comiss xmm0, xmm1
	jz	fcmpBop3
	jg	fcmpBop2
	add	rbx, 2
fcmpBop2:
	dec	rbx
fcmpBop3:
	mov	[rpsp], rbx
	jmp	rnext

;========================================

entry dplusBop
	movsd xmm0, QWORD[rpsp]
	add	rpsp,8
    addsd xmm0, QWORD[rpsp]
    movq rax, xmm0
    mov [rpsp], rax
	jmp	rnext
	
;========================================
	
entry dminusBop
	movsd xmm0, QWORD[rpsp+8]
    movsd xmm1, QWORD[rpsp]
    subsd xmm0, xmm1
    movq rax, xmm0
	add	rpsp,8
    mov [rpsp], rax
	jmp	rnext
	
;========================================
	
entry dtimesBop
	movsd xmm0, QWORD[rpsp]
	add	rpsp,8
    mulsd xmm0, [rpsp]
    movq rax, xmm0
    mov [rpsp], rax
	jmp	rnext
	
;========================================
	
entry ddivideBop
	movsd xmm0, QWORD[rpsp+8]
    movsd xmm1, QWORD[rpsp]
    divsd xmm0, xmm1
    movq rax, xmm0
	add	rpsp,8
    mov [rpsp], rax
	jmp	rnext
	
;========================================
	
entry dEquals0Bop
    xor rax, rax
    movq xmm0, rax
	jmp	dEqualsBop1
	
entry dEqualsBop
    movsd xmm0, QWORD[rpsp]
	add	rpsp, 8
    xor rax, rax
dEqualsBop1:
    movsd xmm1, QWORD[rpsp]
    comisd xmm0, xmm1
	jnz	dEqualsBop2
	dec	rax
dEqualsBop2:
	mov	[rpsp], rax
	jmp	rnext
	
;========================================
	
entry dNotEquals0Bop
    xor rax, rax
    movq xmm0, rax
	jmp	dNotEqualsBop1
	
entry dNotEqualsBop
    movsd xmm0, QWORD[rpsp]
	add	rpsp, 8
    xor rax, rax
dNotEqualsBop1:
    movsd xmm1, QWORD[rpsp]
    comisd xmm0, xmm1
	jz	dNotEqualsBop2
	dec	rax
dNotEqualsBop2:
	mov	[rpsp], rax
	jmp	rnext
	
;========================================
	
entry dGreaterThan0Bop
    xor rax, rax
    movq xmm1, rax
	jmp	dGreaterThanBop1
	
entry dGreaterThanBop
    movsd xmm1, QWORD[rpsp]
	add	rpsp, 8
    xor rax, rax
dGreaterThanBop1:
    movsd xmm0, QWORD[rpsp]
    comisd xmm0, xmm1
	jbe dGreaterThanBop2
	dec	rax
dGreaterThanBop2:
	mov	[rpsp], rax
	jmp	rnext
    
	
;========================================
	
entry dGreaterEquals0Bop
    xor rax, rax
    movq xmm1, rax
	jmp	dGreaterEqualsBop1
	
entry dGreaterEqualsBop
    movsd xmm1, QWORD[rpsp]
	add	rpsp, 8
    xor rax, rax
dGreaterEqualsBop1:
    movsd xmm0, QWORD[rpsp]
    comisd xmm0, xmm1
	jb dGreaterEqualsBop2
	dec	rax
dGreaterEqualsBop2:
	mov	[rpsp], rax
	jmp	rnext
    
	
;========================================
	
entry dLessThan0Bop
    xor rax, rax
    movq xmm1, rax
	jmp	dLessThanBop1
	
entry dLessThanBop
    movsd xmm1, QWORD[rpsp]
	add	rpsp, 8
    xor rax, rax
dLessThanBop1:
    movsd xmm0, QWORD[rpsp]
    comisd xmm0, xmm1
	jae dLessThanBop2
	dec	rax
dLessThanBop2:
	mov	[rpsp], rax
	jmp	rnext
    
	
;========================================
	
entry dLessEquals0Bop
    xor rax, rax
    movq xmm1, rax
	jmp	dLessEqualsBop1
	
entry dLessEqualsBop
    movsd xmm1, QWORD[rpsp]
	add	rpsp, 8
    xor rax, rax
dLessEqualsBop1:
    movsd xmm0, QWORD[rpsp]
    comisd xmm0, xmm1
	ja dLessEqualsBop2
	dec	rax
dLessEqualsBop2:
	mov	[rpsp], rax
	jmp	rnext
    
	
;========================================
	
entry dWithinBop
	movsd xmm0, QWORD[rpsp + 8]       ; low bound
	movsd xmm1, QWORD[rpsp + 16]      ; check value
	xor	rbx, rbx
    comisd xmm0, xmm1
	ja dWithinBop2
    movsd xmm0, QWORD[rpsp]           ; hi bound
    comisd xmm0, xmm1
	jb dWithinBop2
	dec	rbx
dWithinBop2:
	add	rpsp, 16
	mov	[rpsp], rbx
	jmp	rnext

	
;========================================
	
entry dMinBop
    movsd xmm0, QWORD[rpsp]
	add	rpsp, 8
    comisd xmm0, [rpsp]
    jae dMinBop2
    movsd QWORD[rpsp], xmm0
dMinBop2:
	jmp	rnext

;========================================
	
entry dMaxBop
    movsd xmm0, QWORD[rpsp]
	add	rpsp, 8
    comisd xmm0, [rpsp]
    jbe dMaxBop2
    movsd QWORD[rpsp], xmm0
dMaxBop2:
	jmp	rnext
	
;========================================
	
unaryDoubleFunc	dsinBop, sin
unaryDoubleFunc	dasinBop, asin
unaryDoubleFunc	dcosBop, cos
unaryDoubleFunc	dacosBop, acos
unaryDoubleFunc	dtanBop, tan
unaryDoubleFunc	datanBop, atan
unaryDoubleFunc	dsqrtBop, sqrt
unaryDoubleFunc	dexpBop, exp
unaryDoubleFunc	dlnBop, log
unaryDoubleFunc	dlog10Bop, log10
unaryDoubleFunc	dceilBop, ceil
unaryDoubleFunc	dfloorBop, floor

;========================================

unaryFloatFunc	fsinBop, sinf
unaryFloatFunc	fasinBop, asinf
unaryFloatFunc	fcosBop, cosf
unaryFloatFunc	facosBop, acosf
unaryFloatFunc	ftanBop, tanf
unaryFloatFunc	fatanBop, atanf
unaryFloatFunc	fsqrtBop, sqrtf
unaryFloatFunc	fexpBop, expf
unaryFloatFunc	flnBop, logf
unaryFloatFunc	flog10Bop, log10f
unaryFloatFunc	fceilBop, ceilf
unaryFloatFunc	ffloorBop, floorf

;========================================
	
entry datan2Bop
%ifdef LINUX
	push rip
	push rnext
%endif
    movsd xmm1, QWORD[rpsp]
    add rpsp, 8
    movsd xmm0, QWORD[rpsp]
	sub rsp, kShadowSpace			; shadow space
	xcall	atan2
	add rsp, kShadowSpace
    movsd QWORD[rpsp], xmm0
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================
	
entry fatan2Bop
%ifdef LINUX
	push rip
	push rnext
%endif
    movss xmm1, DWORD[rpsp]
    add rpsp, 8
    movss xmm0, DWORD[rpsp]
	sub rsp, kShadowSpace			; shadow space
	xcall	atan2f
	add rsp, kShadowSpace
    movss DWORD[rpsp], xmm0
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================
	
entry dpowBop
	; a^x
%ifdef LINUX
	push rip
	push rnext
%endif
    movsd xmm1, QWORD[rpsp]
    add rpsp, 8
    movsd xmm0, QWORD[rpsp]
	sub rsp, kShadowSpace			; shadow space
	xcall	pow
	add rsp, kShadowSpace
    movsd QWORD[rpsp], xmm0
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================
	
entry fpowBop
	; a^x
%ifdef LINUX
	push rip
	push rnext
%endif
    movss xmm1, DWORD[rpsp]
    add rpsp, 8
    movss xmm0, DWORD[rpsp]
	sub rsp, kShadowSpace			; shadow space
	xcall	powf
	add rsp, kShadowSpace
    xor rax, rax
    movd eax, xmm0
    mov [rpsp], rax
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry dabsBop
	fld	QWORD[rpsp]
	fabs
	fstp QWORD[rpsp]
	jmp	rnext
	
;========================================

entry fabsBop
	fld	DWORD[rpsp]
	fabs
    ; the highword of TOS should already be 0, since it was our float input
	fstp DWORD[rpsp]
	jmp	rnext
	
;========================================

entry dldexpBop
	; ldexp( a, n )
	; TOS is n (int), a (double)
	; get arg a
    movsd xmm0, QWORD[rpsp + 8]
	; get arg n
%ifdef LINUX
	push rip
	push rnext
    mov rdi, [rpsp]
%else
    mov rdx, [rpsp]
%endif
	sub rsp, kShadowSpace			; shadow space
	xcall ldexp
	add rsp, kShadowSpace
	add	rpsp, 8
    movsd QWORD[rpsp], xmm0
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry fldexpBop
	; ldexpf( a, n )
	; TOS is n (int), a (float)
	; get arg a
    movss xmm0, DWORD[rpsp + 8]
	; get arg n
%ifdef LINUX
	push rip
	push rnext
    mov rdi, [rpsp]
%else
    mov rdx, [rpsp]
%endif
	sub rsp, kShadowSpace			; shadow space
	xcall ldexpf
	add rsp, kShadowSpace
	add	rpsp, 8
    xor rax, rax
    movd eax, xmm0
    mov [rpsp], rax
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry dfrexpBop
	; frexp( a, ptrToIntExponentReturn )
	; get arg a
    movsd xmm0, QWORD[rpsp]
	; get arg ptrToIntExponentReturn
    xor rax, rax
    sub rpsp, 8
    mov [rpsp], rax
%ifdef LINUX
	push rip
	push rnext
    mov rdi, rpsp
%else
    mov rdx, rpsp
%endif
	sub rsp, kShadowSpace			; shadow space
	xcall frexp
	add rsp, kShadowSpace
    movsd QWORD[rpsp + 8], xmm0
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry ffrexpBop
	; frexpf( a, ptrToIntExponentReturn )
	; get arg a
    movss xmm0, DWORD[rpsp]
	; get arg ptrToIntExponentReturn
    xor rax, rax
    sub rpsp, 8
    mov [rpsp], rax
%ifdef LINUX
	push rip
	push rnext
    mov rdi, rpsp
%else
    mov rdx, rpsp
%endif
	sub rsp, kShadowSpace			; shadow space
	xcall frexpf
	add rsp, kShadowSpace
    xor rax, rax
    movd eax, xmm0
    mov [rpsp + 8], rax
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry dmodfBop
	; modf( a, ptrToDoubleWholeReturn )
	; get arg a
    movsd xmm0, QWORD[rpsp]
	; get arg ptrToDoubleWholeReturn
%ifdef LINUX
	push rip
	push rnext
    mov rdi, rpsp
%else
    mov rdx, rpsp
%endif
	sub rsp, kShadowSpace			; shadow space
	xcall modf
	add rsp, kShadowSpace
    sub rpsp, 8
    movsd QWORD[rpsp], xmm0
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry fmodfBop
	; modf( a, ptrToFloatWholeReturn )
	; get arg a
    movss xmm0, DWORD[rpsp]
	; get arg ptrToIntExponentReturn
%ifdef LINUX
	push rip
	push rnext
    mov rdi, rpsp
%else
    mov rdx, rpsp
%endif
    xor rax, rax
    mov [rpsp], rax
	sub rsp, kShadowSpace			; shadow space
	xcall modff
	add rsp, kShadowSpace
    sub rpsp, 8
    xor rax, rax
    movd eax, xmm0
    mov [rpsp], rax
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry dfmodBop
    ; fmod(numerator denominator)
    ; get arg denominator
%ifdef LINUX
	push rip
	push rnext
%endif
    movsd xmm1, QWORD[rpsp]
    add rpsp, 8
    ; get arg numerator
    movsd xmm0, QWORD[rpsp]
	sub rsp, kShadowSpace			; shadow space
	xcall fmod
	add rsp, kShadowSpace
    movsd QWORD[rpsp], xmm0
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry ffmodBop
    ; fmodf(numerator denominator)
    ; get arg denominator
%ifdef LINUX
	push rip
	push rnext
%endif
    movss xmm1, DWORD[rpsp]
    add rpsp, 8
    ; get arg numerator
    movss xmm0, DWORD[rpsp]
	sub rsp, kShadowSpace			; shadow space
	xcall fmodf
	add rsp, kShadowSpace
    xor rax, rax
    movd eax, xmm0
    mov [rpsp], rax
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext

;========================================

entry umtimesBop
	mov	rax, [rpsp]
	mul QWORD[rpsp + 8]		; result hiword in rdx, loword in rax
	mov	[rpsp + 8], rax
	mov	[rpsp], rdx
	jmp	rnext

;========================================

entry mtimesBop
	mov	rax, [rpsp]
	imul QWORD[rpsp + 8]	; result hiword in rdx, loword in rax
	mov	[rpsp + 8], rax
	mov	[rpsp], rdx
	jmp	rnext

;========================================

entry i2fBop
; TODO: need to clear hiword on stack?
	cvtsi2ss xmm0, DWORD[rpsp]
    xor rax, rax
    movd eax, xmm0
    mov [rpsp], rax
	jmp	rnext	

;========================================

entry i2dBop
	cvtsi2sd xmm0, DWORD[rpsp]
    movsd QWORD[rpsp], xmm0
	jmp	rnext	

;========================================

entry f2iBop
    xor rax, rax
	cvttss2si eax, DWORD[rpsp]
    mov [rpsp], rax
	jmp	rnext

;========================================

entry f2dBop
    cvtss2sd xmm0, DWORD[rpsp]
    movsd QWORD[rpsp], xmm0
	jmp	rnext
		
;========================================

entry d2iBop
    xor rax, rax
	cvttsd2si eax, QWORD[rax]
    mov [rpsp], eax
	jmp	rnext

;========================================

entry d2fBop
    xor rax, rax
	cvttsd2si eax, QWORD[rpsp]
    mov [rpsp], eax
	jmp	rnext

;========================================

entry doExitBop
	; check param stack
	mov	rbx, [rcore + FCore.STPtr]
	cmp	rbx, rpsp
	jl	doExitBop2
	; check return stack
	mov	rbx, [rcore + FCore.RTPtr]
	cmp	rbx, rrp
	jle	doExitBop1
	mov	rip, [rrp]
	add	rrp, 8
	test	rip, rip
	jz doneBop
	jmp	rnext

doExitBop1:
	mov	rax, kForthErrorReturnStackUnderflow
	jmp	interpFuncErrorExit
	
doExitBop2:
	mov	rax, kForthErrorParamStackUnderflow
	jmp	interpFuncErrorExit
	
;========================================

entry doExitLBop
    ; rstack: local_var_storage oldFP oldIP
    ; FP points to oldFP
	; check param stack
	mov	rbx, [rcore + FCore.STPtr]
	cmp	rbx, rpsp
	jl	doExitBop2
	mov	rax, rfp
	mov	rip, [rax]
	mov	rfp, rip
	add	rax, 8
	; check return stack
	mov	rbx, [rcore + FCore.RTPtr]
	cmp	rbx, rax
	jle	doExitBop1
	mov	rip, [rax]
	add	rax, 8
	mov	rrp, rax
	test	rip, rip
	jz doneBop
	jmp	rnext
	
;========================================


entry doExitMBop
    ; rstack: oldIP oldTP
	; check param stack
	mov	rbx, [rcore + FCore.STPtr]
	cmp	rbx, rpsp
	jl	doExitBop2
	mov	rax, rrp
	mov	rbx, [rcore + FCore.RTPtr]
	add	rax, 16
	; check return stack
	cmp	rbx, rax
	jl	doExitBop1
	mov	rrp, rax
	mov	rip, [rax - 16]	; IP = oldIP
	mov	rbx, [rax - 8]
	mov	[rcore + FCore.TPtr], rbx
	test	rip, rip
	jz doneBop
	jmp	rnext

;========================================

entry doExitMLBop
    ; rstack: local_var_storage oldFP oldIP oldTP
    ; FP points to oldFP
	; check param stack
	mov	rbx, [rcore + FCore.STPtr]
	cmp	rbx, rpsp
	jl	doExitBop2
	mov	rax, rfp
	mov	rfp, [rax]
	mov	[rcore + FCore.FPtr], rfp
	add	rax, 24
	; check return stack
	mov	rbx, [rcore + FCore.RTPtr]
	cmp	rbx, rax
	jl	doExitBop1
	mov	rrp, rax
	mov	rip, [rax - 16]	; IP = oldIP
	mov	rbx, [rax - 8]
	mov	[rcore + FCore.TPtr], rbx
	test	rip, rip
	jz doneBop
	jmp	rnext
	
;========================================

entry callBop
	; rpush current IP
	sub	rrp, 8
	mov	[rrp], rip
	; pop new IP
	mov	rip, [rpsp]
	add	rpsp, 8
	test	rip, rip
	jz doneBop
	jmp	rnext
	
;========================================

entry gotoBop
	mov	rip, [rpsp]
	test	rip, rip
	jz doneBop
	jmp	rnext

;========================================
;
; TOS is start-index
; TOS+8 is end-index
; the op right after this one should be a branch just past end of loop (used by leave)
; 
entry doDoBop
	sub	rrp, 24
	; @RP-2 holds top-of-loop-IP
	add	rip, 4    ; skip over loop exit branch right after this op
	mov	[rrp + 16], rip
	; @RP-1 holds end-index
	mov	rax, [rpsp + 8]
	mov	[rrp + 8], rax
	; @RP holds current-index
	mov	rax, [rpsp]
	mov	[rrp], rax
	add	rpsp, 16
	jmp	rnext
	
;========================================
;
; TOS is start-index
; TOS+8 is end-index
; the op right after this one should be a branch just past end of loop (used by leave)
; 
entry doCheckDoBop
	mov	rax, [rpsp]		; rax is start index
	mov	rcx, [rpsp + 8]	; rcx is end index
	add	rpsp, 16
	cmp	rax, rcx
	jge	doCheckDoBop1
	
	sub	rrp, 24
	; @RP-2 holds top-of-loop-IP
	add	rip, 4    ; skip over loop exit branch right after this op
	mov	[rrp + 16], rip
	; @RP-1 holds end-index
	mov	[rrp + 8], rcx
	; @RP holds current-index
	mov	[rrp], rax
doCheckDoBop1:
	jmp	rnext
	
;========================================
;
; TOS is start-index
; the op right after this one should be a branch just past end of loop (used by leave)
; 
entry doForBop
	sub	rrp, 24
	; @RP-2 holds top-of-loop-IP
	add	rip, 4    ; skip over loop exit branch right after this op
	mov	[rrp + 16], rip
	; @RP holds current-index
	mov	rax, [rpsp]
	mov	[rrp], rax
	add	rpsp, 8
	jmp	rnext
	
;========================================

entry doLoopBop
	mov	rax, [rrp]
	inc	rax
	cmp	rax, [rrp + 8]
	jge	doLoopBop1	; jump if done
	mov	[rrp], rax
	mov	rip, [rrp + 16]
	jmp	rnext

doLoopBop1:
	add	rrp, 24
	jmp	rnext
	
;========================================

entry doLoopNBop
	mov	rax, [rpsp]		; pop N into rax
	add	rpsp, 8
	or	rax, rax
	jl	doLoopNBop1		; branch if increment negative
	add	rax, [rrp]		; rax is new i
	cmp	rax, [rrp + 8]
	jge	doLoopBop1		; jump if done
	mov	[rrp], rax		; update i
	mov	rip, [rrp + 16]		; branch to top of loop
	jmp	rnext

doLoopNBop1:
	add	rax, [rrp]
	cmp	rax, [rrp + 8]
	jl	doLoopBop1		; jump if done
	mov	[rrp], rax		; update i
	mov	rip, [rrp + 16]		; branch to top of loop
	jmp	rnext
	
;========================================

entry doNextBop
	mov	rax, [rrp]
	dec	rax
	jl	doNextBop1	; jump if done
	mov	[rrp], rax
	mov	rip, [rrp + 16]
	jmp	rnext

doNextBop1:
	add	rrp, 24
	jmp	rnext
	
;========================================

entry iBop
	mov	rbx, [rrp]
	sub	rpsp, 8
	mov	[rpsp], rbx
	jmp	rnext
	
;========================================

entry jBop
	mov	rbx, [rrp + 24]
	sub	rpsp, 8
	mov	[rpsp], rbx
	jmp	rnext
	
;========================================

entry unloopBop
	add	rrp, 24
	jmp	rnext
	
;========================================

entry leaveBop
	; point IP at the branch instruction which is just before top of loop
	mov	rip, [rrp + 16]
	sub	rip, 4
	; drop current index, end index, top-of-loop-IP
	add	rrp, 24
	jmp	rnext
	
;========================================

entry orBop
	mov	rax, [rpsp]
	add	rpsp, 8
	or	[rpsp], rax
	jmp	rnext
	
;========================================

entry andBop
	mov	rax, [rpsp]
	add	rpsp, 8
	and	[rpsp], rax
	jmp	rnext
	
;========================================

entry xorBop
	mov	rax, [rpsp]
	add	rpsp, 8
	xor	[rpsp], rax
	jmp	rnext
	
;========================================

entry invertBop
	mov	rax, -1
	xor	[rpsp], rax
	jmp	rnext
	
;========================================

entry lshiftBop
	mov	rcx, [rpsp]
	add	rpsp, 8
	mov	rbx, [rpsp]
	shl	rbx, cl
	mov	[rpsp], rbx
	jmp	rnext
	
;========================================

entry arshiftBop
	mov	rcx, [rpsp]
	add	rpsp, 8
	mov	rbx, [rpsp]
	sar	rbx, cl
	mov	[rpsp], rbx
	jmp	rnext
	
;========================================

entry rshiftBop
	mov	rcx, [rpsp]
	add	rpsp, 8
	mov	rbx, [rpsp]
	shr	rbx, cl
	mov	[rpsp], rbx
	jmp	rnext
	
;========================================

entry rotateBop
	mov	rcx, [rpsp]
	add	rpsp, 8
	mov	rbx, [rpsp]
	and	cl, 03Fh
	rol	rbx, cl
	mov	[rpsp], rbx
	jmp	rnext
	
;========================================
entry reverseBop
    ; TODO!
    ; Knuth's algorithm
    ; a = (a << 15) | (a >> 17);
	mov	rax, [rpsp]
	rol	rax, 15
    ; b = (a ^ (a >> 10)) & 0x003f801f;
	mov	rbx, rax
	shr	rbx, 10
	xor	rbx, rax
	and	rbx, 03F801Fh
    ; a = (b + (b << 10)) ^ a;
	mov	rcx, rbx
	shl	rcx, 10
	add	rcx, rbx
	xor	rax, rcx
    ; b = (a ^ (a >> 4)) & 0x0e038421;
	mov	rbx, rax
	shr	rbx, 4
	xor	rbx, rax
	and	rbx, 0E038421h
    ; a = (b + (b << 4)) ^ a;
	mov	rcx, rbx
	shl	rcx, 4
	add	rcx, rbx
	xor	rax, rcx
    ; b = (a ^ (a >> 2)) & 0x22488842;
	mov	rbx, rax
	shr	rbx, 2
	xor	rbx, rax
	and	rbx, 022488842h
    ; a = (b + (b << 2)) ^ a;
	mov	rcx, rbx
	shl rcx, 2
	add	rbx, rcx
	xor	rax, rbx
	mov	[rpsp], rax
	jmp rnext

;========================================

entry countLeadingZerosBop
	mov	rax, [rpsp]
    ; NASM on Mac wouldn't do tzcnt or lzcnt, so I had to use bsr
%ifdef MACOSX
    mov rbx, 64
    or  rax, rax
    jz  clz1
    bsr rcx, rax
    mov rbx, 63
    sub rbx, rcx
clz1:
%else
    lzcnt    rbx, rax
%endif
	mov	[rpsp], rbx
	jmp rnext

;========================================

entry countTrailingZerosBop
	mov	rax, [rpsp]
    ; NASM on Mac wouldn't do tzcnt or lzcnt, so I had to use bsf
%ifdef MACOSX
    mov rbx, 64
    or  rax, rax
    jz  ctz1
    bsf rbx, rax
ctz1:
%else
    tzcnt	rbx, rax
%endif
	mov	[rpsp], rbx
	jmp rnext

;========================================

entry archX86Bop
entry trueBop
	mov	rax, -1
	sub	rpsp, 8
	mov	[rpsp], rax
	jmp	rnext
	
;========================================

entry archARMBop
entry falseBop
	xor	rax, rax
	sub	rpsp, 8
	mov	[rpsp], rax
	jmp	rnext
	
;========================================

entry nullBop
	xor	rax, rax
	sub	rpsp, 8
	mov	[rpsp], rax
	jmp	rnext
	
;========================================

entry dnullBop
	xor	rax, rax
	sub	rpsp, 16
	mov	[rpsp + 8], rax
	mov	[rpsp], rax
	jmp	rnext
	
;========================================

entry equals0Bop
	xor	rbx, rbx
	jmp	equalsBop1
	
;========================================

entry equalsBop
	mov	rbx, [rpsp]
	add	rpsp, 8
equalsBop1:
	xor	rax, rax
	cmp	rbx, [rpsp]
	jnz	equalsBop2
	dec	rax
equalsBop2:
	mov	[rpsp], rax
	jmp	rnext
	
;========================================

entry notEquals0Bop
	xor	rbx, rbx
	jmp	notEqualsBop1
	
;========================================

entry notEqualsBop
	mov	rbx, [rpsp]
	add	rpsp, 8
notEqualsBop1:
	xor	rax, rax
	cmp	rbx, [rpsp]
	jz	notEqualsBop2
	dec	rax
notEqualsBop2:
	mov	[rpsp], rax
	jmp	rnext
	
;========================================

entry greaterThan0Bop
	xor	rbx, rbx
	jmp	gtBop1
	
;========================================

entry greaterThanBop
	mov	rbx, [rpsp]		; rbx = b
	add	rpsp, 8
gtBop1:
	xor	rax, rax
	cmp	[rpsp], rbx
	jle	gtBop2
	dec	rax
gtBop2:
	mov	[rpsp], rax
	jmp	rnext

;========================================

entry greaterEquals0Bop
	xor	rbx, rbx
	jmp	geBop1
	
;========================================

entry greaterEqualsBop
	mov	rbx, [rpsp]
	add	rpsp, 8
geBop1:
	xor	rax, rax
	cmp	[rpsp], rbx
	jl	geBop2
	dec	rax
geBop2:
	mov	[rpsp], rax
	jmp	rnext
	

;========================================

entry lessThan0Bop
	xor	rbx, rbx
	jmp	ltBop1
	
;========================================

entry lessThanBop
	mov	rbx, [rpsp]
	add	rpsp, 8
ltBop1:
	xor	rax, rax
	cmp	[rpsp], rbx
	jge	ltBop2
	dec	rax
ltBop2:
	mov	[rpsp], rax
	jmp	rnext
	
;========================================

entry lessEquals0Bop
	xor	rbx, rbx
	jmp	leBop1
	
;========================================

entry lessEqualsBop
	mov	rbx, [rpsp]
	add	rpsp, 8
leBop1:
	xor	rax, rax
	cmp	[rpsp], rbx
	jg	leBop2
	dec	rax
leBop2:
	mov	[rpsp], rax
	jmp	rnext
	
;========================================

entry unsignedGreaterThanBop
	mov	rbx, [rpsp]
	add	rpsp, 8
ugtBop1:
	xor	rax, rax
	cmp	[rpsp], rbx
	jbe	ugtBop2
	dec	rax
ugtBop2:
	mov	[rpsp], rax
	jmp	rnext
	
;========================================

entry unsignedLessThanBop
	mov	rbx, [rpsp]
	add	rpsp, 8
ultBop1:
	xor	rax, rax
	cmp	[rpsp], rbx
	jae	ultBop2
	dec	rax
ultBop2:
	mov	[rpsp], rax
	jmp	rnext
	
;========================================

entry withinBop
	; tos: hiLimit loLimit value
	xor	rax, rax
	mov	rbx, [rpsp + 16]	; rbx = value
withinBop1:
	cmp	[rpsp], rbx
	jle	withinFail
	cmp	[rpsp + 8], rbx
	jg	withinFail
	dec	rax
withinFail:
	add rpsp, 16
	mov	[rpsp], rax		
	jmp	rnext
	
;========================================

entry clampBop
	; tos: hiLimit loLimit value
	mov	rbx, [rpsp + 16]    ; rbx = value
	mov	rax, [rpsp]         ; rax = hiLimit
	cmp	rax, rbx
	jle clampFail
	mov	rax, [rpsp + 8]
	cmp	rax, rbx
	jg	clampFail
	mov	rax, rbx            ; value was within range
clampFail:
	add rpsp, 16
	mov	[rpsp], rax		
	jmp	rnext
	
;========================================

entry minBop
	mov	rbx, [rpsp]
	add	rpsp, 8
	cmp	[rpsp], rbx
	jl	minBop1
	mov	[rpsp], rbx
minBop1:
	jmp	rnext
	
;========================================

entry maxBop
	mov	rbx, [rpsp]
	add	rpsp, 8
	cmp	[rpsp], rbx
	jg	maxBop1
	mov	[rpsp], rbx
maxBop1:
	jmp	rnext
	
;========================================

entry icmpBop
	mov	ebx, DWORD[rpsp]		; ebx = b
	add	rpsp, 8
	xor	rax, rax
	cmp	DWORD[rpsp], ebx
	jz	icmpBop3
	jl	icmpBop2
	add	rax, 2
icmpBop2:
	dec	rax
icmpBop3:
	mov	[rpsp], rax
	jmp	rnext

;========================================

entry uicmpBop
	mov	ebx, DWORD[rpsp]
	add	rpsp, 8
	xor	rax, rax
	cmp	DWORD[rpsp], ebx
	jz	uicmpBop3
	jb	uicmpBop2
	add	rax, 2
uicmpBop2:
	dec	rax
uicmpBop3:
	mov	[rpsp], rax
	jmp	rnext
	
;========================================

entry lcmpBop
	mov	rbx, [rpsp]		; rbx = b
	add	rpsp, 8
	xor	rax, rax
	cmp	[rpsp], rbx
	jz	lcmpBop3
	jl	lcmpBop2
	add	rax, 2
lcmpBop2:
	dec	rax
lcmpBop3:
	mov	[rpsp], rax
	jmp	rnext

;========================================

entry ulcmpBop
	mov	rbx, [rpsp]
	add	rpsp, 8
	xor	rax, rax
	cmp	[rpsp], rbx
	jz	ulcmpBop3
	jb	ulcmpBop2
	add	rax, 2
ulcmpBop2:
	dec	rax
ulcmpBop3:
	mov	[rpsp], rax
	jmp	rnext
	
;========================================

entry rpushBop
	mov	rbx, [rpsp]
	add	rpsp, 8
	sub	rrp, 8
	mov	[rrp], rbx
	jmp	rnext
	
;========================================

entry rpopBop
	mov	rbx, [rrp]
	add	rrp, 8
	sub	rpsp, 8
	mov	[rpsp], rbx
	jmp	rnext
	
;========================================

entry rpeekBop
	mov	rbx, [rrp]
	sub	rpsp, 8
	mov	[rpsp], rbx
	jmp	rnext
	
;========================================

entry rdropBop
	add	rrp, 8
	jmp	rnext
	
;========================================

entry rpBop
	lea	rax, [rcore + FCore.RPtr]
	jmp	longEntry
	
;========================================

entry r0Bop
	mov	rax, [rcore + FCore.RTPtr]
	sub	rpsp, 8
	mov	[rpsp], rax
	jmp	rnext
	
;========================================

entry dupBop
	mov	rax, [rpsp]
	sub	rpsp, 8
	mov	[rpsp], rax
	jmp	rnext

;========================================

entry checkDupBop
	mov	rax, [rpsp]
	or	rax, rax
	jz	dupNon0Bop1
	sub	rpsp, 8
	mov	[rpsp], rax
dupNon0Bop1:
	jmp	rnext

;========================================

entry swapBop
	mov	rax, [rpsp]
	mov	rbx, [rpsp + 8]
	mov	[rpsp], rbx
	mov	[rpsp + 8], rax
	jmp	rnext
	
;========================================

entry dropBop
	add	rpsp, 8
	jmp	rnext
	
;========================================

entry overBop
	mov	rax, [rpsp + 8]
	sub	rpsp, 8
	mov	[rpsp], rax
	jmp	rnext
	
;========================================

entry rotBop
	mov	rax, [rpsp]         ; tos[0], will go in tos[1]
	mov	rbx, [rpsp + 16]    ; tos[2], will go in tos[0]
	mov	[rpsp], rbx
	mov	rbx, [rpsp + 8]     ; tos[1], will go in tos[2]
	mov	[rpsp + 16], rbx
	mov	[rpsp + 8], rax
	jmp	rnext
	
;========================================

entry reverseRotBop
	mov	rax, [rpsp]         ; tos[0], will go in tos[2]
	mov	rbx, [rpsp + 8]     ; tos[1], will go in tos[0]
	mov	[rpsp], rbx
	mov	rbx, [rpsp + 16]    ; tos[2], will go in tos[1]
	mov	[rpsp + 8], rbx
	mov	[rpsp + 16], rax
	jmp	rnext
	
;========================================

entry nipBop
	mov	rax, [rpsp]
	add	rpsp, 8
	mov	[rpsp], rax
	jmp	rnext
	
;========================================

entry tuckBop
	mov	rax, [rpsp]
	mov	rbx, [rpsp + 8]
	sub	rpsp, 8
	mov	[rpsp], rax
	mov	[rpsp + 8], rbx
	mov	[rpsp + 16], rax
	jmp	rnext

;========================================

entry pickBop
	mov rax, [rpsp]
	add rax, 1
	mov	rbx, [rpsp + rax*8]
	mov	[rpsp], rbx
	jmp	rnext
	
;========================================

entry spBop
	; this is overkill to make sp look like other vars
	mov	rbx, [rcore + FCore.varMode]
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	cmp	rbx, kVarSetMinus
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, spActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx
	
spFetch:
	mov	rax, rpsp
	sub	rpsp, 8
	mov	[rpsp], rax
	jmp	rnext

spRef:
	; returns address of SP shadow copy
	lea	rax, [rcore + FCore.SPtr]
	sub	rpsp, 8
	mov	[rpsp], rax
	mov [rcore + FCore.SPtr], rpsp
	jmp	rnext
	
spStore:
	mov	rbx, [rpsp]
	mov	rpsp, rbx
	jmp	rnext

spPlusStore:
	mov	rax, [rpsp]
	add	rpsp, 8
	add	rpsp, rax
	jmp	rnext

spMinusStore:
	mov	rax, [rpsp]
	add	rpsp, 8
	sub	rpsp, rax
	jmp	rnext

spActionTable:
	DQ	spFetch
	DQ	spFetch
	DQ	spRef
	DQ	spStore
	DQ	spPlusStore
	DQ	spMinusStore

	
;========================================

entry s0Bop
	mov	rax, [rcore + FCore.STPtr]
	sub	rpsp, 8
	mov	[rpsp], rax
	jmp	rnext
	
;========================================

entry fpBop
	lea	rax, [rcore + FCore.FPtr]
	jmp	longEntry
	
;========================================

entry ipBop
	; let the common intVarAction code change the shadow copy of IP,
	; then jump back to ipFixup to copy the shadow copy of IP into IP register (rip)
	push	rnext
	mov	[rcore + FCore.IPtr], rip
	lea	rax, [rcore + FCore.IPtr]
	mov	rnext, ipFixup
	jmp	longEntry
	
entry	ipFixup	
	mov	rip, [rcore + FCore.IPtr]
	pop	rnext
	jmp	rnext
	
;========================================

entry ddupBop
	mov	rax, [rpsp]
	mov	rbx, [rpsp + 8]
	sub	rpsp, 16
	mov	[rpsp], rax
	mov	[rpsp + 8], rbx
	jmp	rnext
	
;========================================

entry dswapBop
	mov	rax, [rpsp]
	mov	rbx, [rpsp + 16]
	mov	[rpsp + 16], rax
	mov	[rpsp], rbx
	mov	rax, [rpsp + 8]
	mov	rbx, [rpsp + 24]
	mov	[rpsp + 24], rax
	mov	[rpsp + 8], rbx
	jmp	rnext
	
;========================================

entry ddropBop
	add	rpsp, 16
	jmp	rnext
	
;========================================

entry doverBop
	mov	rax, [rpsp + 16]
	mov	rbx, [rpsp + 24]
	sub	rpsp, 16
	mov	[rpsp], rax
	mov	[rpsp + 8], rbx
	jmp	rnext
	
;========================================

entry drotBop
	mov	rax, [rpsp + 40]
	mov	rbx, [rpsp + 24]
	mov	[rpsp + 40], rbx
	mov	rbx, [rpsp + 8]
	mov	[rpsp + 24], rbx
	mov	[rpsp + 8], rax
	mov	rax, [rpsp + 32]
	mov	rbx, [rpsp + 16]
	mov	[rpsp + 32], rbx
	mov	rbx, [rpsp]
	mov	[rpsp + 16], rbx
	mov	[rpsp], rax
	jmp	rnext
	
;========================================

entry startTupleBop
	sub	rrp, 8
	mov	[rrp], rpsp
	jmp	rnext
	
;========================================

entry endTupleBop
	mov	rbx, [rrp]
	add	rrp, 8
	sub	rbx, rpsp
	sub	rpsp, 8
	sar	rbx, 3
	mov	[rpsp], rbx
	jmp	rnext
	
;========================================

entry hereBop
	mov	rax, [rcore + FCore.DictionaryPtr]
    mov	rbx, [rax + MemorySection.pCurrent]
    sub rpsp, 8
    mov [rpsp], rbx
    jmp rnext

;========================================

entry dpBop
    mov	rax, [rcore + FCore.DictionaryPtr]
    lea	rbx, [rax + MemorySection.pCurrent]
    sub rpsp, 8
    mov [rpsp], rbx
    jmp rnext

;========================================

entry lstoreBop
	mov	rax, [rpsp]
	mov	rbx, [rpsp + 8]
	mov	[rax], rbx
	add	rpsp, 16
	jmp	rnext
	
;========================================

entry lstoreNextBop
	mov	rax, [rpsp]         ; rax -> dst ptr
	mov	rcx, [rax]
	mov	rbx, [rpsp + 8]     ; rbx is value to store
	mov	[rcx], rbx
	add	rcx, 8
	mov	[rax], rcx
	add	rpsp, 16
	jmp	rnext
	
;========================================

entry dfetchBop
entry lfetchBop
	mov	rax, [rpsp]
	mov	rbx, [rax]
	mov	[rpsp], rbx
	jmp	rnext
	
;========================================

entry lfetchNextBop
	mov	rax, [rpsp]		; rax -> src ptr
	mov	rcx, [rax]		; rcx is src ptr
	mov	rbx, [rcx]
	mov	[rpsp], rbx
	add	rcx, 8
	mov	[rax], rcx
	jmp	rnext
	
;========================================

entry plusStoreCellBop
	mov	rax, [rpsp]
	mov rbx, [rpsp+8]
	add rpsp, 16
	add rbx, [rax]
	mov [rax], rbx
	jmp	rnext
	
;========================================

entry plusStoreAtomicCellBop
	mov	rax, [rpsp]
	add rpsp, 8
	mov	rbx, [rpsp]
	mov rcx, rbx
	lock xadd QWORD[rax], rbx
	add rbx, rcx
	mov	[rpsp], rbx
	jmp	rnext
	
;========================================

entry istoreBop
	mov	rax, [rpsp]
	mov	rbx, [rpsp + 8]
	add	rpsp, 16
	mov	DWORD[rax], ebx
	jmp	rnext
	
;========================================

entry ifetchBop
	mov	rax, [rpsp]
	movsx rbx, DWORD[rax]
	mov	[rpsp], rbx
	jmp	rnext
	
;========================================

entry istoreNextBop
	mov	rax, [rpsp]         ; rax -> dst ptr
	mov	rcx, [rax]
	mov	rbx, [rpsp + 8]
	add	rpsp, 16
	mov	DWORD[rcx], ebx
	add	rcx, 4
	mov	[rax], rcx
	jmp	rnext
	
;========================================

entry ifetchNextBop
	mov	rax, [rpsp]
	mov	rcx, [rax]
	movsx rbx, DWORD[rcx]
	mov	[rpsp], rbx
	add	rcx, 4
	mov	[rax], rcx
	jmp	rnext
	
;========================================

entry ubfetchBop
	mov	rax, [rpsp]
	xor rbx, rbx
    mov bl, BYTE[rax]
	mov	[rpsp], rbx
	jmp	rnext
	
;========================================

entry bstoreBop
	mov	rax, [rpsp]
    xor rbx, rbx
	mov	bl, BYTE[rpsp + 8]
	add	rpsp, 16
	mov	[rax], bl
	jmp	rnext
	
;========================================

entry bfetchBop
	mov	rax, [rpsp]
	movsx rbx, BYTE[rax]
	mov	[rpsp], rbx
	jmp	rnext
	
;========================================

entry bstoreNextBop
	mov	rax, [rpsp]		; rax -> dst ptr
	mov	rcx, [rax]
	xor rbx, rbx
    mov bl, BYTE[rpsp + 8]
	add	rpsp, 16
	mov	BYTE[rcx], bl
	add	rcx, 1
	mov	[rax], rcx
	jmp	rnext
	
;========================================

entry bfetchNextBop
	mov	rax, [rpsp]
	mov	rcx, [rax]
	movsx rbx, BYTE[rcx]
	mov	[rpsp], rbx
	add	rcx, 1
	mov	[rax], rcx
	jmp	rnext
	
;========================================

entry sstoreBop
	mov	rax, [rpsp]
	mov	bx, [rpsp + 8]
	add	rpsp, 16
	mov	WORD[rax], bx
	jmp	rnext
	
;========================================

entry sstoreNextBop
	mov	rax, [rpsp]		; rax -> dst ptr
	mov	rcx, [rax]
	mov	rbx, [rpsp + 8]
	add	rpsp, 16
	mov	WORD[rcx], bx
	add	rcx, 2
	mov	[rax], rcx
	jmp	rnext
	
;========================================

entry sfetchBop
	mov	rax, [rpsp]
	movsx rbx, WORD[rax]
	mov	[rpsp], rbx
	jmp	rnext
	
;========================================

entry sfetchNextBop
	mov	rax, [rpsp]
	mov	rcx, [rax]
	movsx rbx, WORD[rcx]
	mov	[rpsp], rbx
	add	rcx, 2
	mov	[rax], rcx
	jmp	rnext
	
;========================================

entry moveBop
	;	TOS: nBytes dstPtr srcPtr
%ifdef LINUX
	push rip
	push rnext
	mov	rdx, [rpsp]
	mov	rsi, [rpsp + 16]
	mov	rdi, [rpsp + 8]
%else
	mov	r8, [rpsp]
	mov	rdx, [rpsp + 16]
	mov	rcx, [rpsp + 8]
%endif
	sub rsp, kShadowSpace			; shadow space
	xcall	memmove
	add rsp, kShadowSpace
	add	rpsp, 24
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext

;========================================

entry memcmpBop
	;	TOS: nBytes mem2Ptr mem1Ptr
%ifdef LINUX
	push rip
	push rnext
	mov	rdx, [rpsp]
	mov	rsi, [rpsp + 8]
	mov	rdi, [rpsp + 16]
%else
	mov	r8, [rpsp]
	mov	rdx, [rpsp + 8]
	mov	rcx, [rpsp + 16]
%endif
	sub rsp, kShadowSpace			; shadow space
	xcall	memcmp
	add rsp, kShadowSpace
	add	rpsp, 16
    mov [rpsp], rax
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext

;========================================

entry fillBop
	;	TOS: byteVal nBytes dstPtr
%ifdef LINUX
	push rip
	push rnext
	mov	rsi, [rpsp]			; arg1
	mov	rdx, [rpsp + 8]		; arg2
	mov	rdi, [rpsp + 16]	; arg0
%else
	mov	rdx, [rpsp]			; arg1
	mov	r8, [rpsp + 8]		; arg2
	mov	rcx, [rpsp + 16]	; arg0
%endif
	sub rsp, kShadowSpace			; shadow space
	xcall	memset
	add rsp, kShadowSpace
	add	rpsp, 24
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext

;========================================

entry fetchVaractionBop
	mov	rax, kVarGet
	mov	[rcore + FCore.varMode], rax
	jmp	rnext
	
;========================================

entry intoVaractionBop
	mov	rax, kVarSet
	mov	[rcore + FCore.varMode], rax
	jmp	rnext
	
;========================================

entry addToVaractionBop
	mov	rax, kVarSetPlus
	mov	[rcore + FCore.varMode], rax
	jmp	rnext
	
;========================================

entry subtractFromVaractionBop
	mov	rax, kVarSetMinus
	mov	[rcore + FCore.varMode], rax
	jmp	rnext
	
;========================================

entry oclearVaractionBop
	mov	rax, kVarClear
	mov	[rcore + FCore.varMode], rax
	jmp	rnext
		
;========================================

entry odropBop
	; TOS is object to check - if its refcount is already 0, invoke delete method
	;  otherwise do nothing
	mov	rax, [rpsp]
	add	rpsp, 8
	; TODO: how should atomic refcounts be handled here?
	mov	rbx, [rax + Object.refCount]
	or	rbx, rbx
	jnz .odrop1

	; refcount is 0, delete the object
	; push this ptr on return stack
	sub	rrp, 8
	mov	rcx, [rcore + FCore.TPtr]
	mov	[rrp], rcx

	; set this to object to delete
	mov	[rcore + FCore.TPtr], rax

	mov	rbx, [rax]	; rbx = methods pointer
	mov	ebx, [rbx]	; rbx = method 0 (delete)

	; execute the delete method opcode which is in rbx
	mov	rax, [rcore + FCore.innerExecute]
	jmp rax

.odrop1:
	jmp rnext

;========================================

entry refVaractionBop
	mov	rax, kVarRef
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

;========================================

entry setVaropBop
	mov   rax, [rpsp]
	add   rpsp, 8
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

;========================================

entry getVaropBop
	mov	rax, [rcore + FCore.varMode]
	sub   rpsp, 8
	mov   [rpsp], rax
	jmp	rnext

;========================================

entry byteVarActionBop
	mov	rax, [rpsp]
	add	rpsp, 8
	jmp	byteEntry
	
;========================================

entry ubyteVarActionBop
	mov	rax, [rpsp]
	add	rpsp, 8
	jmp	ubyteEntry
	
;========================================

entry shortVarActionBop
	mov	rax, [rpsp]
	add	rpsp, 8
	jmp	shortEntry
	
;========================================

entry ushortVarActionBop
	mov	rax, [rpsp]
	add	rpsp, 8
	jmp	ushortEntry
	
;========================================

entry intVarActionBop
	mov	rax, [rpsp]
	add	rpsp, 8
	jmp	intEntry
	
;========================================

entry uintVarActionBop
	mov	rax, [rpsp]
	add	rpsp, 8
	jmp	uintEntry
	
;========================================

entry floatVarActionBop
	mov	rax, [rpsp]
	add	rpsp, 8
	jmp	floatEntry
	
;========================================

entry doubleVarActionBop
	mov	rax, [rpsp]
	add	rpsp, 8
	jmp	doubleEntry
	
;========================================

entry longVarActionBop
	mov	rax, [rpsp]
	add	rpsp, 8
	jmp	longEntry
	
;========================================

entry opVarActionBop
	mov	rax, [rpsp]
	add	rpsp, 8
	jmp	opEntry
	
;========================================

entry objectVarActionBop
	mov	rax, [rpsp]
	add	rpsp, 8
	jmp	objectEntry
	
;========================================

entry stringVarActionBop
	mov	rax, [rpsp]
	add	rpsp, 8
	jmp	stringEntry
	
;========================================

entry strcpyBop
	;	TOS: srcPtr dstPtr
%ifdef LINUX
	push rip
	push rnext
	mov	rsi, [rpsp]
	mov	rdi, [rpsp + 8]
%else
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
%endif
	sub rsp, kShadowSpace			; shadow space
	xcall	strcpy
	add rsp, kShadowSpace
	add	rpsp, 16
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext

;========================================

entry strncpyBop
	;	TOS: maxBytes srcPtr dstPtr
%ifdef LINUX
	push rip
	push rnext
	mov	rdx, [rpsp]
	mov	rsi, [rpsp + 8]
	mov	rdi, [rpsp + 16]
%else
	mov	r8, [rpsp]
	mov	rdx, [rpsp + 8]
	mov	rcx, [rpsp + 16]
%endif
	sub rsp, kShadowSpace			; shadow space
	xcall	strncpy
	add rsp, kShadowSpace
	add	rpsp, 24
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext

;========================================

entry strlenBop
	mov	rax, [rpsp]
	mov rcx, rax
	xor	rbx, rbx
strlenBop1:
	mov	bl, [rax]
	test bl, 255
	jz	strlenBop2
	add	rax, 1
	jmp	strlenBop1

strlenBop2:
	sub rax, rcx
	mov	[rpsp], rax
	jmp	rnext

;========================================

entry strcatBop
	;	TOS: srcPtr dstPtr
%ifdef LINUX
	push rip
	push rnext
	mov	rsi, [rpsp]
	mov	rdi, [rpsp + 8]
%else
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
%endif
	sub rsp, kShadowSpace			; shadow space
	xcall	strcat
	add rsp, kShadowSpace
	add	rpsp, 16
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext

;========================================

entry strncatBop
	;	TOS: maxBytes srcPtr dstPtr
%ifdef LINUX
	push rip
	push rnext
	mov	rdx, [rpsp]
	mov	rsi, [rpsp + 8]
	mov	rdi, [rpsp + 16]
%else
	mov	r8, [rpsp]
	mov	rdx, [rpsp + 8]
	mov	rcx, [rpsp + 16]
%endif
	sub rsp, kShadowSpace			; shadow space
	xcall	strncat
	add rsp, kShadowSpace
	add	rpsp, 24
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext

;========================================

entry strchrBop
	;	TOS: char strPtr
%ifdef LINUX
	push rip
	push rnext
	mov	rsi, [rpsp]
	mov	rdi, [rpsp + 8]
%else
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
%endif
	sub rsp, kShadowSpace			; shadow space
	xcall	strchr
	add rsp, kShadowSpace
	add	rpsp, 8
	mov	[rpsp], rax
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry strrchrBop
	;	TOS: char strPtr
%ifdef LINUX
	push rip
	push rnext
	mov	rsi, [rpsp]
	mov	rdi, [rpsp + 8]
%else
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
%endif
	sub rsp, kShadowSpace			; shadow space
	xcall	strrchr
	add rsp, kShadowSpace
	add	rpsp, 8
%ifdef LINUX
	pop rnext
	pop rip
%endif
	mov	[rpsp], rax
	jmp	restoreNext
	
;========================================

entry strcmpBop
	;	TOS: ptr2 ptr1
%ifdef LINUX
	push rip
	push rnext
	mov	rsi, [rpsp]
	mov	rdi, [rpsp + 8]
%else
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
%endif
	sub rsp, kShadowSpace			; shadow space
	xcall	strcmp
	add rsp, kShadowSpace
strcmp1:
	xor	rbx, rbx
%ifdef LINUX
	cmp	eax, ebx
%else
	cmp	rax, rbx
%endif
	jz	strcmp3		; if strings equal, return 0
	jg	strcmp2
	sub	rbx, 2
strcmp2:
	inc	rbx
strcmp3:
	add	rpsp, 8
	mov	[rpsp], rbx
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry stricmpBop
	;	TOS: ptr2 ptr1
%ifdef LINUX
	push rip
	push rnext
	mov	rsi, [rpsp]
	mov	rdi, [rpsp + 8]
%else
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
%endif
	sub rsp, kShadowSpace			; shadow space
%ifdef WIN64
    xcall	stricmp
%else
	xcall	strcasecmp
%endif
	add rsp, kShadowSpace
	jmp	strcmp1
	
;========================================

entry strncmpBop
	;	TOS: numChars ptr2 ptr1
%ifdef LINUX
	push rip
	push rnext
	mov	rdx, [rpsp]
	mov	rsi, [rpsp + 8]
	mov	rdi, [rpsp + 16]
%else
	mov	r8, [rpsp]
	mov	rdx, [rpsp + 8]
	mov	rcx, [rpsp + 16]
%endif
	sub rsp, kShadowSpace			; shadow space
	xcall	strncmp
	add rsp, kShadowSpace
strncmp1:
	xor	rbx, rbx
	cmp	rax, rbx
	jz	strncmp3		; if strings equal, return 0
	jg	strncmp2
	sub	rbx, 2
strncmp2:
	inc	rbx
strncmp3:
	add	rpsp, 16
	mov	[rpsp], rbx
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry strstrBop
	;	TOS: ptr2 ptr1
%ifdef LINUX
	push rip
	push rnext
	mov	rsi, [rpsp]
	mov	rdi, [rpsp + 8]
%else
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
%endif
	sub rsp, kShadowSpace			; shadow space
	xcall	strstr
	add rsp, kShadowSpace
	add	rpsp, 8
	mov	[rpsp], rax
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry strtokBop
	;	TOS: ptr2 ptr1
%ifdef LINUX
	push rip
	push rnext
	mov	rsi, [rpsp]
	mov	rdi, [rpsp + 8]
%else
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
%endif
	sub rsp, kShadowSpace			; shadow space
	xcall	strtok
	add rsp, kShadowSpace
	add	rpsp, 8
	mov	[rpsp], rax
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry litBop
	movsx	rax, DWORD[rip]
	add	rip, 4
	sub	rpsp, 8
	mov	[rpsp], rax
	jmp	rnext
	
entry ulitBop
entry flitBop
    xor rax, rax            ; is this necessary?
	mov	eax, DWORD[rip]
	add	rip, 4
	sub	rpsp, 8
	mov	[rpsp], rax
	jmp	rnext
	
;========================================

entry dlitBop
	mov	rax, [rip]
	add	rip, 8
	sub	rpsp, 8
	mov	[rpsp], rax
	jmp	rnext
	
;========================================

; doDoes is executed while executing the user word
; it puts the parameter address of the user word on TOS
; top of rstack is parameter address
;
; : plusser builds , does @ + ;
; 5 plusser plus5
;
; the above 2 lines generates 3 new ops:
;	plusser
;	unnamed op
;	plus5
;
; code generated for above:
;
; plusser userOp(100) starts here
;	0	op(builds)
;	4	op(comma)
;	8	op(endBuilds)		compiled by "does"
;	12	101					compiled by "does"
; unnamed userOp(101) starts here
;	16	op(doDoes)			compiled by "does"
;	20	op(fetch)
;	24	op(plus)
;	28	op(doExit)
;
; plus5 userOp(102) starts here
;	32	userOp(101)
;	36	5
;
; ...
;	68	intCons(7)
;	72	userOp(102)		"plus5"
;	76	op(%d)
;
; we are executing some userOp when we hit the plus5 at 72
;	IP		next op			PS		RS
;--------------------------------------------
;	68		intCons(7)		()
;	72		userOp(102)		(7)		()
;	32		userOp(101)		(7)		(76)
;	16		op(doDoes)		(7)		(36,76)
;	20		op(fetch)		(36,7)	(76)
;	24		op(plus)		(5,7)	(76)
;	28		op(doExit)		(12)	(76)
;	76		op(%d)			(12)	()
;
entry doDoesBop
	mov	rbx, [rrp]	; rbx points at param field
	sub	rpsp, 8
	mov	[rpsp], rbx
	add	rrp, 8
	jmp	rnext
	
;========================================

entry doVariableBop
	; push IP
	sub	rpsp, 8
	mov	[rpsp], rip
	; rpop new ip
	mov	rip, [rrp]
	add	rrp, 8
	jmp	rnext
	
;========================================

entry doConstantBop
	; push longword @ IP
	mov	rax, [rip]
	sub	rpsp, 8
	mov	[rpsp], rax
	; rpop new ip
	mov	rip, [rrp]
	add	rrp, 8
	jmp	rnext
	
;========================================

entry doStructBop
	; push IP
	sub	rpsp, 8
	mov	[rpsp], rip
	; rpop new ip
	mov	rip, [rrp]
	add	rrp, 8
	jmp	rnext

;========================================

entry doStructArrayBop
	; TOS is array index
	; rip -> bytes per element, followed by element 0
	mov	rax, [rip]		; rax = bytes per element
    ; TODO: update code to use 8 bytes for element size
	add	rip, 8			; rip -> element 0
	imul rax, [rpsp]
	add	rax, rip		; add in array base addr
	mov	[rpsp], rax
	; rpop new ip
	mov	rip, [rrp]
	add	rrp, 8
	jmp	rnext

;========================================

entry thisBop
	mov	rax, [rcore + FCore.TPtr]
	sub	rpsp, 8
	mov	[rpsp], rax
	jmp	rnext
	
;========================================

entry devolveBop
	mov	rax, [rcore + FCore.TPtr]
    mov rbx, [rax + 16]             ; rbx is wrapped object this ptr
	mov	[rcore + FCore.TPtr], rbx   ; set this to point to wrapped object
	jmp	rnext
	
;========================================

entry executeBop
	mov	ebx, [rpsp]
	add	rpsp, 8
	mov	rax, [rcore + FCore.innerExecute]
	jmp rax
	
;========================================

entry executeMethodBop
    ; TOS is object pointer, next is method opcode (not index)
    ; this is compiled to implement super.xxx, and shouldn't be used in normal code
    ; push this on rstack

	; get this ptr from TOS	
	mov	rax, [rpsp]
	add	rpsp, 8
	or	rax, rax
	jnz executeMethod1
	mov	rax, kForthErrorBadObject
	jmp	interpFuncErrorExit
executeMethod1:
	; push current this on return stack
	mov	rcx, [rcore + FCore.TPtr]
	sub	rrp, 8
	mov	[rrp], rcx
	mov	[rcore + FCore.TPtr], rax

    mov rbx, [rpsp]
    add rpsp, 8
	mov	rax, [rcore + FCore.innerExecute]
	jmp rax
	
;========================================

entry	fopenBop
%ifdef LINUX
	push rip
	push rnext
	mov	rsi, [rpsp]
	mov	rdi, [rpsp + 8]
%else
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
%endif
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.fileOpen]
	sub rsp, kShadowSpace			; shadow space
	call rax
	add rsp, kShadowSpace
	add	rpsp, 8
	mov	[rpsp], rax	; push fopen result
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry	fcloseBop
%ifdef LINUX
	push rip
	push rnext
	mov	rdi, [rpsp]
%else
	mov	rcx, [rpsp]
%endif
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.fileClose]
	sub rsp, kShadowSpace			; shadow space
	call rax
	add rsp, kShadowSpace
	mov	[rpsp], rax	; push fclose result
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry	fseekBop
%ifdef LINUX
	push rip
	push rnext
	mov	rdx, [rpsp]
	mov	rsi, [rpsp + 8]
	mov	rdi, [rpsp + 16]
%else
	mov	r8, [rpsp]
	mov	rdx, [rpsp + 8]
	mov	rcx, [rpsp + 16]
%endif
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.fileSeek]
	sub rsp, kShadowSpace			; shadow space
	call rax
	add rsp, kShadowSpace
	add	rpsp, 16
	mov	[rpsp], rax	; push fseek result
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry	freadBop
%ifdef LINUX
	push rip
	push rnext
	mov	rcx, [rpsp]
	mov	rdx, [rpsp + 8]
	mov	rsi, [rpsp + 16]
	mov	rdi, [rpsp + 24]
%else
	mov	r9, [rpsp]
	mov	r8, [rpsp + 8]
	mov	rdx, [rpsp + 16]
	mov	rcx, [rpsp + 24]
%endif
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.fileRead]
	sub rsp, kShadowSpace			; shadow space
	call rax
	add rsp, kShadowSpace
	add	rpsp, 24
	mov	[rpsp], rax	; push fread result
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry	fwriteBop
%ifdef LINUX
	push rip
	push rnext
	mov	rcx, [rpsp]
	mov	rdx, [rpsp + 8]
	mov	rsi, [rpsp + 16]
	mov	rdi, [rpsp + 24]
%else
	mov	r9, [rpsp]
	mov	r8, [rpsp + 8]
	mov	rdx, [rpsp + 16]
	mov	rcx, [rpsp + 24]
%endif
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.fileWrite]
	sub rsp, kShadowSpace			; shadow space
	call rax
	add rsp, kShadowSpace
	add	rpsp, 24
	mov	[rpsp], rax	; push fwrite result
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry	fgetcBop
%ifdef LINUX
	push rip
	push rnext
	mov	rdi, [rpsp]
%else
	mov	rcx, [rpsp]
%endif
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.fileGetChar]
	sub rsp, kShadowSpace			; shadow space
	call rax
	add rsp, kShadowSpace
	mov	[rpsp], rax	; push fgetc result
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry	fputcBop
%ifdef LINUX
	push rip
	push rnext
	mov	rsi, [rpsp]
	mov	rdi, [rpsp + 8]
%else
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
%endif
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.filePutChar]
	sub rsp, kShadowSpace			; shadow space
	call rax
	add rsp, kShadowSpace
	add rpsp, 8
	mov	[rpsp], rax	; push fputc result
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry	feofBop
%ifdef LINUX
	push rip
	push rnext
	mov	rdi, [rpsp]
%else
	mov	rcx, [rpsp]
%endif
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.fileAtEnd]
	sub rsp, kShadowSpace			; shadow space
	call rax
	add rsp, kShadowSpace
	mov	[rpsp], rax	; push feof result
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry	fexistsBop
%ifdef LINUX
	push rip
	push rnext
	mov	rdi, [rpsp]
%else
	mov	rcx, [rpsp]
%endif
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.fileExists]
	sub rsp, kShadowSpace			; shadow space
	call rax
	add rsp, kShadowSpace
	mov	[rpsp], rax	; push fexists result
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry	ftellBop
%ifdef LINUX
	push rip
	push rnext
	mov	rdi, [rpsp]
%else
	mov	rcx, [rpsp]
%endif
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.fileTell]
	sub rsp, kShadowSpace			; shadow space
	call rax
	add rsp, kShadowSpace
	mov	[rpsp], rax	; push ftell result
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry	flenBop
%ifdef LINUX
	push rip
	push rnext
	mov	rdi, [rpsp]
%else
	mov	rcx, [rpsp]
%endif
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.fileGetLength]
	sub rsp, kShadowSpace			; shadow space
	call rax
	add rsp, kShadowSpace
	mov	[rpsp], rax	; push flen result
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry	fgetsBop
%ifdef LINUX
	push rip
	push rnext
	mov	rdx, [rpsp]
	mov	rsi, [rpsp + 8]
	mov	rdi, [rpsp + 16]
%else
	mov	r8, [rpsp]
	mov	rdx, [rpsp + 8]
	mov	rcx, [rpsp + 16]
%endif
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.fileGetString]
	sub rsp, kShadowSpace			; shadow space
	call rax
	add rsp, kShadowSpace
	add	rpsp, 16
	mov	[rpsp], rax	; push fgets result
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext
	
;========================================

entry	fputsBop
%ifdef LINUX
	push rip
	push rnext
	mov	rsi, [rpsp]
	mov	rdi, [rpsp + 8]
%else
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
%endif
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.filePutString]
	sub rsp, kShadowSpace			; shadow space
	call rax
	add rsp, kShadowSpace
	add	rpsp, 8
	mov	[rpsp], rax	; push fputs result
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext

	
;========================================
entry	setTraceBop
	mov	rax, [rpsp]
	add	rpsp, 8
	mov	[rcore + FCore.traceFlags], rax
	mov	[rcore + FCore.SPtr], rpsp
	mov	[rcore + FCore.IPtr], rip
	jmp interpFuncReenter

%ifndef LINUX

;========================================

;extern void snprintfSub( CoreState* pCore );
;extern void fscanfSub( CoreState* pCore );
;extern void sscanfSub( CoreState* pCore );

;========================================

; fprintfOp (C++ version)
;  fprintfSub(pCore)
;   fprintfSubCore

; fprintfOp (assembler version)
;  fprintfSubCore

; extern void fprintfSub( CoreState* pCore );
entry fprintfSub
    ; called from C++
    ; rcx is pCore
    push rcore
    push rpsp
    push rfp
	; stack should be 16-byte aligned at this point
    ; params refer to parameters passed to fprintf: formatStr filePtr arg1..argN
    ; arguments refer to things which are to be printed: arg1..argN
    
    mov rcore, rcx                        ; rcore -> CoreState
	mov	rpsp, [rcore + FCore.SPtr]

    ; TOS: N argN..arg1 formatStr filePtr       (arg1 to argN are optional)
    mov rax, [rpsp]     ; rax is number of arguments
	mov rfp, rsp        ; rfp is saved system stack pointer

    ; if there are less than 3 arguments, none will be passed on system stack
    cmp rax, 3
    jl .fprint1
    ; if number of args is odd, do a dummy push to fix stack alignment
    or rax, rax
    jpo .fprint1
    push rax
.fprint1:
    lea rcx, [rpsp + rax*8]     ; rcx -> arg1
	mov r10, rcx
	mov rdx, [rcx + 8]			; rdx = formatStr (2nd param)
    dec rax
    jl .fprint9
    ; deal with arg1 and arg2
    ; stick them in both r8/r9 and xmm2/xmm3, since they may be floating point
    mov r8, [rcx]
    movq xmm2, r8
    sub rcx, 8
    dec rax
    jl .fprint9
    mov r9, [rcx]
    movq xmm3, r9
    sub rcx, 8
    dec rax
    jl .fprint9

    ; push args 3..N on system stack
    lea rcx, [rpsp + 8]     ; rcx -> argN
.fprintLoop:
    mov r11, [rcx]
    push r11
    add rcx, 8
    dec rax
    jge .fprintLoop
.fprint9:
    ; all args have been fetched except format and file
	lea rpsp, [r10 + 16]    ; rpsp -> filePtr on TOS
    mov rcx, [rpsp]         ; rcx = filePtr (1st param)
    ; rcx - filePtr
    ; rdx - formatStr
    ; r8 & xmm2 - arg1
    ; r9 & xmm3 - arg2
    ; rest of arguments are on system stack
	sub rsp, kShadowSpace			; shadow space
    xcall fprintf
    mov [rpsp], rax
	; do stack cleanup
	mov rsp, rfp
	mov	[rcore + FCore.SPtr], rpsp
    pop rfp
    pop rpsp
    pop rcore
	ret

;========================================

entry snprintfSub
    ; called from C++
    ; rcx is pCore
    push rcore
    push rpsp
    push rfp
	; stack should be 16-byte aligned at this point
    ; params refer to parameters passed to snprintf: bufferPtr bufferSize formatStr arg1..argN
    ; arguments refer to things which are to be printed: arg1..argN
    
    mov rcore, rcx                        ; rcore -> CoreState
	mov	rpsp, [rcore + FCore.SPtr]

    ; TOS: N argN ... arg1 formatStr bufferSize bufferPtr       (arg1 to argN are optional)
    mov rax, [rpsp]     ; rax is number of arguments
	mov rfp, rsp        ; rfp is saved system stack pointer
    
    ; if there are less than 2 arguments, none will be passed on system stack
    cmp rax, 2
    jl .snprint1          
    ; if number of args is even, do a dummy push to fix stack alignment
    or rax, rax
    jpo .snprint1
    push rax
.snprint1:
    lea rcx, [rpsp + rax*8]     ; rcx -> arg1
	mov r10, rcx
    dec rax
    jl .snprint9
    ; rcx -> arg1
    ; deal with arg1
    ; stick it in both r9 and xmm3, since it may be floating point
    mov r9, [rcx]
    movq xmm3, r9
    sub rcx, 8
    dec rax
    jl .snprint9
    
    ; push args 2..N on system stack
    lea rcx, [rpsp + 8]
.snprintLoop:
    mov r11, [rcx]
    push r11
    add rcx, 8
    dec rax
    jge .snprintLoop
.snprint9:
    ; TOS: N argN ... arg1 formatStr bufferSize bufferPtr       (arg1 to argN are optional)
    ; all args have been fetched except formatStr, bufferSize and bufferPtr
	lea rpsp, [r10 + 24]        ; rpsp -> bufferPtr on TOS
    mov rcx, [rpsp]             ; rcx = bufferPtr (1st param)
	mov rdx, [r10 + 16]			; rdx = bufferSize (2nd param)
	mov r8, [r10 + 8]			; r8 =  formatStr (3rd param)
    ; rcx - bufferPtr
    ; rdx - bufferSize
    ; r8 - formatStr
    ; r9 & xmm3 - arg1
    ; rest of arguments are on system stack
	sub rsp, kShadowSpace			; shadow space
    xcall snprintf
    mov [rpsp], rax
	; do stack cleanup
	mov rsp, rfp
	mov	[rcore + FCore.SPtr], rpsp
    pop rfp
    pop rpsp
    pop rcore
	ret

;========================================

; extern int oStringFormatSub( CoreState* pCore, char* pBuffer, int bufferSize );
entry oStringFormatSub
    ; called from C++
    ; rcx is pCore
    ; rdx is bufferPtr
    ; r8 is bufferSize
    
	;sub rsp, 128
    push rcore			; r12
    push rpsp			; r14
    push rfp			; r13
	; stack should be 16-byte aligned at this point
    ; params refer to parameters passed to snprintf: bufferPtr bufferSize formatStr arg1..argN
    ; arguments refer to things which are to be printed: arg1..argN
    
    mov rcore, rcx                        ; rcore -> CoreState
	mov	rpsp, [rcore + FCore.SPtr]

    ; TOS: N argN ... arg1 formatStr bufferSize bufferPtr       (arg1 to argN are optional)
    mov rax, [rpsp]     ; rax is number of arguments
	mov rfp, rsp        ; rfp is saved system stack pointer
    
    ; if there are less than 2 arguments, none will be passed on system stack
    cmp rax, 2
    jl .sformat1          
    ; if number of args is even, do a dummy push to fix stack alignment
    or rax, rax
    jpo .sformat1
    push rax
.sformat1:
    lea rcx, [rpsp + rax*8]     ; rcx -> arg1
	mov r10, rcx
    dec rax
    jl .sformat9
    ; rcx -> arg1
    ; deal with arg1
    ; stick it in both r9 and xmm3, since it may be floating point
    mov r9, [rcx]
    movq xmm3, r9
    sub rcx, 8
    dec rax
    jl .sformat9
    
    ; push args 2..N on system stack
    lea rcx, [rpsp + 8]
.sformatLoop:
    mov r11, [rcx]
    push r11
    add rcx, 8
    dec rax
    jge .sformatLoop
.sformat9:
    ; TOS: N argN ... arg1 formatStr bufferSize bufferPtr       (arg1 to argN are optional)
    ; all args have been fetched except formatStr, bufferSize and bufferPtr
	lea rpsp, [r10 + 8]         ; rpsp -> bufferPtr on TOS
    mov rcx, rdx                ; rcx = bufferPtr (1st param)
	mov rdx, r8                 ; rdx = bufferSize (2nd param)
	mov r8, [rpsp]				; r8 =  formatStr (3rd param)
    ; rcx - bufferPtr
    ; rdx - bufferSize
    ; r8 - formatStr
    ; r9 & xmm3 - arg1
    ; rest of arguments are on system stack
	sub rsp, kShadowSpace			; shadow space
    xcall snprintf
    add rpsp, 8
	; do stack cleanup
	mov rsp, rfp
	mov	[rcore + FCore.SPtr], rpsp
    pop rfp
    pop rpsp
    pop rcore
	;add rsp, 128
	ret

;========================================

entry fscanfSub
    ; called from C++
    ; rcx is pCore
    push rcore
    push rpsp
    push rfp
	; stack should be 16-byte aligned at this point
    ; params refer to parameters passed to fscanf: formatStr filePtr arg1..argN
    ; arguments refer to things which are to be printed: arg1..argN
    
    mov rcore, rcx                        ; rcore -> CoreState
	mov	rpsp, [rcore + FCore.SPtr]

    ; TOS: N argN..arg1 formatStr filePtr       (arg1 to argN are optional)
    mov rax, [rpsp]     ; rax is number of arguments
	mov rfp, rsp        ; rfp is saved system stack pointer

    ; if there are less than 3 arguments, none will be passed on system stack
    cmp rax, 3
    jl .fscan1
    ; if number of args is odd, do a dummy push to fix stack alignment
    or rax, rax
    jpo .fscan1
    push rax
.fscan1:
    lea rcx, [rpsp + rax*8]     ; rcx -> arg1
	mov r10, rcx
	mov rdx, [rcx + 8]          ; rdx = formatStr (2nd param)
    dec rax
    jl .fscan9
    ; deal with arg1 and arg2
    mov r8, [rcx]
    sub rcx, 8
    dec rax
    jl .fscan9
    mov r9, [rcx]
    sub rcx, 8
    dec rax
    jl .fscan9

    ; push args 3..N on system stack
    lea rcx, [rpsp + 8]     ; rcx -> argN
.fscanLoop:
    mov r11, [rcx]
    push r11
    add rcx, 8
    dec rax
    jge .fscanLoop
.fscan9:
    ; all args have been fetched except format and file
	lea rpsp, [r10 + 16]    ; rpsp -> filePtr on TOS
    mov rcx, [rpsp]         ; rcx = filePtr (1st param)
    ; rcx - filePtr
    ; rdx - formatStr
    ; r8 - arg1 ptr
    ; r9 - arg2 ptr
    ; rest of arguments are on system stack
	sub rsp, kShadowSpace			; shadow space
    xcall fscanf
    mov [rpsp], rax
	; do stack cleanup
	mov rsp, rfp
	mov	[rcore + FCore.SPtr], rpsp
    pop rfp
    pop rpsp
    pop rcore
	ret

;========================================

entry sscanfSub
    ; called from C++
    ; rcx is pCore
    push rcore
    push rpsp
    push rfp
	; stack should be 16-byte aligned at this point
    ; params refer to parameters passed to sscanf: formatStr bufferPtr arg1..argN
    ; arguments refer to things which are to be printed: arg1..argN
    
    mov rcore, rcx                        ; rcore -> CoreState
	mov	rpsp, [rcore + FCore.SPtr]

    ; TOS: N argN..arg1 formatStr filePtr       (arg1 to argN are optional)
    mov rax, [rpsp]     ; rax is number of arguments
	mov rfp, rsp        ; rfp is saved system stack pointer

    ; if there are less than 3 arguments, none will be passed on system stack
    cmp rax, 3
    jl .sscan1
    ; if number of args is odd, do a dummy push to fix stack alignment
    or rax, rax
    jpo .sscan1
    push rax
.sscan1:
    lea rcx, [rpsp + rax*8]     ; rcx -> arg1
	mov r10, rcx
	mov rdx, [rcx + 8]			; rdx = formatStr (2nd param)
    dec rax
    jl .sscan9
    ; deal with arg1 and arg2
    mov r8, [rcx]
    sub rcx, 8
    dec rax
    jl .sscan9
    mov r9, [rcx]
    sub rcx, 8
    dec rax
    jl .sscan9

    ; push args 3..N on system stack
    lea rcx, [rpsp + 8]     ; rcx -> argN
.sscanLoop:
    mov r11, [rcx]
    push r11
    add rcx, 8
    dec rax
    jge .sscanLoop
.sscan9:
    ; all args have been fetched except format and bufferPtr
	lea rpsp, [r10 + 16]    ; rpsp -> bufferPtr on TOS
    mov rcx, [rpsp]         ; rcx = bufferPtr (1st param)
    ; rcx - bufferPtr
    ; rdx - formatStr
    ; r8 - arg1 ptr
    ; r9 - arg2 ptr
    ; rest of arguments are on system stack
	sub rsp, kShadowSpace			; shadow space
    xcall sscanf
    mov [rpsp], rax
	; do stack cleanup
	mov rsp, rfp
	mov	[rcore + FCore.SPtr], rpsp
    pop rfp
    pop rpsp
    pop rcore
	ret

%endif

;========================================
entry dllEntryPointType
	; rbx is opcode:
	; bits 0..15 are index into CoreState userOps table
	; 16..18 are flags
	; 19..23 are arg count
	; args are on TOS
	mov	rax, rbx
	and	rax, 0000FFFFh		; rax is userOps table index for this dll routine
	cmp	rax, rnumops
	jge	badUserDef

%ifdef LINUX
	push rip
	push rnext
	mov	rsi, [rpsp]
	mov	rdi, [rpsp + 8]

	mov	rdi, [roptab + rax*8]

	mov	rsi, rbx
	shr	rsi, 19
	and	rsi, 1Fh

	mov rdx, rbx
	shr rdx, 16
	and rdx, 7

	mov rcx, rcore

	; rdi - dll routine address
	; rsi - arg count
	; rdx - flags
	; rcx - pCore
%else
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
	mov	rcx, [roptab + rax*8]

	mov	rdx, rbx
	shr	rdx, 19
	and	rdx, 1Fh

	mov r8, rbx
	shr r8, 16
	and r8, 7

	mov r9, rcore

	; rcx - dll routine address
	; rdx - arg count
	; r8 - flags
	; r9 - pCore
%endif

	sub rsp, kShadowSpace
	xcall	CallDLLRoutine
	add rsp, kShadowSpace
    mov rpsp, [rcore + FCore.SPtr]
%ifdef LINUX
	pop rnext
	pop rip
%endif
	jmp	restoreNext


;-----------------------------------------------
;
; NUM VAROP OP combo ops
;  
entry nvoComboType
	; rbx: bits 0..10 are signed integer, bits 11..12 are varop-2, bit 13..23 are opcode
	mov	rax, rbx
	sub	rpsp, 8
	and	rax, 0x400
	jnz	nvoNegative
	; positive constant
	mov	rax, rbx
	and	rax, 0x3FF
	jmp	nvoCombo1

nvoNegative:
	mov rax, 0xFFFFFFFFFFFFF800
	or	rax, rbx			; sign extend bits 11-31
nvoCombo1:
	mov	[rpsp], rax
	; set the varop from bits 11-12
	shr	rbx, 11
	mov	rax, rbx
	
	and	rax, 3							; rax = varop - 2
	add	rax, 2
	mov	[rcore + FCore.varMode], rax
	
	; extract the opcode
	shr	rbx, 2
	and	rbx, 0000007FFh			; rbx is 11 bit opcode
	; opcode is in rbx
	mov	rax, [rcore + FCore.innerExecute]
	jmp rax

;-----------------------------------------------
;
; NUM VAROP combo ops
;  
entry nvComboType
	; rbx: bits 0..21 are signed integer, bits 22-23 are varop-2
	mov	rax, rbx
	sub	rpsp, 8
	and	rax,00200000h
	jnz	nvNegative
	; positive constant
	mov	rax, rbx
	and	rax,001FFFFFh
	jmp	nvCombo1

nvNegative:
	mov rax, 0xFFFFFFFFFFE00000
	or rax, rbx			; sign extend bits 22-31
nvCombo1:
	mov	[rpsp], rax
	; set the varop from bits 22-23
	shr	rbx, 22							; rbx = varop - 2
	and	rbx, 3
	add	rbx, 2
	mov	[rcore + FCore.varMode], rbx

	jmp rnext

;-----------------------------------------------
;
; NUM OP combo ops
;  
entry noComboType
	; rbx: bits 0..12 are signed integer, bits 13..23 are opcode
	mov	rax, rbx
	sub	rpsp, 8
	and	rax,000001000h
	jnz	noNegative
	; positive constant
	mov	rax, rbx
	and	rax,00FFFh
	jmp	noCombo1

noNegative:
	mov rcx, 0xFFFFFFFFFFFFE000			; sign extend bits 13-31
	or rax, rcx
noCombo1:
	mov	[rpsp], rax
	; extract the opcode
	mov	rax, rbx
	shr	rbx, 13
	and	rbx, 0000007FFh			; rbx is 11 bit opcode
	; opcode is in rbx
	mov	rax, [rcore + FCore.innerExecute]
	jmp rax
	
;-----------------------------------------------
;
; VAROP OP combo ops
;  
entry voComboType
	; rbx: bits 0-1 are varop-2, bits 2-23 are opcode
	; set the varop from bits 0-1
	mov	rax, 000000003h
	and	rax, rbx
	add	rax, 2
	mov	[rcore + FCore.varMode], rax
	
	; extract the opcode
	shr	rbx, 2
	and	rbx, 0003FFFFFh			; rbx is 22 bit opcode

	; opcode is in rbx
	mov	rax, [rcore + FCore.innerExecute]
	jmp rax

;-----------------------------------------------
;
; OP ZBRANCH combo ops
;  
entry ozbComboType
	; rbx: bits 0..11 are opcode, bits 12-23 are signed integer branch offset in longs
	mov	rax, rbx
	shr	rax, 10
	and	rax, 03FFCh
	push	rax
	push	rnext
	mov	rnext, ozbCombo1
	and	rbx, 0FFFh
	; opcode is in rbx
	mov	rax, [rcore + FCore.innerExecute]
	jmp rax
	
ozbCombo1:
	pop	rnext
	pop	rax
	mov	rbx, [rpsp]
	add	rpsp, 8
	or	rbx, rbx
	jnz	ozbCombo2			; if TOS not 0, don't branch
	mov	rbx, rax
	and	rax, 02000h
	jz	ozbForward
	; backward branch
	mov rax, 0xFFFFFFFFFFFFC000
	or rbx, rax
ozbForward:
	add	rip, rbx
ozbCombo2:
	jmp	rnext
	
;-----------------------------------------------
;
; OP NZBRANCH combo ops
;  
entry onzbComboType
	; rbx: bits 0..11 are opcode, bits 12-23 are signed integer branch offset in longs
	mov	rax, rbx
	shr	rax, 10
	and	rax, 03FFCh
	push	rax
	push	rnext
	mov	rnext, onzbCombo1
	and	rbx, 0FFFh
	; first, go execute the opcode in rbx, when it jumps to rnext it will go to onzbCombo1
	; opcode is in rbx
	mov	rax, [rcore + FCore.innerExecute]
	jmp rax
	
onzbCombo1:
	pop	rnext				; unstomp rnext
	pop	rax					; rax is branch offset
	mov	rbx, [rpsp]			; pop and test TOS
	add	rpsp, 8
	or	rbx, rbx
	jz	onzbCombo2			; if TOS 0, don't branch
	; we are going to branch, see if it forward or backward
	mov	rbx, rax
	and	rax, 02000h			; test sign bit of offset
	jz	onzbForward
	; backward branch
	mov rax, 0xFFFFFFFFFFFFC000
	or rbx, rax
onzbForward:
	add	rip, rbx
onzbCombo2:
	jmp	rnext
	
;-----------------------------------------------
;
; squished float literal
;  
entry squishedFloatType
	; rbx: bit 23 is sign, bits 22..18 are exponent, bits 17..0 are mantissa
	; to unsquish a float:
	;   sign = (inVal & 0x800000) << 8
	;   exponent = (((inVal >> 18) & 0x1f) + (127 - 15)) << 23
	;   mantissa = (inVal & 0x3ffff) << 5
	;   outVal = sign | exponent | mantissa
	push	rip
	mov	rax, rbx
	and	rax, 00800000h
	shl	rax, 8			; sign bit
	
	mov	rip, rbx
	shr	rbx, 18
	and	rbx, 1Fh
	add	rbx, 112
	shl	rbx, 23			; rbx is exponent
	or	rax, rbx
	
	and	rip, 03FFFFh
	shl	rip, 5
	or	rax, rip
	
	sub	rpsp, 8
	mov	[rpsp], rax
	pop	rip
	jmp	rnext
	

;-----------------------------------------------
;
; squished double literal
;  
entry squishedDoubleType
	; rbx: bit 23 is sign, bits 22..18 are exponent, bits 17..0 are mantissa
	; to unsquish a double:
	;   sign = (inVal & 0x800000) << 8
	;   exponent = (((inVal >> 18) & 0x1f) + (1023 - 15)) << 20
	;   mantissa = (inVal & 0x3ffff) << 2
	;   outVal = (sign | exponent | mantissa) << 32
	mov	rax, rbx
	and	rax, 00800000h
	shl	rax, 8			; sign bit
	
	mov	rcx, rbx
	shr	rbx, 18
	and	rbx, 1Fh
	add	rbx, 1008
	shl	rbx, 20			; rbx is exponent
	or	rax, rbx
	
	and	rcx, 03FFFFh
	shl	rcx, 2
	or	rax, rcx
	
	; loword of double is all zeros
	shl rax, 32
	sub	rpsp, 8
	mov	[rpsp], rax
	jmp	rnext
	

;-----------------------------------------------
;
; squished long literal
;  
entry squishedLongType
	; get low-24 bits of opcode
	mov	rax, rbx
	sub	rpsp, 8
	and	rax,00800000h
	jnz	longConstantNegative
	; positive constant
	and	rbx,00FFFFFFh
	mov	[rpsp], rbx
	jmp	rnext

longConstantNegative:
	mov rax, 0xFFFFFFFFFF000000
	or	rbx, rax
	mov	[rpsp], rbx
	jmp	rnext
	

;-----------------------------------------------
;
; LOCALREF OP combo ops
;
entry lroComboType
	; rbx: bits 0..11 are frame offset in cells, bits 12-23 are op
	push	rbx
	and	rbx, 0FFFH
	sal	rbx, 3
	mov	rax, rfp
	sub	rax, rbx
	sub	rpsp, 8
	mov	[rpsp], rax
	
	pop	rbx
	shr	rbx, 12
	and	rbx, 0FFFH			; rbx is 12 bit opcode
	; opcode is in rbx
	mov	rax, [rcore + FCore.innerExecute]
	jmp rax
	
;-----------------------------------------------
;
; MEMBERREF OP combo ops
;
entry mroComboType
	; rbx: bits 0..11 are member offset in bytes, bits 12-23 are op
	push	rbx
	and	rbx, 0FFFH
	mov	rax, [rcore + FCore.TPtr]
	add	rax, rbx
	sub	rpsp, 8
	mov	[rpsp], rax

	pop	rbx
	shr	rbx, 12
	and	rbx, 0FFFH			; rbx is 12 bit opcode
	; opcode is in rbx
	mov	rax, [rcore + FCore.innerExecute]
	jmp rax


;=================================================================================================
;
;                                    opType table
;  
;=================================================================================================
entry opTypesTable
; TBD: check the order of these
; TBD: copy these into base of CoreState, fill unused slots with badOptype
;	00 - 09
	DQ	externalBuiltin		; kOpNative = 0,
	DQ	nativeImmediate		; kOpNativeImmediate,
	DQ	userDefType			; kOpUserDef,
	DQ	userDefType			; kOpUserDefImmediate,
	DQ	cCodeType				; kOpCCode,         
	DQ	cCodeType				; kOpCCodeImmediate,
	DQ	relativeDefType		; kOpRelativeDef,
	DQ	relativeDefType		; kOpRelativeDefImmediate,
	DQ	dllEntryPointType		; kOpDLLEntryPoint,
	DQ	extOpType	
;	10 - 19
	DQ	branchType				; kOpBranch = 10,
	DQ	branchNZType			; kOpBranchNZ,
	DQ	branchZType			    ; kOpBranchZ,
	DQ	caseBranchTType			; kOpCaseBranchT,
	DQ	caseBranchFType			; kOpCaseBranchF,
	DQ	pushBranchType			; kOpPushBranch,	
	DQ	relativeDefBranchType	; kOpRelativeDefBranch,
	DQ	relativeDataType		; kOpRelativeData,
	DQ	relativeDataType		; kOpRelativeString,
	DQ	extOpType	
;	20 - 29
	DQ	constantType			; kOpConstant = 20,   
	DQ	constantStringType		; kOpConstantString,	
	DQ	offsetType				; kOpOffset,          
	DQ	arrayOffsetType		; kOpArrayOffset,     
	DQ	allocLocalsType		; kOpAllocLocals,     
	DQ	localRefType			; kOpLocalRef,
	DQ	initLocalStringType	; kOpLocalStringInit, 
	DQ	localStructArrayType	; kOpLocalStructArray,
	DQ	offsetFetchType		; kOpOffsetFetch,          
	DQ	memberRefType			; kOpMemberRef,	

;	30 - 39
	DQ	localByteType			; 30 - 42 : local variables
	DQ	localUByteType
	DQ	localShortType
	DQ	localUShortType
	DQ	localIntType
	DQ	localUIntType
	DQ	localLongType
	DQ	localLongType
	DQ	localFloatType
	DQ	localDoubleType
	
;	40 - 49
	DQ	localStringType
	DQ	localOpType
	DQ	localObjectType
	DQ	localByteArrayType		; 43 - 55 : local arrays
	DQ	localUByteArrayType
	DQ	localShortArrayType
	DQ	localUShortArrayType
	DQ	localIntArrayType
	DQ	localUIntArrayType
	DQ	localLongArrayType
	
;	50 - 59
	DQ	localLongArrayType
	DQ	localFloatArrayType
	DQ	localDoubleArrayType
	DQ	localStringArrayType
	DQ	localOpArrayType
	DQ	localObjectArrayType
	DQ	fieldByteType			; 56 - 68 : field variables
	DQ	fieldUByteType
	DQ	fieldShortType
	DQ	fieldUShortType
	
;	60 - 69
	DQ	fieldIntType
	DQ	fieldUIntType
	DQ	fieldLongType
	DQ	fieldLongType
	DQ	fieldFloatType
	DQ	fieldDoubleType
	DQ	fieldStringType
	DQ	fieldOpType
	DQ	fieldObjectType
	DQ	fieldByteArrayType		; 69 - 81 : field arrays
	
;	70 - 79
	DQ	fieldUByteArrayType
	DQ	fieldShortArrayType
	DQ	fieldUShortArrayType
	DQ	fieldIntArrayType
	DQ	fieldUIntArrayType
	DQ	fieldLongArrayType
	DQ	fieldLongArrayType
	DQ	fieldFloatArrayType
	DQ	fieldDoubleArrayType
	DQ	fieldStringArrayType
	
;	80 - 89
	DQ	fieldOpArrayType
	DQ	fieldObjectArrayType
	DQ	memberByteType			; 82 - 94 : member variables
	DQ	memberUByteType
	DQ	memberShortType
	DQ	memberUShortType
	DQ	memberIntType
	DQ	memberUIntType
	DQ	memberLongType
	DQ	memberLongType
	
;	90 - 99
	DQ	memberFloatType
	DQ	memberDoubleType
	DQ	memberStringType
	DQ	memberOpType
	DQ	memberObjectType
	DQ	memberByteArrayType	; 95 - 107 : member arrays
	DQ	memberUByteArrayType
	DQ	memberShortArrayType
	DQ	memberUShortArrayType
	DQ	memberIntArrayType
	
;	100 - 109
	DQ	memberUIntArrayType
	DQ	memberLongArrayType
	DQ	memberLongArrayType
	DQ	memberFloatArrayType
	DQ	memberDoubleArrayType
	DQ	memberStringArrayType
	DQ	memberOpArrayType
	DQ	memberObjectArrayType
	DQ	methodWithThisType
	DQ	methodWithTOSType
	
;	110 - 119
	DQ	memberStringInitType
	DQ	nvoComboType
	DQ	nvComboType
	DQ	noComboType
	DQ	voComboType
	DQ	ozbComboType
	DQ	onzbComboType
	
	DQ	squishedFloatType
	DQ	squishedDoubleType
	DQ	squishedLongType
	
;	120 - 122
	DQ	lroComboType
	DQ	mroComboType
	DQ  extOpType       ; was methodWithSuperType
	
;	123 - 127
    DQ  nativeU32Type
    DQ  nativeS32Type
    DQ  nativeU32Type
    DQ  native64Type
    DQ  native64Type

;	128 - 132
    DQ  cCodeU32Type
    DQ  cCodeS32Type
    DQ  cCodeU32Type
    DQ  cCode64Type
    DQ  cCode64Type

;	133 - 137
    DQ  userDefU32Type
    DQ  userDefS32Type
    DQ  userDefU32Type
    DQ  userDef64Type
    DQ  userDef64Type

;   138 - 139
    DQ  methodWithLocalObjectType
    DQ  methodWithMemberObjectType

;	140 - 199
	DQ	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
;	150 - 199
	DQ	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	DQ	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	DQ	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	DQ	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	DQ	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
;	200 - 249
	DQ	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	DQ	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	DQ	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	DQ	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	DQ	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
;	250 - 255
	DQ	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	
endOpTypesTable:
	DQ	0
	
