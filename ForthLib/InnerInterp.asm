BITS 32

%include "core.inc"

;cextern	_filbuf
;cextern	_chkesp

EXTERN _CallDLLRoutine

;%define FCore ForthCoreState
;%define FileFunc ForthFileInterface

;	COMDAT _main
SECTION .text
;_c$ = -4

;_TEXT	SEGMENT

; register usage in a forthOp:
;
;	EAX		free
;	EBX		free
;	ECX		free
;	EDX		SP
;	ESI		IP
;	EDI		inner interp PC (constant)
;	EBP		core ptr (constant)
%define rpsp edx
%define rip esi
%define rnext edi
%define rcore ebp

; when in a opType routine:
;	AL		8-bit opType
;	EBX		full 32-bit opcode (need to mask off top 8 bits)

; remember when calling extern cdecl functions:
; 1) they are free to stomp EAX, EBX, ECX and EDX
; 2) they are free to modify their input params on stack

; if you need more than EAX, EBX and ECX in a routine, save ESI/IP & EDX/SP in FCore at start with these instructions:
;	mov	[rcore + FCore.IPtr], rip
;	mov	[rcore + FCore.SPtr], rpsp
; jump to interpFunc at end - interpFunc will restore ESI, EDX, and EDI and go back to inner loop

; OSX requires that the system stack be on a 16-byte boundary before you call any system routine
; In general in the inner interpreter code the system stack is kept at 16-byte boundary, so to
; determine how much need to offset the system stack to maintain OSX stack alignment, you count
; the number of pushes done, including both arguments to the called routine and any extra registers
; you have saved on the stack, and use this formula:
; offset = 4 * (3 - (numPushesDone mod 4))

; these are the trace flags that force the external trace output to be
; called each time an instruction is executed
traceDebugFlags EQU	kLogProfiler + kLogStack + kLogInnerInterpreter

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
GLOBAL _%1
EXTERN _%2
_%1:
%endif
	push	rpsp
	push	rip
	mov	eax, [rpsp+4]
	push	eax
	mov	eax, [rpsp]
	push	eax
%ifdef LINUX
	call	%2
%else
	call	_%2
%endif
	add	esp, 8
	pop	rip
	pop	rpsp
	fstp	QWORD[rpsp]
	jmp	rnext
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
GLOBAL _%1
EXTERN _%2
_%1:
%endif
	push	rpsp
	push	rip
	mov	eax, [rpsp]
	push	eax
%ifdef LINUX
	call	%2
%else
	call	_%2
%endif
	add	esp, 4
	pop	rip
	pop	rpsp
	fstp	DWORD[rpsp]
	jmp	rnext
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
;	ebx holds the opcode
;
entry extOpType
	; get the C routine to handle this optype from optypeAction table in FCore
	mov	eax, ebx
	shr	eax, 24							; eax is 8-bit optype
	mov	ecx, [rcore + FCore.optypeAction]
	mov	eax, [ecx + eax*4]				; eax is C routine to dispatch to
	; save current IP and SP	
	mov	[rcore + FCore.IPtr], rip
	mov	[rcore + FCore.SPtr], rpsp
	; we need to push rcore twice - the C compiler feels free to overwrite its input parameters,
	; so the top copy of EBP may be trashed on return from C land
	push	rcore		; push core ptr (our save)
	and	ebx, 00FFFFFFh
	push	ebx		; push 24-bit opVal (input to C routine)
	push	rcore		; push core ptr (input to C routine)
	call	eax
	add	esp, 8		; discard inputs to C routine 
	pop	rcore
	
	; load IP and SP from core, in case C routine modified them
	; NOTE: we can't just jump to interpFunc, since that will replace rnext & break single stepping
	mov	rip, [rcore + FCore.IPtr]
	mov	rpsp, [rcore + FCore.SPtr]
	mov	eax, [rcore + FCore.state]
	or	eax, eax
	jnz	extOpType1	; if something went wrong
	jmp	rnext			; if everything is ok
	
; NOTE: Feb. 14 '07 - doing the right thing here - restoring IP & SP and jumping to
; the interpreter loop exit point - causes an access violation exception ?why?
	;mov	rip, [rcore + FCore.IPtr]
	;mov	rpsp, [rcore + FCore.SPtr]
	;jmp	interpLoopExit	; if something went wrong
	
extOpType1:
	ret

;-----------------------------------------------
;
; InitAsmTables - initializes first part of optable, where op positions are referenced by constants
;
; extern void InitAsmTables( ForthCoreState *pCore );
entry InitAsmTables
	push rcore
	mov	rcore, esp	; 0(rcore) = old_ebp 4(rcore)=return_addr  8(rcore)=ForthCore_ptr
	push ebx
	push ecx
	push rpsp
	push rnext
	push rcore

	mov	rcore,[rcore + 8]		; rcore -> ForthCore struct
	
	; setup normal (non-debug) inner interpreter re-entry point
	mov	ebx, interpLoopDebug
	mov	[rcore + FCore.innerLoop], ebx
	mov	ebx, interpLoopExecuteEntry
	mov	[rcore + FCore.innerExecute], ebx
	

	pop rcore
	pop	rnext
	pop rpsp
	pop ecx
	pop ebx
	leave
	ret

;-----------------------------------------------
;
; single step a thread
;
; extern OpResult InterpretOneOpFast( ForthCoreState *pCore, forthop op );
entry InterpretOneOpFast
	push rcore
	mov	rcore, esp
	push ebx
	push ecx
	push rpsp
	push rip
	push rnext
	push rcore
	
	mov	ebx, [rcore + 12]
	mov	rcore, [rcore + 8]      ; rcore -> ForthCore
	mov	rip, [rcore + FCore.IPtr]
	mov	rpsp, [rcore + FCore.SPtr]
	mov	rnext, InterpretOneOpFastExit
	; TODO: should we reset state to OK before every step?
	mov	eax, kResultOk
	mov	[rcore + FCore.state], eax
	; instead of jumping directly to the inner loop, do a call so that
	; error exits which do a return instead of branching to inner loop will work
	call	interpLoopExecuteEntry
	jmp	InterpretOneOpFastExit2

InterpretOneOpFastExit:		; this is exit for state == OK - discard the unused return address from call above
	add	esp, 4
InterpretOneOpFastExit2:	; this is exit for state != OK
	mov	[rcore + FCore.IPtr], rip
	mov	[rcore + FCore.SPtr], rpsp
	mov	eax, [rcore + FCore.state]

	pop rcore
	pop	rnext
	pop	rip
	pop rpsp
	pop ecx
	pop ebx
	leave
	ret

;-----------------------------------------------
;
; inner interpreter C entry point
;
; extern OpResult InnerInterpreterFast( ForthCoreState *pCore );
entry InnerInterpreterFast
	push rcore
	mov	rcore, esp
	push ebx
	push ecx
	push rpsp
	push rip
	push rnext
	push rcore
	
	mov	rcore, [rcore + 8]      ; rcore -> ForthCore
	mov	eax, kResultOk
	mov	[rcore + FCore.state], eax

	call	interpFunc

	pop rcore
	pop	rnext
	pop	rip
	pop rpsp
	pop ecx
	pop ebx
	leave
	ret

;-----------------------------------------------
;
; inner interpreter
;	jump to interpFunc if you need to reload IP, SP, interpLoop
entry interpFunc
	mov	eax, [rcore + FCore.traceFlags]
	and	eax, traceDebugFlags
	jz .interpFunc1
	mov	ebx, traceLoopDebug
	mov	[rcore + FCore.innerLoop], ebx
	mov	ebx, traceLoopExecuteEntry
	mov	[rcore + FCore.innerExecute], ebx
	jmp	.interpFunc2
.interpFunc1:
	mov	ebx, interpLoopDebug
	mov	[rcore + FCore.innerLoop], ebx
	mov	ebx, interpLoopExecuteEntry
	mov	[rcore + FCore.innerExecute], ebx
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
entry interpLoop
	mov	ebx, [rip]		; ebx is opcode
	add	rip, 4			; advance IP
	; interpLoopExecuteEntry is entry for executeBop/methodWithThis/methodWithTos - expects opcode in ebx
interpLoopExecuteEntry:
%ifdef MACOSX
    ; check if system stack has become unaligned
    lea ecx, [rcore + FCore.scratch]
    mov eax, esp
    add eax, 4
    and eax, 15
    je goodStack
    mov eax,[ecx]       ; put a breakpoint here to catch when system stack misaligns
goodStack:
    ; save the last 4 opcodes dispatched to help track down system stack align problems
    mov eax, [ecx + 8]
    mov [ecx + 12], eax
    mov eax, [ecx + 4]
    mov [ecx + 8], eax
    mov eax, [ecx]
    mov [ecx + 4], eax
    mov [ecx], ebx
badStack:
%endif
	cmp	ebx, [rcore + FCore.numOps]
	jae	notNative
	mov eax, [rcore + FCore.ops]
	mov	ecx, [eax+ebx*4]
	jmp	ecx

entry traceLoopDebug
	; while debugging, store IP,SP in corestate shadow copies after every instruction
	;   so crash stacktrace will be more accurate (off by only one instruction)
	mov	[rcore + FCore.IPtr], rip
	mov	[rcore + FCore.SPtr], rpsp
	mov	ebx, [rip]		; ebx is opcode
	mov	eax, rip		; eax is the IP for trace
	jmp	traceLoopDebug2

	; traceLoopExecuteEntry is entry for executeBop/methodWithThis/methodWithTos - expects opcode in ebx
