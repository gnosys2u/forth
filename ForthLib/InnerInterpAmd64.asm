
DEFAULT REL
BITS 64

%include "core64.inc"

;
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

; OSX requires that the system stack be on a 16-byte boundary before you call any system routine
; In general in the inner interpreter code the system stack is kept at 16-byte boundary, so to
; determine how much need to offset the system stack to maintain OSX stack alignment, you count
; the number of pushes done, including both arguments to the called routine and any extra registers
; you have saved on the stack, and use this formula:
; offset = 4 * (3 - (numPushesDone mod 4))

; these are the trace flags that force the external trace output to be
; called each time an instruction is executed
traceDebugFlags EQU	kLogProfiler + kLogStack + kLogInnerInterpreter

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
	sub rsp, 32			; shadow space
%ifdef LINUX
	call	%2
%else
	call	%2
%endif
	add rsp, 32
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
	sub rsp, 32			; shadow space
%ifdef LINUX
	call	%2
%else
	call	%2
%endif
	add rsp, 32
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
	mov	rax, [rcx + rax*4]				; rax is C routine to dispatch to
	and	r8, 00FFFFFFh
    mov rcx, rcore  ; 1st param to C routine
    mov rdx, r8     ; 2nd param to C routine
    ; stack is already 16-byte aligned
	sub rsp, 32			; shadow space
	call	rax
	add rsp, 32
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
	;jmp	interpLoopExit	; if something went wrong
	
extOpType1:
; TODO!
	ret

;-----------------------------------------------
;
; InitAsmTables - initializes first part of optable, where op positions are referenced by constants
;
; extern void InitAsmTables( ForthCoreState *pCore );
entry InitAsmTables

	; rcx -> ForthCore struct
	
	; setup normal (non-debug) inner interpreter re-entry point
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
	ret

;-----------------------------------------------
;
; single step a thread
;
; extern eForthResult InterpretOneOpFast( ForthCoreState *pCore, forthop op );
entry InterpretOneOpFast
    ; rcx is pCore
    ; rdx is op
    
	push rbx
    push rdi
    push rsi
    push r12
    push r13
    push r14
    push r15
	; stack should be 16-byte aligned at this point
    mov rcore, rcx                        ; rcore -> ForthCoreState
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
	sub rsp, 32			; shadow space
	call	interpLoopExecuteEntry
	jmp	InterpretOneOpFastExit2

InterpretOneOpFastExit:		; this is exit for state == OK - discard the unused return address from call above
	add	rsp, 8
InterpretOneOpFastExit2:	; this is exit for state != OK
	add rsp, 32
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
; inner interpreter C entry point
;
; extern eForthResult InnerInterpreterFast( ForthCoreState *pCore );
entry InnerInterpreterFast
	push rbx
    push rdi
    push rsi
    push r12
    push r13
    push r14
    push r15
	; stack should be 16-byte aligned at this point
    
    mov rcore, rcx                        ; rcore -> ForthCoreState
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
;   call has saved all non-volatile registers
;   rcore is set by caller
;	jump to interpFuncReenter if you need to reload IP, SP, interpLoop
entry interpFunc
	sub rsp, 8			; 16-byte align stack
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
	; while debugging, store IP,SP in corestate shadow copies after every instruction
	;   so crash stacktrace will be more accurate (off by only one instruction)
	mov	[rcore + FCore.IPtr], rip
	mov	[rcore + FCore.SPtr], rpsp
	mov	[rcore + FCore.RPtr], rrp
	mov	[rcore + FCore.FPtr], rfp
entry interpLoop
	mov	ebx, [rip]		; rbx is opcode
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
    mov rcx, rcore      ; 1st param - core
    mov rdx, rax        ; 2nd param - IP
    mov r8, rbx         ; 3rd param - opcode (used if IP param is null)
	sub rsp, 32			; shadow space
	xcall traceOp
	add rsp, 32
	mov roptab, [rcore + FCore.ops]
	mov rnumops, [rcore + FCore.numOps]
	mov racttab, [rcore + FCore.optypeAction]
	add	rip, 4			; advance IP
	jmp interpLoopExecuteEntry

interpLoopExit:
	add rsp, 8
	mov	[rcore + FCore.state], rax
	mov	[rcore + FCore.IPtr], rip
	mov	[rcore + FCore.SPtr], rpsp
	mov	[rcore + FCore.RPtr], rrp
	mov	[rcore + FCore.FPtr], rfp
	ret

badOptype:
	mov	rax, kForthErrorBadOpcodeType
	jmp	interpLoopErrorExit

badVarOperation:
	mov	rax, kForthErrorBadVarOperation
	jmp	interpLoopErrorExit
	
badOpcode:
	mov	rax, kForthErrorBadOpcode

	
interpLoopErrorExit:
	; error exit point
	; rax is error code
	mov	[rcore + FCore.error], rax
	mov	rax, kResultError
	jmp	interpLoopExit
	
interpLoopFatalErrorExit:
	; fatal error exit point
	; rax is error code
	mov	[rcore + FCore.error], rax
	mov	rax, kResultFatalError
	jmp	interpLoopExit
	
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
    jmp rnext
    
; externalBuiltin is invoked when a builtin op which is outside of range of table is invoked
externalBuiltin:
	; it should be impossible to get here now
	jmp	badOpcode
	
;-----------------------------------------------
;
; cCodeType is used by "builtin" ops which are only defined in C++
;
;	rbx holds the opcode
;
cCodeType:
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
    mov rcx, rcore      ; 1st param - core
	sub rsp, 32			; shadow space
	call	rax
	add rsp, 32
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
	jnz	interpLoopExit		; if something went wrong (should this be interpLoopErrorExit?)
	jmp	rnext					; if everything is ok
	
; NOTE: Feb. 14 '07 - doing the right thing here - restoring IP & SP and jumping to
; the interpreter loop exit point - causes an access violation exception ?why?
	;mov	rip, [rcore + FCore.IPtr]
	;mov	rpsp, [rcore + FCore.SPtr]
	;jmp	interpLoopExit	; if something went wrong
	

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
	jmp	interpLoopErrorExit

;-----------------------------------------------
;
; user-defined ops (forth words defined with colon)
;
entry relativeDefType
	; rbx is offset (in longs) from dictionary base of user definition
	and	rbx, 00FFFFFFh
	sal	rbx, 2
	mov	rax, [rcore + FCore.DictionaryPtr]
	add	rbx, [rax + ForthMemorySection.pBase]
	cmp	rbx, [rax + ForthMemorySection.pCurrent]
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
	add	rbx, [rax + ForthMemorySection.pBase]
	cmp	rbx, [rax + ForthMemorySection.pCurrent]
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
	mov	rcx, [rax + ForthMemorySection.pBase]
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
; local byte ops
;
entry localByteType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz localByteType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
localByteType1:
	; get ptr to byte var into rax
	mov	rax, rfp
	and	rbx, 001FFFFFh
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
	jmp	rnext

entry localUByteType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz localUByteType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
localUByteType1:
	; get ptr to byte var into rax
	mov	rax, rfp
	and	rbx, 001FFFFFh
	sal	rbx, 3
	sub	rax, rbx
	; see if it is a fetch
ubyteEntry:
	mov	rbx, [rcore + FCore.varMode]
	or	rbx, rbx
	jnz	localByte1
	; fetch local unsigned byte
localUByteFetch:
	sub	rpsp, 8
	xor	rbx, rbx
	mov	bl, [rax]
	mov	[rpsp], rbx
	jmp	rnext

localByte1:
	cmp	rbx, kVarMinusStore
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, localByteActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx

localByteRef:
	sub	rpsp, 8
	mov	[rpsp], rax
	; set var operation back to fetch
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext
	
localByteStore:
	mov	rbx, [rpsp]
	mov	[rax], bl
	add	rpsp, 8
	; set var operation back to fetch
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localBytePlusStore:
	xor	rbx, rbx
	mov	bl, [rax]
	add	rbx, [rpsp]
	mov	[rax], bl
	add	rpsp, 8
	; set var operation back to fetch
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localByteMinusStore:
	xor	rbx, rbx
	mov	bl, [rax]
	sub	rbx, [rpsp]
	mov	[rax], bl
	add	rpsp, 8
	; set var operation back to fetch
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localByteActionTable:
	DQ	localByteFetch
	DQ	localByteFetch
	DQ	localByteRef
	DQ	localByteStore
	DQ	localBytePlusStore
	DQ	localByteMinusStore

localUByteActionTable:
	DQ	localUByteFetch
	DQ	localUByteFetch
	DQ	localByteRef
	DQ	localByteStore
	DQ	localBytePlusStore
	DQ	localByteMinusStore

entry fieldByteType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz fieldByteType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
fieldByteType1:
	; get ptr to byte var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, 001FFFFFh
	add	rax, rbx
	jmp	byteEntry

entry fieldUByteType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz fieldUByteType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
fieldUByteType1:
	; get ptr to byte var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, 001FFFFFh
	add	rax, rbx
	jmp	ubyteEntry

entry memberByteType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz memberByteType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
memberByteType1:
	; get ptr to byte var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, 001FFFFFh
	add	rax, rbx
	jmp	byteEntry

entry memberUByteType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz memberUByteType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
memberUByteType1:
	; get ptr to byte var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, 001FFFFFh
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
; local short ops
;
entry localShortType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz localShortType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
localShortType1:
	; get ptr to short var into rax
	mov	rax, rfp
	and	rbx, 001FFFFFh
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
	jmp	rnext

entry localUShortType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz localUShortType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
localUShortType1:
	; get ptr to short var into rax
	mov	rax, rfp
	and	rbx, 001FFFFFh
	sal	rbx, 3
	sub	rax, rbx
	; see if it is a fetch
ushortEntry:
	mov	rbx, [rcore + FCore.varMode]
	or	rbx, rbx
	jnz	localShort1
	; fetch local unsigned short
localUShortFetch:
	sub	rpsp, 8
    ; TODO: is this correct way to sign extend?
	movsx	rbx, WORD[rax]
	xor	rbx, rbx
	mov	bx, [rax]
	mov	[rpsp], rbx
	jmp	rnext

localShort1:
	cmp	rbx, kVarMinusStore
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, localShortActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx
localShortRef:
	sub	rpsp, 8
	mov	[rpsp], rax
	; set var operation back to fetch
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext
	
localShortStore:
	mov	rbx, [rpsp]
	mov	[rax], bx
	add	rpsp, 8
	; set var operation back to fetch
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localShortPlusStore:
	movsx	rbx, WORD[rax]
	add	rbx, [rpsp]
	mov	[rax], bx
	add	rpsp, 8
	; set var operation back to fetch
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localShortMinusStore:
	movsx	rbx, WORD[rax]
	sub	rbx, [rpsp]
	mov	[rax], bx
	add	rpsp, 8
	; set var operation back to fetch
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localShortActionTable:
	DQ	localShortFetch
	DQ	localShortFetch
	DQ	localShortRef
	DQ	localShortStore
	DQ	localShortPlusStore
	DQ	localShortMinusStore

localUShortActionTable:
	DQ	localUShortFetch
	DQ	localUShortFetch
	DQ	localShortRef
	DQ	localShortStore
	DQ	localShortPlusStore
	DQ	localShortMinusStore

entry fieldShortType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz fieldShortType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
fieldShortType1:
	; get ptr to short var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, 001FFFFFh
	add	rax, rbx
	jmp	shortEntry

entry fieldUShortType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz fieldUShortType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
fieldUShortType1:
	; get ptr to unsigned short var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, 001FFFFFh
	add	rax, rbx
	jmp	ushortEntry

entry memberShortType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz memberShortType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
memberShortType1:
	; get ptr to short var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, 001FFFFFh
	add	rax, rbx
	jmp	shortEntry

entry memberUShortType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz memberUShortType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
memberUShortType1:
	; get ptr to unsigned short var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, 001FFFFFh
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
; local int ops
;
entry localIntType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz localIntType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
localIntType1:
	; get ptr to int var into rax
	mov	rax, rfp
	and	rbx, 001FFFFFh
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
	jmp	rnext