traceLoopExecuteEntry:
	xor	eax, eax		; put null in trace IP for indirect execution (op isn't at IP)
	sub	rip, 4			; actual IP was already advanced by execute/method op, don't double advance it
traceLoopDebug2:
	push rpsp
    sub esp, 12         ; 16-byte align for OSX
    push ebx            ; opcode
    push eax            ; IP
    push rcore            ; core
	xcall traceOp
	pop rcore
	pop eax
	pop ebx
    add esp, 12
	pop rpsp
	add	rip, 4			; advance IP
	jmp interpLoopExecuteEntry

interpLoopExit:
	mov	[rcore + FCore.state], eax
	mov	[rcore + FCore.IPtr], rip
	mov	[rcore + FCore.SPtr], rpsp
	ret

badOptype:
	mov	eax, kForthErrorBadOpcodeType
	jmp	interpLoopErrorExit

badVarOperation:
	mov	eax, kForthErrorBadVarOperation
	jmp	interpLoopErrorExit
	
badOpcode:
	mov	eax, kForthErrorBadOpcode

	
interpLoopErrorExit:
	; error exit point
	; eax is error code
	mov	[rcore + FCore.error], eax
	mov	eax, kResultError
	jmp	interpLoopExit
	
interpLoopFatalErrorExit:
	; fatal error exit point
	; eax is error code
	mov	[rcore + FCore.error], eax
	mov	eax, kResultFatalError
	jmp	interpLoopExit
	
; op (in ebx) is not defined in assembler, dispatch through optype table
notNative:
	mov	eax, ebx			; leave full opcode in ebx
	shr	eax, 24			; eax is 8-bit optype
	mov	eax, [opTypesTable + eax*4]
	jmp	eax

nativeImmediate:
	and	ebx, 00FFFFFFh
	cmp	ebx, [rcore + FCore.numOps]
	jae	badOpcode
	mov eax, [rcore + FCore.ops]
	mov	ecx, [eax+ebx*4]
	jmp	ecx

; externalBuiltin is invoked when a builtin op which is outside of range of table is invoked
externalBuiltin:
	; it should be impossible to get here now
	jmp	badOpcode
	
;-----------------------------------------------
;
; combos which push an immediate literal, then execute a builtin op defined in assembler
;
entry native32Type
    mov	eax, [rip]
    add rip, 4
    sub rpsp, 4
    mov [rpsp], eax
	and	ebx, 00FFFFFFh
	cmp	ebx, [rcore + FCore.numOps]
	jae	badOpcode
	mov eax, [rcore + FCore.ops]
	mov	ecx, [eax+ebx*4]
	jmp	ecx

entry native64Type
    mov	eax, [rip]
    sub rpsp, 8
    mov [rpsp], eax
    mov	eax, [rip + 4]
    add rip, 8
    mov [rpsp + 4], eax
	and	ebx, 00FFFFFFh
	cmp	ebx, [rcore + FCore.numOps]
	jae	badOpcode
	mov eax, [rcore + FCore.ops]
	mov	ecx, [eax+ebx*4]
	jmp	ecx

;-----------------------------------------------
;
; cCodeType is used by "builtin" ops which are only defined in C++
;
;	ebx holds the opcode
;
entry cCodeType
	and	ebx, 00FFFFFFh
	; dispatch to C version if valid
	cmp	ebx, [rcore + FCore.numOps]
	jae	badOpcode
	; ebx holds the opcode which was just dispatched, use its low 24-bits as index into builtinOps table of ops defined in C/C++
	mov	eax, ebx
	mov	ebx, [rcore + FCore.ops]
	mov	eax, [ebx + eax*4]				; eax is C routine to dispatch to
	; save current IP and SP	
	mov	[rcore + FCore.IPtr], rip
	mov	[rcore + FCore.SPtr], rpsp
	; we need to push rcore twice - the C compiler feels free to overwrite its input parameters,
	; so the top copy of EBP may be trashed on return from C land
%ifdef MACOSX
    sub esp, 4      ; 16-byte align for OSX
%endif
	push	rcore		; push core ptr (our save)
	push	rcore		; push core ptr (input to C routine)
	call	eax
	add	esp, 4		; discard inputs to C routine
	pop	rcore
%ifdef MACOSX
    add esp, 4
%endif
	; load IP and SP from core, in case C routine modified them
	; NOTE: we can't just jump to interpFunc, since that will replace rnext & break single stepping
	mov	rip, [rcore + FCore.IPtr]
	mov	rpsp, [rcore + FCore.SPtr]
	mov	eax, [rcore + FCore.state]
	or	eax, eax
	jnz	interpLoopExit		; if something went wrong (should this be interpLoopErrorExit?)
	jmp	rnext					; if everything is ok
	
; NOTE: Feb. 14 '07 - doing the right thing here - restoring IP & SP and jumping to
; the interpreter loop exit point - causes an access violation exception ?why?
	;mov	rip, [rcore + FCore.IPtr]
	;mov	rpsp, [rcore + FCore.SPtr]
	;jmp	interpLoopExit	; if something went wrong
	

;-----------------------------------------------
;
; combos which push an immediate literal, then execute a builtin op defined in C++
;
entry cCode32Type
    mov	eax, [rip]
    add rip, 4
    sub rpsp, 4
    mov [rpsp], eax
    jmp cCodeType

entry cCode64Type
    mov	eax, [rip]
    sub rpsp, 8
    mov [rpsp], eax
    mov	eax, [rip + 4]
    add rip, 8
    mov [rpsp + 4], eax
    jmp cCodeType

;-----------------------------------------------
;
; user-defined ops (forth words defined with colon)
;
entry userDefType
	; get low-24 bits of opcode & check validity
	and	ebx, 00FFFFFFh
	cmp	ebx, [rcore + FCore.numOps]
	jge	badUserDef
	; push IP on rstack
	mov	eax, [rcore + FCore.RPtr]
	sub	eax, 4
	mov	[eax], rip
	mov	[rcore + FCore.RPtr], eax
	; get new IP
	mov	eax, [rcore + FCore.ops]
	mov	rip, [eax+ebx*4]
	jmp	rnext

badUserDef:
	mov	eax, kForthErrorBadOpcode
	jmp	interpLoopErrorExit

;-----------------------------------------------
;
; combos which push an immediate literal, then execute a builtin op defined in C++
;
entry userDef32Type
    mov	eax, [rip]
    add rip, 4
    sub rpsp, 4
    mov [rpsp], eax
    jmp userDefType

entry userDef64Type
    mov	eax, [rip]
    sub rpsp, 8
    mov [rpsp], eax
    mov	eax, [rip + 4]
    add rip, 8
    mov [rpsp + 4], eax
    jmp userDefType

;-----------------------------------------------
;
; user-defined ops (forth words defined with colon)
;
entry relativeDefType
	; ebx is offset from dictionary base of user definition
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	mov	eax, [rcore + FCore.DictionaryPtr]
	add	ebx, [eax + ForthMemorySection.pBase]
	cmp	ebx, [eax + ForthMemorySection.pCurrent]
	jge	badUserDef
	; push IP on rstack
	mov	eax, [rcore + FCore.RPtr]
	sub	eax, 4
	mov	[eax], rip
	mov	[rcore + FCore.RPtr], eax
	mov	rip, ebx
	jmp	rnext

;-----------------------------------------------
;
; unconditional branch ops
;
entry branchType
	; get low-24 bits of opcode
	mov	eax, ebx
	and	eax, 00800000h
	jnz	branchBack
	; branch forward
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	add	rip, ebx
	jmp	rnext

branchBack:
	or	ebx,0FF000000h
	sal	ebx, 2
	add	rip, ebx
	jmp	rnext

;-----------------------------------------------
;
; branch-on-zero ops
;
entry branchZType
	mov	eax, [rpsp]
	add	rpsp, 4
	or	eax, eax
	jz	branchType		; branch taken
	jmp	rnext	; branch not taken

;-----------------------------------------------
;
; branch-on-notzero ops
;
entry branchNZType
	mov	eax, [rpsp]
	add	rpsp, 4
	or	eax, eax
	jnz	branchType		; branch taken
	jmp	rnext	; branch not taken

;-----------------------------------------------
;
; case branch ops
;
entry caseBranchTType
    ; TOS: this_case_value case_selector
	mov	eax, [rpsp]		; eax is this_case_value
	add	rpsp, 4
	cmp	eax, [rpsp]
	jnz	caseMismatch
	; case did match - branch to case body
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	add	rip, ebx
	add	rpsp, 4
caseMismatch:
	jmp	rnext

entry caseBranchFType
    ; TOS: this_case_value case_selector
	mov	eax, [rpsp]		; eax is this_case_value
	add	rpsp, 4
	cmp	eax, [rpsp]
	jz	caseMatched
	; case didn't match - branch to next case test
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	add	rip, ebx
	jmp	rnext

caseMatched:
	add	rpsp, 4
	jmp	rnext
	
;-----------------------------------------------
;
; branch around block ops
;
entry pushBranchType
	sub	rpsp, 4			; push IP (pointer to block)
	mov	[rpsp], rip
	and	ebx, 00FFFFFFh	; branch around block
	sal	ebx, 2
	add	rip, ebx
	jmp	rnext

;-----------------------------------------------
;
; relative data block ops
;
entry relativeDataType
	; ebx is offset from dictionary base of user definition
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	mov	eax, [rcore + FCore.DictionaryPtr]
	add	ebx, [eax + ForthMemorySection.pBase]
	cmp	ebx, [eax + ForthMemorySection.pCurrent]
	jge	badUserDef
	; push address of data on pstack
	sub	rpsp, 4
	mov	[rpsp], ebx
	jmp	rnext

;-----------------------------------------------
;
; relative data ops
;
entry relativeDefBranchType
	; push relativeDef opcode for immediately following anonymous definition (IP in rip points to it)
	; compute offset from dictionary base to anonymous def
	mov	eax, [rcore + FCore.DictionaryPtr]
	mov	ecx, [eax + ForthMemorySection.pBase]
	mov	eax, rip
	sub	eax, ecx
	sar	eax, 2
	; stick the optype in top 8 bits
	mov	ecx, kOpRelativeDef << 24
	or	eax, ecx
	sub	rpsp, 4
	mov	[rpsp], eax
	; advance IP past anonymous definition
	and	ebx, 00FFFFFFh	; branch around block
	sal	ebx, 2
	add	rip, ebx
	jmp	rnext


;-----------------------------------------------
;
; 24-bit constant ops
;
entry constantType
	; get low-24 bits of opcode
	mov	eax, ebx
	sub	rpsp, 4
	and	eax,00800000h
	jnz	constantNegative
	; positive constant
	and	ebx,00FFFFFFh
	mov	[rpsp], ebx
	jmp	rnext

constantNegative:
	or	ebx, 0FF000000h
	mov	[rpsp], ebx
	jmp	rnext

;-----------------------------------------------
;
; 24-bit offset ops
;
entry offsetType
	; get low-24 bits of opcode
	mov	eax, ebx
	and	eax, 00800000h
	jnz	offsetNegative
	; positive constant
	and	ebx, 00FFFFFFh
	add	[rpsp], ebx
	jmp	rnext

offsetNegative:
	or	ebx, 0FF000000h
	add	[rpsp], ebx
	jmp	rnext

;-----------------------------------------------
;
; 24-bit offset fetch ops
;
entry offsetFetchType
	; get low-24 bits of opcode
	mov	eax, ebx
	and	eax, 00800000h
	jnz	offsetFetchNegative
	; positive constant
	and	ebx, 00FFFFFFh
	add	ebx, [rpsp]
	mov	eax, [ebx]
	mov	[rpsp], eax
	jmp	rnext

offsetFetchNegative:
	or	ebx, 0FF000000h
	add	ebx, [rpsp]
	mov	eax, [ebx]
	mov	[rpsp], eax
	jmp	rnext

;-----------------------------------------------
;
; array offset ops
;
entry arrayOffsetType
	; get low-24 bits of opcode
	and	ebx, 00FFFFFFh		; ebx is size of one element
	; TOS is array base, tos-1 is index
	imul	ebx, [rpsp+4]	; multiply index by element size
	add	ebx, [rpsp]			; add array base addr
	add	rpsp, 4
	mov	[rpsp], ebx
	jmp	rnext

;-----------------------------------------------
;
; local struct array ops
;
entry localStructArrayType
   ; bits 0..11 are padded struct length in bytes, bits 12..23 are frame offset in longs
   ; multiply struct length by TOS, add in (negative) frame offset, and put result on TOS
	mov	eax, 00000FFFh
	and	eax, ebx                ; eax is padded struct length in bytes
	imul	eax, [rpsp]              ; multiply index * length
	add	eax, [rcore + FCore.FPtr]
	and	ebx, 00FFF000h
	sar	ebx, 10							; ebx = frame offset in bytes of struct[0]
	sub	eax, ebx						; eax -> struct[N]
	mov	[rpsp], eax
	jmp	rnext

;-----------------------------------------------
;
; string constant ops
;
entry constantStringType
	; IP points to beginning of string
	; low 24-bits of ebx is string len in longs
	sub	rpsp, 4
	mov	[rpsp], rip		; push string ptr
	; get low-24 bits of opcode
	and	ebx, 00FFFFFFh
	shl	ebx, 2
	; advance IP past string
	add	rip, ebx
	jmp	rnext

;-----------------------------------------------
;
; local stack frame allocation ops
;
entry allocLocalsType
	; rpush old FP
	mov	ecx, [rcore + FCore.FPtr]
	mov	eax, [rcore + FCore.RPtr]
	sub	eax, 4
	mov	[eax], ecx
	; set FP = RP, points at old FP
	mov	[rcore + FCore.FPtr], eax
	; allocate amount of storage specified by low 24-bits of op on rstack
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	mov	[rcore + FCore.RPtr], eax
	; clear out allocated storage
	mov ecx, eax
	xor eax, eax
alt1:
	mov [ecx], eax
	add ecx, 4
	sub ebx, 4
	jnz alt1
	jmp	rnext

;-----------------------------------------------
;
; local string init ops
;
entry initLocalStringType
   ; bits 0..11 are string length in bytes, bits 12..23 are frame offset in longs
   ; init the current & max length fields of a local string
	mov	eax, 00FFF000h
	and	eax, ebx
	sar	eax, 10							; eax = frame offset in bytes
	mov	ecx, [rcore + FCore.FPtr]
	sub	ecx, eax						; ecx -> max length field
	and	ebx, 00000FFFh					; ebx = max length
	mov	[ecx], ebx						; set max length
	xor	eax, eax
	mov	[ecx+4], eax					; set current length to 0
	mov	[ecx+5], al						; add terminating null
	jmp	rnext

;-----------------------------------------------
;
; local reference ops
;
entry localRefType
	; push local reference - ebx is frame offset in longs
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	sub	rpsp, 4
	mov	[rpsp], eax
	jmp	rnext
	
;-----------------------------------------------
;
; member reference ops
;
entry memberRefType
	; push member reference - ebx is member offset in bytes
	mov	eax, [rcore + FCore.TPtr]
	and	ebx, 00FFFFFFh
	add	eax, ebx
	sub	rpsp, 4
	mov	[rpsp], eax
	jmp	rnext
	
;-----------------------------------------------
;
; byte ops
;
entry localByteType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz localByteType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
localByteType1:
	; get ptr to byte var into eax
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, VAROP_LOMASK
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
byteEntry:
	mov	ebx, [rcore + FCore.varMode]
	or	ebx, ebx
	jnz	localByte1
	; fetch local byte
localByteFetch:
	sub	rpsp, 4
	movsx	ebx, BYTE[eax]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localByte1:
	cmp	ebx, kVarGetDec
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, [localByteActionTable + ebx*4]
	jmp	ebx

localByteActionTable:
	DD	localByteFetch
	DD	localByteFetch
	DD	localRef
	DD	localByteStore
	DD	localBytePlusStore
	DD	localByteMinusStore
    DD  localByteClear
    DD  localBytePlus
    DD  localByteInc
    DD  localByteMinus
    DD  localByteDec
    DD  localByteIncGet
    DD  localByteDecGet
    DD  localByteGetInc
    DD  localByteGetDec

entry localUByteType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz localUByteType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
localUByteType1:
	; get ptr to byte var into eax
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, VAROP_LOMASK
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
ubyteEntry:
	mov	ebx, [rcore + FCore.varMode]
	or	ebx, ebx
	jnz	localUByte1
	; fetch local unsigned byte
localUByteFetch:
	sub	rpsp, 4
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx	; set var operation back to default
	mov	bl, [eax]
	mov	[rpsp], ebx
	jmp	rnext

localUByte1:
	cmp	ebx, kVarGetDec
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, [localUByteActionTable + ebx*4]
	jmp	ebx

localUByteActionTable:
	DD	localUByteFetch
	DD	localUByteFetch
	DD	localRef
	DD	localByteStore
	DD	localBytePlusStore
	DD	localByteMinusStore
    DD  localByteClear
    DD  localUBytePlus
    DD  localByteInc
    DD  localUByteMinus
    DD  localByteDec
    DD  localUByteIncGet
    DD  localUByteDecGet
    DD  localUByteGetInc
    DD  localUByteGetDec

localRef:
	sub	rpsp, 4
	mov	[rpsp], eax
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext
	
localByteStore:
	mov	ebx, [rpsp]
	mov	[eax], bl
	add	rpsp, 4
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localBytePlusStore:
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx	; set var operation back to default
	mov	bl, [eax]
	add	ebx, [rpsp]
	mov	[eax], bl
	add	rpsp, 4
	jmp	rnext

localByteMinusStore:
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx	; set var operation back to default
	mov	bl, [eax]
	sub	ebx, [rpsp]
	mov	[eax], bl
	add	rpsp, 4
	jmp	rnext

localByteClear:
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx	; set var operation back to default
	mov	[eax], bl
	jmp	rnext

localBytePlus:
	movsx	ebx, BYTE[eax]
	add	ebx, [rpsp]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localByteInc:
    inc BYTE[eax]
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localByteMinus:
	movsx	ebx, BYTE[eax]
    mov eax, [rpsp]
	sub	eax, ebx
	mov	[rpsp], eax
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localByteDec:
    dec BYTE[eax]
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localByteIncGet:
    inc BYTE[eax]
	sub	rpsp, 4
	movsx	ebx, BYTE[eax]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localByteDecGet:
    dec BYTE[eax]
	sub	rpsp, 4
	movsx	ebx, BYTE[eax]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localByteGetInc:
	sub	rpsp, 4
	movsx	ebx, BYTE[eax]
    inc BYTE[eax]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localByteGetDec:
	sub	rpsp, 4
	movsx	ebx, BYTE[eax]
    dec BYTE[eax]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localUBytePlus:
    xor ebx, ebx
	mov	[rcore + FCore.varMode], ebx	; set var operation back to default
    mov bl, BYTE[eax]
	add	ebx, [rpsp]
	mov	[rpsp], ebx
	jmp	rnext

localUByteMinus:
    xor ebx, ebx
	mov	[rcore + FCore.varMode], ebx	; set var operation back to default
    mov bl, BYTE[eax]
    mov eax, [rpsp]
	sub	eax, ebx
	mov	[rpsp], eax
	jmp	rnext

localUByteIncGet:
    inc BYTE[eax]
    xor ebx, ebx
	mov	[rcore + FCore.varMode], ebx	; set var operation back to default
    mov bl, BYTE[eax]
	sub	rpsp, 4
	mov	[rpsp], ebx
	jmp	rnext

localUByteDecGet:
    dec BYTE[eax]
    xor ebx, ebx
	mov	[rcore + FCore.varMode], ebx	; set var operation back to default
    mov bl, BYTE[eax]
	sub	rpsp, 4
	mov	[rpsp], ebx
	jmp	rnext

localUByteGetInc:
	sub	rpsp, 4
    xor ebx, ebx
	mov	[rcore + FCore.varMode], ebx	; set var operation back to default
    mov bl, BYTE[eax]
    inc BYTE[eax]
	mov	[rpsp], ebx
	jmp	rnext

localUByteGetDec:
	sub	rpsp, 4
    xor ebx, ebx
	mov	[rcore + FCore.varMode], ebx	; set var operation back to default
    mov bl, BYTE[eax]
    dec BYTE[eax]
	mov	[rpsp], ebx
	jmp	rnext

entry fieldByteType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz fieldByteType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
fieldByteType1:
	; get ptr to byte var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [rpsp]
	add	rpsp, 4
	and	ebx, VAROP_LOMASK
	add	eax, ebx
	jmp	byteEntry

entry fieldUByteType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz fieldUByteType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
fieldUByteType1:
	; get ptr to byte var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [rpsp]
	add	rpsp, 4
	and	ebx, VAROP_LOMASK
	add	eax, ebx
	jmp	ubyteEntry

entry memberByteType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz memberByteType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
memberByteType1:
	; get ptr to byte var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [rcore + FCore.TPtr]
	and	ebx, VAROP_LOMASK
	add	eax, ebx
	jmp	byteEntry

entry memberUByteType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz memberUByteType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
memberUByteType1:
	; get ptr to byte var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [rcore + FCore.TPtr]
	and	ebx, VAROP_LOMASK
	add	eax, ebx
	jmp	ubyteEntry

entry localByteArrayType
	; get ptr to byte var into eax
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	add	eax, [rpsp]		; add in array index on TOS
	add	rpsp, 4
	jmp	byteEntry

entry localUByteArrayType
	; get ptr to byte var into eax
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	add	eax, [rpsp]		; add in array index on TOS
	add	rpsp, 4
	jmp	ubyteEntry

entry fieldByteArrayType
	; get ptr to byte var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [rpsp]
	add	eax, [rpsp+4]
	add	rpsp, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	byteEntry

entry fieldUByteArrayType
	; get ptr to byte var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [rpsp]
	add	eax, [rpsp+4]
	add	rpsp, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	ubyteEntry

entry memberByteArrayType
	; get ptr to byte var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [rcore + FCore.TPtr]
	add	eax, [rpsp]
	add	rpsp, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	byteEntry

entry memberUByteArrayType
	; get ptr to byte var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [rcore + FCore.TPtr]
	add	eax, [rpsp]
	add	rpsp, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	ubyteEntry

;-----------------------------------------------
;
;  short ops
;
entry localShortType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz localShortType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
localShortType1:
	; get ptr to short var into eax
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, VAROP_LOMASK
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
shortEntry:
	mov	ebx, [rcore + FCore.varMode]
	or	ebx, ebx
	jnz	localShort1
	; fetch local short
localShortFetch:
	sub	rpsp, 4
	movsx	ebx, WORD[eax]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localShort1:
	cmp	ebx, kVarGetDec
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, [localShortActionTable + ebx*4]
	jmp	ebx

localShortActionTable:
	DD	localShortFetch
	DD	localShortFetch
	DD	localRef
	DD	localShortStore
	DD	localShortPlusStore
	DD	localShortMinusStore
    DD  localShortClear
    DD  localShortPlus
    DD  localShortInc
    DD  localShortMinus
    DD  localShortDec
    DD  localShortIncGet
    DD  localShortDecGet
    DD  localShortGetInc
    DD  localShortGetDec

entry localUShortType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz localUShortType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
localUShortType1:
	; get ptr to short var into eax
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, VAROP_LOMASK
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
ushortEntry:
	mov	ebx, [rcore + FCore.varMode]
	or	ebx, ebx
	jnz	localUShort1
	; fetch local unsigned short
localUShortFetch:
	sub	rpsp, 4
	movsx	ebx, WORD[eax]
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx	; set var operation back to default
	mov	bx, [eax]
	mov	[rpsp], ebx
	jmp	rnext

localUShort1:
	cmp	ebx, kVarGetDec
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, [localShortActionTable + ebx*4]
	jmp	ebx

localUShortActionTable:
	DD	localUShortFetch
	DD	localUShortFetch
	DD	localRef
	DD	localShortStore
	DD	localShortPlusStore
	DD	localShortMinusStore
    DD  localShortClear
    DD  localUShortPlus
    DD  localShortInc
    DD  localUShortMinus
    DD  localShortDec
    DD  localUShortIncGet
    DD  localUShortDecGet
    DD  localUShortGetInc
    DD  localUShortGetDec

localShortStore:
	mov	ebx, [rpsp]
	mov	[eax], bx
	add	rpsp, 4
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localShortPlusStore:
	movsx	ebx, WORD[eax]
	add	ebx, [rpsp]
	mov	[eax], bx
	add	rpsp, 4
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localShortMinusStore:
	movsx	ebx, WORD[eax]
	sub	ebx, [rpsp]
	mov	[eax], bx
	add	rpsp, 4
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localShortClear:
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx	; set var operation back to default
	mov	[eax], bx
	jmp	rnext

localShortPlus:
	movsx	ebx, WORD[eax]
	add	ebx, [rpsp]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localShortInc:
    inc WORD[eax]
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localShortMinus:
	movsx	ebx, WORD[eax]
    mov eax, [rpsp]
	sub	eax, ebx
	mov	[rpsp], eax
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localShortDec:
    dec WORD[eax]
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localShortIncGet:
    inc WORD[eax]
	sub	rpsp, 4
	movsx	ebx, WORD[eax]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localShortDecGet:
    dec WORD[eax]
	sub	rpsp, 4
	movsx	ebx, WORD[eax]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localShortGetInc:
	sub	rpsp, 4
	movsx	ebx, WORD[eax]
    inc WORD[eax]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localShortGetDec:
	sub	rpsp, 4
	movsx	ebx, WORD[eax]
    dec WORD[eax]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localUShortPlus:
    xor ebx, ebx
	mov	[rcore + FCore.varMode], ebx	; set var operation back to default
    mov bx, WORD[eax]
	add	ebx, [rpsp]
	mov	[rpsp], ebx
	jmp	rnext

localUShortMinus:
    xor ebx, ebx
	mov	[rcore + FCore.varMode], ebx	; set var operation back to default
    mov bx, WORD[eax]
    mov eax, [rpsp]
	sub	eax, ebx
	mov	[rpsp], eax
	jmp	rnext

localUShortIncGet:
    inc WORD[eax]
    xor ebx, ebx
	mov	[rcore + FCore.varMode], ebx	; set var operation back to default
    mov bx, WORD[eax]
	sub	rpsp, 4
	mov	[rpsp], ebx
	jmp	rnext

localUShortDecGet:
    dec WORD[eax]
    xor ebx, ebx
	mov	[rcore + FCore.varMode], ebx	; set var operation back to default
    mov bx, WORD[eax]
	sub	rpsp, 4
	mov	[rpsp], ebx
	jmp	rnext

localUShortGetInc:
	sub	rpsp, 4
    xor ebx, ebx
	mov	[rcore + FCore.varMode], ebx	; set var operation back to default
    mov bx, WORD[eax]
    inc WORD[eax]
	mov	[rpsp], ebx
	jmp	rnext

localUShortGetDec:
	sub	rpsp, 4
    xor ebx, ebx
	mov	[rcore + FCore.varMode], ebx	; set var operation back to default
    mov bx, WORD[eax]
    dec WORD[eax]
	mov	[rpsp], ebx
	jmp	rnext

entry fieldShortType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz fieldShortType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
fieldShortType1:
	; get ptr to short var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [rpsp]
	add	rpsp, 4
	and	ebx, VAROP_LOMASK
	add	eax, ebx
	jmp	shortEntry

entry fieldUShortType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz fieldUShortType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
fieldUShortType1:
	; get ptr to unsigned short var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [rpsp]
	add	rpsp, 4
	and	ebx, VAROP_LOMASK
	add	eax, ebx
	jmp	ushortEntry

entry memberShortType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz memberShortType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
memberShortType1:
	; get ptr to short var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [rcore + FCore.TPtr]
	and	ebx, VAROP_LOMASK
	add	eax, ebx
	jmp	shortEntry

entry memberUShortType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz memberUShortType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
memberUShortType1:
	; get ptr to unsigned short var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [rcore + FCore.TPtr]
	and	ebx, VAROP_LOMASK
	add	eax, ebx
	jmp	ushortEntry

entry localShortArrayType
	; get ptr to int var into eax
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, 00FFFFFFh	; ebx is frame offset in longs
	sal	ebx, 2
	sub	eax, ebx
	mov	ebx, [rpsp]		; add in array index on TOS
	add	rpsp, 4
	sal	ebx, 1
	add	eax, ebx
	jmp	shortEntry

entry localUShortArrayType
	; get ptr to int var into eax
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, 00FFFFFFh	; ebx is frame offset in longs
	sal	ebx, 2
	sub	eax, ebx
	mov	ebx, [rpsp]		; add in array index on TOS
	add	rpsp, 4
	sal	ebx, 1
	add	eax, ebx
	jmp	ushortEntry

entry fieldShortArrayType
	; get ptr to short var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [rpsp+4]	; eax = index
	sal	eax, 1
	add	eax, [rpsp]		; add in struct base ptr
	add	rpsp, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	shortEntry

entry fieldUShortArrayType
	; get ptr to short var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [rpsp+4]	; eax = index
	sal	eax, 1
	add	eax, [rpsp]		; add in struct base ptr
	add	rpsp, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	ushortEntry

entry memberShortArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [rpsp]	; eax = index
	sal	eax, 1
	add	eax, [rcore + FCore.TPtr]
	add	rpsp, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	shortEntry

entry memberUShortArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [rpsp]	; eax = index
	sal	eax, 1
	add	eax, [rcore + FCore.TPtr]
	add	rpsp, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	ushortEntry


;-----------------------------------------------
;
;  int ops
;
entry localIntType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz localIntType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
localIntType1:
	; get ptr to int var into eax
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, VAROP_LOMASK
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
intEntry:
	mov	ebx, [rcore + FCore.varMode]
	or	ebx, ebx
	jnz	localInt1
	; fetch local int
localIntFetch:
	sub	rpsp, 4
	mov	ebx, [eax]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localInt1:
	cmp	ebx, kVarGetDec
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, [localIntActionTable + ebx*4]
	jmp	ebx

localIntActionTable:
	DD	localIntFetch
	DD	localIntFetch
	DD	localRef
	DD	localIntStore
	DD	localIntPlusStore
	DD	localIntMinusStore
    DD  localIntClear
    DD  localIntPlus
    DD  localIntInc
    DD  localIntMinus
    DD  localIntDec
    DD  localIntIncGet
    DD  localIntDecGet
    DD  localIntGetInc
    DD  localIntGetDec

localIntStore:
	mov	ebx, [rpsp]
	mov	[eax], ebx
	add	rpsp, 4
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localIntPlusStore:
	mov	ebx, [eax]
	add	ebx, [rpsp]
	mov	[eax], ebx
	add	rpsp, 4
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localIntMinusStore:
	mov	ebx, [eax]
	sub	ebx, [rpsp]
	mov	[eax], ebx
	add	rpsp, 4
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localIntClear:
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx	; set var operation back to default
	mov	[eax], ebx
	jmp	rnext

localIntPlus:
	mov	ebx, [eax]
	add	ebx, [rpsp]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localIntInc:
    inc DWORD[eax]
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localIntMinus:
	mov	ebx, [eax]
    mov eax, [rpsp]
	sub	eax, ebx
	mov	[rpsp], eax
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localIntDec:
    dec DWORD[eax]
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localIntIncGet:
    inc DWORD[eax]
	sub	rpsp, 4
	mov	ebx, [eax]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localIntDecGet:
    dec DWORD[eax]
	sub	rpsp, 4
	mov	ebx, [eax]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localIntGetInc:
	sub	rpsp, 4
	mov	ebx, [eax]
    inc DWORD[eax]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localIntGetDec:
	sub	rpsp, 4
	mov	ebx, [eax]
    dec DWORD[eax]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

entry fieldIntType
	; get ptr to int var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz fieldIntType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
fieldIntType1:
	mov	eax, [rpsp]
	add	rpsp, 4
	and	ebx, VAROP_LOMASK
	add	eax, ebx
	jmp	intEntry

entry memberIntType
	; get ptr to int var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz memberIntType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
memberIntType1:
	mov	eax, [rcore + FCore.TPtr]
	and	ebx, VAROP_LOMASK
	add	eax, ebx
	jmp	intEntry

entry localIntArrayType
	; get ptr to int var into eax
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, 00FFFFFFh
	sub	ebx, [rpsp]		; add in array index on TOS
	add	rpsp, 4
	sal	ebx, 2
	sub	eax, ebx
	jmp	intEntry

entry fieldIntArrayType
	; get ptr to int var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [rpsp+4]	; eax = index
	sal	eax, 2
	add	eax, [rpsp]		; add in struct base ptr
	add	rpsp, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	intEntry

entry memberIntArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [rpsp]	; eax = index
	sal	eax, 2
	add	eax, [rcore + FCore.TPtr]
	add	rpsp, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	intEntry

;-----------------------------------------------
;
;  float ops
;
entry localFloatType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz localFloatType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
localFloatType1:
	; get ptr to float var into eax
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, VAROP_LOMASK
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
floatEntry:
	mov	ebx, [rcore + FCore.varMode]
	or	ebx, ebx
	jnz	localFloat1
	; fetch local float
localFloatFetch:
	sub	rpsp, 4
	mov	ebx, [eax]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localFloatRef:
	sub	rpsp, 4
	mov	[rpsp], eax
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext
	
localFloatStore:
	mov	ebx, [rpsp]
	mov	[eax], ebx
	add	rpsp, 4
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localFloatPlusStore:
	fld	DWORD[eax]
	; set var operation back to default
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx
	fadd	DWORD[rpsp]
	add	rpsp, 4
	fstp	DWORD[eax]
	jmp	rnext

localFloatMinusStore:
	fld	DWORD[eax]
	; set var operation back to default
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx
	fsub	DWORD[rpsp]
	add	rpsp, 4
	fstp	DWORD[eax]
	jmp	rnext

localFloatPlus:
	movss xmm0, DWORD[eax]
    addss xmm0, DWORD[rpsp]
    movss DWORD[rpsp], xmm0
	; set var operation back to default
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx
	jmp	rnext

localFloatMinus:
	movss xmm0, DWORD[rpsp]
    subss xmm0, DWORD[eax]
    movss DWORD[rpsp], xmm0
	; set var operation back to default
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx
	jmp	rnext

localFloatActionTable:
	DD	localFloatFetch
	DD	localFloatFetch
	DD	localFloatRef
	DD	localFloatStore
	DD	localFloatPlusStore
	DD	localFloatMinusStore
    DD  localIntClear
    DD  localFloatPlus
    DD  badVarOperation      ; this is varop Inc, which doesn't exist for float and double
    DD  localFloatMinus

localFloat1:
	cmp	ebx, kVarGetDec
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, [localFloatActionTable + ebx*4]
	jmp	ebx

entry fieldFloatType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz fieldFloatType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
fieldFloatType1:
	; get ptr to float var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [rpsp]
	add	rpsp, 4
	and	ebx, VAROP_LOMASK
	add	eax, ebx
	jmp	floatEntry

entry memberFloatType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz memberFloatType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
memberFloatType1:
	; get ptr to float var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [rcore + FCore.TPtr]
	and	ebx, VAROP_LOMASK
	add	eax, ebx
	jmp	floatEntry

entry localFloatArrayType
	; get ptr to float var into eax
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, 00FFFFFFh
	sub	ebx, [rpsp]		; add in array index on TOS
	add	rpsp, 4
	sal	ebx, 2
	sub	eax, ebx
	jmp	floatEntry

entry fieldFloatArrayType
	; get ptr to float var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [rpsp+4]	; eax = index
	sal	eax, 2
	add	eax, [rpsp]		; add in struct base ptr
	add	rpsp, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	floatEntry

entry memberFloatArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [rpsp]	; eax = index
	sal	eax, 2
	add	eax, [rcore + FCore.TPtr]
	add	rpsp, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	floatEntry
	
;-----------------------------------------------
;
;  double ops
;
entry localDoubleType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz localDoubleType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
localDoubleType1:
	; get ptr to double var into eax
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, VAROP_LOMASK
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
doubleEntry:
	mov	ebx, [rcore + FCore.varMode]
	or	ebx, ebx
	jnz	localDouble1
	; fetch local double
localDoubleFetch:
	sub	rpsp, 8
	mov	ebx, [eax]
	mov	[rpsp], ebx
	mov	ebx, [eax+4]
	mov	[rpsp+4], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localDoubleRef:
	sub	rpsp, 4
	mov	[rpsp], eax
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext
	
localDoubleStore:
	mov	ebx, [rpsp]
	mov	[eax], ebx
	mov	ebx, [rpsp+4]
	mov	[eax+4], ebx
	add	rpsp, 8
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localDoublePlusStore:
	fld	QWORD[eax]
	; set var operation back to default
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx
	fadd	QWORD[rpsp]
	add	rpsp, 8
	fstp	QWORD[eax]
	jmp	rnext

localDoubleMinusStore:
	fld	QWORD[eax]
	; set var operation back to default
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx
	fsub	QWORD[rpsp]
	add	rpsp, 8
	fstp	QWORD[eax]
	jmp	rnext

localDoublePlus:
	movsd   xmm0, QWORD[eax]
    addsd   xmm0, QWORD[rpsp]
    movsd QWORD[rpsp], xmm0
	; set var operation back to default
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx
	jmp	rnext

localDoubleMinus:
	movsd xmm0, QWORD[rpsp]
    subsd xmm0, [eax]
    movsd QWORD[rpsp], xmm0
	; set var operation back to default
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx
	jmp	rnext

localDoubleActionTable:
	DD	localDoubleFetch
	DD	localDoubleFetch
	DD	localDoubleRef
	DD	localDoubleStore
	DD	localDoublePlusStore
	DD	localDoubleMinusStore
    DD  localLongClear
	DD	localDoublePlus
    DD  badVarOperation     ; this is varop Inc, which doesn't exist for float and double
	DD	localDoubleMinus

localDouble1:
	cmp	ebx, kVarGetDec
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, [localDoubleActionTable + ebx*4]
	jmp	ebx

entry fieldDoubleType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz fieldDoubleType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
fieldDoubleType1:
	; get ptr to double var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [rpsp]
	add	rpsp, 4
	and	ebx, VAROP_LOMASK
	add	eax, ebx
	jmp	doubleEntry

entry memberDoubleType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz memberDoubleType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
memberDoubleType1:
	; get ptr to double var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [rcore + FCore.TPtr]
	and	ebx, VAROP_LOMASK
	add	eax, ebx
	jmp	doubleEntry

entry localDoubleArrayType
	; get ptr to double var into eax
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	mov	ebx, [rpsp]		; add in array index on TOS
	add	rpsp, 4
	sal	ebx, 3
	add	eax, ebx
	jmp doubleEntry

entry fieldDoubleArrayType
	; get ptr to double var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [rpsp+4]	; eax = index
	sal	eax, 3
	add	eax, [rpsp]		; add in struct base ptr
	add	rpsp, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	doubleEntry

entry memberDoubleArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [rpsp]	; eax = index
	sal	eax, 3
	add	eax, [rcore + FCore.TPtr]
	add	rpsp, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	doubleEntry
	
;-----------------------------------------------
;
;  string ops
;
GLOBAL localStringType, stringEntry, localStringFetch, localStringStore, localStringAppend, localStringClear
entry localStringType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz localStringType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
localStringType1:
	; get ptr to string var into eax
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, VAROP_LOMASK
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
stringEntry:
	mov	ebx, [rcore + FCore.varMode]
	or	ebx, ebx
	jnz	localString1
	; fetch local string
localStringFetch:
	sub	rpsp, 4
	add	eax, 8		; skip maxLen & currentLen fields
	mov	[rpsp], eax
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localString1:
	cmp	ebx, kVarClear
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, [localStringActionTable + ebx*4]
	jmp	ebx
	
; ref on a string returns the address of maxLen field, not the string chars
localStringRef:
	sub	rpsp, 4
	mov	[rpsp], eax
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext
	
localStringStore:
	; eax -> dest string maxLen field
	; TOS is src string addr
	mov	[rcore + FCore.IPtr], rip	; IP will get stomped
	mov	ecx, [rpsp]			; ecx -> chars of src string
	add	rpsp, 4
	mov	[rcore + FCore.SPtr], rpsp
	lea	rnext, [eax + 8]		; rnext -> chars of dst string
    sub esp, 4              ; 16-byte align for mac
	push	ecx				; strlen will stomp on ecx
	push	ecx
	xcall	strlen
	mov ecx, [esp + 4]
    add esp, 12
	; eax is src string length

	; figure how many chars to copy
	mov	ebx, [rnext - 8]		; ebx = max string length
	cmp	eax, ebx
	jle lsStore1
	mov	eax, ebx
lsStore1:
	; set current length field
	mov	[rnext - 4], eax
	
	; do the copy
	sub esp, 12      ; thanks OSX!
	push	eax		; push numBytes
	push	eax		; and save a copy in case strncpy modifies its stack inputs
	push	ecx		; srcPtr
	push	rnext		; dstPtr
	xcall	strncpy
	add esp, 12
	pop	rip			; rip = numBytes
    	add esp, 12

	; add the terminating null
	xor	eax, eax
	mov	[rnext + rip], al
		
	; set var operation back to default
	mov	[rcore + FCore.varMode], eax
	jmp	interpFunc

localStringAppend:
	; eax -> dest string maxLen field
	; TOS is src string addr
	mov	[rcore + FCore.IPtr], rip	; IP will get stomped
	mov	ecx, [rpsp]			; ecx -> chars of src string
	add	rpsp, 4
	mov	[rcore + FCore.SPtr], rpsp
	lea	rnext, [eax + 8]		; rnext -> chars of dst string
    sub esp, 8              ; 16-byte align for mac
	push	ecx
	xcall	strlen
	add	esp, 12
	; eax is src string length

	; figure how many chars to copy
	mov	ebx, [rnext - 8]		; ebx = max string length
	mov	rip, [rnext - 4]		; rip = cur string length
	add	rip, eax
	cmp	rip, ebx
	jle lsAppend1
	mov	eax, ebx
	sub	eax, [rnext - 4]		; #bytes to copy = maxLen - curLen
	mov	rip, ebx			; new curLen = maxLen
lsAppend1:
	; set current length field
	mov	[rnext - 4], rip
	
	; do the append
	push	eax		; push numBytes
	push	ecx		; srcPtr
	push	rnext		; dstPtr
	; don't need to worry about strncat stomping registers since we jump to interpFunc
	xcall	strncat
	add	esp, 12

	; add the terminating null
	xor	eax, eax
	mov	rip, [rnext - 4]
	mov	[rnext + rip], al
		
	; set var operation back to default
	mov	[rcore + FCore.varMode], eax
	jmp	interpFunc

localStringClear:
	; eax -> dest string maxLen field
    add eax, 4              ; skip maxLen field
    xor ebx, ebx
	mov	[rcore + FCore.varMode], ebx	; set var operation back to default
    mov [eax], ebx          ; set curLen = 0
    add eax, 4
    mov [eax], bl           ; and set first byte to terminating nul
	jmp	rnext

localStringActionTable:
	DD	localStringFetch
	DD	localStringFetch
	DD	localStringRef
	DD	localStringStore
	DD	localStringAppend
	DD	badVarOperation
	DD	localStringClear

entry fieldStringType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz fieldStringType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
fieldStringType1:
	; get ptr to byte var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [rpsp]
	add	rpsp, 4
	and	ebx, VAROP_LOMASK
	add	eax, ebx
	jmp	stringEntry

entry memberStringType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz memberStringType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
memberStringType1:
	; get ptr to byte var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [rcore + FCore.TPtr]
	and	ebx, VAROP_LOMASK
	add	eax, ebx
	jmp	stringEntry

entry localStringArrayType
	; get ptr to int var into eax
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx		; eax -> maxLen field of string[0]
	mov	ebx, [eax]
	sar	ebx, 2
	add	ebx, 3			; ebx is element length in longs
	imul	ebx, [rpsp]	; mult index * element length
	add	rpsp, 4
	sal	ebx, 2			; ebx is offset in bytes
	add	eax, ebx
	jmp stringEntry

entry fieldStringArrayType
	; get ptr to string var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	and	ebx, 00FFFFFFh
	add	ebx, [rpsp]		; ebx -> maxLen field of string[0]
	mov	eax, [ebx]		; eax = maxLen
	sar	eax, 2
	add	eax, 3			; eax is element length in longs
	imul	eax, [rpsp+4]	; mult index * element length
	sal	eax, 2
	add	eax, ebx		; eax -> maxLen field of string[N]
	add	rpsp, 8
	jmp	stringEntry

entry memberStringArrayType
	; get ptr to string var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	and	ebx, 00FFFFFFh
	add	ebx, [rcore + FCore.TPtr]	; ebx -> maxLen field of string[0]
	mov	eax, [ebx]		; eax = maxLen
	sar	eax, 2
	add	eax, 3			; eax is element length in longs
	imul	eax, [rpsp]	; mult index * element length
	sal	eax, 2
	add	eax, ebx		; eax -> maxLen field of string[N]
	add	rpsp, 4
	jmp	stringEntry

;-----------------------------------------------
;
;  op ops
;
entry localOpType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz localOpType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
localOpType1:
	; get ptr to op var into ebx
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, VAROP_LOMASK
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch (execute for ops)
opEntry:
	mov	ebx, [rcore + FCore.varMode]
	or	ebx, ebx
	jnz	localOp1
	; execute local op
localOpExecute:
    xor ebx, ebx
	mov	[rcore + FCore.varMode], ebx	; set var operation back to default
	mov	ebx, [eax]
	mov	eax, [rcore + FCore.innerExecute]
	jmp eax

localOpFetch:
	sub	rpsp, 4
	mov	ebx, [eax]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localOpActionTable:
	DD	localOpExecute
	DD	localOpFetch
	DD	localRef
	DD	localIntStore

localOp1:
	cmp	ebx, kVarSet
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, [localOpActionTable + ebx*4]
	jmp	ebx

entry fieldOpType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz fieldOpType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
fieldOpType1:
	; get ptr to op var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [rpsp]
	add	rpsp, 4
	and	ebx, VAROP_LOMASK
	add	eax, ebx
	jmp	opEntry

entry memberOpType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz memberOpType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
memberOpType1:
	; get ptr to op var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [rcore + FCore.TPtr]
	and	ebx, VAROP_LOMASK
	add	eax, ebx
	jmp	opEntry

entry localOpArrayType
	; get ptr to op var into eax
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, 00FFFFFFh
	sub	ebx, [rpsp]		; add in array index on TOS
	add	rpsp, 4
	sal	ebx, 2
	sub	eax, ebx
	jmp	opEntry

entry fieldOpArrayType
	; get ptr to op var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [rpsp+4]	; eax = index
	sal	eax, 2
	add	eax, [rpsp]		; add in struct base ptr
	add	rpsp, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	opEntry

entry memberOpArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [rpsp]	; eax = index
	sal	eax, 2
	add	eax, [rcore + FCore.TPtr]
	add	rpsp, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	opEntry
	
;-----------------------------------------------
;
;  long (int64) ops
;
entry localLongType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz localLongType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
localLongType1:
	; get ptr to long var into eax
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, VAROP_LOMASK
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
longEntry:
	mov	ebx, [rcore + FCore.varMode]
	or	ebx, ebx
	jnz	localLong1
	; fetch local double
localLongFetch:
	sub	rpsp, 8
	mov	ebx, [eax]
	mov	[rpsp+4], ebx
	mov	ebx, [eax+4]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localLong1:
	cmp	ebx, kVarGetDec
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, [localLongActionTable + ebx*4]
	jmp	ebx

localLongRef:
	sub	rpsp, 4
	mov	[rpsp], eax
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext
	
localLongStore:
	mov	ebx, [rpsp]
	mov	[eax+4], ebx
	mov	ebx, [rpsp+4]
	mov	[eax], ebx
	add	rpsp, 8
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localLongPlusStore:
	mov	ebx, [eax]
	add	ebx, [rpsp+4]
	mov	[eax], ebx
	mov	ebx, [eax+4]
	adc	ebx, [rpsp]
	mov	[eax+4], ebx
	; set var operation back to default
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx
	add	rpsp, 8
	jmp	rnext

localLongMinusStore:
	mov	ebx, [eax]
	sub	ebx, [rpsp+4]
	mov	[eax], ebx
	mov	ebx, [eax+4]
	sbb	ebx, [rpsp]
	mov	[eax+4], ebx
	; set var operation back to default
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx
	add	rpsp, 8
	jmp	rnext

localLongClear:
	xor	ebx, ebx
	mov	[eax], ebx
	mov	[eax+4], ebx
	; set var operation back to default
	mov	[rcore + FCore.varMode], ebx
	jmp	rnext

localLongPlus:
	mov	ebx, [eax]
	add	ebx, [rpsp+4]
	mov	[rpsp+4], ebx
	mov	ebx, [eax+4]
	adc	ebx, [rpsp]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localLongInc:
	; set var operation back to default
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx
    inc ebx
    add ebx, [eax]
    mov [eax], ebx
    jnc localLongInc1
    inc DWORD[eax + 4]
localLongInc1:
    jmp	rnext

localLongMinus:
    mov ebx, [rpsp+4]
    sub ebx, [eax]
	mov	[rpsp+4], ebx
    mov ebx, [rpsp]
    sbb ebx, [eax+4]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localLongDec:
	; set var operation back to default
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx
    mov ebx, [eax]
    or  ebx, ebx
    jnz localLongDec1
    dec DWORD[eax + 4]
localLongDec1:
    dec ebx
    mov [eax], ebx
	jmp	rnext

localLongIncGet:
    sub rpsp, 8
	; set var operation back to default
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx
    inc ebx
    add ebx, [eax]
    mov [eax], ebx
    mov [rpsp+4], ebx
    mov ebx, [eax+4]
    jnc localLongIncGet1
    inc ebx
localLongIncGet1:
    mov [eax+4], ebx
    mov [rpsp], ebx
	jmp	rnext

localLongDecGet:
    sub rpsp, 8
	; set var operation back to default
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx
    mov ebx, [eax]
    mov ecx, [eax+4]
    or  ebx, ebx
    jnz localLongDecGet1
    dec ecx
localLongDecGet1:
    dec ebx
    mov [eax], ebx
    mov [rpsp+4], ebx
    mov [eax+4], ecx
    mov [rpsp], ecx
	jmp	rnext

localLongGetInc:
    sub rpsp, 8
	; set var operation back to default
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx
    inc ebx
    mov ecx, [eax]
    mov [rpsp+4], ecx
    add ecx, ebx
    mov [eax], ecx
    mov ecx, [eax+4]
    mov [rpsp], ecx
    jnc localLongGetInc1
    inc ecx
localLongGetInc1:
    mov [eax+4], ecx
	jmp	rnext

localLongGetDec:
    sub rpsp, 8
	; set var operation back to default
	xor	ebx, ebx
	mov	[rcore + FCore.varMode], ebx
    mov ebx, [eax]
    mov ecx, [eax+4]
    mov [rpsp+4], ebx
    mov [rpsp], ecx
    or  ebx, ebx
    jnz localLongGetDec1
    dec ecx
localLongGetDec1:
    dec ebx
    mov [eax], ebx
    mov [eax+4], ecx
	jmp	rnext

localLongActionTable:
	DD	localLongFetch
	DD	localLongFetch
	DD	localLongRef            ; why isn't this just localRef?
	DD	localLongStore
	DD	localLongPlusStore
	DD	localLongMinusStore
    DD  localLongClear
    DD  localLongPlus
    DD  localLongInc
    DD  localLongMinus
    DD  localLongDec
    DD  localLongIncGet
    DD  localLongDecGet
    DD  localLongGetInc
    DD  localLongGetDec

entry fieldLongType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz fieldLongType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
fieldLongType1:
	; get ptr to double var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [rpsp]
	add	rpsp, 4
	and	ebx, VAROP_LOMASK
	add	eax, ebx
	jmp	longEntry

entry memberLongType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz memberLongType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
memberLongType1:
	; get ptr to double var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [rcore + FCore.TPtr]
	and	ebx, VAROP_LOMASK
	add	eax, ebx
	jmp	longEntry

entry localLongArrayType
	; get ptr to double var into eax
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	mov	ebx, [rpsp]		; add in array index on TOS
	add	rpsp, 4
	sal	ebx, 3
	add	eax, ebx
	jmp longEntry

entry fieldLongArrayType
	; get ptr to double var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [rpsp+4]	; eax = index
	sal	eax, 3
	add	eax, [rpsp]		; add in struct base ptr
	add	rpsp, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	longEntry

entry memberLongArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [rpsp]	; eax = index
	sal	eax, 3
	add	eax, [rcore + FCore.TPtr]
	add	rpsp, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	longEntry
	
;-----------------------------------------------
;
;  object ops
;
entry localObjectType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz localObjectType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
localObjectType1:
	; get ptr to Object var into eax
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, VAROP_LOMASK
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
objectEntry:
	mov	ebx, [rcore + FCore.varMode]
	or	ebx, ebx
	jnz	localObject1
	; fetch local Object
localObjectFetch:
	sub	rpsp, 4
	mov	ebx, [eax]
	mov	[rpsp], ebx
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

localObject1:
	cmp	ebx, kVarClear
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, [localObjectActionTable + ebx*4]
	jmp	ebx

localObjectRef:
	sub	rpsp, 4
	mov	[rpsp], eax
	; set var operation back to default
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	jmp	rnext
	
localObjectStore:
	; TOS is new object
	; eax -> destinationVar/old object
	xor	ebx, ebx			; set var operation back to default/fetch
	mov	[rcore + FCore.varMode], ebx
los0:
	push rnext
	mov ebx, [eax]		; ebx = olbObj
	mov rnext, eax		; rnext -> destinationVar
	mov eax, [rpsp]		; eax = newObj
	cmp eax, ebx
	jz losx				; objects are same, don't change refcount
	; handle newObj refcount
	or eax, eax
	jz los1				; if newObj is null, don't try to increment refcount

	; newObj isn't null, increment its refcount
%ifdef ATOMIC_REFCOUNTS
	mov	ecx, 1
	lock xadd dword[eax + Object.refCount], ecx
%else
	inc DWORD[eax + Object.refCount]
%endif

los1:
	mov	[rnext], eax		; destinationVar = newObj

	; handle oldObj refcount
	or ebx, ebx
	jz losx				; if oldObj is null, don't try to decrement refcount

	; oldObj  isn't null, decrement its refcount
%ifdef ATOMIC_REFCOUNTS
	mov	ecx, -1
	lock xadd dword[ebx + Object.refCount], ecx
	cmp	ecx, 1
	jz los3
%else
	dec DWORD[ebx + Object.refCount]
	jz los3
%endif

losx:
	add	rpsp, 4
	pop rnext
	jmp	rnext

	; object var held last reference to oldObj, invoke olbObj.delete method
los3:
	; ebx = oldObj

	; push this ptr on return stack
	mov	ecx, [rcore + FCore.RPtr]
	sub	ecx, 4
	mov	[rcore + FCore.RPtr], ecx
	mov	eax, [rcore + FCore.TPtr]
	mov	[ecx], eax
	
	; set this to oldObj
	mov	[rcore + FCore.TPtr], ebx
	mov	ebx, [ebx]	; ebx = oldObj methods pointer
	mov	ebx, [ebx]	; ebx = oldObj method 0 (delete)

	pop rnext
	
	add	rpsp, 4

	; execute the delete method opcode which is in ebx
	mov	eax, [rcore + FCore.innerExecute]
	jmp eax

localObjectClear:
	; TOS is new object, eax points to destination/old object
	xor	ebx, ebx			; set var operation back to default/fetch
	mov	[rcore + FCore.varMode], ebx
	sub	rpsp, 4
	mov [rpsp], ebx
	; for now, do the clear operation by pushing null on TOS then doing a regular object store
	; later on optimize it since we know source is a null
	jmp	los0

; store object on TOS in variable pointed to by eax
; do not adjust reference counts of old object or new object
localObjectStoreNoRef:
	; TOS is new object, eax points to destination/old object
	xor	ebx, ebx			; set var operation back to default/fetch
	mov	[rcore + FCore.varMode], ebx
	mov	ebx, [rpsp]
	mov	[eax], ebx
	add	rpsp, 4
	jmp	rnext

; clear object reference, leave on TOS
localObjectUnref:
	; leave object on TOS
	sub	rpsp, 4
	mov	ebx, [eax]
	mov	[rpsp], ebx
	; if object var is already null, do nothing else
	or	ebx, ebx
	jz	lou2
	; clear object var
	mov	ecx, eax		; ecx -> object var
	xor	eax, eax
	mov	[ecx], eax
	; set var operation back to default
	mov	[rcore + FCore.varMode], eax
	; get object refcount, see if it is already 0

	; decrement object refcount
%ifdef ATOMIC_REFCOUNTS
	mov	ecx, -1
	lock xadd dword[ebx + Object.refCount], ecx
	or ecx, ecx
	; ugh, this leaves count as negative
	jz	louNegativeCountError
%else
	mov	eax, [ebx + Object.refCount]
	or	eax, eax
	jz	louNegativeCountError
lou1:
	sub	eax, 1
	mov	[ebx + Object.refCount], eax
%endif
lou2:
	jmp	rnext

louNegativeCountError:
	; report refcount negative error
	mov	eax, kForthErrorBadReferenceCount
	jmp	interpLoopErrorExit
	
localObjectActionTable:
	DD	localObjectFetch
	DD	localObjectFetch
	DD	localObjectRef
	DD	localObjectStore
	DD	localObjectStoreNoRef
	DD	localObjectUnref
	DD	localObjectClear

entry fieldObjectType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz fieldObjectType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
fieldObjectType1:
	; get ptr to Object var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [rpsp]
	add	rpsp, 4
	and	ebx, VAROP_LOMASK
	add	eax, ebx
	jmp	objectEntry

entry memberObjectType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, VAROP_HIMASK
	jz memberObjectType1
	shr	eax, VAROP_SHIFT
	mov	[rcore + FCore.varMode], eax
memberObjectType1:
	; get ptr to Object var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [rcore + FCore.TPtr]
	and	ebx, VAROP_LOMASK
	add	eax, ebx
	jmp	objectEntry

entry localObjectArrayType
	; get ptr to Object var into eax
	mov	eax, [rcore + FCore.FPtr]
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	mov	ebx, [rpsp]		; add in array index on TOS
	add	rpsp, 4
	sal	ebx, 3
	add	eax, ebx
	jmp objectEntry

entry fieldObjectArrayType
	; get ptr to Object var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [rpsp+4]	; eax = index
	sal	eax, 3
	add	eax, [rpsp]		; add in struct base ptr
	add	rpsp, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	objectEntry

entry memberObjectArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [rpsp]	; eax = index
	sal	eax, 3
	add	eax, [rcore + FCore.TPtr]
	add	rpsp, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	objectEntry
	
;-----------------------------------------------
;
; method invocation ops
;

; invoke a method on object currently referenced by this ptr pair
entry methodWithThisType
	; ebx is method number
	; push this on return stack
	mov	ecx, [rcore + FCore.RPtr]
	sub	ecx, 4
	mov	[rcore + FCore.RPtr], ecx
	mov	eax, [rcore + FCore.TPtr]
	or	eax, eax
	jnz methodThis1
	mov	eax, kForthErrorBadObject
	jmp	interpLoopErrorExit
methodThis1:
	mov	[ecx], eax
	
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	add	ebx, [eax]
	mov	ebx, [ebx]	; ebx = method opcode
	mov	eax, [rcore + FCore.innerExecute]
	jmp eax
	
; invoke a method on an object referenced by ptr pair on TOS
entry methodWithTOSType
	; TOS is object
	; ebx is method number
	; push current this on return stack
	mov	ecx, [rcore + FCore.RPtr]
	sub	ecx, 4
	mov	[rcore + FCore.RPtr], ecx
	mov	eax, [rcore + FCore.TPtr]
	mov	[ecx], eax

	; set this ptr from TOS	
	mov	eax, [rpsp]
	or	eax, eax
	jnz methodTos1
	mov	eax, kForthErrorBadObject
	jmp	interpLoopErrorExit
methodTos1:
	mov	[rcore + FCore.TPtr], eax
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	add	ebx, [eax]
	mov	ebx, [ebx]	; ebx = method opcode
	add	rpsp, 4
	mov	eax, [rcore + FCore.innerExecute]
	jmp eax
	
; invoke a method on super class of object currently referenced by this ptr pair
entry methodWithSuperType
	; ebx is method number
	mov	ecx, [rcore + FCore.RPtr]
	sub	ecx, 8
	mov	[rcore + FCore.RPtr], ecx
	mov	eax, [rcore + FCore.TPtr]
    push ebx
	; push original methods ptr on return stack
	mov	ebx, [eax]			; ebx is original methods ptr
	mov	[ecx+4], ebx
	; push this on return stack
	mov	[ecx], eax
	
	mov	ecx, [ebx-4]		; ecx -> class vocabulary object
	mov	eax, [ecx+8]		; eax -> class vocabulary
	push rpsp
    push ecx
    sub esp, 12         ; 16-byte align for OSX
    push eax            ; class vocabulary
	xcall getSuperClassMethods
	; eax -> super class methods table
	mov	ebx, [rcore + FCore.TPtr]
	mov	[ebx], eax		; set this methods ptr to super class methods
	add esp, 16
	pop ecx
	pop rpsp
	pop ebx
	; ebx is method number
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	add	ebx, eax
	mov	ebx, [ebx]	; ebx = method opcode
	mov	eax, [rcore + FCore.innerExecute]
	jmp eax
	
;-----------------------------------------------
;
; member string init ops
;
entry memberStringInitType
   ; bits 0..11 are string length in bytes, bits 12..23 are member offset in longs
   ; init the current & max length fields of a member string
	mov	eax, 00FFF000h
	and	eax, ebx
	sar	eax, 10							; eax = member offset in bytes
	mov	ecx, [rcore + FCore.TPtr]
	add	ecx, eax						; ecx -> max length field
	and	ebx, 00000FFFh					; ebx = max length
	mov	[ecx], ebx						; set max length
	xor	eax, eax
	mov	[ecx+4], eax					; set current length to 0
	mov	[ecx+8], al						; add terminating null
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
	; get ptr to byte var into eax
	mov	eax, rip
	; pop rstack
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	byteEntry

;-----------------------------------------------
;
; doUByteOp is compiled as the first op in global unsigned byte vars
; the byte data field is immediately after this op
;
entry doUByteBop
	; get ptr to byte var into eax
	mov	eax, rip
	; pop rstack
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	ubyteEntry

;-----------------------------------------------
;
; doByteArrayOp is compiled as the first op in global byte arrays
; the data array is immediately after this op
;
entry doByteArrayBop
	; get ptr to byte var into eax
	mov	eax, rip
	add	eax, [rpsp]
	add	rpsp, 4
	; pop rstack
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	byteEntry

entry doUByteArrayBop
	; get ptr to byte var into eax
	mov	eax, rip
	add	eax, [rpsp]
	add	rpsp, 4
	; pop rstack
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	ubyteEntry

;-----------------------------------------------
;
; doShortOp is compiled as the first op in global short vars
; the short data field is immediately after this op
;
entry doShortBop
	; get ptr to short var into eax
	mov	eax, rip
	; pop rstack
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	shortEntry

;-----------------------------------------------
;
; doUShortOp is compiled as the first op in global unsigned short vars
; the short data field is immediately after this op
;
entry doUShortBop
	; get ptr to short var into eax
	mov	eax, rip
	; pop rstack
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	ushortEntry

;-----------------------------------------------
;
; doShortArrayOp is compiled as the first op in global short arrays
; the data array is immediately after this op
;
entry doShortArrayBop
	; get ptr to short var into eax
	mov	eax, rip
	mov	ebx, [rpsp]		; ebx = array index
	add	rpsp, 4
	sal	ebx, 1
	add	eax, ebx	
	; pop rstack
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	shortEntry

entry doUShortArrayBop
	; get ptr to short var into eax
	mov	eax, rip
	mov	ebx, [rpsp]		; ebx = array index
	add	rpsp, 4
	sal	ebx, 1
	add	eax, ebx	
	; pop rstack
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	ushortEntry

;-----------------------------------------------
;
; doIntOp is compiled as the first op in global int vars
; the int data field is immediately after this op
;
entry doIntBop
	; get ptr to int var into eax
	mov	eax, rip
	; pop rstack
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	intEntry

;-----------------------------------------------
;
; doIntArrayOp is compiled as the first op in global int arrays
; the data array is immediately after this op
;
entry doIntArrayBop
	; get ptr to int var into eax
	mov	eax, rip
	mov	ebx, [rpsp]		; ebx = array index
	add	rpsp, 4
	sal	ebx, 2
	add	eax, ebx	
	; pop rstack
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	intEntry

;-----------------------------------------------
;
; doFloatOp is compiled as the first op in global float vars
; the float data field is immediately after this op
;
entry doFloatBop
	; get ptr to float var into eax
	mov	eax, rip
	; pop rstack
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	floatEntry

;-----------------------------------------------
;
; doFloatArrayOp is compiled as the first op in global float arrays
; the data array is immediately after this op
;
entry doFloatArrayBop
	; get ptr to float var into eax
	mov	eax, rip
	mov	ebx, [rpsp]		; ebx = array index
	add	rpsp, 4
	sal	ebx, 2
	add	eax, ebx	
	; pop rstack
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	floatEntry

;-----------------------------------------------
;
; doDoubleOp is compiled as the first op in global double vars
; the data field is immediately after this op
;
entry doDoubleBop
	; get ptr to double var into eax
	mov	eax, rip
	; pop rstack
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	doubleEntry

;-----------------------------------------------
;
; doDoubleArrayOp is compiled as the first op in global double arrays
; the data array is immediately after this op
;
entry doDoubleArrayBop
	; get ptr to double var into eax
	mov	eax, rip
	mov	ebx, [rpsp]		; ebx = array index
	add	rpsp, 4
	sal	ebx, 3
	add	eax, ebx	
	; pop rstack
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	doubleEntry

;-----------------------------------------------
;
; doStringOp is compiled as the first op in global string vars
; the data field is immediately after this op
;
entry doStringBop
	; get ptr to string var into eax
	mov	eax, rip
	; pop rstack
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	stringEntry

;-----------------------------------------------
;
; doStringArrayOp is compiled as the first op in global string arrays
; the data array is immediately after this op
;
entry doStringArrayBop
	; get ptr to string var into eax
	mov	eax, rip		; eax -> maxLen field of string[0]
	mov	ebx, [eax]		; ebx = maxLen
	sar	ebx, 2
	add	ebx, 3			; ebx is element length in longs
	imul	ebx, [rpsp]	; mult index * element length
	add	rpsp, 4
	sal	ebx, 2			; ebx is offset in bytes
	add	eax, ebx
	; pop rstack
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp stringEntry

;-----------------------------------------------
;
; doOpOp is compiled as the first op in global op vars
; the op data field is immediately after this op
;
entry doOpBop
	; get ptr to int var into eax
	mov	eax, rip
	; pop rstack
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	opEntry

;-----------------------------------------------
;
; doOpArrayOp is compiled as the first op in global op arrays
; the data array is immediately after this op
;
entry doOpArrayBop
	; get ptr to op var into eax
	mov	eax, rip
	mov	ebx, [rpsp]		; ebx = array index
	add	rpsp, 4
	sal	ebx, 2
	add	eax, ebx	
	; pop rstack
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	opEntry

;-----------------------------------------------
;
; doLongOp is compiled as the first op in global int64 vars
; the data field is immediately after this op
;
entry doLongBop
	; get ptr to double var into eax
	mov	eax, rip
	; pop rstack
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	longEntry

;-----------------------------------------------
;
; doLongArrayOp is compiled as the first op in global int64 arrays
; the data array is immediately after this op
;
entry doLongArrayBop
	; get ptr to double var into eax
	mov	eax, rip
	mov	ebx, [rpsp]		; ebx = array index
	add	rpsp, 4
	sal	ebx, 3
	add	eax, ebx	
	; pop rstack
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	longEntry

;-----------------------------------------------
;
; doObjectOp is compiled as the first op in global Object vars
; the data field is immediately after this op
;
entry doObjectBop
	; get ptr to Object var into eax
	mov	eax, rip
	; pop rstack
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	objectEntry

;-----------------------------------------------
;
; doObjectArrayOp is compiled as the first op in global Object arrays
; the data array is immediately after this op
;
entry doObjectArrayBop
	; get ptr to Object var into eax
	mov	eax, rip
	mov	ebx, [rpsp]		; ebx = array index
	add	rpsp, 4
	; TODO: is this right for 32-bits? shouldn't it be sal 2?
	sal	ebx, 3
	add	eax, ebx	
	; pop rstack
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	objectEntry

;========================================

entry initStringBop
	;	TOS: len strPtr
	mov	ebx, [rpsp+4]	; ebx -> first char of string
	xor	eax, eax
	mov	[ebx-4], eax	; set current length = 0
	mov	[ebx], al		; set first char to terminating null
	mov	eax, [rpsp]		; eax == string length
	mov	[ebx-8], eax	; set maximum length field
	add	rpsp, 8
	jmp	rnext

;========================================

entry strFixupBop
	mov	eax, [rpsp]
	add	rpsp, 4
	mov ecx, eax
	xor	ebx, ebx
	; stuff a nul at end of string storage - there should already be one there or earlier
	add	ecx, [eax-8]
	mov	[ecx], bl
	mov ecx, eax
strFixupBop1:
	mov	bl, [eax]
	test	bl, 255
	jz	strFixupBop2
	add	eax, 1
	jmp	strFixupBop1

strFixupBop2:
	sub	eax, ecx
	mov	ebx, [ecx-8]
	cmp	ebx, eax
	jge	strFixupBop3
	; characters have been written past string storage end
	mov	eax, kForthErrorStringOverflow
	jmp	interpLoopErrorExit

strFixupBop3:
	mov	[ecx-4], eax
	jmp	rnext

;========================================

entry doneBop
	mov	eax, kResultDone
	jmp	interpLoopExit

;========================================

entry yieldBop
	mov	eax, kResultYield
	jmp	interpLoopExit

;========================================

entry abortBop
	mov	eax, kForthErrorAbort
	jmp	interpLoopFatalErrorExit

;========================================

entry noopBop
	jmp	rnext

;========================================
	
entry plusBop
	mov	eax, [rpsp]
	add	rpsp, 4
	add	eax, [rpsp]
	mov	[rpsp], eax
	jmp	rnext

;========================================
	
entry minusBop
	mov	eax, [rpsp]
	add	rpsp, 4
	mov	ebx, [rpsp]
	sub	ebx, eax
	mov	[rpsp], ebx
	jmp	rnext

;========================================

entry timesBop
	mov	eax, [rpsp]
	add	rpsp, 4
	imul	eax, [rpsp]
	mov	[rpsp], eax
	jmp	rnext

;========================================
	
entry times2Bop
	mov	eax, [rpsp]
	add	eax, eax
	mov	[rpsp], eax
	jmp	rnext

;========================================
	
entry times4Bop
	mov	eax, [rpsp]
	sal	eax, 2
	mov	[rpsp], eax
	jmp	rnext

;========================================
	
entry times8Bop
	mov	eax, [rpsp]
	sal	eax, 3
	mov	[rpsp], eax
	jmp	rnext

;========================================
	
entry divideBop
	; idiv takes 64-bit numerator in rpsp:eax
	mov	ebx, rpsp
	mov	eax, [rpsp+4]	; get numerator
	cdq					; convert into 64-bit in rpsp:eax
	idiv	DWORD[ebx]		; eax is quotient, rpsp is remainder
	add	ebx, 4
	mov	[ebx], eax
	mov	rpsp, ebx
	jmp	rnext

;========================================

entry divide2Bop
	mov	eax, [rpsp]
	sar	eax, 1
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry divide4Bop
	mov	eax, [rpsp]
	sar	eax, 2
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry divide8Bop
	mov	eax, [rpsp]
	sar	eax, 3
	mov	[rpsp], eax
	jmp	rnext
	
;========================================
	
entry divmodBop
	; idiv takes 64-bit numerator in rpsp:eax
	mov	ebx, rpsp
	mov	eax, [rpsp+4]	; get numerator
	cdq					; convert into 64-bit in rpsp:eax
	idiv	DWORD[ebx]		; eax is quotient, rpsp is remainder
	mov	[ebx+4], rpsp
	mov	[ebx], eax
	mov	rpsp, ebx
	jmp	rnext
	
;========================================
	
entry modBop
	; idiv takes 64-bit numerator in rpsp:eax
	mov	ebx, rpsp
	mov	eax, [rpsp+4]	; get numerator
	cdq					; convert into 64-bit in rpsp:eax
	idiv	DWORD[ebx]		; eax is quotient, rpsp is remainder
	add	ebx, 4
	mov	[ebx], rpsp
	mov	rpsp, ebx
	jmp	rnext
	
;========================================
	
entry negateBop
	mov	eax, [rpsp]
	neg	eax
	mov	[rpsp], eax
	jmp	rnext
	
;========================================
	
entry fplusBop
	fld	DWORD[rpsp+4]
	fadd	DWORD[rpsp]
	add	rpsp,4
	fstp	DWORD[rpsp]
	jmp	rnext
	
;========================================
	
entry fminusBop
	fld	DWORD[rpsp+4]
	fsub	DWORD[rpsp]
	add	rpsp,4
	fstp	DWORD[rpsp]
	jmp	rnext
	
;========================================
	
entry ftimesBop
	fld	DWORD[rpsp+4]
	fmul	DWORD[rpsp]
	add	rpsp,4
	fstp	DWORD[rpsp]
	jmp	rnext
	
;========================================
	
entry fdivideBop
	fld	DWORD[rpsp+4]
	fdiv	DWORD[rpsp]
	add	rpsp,4
	fstp	DWORD[rpsp]
	jmp	rnext
	
;========================================
	
entry faddBlockBop
	; TOS: num pDst pSrcB pSrcA
	push	rpsp
	mov	eax, [rpsp+12]
	mov	ebx, [rpsp+8]
	mov	ecx, [rpsp]
	mov	rpsp, [rpsp+4]
.faddBlockBop1:
	fld	DWORD[eax]
	fadd	DWORD[ebx]
	fstp	DWORD[rpsp]
	add eax,4
	add ebx,4
	add	rpsp,4
	sub ecx,1
	jnz .faddBlockBop1
	pop rpsp
	add	rpsp,16
	jmp	rnext
	
;========================================
	
entry fsubBlockBop
	; TOS: num pDst pSrcB pSrcA
	push	rpsp
	mov	eax, [rpsp+12]
	mov	ebx, [rpsp+8]
	mov	ecx, [rpsp]
	mov	rpsp, [rpsp+4]
.fsubBlockBop1:
	fld	DWORD[eax]
	fsub	DWORD[ebx]
	fstp	DWORD[rpsp]
	add eax,4
	add ebx,4
	add	rpsp,4
	sub ecx,1
	jnz .fsubBlockBop1
	pop rpsp
	add	rpsp,16
	jmp	rnext
	
;========================================
	
entry fmulBlockBop
	; TOS: num pDst pSrcB pSrcA
	push	rpsp
	mov	eax, [rpsp+12]
	mov	ebx, [rpsp+8]
	mov	ecx, [rpsp]
	mov	rpsp, [rpsp+4]
.fmulBlockBop1:
	fld	DWORD[eax]
	fmul	DWORD[ebx]
	fstp	DWORD[rpsp]
	add eax,4
	add ebx,4
	add	rpsp,4
	sub ecx,1
	jnz .fmulBlockBop1
	pop rpsp
	add	rpsp,16
	jmp	rnext
	
;========================================
	
entry fdivBlockBop
	; TOS: num pDst pSrcB pSrcA
	push	rpsp
	mov	eax, [rpsp+12]
	mov	ebx, [rpsp+8]
	mov	ecx, [rpsp]
	mov	rpsp, [rpsp+4]
.fdivBlockBop1:
	fld	DWORD[eax]
	fdiv	DWORD[ebx]
	fstp	DWORD[rpsp]
	add eax,4
	add ebx,4
	add	rpsp,4
	sub ecx,1
	jnz .fdivBlockBop1
	pop rpsp
	add	rpsp,16
	jmp	rnext
	
;========================================
	
entry fscaleBlockBop
	; TOS: num scale pDst pSrc
	mov	eax, [rpsp+12]	; pSrc
	mov	ebx, [rpsp+8]	; pDst
	fld	DWORD[rpsp + 4]	; scale
	mov	ecx, [rpsp]		; num
.fscaleBlockBop1:
	fld	DWORD[eax]
	fmul	st0, st1
	fstp	DWORD[ebx]
	add eax,4
	add ebx,4
	sub ecx,1
	jnz .fscaleBlockBop1
	ffree	st0
	add	rpsp,16
	jmp	rnext
	
;========================================
	
entry foffsetBlockBop
	; TOS: num scale pDst pSrc
	mov	eax, [rpsp+12]	; pSrc
	mov	ebx, [rpsp+8]	; pDst
	fld	DWORD[rpsp + 4]	; scale
	mov	ecx, [rpsp]		; num
.foffsetBlockBop1:
	fld	DWORD[eax]
	fadd	st0, st1
	fstp	DWORD[ebx]
	add eax,4
	add ebx,4
	sub ecx,1
	jnz .foffsetBlockBop1
	ffree	st0
	add	rpsp,16
	jmp	rnext
	
;========================================
	
entry fmixBlockBop
	; TOS: num scale pDst pSrc
	mov	eax, [rpsp+12]	; pSrc
	mov	ebx, [rpsp+8]	; pDst
	fld	DWORD[rpsp + 4]	; scale
	mov	ecx, [rpsp]		; num
.fmixBlockBop1:
	fld	DWORD[eax]
	fmul	st0, st1
	fadd	DWORD[ebx]
	fstp	DWORD[ebx]
	add eax,4
	add ebx,4
	sub ecx,1
	jnz .fmixBlockBop1
	ffree	st0
	add	rpsp,16
	jmp	rnext
	
;========================================
	
entry daddBlockBop
	; TOS: num pDst pSrcB pSrcA
	push	rpsp
	mov	eax, [rpsp+12]
	mov	ebx, [rpsp+8]
	mov	ecx, [rpsp]
	mov	rpsp, [rpsp+4]
.daddBlockBop1:
	fld	QWORD[eax]
	fadd	QWORD[ebx]
	fstp	QWORD[rpsp]
	add eax,8
	add ebx,8
	add	rpsp,8
	sub ecx,1
	jnz .daddBlockBop1
	pop rpsp
	add	rpsp,16
	jmp	rnext
	
;========================================
	
entry dsubBlockBop
	; TOS: num pDst pSrcB pSrcA
	push	rpsp
	mov	eax, [rpsp+12]
	mov	ebx, [rpsp+8]
	mov	ecx, [rpsp]
	mov	rpsp, [rpsp+4]
.dsubBlockBop1:
	fld	QWORD[eax]
	fsub	QWORD[ebx]
	fstp	QWORD[rpsp]
	add eax,8
	add ebx,8
	add	rpsp,8
	sub ecx,1
	jnz .dsubBlockBop1
	pop rpsp
	add	rpsp,16
	jmp	rnext
	
;========================================
	
entry dmulBlockBop
	; TOS: num pDst pSrcB pSrcA
	push	rpsp
	mov	eax, [rpsp+12]
	mov	ebx, [rpsp+8]
	mov	ecx, [rpsp]
	mov	rpsp, [rpsp+4]
.dmulBlockBop1:
	fld	QWORD[eax]
	fmul	QWORD[ebx]
	fstp	QWORD[rpsp]
	add eax,8
	add ebx,8
	add	rpsp,8
	sub ecx,1
	jnz .dmulBlockBop1
	pop rpsp
	add	rpsp,16
	jmp	rnext
	
;========================================
	
entry ddivBlockBop
	; TOS: num pDst pSrcB pSrcA
	push	rpsp
	mov	eax, [rpsp+12]
	mov	ebx, [rpsp+8]
	mov	ecx, [rpsp]
	mov	rpsp, [rpsp+4]
.ddivBlockBop1:
	fld	QWORD[eax]
	fdiv	QWORD[ebx]
	fstp	QWORD[rpsp]
	add eax,8
	add ebx,8
	add	rpsp,8
	sub ecx,1
	jnz .ddivBlockBop1
	pop rpsp
	add	rpsp,16
	jmp	rnext
	
;========================================
	
entry dscaleBlockBop
	; TOS: num scale pDst pSrc
	mov	eax, [rpsp+16]	; pSrc
	mov	ebx, [rpsp+12]	; pDst
	fld	QWORD[rpsp + 4]	; scale
	mov	ecx, [rpsp]		; num
.dscaleBlockBop1:
	fld	QWORD[eax]
	fmul	st0, st1
	fstp	QWORD[ebx]
	add eax,8
	add ebx,8
	sub ecx,1
	jnz .dscaleBlockBop1
	ffree	st0
	add	rpsp,20
	jmp	rnext
	
;========================================
	
entry doffsetBlockBop
	; TOS: num scale pDst pSrc
	mov	eax, [rpsp+16]	; pSrc
	mov	ebx, [rpsp+12]	; pDst
	fld	QWORD[rpsp + 4]	; scale
	mov	ecx, [rpsp]		; num
.doffsetBlockBop1:
	fld	QWORD[eax]
	fadd	st0, st1
	fstp	QWORD[ebx]
	add eax,8
	add ebx,8
	sub ecx,1
	jnz .doffsetBlockBop1
	ffree	st0
	add	rpsp,20
	jmp	rnext
	
;========================================
	
entry dmixBlockBop
	; TOS: num scale pDst pSrc
	mov	eax, [rpsp+16]	; pSrc
	mov	ebx, [rpsp+12]	; pDst
	fld	QWORD[rpsp + 4]	; scale
	mov	ecx, [rpsp]		; num
.dmixBlockBop1:
	fld	QWORD[eax]
	fmul	st0, st1
	fadd	QWORD[ebx]
	fstp	QWORD[ebx]
	add eax,8
	add ebx,8
	sub ecx,1
	jnz .dmixBlockBop1
	ffree	st0
	add	rpsp,20
	jmp	rnext
	
;========================================
	
entry fEquals0Bop
	fldz
	jmp	fEqualsBop1
	
entry fEqualsBop
	fld	DWORD[rpsp]
	add	rpsp, 4
fEqualsBop1:
	fld	DWORD[rpsp]
	xor	ebx, ebx
	fucompp
	fnstsw	ax
	test	ah, 44h
	jp	fEqualsBop2
	dec	ebx
fEqualsBop2:
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================
	
entry fNotEquals0Bop
	fldz
	jmp	fNotEqualsBop1
	
entry fNotEqualsBop
	fld	DWORD[rpsp]
	add	rpsp, 4
fNotEqualsBop1:
	fld	DWORD[rpsp]
	xor	ebx, ebx
	fucompp
	fnstsw	ax
	test	ah, 44h
	jnp	fNotEqualsBop2
	dec	ebx
fNotEqualsBop2:
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================
	
entry fGreaterThan0Bop
	fldz
	jmp	fGreaterThanBop1
	
entry fGreaterThanBop
	fld	DWORD[rpsp]
	add	rpsp, 4
fGreaterThanBop1:
	fld	DWORD[rpsp]
	xor	ebx, ebx
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 41h
	jne	fGreaterThanBop2
	dec	ebx
fGreaterThanBop2:
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================
	
entry fGreaterEquals0Bop
	fldz
	jmp	fGreaterEqualsBop1
	
entry fGreaterEqualsBop
	fld	DWORD[rpsp]
	add	rpsp, 4
fGreaterEqualsBop1:
	fld	DWORD[rpsp]
	xor	ebx, ebx
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 1
	jne	fGreaterEqualsBop2
	dec	ebx
fGreaterEqualsBop2:
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================
	
entry fLessThan0Bop
	fldz
	jmp	fLessThanBop1
	
entry fLessThanBop
	fld	DWORD[rpsp]
	add	rpsp, 4
fLessThanBop1:
	fld	DWORD[rpsp]
	xor	ebx, ebx
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 5
	jp	fLessThanBop2
	dec	ebx
fLessThanBop2:
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================
	
entry fLessEquals0Bop
	fldz
	jmp	fLessEqualsBop1
	
entry fLessEqualsBop
	fld	DWORD[rpsp]
	add	rpsp, 4
fLessEqualsBop1:
	fld	DWORD[rpsp]
	xor	ebx, ebx
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 41h
	jp	fLessEqualsBop2
	dec	ebx
fLessEqualsBop2:
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================
	
entry fWithinBop
	fld	DWORD[rpsp+4]
	fld	DWORD[rpsp+8]
	xor	ebx, ebx
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 41h
	jne	fWithinBop2
	fld	DWORD[rpsp]
	fld	DWORD[rpsp+8]
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 5
	jp	fWithinBop2
	dec	ebx
fWithinBop2:
	add	rpsp, 8
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================
	
entry fMinBop
	fld	DWORD[rpsp]
	fld	DWORD[rpsp+4]
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 41h
	jne	fMinBop2
	mov	eax, [rpsp]
	mov	[rpsp+4], eax
fMinBop2:
	add	rpsp, 4
	jmp	rnext
	
;========================================
	
entry fMaxBop
	fld	DWORD[rpsp]
	fld	DWORD[rpsp+4]
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 5
	jp	fMaxBop2
	mov	eax, [rpsp]
	mov	[rpsp+4], eax
fMaxBop2:
	add	rpsp, 4
	jmp	rnext
	
;========================================
	
entry dcmpBop
	fld	QWORD[rpsp]
	add	rpsp, 8
	fld	QWORD[rpsp]
	add	rpsp, 4
	xor	ebx, ebx
	fcomp	st1
	fnstsw	ax
	fstp	st0
	sahf
	jz	dcmpBop3
	jb	dcmpBop2
	add	ebx, 2
dcmpBop2:
	dec	ebx
dcmpBop3:
	mov	[rpsp], ebx
	jmp	rnext

;========================================
	
entry fcmpBop
	fld	DWORD[rpsp]
	add	rpsp, 4
	fld	DWORD[rpsp]
	xor	ebx, ebx
	fcomp	st1
	fnstsw	ax
	fstp	st0
	sahf
	jz	fcmpBop3
	jb	fcmpBop2
	add	ebx, 2
fcmpBop2:
	dec	ebx
fcmpBop3:
	mov	[rpsp], ebx
	jmp	rnext

;========================================
	
entry dplusBop
	fld	QWORD[rpsp+8]
	fadd	QWORD[rpsp]
	add	rpsp,8
	fstp	QWORD[rpsp]
	jmp	rnext
	
;========================================
	
entry dminusBop
	fld	QWORD[rpsp+8]
	fsub	QWORD[rpsp]
	add	rpsp,8
	fstp	QWORD[rpsp]
	jmp	rnext
	
;========================================
	
entry dtimesBop
	fld	QWORD[rpsp+8]
	fmul	QWORD[rpsp]
	add	rpsp,8
	fstp	QWORD[rpsp]
	jmp	rnext
	
;========================================
	
entry ddivideBop
	fld	QWORD[rpsp+8]
	fdiv	QWORD[rpsp]
	add	rpsp,8
	fstp	QWORD[rpsp]
	jmp	rnext
	
;========================================
	
entry dEquals0Bop
	fldz
	jmp	dEqualsBop1
	
entry dEqualsBop
	fld	QWORD[rpsp]
	add	rpsp, 8
dEqualsBop1:
	fld	QWORD[rpsp]
	add	rpsp, 4
	xor	ebx, ebx
	fucompp
	fnstsw	ax
	test	ah, 44h
	jp	dEqualsBop2
	dec	ebx
dEqualsBop2:
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================
	
entry dNotEquals0Bop
	fldz
	jmp	dNotEqualsBop1
	
entry dNotEqualsBop
	fld	QWORD[rpsp]
	add	rpsp, 8
dNotEqualsBop1:
	fld	QWORD[rpsp]
	add	rpsp, 4
	xor	ebx, ebx
	fucompp
	fnstsw	ax
	test	ah, 44h
	jnp	dNotEqualsBop2
	dec	ebx
dNotEqualsBop2:
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================
	
entry dGreaterThan0Bop
	fldz
	jmp	dGreaterThanBop1
	
entry dGreaterThanBop
	fld	QWORD[rpsp]
	add	rpsp, 8
dGreaterThanBop1:
	fld	QWORD[rpsp]
	add	rpsp, 4
	xor	ebx, ebx
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 41h
	jne	dGreaterThanBop2
	dec	ebx
dGreaterThanBop2:
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================
	
entry dGreaterEquals0Bop
	fldz
	jmp	dGreaterEqualsBop1
	
entry dGreaterEqualsBop
	fld	QWORD[rpsp]
	add	rpsp, 8
dGreaterEqualsBop1:
	fld	QWORD[rpsp]
	add	rpsp, 4
	xor	ebx, ebx
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 1
	jne	dGreaterEqualsBop2
	dec	ebx
dGreaterEqualsBop2:
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================
	
entry dLessThan0Bop
	fldz
	jmp	dLessThanBop1
	
entry dLessThanBop
	fld	QWORD[rpsp]
	add	rpsp, 8
dLessThanBop1:
	fld	QWORD[rpsp]
	add	rpsp, 4
	xor	ebx, ebx
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 5
	jp	dLessThanBop2
	dec	ebx
dLessThanBop2:
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================
	
entry dLessEquals0Bop
	fldz
	jmp	dLessEqualsBop1
	
entry dLessEqualsBop
	fld	QWORD[rpsp]
	add	rpsp, 8
dLessEqualsBop1:
	fld	QWORD[rpsp]
	add	rpsp, 4
	xor	ebx, ebx
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 41h
	jp	dLessEqualsBop2
	dec	ebx
dLessEqualsBop2:
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================
	
entry dWithinBop
	fld	QWORD[rpsp+8]
	fld	QWORD[rpsp+16]
	xor	ebx, ebx
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 41h
	jne	dWithinBop2
	fld	QWORD[rpsp]
	fld	QWORD[rpsp+16]
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 5
	jp	dWithinBop2
	dec	ebx
dWithinBop2:
	add	rpsp, 20
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================
	
entry dMinBop
	fld	QWORD[rpsp]
	fld	QWORD[rpsp+8]
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 41h
	jne	dMinBop2
	mov	eax, [rpsp]
	mov	ebx, [rpsp+4]
	mov	[rpsp+8], eax
	mov	[rpsp+12], ebx
dMinBop2:
	add	rpsp, 8
	jmp	rnext
	
;========================================
	
entry dMaxBop
	fld	QWORD[rpsp]
	fld	QWORD[rpsp+8]
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 5
	jp	dMaxBop2
	mov	eax, [rpsp]
	mov	ebx, [rpsp+4]
	mov	[rpsp+8], eax
	mov	[rpsp+12], ebx
dMaxBop2:
	add	rpsp, 8
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
    sub esp, 4      ; 16-byte align for mac
	push	rpsp
	push	rip
	mov	eax, [rpsp+4]
	push	eax
	mov	eax, [rpsp]
	push	eax
	mov	eax, [rpsp+12]
	push	eax
	mov	eax, [rpsp+8]
	push	eax
	xcall	atan2
	add	esp, 16
	pop	rip
	pop	rpsp
    add esp, 4
	add	rpsp,8
	fstp	QWORD[rpsp]
	jmp	rnext
	
;========================================
	
entry fatan2Bop
    sub esp, 12     ; 16-byte align for mac
	push	rpsp
	push	rip
	mov	eax, [rpsp]
	push	eax
	mov	eax, [rpsp+4]
	push	eax
	xcall	atan2f
	add	esp, 8
	pop	rip
	pop	rpsp
    add esp, 12
	add	rpsp, 4
	fstp	DWORD[rpsp]
	jmp	rnext
	
;========================================
	
entry dpowBop
	; a^x
    sub esp, 4      ; 16-byte align for mac
	push	rpsp
	push	rip
	; push x
	mov	eax, [rpsp+4]
	push	eax
	mov	eax, [rpsp]
	push	eax
	; push a
	mov	eax, [rpsp+12]
	push	eax
	mov	eax, [rpsp+8]
	push	eax
	xcall	pow
	add	esp, 16
	pop	rip
	pop	rpsp
    add esp, 4
	add	rpsp, 8
	fstp	QWORD[rpsp]
	jmp	rnext
	
;========================================
	
entry fpowBop
	; a^x
    sub esp, 12     ; 16-byte align for mac
	push	rpsp
	push	rip
	; push x
	mov	eax, [rpsp]
	push	eax
	; push a
	mov	eax, [rpsp+4]
	push	eax
	xcall	powf
	add	esp, 8
	pop	rip
	pop	rpsp
    add esp, 12
	add	rpsp, 4
	fstp	DWORD[rpsp]
	jmp	rnext
	
;========================================

entry dabsBop
	fld	QWORD[rpsp]
	fabs
	fstp	QWORD[rpsp]
	jmp	rnext
	
;========================================

entry fabsBop
	fld	DWORD[rpsp]
	fabs
	fstp	DWORD[rpsp]
	jmp	rnext
	
;========================================

entry dldexpBop
	; ldexp( a, n )
    sub esp, 8      ; 16-byte align for mac
	push	rpsp
	push	rip
	; TOS is n (int), a (double)
	; get arg n
	mov	eax, [rpsp]
	push	eax
	; get arg a
	mov	eax, [rpsp+8]
	push	eax
	mov	eax, [rpsp+4]
	push	eax
	xcall	ldexp
	add	esp, 12
	pop	rip
	pop	rpsp
    add esp, 8
	add	rpsp, 4
	fstp	QWORD[rpsp]
	jmp	rnext
	
;========================================

entry fldexpBop
	; ldexpf( a, n )
    sub esp, 12     ; 16-byte align for mac
	push	rpsp
	push	rip
	; TOS is n (int), a (float)
	; get arg n
	mov	eax, [rpsp]
	push	eax
	; get arg a
	mov	eax, [rpsp+4]
	push	eax
	xcall	ldexpf
	add	esp, 8
	pop	rip
	pop	rpsp
    add esp, 12
	add	rpsp, 4
	fstp	DWORD[rpsp]
	jmp	rnext
	
;========================================

entry dfrexpBop
	; frexp( a, ptrToIntExponentReturn )
	sub	rpsp, 4
    sub esp, 8      ; 16-byte align for mac
	push	rpsp
	push	rip
	; TOS is a (double)
	; we return TOS: nInt aFrac
	; alloc nInt
	push	rpsp
	; get arg a
	mov	eax, [rpsp+8]
	push	eax
	mov	eax, [rpsp+4]
	push	eax
	xcall	frexp
	add	esp, 12
	pop	rip
	pop	rpsp
    add esp, 8
	fstp	QWORD[rpsp+4]
	jmp	rnext
	
;========================================

entry ffrexpBop
	; frexpf( a, ptrToIntExponentReturn )
    sub esp, 12     ; 16-byte align for mac
	; get arg a
	mov	eax, [rpsp]
	sub	rpsp, 4
	push	rpsp
	push	rip
	; TOS is a (float)
	; we return TOS: nInt aFrac
	; alloc nInt
	push	rpsp
	push	eax
	xcall	frexpf
	add	esp, 8
	pop	rip
	pop	rpsp
    add esp, 12
	fstp	DWORD[rpsp+4]
	jmp	rnext
	
;========================================

entry dmodfBop
	; modf( a, ptrToDoubleWholeReturn )
    sub esp, 8      ; 16-byte align for mac
	mov	eax, rpsp
	sub	rpsp, 8
	push	rpsp
	push	rip
	; TOS is a (double)
	; we return TOS: bFrac aWhole
	; alloc nInt
	push	eax
	; get arg a
	mov	eax, [rpsp+12]
	push	eax
	mov	eax, [rpsp+8]
	push	eax
	xcall	modf
	add	esp, 12
	pop	rip
	pop	rpsp
    add esp, 8
	fstp	QWORD[rpsp]
	jmp	rnext
	
;========================================

entry fmodfBop
	; modf( a, ptrToFloatWholeReturn )
    sub esp, 12     ; 16-byte align for mac
	mov	eax, rpsp
	sub	rpsp, 4
	push	rpsp
	push	rip
	; TOS is a (float)
	; we return TOS: bFrac aWhole
	; alloc nInt
	push	eax
	; get arg a
	mov	eax, [rpsp+4]
	push	eax
	xcall	modff
	add	esp, 8
	pop	rip
	pop	rpsp
    add esp, 12
	fstp	DWORD[rpsp]
	jmp	rnext
	
;========================================

entry dfmodBop
    sub esp, 4      ; 16-byte align for mac
	push	rpsp
	push	rip
	mov	eax, [rpsp+4]
	push	eax
	mov	eax, [rpsp]
	push	eax
	mov	eax, [rpsp+12]
	push	eax
	mov	eax, [rpsp+8]
	push	eax
	xcall	fmod
	add	esp, 16
	pop	rip
	pop	rpsp
    add esp, 4
	add	rpsp, 8
	fstp	QWORD[rpsp]
	jmp	rnext
	
;========================================

entry ffmodBop
    sub esp, 12     ; 16-byte align for mac
	push	rpsp
	push	rip
	mov	eax, [rpsp]
	push	eax
	mov	eax, [rpsp+4]
	push	eax
	xcall	fmodf
	add	esp, 8
	pop	rip
	pop	rpsp
    add esp, 12
	add	rpsp, 4
	fstp	DWORD[rpsp]
	jmp	rnext
	
;========================================

entry lplusBop
	mov     ebx, [rpsp+4]
	mov     eax, [rpsp]
	add     rpsp, 8
	add     ebx, [rpsp+4]
	adc     eax, [rpsp]
	mov     [rpsp+4], ebx
	mov     [rpsp], eax
	jmp     rnext

;========================================

entry lminusBop
	mov     ebx, [rpsp+12]
	mov     eax, [rpsp+8]
	sub     ebx, [rpsp+4]
	sbb     eax, [rpsp]
	add     rpsp, 8
	mov     [rpsp+4], ebx
	mov     [rpsp], eax
	jmp     rnext

;========================================

entry ltimesBop
	; based on http://stackoverflow.com/questions/1131833/how-do-you-multiply-two-64-bit-numbers-in-x86-assembler
	push	rnext
	push	rip
	; rpsp always holds hi 32 bits of mul result
	mov	ebx, rpsp	; so we will use ebx for the forth stack ptr
	; TOS: bhi ebx   blo ebx+4   ahi ebx+8   alo ebx+12
	xor	ecx, ecx	; ecx holds sign flag
	mov	eax, [ebx]

	or	eax, eax
	jge	ltimes1
	; b is negative
	not	ecx
	mov	rip, [ebx+4]
	not	rip
	not	eax
	add	rip, 1
	adc	eax, 0
	mov	[ebx+4], rip
	mov	[ebx], eax
ltimes1:

	mov	eax, [ebx+8]
	mov	rip, [ebx+12]
	or	eax, eax
	jge	ltimes2
	; a is negative
	not	ecx
	not	rip
	not	eax
	add	rip, 1
	adc	eax, 0
	mov	[ebx+12], rip
	mov	[ebx+8], eax
ltimes2:
  
	; alo(rip) * blo
	mov	eax, [ebx+4]
	mul	rip				; rpsp is hipart, eax is final lopart
	mov	rnext, rpsp		; rnext is hipart accumulator
  
	mov	rip, [ebx+12]	; rip = alo
	mov	[ebx+12], eax	; ebx+12 = final lopart

	; alo * bhi
	mov	eax, [ebx]		; eax = bhi
	mul	rip
	add	rnext, eax
  
	; ahi * blo
	mov	rip, [ebx+8]	; rip = ahi
	mov	eax, [ebx+4]	; eax = blo
	mul	rip
	add	rnext, eax		; rnext = hiResult
  
	; invert result if needed
	or	ecx, ecx
	jge	ltimes3
	mov	eax, [ebx+12]	; eax = loResult
	not	eax
	not	rnext
	add	eax, 1
	adc	rnext, 0
	mov	[ebx+12], eax
ltimes3:

	mov	[ebx+8], rnext

	add	ebx, 8
	mov	rpsp, ebx
	pop	rip
	pop	rnext

	jmp     rnext

;========================================

entry umtimesBop
	mov	eax, [rpsp]
	mov	ebx, [rpsp+4]
	push rpsp
	mul	ebx		; result hiword in rpsp, loword in eax
	mov	ebx, rpsp
	pop	rpsp
	mov	[rpsp+4], eax
	mov	[rpsp], ebx
	jmp	rnext

;========================================

entry mtimesBop
	mov	eax, [rpsp]
	mov	ebx, [rpsp+4]
	push rpsp
	imul	ebx		; result hiword in rpsp, loword in eax
	mov	ebx, rpsp
	pop	rpsp
	mov	[rpsp+4], eax
	mov	[rpsp], ebx
	jmp	rnext

;========================================

entry i2fBop
	fild	DWORD[rpsp]
	fstp	DWORD[rpsp]
	jmp	rnext	

;========================================

entry i2dBop
	fild	DWORD[rpsp]
	sub	rpsp, 4
	fstp	QWORD[rpsp]
	jmp	rnext	

;========================================

entry f2iBop
	sub	esp, 4
	fwait
	fnstcw	WORD[esp]
	fwait
	mov	ax, WORD[esp]
	mov	WORD[esp + 2], ax	; save copy for restoring when done
	or	ax, 0C00h		; set both rounding control bits
	mov	WORD[esp], ax
	fldcw	WORD[esp]
    fld     DWORD[rpsp]
    fistp   DWORD[rpsp]
	fldcw	WORD[esp + 2]
	add	esp, 4
	jmp	rnext

;========================================

entry f2dBop
	fld	DWORD[rpsp]
	sub	rpsp, 4
	fstp	QWORD[rpsp]
	jmp	rnext
		
;========================================

entry d2iBop
	sub	esp, 4
	fwait
	fnstcw	WORD[esp]
	fwait
	mov	ax, WORD[esp]
	mov	WORD[esp + 2], ax	; save copy for restoring when done
	or	ax, 0C00h		; set both rounding control bits
	mov	WORD[esp], ax
	fldcw	WORD[esp]
    fld     QWORD[rpsp]
    add rpsp, 4
    fistp   DWORD[rpsp]
	fldcw	WORD[esp + 2]
	add	esp, 4
	jmp	rnext

;========================================

entry d2fBop
	fld	QWORD[rpsp]
	add	rpsp, 4
	fstp	DWORD[rpsp]
	jmp	rnext

;========================================

entry doExitBop
	; check param stack
	mov	ebx, [rcore + FCore.STPtr]
	cmp	ebx, rpsp
	jl	doExitBop2
	; check return stack
	mov	eax, [rcore + FCore.RPtr]
	mov	ebx, [rcore + FCore.RTPtr]
	cmp	ebx, eax
	jle	doExitBop1
	mov	rip, [eax]
	add	eax, 4
	mov	[rcore + FCore.RPtr], eax
	test	rip, rip
	jz doneBop
	jmp	rnext

doExitBop1:
	mov	eax, kForthErrorReturnStackUnderflow
	jmp	interpLoopErrorExit
	
doExitBop2:
	mov	eax, kForthErrorParamStackUnderflow
	jmp	interpLoopErrorExit
	
;========================================

entry doExitLBop
    ; rstack: local_var_storage oldFP oldIP
    ; FP points to oldFP
	; check param stack
	mov	ebx, [rcore + FCore.STPtr]
	cmp	ebx, rpsp
	jl	doExitBop2
	mov	eax, [rcore + FCore.FPtr]
	mov	rip, [eax]
	mov	[rcore + FCore.FPtr], rip
	add	eax, 4
	; check return stack
	mov	ebx, [rcore + FCore.RTPtr]
	cmp	ebx, eax
	jle	doExitBop1
	mov	rip, [eax]
	add	eax, 4
	mov	[rcore + FCore.RPtr], eax
	test	rip, rip
	jz doneBop
	jmp	rnext
	
;========================================


entry doExitMBop
    ; rstack: oldIP oldTP
	; check param stack
	mov	ebx, [rcore + FCore.STPtr]
	cmp	ebx, rpsp
	jl	doExitBop2
	mov	eax, [rcore + FCore.RPtr]
	mov	ebx, [rcore + FCore.RTPtr]
	add	eax, 8
	; check return stack
	cmp	ebx, eax
	jl	doExitBop1
	mov	[rcore + FCore.RPtr], eax
	mov	rip, [eax-8]	; IP = oldIP
	mov	ebx, [eax-4]
	mov	[rcore + FCore.TPtr], ebx
	test	rip, rip
	jz doneBop
	jmp	rnext

;========================================

entry doExitMLBop
    ; rstack: local_var_storage oldFP oldIP oldTP
    ; FP points to oldFP
	; check param stack
	mov	ebx, [rcore + FCore.STPtr]
	cmp	ebx, rpsp
	jl	doExitBop2
	mov	eax, [rcore + FCore.FPtr]
	mov	rip, [eax]
	mov	[rcore + FCore.FPtr], rip
	add	eax, 12
	; check return stack
	mov	ebx, [rcore + FCore.RTPtr]
	cmp	ebx, eax
	jl	doExitBop1
	mov	[rcore + FCore.RPtr], eax
	mov	rip, [eax-8]	; IP = oldIP
	mov	ebx, [eax-4]
	mov	[rcore + FCore.TPtr], ebx
	test	rip, rip
	jz doneBop
	jmp	rnext
	
;========================================

entry callBop
	; rpush current IP
	mov	eax, [rcore + FCore.RPtr]
	sub	eax, 4
	mov	[eax], rip
	mov	[rcore + FCore.RPtr], eax
	; pop new IP
	mov	rip, [rpsp]
	add	rpsp, 4
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
; TOS+4 is end-index
; the op right after this one should be a branch just past end of loop (used by leave)
; 
entry doDoBop
	mov	ebx, [rcore + FCore.RPtr]
	sub	ebx, 12
	mov	[rcore + FCore.RPtr], ebx
	; @RP-2 holds top-of-loop-IP
	add	rip, 4    ; skip over loop exit branch right after this op
	mov	[ebx+8], rip
	; @RP-1 holds end-index
	mov	eax, [rpsp+4]
	mov	[ebx+4], eax
	; @RP holds current-index
	mov	eax, [rpsp]
	mov	[ebx], eax
	add	rpsp, 8
	jmp	rnext
	
;========================================
;
; TOS is start-index
; TOS+4 is end-index
; the op right after this one should be a branch just past end of loop (used by leave)
; 
entry doCheckDoBop
	mov	eax, [rpsp]		; eax is start index
	mov	ecx, [rpsp+4]	; ecx is end index
	add	rpsp, 8
	cmp	eax,ecx
	jge	doCheckDoBop1
	
	mov	ebx, [rcore + FCore.RPtr]
	sub	ebx, 12
	mov	[rcore + FCore.RPtr], ebx
	; @RP-2 holds top-of-loop-IP
	add	rip, 4    ; skip over loop exit branch right after this op
	mov	[ebx+8], rip
	; @RP-1 holds end-index
	mov	[ebx+4], ecx
	; @RP holds current-index
	mov	[ebx], eax
doCheckDoBop1:
	jmp	rnext
	
;========================================

entry doLoopBop
	mov	ebx, [rcore + FCore.RPtr]
	mov	eax, [ebx]
	inc	eax
	cmp	eax, [ebx+4]
	jge	doLoopBop1	; jump if done
	mov	[ebx], eax
	mov	rip, [ebx+8]
	jmp	rnext

doLoopBop1:
	add	ebx,12
	mov	[rcore + FCore.RPtr], ebx
	jmp	rnext
	
;========================================

entry doLoopNBop
	mov	ebx, [rcore + FCore.RPtr]	; rcore is RP
	mov	eax, [rpsp]		; pop N into eax
	add	rpsp, 4
	or	eax, eax
	jl	doLoopNBop1		; branch if increment negative
	add	eax, [ebx]		; eax is new i
	cmp	eax, [ebx+4]
	jge	doLoopBop1		; jump if done
	mov	[ebx], eax		; update i
	mov	rip, [ebx+8]		; branch to top of loop
	jmp	rnext

doLoopNBop1:
	add	eax, [ebx]
	cmp	eax, [ebx+4]
	jl	doLoopBop1		; jump if done
	mov	[ebx], eax		; ipdate i
	mov	rip, [ebx+8]		; branch to top of loop
	jmp	rnext
	
;========================================

entry iBop
	mov	eax, [rcore + FCore.RPtr]
	mov	ebx, [eax]
	sub	rpsp,4
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================

entry jBop
	mov	eax, [rcore + FCore.RPtr]
	mov	ebx, [eax+12]
	sub	rpsp,4
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================

entry unloopBop
	mov	eax, [rcore + FCore.RPtr]
	add	eax, 12
	mov	[rcore + FCore.RPtr], eax
	jmp	rnext
	
;========================================

entry leaveBop
	mov	eax, [rcore + FCore.RPtr]
	; point IP at the branch instruction which is just before top of loop
	mov	rip, [eax+8]
	sub	rip, 4
	; drop current index, end index, top-of-loop-IP
	add	eax, 12
	mov	[rcore + FCore.RPtr], eax
	jmp	rnext
	
;========================================

entry orBop
	mov	eax, [rpsp]
	add	rpsp, 4
	or	[rpsp], eax
	jmp	rnext
	
;========================================

entry andBop
	mov	eax, [rpsp]
	add	rpsp, 4
	and	[rpsp], eax
	jmp	rnext
	
;========================================

entry xorBop
	mov	eax, [rpsp]
	add	rpsp, 4
	xor	[rpsp], eax
	jmp	rnext
	
;========================================

entry invertBop
	mov	eax,0FFFFFFFFh
	xor	[rpsp], eax
	jmp	rnext
	
;========================================

entry lshiftBop
	mov	ecx, [rpsp]
	add	rpsp, 4
	mov	ebx, [rpsp]
	shl	ebx, cl
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================

entry lshift64Bop
	mov	ecx, [rpsp]
	add	rpsp, 4
	mov	ebx, [rpsp]
	mov	eax, [rpsp+4]
	shld	ebx, eax, cl
	shl	eax, cl
	mov	[rpsp], ebx
	mov	[rpsp+4], eax
	jmp	rnext
	
;========================================

entry arshiftBop
	mov	ecx, [rpsp]
	add	rpsp, 4
	mov	ebx, [rpsp]
	sar	ebx, cl
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================

entry rshiftBop
	mov	ecx, [rpsp]
	add	rpsp, 4
	mov	ebx, [rpsp]
	shr	ebx, cl
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================

entry rshift64Bop
	mov	ecx, [rpsp]
	add	rpsp, 4
	mov	ebx, [rpsp]
	mov	eax, [rpsp+4]
	shrd	eax, ebx, cl
	shr	ebx, cl
	mov	[rpsp], ebx
	mov	[rpsp+4], eax
	jmp	rnext
	
;========================================

entry rotateBop
	mov	ecx, [rpsp]
	add	rpsp, 4
	mov	ebx, [rpsp]
	and	cl, 01Fh
	rol	ebx, cl
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================

entry rotate64Bop
	mov	ecx, [rpsp]
	mov	ebx, [rpsp+4]
	mov	eax, [rpsp+8]
	push	rpsp
	; if rotate count is >31, swap lo & hi parts
	bt ecx, 5
	jnc	rotate64Bop_1
	xchg	eax, ebx
rotate64Bop_1:
	and	cl, 01Fh
	mov	rpsp, ebx
	shld	ebx, eax, cl
	xchg	rpsp, ebx
	shld	eax, ebx, cl
	mov	ebx, rpsp
	pop	rpsp
	add	rpsp, 4
	mov	[rpsp], ebx
	mov	[rpsp+4], eax
	jmp	rnext

;========================================
entry reverseBop
    ; Knuth's algorithm
    ; a = (a << 15) | (a >> 17);
	mov	eax, [rpsp]
	rol	eax, 15
    ; b = (a ^ (a >> 10)) & 0x003f801f;
	mov	ebx, eax
	shr	ebx, 10
	xor	ebx, eax
	and	ebx, 03F801Fh
    ; a = (b + (b << 10)) ^ a;
	mov	ecx, ebx
	shl	ecx, 10
	add	ecx, ebx
	xor	eax, ecx
    ; b = (a ^ (a >> 4)) & 0x0e038421;
	mov	ebx, eax
	shr	ebx, 4
	xor	ebx, eax
	and	ebx, 0E038421h
    ; a = (b + (b << 4)) ^ a;
	mov	ecx, ebx
	shl	ecx, 4
	add	ecx, ebx
	xor	eax, ecx
    ; b = (a ^ (a >> 2)) & 0x22488842;
	mov	ebx, eax
	shr	ebx, 2
	xor	ebx, eax
	and	ebx, 022488842h
    ; a = (b + (b << 2)) ^ a;
	mov	ecx, ebx
	shl ecx, 2
	add	ebx, ecx
	xor	eax, ebx
	mov	[rpsp], eax
	jmp rnext

;========================================

entry countLeadingZerosBop
	mov	eax, [rpsp]
    ; NASM on Mac wouldn't do tzcnt or lzcnt, so I had to use bsr
%ifdef MACOSX
    mov ebx, 32
    or  eax, eax
    jz  clz1
    bsr ecx, eax
    mov ebx, 31
    sub ebx, ecx
clz1:
%else
    lzcnt    ebx, eax
%endif
	mov	[rpsp], ebx
	jmp rnext

;========================================

entry countTrailingZerosBop
	mov	eax, [rpsp]
    ; NASM on Mac wouldn't do tzcnt or lzcnt, so I had to use bsf
%ifdef MACOSX
    mov ebx, 32
    or  eax, eax
    jz  ctz1
    bsf ebx, eax
ctz1:
%else
    tzcnt	ebx, eax
%endif
	mov	[rpsp], ebx
	jmp rnext

;========================================

entry archX86Bop
entry trueBop
	mov	eax,0FFFFFFFFh
	sub	rpsp, 4
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry archARMBop
entry falseBop
	xor	eax, eax
	sub	rpsp, 4
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry nullBop
	xor	eax, eax
	sub	rpsp, 4
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry dnullBop
	xor	eax, eax
	sub	rpsp, 8
	mov	[rpsp+4], eax
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry equals0Bop
	xor	ebx, ebx
	jmp	equalsBop1
	
;========================================

entry equalsBop
	mov	ebx, [rpsp]
	add	rpsp, 4
equalsBop1:
	xor	eax, eax
	cmp	ebx, [rpsp]
	jnz	equalsBop2
	dec	eax
equalsBop2:
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry notEquals0Bop
	xor	ebx, ebx
	jmp	notEqualsBop1
	
;========================================

entry notEqualsBop
	mov	ebx, [rpsp]
	add	rpsp, 4
notEqualsBop1:
	xor	eax, eax
	cmp	ebx, [rpsp]
	jz	notEqualsBop2
	dec	eax
notEqualsBop2:
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry greaterThan0Bop
	xor	ebx, ebx
	jmp	gtBop1
	
;========================================

entry greaterThanBop
	mov	ebx, [rpsp]		; ebx = b
	add	rpsp, 4
gtBop1:
	xor	eax, eax
	cmp	[rpsp], ebx
	jle	gtBop2
	dec	eax
gtBop2:
	mov	[rpsp], eax
	jmp	rnext

;========================================

entry greaterEquals0Bop
	xor	ebx, ebx
	jmp	geBop1
	
;========================================

entry greaterEqualsBop
	mov	ebx, [rpsp]
	add	rpsp, 4
geBop1:
	xor	eax, eax
	cmp	[rpsp], ebx
	jl	geBop2
	dec	eax
geBop2:
	mov	[rpsp], eax
	jmp	rnext
	

;========================================

entry lessThan0Bop
	xor	ebx, ebx
	jmp	ltBop1
	
;========================================

entry lessThanBop
	mov	ebx, [rpsp]
	add	rpsp, 4
ltBop1:
	xor	eax, eax
	cmp	[rpsp], ebx
	jge	ltBop2
	dec	eax
ltBop2:
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry lessEquals0Bop
	xor	ebx, ebx
	jmp	leBop1
	
;========================================

entry lessEqualsBop
	mov	ebx, [rpsp]
	add	rpsp, 4
leBop1:
	xor	eax, eax
	cmp	[rpsp], ebx
	jg	leBop2
	dec	eax
leBop2:
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry unsignedGreaterThanBop
	mov	ebx, [rpsp]
	add	rpsp, 4
ugtBop1:
	xor	eax, eax
	cmp	[rpsp], ebx
	jbe	ugtBop2
	dec	eax
ugtBop2:
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry unsignedLessThanBop
	mov	ebx, [rpsp]
	add	rpsp, 4
ultBop1:
	xor	eax, eax
	cmp	[rpsp], ebx
	jae	ultBop2
	dec	eax
ultBop2:
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry withinBop
	; tos: hiLimit loLimit value
	xor	eax, eax
	mov	ebx, [rpsp+8]	; ebx = value
withinBop1:
	cmp	[rpsp], ebx
	jle	withinFail
	cmp	[rpsp+4], ebx
	jg	withinFail
	dec	eax
withinFail:
	add rpsp, 8
	mov	[rpsp], eax		
	jmp	rnext
	
;========================================

entry clampBop
	; tos: hiLimit loLimit value
	mov	ebx, [rpsp+8]	; ebx = value
	mov	eax, [rpsp]		; eax = hiLimit
	cmp	eax, ebx
	jle clampFail
	mov	eax, [rpsp+4]
	cmp	eax, ebx
	jg	clampFail
	mov	eax, ebx		; value was within range
clampFail:
	add rpsp, 8
	mov	[rpsp], eax		
	jmp	rnext
	
;========================================

entry minBop
	mov	ebx, [rpsp]
	add	rpsp, 4
	cmp	[rpsp], ebx
	jl	minBop1
	mov	[rpsp], ebx
minBop1:
	jmp	rnext
	
;========================================

entry maxBop
	mov	ebx, [rpsp]
	add	rpsp, 4
	cmp	[rpsp], ebx
	jg	maxBop1
	mov	[rpsp], ebx
maxBop1:
	jmp	rnext
	
	
;========================================

entry lcmpBop
	xor	eax, eax
	mov ebx, [rpsp+8]
	sub	ebx, [rpsp]
	mov ebx, [rpsp+12]
	sbb	ebx, [rpsp+4]
	jz	lcmpBop3
	jl	lcmpBop2
	add	eax, 2
lcmpBop2:
	dec	eax
lcmpBop3:
	add rpsp, 12
	mov	[rpsp], eax
	jmp	rnext

;========================================

entry ulcmpBop
	xor	eax, eax
	mov ebx, [rpsp+8]
	sub	ebx, [rpsp]
	mov ebx, [rpsp+12]
	sbb	ebx, [rpsp+4]
	jz	ulcmpBop3
	jb	ulcmpBop2
	add	eax, 2
ulcmpBop2:
	dec	eax
ulcmpBop3:
	add rpsp, 12
	mov	[rpsp], eax
	jmp	rnext

;========================================

entry lEqualsBop
	xor	eax, eax
	mov ebx, [rpsp+8]
	mov ecx, [rpsp+12]
	sub	ebx, [rpsp]
	sbb	ecx, [rpsp+4]
	jnz	leqBop1
	dec	eax
leqBop1:
	add rpsp, 12
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry lEquals0Bop
	xor eax, eax
	mov ebx, [rpsp+4]
	or ebx, [rpsp]
	jnz	leq0Bop1
	dec	eax
leq0Bop1:
	add rpsp, 4
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry lNotEqualsBop
	xor	eax, eax
	mov ebx, [rpsp+8]
	sub	ebx, [rpsp]
	mov ebx, [rpsp+12]
	sbb	ebx, [rpsp+4]
	jz	lneqBop1
	dec	eax
lneqBop1:
	add rpsp, 12
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry lNotEquals0Bop
	xor	eax, eax
	mov ebx, [rpsp+4]
	or ebx, [rpsp]
	jz	lneq0Bop1
	dec	eax
lneq0Bop1:
	add rpsp, 4
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry lGreaterThanBop
	xor	eax, eax
	mov ebx, [rpsp+8]
	sub	ebx, [rpsp]
	mov ebx, [rpsp+12]
	sbb	ebx, [rpsp+4]
	jle	lgtBop
	dec	eax
lgtBop:
	add rpsp, 12
	mov	[rpsp], eax
	jmp	rnext

;========================================

entry lGreaterThan0Bop
	xor	eax, eax
	cmp eax, [rpsp]
	jl lgt0Bop2		; if hiword is negative, return false
	jg lgt0Bop1		; if hiword is positive, return true
	; hiword was zero, need to check low word (unsigned)
	cmp eax, [rpsp+4]
	jz lgt0Bop2		; loword also 0, return false
lgt0Bop1:
	dec	eax
lgt0Bop2:
	add	rpsp, 4
	mov	[rpsp], eax
	jmp	rnext

;========================================

entry lGreaterEqualsBop
	xor	eax, eax
	mov ebx, [rpsp+8]
	sub	ebx, [rpsp]
	mov ebx, [rpsp+12]
	sbb	ebx, [rpsp+4]
	jl	lgeBop
	dec	eax
lgeBop:
	add rpsp, 12
	mov	[rpsp], eax
	jmp	rnext

;========================================

entry lGreaterEquals0Bop
	xor	eax, eax
	cmp eax, [rpsp]
	jl lge0Bop		; if hiword is negative, return false
	dec	eax
lge0Bop:
	add	rpsp, 4
	mov	[rpsp], eax
	jmp	rnext

;========================================

entry lLessThanBop
	xor	eax, eax
	mov ebx, [rpsp+8]
	sub	ebx, [rpsp]
	mov ebx, [rpsp+12]
	sbb	ebx, [rpsp+4]
	jge	lltBop
	dec	eax
lltBop:
	add rpsp, 12
	mov	[rpsp], eax
	jmp	rnext

;========================================

entry lLessThan0Bop
	xor	eax, eax
	cmp eax, [rpsp]
	jge lt0Bop		; if hiword is negative, return true
	dec	eax
lt0Bop:
	add	rpsp, 4
	mov	[rpsp], eax
	jmp	rnext

;========================================

entry lLessEqualsBop
	xor	eax, eax
	mov ebx, [rpsp+8]
	sub	ebx, [rpsp]
	mov ebx, [rpsp+12]
	sbb	ebx, [rpsp+4]
	jg	lleBop
	dec	eax
lleBop:
	add rpsp, 12
	mov	[rpsp], eax
	jmp	rnext

;========================================

entry lLessEquals0Bop
	xor	eax, eax
	cmp eax, [rpsp]
	jg lle1Bop		; if hiword is positive, return false
	jl lle0Bop		; if hiword is negative, return true
	; hiword was 0, need to test loword
	cmp eax, [rpsp+4]
	jnz lle1Bop		; if loword is positive, return false
lle0Bop:
	dec	eax
lle1Bop:
	add	rpsp, 4
	mov	[rpsp], eax
	jmp	rnext

;========================================

entry icmpBop
	mov	ebx, [rpsp]		; ebx = b
	add	rpsp, 4
	xor	eax, eax
	cmp	[rpsp], ebx
	jz	icmpBop3
	jl	icmpBop2
	add	eax, 2
icmpBop2:
	dec	eax
icmpBop3:
	mov	[rpsp], eax
	jmp	rnext

;========================================

entry uicmpBop
	mov	ebx, [rpsp]
	add	rpsp, 4
	xor	eax, eax
	cmp	[rpsp], ebx
	jz	uicmpBop3
	jb	uicmpBop2
	add	eax, 2
uicmpBop2:
	dec	eax
uicmpBop3:
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry rpushBop
	mov	ebx, [rpsp]
	add	rpsp, 4
	mov	eax, [rcore + FCore.RPtr]
	sub	eax, 4
	mov	[rcore + FCore.RPtr], eax
	mov	[eax], ebx
	jmp	rnext
	
;========================================

entry rpopBop
	mov	eax, [rcore + FCore.RPtr]
	mov	ebx, [eax]
	add	eax, 4
	mov	[rcore + FCore.RPtr], eax
	sub	rpsp, 4
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================

entry rpeekBop
	mov	eax, [rcore + FCore.RPtr]
	mov	ebx, [eax]
	sub	rpsp, 4
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================

entry rdropBop
	mov	eax, [rcore + FCore.RPtr]
	add	eax, 4
	mov	[rcore + FCore.RPtr], eax
	jmp	rnext
	
;========================================

entry rpBop
	lea	eax, [rcore + FCore.RPtr]
	jmp	intEntry
	
;========================================

entry r0Bop
	mov	eax, [rcore + FCore.RTPtr]
	sub	rpsp, 4
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry dupBop
	mov	eax, [rpsp]
	sub	rpsp, 4
	mov	[rpsp], eax
	jmp	rnext

;========================================

entry checkDupBop
	mov	eax, [rpsp]
	or	eax, eax
	jz	dupNon0Bop1
	sub	rpsp, 4
	mov	[rpsp], eax
dupNon0Bop1:
	jmp	rnext

;========================================

entry swapBop
	mov	eax, [rpsp]
	mov	ebx, [rpsp+4]
	mov	[rpsp], ebx
	mov	[rpsp+4], eax
	jmp	rnext
	
;========================================

entry dropBop
	add	rpsp, 4
	jmp	rnext
	
;========================================

entry overBop
	mov	eax, [rpsp+4]
	sub	rpsp, 4
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry rotBop
	mov	eax, [rpsp]		; tos[0], will go in tos[1]
	mov	ebx, [rpsp+8]	; tos[2], will go in tos[0]
	mov	[rpsp], ebx
	mov	ebx, [rpsp+4]	; tos[1], will go in tos[2]
	mov	[rpsp+8], ebx
	mov	[rpsp+4], eax
	jmp	rnext
	
;========================================

entry reverseRotBop
	mov	eax, [rpsp]		; tos[0], will go in tos[2]
	mov	ebx, [rpsp+4]	; tos[1], will go in tos[0]
	mov	[rpsp], ebx
	mov	ebx, [rpsp+8]	; tos[2], will go in tos[1]
	mov	[rpsp+4], ebx
	mov	[rpsp+8], eax
	jmp	rnext
	
;========================================

entry nipBop
	mov	eax, [rpsp]
	add	rpsp, 4
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry tuckBop
	mov	eax, [rpsp]
	mov	ebx, [rpsp+4]
	sub	rpsp, 4
	mov	[rpsp], eax
	mov	[rpsp+4], ebx
	mov	[rpsp+8], eax
	jmp	rnext
	
;========================================

entry pickBop
	mov	eax, [rpsp]
	add   eax, 1
	mov	ebx, [rpsp+eax*4]
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================

entry spBop
	; this is overkill to make sp look like other vars
	mov	ebx, [rcore + FCore.varMode]
	xor	eax, eax
	mov	[rcore + FCore.varMode], eax
	cmp	ebx, kVarSetMinus
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, [spActionTable + ebx*4]
	jmp	ebx
	
spFetch:
	mov	eax, rpsp
	sub	rpsp, 4
	mov	[rpsp], eax
	jmp	rnext

spRef:
	; returns address of SP shadow copy
	lea	eax, [rcore + FCore.SPtr]
	sub	rpsp, 4
	mov	[rpsp], eax
	jmp	rnext
	
spStore:
	mov	ebx, [rpsp]
	mov	rpsp, ebx
	jmp	rnext

spPlusStore:
	mov	eax, [rpsp]
	add	rpsp, 4
	add	rpsp, eax
	jmp	rnext

spMinusStore:
	mov	eax, [rpsp]
	add	rpsp, 4
	sub	rpsp, eax
	jmp	rnext

spActionTable:
	DD	spFetch
	DD	spFetch
	DD	spRef
	DD	spStore
	DD	spPlusStore
	DD	spMinusStore

	
;========================================

entry s0Bop
	mov	eax, [rcore + FCore.STPtr]
	sub	rpsp, 4
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry fpBop
	lea	eax, [rcore + FCore.FPtr]
	jmp	intEntry
	
;========================================

entry ipBop
	; let the common intVarAction code change the shadow copy of IP,
	; then jump back to ipFixup to copy the shadow copy of IP into IP register (rip)
	push	rnext
	mov	[rcore + FCore.IPtr], rip
	lea	eax, [rcore + FCore.IPtr]
	mov	rnext, ipFixup
	jmp	intEntry
	
entry	ipFixup	
	mov	rip, [rcore + FCore.IPtr]
	pop	rnext
	jmp	rnext
	
;========================================

entry ddupBop
	mov	eax, [rpsp]
	mov	ebx, [rpsp+4]
	sub	rpsp, 8
	mov	[rpsp], eax
	mov	[rpsp+4], ebx
	jmp	rnext
	
;========================================

entry dswapBop
	mov	eax, [rpsp]
	mov	ebx, [rpsp+8]
	mov	[rpsp+8], eax
	mov	[rpsp], ebx
	mov	eax, [rpsp+4]
	mov	ebx, [rpsp+12]
	mov	[rpsp+12], eax
	mov	[rpsp+4], ebx
	jmp	rnext
	
;========================================

entry ddropBop
	add	rpsp, 8
	jmp	rnext
	
;========================================

entry doverBop
	mov	eax, [rpsp+8]
	mov	ebx, [rpsp+12]
	sub	rpsp, 8
	mov	[rpsp], eax
	mov	[rpsp+4], ebx
	jmp	rnext
	
;========================================

entry drotBop
	mov	eax, [rpsp+20]
	mov	ebx, [rpsp+12]
	mov	[rpsp+20], ebx
	mov	ebx, [rpsp+4]
	mov	[rpsp+12], ebx
	mov	[rpsp+4], eax
	mov	eax, [rpsp+16]
	mov	ebx, [rpsp+8]
	mov	[rpsp+16], ebx
	mov	ebx, [rpsp]
	mov	[rpsp+8], ebx
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry startTupleBop
	mov	eax, [rcore + FCore.RPtr]
	sub	eax, 4
	mov	[rcore + FCore.RPtr], eax
	mov	[eax], rpsp
	jmp	rnext
	
;========================================

entry endTupleBop
	mov	eax, [rcore + FCore.RPtr]
	mov	ebx, [eax]
	add	eax, 4
	mov	[rcore + FCore.RPtr], eax
	sub	ebx, rpsp
	sub	rpsp, 4
	sar	ebx, 2
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================

entry hereBop
	mov	eax, [rcore + FCore.DictionaryPtr]
    mov	ebx, [eax + ForthMemorySection.pCurrent]
    sub rpsp, 4
    mov [rpsp], ebx
    jmp rnext

;========================================

entry dpBop
    mov	eax, [rcore + FCore.DictionaryPtr]
    lea	ebx, [eax + ForthMemorySection.pCurrent]
    sub rpsp, 4
    mov [rpsp], ebx
    jmp rnext

;========================================

entry lstoreBop
	mov	eax, [rpsp]
	mov	ebx, [rpsp+4]
	mov	[eax+4], ebx
	mov	ebx, [rpsp+8]
	mov	[eax], ebx
	add	rpsp, 12
	jmp	rnext
	
;========================================

entry lstoreNextBop
	mov	eax, [rpsp]		; eax -> dst ptr
	mov	ecx, [eax]
	mov	ebx, [rpsp+8]
	mov	[ecx], ebx
	mov	ebx, [rpsp+4]
	mov	[ecx+4], ebx
	add	ecx, 8
	mov	[eax], ecx
	add	rpsp, 12
	jmp	rnext
	
;========================================

entry lfetchBop
	mov	eax, [rpsp]
	sub	rpsp, 4
	mov	ebx, [eax+4]
	mov	[rpsp], ebx
	mov	ebx, [eax]
	mov	[rpsp+4], ebx
	jmp	rnext
	
;========================================

entry lfetchNextBop
	mov	eax, [rpsp]		; eax -> src ptr
	sub	rpsp, 4
	mov	ecx, [eax]		; ecx is src ptr
	mov	ebx, [ecx+4]
	mov	[rpsp], ebx
	mov	ebx, [ecx]
	mov	[rpsp+4], ebx
	add	ecx, 8
	mov	[eax], ecx
	jmp	rnext
	
;========================================

entry plusStoreCellBop
	mov	eax, [rpsp]
	mov ebx, [rpsp+4]
	add rpsp, 8
	add ebx, [eax]
	mov [eax], ebx
	jmp	rnext
	
;========================================

entry plusStoreAtomicCellBop
	mov	eax, [rpsp]
	add rpsp, 4
	mov	ebx, [rpsp]
	mov ecx, ebx
	lock xadd DWORD[eax], ebx
	add ebx, ecx
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================

entry istoreBop
	mov	eax, [rpsp]
	mov	ebx, [rpsp+4]
	add	rpsp, 8
	mov	[eax], ebx
	jmp	rnext
	
;========================================

entry ifetchBop
	mov	eax, [rpsp]
	mov	ebx, [eax]
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================

entry istoreNextBop
	mov	eax, [rpsp]		; eax -> dst ptr
	mov	ecx, [eax]
	mov	ebx, [rpsp+4]
	add	rpsp, 8
	mov	[ecx], ebx
	add	ecx, 4
	mov	[eax], ecx
	jmp	rnext
	
;========================================

entry ifetchNextBop
	mov	eax, [rpsp]
	mov	ecx, [eax]
	mov	ebx, [ecx]
	mov	[rpsp], ebx
	add	ecx, 4
	mov	[eax], ecx
	jmp	rnext
	
;========================================

entry ubfetchBop
	mov	eax, [rpsp]
	xor ebx, ebx
    mov bl, BYTE[eax]
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================

entry bstoreBop
	mov	eax, [rpsp]
	mov	ebx, [rpsp+4]
	add	rpsp, 8
	mov	[eax], bl
	jmp	rnext
	
;========================================

entry bfetchBop
	mov	eax, [rpsp]
	movsx ebx, BYTE[eax]
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================

entry bstoreNextBop
	mov	eax, [rpsp]		; eax -> dst ptr
	mov	ecx, [eax]
	mov	ebx, [rpsp+4]
	add	rpsp, 8
	mov	[ecx], bl
	add	ecx, 1
	mov	[eax], ecx
	jmp	rnext
	
;========================================

entry bfetchNextBop
	mov	eax, [rpsp]
	mov	ecx, [eax]
	movsx ebx, BYTE[ecx]
	mov	[rpsp], ebx
	add	ecx, 1
	mov	[eax], ecx
	jmp	rnext
	
;========================================

entry sstoreBop
	mov	eax, [rpsp]
	mov	bx, [rpsp+4]
	add	rpsp, 8
	mov	[eax], bx
	jmp	rnext
	
;========================================

entry sstoreNextBop
	mov	eax, [rpsp]		; eax -> dst ptr
	mov	ecx, [eax]
	mov	ebx, [rpsp+4]
	add	rpsp, 8
	mov	[ecx], bx
	add	ecx, 2
	mov	[eax], ecx
	jmp	rnext
	
;========================================

entry sfetchBop
	mov	eax, [rpsp]
	movsx ebx, WORD[eax]
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================

entry sfetchNextBop
	mov	eax, [rpsp]
	mov	ecx, [eax]
	movsx ebx, WORD[ecx]
	mov	[rpsp], ebx
	add	ecx, 2
	mov	[eax], ecx
	jmp	rnext
	
;========================================

entry dstoreBop
	mov	eax, [rpsp]
	mov	ebx, [rpsp+4]
	mov	[eax], ebx
	mov	ebx, [rpsp+8]
	mov	[eax+4], ebx
	add	rpsp, 12
	jmp	rnext
	
;========================================

entry dstoreNextBop
	mov	eax, [rpsp]		; eax -> dst ptr
	mov	ecx, [eax]
	mov	ebx, [rpsp+4]
	mov	[ecx], ebx
	mov	ebx, [rpsp+8]
	mov	[ecx+4], ebx
	add	ecx, 8
	mov	[eax], ecx
	add	rpsp, 12
	jmp	rnext
	
;========================================

entry dfetchBop
	mov	eax, [rpsp]
	sub	rpsp, 4
	mov	ebx, [eax]
	mov	[rpsp], ebx
	mov	ebx, [eax+4]
	mov	[rpsp+4], ebx
	jmp	rnext
	
;========================================

entry dfetchNextBop
	mov	eax, [rpsp]
	sub	rpsp, 4
	mov	ecx, [eax]
	mov	ebx, [ecx]
	mov	[rpsp], ebx
	mov	ebx, [ecx+4]
	mov	[rpsp+4], ebx
	add	ecx, 8
	mov	[eax], ecx
	jmp	rnext
	
;========================================

entry ostoreBop
	mov	eax, [rpsp]
	mov	ebx, [rpsp+4]
	mov	[eax+4], ebx
	mov	ebx, [rpsp+8]
	mov	[eax], ebx
	add	rpsp, 12
	jmp	rnext
	
;========================================

entry ostoreNextBop
	mov	eax, [rpsp]		; eax -> dst ptr
	mov	ecx, [eax]
	mov	ebx, [rpsp+4]
	mov	[ecx+4], ebx
	mov	ebx, [rpsp+8]
	mov	[ecx], ebx
	add	ecx, 8
	mov	[eax], ecx
	add	rpsp, 12
	jmp	rnext
	
;========================================

entry ofetchBop
	mov	eax, [rpsp]
	sub	rpsp, 4
	mov	ebx, [eax+4]
	mov	[rpsp], ebx
	mov	ebx, [eax]
	mov	[rpsp+4], ebx
	jmp	rnext
	
;========================================

entry ofetchNextBop
	mov	eax, [rpsp]
	sub	rpsp, 4
	mov	ecx, [eax]
	mov	ebx, [ecx+4]
	mov	[rpsp], ebx
	mov	ebx, [ecx]
	mov	[rpsp+4], ebx
	add	ecx, 8
	mov	[eax], ecx
	jmp	rnext
	
;========================================

entry moveBop
	;	TOS: nBytes dstPtr srcPtr
	push	rpsp
	push	rip
    sub esp, 8      ; 16-byte align for mac
	mov	eax, [rpsp]
	push	eax
	mov	eax, [rpsp+8]
	push	eax
	mov	eax, [rpsp+4]
	push	eax
	xcall	memmove
	add	esp, 20
	pop	rip
	pop	rpsp
	add	rpsp, 12
	jmp	rnext

;========================================

entry memcmpBop
	;	TOS: nBytes mem2Ptr mem1Ptr
	push	rpsp
	push	rip
    sub esp, 8      ; 16-byte align for mac
	mov	eax, [rpsp]
	push	eax
	mov	eax, [rpsp+4]
	push	eax
	mov	eax, [rpsp+8]
	push	eax
	xcall	memcmp
	add	esp, 20
	pop	rip
	pop	rpsp
	add	rpsp, 12
	jmp	rnext

;========================================

entry fillBop
	;	TOS: nBytes byteVal dstPtr
	push	rpsp
	push	rip
    sub esp, 8      ; 16-byte align for mac
	mov	eax, [rpsp+4]
	push	eax
	mov	eax, [rpsp]
	and	eax, 0FFh
	push	eax
	mov	eax, [rpsp+8]
	push	eax
	xcall	memset
	add	esp, 20
	pop	rip
	pop	rpsp
	add	rpsp, 12
	jmp	rnext

;========================================

entry fetchVaractionBop
	mov	eax, kVarGet
	mov	[rcore + FCore.varMode], eax
	jmp	rnext
	
;========================================

entry intoVaractionBop
	mov	eax, kVarSet
	mov	[rcore + FCore.varMode], eax
	jmp	rnext
	
;========================================

entry addToVaractionBop
	mov	eax, kVarSetPlus
	mov	[rcore + FCore.varMode], eax
	jmp	rnext
	
;========================================

entry subtractFromVaractionBop
	mov	eax, kVarSetMinus
	mov	[rcore + FCore.varMode], eax
	jmp	rnext
	
;========================================

entry oclearVaractionBop
	mov	eax, kVarClear
	mov	[rcore + FCore.varMode], eax
	jmp	rnext
		
;========================================

entry odropBop
	; TOS is object to check - if its refcount is already 0, invoke delete method
	;  otherwise do nothing
	mov	eax, [rpsp]
	add	rpsp, 4
	; TODO: how should atomic refcounts be handled here?
	mov	ebx, [eax + Object.refCount]
	or	ebx, ebx
	jnz .odrop1

	; refcount is 0, delete the object
	; push this ptr on return stack
	mov	ebx, [rcore + FCore.RPtr]
	sub	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	mov	ecx, [rcore + FCore.TPtr]
	mov	[ebx], ecx

	; set this to object to delete
	mov	[rcore + FCore.TPtr], eax

	mov	ebx, [eax]	; ebx = methods pointer
	mov	ebx, [ebx]	; ebx = method 0 (delete)

	; execute the delete method opcode which is in ebx
	mov	eax, [rcore + FCore.innerExecute]
	jmp eax

.odrop1:
	jmp rnext

;========================================

entry refVaractionBop
	mov	eax, kVarRef
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

;========================================

entry setVaropBop
	mov   eax, [rpsp]
	add   rpsp, 4
	mov	[rcore + FCore.varMode], eax
	jmp	rnext

;========================================

entry getVaropBop
	mov	eax, [rcore + FCore.varMode]
	sub   rpsp, 4
	mov   [rpsp], eax
	jmp	rnext

;========================================

entry byteVarActionBop
	mov	eax,[rpsp]
	add	rpsp, 4
	jmp	byteEntry
	
;========================================

entry ubyteVarActionBop
	mov	eax,[rpsp]
	add	rpsp, 4
	jmp	ubyteEntry
	
;========================================

entry shortVarActionBop
	mov	eax,[rpsp]
	add	rpsp, 4
	jmp	shortEntry
	
;========================================

entry ushortVarActionBop
	mov	eax,[rpsp]
	add	rpsp, 4
	jmp	ushortEntry
	
;========================================

entry intVarActionBop
	mov	eax,[rpsp]
	add	rpsp, 4
	jmp	intEntry
	
;========================================

entry floatVarActionBop
	mov	eax,[rpsp]
	add	rpsp, 4
	jmp	floatEntry
	
;========================================

entry doubleVarActionBop
	mov	eax,[rpsp]
	add	rpsp, 4
	jmp	doubleEntry
	
;========================================

entry longVarActionBop
	mov	eax,[rpsp]
	add	rpsp, 4
	jmp	longEntry
	
;========================================

entry opVarActionBop
	mov	eax,[rpsp]
	add	rpsp, 4
	jmp	opEntry
	
;========================================

entry objectVarActionBop
	mov	eax,[rpsp]
	add	rpsp, 4
	jmp	objectEntry
	
;========================================

entry stringVarActionBop
	mov	eax,[rpsp]
	add	rpsp, 4
	jmp	stringEntry
	
;========================================

entry strcpyBop
	;	TOS: srcPtr dstPtr
	push	rpsp
	push	rip
    sub esp, 12     ; 16-byte align for mac
	mov	eax, [rpsp]
	push	eax
	mov	eax, [rpsp+4]
	push	eax
	xcall	strcpy
	add	esp, 20
	pop	rip
	pop	rpsp
	add	rpsp, 8
	jmp	rnext

;========================================

entry strncpyBop
	;	TOS: maxBytes srcPtr dstPtr
	push	rpsp
	push	rip
    sub esp, 8     ; 16-byte align for mac
	mov	eax, [rpsp]
	push	eax
	mov	eax, [rpsp+4]
	push	eax
	mov	eax, [rpsp+8]
	push	eax
	xcall	strncpy
	add	esp, 20
	pop	rip
	pop	rpsp
	add	rpsp, 12
	jmp	rnext

;========================================

entry strlenBop
	mov	eax, [rpsp]
	mov ecx, eax
	xor	ebx, ebx
strlenBop1:
	mov	bl, [eax]
	test	bl, 255
	jz	strlenBop2
	add	eax, 1
	jmp	strlenBop1

strlenBop2:
	sub eax, ecx
	mov	[rpsp], eax
	jmp	rnext

;========================================

entry strcatBop
	;	TOS: srcPtr dstPtr
	push	rpsp
	push	rip
    sub esp, 12     ; 16-byte align for mac
	mov	eax, [rpsp]
	push	eax
	mov	eax, [rpsp+4]
	push	eax
	xcall	strcat
	add	esp, 20
	pop	rip
	pop	rpsp
	add	rpsp, 8
	jmp	rnext

;========================================

entry strncatBop
	;	TOS: maxBytes srcPtr dstPtr
	push	rpsp
	push	rip
    sub esp, 8     ; 16-byte align for mac
	mov	eax, [rpsp]
	push	eax
	mov	eax, [rpsp+4]
	push	eax
	mov	eax, [rpsp+8]
	push	eax
	xcall	strncat
	add	esp, 20
	pop	rip
	pop	rpsp
	add	rpsp, 12
	jmp	rnext

;========================================

entry strchrBop
	;	TOS: char strPtr
	push	rpsp
	push	rip
    sub esp, 12     ; 16-byte align for mac
	mov	eax, [rpsp]
	push	eax
	mov	eax, [rpsp+4]
	push	eax
	xcall	strchr
	add	esp, 20
	pop	rip
	pop	rpsp
	add	rpsp, 4
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry strrchrBop
	;	TOS: char strPtr
	push	rpsp
	push	rip
    sub esp, 12     ; 16-byte align for mac
	mov	eax, [rpsp]
	push	eax
	mov	eax, [rpsp+4]
	push	eax
	xcall	strrchr
	add	esp, 20
	pop	rip
	pop	rpsp
	add	rpsp, 4
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry strcmpBop
	;	TOS: ptr2 ptr1
	push	rpsp
	push	rip
    sub esp, 12     ; 16-byte align for mac
	mov	eax, [rpsp]
	push	eax
	mov	eax, [rpsp+4]
	push	eax
	xcall	strcmp
strcmp1:
	xor	ebx, ebx
	cmp	eax, ebx
	jz	strcmp3		; if strings equal, return 0
	jg	strcmp2
	sub	ebx, 2
strcmp2:
	inc	ebx
strcmp3:
	add	esp, 20
	pop	rip
	pop	rpsp
	add	rpsp, 4
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================

entry stricmpBop
	;	TOS: ptr2 ptr1
	push	rpsp
	push	rip
    sub esp, 12     ; 16-byte align for mac
	mov	eax, [rpsp]
	push	eax
	mov	eax, [rpsp+4]
	push	eax
%ifdef WIN32
    xcall	stricmp
%else
	xcall	strcasecmp
%endif
	jmp	strcmp1
	
;========================================

entry strncmpBop
	;	TOS: numChars ptr2 ptr1
	push	rpsp
	push	rip
    sub esp, 8     ; 16-byte align for mac
	mov	eax, [rpsp]
	push	eax
	mov	eax, [rpsp+4]
	push	eax
	mov	eax, [rpsp+8]
	push	eax
	xcall	strncmp
strncmp1:
	xor	ebx, ebx
	cmp	eax, ebx
	jz	strncmp3		; if strings equal, return 0
	jg	strncmp2
	sub	ebx, 2
strncmp2:
	inc	ebx
strncmp3:
	add	esp, 20
	pop	rip
	pop	rpsp
	add	rpsp, 8
	mov	[rpsp], ebx
	jmp	rnext
	
;========================================

entry strstrBop
	;	TOS: ptr2 ptr1
	push	rpsp
	push	rip
    sub esp, 12     ; 16-byte align for mac
	mov	eax, [rpsp]
	push	eax
	mov	eax, [rpsp+4]
	push	eax
	xcall	strstr
	add	esp, 20
	pop	rip
	pop	rpsp
	add	rpsp, 4
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry strtokBop
	;	TOS: ptr2 ptr1
	push	rpsp
	push	rip
    sub esp, 12     ; 16-byte align for mac
	mov	eax, [rpsp]
	push	eax
	mov	eax, [rpsp+4]
	push	eax
	xcall	strtok
	add	esp, 20
	pop	rip
	pop	rpsp
	add	rpsp, 4
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry litBop
entry flitBop
entry ulitBop
	mov	eax, [rip]
	add	rip, 4
	sub	rpsp, 4
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry dlitBop
	mov	eax, [rip]
	mov	ebx, [rip+4]
	add	rip, 8
	sub	rpsp, 8
	mov	[rpsp], eax
	mov	[rpsp+4], ebx
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
	mov	eax, [rcore + FCore.RPtr]
	sub	rpsp, 4
	mov	ebx, [eax]	; ebx points at param field
	mov	[rpsp], ebx
	add	eax, 4
	mov	[rcore + FCore.RPtr], eax
	jmp	rnext
	
;========================================

entry doVariableBop
	; push IP
	sub	rpsp, 4
	mov	[rpsp], rip
	; rpop new ip
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	rnext
	
;========================================

entry doConstantBop
	; push longword @ IP
	mov	eax, [rip]
	sub	rpsp, 4
	mov	[rpsp], eax
	; rpop new ip
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	rnext
	
;========================================

entry doDConstantBop
	; push quadword @ IP
	mov	eax, [rip]
	sub	rpsp, 8
	mov	[rpsp], eax
	mov	eax, [rip+4]
	mov	[rpsp+4], eax
	; rpop new ip
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	rnext
	
;========================================

entry doStructBop
	; push IP
	sub	rpsp, 4
	mov	[rpsp], rip
	; rpop new ip
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	rnext

;========================================

entry doStructArrayBop
	; TOS is array index
	; rip -> bytes per element, followed by element 0
	mov	eax, [rip]		; eax = bytes per element
	add	rip, 4			; rip -> element 0
	imul	eax, [rpsp]
	add	eax, rip		; add in array base addr
	mov	[rpsp], eax
	; rpop new ip
	mov	ebx, [rcore + FCore.RPtr]
	mov	rip, [ebx]
	add	ebx, 4
	mov	[rcore + FCore.RPtr], ebx
	jmp	rnext

;========================================

entry thisBop
	mov	eax, [rcore + FCore.TPtr]
	sub	rpsp, 4
	mov	[rpsp], eax
	jmp	rnext
	
;========================================

entry devolveBop
	mov	eax, [rcore + FCore.TPtr]
    mov ebx, [eax + 8]              ; ebx is wrapped object this ptr
	mov	[rcore + FCore.TPtr], ebx   ; set this to point to wrapped object
	jmp	rnext
	
;========================================

entry unsuperBop
	; pop original pMethods off rstack
	mov	eax, [rcore + FCore.RPtr]
	mov	ebx, [eax]
	add	eax, 4
	mov	[rcore + FCore.RPtr], eax
	; store original pMethods in this.pMethods
	mov	eax, [rcore + FCore.TPtr]
	mov	[eax], ebx
	jmp	rnext
	
;========================================

entry executeBop
	mov	ebx, [rpsp]
	add	rpsp, 4
	mov	eax, [rcore + FCore.innerExecute]
	jmp eax
	
;========================================

entry	fopenBop
	push	rip
	mov	eax, [rpsp]	; pop access string
	add	rpsp, 4
	push	rpsp
	push	eax
	mov	eax, [rpsp]	; pop pathname string
	push	eax
	mov	eax, [rcore + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.fileOpen]
	call	eax
	add		sp, 8
	pop	rpsp
	pop	rip
	mov	[rpsp], eax	; push fopen result
	jmp	rnext
	
;========================================

entry	fcloseBop
	push	rip
	mov	eax, [rpsp]	; pop file pointer
	push	rpsp
	push	eax
	mov	eax, [rcore + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.fileClose]
	call	eax
	add	sp,4
	pop	rpsp
	pop	rip
	mov	[rpsp], eax	; push fclose result
	jmp	rnext
	
;========================================

entry	fseekBop
	push	rip
	push	rpsp
	mov	eax, [rpsp]	; pop control
	push	eax
	mov	eax, [rpsp+4]	; pop offset
	push	eax
	mov	eax, [rpsp+8]	; pop file pointer
	push	eax
	mov	eax, [rcore + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.fileSeek]
	call	eax
	add		sp, 12
	pop	rpsp
	pop	rip
	add	rpsp, 8
	mov	[rpsp], eax	; push fseek result
	jmp	rnext
	
;========================================

entry	freadBop
	push	rip
	push	rpsp
	mov	eax, [rpsp]	; pop file pointer
	push	eax
	mov	eax, [rpsp+4]	; pop numItems
	push	eax
	mov	eax, [rpsp+8]	; pop item size
	push	eax
	mov	eax, [rpsp+12]	; pop dest pointer
	push	eax
	mov	eax, [rcore + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.fileRead]
	call	eax
	add		sp, 16
	pop	rpsp
	pop	rip
	add	rpsp, 12
	mov	[rpsp], eax	; push fread result
	jmp	rnext
	
;========================================

entry	fwriteBop
	push	rip
	push	rpsp
	mov	eax, [rpsp]	; pop file pointer
	push	eax
	mov	eax, [rpsp+4]	; pop numItems
	push	eax
	mov	eax, [rpsp+8]	; pop item size
	push	eax
	mov	eax, [rpsp+12]	; pop dest pointer
	push	eax
	mov	eax, [rcore + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.fileWrite]
	call	eax
	add		sp, 16
	pop	rpsp
	pop	rip
	add	rpsp, 12
	mov	[rpsp], eax	; push fwrite result
	jmp	rnext
	
;========================================

entry	fgetcBop
	push	rip
	mov	eax, [rpsp]	; pop file pointer
	push	rpsp
	push	eax
	mov	eax, [rcore + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.fileGetChar]
	call	eax
	add	sp, 4
	pop	rpsp
	pop	rip
	mov	[rpsp], eax	; push fgetc result
	jmp	rnext
	
;========================================

entry	fputcBop
	push	rip
	mov	eax, [rpsp]	; pop char to put
	add	rpsp, 4
	push	rpsp
	push	eax
	mov	eax, [rpsp]	; pop file pointer
	push	eax
	mov	eax, [rcore + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.filePutChar]
	call	eax
	add		sp, 8
	pop	rpsp
	pop	rip
	mov	[rpsp], eax	; push fputc result
	jmp	rnext
	
;========================================

entry	feofBop
	push	rip
	mov	eax, [rpsp]	; pop file pointer
	push	rpsp
	push	eax
	mov	eax, [rcore + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.fileAtEnd]
	call	eax
	add	sp, 4
	pop	rpsp
	pop	rip
	mov	[rpsp], eax	; push feof result
	jmp	rnext
	
;========================================

entry	fexistsBop
	push	rip
	mov	eax, [rpsp]	; pop filename pointer
	push	rpsp
	push	eax
	mov	eax, [rcore + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.fileExists]
	call	eax
	add	sp, 4
	pop	rpsp
	pop	rip
	mov	[rpsp], eax	; push fexists result
	jmp	rnext
	
;========================================

entry	ftellBop
	push	rip
	mov	eax, [rpsp]	; pop file pointer
	push	rpsp
	push	eax
	mov	eax, [rcore + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.fileTell]
	call	eax
	add	sp, 4
	pop	rpsp
	pop	rip
	mov	[rpsp], eax	; push ftell result
	jmp	rnext
	
;========================================

entry	flenBop
	push	rip
	mov	eax, [rpsp]	; pop file pointer
	push	rpsp
	push	eax
	mov	eax, [rcore + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.fileGetLength]
	call	eax
	add	sp, 4
	pop	rpsp
	pop	rip
	mov	[rpsp], eax	; push flen result
	jmp	rnext
	
;========================================

entry	fgetsBop
	push	rip
	push	rpsp
	mov	eax, [rpsp]	; pop file
	push	eax
	mov	eax, [rpsp+4]	; pop maxChars
	push	eax
	mov	eax, [rpsp+8]	; pop buffer
	push	eax
	mov	eax, [rcore + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.fileGetString]
	call	eax
	add		sp, 12
	pop	rpsp
	pop	rip
	add	rpsp, 8
	mov	[rpsp], eax	; push fgets result
	jmp	rnext
	
;========================================

entry	fputsBop
	push	rip
	push	rpsp
	mov	eax, [rpsp]	; pop file
	push	eax
	mov	eax, [rpsp+4]	; pop buffer
	push	eax
	mov	eax, [rcore + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.filePutString]
	call	eax
	add		sp, 8
	pop	rpsp
	pop	rip
	add	rpsp, 4
	mov	[rpsp], eax	; push fseek result
	jmp	rnext
	
;========================================
entry	setTraceBop
	mov	eax, [rpsp]
	add	rpsp, 4
	mov	[rcore + FCore.traceFlags], eax
	mov	[rcore + FCore.SPtr], rpsp
	mov	[rcore + FCore.IPtr], rip
	jmp interpFunc

;========================================

;extern void fprintfSub( ForthCoreState* pCore );
;extern void snprintfSub( ForthCoreState* pCore );
;extern void fscanfSub( ForthCoreState* pCore );
;extern void sscanfSub( ForthCoreState* pCore );

;========================================

entry fprintfSubCore
    ; TOS: N argN ... arg1 formatStr filePtr       (arg1 to argN are optional)
	mov	rnext, [rpsp]
	add	rnext, 2
	add	rpsp, 4
	mov	rip, rnext
fprintfSub1:
	sub	rip, 1
	jl	fprintfSub2
	mov	ebx, [rpsp]
	add	rpsp, 4
	push	ebx
	jmp fprintfSub1
fprintfSub2:
	; all args have been moved from parameter stack to PC stack
	mov	[rcore + FCore.SPtr], rpsp
	xcall	fprintf
	mov	rpsp, [rcore + FCore.SPtr]
	sub	rpsp, 4
	mov	[rpsp], eax		; return result on parameter stack
	mov	[rcore + FCore.SPtr], rpsp
	; cleanup PC stack
	mov	ebx, rnext
	sal	ebx, 2
	add	esp, ebx
	ret
	
; extern void fprintfSub( ForthCoreState* pCore );

entry fprintfSub
;fprintfSub PROC near C public uses ebx rip rpsp ecx rnext rcore,
;	core:PTR
	push rcore
	mov	rcore,esp
	push ebx
	push ecx
	push rpsp
	push rip
	push rnext
	push rcore
	
	mov	rcore, [rcore + 8]
	mov	rpsp, [rcore + FCore.SPtr]
	call	fprintfSubCore
	
	pop rcore
	pop	rnext
	pop	rip
	pop rpsp
	pop ecx
	pop ebx
	leave
	ret

;========================================

entry snprintfSubCore
    ; TOS: N argN ... arg1 formatStr bufferSize bufferPtr       (arg1 to argN are optional)
	mov	rnext, [rpsp]
	add	rnext, 3
	add	rpsp, 4
	mov	rip, rnext
snprintfSub1:
	sub	rip, 1
	jl	snprintfSub2
	mov	ebx, [rpsp]
	add	rpsp, 4
	push	ebx
	jmp snprintfSub1
snprintfSub2:
	; all args have been moved from parameter stack to PC stack
	mov	[rcore + FCore.SPtr], rpsp
%ifdef WIN32
	xcall	_snprintf
%else
    xcall	snprintf
%endif
	mov	rpsp, [rcore + FCore.SPtr]
	sub	rpsp, 4
	mov	[rpsp], eax		; return result on parameter stack
	mov	[rcore + FCore.SPtr], rpsp
	; cleanup PC stack
	mov	ebx, rnext
	sal	ebx, 2
	add	esp, ebx
	ret
	
; extern long snprintfSub( ForthCoreState* pCore );

entry snprintfSub
;snprintfSub PROC near C GLOBAL uses ebx rip rpsp ecx rnext rcore,
;	core:PTR
	push rcore
	mov	rcore,esp
	push ebx
	push ecx
	push rpsp
	push rip
	push rnext
	push rcore
	
	mov	rcore, [rcore + 8]
	mov	rpsp, [rcore + FCore.SPtr]
	call	snprintfSubCore
	
	pop rcore
	pop	rnext
	pop	rip
	pop rpsp
	pop ecx
	pop ebx
	leave
	ret

;========================================

; extern int oStringFormatSub( ForthCoreState* pCore, char* pBuffer, int bufferSize );
entry oStringFormatSub;
;oStringFormatSub PROC near C public uses ebx rip rpsp ecx rnext rcore,
;	core:PTR,
;	pBuffer:PTR,
;	bufferSize:DWORD
    ; TOS: numArgs argN ... arg1 formatStr (arg1 to argN are optional)

	push rcore
	mov	rcore,esp
	push ebx
	push rip
	push rpsp
	push ecx
	push rnext
	push rcore
	
	mov	ebx, [rcore + 8]      ; ebx -> core

	; store old SP on rstack
	mov	rpsp, [ebx + FCore.RPtr]
	sub	rpsp, 4
	mov	[rpsp], esp
	mov	[ebx + FCore.RPtr], rpsp

	; copy arg1 ... argN from param stack to PC stack
	mov	rpsp, [ebx + FCore.SPtr]
	mov	rnext, [rpsp]	; get numArgs
	add	rnext, 1		; add one for the format string
	add	rpsp, 4
oSFormatSub1:
	sub	rnext, 1
	jl	oSFormatSub2
	mov	eax, [rpsp]
	add	rpsp, 4
	push	eax
	jmp oSFormatSub1
oSFormatSub2:
	; all args have been copied from parameter stack to PC stack
	mov	[ebx + FCore.SPtr], rpsp
	mov	eax, [rcore + 16]         ; bufferSize
	push eax
	mov	eax, [rcore + 12]         ; pBuffer
	push eax

%ifdef WIN32
    xcall	_snprintf
%else
    xcall	snprintf
%endif

	; cleanup PC stack
	mov	rpsp, [ebx + FCore.RPtr]
	mov	esp, [rpsp]
	add	rpsp, 4
	mov	[ebx + FCore.RPtr], rpsp

	pop rcore
	pop	rnext
	pop ecx
	pop rpsp
	pop	rip
	pop ebx
	leave
	ret

;========================================

entry fscanfSubCore
	mov	rnext, [rpsp]
	add	rnext, 2
	add	rpsp, 4
	mov	rip, rnext
fscanfSub1:
	sub	rip, 1
	jl	fscanfSub2
	mov	ebx, [rpsp]
	add	rpsp, 4
	push	ebx
	jmp fscanfSub1
fscanfSub2:
	; all args have been moved from parameter stack to PC stack
	mov	[rcore + FCore.SPtr], rpsp
	xcall	fscanf
	mov	rpsp, [rcore + FCore.SPtr]
	sub	rpsp, 4
	mov	[rpsp], eax		; return result on parameter stack
	mov	[rcore + FCore.SPtr], rpsp
	; cleanup PC stack
	mov	ebx, rnext
	sal	ebx, 2
	add	esp, ebx
	ret
	
; extern long fscanfSub( ForthCoreState* pCore );

entry fscanfSub
;fscanfSub PROC near C public uses ebx rip rpsp ecx rnext rcore,
;	core:PTR
	push rcore
	mov	rcore,esp
	push ebx
	push ecx
	push rpsp
	push rip
	push rnext
	push rcore
	
	mov	rcore, [rcore + 8]
	mov	rpsp, [rcore + FCore.SPtr]
	call	fscanfSubCore
	
	pop rcore
	pop	rnext
	pop	rip
	pop rpsp
	pop ecx
	pop ebx
	leave
	ret

;========================================

entry sscanfSubCore
	mov	rnext, [rpsp]
	add	rnext, 2
	add	rpsp, 4
	mov	rip, rnext
sscanfSub1:
	sub	rip, 1
	jl	sscanfSub2
	mov	ebx, [rpsp]
	add	rpsp, 4
	push	ebx
	jmp sscanfSub1
sscanfSub2:
	; all args have been moved from parameter stack to PC stack
	mov	[rcore + FCore.SPtr], rpsp
	xcall	sscanf
	mov	rpsp, [rcore + FCore.SPtr]
	sub	rpsp, 4
	mov	[rpsp], eax		; return result on parameter stack
	mov	[rcore + FCore.SPtr], rpsp
	; cleanup PC stack
	mov	ebx, rnext
	sal	ebx, 2
	add	esp, ebx
	ret

; extern long sscanfSub( ForthCoreState* pCore );

entry sscanfSub
;sscanfSub PROC near C public uses ebx rip rpsp ecx rnext rcore,
;	core:PTR
	push rcore
	mov	rcore,esp
	push ebx
	push ecx
	push rpsp
	push rip
	push rnext
	push rcore
	
	mov	rcore, [rcore + 8]
	mov	rpsp, [rcore + FCore.SPtr]
	call	sscanfSubCore
	
	pop rcore
	pop	rnext
	pop	rip
	pop rpsp
	pop ecx
	pop ebx
	leave
	ret

;========================================
entry dllEntryPointType
	mov	[rcore + FCore.IPtr], rip
	mov	[rcore + FCore.SPtr], rpsp
	mov	eax, ebx
	and	eax, 0000FFFFh
	cmp	eax, [rcore + FCore.numOps]
	jge	badUserDef
	; push core ptr
	push	rcore
	; push flags
	mov	rip, ebx
	shr	rip, 16
	and	rip, 7
	push	rip
	; push arg count
	mov	rip, ebx
	shr	rip, 19
	and	rip, 1Fh
	push	rip
	; push entry point address
	mov	rip, [rcore + FCore.ops]
	mov	rpsp, [rip+eax*4]
	push	rpsp
	xcall	CallDLLRoutine
	add	esp, 12
	pop	rcore
	jmp	interpFunc


;-----------------------------------------------
;
; NUM VAROP OP combo ops
;  
entry nvoComboType
	; ebx: bits 0..10 are signed integer, bits 11..12 are varop-2, bit 13..23 are opcode
	mov	eax, ebx
	sub	rpsp, 4
	and	eax,00000400h
	jnz	nvoNegative
	; positive constant
	mov	eax, ebx
	and	eax,003FFh
	jmp	nvoCombo1

nvoNegative:
	mov	eax, ebx
	or	eax, 0FFFFF800h			; sign extend bits 11-31
nvoCombo1:
	mov	[rpsp], eax
	; set the varop from bits 11-12
	shr	ebx, 11
	mov	eax, ebx
	
	and	eax, 3							; eax = varop - 2
	add	eax, 2
	mov	[rcore + FCore.varMode], eax
	
	; extract the opcode
	shr	ebx, 2
	and	ebx, 0000007FFh			; ebx is 11 bit opcode
	; opcode is in ebx
	mov	eax, [rcore + FCore.innerExecute]
	jmp eax

;-----------------------------------------------
;
; NUM VAROP combo ops
;  
entry nvComboType
	; ebx: bits 0..21 are signed integer, bits 22-23 are varop-2
	mov	eax, ebx
	sub	rpsp, 4
	and	eax,00200000h
	jnz	nvNegative
	; positive constant
	mov	eax, ebx
	and	eax,001FFFFFh
	jmp	nvCombo1

nvNegative:
	mov	eax, ebx
	or	ebx, 0FFE00000h			; sign extend bits 22-31
nvCombo1:
	mov	[rpsp], eax
	; set the varop from bits 22-23
	shr	ebx, 22							; ebx = varop - 2
	and	ebx, 3
	add	ebx, 2
	mov	[rcore + FCore.varMode], ebx

	jmp rnext

;-----------------------------------------------
;
; NUM OP combo ops
;  
entry noComboType
	; ebx: bits 0..12 are signed integer, bits 13..23 are opcode
	mov	eax, ebx
	sub	rpsp, 4
	and	eax,000001000h
	jnz	noNegative
	; positive constant
	mov	eax, ebx
	and	eax,00FFFh
	jmp	noCombo1

noNegative:
	or	eax, 0FFFFE000h			; sign extend bits 13-31
noCombo1:
	mov	[rpsp], eax
	; extract the opcode
	mov	eax, ebx
	shr	ebx, 13
	and	ebx, 0000007FFh			; ebx is 11 bit opcode
	; opcode is in ebx
	mov	eax, [rcore + FCore.innerExecute]
	jmp eax
	
;-----------------------------------------------
;
; VAROP OP combo ops
;  
entry voComboType
	; ebx: bits 0-1 are varop-2, bits 2-23 are opcode
	; set the varop from bits 0-1
	mov	eax, 000000003h
	and	eax, ebx
	add	eax, 2
	mov	[rcore + FCore.varMode], eax
	
	; extract the opcode
	shr	ebx, 2
	and	ebx, 0003FFFFFh			; ebx is 22 bit opcode

	; opcode is in ebx
	mov	eax, [rcore + FCore.innerExecute]
	jmp eax

;-----------------------------------------------
;
; OP ZBRANCH combo ops
;  
entry ozbComboType
	; ebx: bits 0..11 are opcode, bits 12-23 are signed integer branch offset in longs
	mov	eax, ebx
	shr	eax, 10
	and	eax, 03FFCh
	push	eax
	push	rnext
	mov	rnext, ozbCombo1
	and	ebx, 0FFFh
	; opcode is in ebx
	mov	eax, [rcore + FCore.innerExecute]
	jmp eax
	
ozbCombo1:
	pop	rnext
	pop	eax
	mov	ebx, [rpsp]
	add	rpsp, 4
	or	ebx, ebx
	jnz	ozbCombo2			; if TOS not 0, don't branch
	mov	ebx, eax
	and	eax, 02000h
	jz	ozbForward
	; backward branch
	or	ebx,0FFFFC000h
ozbForward:
	add	rip, ebx
ozbCombo2:
	jmp	rnext
	
;-----------------------------------------------
;
; OP NZBRANCH combo ops
;  
entry onzbComboType
	; ebx: bits 0..11 are opcode, bits 12-23 are signed integer branch offset in longs
	mov	eax, ebx
	shr	eax, 10
	and	eax, 03FFCh
	push	eax
	push	rnext
	mov	rnext, onzbCombo1
	and	ebx, 0FFFh
	; opcode is in ebx
	mov	eax, [rcore + FCore.innerExecute]
	jmp eax
	
onzbCombo1:
	pop	rnext
	pop	eax
	mov	ebx, [rpsp]
	add	rpsp, 4
	or	ebx, ebx
	jz	onzbCombo2			; if TOS 0, don't branch
	mov	ebx, eax
	and	eax, 02000h
	jz	onzbForward
	; backward branch
	or	ebx,0FFFFC000h
onzbForward:
	add	rip, ebx
onzbCombo2:
	jmp	rnext
	
;-----------------------------------------------
;
; squished float literal
;  
entry squishedFloatType
	; ebx: bit 23 is sign, bits 22..18 are exponent, bits 17..0 are mantissa
	; to unsquish a float:
	;   sign = (inVal & 0x800000) << 8
	;   exponent = (((inVal >> 18) & 0x1f) + (127 - 15)) << 23
	;   mantissa = (inVal & 0x3ffff) << 5
	;   outVal = sign | exponent | mantissa
	push	rip
	mov	eax, ebx
	and	eax, 00800000h
	shl	eax, 8			; sign bit
	
	mov	rip, ebx
	shr	ebx, 18
	and	ebx, 1Fh
	add	ebx, 112
	shl	ebx, 23			; ebx is exponent
	or	eax, ebx
	
	and	rip, 03FFFFh
	shl	rip, 5
	or	eax, rip
	
	sub	rpsp, 4
	mov	[rpsp], eax
	pop	rip
	jmp	rnext
	

;-----------------------------------------------
;
; squished double literal
;  
entry squishedDoubleType
	; ebx: bit 23 is sign, bits 22..18 are exponent, bits 17..0 are mantissa
	; to unsquish a double:
	;   sign = (inVal & 0x800000) << 8
	;   exponent = (((inVal >> 18) & 0x1f) + (1023 - 15)) << 20
	;   mantissa = (inVal & 0x3ffff) << 2
	;   outVal = (sign | exponent | mantissa) << 32
	push	rip
	mov	eax, ebx
	and	eax, 00800000h
	shl	eax, 8			; sign bit
	
	mov	rip, ebx
	shr	ebx, 18
	and	ebx, 1Fh
	add	ebx, 1008
	shl	ebx, 20			; ebx is exponent
	or	eax, ebx
	
	and	rip, 03FFFFh
	shl	rip, 2
	or	eax, rip
	
	sub	rpsp, 4
	mov	[rpsp], eax
	sub	rpsp, 4
	; loword of double is all zeros
	xor	eax, eax
	mov	[rpsp], eax
	pop	rip
	jmp	rnext
	

;-----------------------------------------------
;
; squished long literal
;  
entry squishedLongType
	; get low-24 bits of opcode
	mov	eax, ebx
	sub	rpsp, 8
	and	eax,00800000h
	jnz	longConstantNegative
	; positive constant
	and	ebx,00FFFFFFh
	mov	[rpsp+4], ebx
	xor	ebx, ebx
	mov	[rpsp], ebx
	jmp	rnext

longConstantNegative:
	or	ebx, 0FF000000h
	mov	[rpsp+4], ebx
	xor	ebx, ebx
	sub	ebx, 1
	mov	[rpsp], ebx
	jmp	rnext
	

;-----------------------------------------------
;
; LOCALREF OP combo ops
;
entry lroComboType
	; ebx: bits 0..11 are frame offset in longs, bits 12-23 are op
	push	ebx
	and	ebx, 0FFFH
	sal	ebx, 2
	mov	eax, [rcore + FCore.FPtr]
	sub	eax, ebx
	sub	rpsp, 4
	mov	[rpsp], eax
	
	pop	ebx
	shr	ebx, 12
	and	ebx, 0FFFH			; ebx is 12 bit opcode
	; opcode is in ebx
	mov	eax, [rcore + FCore.innerExecute]
	jmp eax
	
;-----------------------------------------------
;
; MEMBERREF OP combo ops
;
entry mroComboType
	; ebx: bits 0..11 are member offset in bytes, bits 12-23 are op
	push	ebx
	and	ebx, 0FFFH
	mov	eax, [rcore + FCore.TPtr]
	add	eax, ebx
	sub	rpsp, 4
	mov	[rpsp], eax

	pop	ebx
	shr	ebx, 12
	and	ebx, 0FFFH			; ebx is 12 bit opcode
	; opcode is in ebx
	mov	eax, [rcore + FCore.innerExecute]
	jmp eax


;=================================================================================================
;
;                                    opType table
;  
;=================================================================================================
entry opTypesTable
; TBD: check the order of these
; TBD: copy these into base of ForthCoreState, fill unused slots with badOptype
;	00 - 09
	DD	externalBuiltin		; kOpNative = 0,
	DD	nativeImmediate		; kOpNativeImmediate,
	DD	userDefType			; kOpUserDef,
	DD	userDefType			; kOpUserDefImmediate,
	DD	cCodeType				; kOpCCode,         
	DD	cCodeType				; kOpCCodeImmediate,
	DD	relativeDefType		; kOpRelativeDef,
	DD	relativeDefType		; kOpRelativeDefImmediate,
	DD	dllEntryPointType		; kOpDLLEntryPoint,
	DD	extOpType	
;	10 - 19
	DD	branchType				; kOpBranch = 10,
	DD	branchNZType			; kOpBranchNZ,
	DD	branchZType			    ; kOpBranchZ,
	DD	caseBranchTType			; kOpCaseBranchT,
	DD	caseBranchFType			; kOpCaseBranchF,
	DD	pushBranchType			; kOpPushBranch,	
	DD	relativeDefBranchType	; kOpRelativeDefBranch,
	DD	relativeDataType		; kOpRelativeData,
	DD	relativeDataType		; kOpRelativeString,
	DD	extOpType	
;	20 - 29
	DD	constantType			; kOpConstant = 20,   
	DD	constantStringType		; kOpConstantString,	
	DD	offsetType				; kOpOffset,          
	DD	arrayOffsetType		; kOpArrayOffset,     
	DD	allocLocalsType		; kOpAllocLocals,     
	DD	localRefType			; kOpLocalRef,
	DD	initLocalStringType	; kOpLocalStringInit, 
	DD	localStructArrayType	; kOpLocalStructArray,
	DD	offsetFetchType		; kOpOffsetFetch,          
	DD	memberRefType			; kOpMemberRef,	

;	30 - 39
	DD	localByteType			; 30 - 42 : local variables
	DD	localUByteType
	DD	localShortType
	DD	localUShortType
	DD	localIntType
	DD	localIntType
	DD	localLongType
	DD	localLongType
	DD	localFloatType
	DD	localDoubleType
	
;	40 - 49
	DD	localStringType
	DD	localOpType
	DD	localObjectType
	DD	localByteArrayType		; 43 - 55 : local arrays
	DD	localUByteArrayType
	DD	localShortArrayType
	DD	localUShortArrayType
	DD	localIntArrayType
	DD	localIntArrayType
	DD	localLongArrayType
	
;	50 - 59
	DD	localLongArrayType
	DD	localFloatArrayType
	DD	localDoubleArrayType
	DD	localStringArrayType
	DD	localOpArrayType
	DD	localObjectArrayType
	DD	fieldByteType			; 56 - 68 : field variables
	DD	fieldUByteType
	DD	fieldShortType
	DD	fieldUShortType
	
;	60 - 69
	DD	fieldIntType
	DD	fieldIntType
	DD	fieldLongType
	DD	fieldLongType
	DD	fieldFloatType
	DD	fieldDoubleType
	DD	fieldStringType
	DD	fieldOpType
	DD	fieldObjectType
	DD	fieldByteArrayType		; 69 - 81 : field arrays
	
;	70 - 79
	DD	fieldUByteArrayType
	DD	fieldShortArrayType
	DD	fieldUShortArrayType
	DD	fieldIntArrayType
	DD	fieldIntArrayType
	DD	fieldLongArrayType
	DD	fieldLongArrayType
	DD	fieldFloatArrayType
	DD	fieldDoubleArrayType
	DD	fieldStringArrayType
	
;	80 - 89
	DD	fieldOpArrayType
	DD	fieldObjectArrayType
	DD	memberByteType			; 82 - 94 : member variables
	DD	memberUByteType
	DD	memberShortType
	DD	memberUShortType
	DD	memberIntType
	DD	memberIntType
	DD	memberLongType
	DD	memberLongType
	
;	90 - 99
	DD	memberFloatType
	DD	memberDoubleType
	DD	memberStringType
	DD	memberOpType
	DD	memberObjectType
	DD	memberByteArrayType	; 95 - 107 : member arrays
	DD	memberUByteArrayType
	DD	memberShortArrayType
	DD	memberUShortArrayType
	DD	memberIntArrayType
	
;	100 - 109
	DD	memberIntArrayType
	DD	memberLongArrayType
	DD	memberLongArrayType
	DD	memberFloatArrayType
	DD	memberDoubleArrayType
	DD	memberStringArrayType
	DD	memberOpArrayType
	DD	memberObjectArrayType
	DD	methodWithThisType
	DD	methodWithTOSType
	
;	110 - 119
	DD	memberStringInitType
	DD	nvoComboType
	DD	nvComboType
	DD	noComboType
	DD	voComboType
	DD	ozbComboType
	DD	onzbComboType
	
	DD	squishedFloatType
	DD	squishedDoubleType
	DD	squishedLongType
	
;	120 - 122
	DD	lroComboType
	DD	mroComboType
	DD	methodWithSuperType
	
;	123 - 127
    DD  native32Type
    DD  native32Type
    DD  native32Type
    DD  native64Type
    DD  native64Type

;	128 - 132
    DD  cCode32Type
    DD  cCode32Type
    DD  cCode32Type
    DD  cCode64Type
    DD  cCode64Type

;	133 - 137
    DD  userDef32Type
    DD  userDef32Type
    DD  userDef32Type
    DD  userDef64Type
    DD  userDef64Type

;	138 - 149
	DD	extOpType,extOpType
	DD	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
;	150 - 199
	DD	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	DD	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	DD	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	DD	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	DD	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
;	200 - 249
	DD	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	DD	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	DD	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	DD	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	DD	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
;	250 - 255
	DD	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	
endOpTypesTable:
	DD	0
	