localIntRef:
	sub	rpsp, 8
	mov	[rpsp], rax
	; set var operation back to fetch
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext
	
localIntStore:
	mov	ebx, [rpsp]
	mov	[rax], ebx
	add	rpsp, 8
	; set var operation back to fetch
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localIntPlusStore:
	mov	rbx, [rax]
	add	rbx, [rpsp]
	mov	[rax], rbx
	add	rpsp, 8
	; set var operation back to fetch
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localIntMinusStore:
	mov	rbx, [rax]
	sub	rbx, [rpsp]
	mov	[rax], rbx
	add	rpsp, 8
	; set var operation back to fetch
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localIntActionTable:
	DQ	localIntFetch
	DQ	localIntFetch
	DQ	localIntRef
	DQ	localIntStore
	DQ	localIntPlusStore
	DQ	localIntMinusStore

localInt1:
	cmp	rbx, kVarMinusStore
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, localIntActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx

entry localUIntType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz localIntType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
localUIntType1:
	; get ptr to int var into rax
	mov	rax, rfp
	and	rbx, 001FFFFFh
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
	mov	[rcore + FCore.varMode], rbx
	mov	ebx, DWORD[rax]
	mov	[rpsp], rbx
	jmp	rnext

localUIntActionTable:
	DQ	localUIntFetch
	DQ	localUIntFetch
	DQ	localIntRef
	DQ	localIntStore
	DQ	localIntPlusStore
	DQ	localIntMinusStore

localUInt1:
	cmp	rbx, kVarMinusStore
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, localUIntActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx

entry fieldIntType
	; get ptr to int var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz fieldIntType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
fieldIntType1:
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, 001FFFFFh
	add	rax, rbx
	jmp	intEntry

entry fieldUIntType
	; get ptr to uint var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz fieldUIntType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
fieldUIntType1:
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, 001FFFFFh
	add	rax, rbx
	jmp	uintEntry

entry memberIntType
	; get ptr to int var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz memberIntType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
memberIntType1:
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, 001FFFFFh
	add	rax, rbx
	jmp	intEntry

entry memberUIntType
	; get ptr to int var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz memberUIntType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
memberUIntType1:
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, 001FFFFFh
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
; local float ops
;
entry localFloatType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz localFloatType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
localFloatType1:
	; get ptr to float var into rax
	mov	rax, rfp
	and	rbx, 001FFFFFh
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
	mov	ebx, [rax]
	mov	[rpsp], rbx
	jmp	rnext

localFloatRef:
	sub	rpsp, 8
	mov	[rpsp], rax
	; set var operation back to fetch
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext
	
localFloatStore:
	mov	rbx, [rpsp]
	mov	[rax], ebx
	add	rpsp, 8
	; set var operation back to fetch
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localFloatPlusStore:
	movss xmm0, DWORD[rax]
    addss xmm0, DWORD[rpsp]
    movss DWORD[rax], xmm0
	add	rpsp, 8
	; set var operation back to fetch
	xor	rbx, rbx
	mov	[rcore + FCore.varMode], rbx
	jmp	rnext

localFloatMinusStore:
	movss xmm0, DWORD[rax]
    subss xmm0, DWORD[rpsp]
    movss DWORD[rax], xmm0
	add	rpsp, 8
	; set var operation back to fetch
	xor	rbx, rbx
	mov	[rcore + FCore.varMode], rbx
	jmp	rnext

localFloatActionTable:
	DQ	localFloatFetch
	DQ	localFloatFetch
	DQ	localFloatRef
	DQ	localFloatStore
	DQ	localFloatPlusStore
	DQ	localFloatMinusStore

localFloat1:
	cmp	rbx, kVarMinusStore
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, localFloatActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx

entry fieldFloatType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz fieldFloatType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
fieldFloatType1:
	; get ptr to float var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, 001FFFFFh
	add	rax, rbx
	jmp	floatEntry

entry memberFloatType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz memberFloatType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
memberFloatType1:
	; get ptr to float var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, 001FFFFFh
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
; local double ops
;
entry localDoubleType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz localDoubleType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
localDoubleType1:
	; get ptr to double var into rax
	mov	rax, rfp
	and	rbx, 001FFFFFh
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
	jmp	rnext

localDoubleRef:
	sub	rpsp, 8
	mov	[rpsp], rax
	; set var operation back to fetch
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext
	
localDoubleStore:
	mov	rbx, [rpsp]
	mov	[rax], rbx
	add	rpsp, 8
	; set var operation back to fetch
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localDoublePlusStore:
	movsd   xmm0, QWORD[rax]
    addsd   xmm0, QWORD[rpsp]
    movsd QWORD[rax], xmm0
	add	rpsp, 8
	; set var operation back to fetch
	xor	rbx, rbx
	mov	[rcore + FCore.varMode], rbx
	jmp	rnext

localDoubleMinusStore:
	movsd xmm0, QWORD[rax]
    subsd xmm0, [rpsp]
    movsd QWORD[rax], xmm0
	add	rpsp, 8
	; set var operation back to fetch
	xor	rbx, rbx
	mov	[rcore + FCore.varMode], rbx
	jmp	rnext

localDoubleActionTable:
	DQ	localDoubleFetch
	DQ	localDoubleFetch
	DQ	localDoubleRef
	DQ	localDoubleStore
	DQ	localDoublePlusStore
	DQ	localDoubleMinusStore

localDouble1:
	cmp	rbx, kVarMinusStore
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, localDoubleActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx

entry fieldDoubleType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz fieldDoubleType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
fieldDoubleType1:
	; get ptr to double var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, 001FFFFFh
	add	rax, rbx
	jmp	doubleEntry

entry memberDoubleType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz memberDoubleType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
memberDoubleType1:
	; get ptr to double var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, 001FFFFFh
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
; local string ops
;
GLOBAL localStringType, stringEntry, localStringFetch, localStringStore, localStringAppend
entry localStringType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz localStringType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
localStringType1:
	; get ptr to string var into rax
	mov	rax, rfp
	and	rbx, 001FFFFFh
	sal	rbx, 3
	sub	rax, rbx
	; see if it is a fetch
stringEntry:
	mov	rbx, [rcore + FCore.varMode]
	or	rbx, rbx
	jnz	localString1
	; fetch local string
localStringFetch:
	sub	rpsp, 8
	add	rax, 8		; skip maxLen & currentLen fields
	mov	[rpsp], rax
	; set var operation back to fetch
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localString1:
	cmp	rbx, kVarPlusStore
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, localStringActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx
	
; ref on a string returns the address of maxLen field, not the string chars
localStringRef:
	sub	rpsp, 8
	mov	[rpsp], rax
	; set var operation back to fetch
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext
	
localStringStore:
	; rax -> dest string maxLen field
	; TOS is src string addr
    mov rbx, rax            ; strlen will stomp rax
	mov	rcx, [rpsp]			; rcx -> chars of src string
	sub rsp, 32			; shadow space
	xcall	strlen
	add rsp, 32
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
    lea rcx, [rbx + 8]      ; 1st param - dest pointer
    mov rdx, [rpsp]         ; 2nd param - src pointer
    mov r8, rax             ; 3rd param - num chars to copy

    add rpsp, 8
    mov rbx, rcx
    add rbx, rax            ; rbx -> end of dest string
	; add the terminating null
	xor	rax, rax
	mov	[rbx], al
	; set var operation back to fetch
	mov	[rcore + FCore.varMode], rax

	sub rsp, 32			; shadow space
	xcall	memcpy
	add rsp, 32

	jmp	restoreNext

localStringAppend:
	; rax -> dest string maxLen field
	; TOS is src string addr
    mov rbx, rax            ; strlen will stomp rax
	mov	rcx, [rpsp]			; rcx -> chars of src string
	sub rsp, 32			; shadow space
	xcall	strlen
	add rsp, 32
	; rax is src string length
	; rbx -> dest string maxLen field
    ; TOS -> src string
	; figure how many chars to copy
    mov r8, [rbx]
    mov rcx, [rbx + 4]
    sub r8, rcx       ; r8 is max bytes to copy
	cmp	rax, r8
	jg lsAppend1
    mov r8, rax
lsAppend1:
    ; r8 is #bytes to copy
    ; rcx is current length
    
	; set current length field
    add rcx, r8
    mov [rbx + 4], rcx
	
	; do the copy
    lea rcx, [rbx + 8]      ; 1st param - dest pointer
    add rcx, [rbx + 4]      ;   at end of current dest string
    mov rdx, [rpsp]         ; 2nd param - src pointer
    add rpsp, 8
    ; 3rd param - num chars to copy - already in r8
    mov rbx, rcx
    add rbx, r8            ; rbx -> end of dest string
	sub rsp, 32			; shadow space
	xcall	memcpy
	add rsp, 32
    
	; add the terminating null
	xor	rax, rax
	mov	[rbx], al
		
	; set var operation back to fetch
	mov	[rcore + FCore.varMode], rax
	jmp	restoreNext

localStringActionTable:
	DQ	localStringFetch
	DQ	localStringFetch
	DQ	localStringRef
	DQ	localStringStore
	DQ	localStringAppend

entry fieldStringType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz fieldStringType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
fieldStringType1:
	; get ptr to byte var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, 001FFFFFh
	add	rax, rbx
	jmp	stringEntry

entry memberStringType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz memberStringType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
memberStringType1:
	; get ptr to byte var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, 001FFFFFh
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
; local op ops
;
entry localOpType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz localOpType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
localOpType1:
	; get ptr to op var into rbx
	mov	rax, rfp
	and	rbx, 001FFFFFh
	sal	rbx, 3
	sub	rax, rbx
	; see if it is a fetch (execute for ops)
opEntry:
	mov	rbx, [rcore + FCore.varMode]
	or	rbx, rbx
	jnz	localOp1
	; execute local op
localOpExecute:
	mov	ebx, [rax]
	mov	rax, [rcore + FCore.innerExecute]
	jmp rax

localOpActionTable:
	DQ	localOpExecute
	DQ	localUIntFetch
	DQ	localIntRef
	DQ	localIntStore

localOp1:
	cmp	rbx, kVarStore
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, localOpActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx

entry fieldOpType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz fieldOpType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
fieldOpType1:
	; get ptr to op var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, 001FFFFFh
	add	rax, rbx
	jmp	opEntry

entry memberOpType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz memberOpType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
memberOpType1:
	; get ptr to op var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, 001FFFFFh
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
; local long (int64) ops
;
entry localLongType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz localLongType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
localLongType1:
	; get ptr to long var into rax
	mov	rax, rfp
	and	rbx, 001FFFFFh
	sal	rbx, 3
	sub	rax, rbx
	; see if it is a fetch
longEntry:
	mov	rbx, [rcore + FCore.varMode]
	or	rbx, rbx
	jnz	localLong1
	; fetch local double
localLongFetch:
	sub	rpsp, 8
	mov	rbx, [rax]
	mov	[rpsp], rbx
	jmp	rnext

localLong1:
	cmp	rbx, kVarMinusStore
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, localLongActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx

localLongRef:
	sub	rpsp, 8
	mov	[rpsp], rax
	; set var operation back to fetch
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext
	
localLongStore:
	mov	rbx, [rpsp]
	mov	[rax], rbx
	add	rpsp, 8
	; set var operation back to fetch
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

localLongPlusStore:
	mov	rbx, [rax]
	add	rbx, [rpsp]
	mov	[rax], rbx
	; set var operation back to fetch
	xor	rbx, rbx
	mov	[rcore + FCore.varMode], rbx
	add	rpsp, 8
	jmp	rnext

localLongMinusStore:
	mov	rbx, [rax]
	sub	rbx, [rpsp]
	mov	[rax], rbx
	; set var operation back to fetch
	xor	rbx, rbx
	mov	[rcore + FCore.varMode], rbx
	add	rpsp, 8
	jmp	rnext

localLongActionTable:
	DQ	localLongFetch
	DQ	localLongFetch
	DQ	localLongRef
	DQ	localLongStore
	DQ	localLongPlusStore
	DQ	localLongMinusStore

entry fieldLongType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz fieldLongType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
fieldLongType1:
	; get ptr to double var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, 001FFFFFh
	add	rax, rbx
	jmp	longEntry

entry memberLongType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz memberLongType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
memberLongType1:
	; get ptr to double var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, 001FFFFFh
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
; local object ops
;
entry localObjectType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz localObjectType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
localObjectType1:
	; get ptr to Object var into rax
	mov	rax, rfp
	and	rbx, 001FFFFFh
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
	jmp	rnext

localObject1:
	cmp	rbx, kVarObjectClear
	jg	badVarOperation
	; dispatch based on value in rbx
	mov rcx, localObjectActionTable
	mov	rbx, [rcx + rbx*8]
	jmp	rbx

localObjectRef:
	sub	rpsp, 8
	mov	[rpsp], rax
	; set var operation back to fetch
	xor	rax, rax
	mov	[rcore + FCore.varMode], rax
	jmp	rnext
	
entry localObjectStore
	; TOS is new object, rax points to destination/old object
	xor	rbx, rbx			; set var operation back to default/fetch
los0:
	mov	[rcore + FCore.varMode], rbx
	mov rbx, [rax]		; rbx = olbObj
	mov rcx, rax		; rcx -> destination
	mov rax, [rpsp]		; rax = newObj
	add	rpsp, 8
	mov	[rcx], rax		; var = newObj
	cmp rax, rbx
	jz losx				; objects are same, don't change refcount
	; handle newObj refcount
	or rax, rax
	jz los1				; if newObj is null, don't try to increment refcount
	inc QWORD[rax + Object.refCount]	; increment newObj refcount
	; handle oldObj refcount
los1:
	or rbx, rbx
	jz losx				; if oldObj is null, don't try to decrement refcount
	dec QWORD[rbx + Object.refCount]
	jz los3
losx:
	jmp	rnext

	; object var held last reference to oldObj, invoke olbObj.delete method
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
	; set var operation back to fetch
	mov	[rcore + FCore.varMode], rax
	; get object refcount, see if it is already 0
	mov	rax, [rbx + Object.refCount]
	or	rax, rax
	jnz	lou1
	; report refcount negative error
	mov	rax, kForthErrorBadReferenceCount
	jmp	interpLoopErrorExit
lou1:
	; decrement object refcount
	sub	rax, 1
	mov	[rbx + Object.refCount], rax
lou2:
	jmp	rnext

	
localObjectActionTable:
	DQ	localObjectFetch
	DQ	localObjectFetch
	DQ	localObjectRef
	DQ	localObjectStore
	DQ	localObjectStoreNoRef
	DQ	localObjectUnref
	DQ	localObjectClear

entry fieldObjectType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz fieldObjectType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
fieldObjectType1:
	; get ptr to Object var into rax
	; TOS is base ptr, rbx is field offset in bytes
	mov	rax, [rpsp]
	add	rpsp, 8
	and	rbx, 001FFFFFh
	add	rax, rbx
	jmp	objectEntry

entry memberObjectType
	mov	rax, rbx
	; see if a varop is specified
	and	rax, 00E00000h
	jz memberObjectType1
	shr	rax, 21
	mov	[rcore + FCore.varMode], rax
memberObjectType1:
	; get ptr to Object var into rax
	; this data ptr is base ptr, rbx is field offset in bytes
	mov	rax, [rcore + FCore.TPtr]
	and	rbx, 001FFFFFh
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
	jmp	interpLoopErrorExit
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
	jmp	interpLoopErrorExit
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
	
; invoke a method on super class of object currently referenced by this ptr pair
entry methodWithSuperType
	; rbx is method number
	sub	rrp, 16
	mov	rax, [rcore + FCore.TPtr]
	; push original methods ptr on return stack
	mov	rcx, [rax]			; rcx is original methods ptr
	mov	[rrp + 8], rcx
	; push this on return stack
	mov	[rrp], rax
	
	mov	rcx, [rcx - 8]		; rcx -> class vocabulary object
	mov	rcx, [rcx + 16]		; 1st param rcx -> class vocabulary
	sub rsp, 32			; shadow space
	xcall getSuperClassMethods
	add rsp, 32
	mov roptab, [rcore + FCore.ops]
	mov rnumops, [rcore + FCore.numOps]
	mov racttab, [rcore + FCore.optypeAction]
	; rax -> super class methods table
	mov	rcx, [rcore + FCore.TPtr]
	mov	[rcx], rax		; set this methods ptr to super class methods
	; rbx is method number
	and	rbx, 00FFFFFFh
	sal	rbx, 2
	add	rbx, rax
	mov	ebx, [rbx]	; rbx = method opcode
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
	jmp	interpLoopErrorExit

strFixupBop3:
	mov	[rcx-4], rax
	jmp	rnext

;========================================

entry doneBop
	mov	rax,kResultDone
	jmp	interpLoopExit

;========================================

entry abortBop
	mov	rax,kForthErrorAbort
	jmp	interpLoopFatalErrorExit

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
    movsd xmm1, QWORD[rpsp]
    add rpsp, 8
    movsd xmm0, QWORD[rpsp]
	sub rsp, 32			; shadow space
	xcall	atan2
	add rsp, 32
    movsd QWORD[rpsp], xmm0
	jmp	restoreNext
	
;========================================
	
entry fatan2Bop
    movss xmm1, DWORD[rpsp]
    add rpsp, 8
    movss xmm0, DWORD[rpsp]
	sub rsp, 32			; shadow space
	xcall	atan2f
	add rsp, 32
    movss DWORD[rpsp], xmm0
	jmp	restoreNext
	
;========================================
	
entry dpowBop
	; a^x
    movsd xmm1, QWORD[rpsp]
    add rpsp, 8
    movsd xmm0, QWORD[rpsp]
	sub rsp, 32			; shadow space
	xcall	pow
	add rsp, 32
    movsd QWORD[rpsp], xmm0
	jmp	restoreNext
	
;========================================
	
entry fpowBop
	; a^x
    movss xmm1, DWORD[rpsp]
    add rpsp, 8
    movss xmm0, DWORD[rpsp]
	sub rsp, 32			; shadow space
	xcall	powf
	add rsp, 32
    xor rax, rax
    movd eax, xmm0
    mov [rpsp], rax
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
    mov rdx, [rpsp]
	sub rsp, 32			; shadow space
	xcall ldexp
	add rsp, 32
	add	rpsp, 8
    movsd QWORD[rpsp], xmm0
	jmp	restoreNext
	
;========================================

entry fldexpBop
	; ldexpf( a, n )
	; TOS is n (int), a (float)
	; get arg a
    movss xmm0, DWORD[rpsp + 8]
	; get arg n
    mov rdx, [rpsp]
	sub rsp, 32			; shadow space
	xcall ldexpf
	add rsp, 32
	add	rpsp, 8
    xor rax, rax
    movd eax, xmm0
    mov [rpsp], rax
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
    mov rdx, rpsp
	sub rsp, 32			; shadow space
	xcall frexp
	add rsp, 32
    movsd QWORD[rpsp + 8], xmm0
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
    mov rdx, rpsp
	sub rsp, 32			; shadow space
	xcall frexpf
	add rsp, 32
    xor rax, rax
    movd eax, xmm0
    mov [rpsp + 8], rax
	jmp	restoreNext
	
;========================================

entry dmodfBop
	; modf( a, ptrToDoubleWholeReturn )
	; get arg a
    movsd xmm0, QWORD[rpsp]
	; get arg ptrToDoubleWholeReturn
    mov rdx, rpsp
	sub rsp, 32			; shadow space
	xcall modf
	add rsp, 32
    sub rpsp, 8
    movsd QWORD[rpsp], xmm0
	jmp	restoreNext
	
;========================================

entry fmodfBop
	; modf( a, ptrToFloatWholeReturn )
	; get arg a
    movss xmm0, DWORD[rpsp]
	; get arg ptrToIntExponentReturn
    mov rdx, rpsp
    xor rax, rax
    mov [rpsp], rax
	sub rsp, 32			; shadow space
	xcall modff
	add rsp, 32
    sub rpsp, 8
    xor rax, rax
    movd eax, xmm0
    mov [rpsp], rax
	jmp	restoreNext
	
;========================================

entry dfmodBop
    ; fmod(numerator denominator)
    ; get arg denominator
    movsd xmm1, QWORD[rpsp]
    add rpsp, 8
    ; get arg numerator
    movsd xmm0, QWORD[rpsp]
	sub rsp, 32			; shadow space
	xcall fmod
	add rsp, 32
    movsd QWORD[rpsp], xmm0
	jmp	restoreNext
	
;========================================

entry ffmodBop
    ; fmodf(numerator denominator)
    ; get arg denominator
    movss xmm1, DWORD[rpsp]
    add rpsp, 8
    ; get arg numerator
    movss xmm0, DWORD[rpsp]
	sub rsp, 32			; shadow space
	xcall fmodf
	add rsp, 32
    xor rax, rax
    movd eax, xmm0
    mov [rpsp], rax
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
	jmp	interpLoopErrorExit
	
doExitBop2:
	mov	rax, kForthErrorParamStackUnderflow
	jmp	interpLoopErrorExit
	
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
	mov	rbx, [rpsp]		; rbx = b
	add	rpsp, 8
	xor	rax, rax
	cmp	[rpsp], rbx
	jz	cmpBop3
	jl	cmpBop2
	add	rax, 2
cmpBop2:
	dec	rax
cmpBop3:
	mov	[rpsp], rax
	jmp	rnext

;========================================

entry uicmpBop
	mov	rbx, [rpsp]
	add	rpsp, 8
	xor	rax, rax
	cmp	[rpsp], rbx
	jz	ucmpBop3
	jb	ucmpBop2
	add	rax, 2
ucmpBop2:
	dec	rax
ucmpBop3:
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
	cmp	rbx, kVarMinusStore
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
    mov	rbx, [rax + ForthMemorySection.pCurrent]
    sub rpsp, 8
    mov [rpsp], rbx
    jmp rnext

;========================================

entry dpBop
    mov	rax, [rcore + FCore.DictionaryPtr]
    lea	rbx, [rax + ForthMemorySection.pCurrent]
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
	mov	r8, [rpsp]
	mov	rdx, [rpsp + 16]
	mov	rcx, [rpsp + 8]
	sub rsp, 32			; shadow space
	xcall	memmove
	add rsp, 32
	add	rpsp, 24
	jmp	restoreNext

;========================================

entry memcmpBop
	;	TOS: nBytes mem2Ptr mem1Ptr
	mov	r8, [rpsp]
	mov	rdx, [rpsp + 8]
	mov	rcx, [rpsp + 16]
	sub rsp, 32			; shadow space
	xcall	memcmp
	add rsp, 32
	add	rpsp, 16
    mov [rpsp], rax
	jmp	restoreNext

;========================================

entry fillBop
	;	TOS: byteVal nBytes dstPtr
	mov	rdx, [rpsp]
	mov	r8, [rpsp + 8]
	mov	rcx, [rpsp + 16]
	sub rsp, 32			; shadow space
	xcall	memset
	add rsp, 32
	add	rpsp, 24
	jmp	restoreNext

;========================================

entry fetchVaractionBop
	mov	rax, kVarFetch
	mov	[rcore + FCore.varMode], rax
	jmp	rnext
	
;========================================

entry intoVaractionBop
	mov	rax, kVarStore
	mov	[rcore + FCore.varMode], rax
	jmp	rnext
	
;========================================

entry addToVaractionBop
	mov	rax, kVarPlusStore
	mov	[rcore + FCore.varMode], rax
	jmp	rnext
	
;========================================

entry subtractFromVaractionBop
	mov	rax, kVarMinusStore
	mov	[rcore + FCore.varMode], rax
	jmp	rnext
	
;========================================

entry oclearVaractionBop
	mov	rax, kVarObjectClear
	mov	[rcore + FCore.varMode], rax
	jmp	rnext
		
;========================================

entry odropBop
	; TOS is object to check - if its refcount is already 0, invoke delete method
	;  otherwise do nothing
	mov	rax, [rpsp]
	add	rpsp, 8
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

entry setVarActionBop
	mov   rax, [rpsp]
	add   rpsp, 8
	mov	[rcore + FCore.varMode], rax
	jmp	rnext

;========================================

entry getVarActionBop
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
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
	sub rsp, 32			; shadow space
	xcall	strcpy
	add rsp, 32
	add	rpsp, 16
	jmp	restoreNext

;========================================

entry strncpyBop
	;	TOS: maxBytes srcPtr dstPtr
	mov	r8, [rpsp]
	mov	rdx, [rpsp + 8]
	mov	rcx, [rpsp + 16]
	sub rsp, 32			; shadow space
	xcall	strncpy
	add rsp, 32
	add	rpsp, 24
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
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
	sub rsp, 32			; shadow space
	xcall	strcat
	add rsp, 32
	add	rpsp, 16
	jmp	restoreNext

;========================================

entry strncatBop
	;	TOS: maxBytes srcPtr dstPtr
	mov	r8, [rpsp]
	mov	rdx, [rpsp + 8]
	mov	rcx, [rpsp + 16]
	sub rsp, 32			; shadow space
	xcall	strncat
	add rsp, 32
	add	rpsp, 24
	jmp	restoreNext

;========================================

entry strchrBop
	;	TOS: char strPtr
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
	sub rsp, 32			; shadow space
	xcall	strchr
	add rsp, 32
	add	rpsp, 8
	mov	[rpsp], rax
	jmp	restoreNext
	
;========================================

entry strrchrBop
	;	TOS: char strPtr
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
	sub rsp, 32			; shadow space
	xcall	strrchr
	add rsp, 32
	add	rpsp, 8
	mov	[rpsp], rax
	jmp	restoreNext
	
;========================================

entry strcmpBop
	;	TOS: ptr2 ptr1
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
	sub rsp, 32			; shadow space
	xcall	strcmp
	add rsp, 32
strcmp1:
	xor	rbx, rbx
	cmp	rax, rbx
	jz	strcmp3		; if strings equal, return 0
	jg	strcmp2
	sub	rbx, 2
strcmp2:
	inc	rbx
strcmp3:
	add	rpsp, 8
	mov	[rpsp], rbx
	jmp	restoreNext
	
;========================================

entry stricmpBop
	;	TOS: ptr2 ptr1
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
	sub rsp, 32			; shadow space
%ifdef WIN64
    xcall	stricmp
%else
	xcall	strcasecmp
%endif
	add rsp, 32
	jmp	strcmp1
	
;========================================

entry strncmpBop
	;	TOS: numChars ptr2 ptr1
	mov	r8, [rpsp]
	mov	rdx, [rpsp + 8]
	mov	rcx, [rpsp + 16]
	sub rsp, 32			; shadow space
	xcall	strncmp
	add rsp, 32
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
	jmp	restoreNext
	
;========================================

entry strstrBop
	;	TOS: ptr2 ptr1
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
	sub rsp, 32			; shadow space
	xcall	strstr
	add rsp, 32
	add	rpsp, 8
	mov	[rpsp], rax
	jmp	restoreNext
	
;========================================

entry strtokBop
	;	TOS: ptr2 ptr1
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
	sub rsp, 32			; shadow space
	xcall	strtok
	add rsp, 32
	add	rpsp, 8
	mov	[rpsp], rax
	jmp	restoreNext
	
;========================================

entry litBop
entry flitBop
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

entry unsuperBop
	; pop original pMethods off rstack
	mov	rbx, [rrp]
	add	rrp, 8
	; store original pMethods in this.pMethods
	mov	rax, [rcore + FCore.TPtr]
	mov	[rax], rbx
	jmp	rnext
	
;========================================

entry executeBop
	mov	ebx, [rpsp]
	add	rpsp, 8
	mov	rax, [rcore + FCore.innerExecute]
	jmp rax
	
;========================================

entry	fopenBop
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.fileOpen]
	sub rsp, 32			; shadow space
	call rax
	add rsp, 32
	add	rpsp, 8
	mov	[rpsp], rax	; push fopen result
	jmp	restoreNext
	
;========================================

entry	fcloseBop
	mov	rcx, [rpsp]
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.fileClose]
	sub rsp, 32			; shadow space
	call rax
	add rsp, 32
	mov	[rpsp], rax	; push fclose result
	jmp	restoreNext
	
;========================================

entry	fseekBop
	mov	r8, [rpsp]
	mov	rdx, [rpsp + 8]
	mov	rcx, [rpsp + 16]
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.fileSeek]
	sub rsp, 32			; shadow space
	call rax
	add rsp, 32
	add	rpsp, 16
	mov	[rpsp], rax	; push fseek result
	jmp	restoreNext
	
;========================================

entry	freadBop
	mov	r9, [rpsp]
	mov	r8, [rpsp + 8]
	mov	rdx, [rpsp + 16]
	mov	rcx, [rpsp + 24]
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.fileRead]
	sub rsp, 32			; shadow space
	call rax
	add rsp, 32
	add	rpsp, 24
	mov	[rpsp], rax	; push fread result
	jmp	restoreNext
	
;========================================

entry	fwriteBop
	mov	r9, [rpsp]
	mov	r8, [rpsp + 8]
	mov	rdx, [rpsp + 16]
	mov	rcx, [rpsp + 24]
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.fileWrite]
	sub rsp, 32			; shadow space
	call rax
	add rsp, 32
	add	rpsp, 24
	mov	[rpsp], rax	; push fwrite result
	jmp	restoreNext
	
;========================================

entry	fgetcBop
	mov	rcx, [rpsp]
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.fileGetChar]
	sub rsp, 32			; shadow space
	call rax
	add rsp, 32
	mov	[rpsp], rax	; push fgetc result
	jmp	restoreNext
	
;========================================

entry	fputcBop
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.filePutChar]
	sub rsp, 32			; shadow space
	call rax
	add rsp, 32
	add rpsp, 8
	mov	[rpsp], rax	; push fputc result
	jmp	restoreNext
	
;========================================

entry	feofBop
	mov	rcx, [rpsp]
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.fileAtEnd]
	sub rsp, 32			; shadow space
	call rax
	add rsp, 32
	mov	[rpsp], rax	; push feof result
	jmp	restoreNext
	
;========================================

entry	fexistsBop
	mov	rcx, [rpsp]
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.fileExists]
	sub rsp, 32			; shadow space
	call rax
	add rsp, 32
	mov	[rpsp], rax	; push fexists result
	jmp	restoreNext
	
;========================================

entry	ftellBop
	mov	rcx, [rpsp]
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.fileTell]
	sub rsp, 32			; shadow space
	call rax
	add rsp, 32
	mov	[rpsp], rax	; push ftell result
	jmp	restoreNext
	
;========================================

entry	flenBop
	mov	rcx, [rpsp]
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.fileGetLength]
	sub rsp, 32			; shadow space
	call rax
	add rsp, 32
	mov	[rpsp], rax	; push flen result
	jmp	restoreNext
	
;========================================

entry	fgetsBop
	mov	r8, [rpsp]
	mov	rdx, [rpsp + 8]
	mov	rcx, [rpsp + 16]
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.fileGetString]
	sub rsp, 32			; shadow space
	call rax
	add rsp, 32
	add	rpsp, 16
	mov	[rpsp], rax	; push fgets result
	jmp	restoreNext
	
;========================================

entry	fputsBop
	mov	rdx, [rpsp]
	mov	rcx, [rpsp + 8]
	mov	rax, [rcore + FCore.FileFuncs]
	mov	rax, [rax + FileFunc.filePutString]
	sub rsp, 32			; shadow space
	call rax
	add rsp, 32
	add	rpsp, 8
	mov	[rpsp], rax	; push fputs result
	jmp	restoreNext
	
;========================================
entry	setTraceBop
	mov	rax, [rpsp]
	add	rpsp, 8
	mov	[rcore + FCore.traceFlags], rax
	mov	[rcore + FCore.SPtr], rpsp
	mov	[rcore + FCore.IPtr], rip
	jmp interpFuncReenter

;========================================

;extern void snprintfSub( ForthCoreState* pCore );
;extern void fscanfSub( ForthCoreState* pCore );
;extern void sscanfSub( ForthCoreState* pCore );

;========================================

; fprintfOp (C++ version)
;  fprintfSub(pCore)
;   fprintfSubCore

; fprintfOp (assembler version)
;  fprintfSubCore

; extern void fprintfSub( ForthCoreState* pCore );
entry fprintfSub
    ; called from C++
    ; rcx is pCore
    push rcore
    push rpsp
    push rfp
	; stack should be 16-byte aligned at this point
    ; params refer to parameters passed to fprintf: formatStr filePtr arg1..argN
    ; arguments refer to things which are to be printed: arg1..argN
    
    mov rcore, rcx                        ; rcore -> ForthCoreState
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
	sub rsp, 32			; shadow space
    xcall fprintf
    mov [rpsp], rax
	; do stack cleanup
	mov rsp, rfp
	mov	[rcore + FCore.SPtr], rpsp
    pop rnext
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
    
    mov rcore, rcx                        ; rcore -> ForthCoreState
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
	sub rsp, 32			; shadow space
    xcall snprintf
    mov [rpsp], rax
	; do stack cleanup
	mov rsp, rfp
	mov	[rcore + FCore.SPtr], rpsp
    pop rnext
    pop rpsp
    pop rcore
	ret

;========================================

; extern int oStringFormatSub( ForthCoreState* pCore, char* pBuffer, int bufferSize );
entry oStringFormatSub
    ; called from C++
    ; rcx is pCore
    ; rdx is bufferPtr
    ; r8 is bufferSize
    
    push rcore
    push rpsp
    push rfp
	; stack should be 16-byte aligned at this point
    ; params refer to parameters passed to snprintf: bufferPtr bufferSize formatStr arg1..argN
    ; arguments refer to things which are to be printed: arg1..argN
    
    mov rcore, rcx                        ; rcore -> ForthCoreState
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
	sub rsp, 32			; shadow space
    xcall snprintf
    add rpsp, 8
	; do stack cleanup
	mov rsp, rfp
	mov	[rcore + FCore.SPtr], rpsp
    pop rnext
    pop rpsp
    pop rcore
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
    
    mov rcore, rcx                        ; rcore -> ForthCoreState
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
	sub rsp, 32			; shadow space
    xcall fscanf
    mov [rpsp], rax
	; do stack cleanup
	mov rsp, rfp
	mov	[rcore + FCore.SPtr], rpsp
    pop rnext
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
    
    mov rcore, rcx                        ; rcore -> ForthCoreState
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
	sub rsp, 32			; shadow space
    xcall sscanf
    mov [rpsp], rax
	; do stack cleanup
	mov rsp, rfp
	mov	[rcore + FCore.SPtr], rpsp
    pop rnext
    pop rpsp
    pop rcore
	ret

;========================================
entry dllEntryPointType
	; rbx is opcode:
	; bits 0..15 are index into ForthCoreState userOps table
	; 16..18 are flags
	; 19..23 are arg count
	; args are on TOS
	mov	rax, rbx
	and	rax, 0000FFFFh		; rax is userOps table index for this dll routine
	cmp	rax, rnumops
	jge	badUserDef

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
	sub rsp, 32
	xcall	CallDLLRoutine
	add rsp, 32
    mov rpsp, [rcore + FCore.SPtr]
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
	push rax
	push rnext
	mov	rnext, onzbCombo1
	and	rbx, 0FFFh
	; opcode is in rbx
	mov	rax, [rcore + FCore.innerExecute]
	jmp rax
	
onzbCombo1:
	pop	rnext
	pop	rax
	mov	rbx, [rpsp]
	add	rpsp, 8
	or	rbx, rbx
	jz	onzbCombo2			; if TOS 0, don't branch
	mov	rbx, rax
	and	rax, 02000h
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
; TBD: copy these into base of ForthCoreState, fill unused slots with badOptype
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
	DQ	localIntType
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
	DQ	localIntArrayType
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
	DQ	fieldIntType
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
	DQ	fieldIntArrayType
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
	DQ	memberIntType
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
	DQ	memberIntArrayType
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
	DQ	methodWithSuperType
	
;	123 - 149
	DQ	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	DQ	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
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
	
