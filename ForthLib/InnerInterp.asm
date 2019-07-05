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

; when in a opType routine:
;	AL		8-bit opType
;	EBX		full 32-bit opcode (need to mask off top 8 bits)

; remember when calling extern cdecl functions:
; 1) they are free to stomp EAX, EBX, ECX and EDX
; 2) they are free to modify their input params on stack

; if you need more than EAX, EBX and ECX in a routine, save ESI/IP & EDX/SP in FCore at start with these instructions:
;	mov	[ebp + FCore.IPtr], esi
;	mov	[ebp + FCore.SPtr], edx
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
	push	edx
	push	esi
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx]
	push	eax
%ifdef LINUX
	call	%2
%else
	call	_%2
%endif
	add	esp, 8
	pop	esi
	pop	edx
	fstp	QWORD[edx]
	jmp	edi
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
	push	edx
	push	esi
	mov	eax, [edx]
	push	eax
%ifdef LINUX
	call	%2
%else
	call	_%2
%endif
	add	esp, 4
	pop	esi
	pop	edx
	fstp	DWORD[edx]
	jmp	edi
%endmacro
	
;========================================
;  safe exception handler
;.safeseh SEH_handler

;SEH_handler   PROC
;	ret

;SEH_handler   ENDP

; for some reason, this is always true, you need to change the name,
; changing the build rule to not define it isn't enough	
%ifdef	ASM_INNER_INTERPRETER

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
	mov	ecx, [ebp + FCore.optypeAction]
	mov	eax, [ecx + eax*4]				; eax is C routine to dispatch to
	; save current IP and SP	
	mov	[ebp + FCore.IPtr], esi
	mov	[ebp + FCore.SPtr], edx
	; we need to push ebp twice - the C compiler feels free to overwrite its input parameters,
	; so the top copy of EBP may be trashed on return from C land
	push	ebp		; push core ptr (our save)
	and	ebx, 00FFFFFFh
	push	ebx		; push 24-bit opVal (input to C routine)
	push	ebp		; push core ptr (input to C routine)
	call	eax
	add	esp, 8		; discard inputs to C routine 
	pop	ebp
	
	; load IP and SP from core, in case C routine modified them
	; NOTE: we can't just jump to interpFunc, since that will replace edi & break single stepping
	mov	esi, [ebp + FCore.IPtr]
	mov	edx, [ebp + FCore.SPtr]
	mov	eax, [ebp + FCore.state]
	or	eax, eax
	jnz	extOpType1	; if something went wrong
	jmp	edi			; if everything is ok
	
; NOTE: Feb. 14 '07 - doing the right thing here - restoring IP & SP and jumping to
; the interpreter loop exit point - causes an access violation exception ?why?
	;mov	esi, [ebp + FCore.IPtr]
	;mov	edx, [ebp + FCore.SPtr]
	;jmp	interpLoopExit	; if something went wrong
	
extOpType1:
	ret

;-----------------------------------------------
;
; InitAsmTables - initializes first part of optable, where op positions are referenced by constants
;
; extern void InitAsmTables( ForthCoreState *pCore );
entry InitAsmTables
	push ebp
	mov	ebp, esp	; 0(ebp) = old_ebp 4(ebp)=return_addr  8(ebp)=ForthCore_ptr
	push ebx
	push ecx
	push edx
	push edi
	push ebp

	mov	ebp,[ebp + 8]		; ebp -> ForthCore struct
	
	; setup normal (non-debug) inner interpreter re-entry point
	mov	ebx, interpLoopDebug
	mov	[ebp + FCore.innerLoop], ebx
	mov	ebx, interpLoopExecuteEntry
	mov	[ebp + FCore.innerExecute], ebx
	

	pop ebp
	pop	edi
	pop edx
	pop ecx
	pop ebx
	leave
	ret

;-----------------------------------------------
;
; single step a thread
;
; extern eForthResult InterpretOneOpFast( ForthCoreState *pCore, long op );
entry InterpretOneOpFast
	push ebp
	mov	ebp, esp
	push ebx
	push ecx
	push edx
	push esi
	push edi
	push ebp
	
	mov	ebx, [ebp + 12]
	mov	ebp, [ebp + 8]      ; ebp -> ForthCore
	mov	esi, [ebp + FCore.IPtr]
	mov	edx, [ebp + FCore.SPtr]
	mov	edi, InterpretOneOpFastExit
	; TODO: should we reset state to OK before every step?
	mov	eax, kResultOk
	mov	[ebp + FCore.state], eax
	; instead of jumping directly to the inner loop, do a call so that
	; error exits which do a return instead of branching to inner loop will work
	call	interpLoopExecuteEntry
	jmp	InterpretOneOpFastExit2

InterpretOneOpFastExit:		; this is exit for state == OK - discard the unused return address from call above
	add	esp, 4
InterpretOneOpFastExit2:	; this is exit for state != OK
	mov	[ebp + FCore.IPtr], esi
	mov	[ebp + FCore.SPtr], edx
	mov	eax, [ebp + FCore.state]

	pop ebp
	pop	edi
	pop	esi
	pop edx
	pop ecx
	pop ebx
	leave
	ret

;-----------------------------------------------
;
; inner interpreter C entry point
;
; extern eForthResult InnerInterpreterFast( ForthCoreState *pCore );
entry InnerInterpreterFast
	push ebp
	mov	ebp, esp
	push ebx
	push ecx
	push edx
	push esi
	push edi
	push ebp
	
	mov	ebp, [ebp + 8]      ; ebp -> ForthCore
	mov	eax, kResultOk
	mov	[ebp + FCore.state], eax

	call	interpFunc

	pop ebp
	pop	edi
	pop	esi
	pop edx
	pop ecx
	pop ebx
	leave
	ret

;-----------------------------------------------
;
; inner interpreter
;	jump to interpFunc if you need to reload IP, SP, interpLoop
entry interpFunc
	mov	eax, [ebp + FCore.traceFlags]
	and	eax, traceDebugFlags
	jz .interpFunc1
	mov	ebx, traceLoopDebug
	mov	[ebp + FCore.innerLoop], ebx
	mov	ebx, traceLoopExecuteEntry
	mov	[ebp + FCore.innerExecute], ebx
	jmp	.interpFunc2
.interpFunc1:
	mov	ebx, interpLoopDebug
	mov	[ebp + FCore.innerLoop], ebx
	mov	ebx, interpLoopExecuteEntry
	mov	[ebp + FCore.innerExecute], ebx
.interpFunc2:
	mov	esi, [ebp + FCore.IPtr]
	mov	edx, [ebp + FCore.SPtr]
	;mov	edi, interpLoopDebug
	mov	edi, [ebp + FCore.innerLoop]
	jmp	edi

entry interpLoopDebug
	; while debugging, store IP,SP in corestate shadow copies after every instruction
	;   so crash stacktrace will be more accurate (off by only one instruction)
	mov	[ebp + FCore.IPtr], esi
	mov	[ebp + FCore.SPtr], edx
entry interpLoop
	mov	ebx, [esi]		; ebx is opcode
	add	esi, 4			; advance IP
	; interpLoopExecuteEntry is entry for executeBop/methodWithThis/methodWithTos - expects opcode in ebx
interpLoopExecuteEntry:
%ifdef MACOSX
    ; check if system stack has become unaligned
    lea ecx, [ebp + FCore.scratch]
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
	cmp	ebx, [ebp + FCore.numOps]
	jae	notNative
	mov eax, [ebp + FCore.ops]
	mov	ecx, [eax+ebx*4]
	jmp	ecx

entry traceLoopDebug
	; while debugging, store IP,SP in corestate shadow copies after every instruction
	;   so crash stacktrace will be more accurate (off by only one instruction)
	mov	[ebp + FCore.IPtr], esi
	mov	[ebp + FCore.SPtr], edx
	mov	ebx, [esi]		; ebx is opcode
	mov	eax, esi		; eax is the IP for trace
	jmp	traceLoopDebug2

	; traceLoopExecuteEntry is entry for executeBop/methodWithThis/methodWithTos - expects opcode in ebx
traceLoopExecuteEntry:
	xor	eax, eax		; put null in trace IP for indirect execution (op isn't at IP)
	sub	esi, 4			; actual IP was already advanced by execute/method op, don't double advance it
traceLoopDebug2:
	push edx
    sub esp, 12         ; 16-byte align for OSX
    push ebx            ; opcode
    push eax            ; IP
    push ebp            ; core
	xcall traceOp
	pop ebp
	pop eax
	pop ebx
    add esp, 12
	pop edx
	add	esi, 4			; advance IP
	jmp interpLoopExecuteEntry

interpLoopExit:
	mov	[ebp + FCore.state], eax
	mov	[ebp + FCore.IPtr], esi
	mov	[ebp + FCore.SPtr], edx
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
	mov	[ebp + FCore.error], eax
	mov	eax, kResultError
	jmp	interpLoopExit
	
interpLoopFatalErrorExit:
	; fatal error exit point
	; eax is error code
	mov	[ebp + FCore.error], eax
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
	cmp	ebx, [ebp + FCore.numOps]
	jae	badOpcode
	mov eax, [ebp + FCore.ops]
	mov	ecx, [eax+ebx*4]
	jmp	ecx

; externalBuiltin is invoked when a builtin op which is outside of range of table is invoked
externalBuiltin:
	; it should be impossible to get here now
	jmp	badOpcode
	
;-----------------------------------------------
;
; cCodeType is used by "builtin" ops which are only defined in C++
;
;	ebx holds the opcode
;
cCodeType:
	and	ebx, 00FFFFFFh
	; dispatch to C version if valid
	cmp	ebx, [ebp + FCore.numOps]
	jae	badOpcode
	; ebx holds the opcode which was just dispatched, use its low 24-bits as index into builtinOps table of ops defined in C/C++
	mov	eax, ebx
	mov	ebx, [ebp + FCore.ops]
	mov	eax, [ebx + eax*4]				; eax is C routine to dispatch to
	; save current IP and SP	
	mov	[ebp + FCore.IPtr], esi
	mov	[ebp + FCore.SPtr], edx
	; we need to push ebp twice - the C compiler feels free to overwrite its input parameters,
	; so the top copy of EBP may be trashed on return from C land
%ifdef MACOSX
    sub esp, 4      ; 16-byte align for OSX
%endif
	push	ebp		; push core ptr (our save)
	push	ebp		; push core ptr (input to C routine)
	call	eax
	add	esp, 4		; discard inputs to C routine
	pop	ebp
%ifdef MACOSX
    add esp, 4
%endif
	; load IP and SP from core, in case C routine modified them
	; NOTE: we can't just jump to interpFunc, since that will replace edi & break single stepping
	mov	esi, [ebp + FCore.IPtr]
	mov	edx, [ebp + FCore.SPtr]
	mov	eax, [ebp + FCore.state]
	or	eax, eax
	jnz	interpLoopExit		; if something went wrong (should this be interpLoopErrorExit?)
	jmp	edi					; if everything is ok
	
; NOTE: Feb. 14 '07 - doing the right thing here - restoring IP & SP and jumping to
; the interpreter loop exit point - causes an access violation exception ?why?
	;mov	esi, [ebp + FCore.IPtr]
	;mov	edx, [ebp + FCore.SPtr]
	;jmp	interpLoopExit	; if something went wrong
	

;-----------------------------------------------
;
; user-defined ops (forth words defined with colon)
;
entry userDefType
	; get low-24 bits of opcode & check validity
	and	ebx, 00FFFFFFh
	cmp	ebx, [ebp + FCore.numOps]
	jge	badUserDef
	; push IP on rstack
	mov	eax, [ebp + FCore.RPtr]
	sub	eax, 4
	mov	[eax], esi
	mov	[ebp + FCore.RPtr], eax
	; get new IP
	mov	eax, [ebp + FCore.ops]
	mov	esi, [eax+ebx*4]
	jmp	edi

badUserDef:
	mov	eax, kForthErrorBadOpcode
	jmp	interpLoopErrorExit

;-----------------------------------------------
;
; user-defined ops (forth words defined with colon)
;
entry relativeDefType
	; ebx is offset from dictionary base of user definition
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	mov	eax, [ebp + FCore.DictionaryPtr]
	add	ebx, [eax + ForthMemorySection.pBase]
	cmp	ebx, [eax + ForthMemorySection.pCurrent]
	jge	badUserDef
	; push IP on rstack
	mov	eax, [ebp + FCore.RPtr]
	sub	eax, 4
	mov	[eax], esi
	mov	[ebp + FCore.RPtr], eax
	mov	esi, ebx
	jmp	edi

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
	add	esi, ebx
	jmp	edi

branchBack:
	or	ebx,0FF000000h
	sal	ebx, 2
	add	esi, ebx
	jmp	edi

;-----------------------------------------------
;
; branch-on-zero ops
;
entry branchZType
	mov	eax, [edx]
	add	edx, 4
	or	eax, eax
	jz	branchType		; branch taken
	jmp	edi	; branch not taken

;-----------------------------------------------
;
; branch-on-notzero ops
;
entry branchNZType
	mov	eax, [edx]
	add	edx, 4
	or	eax, eax
	jnz	branchType		; branch taken
	jmp	edi	; branch not taken

;-----------------------------------------------
;
; case branch ops
;
entry caseBranchTType
    ; TOS: this_case_value case_selector
	mov	eax, [edx]		; eax is this_case_value
	add	edx, 4
	cmp	eax, [edx]
	jnz	caseMismatch
	; case did match - branch to case body
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	add	esi, ebx
	add	edx, 4
caseMismatch:
	jmp	edi

entry caseBranchFType
    ; TOS: this_case_value case_selector
	mov	eax, [edx]		; eax is this_case_value
	add	edx, 4
	cmp	eax, [edx]
	jz	caseMatched
	; case didn't match - branch to next case test
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	add	esi, ebx
	jmp	edi

caseMatched:
	add	edx, 4
	jmp	edi
	
;-----------------------------------------------
;
; branch around block ops
;
entry pushBranchType
	sub	edx, 4			; push IP (pointer to block)
	mov	[edx], esi
	and	ebx, 00FFFFFFh	; branch around block
	sal	ebx, 2
	add	esi, ebx
	jmp	edi

;-----------------------------------------------
;
; relative data block ops
;
entry relativeDataType
	; ebx is offset from dictionary base of user definition
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	mov	eax, [ebp + FCore.DictionaryPtr]
	add	ebx, [eax + ForthMemorySection.pBase]
	cmp	ebx, [eax + ForthMemorySection.pCurrent]
	jge	badUserDef
	; push address of data on pstack
	sub	edx, 4
	mov	[edx], ebx
	jmp	edi

;-----------------------------------------------
;
; relative data ops
;
entry relativeDefBranchType
	; push relativeDef opcode for immediately following anonymous definition (IP in esi points to it)
	; compute offset from dictionary base to anonymous def
	mov	eax, [ebp + FCore.DictionaryPtr]
	mov	ecx, [eax + ForthMemorySection.pBase]
	mov	eax, esi
	sub	eax, ecx
	sar	eax, 2
	; stick the optype in top 8 bits
	mov	ecx, kOpRelativeDef << 24
	or	eax, ecx
	sub	edx, 4
	mov	[edx], eax
	; advance IP past anonymous definition
	and	ebx, 00FFFFFFh	; branch around block
	sal	ebx, 2
	add	esi, ebx
	jmp	edi


;-----------------------------------------------
;
; 24-bit constant ops
;
entry constantType
	; get low-24 bits of opcode
	mov	eax, ebx
	sub	edx, 4
	and	eax,00800000h
	jnz	constantNegative
	; positive constant
	and	ebx,00FFFFFFh
	mov	[edx], ebx
	jmp	edi

constantNegative:
	or	ebx, 0FF000000h
	mov	[edx], ebx
	jmp	edi

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
	add	[edx], ebx
	jmp	edi

offsetNegative:
	or	ebx, 0FF000000h
	add	[edx], ebx
	jmp	edi

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
	add	ebx, [edx]
	mov	eax, [ebx]
	mov	[edx], eax
	jmp	edi

offsetFetchNegative:
	or	ebx, 0FF000000h
	add	ebx, [edx]
	mov	eax, [ebx]
	mov	[edx], eax
	jmp	edi

;-----------------------------------------------
;
; array offset ops
;
entry arrayOffsetType
	; get low-24 bits of opcode
	and	ebx, 00FFFFFFh		; ebx is size of one element
	; TOS is array base, tos-1 is index
	imul	ebx, [edx+4]	; multiply index by element size
	add	ebx, [edx]			; add array base addr
	add	edx, 4
	mov	[edx], ebx
	jmp	edi

;-----------------------------------------------
;
; local struct array ops
;
entry localStructArrayType
   ; bits 0..11 are padded struct length in bytes, bits 12..23 are frame offset in longs
   ; multiply struct length by TOS, add in (negative) frame offset, and put result on TOS
	mov	eax, 00000FFFh
	and	eax, ebx                ; eax is padded struct length in bytes
	imul	eax, [edx]              ; multiply index * length
	add	eax, [ebp + FCore.FPtr]
	and	ebx, 00FFF000h
	sar	ebx, 10							; ebx = frame offset in bytes of struct[0]
	sub	eax, ebx						; eax -> struct[N]
	mov	[edx], eax
	jmp	edi

;-----------------------------------------------
;
; string constant ops
;
entry constantStringType
	; IP points to beginning of string
	; low 24-bits of ebx is string len in longs
	sub	edx, 4
	mov	[edx], esi		; push string ptr
	; get low-24 bits of opcode
	and	ebx, 00FFFFFFh
	shl	ebx, 2
	; advance IP past string
	add	esi, ebx
	jmp	edi

;-----------------------------------------------
;
; local stack frame allocation ops
;
entry allocLocalsType
	; rpush old FP
	mov	ecx, [ebp + FCore.FPtr]
	mov	eax, [ebp + FCore.RPtr]
	sub	eax, 4
	mov	[eax], ecx
	; set FP = RP, points at old FP
	mov	[ebp + FCore.FPtr], eax
	; allocate amount of storage specified by low 24-bits of op on rstack
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	mov	[ebp + FCore.RPtr], eax
	; clear out allocated storage
	mov ecx, eax
	xor eax, eax
alt1:
	mov [ecx], eax
	add ecx, 4
	sub ebx, 4
	jnz alt1
	jmp	edi

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
	mov	ecx, [ebp + FCore.FPtr]
	sub	ecx, eax						; ecx -> max length field
	and	ebx, 00000FFFh					; ebx = max length
	mov	[ecx], ebx						; set max length
	xor	eax, eax
	mov	[ecx+4], eax					; set current length to 0
	mov	[ecx+5], al						; add terminating null
	jmp	edi

;-----------------------------------------------
;
; local reference ops
;
entry localRefType
	; push local reference - ebx is frame offset in longs
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;-----------------------------------------------
;
; member reference ops
;
entry memberRefType
	; push member reference - ebx is member offset in bytes
	mov	eax, [ebp + FCore.TPtr]
	and	ebx, 00FFFFFFh
	add	eax, ebx
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;-----------------------------------------------
;
; local byte ops
;
entry localByteType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz localByteType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
localByteType1:
	; get ptr to byte var into eax
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 001FFFFFh
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
byteEntry:
	mov	ebx, [ebp + FCore.varMode]
	or	ebx, ebx
	jnz	localByte1
	; fetch local byte
localByteFetch:
	sub	edx, 4
	movsx	ebx, BYTE[eax]
	mov	[edx], ebx
	jmp	edi

entry localUByteType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz localUByteType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
localUByteType1:
	; get ptr to byte var into eax
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 001FFFFFh
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
ubyteEntry:
	mov	ebx, [ebp + FCore.varMode]
	or	ebx, ebx
	jnz	localByte1
	; fetch local unsigned byte
localUByteFetch:
	sub	edx, 4
	xor	ebx, ebx
	mov	bl, [eax]
	mov	[edx], ebx
	jmp	edi

localByte1:
	cmp	ebx, kVarMinusStore
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, [localByteActionTable + ebx*4]
	jmp	ebx

localByteRef:
	sub	edx, 4
	mov	[edx], eax
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	jmp	edi
	
localByteStore:
	mov	ebx, [edx]
	mov	[eax], bl
	add	edx, 4
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	jmp	edi

localBytePlusStore:
	xor	ebx, ebx
	mov	bl, [eax]
	add	ebx, [edx]
	mov	[eax], bl
	add	edx, 4
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	jmp	edi

localByteMinusStore:
	xor	ebx, ebx
	mov	bl, [eax]
	sub	ebx, [edx]
	mov	[eax], bl
	add	edx, 4
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	jmp	edi

localByteActionTable:
	DD	localByteFetch
	DD	localByteFetch
	DD	localByteRef
	DD	localByteStore
	DD	localBytePlusStore
	DD	localByteMinusStore

localUByteActionTable:
	DD	localUByteFetch
	DD	localUByteFetch
	DD	localByteRef
	DD	localByteStore
	DD	localBytePlusStore
	DD	localByteMinusStore

entry fieldByteType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz fieldByteType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
fieldByteType1:
	; get ptr to byte var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 001FFFFFh
	add	eax, ebx
	jmp	byteEntry

entry fieldUByteType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz fieldUByteType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
fieldUByteType1:
	; get ptr to byte var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 001FFFFFh
	add	eax, ebx
	jmp	ubyteEntry

entry memberByteType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz memberByteType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
memberByteType1:
	; get ptr to byte var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [ebp + FCore.TPtr]
	and	ebx, 001FFFFFh
	add	eax, ebx
	jmp	byteEntry

entry memberUByteType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz memberUByteType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
memberUByteType1:
	; get ptr to byte var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [ebp + FCore.TPtr]
	and	ebx, 001FFFFFh
	add	eax, ebx
	jmp	ubyteEntry

entry localByteArrayType
	; get ptr to byte var into eax
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	add	eax, [edx]		; add in array index on TOS
	add	edx, 4
	jmp	byteEntry

entry localUByteArrayType
	; get ptr to byte var into eax
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	add	eax, [edx]		; add in array index on TOS
	add	edx, 4
	jmp	ubyteEntry

entry fieldByteArrayType
	; get ptr to byte var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [edx]
	add	eax, [edx+4]
	add	edx, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	byteEntry

entry fieldUByteArrayType
	; get ptr to byte var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [edx]
	add	eax, [edx+4]
	add	edx, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	ubyteEntry

entry memberByteArrayType
	; get ptr to byte var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [ebp + FCore.TPtr]
	add	eax, [edx]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	byteEntry

entry memberUByteArrayType
	; get ptr to byte var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [ebp + FCore.TPtr]
	add	eax, [edx]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	ubyteEntry

;-----------------------------------------------
;
; local short ops
;
entry localShortType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz localShortType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
localShortType1:
	; get ptr to short var into eax
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 001FFFFFh
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
shortEntry:
	mov	ebx, [ebp + FCore.varMode]
	or	ebx, ebx
	jnz	localShort1
	; fetch local short
localShortFetch:
	sub	edx, 4
	movsx	ebx, WORD[eax]
	mov	[edx], ebx
	jmp	edi

entry localUShortType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz localUShortType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
localUShortType1:
	; get ptr to short var into eax
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 001FFFFFh
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
ushortEntry:
	mov	ebx, [ebp + FCore.varMode]
	or	ebx, ebx
	jnz	localShort1
	; fetch local unsigned short
localUShortFetch:
	sub	edx, 4
	movsx	ebx, WORD[eax]
	xor	ebx, ebx
	mov	bx, [eax]
	mov	[edx], ebx
	jmp	edi

localShort1:
	cmp	ebx, kVarMinusStore
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, [localShortActionTable + ebx*4]
	jmp	ebx
localShortRef:
	sub	edx, 4
	mov	[edx], eax
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	jmp	edi
	
localShortStore:
	mov	ebx, [edx]
	mov	[eax], bx
	add	edx, 4
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	jmp	edi

localShortPlusStore:
	movsx	ebx, WORD[eax]
	add	ebx, [edx]
	mov	[eax], bx
	add	edx, 4
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	jmp	edi

localShortMinusStore:
	movsx	ebx, WORD[eax]
	sub	ebx, [edx]
	mov	[eax], bx
	add	edx, 4
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	jmp	edi

localShortActionTable:
	DD	localShortFetch
	DD	localShortFetch
	DD	localShortRef
	DD	localShortStore
	DD	localShortPlusStore
	DD	localShortMinusStore

localUShortActionTable:
	DD	localUShortFetch
	DD	localUShortFetch
	DD	localShortRef
	DD	localShortStore
	DD	localShortPlusStore
	DD	localShortMinusStore

entry fieldShortType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz fieldShortType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
fieldShortType1:
	; get ptr to short var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 001FFFFFh
	add	eax, ebx
	jmp	shortEntry

entry fieldUShortType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz fieldUShortType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
fieldUShortType1:
	; get ptr to unsigned short var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 001FFFFFh
	add	eax, ebx
	jmp	ushortEntry

entry memberShortType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz memberShortType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
memberShortType1:
	; get ptr to short var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [ebp + FCore.TPtr]
	and	ebx, 001FFFFFh
	add	eax, ebx
	jmp	shortEntry

entry memberUShortType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz memberUShortType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
memberUShortType1:
	; get ptr to unsigned short var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [ebp + FCore.TPtr]
	and	ebx, 001FFFFFh
	add	eax, ebx
	jmp	ushortEntry

entry localShortArrayType
	; get ptr to int var into eax
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 00FFFFFFh	; ebx is frame offset in longs
	sal	ebx, 2
	sub	eax, ebx
	mov	ebx, [edx]		; add in array index on TOS
	add	edx, 4
	sal	ebx, 1
	add	eax, ebx
	jmp	shortEntry

entry localUShortArrayType
	; get ptr to int var into eax
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 00FFFFFFh	; ebx is frame offset in longs
	sal	ebx, 2
	sub	eax, ebx
	mov	ebx, [edx]		; add in array index on TOS
	add	edx, 4
	sal	ebx, 1
	add	eax, ebx
	jmp	ushortEntry

entry fieldShortArrayType
	; get ptr to short var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [edx+4]	; eax = index
	sal	eax, 1
	add	eax, [edx]		; add in struct base ptr
	add	edx, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	shortEntry

entry fieldUShortArrayType
	; get ptr to short var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [edx+4]	; eax = index
	sal	eax, 1
	add	eax, [edx]		; add in struct base ptr
	add	edx, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	ushortEntry

entry memberShortArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [edx]	; eax = index
	sal	eax, 1
	add	eax, [ebp + FCore.TPtr]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	shortEntry

entry memberUShortArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [edx]	; eax = index
	sal	eax, 1
	add	eax, [ebp + FCore.TPtr]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	ushortEntry


;-----------------------------------------------
;
; local int ops
;
entry localIntType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz localIntType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
localIntType1:
	; get ptr to float var into eax
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 001FFFFFh
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
intEntry:
	mov	ebx, [ebp + FCore.varMode]
	or	ebx, ebx
	jnz	localInt1
	; fetch local int
localIntFetch:
	sub	edx, 4
	mov	ebx, [eax]
	mov	[edx], ebx
	jmp	edi

localIntRef:
	sub	edx, 4
	mov	[edx], eax
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	jmp	edi
	
localIntStore:
	mov	ebx, [edx]
	mov	[eax], ebx
	add	edx, 4
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	jmp	edi

localIntPlusStore:
	mov	ebx, [eax]
	add	ebx, [edx]
	mov	[eax], ebx
	add	edx, 4
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	jmp	edi

localIntMinusStore:
	mov	ebx, [eax]
	sub	ebx, [edx]
	mov	[eax], ebx
	add	edx, 4
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	jmp	edi

localIntActionTable:
	DD	localIntFetch
	DD	localIntFetch
	DD	localIntRef
	DD	localIntStore
	DD	localIntPlusStore
	DD	localIntMinusStore

localInt1:
	cmp	ebx, kVarMinusStore
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, [localIntActionTable + ebx*4]
	jmp	ebx

entry fieldIntType
	; get ptr to int var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz fieldIntType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
fieldIntType1:
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 001FFFFFh
	add	eax, ebx
	jmp	intEntry

entry memberIntType
	; get ptr to int var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz memberIntType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
memberIntType1:
	mov	eax, [ebp + FCore.TPtr]
	and	ebx, 001FFFFFh
	add	eax, ebx
	jmp	intEntry

entry localIntArrayType
	; get ptr to int var into eax
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 00FFFFFFh
	sub	ebx, [edx]		; add in array index on TOS
	add	edx, 4
	sal	ebx, 2
	sub	eax, ebx
	jmp	intEntry

entry fieldIntArrayType
	; get ptr to int var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [edx+4]	; eax = index
	sal	eax, 2
	add	eax, [edx]		; add in struct base ptr
	add	edx, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	intEntry

entry memberIntArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [edx]	; eax = index
	sal	eax, 2
	add	eax, [ebp + FCore.TPtr]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	intEntry

;-----------------------------------------------
;
; local float ops
;
entry localFloatType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz localFloatType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
localFloatType1:
	; get ptr to float var into eax
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 001FFFFFh
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
floatEntry:
	mov	ebx, [ebp + FCore.varMode]
	or	ebx, ebx
	jnz	localFloat1
	; fetch local float
localFloatFetch:
	sub	edx, 4
	mov	ebx, [eax]
	mov	[edx], ebx
	jmp	edi

localFloatRef:
	sub	edx, 4
	mov	[edx], eax
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	jmp	edi
	
localFloatStore:
	mov	ebx, [edx]
	mov	[eax], ebx
	add	edx, 4
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	jmp	edi

localFloatPlusStore:
	fld	DWORD[eax]
	; set var operation back to fetch
	xor	ebx, ebx
	mov	[ebp + FCore.varMode], ebx
	fadd	DWORD[edx]
	add	edx, 4
	fstp	DWORD[eax]
	jmp	edi

localFloatMinusStore:
	fld	DWORD[eax]
	; set var operation back to fetch
	xor	ebx, ebx
	mov	[ebp + FCore.varMode], ebx
	fsub	DWORD[edx]
	add	edx, 4
	fstp	DWORD[eax]
	jmp	edi

localFloatActionTable:
	DD	localFloatFetch
	DD	localFloatFetch
	DD	localFloatRef
	DD	localFloatStore
	DD	localFloatPlusStore
	DD	localFloatMinusStore

localFloat1:
	cmp	ebx, kVarMinusStore
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, [localFloatActionTable + ebx*4]
	jmp	ebx

entry fieldFloatType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz fieldFloatType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
fieldFloatType1:
	; get ptr to float var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 001FFFFFh
	add	eax, ebx
	jmp	floatEntry

entry memberFloatType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz memberFloatType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
memberFloatType1:
	; get ptr to float var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [ebp + FCore.TPtr]
	and	ebx, 001FFFFFh
	add	eax, ebx
	jmp	floatEntry

entry localFloatArrayType
	; get ptr to float var into eax
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 00FFFFFFh
	sub	ebx, [edx]		; add in array index on TOS
	add	edx, 4
	sal	ebx, 2
	sub	eax, ebx
	jmp	floatEntry

entry fieldFloatArrayType
	; get ptr to float var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [edx+4]	; eax = index
	sal	eax, 2
	add	eax, [edx]		; add in struct base ptr
	add	edx, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	floatEntry

entry memberFloatArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [edx]	; eax = index
	sal	eax, 2
	add	eax, [ebp + FCore.TPtr]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	floatEntry
	
;-----------------------------------------------
;
; local double ops
;
entry localDoubleType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz localDoubleType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
localDoubleType1:
	; get ptr to double var into eax
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 001FFFFFh
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
doubleEntry:
	mov	ebx, [ebp + FCore.varMode]
	or	ebx, ebx
	jnz	localDouble1
	; fetch local double
localDoubleFetch:
	sub	edx, 8
	mov	ebx, [eax]
	mov	[edx], ebx
	mov	ebx, [eax+4]
	mov	[edx+4], ebx
	jmp	edi

localDoubleRef:
	sub	edx, 4
	mov	[edx], eax
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	jmp	edi
	
localDoubleStore:
	mov	ebx, [edx]
	mov	[eax], ebx
	mov	ebx, [edx+4]
	mov	[eax+4], ebx
	add	edx, 8
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	jmp	edi

localDoublePlusStore:
	fld	QWORD[eax]
	; set var operation back to fetch
	xor	ebx, ebx
	mov	[ebp + FCore.varMode], ebx
	fadd	QWORD[edx]
	add	edx, 8
	fstp	QWORD[eax]
	jmp	edi

localDoubleMinusStore:
	fld	QWORD[eax]
	; set var operation back to fetch
	xor	ebx, ebx
	mov	[ebp + FCore.varMode], ebx
	fsub	QWORD[edx]
	add	edx, 8
	fstp	QWORD[eax]
	jmp	edi

localDoubleActionTable:
	DD	localDoubleFetch
	DD	localDoubleFetch
	DD	localDoubleRef
	DD	localDoubleStore
	DD	localDoublePlusStore
	DD	localDoubleMinusStore

localDouble1:
	cmp	ebx, kVarMinusStore
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, [localDoubleActionTable + ebx*4]
	jmp	ebx

entry fieldDoubleType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz fieldDoubleType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
fieldDoubleType1:
	; get ptr to double var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 001FFFFFh
	add	eax, ebx
	jmp	doubleEntry

entry memberDoubleType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz memberDoubleType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
memberDoubleType1:
	; get ptr to double var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [ebp + FCore.TPtr]
	and	ebx, 001FFFFFh
	add	eax, ebx
	jmp	doubleEntry

entry localDoubleArrayType
	; get ptr to double var into eax
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	mov	ebx, [edx]		; add in array index on TOS
	add	edx, 4
	sal	ebx, 3
	add	eax, ebx
	jmp doubleEntry

entry fieldDoubleArrayType
	; get ptr to double var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [edx+4]	; eax = index
	sal	eax, 3
	add	eax, [edx]		; add in struct base ptr
	add	edx, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	doubleEntry

entry memberDoubleArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [edx]	; eax = index
	sal	eax, 3
	add	eax, [ebp + FCore.TPtr]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	doubleEntry
	
;-----------------------------------------------
;
; local string ops
;
GLOBAL localStringType, stringEntry, localStringFetch, localStringStore, localStringAppend
entry localStringType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz localStringType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
localStringType1:
	; get ptr to string var into eax
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 001FFFFFh
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
stringEntry:
	mov	ebx, [ebp + FCore.varMode]
	or	ebx, ebx
	jnz	localString1
	; fetch local string
localStringFetch:
	sub	edx, 4
	add	eax, 8		; skip maxLen & currentLen fields
	mov	[edx], eax
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	jmp	edi

localString1:
	cmp	ebx, kVarPlusStore
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, [localStringActionTable + ebx*4]
	jmp	ebx
	
; ref on a string returns the address of maxLen field, not the string chars
localStringRef:
	sub	edx, 4
	mov	[edx], eax
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	jmp	edi
	
localStringStore:
	; eax -> dest string maxLen field
	; TOS is src string addr
	mov	[ebp + FCore.IPtr], esi	; IP will get stomped
	mov	ecx, [edx]			; ecx -> chars of src string
	add	edx, 4
	mov	[ebp + FCore.SPtr], edx
	lea	edi, [eax + 8]		; edi -> chars of dst string
    sub esp, 4              ; 16-byte align for mac
	push	ecx				; strlen will stomp on ecx
	push	ecx
	xcall	strlen
	mov ecx, [esp + 4]
    add esp, 12
	; eax is src string length

	; figure how many chars to copy
	mov	ebx, [edi - 8]		; ebx = max string length
	cmp	eax, ebx
	jle lsStore1
	mov	eax, ebx
lsStore1:
	; set current length field
	mov	[edi - 4], eax
	
	; do the copy
	sub esp, 12      ; thanks OSX!
	push	eax		; push numBytes
	push	eax		; and save a copy in case strncpy modifies its stack inputs
	push	ecx		; srcPtr
	push	edi		; dstPtr
	xcall	strncpy
	add esp, 12
	pop	esi			; esi = numBytes
    	add esp, 12

	; add the terminating null
	xor	eax, eax
	mov	[edi + esi], al
		
	; set var operation back to fetch
	mov	[ebp + FCore.varMode], eax
	jmp	interpFunc

localStringAppend:
	; eax -> dest string maxLen field
	; TOS is src string addr
	mov	[ebp + FCore.IPtr], esi	; IP will get stomped
	mov	ecx, [edx]			; ecx -> chars of src string
	add	edx, 4
	mov	[ebp + FCore.SPtr], edx
	lea	edi, [eax + 8]		; edi -> chars of dst string
    sub esp, 8              ; 16-byte align for mac
	push	ecx
	xcall	strlen
	add	esp, 12
	; eax is src string length

	; figure how many chars to copy
	mov	ebx, [edi - 8]		; ebx = max string length
	mov	esi, [edi - 4]		; esi = cur string length
	add	esi, eax
	cmp	esi, ebx
	jle lsAppend1
	mov	eax, ebx
	sub	eax, [edi - 4]		; #bytes to copy = maxLen - curLen
	mov	esi, ebx			; new curLen = maxLen
lsAppend1:
	; set current length field
	mov	[edi - 4], esi
	
	; do the append
	push	eax		; push numBytes
	push	ecx		; srcPtr
	push	edi		; dstPtr
	; don't need to worry about stncat stomping registers since we jump to interpFunc
	xcall	strncat
	add	esp, 12

	; add the terminating null
	xor	eax, eax
	mov	esi, [edi - 4]
	mov	[edi + esi], al
		
	; set var operation back to fetch
	mov	[ebp + FCore.varMode], eax
	jmp	interpFunc

localStringActionTable:
	DD	localStringFetch
	DD	localStringFetch
	DD	localStringRef
	DD	localStringStore
	DD	localStringAppend

entry fieldStringType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz fieldStringType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
fieldStringType1:
	; get ptr to byte var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 001FFFFFh
	add	eax, ebx
	jmp	stringEntry

entry memberStringType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz memberStringType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
memberStringType1:
	; get ptr to byte var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [ebp + FCore.TPtr]
	and	ebx, 001FFFFFh
	add	eax, ebx
	jmp	stringEntry

entry localStringArrayType
	; get ptr to int var into eax
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx		; eax -> maxLen field of string[0]
	mov	ebx, [eax]
	sar	ebx, 2
	add	ebx, 3			; ebx is element length in longs
	imul	ebx, [edx]	; mult index * element length
	add	edx, 4
	sal	ebx, 2			; ebx is offset in bytes
	add	eax, ebx
	jmp stringEntry

entry fieldStringArrayType
	; get ptr to string var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	and	ebx, 00FFFFFFh
	add	ebx, [edx]		; ebx -> maxLen field of string[0]
	mov	eax, [ebx]		; eax = maxLen
	sar	eax, 2
	add	eax, 3			; eax is element length in longs
	imul	eax, [edx+4]	; mult index * element length
	sal	eax, 2
	add	eax, ebx		; eax -> maxLen field of string[N]
	add	edx, 8
	jmp	stringEntry

entry memberStringArrayType
	; get ptr to string var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	and	ebx, 00FFFFFFh
	add	ebx, [ebp + FCore.TPtr]	; ebx -> maxLen field of string[0]
	mov	eax, [ebx]		; eax = maxLen
	sar	eax, 2
	add	eax, 3			; eax is element length in longs
	imul	eax, [edx]	; mult index * element length
	sal	eax, 2
	add	eax, ebx		; eax -> maxLen field of string[N]
	add	edx, 4
	jmp	stringEntry

;-----------------------------------------------
;
; local op ops
;
entry localOpType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz localOpType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
localOpType1:
	; get ptr to op var into ebx
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 001FFFFFh
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch (execute for ops)
opEntry:
	mov	ebx, [ebp + FCore.varMode]
	or	ebx, ebx
	jnz	localOp1
	; execute local op
localOpExecute:
	mov	ebx, [eax]
	mov	eax, [ebp + FCore.innerExecute]
	jmp eax

localOpFetch:
	sub	edx, 4
	mov	ebx, [eax]
	mov	[edx], ebx
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	jmp	edi

localOpActionTable:
	DD	localOpExecute
	DD	localOpFetch
	DD	localIntRef
	DD	localIntStore

localOp1:
	cmp	ebx, kVarStore
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, [localOpActionTable + ebx*4]
	jmp	ebx

entry fieldOpType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz fieldOpType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
fieldOpType1:
	; get ptr to op var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 001FFFFFh
	add	eax, ebx
	jmp	opEntry

entry memberOpType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz memberOpType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
memberOpType1:
	; get ptr to op var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [ebp + FCore.TPtr]
	and	ebx, 001FFFFFh
	add	eax, ebx
	jmp	opEntry

entry localOpArrayType
	; get ptr to op var into eax
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 00FFFFFFh
	sub	ebx, [edx]		; add in array index on TOS
	add	edx, 4
	sal	ebx, 2
	sub	eax, ebx
	jmp	opEntry

entry fieldOpArrayType
	; get ptr to op var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [edx+4]	; eax = index
	sal	eax, 2
	add	eax, [edx]		; add in struct base ptr
	add	edx, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	opEntry

entry memberOpArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [edx]	; eax = index
	sal	eax, 2
	add	eax, [ebp + FCore.TPtr]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	opEntry
	
;-----------------------------------------------
;
; local long (int64) ops
;
entry localLongType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz localLongType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
localLongType1:
	; get ptr to long var into eax
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 001FFFFFh
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
longEntry:
	mov	ebx, [ebp + FCore.varMode]
	or	ebx, ebx
	jnz	localLong1
	; fetch local double
localLongFetch:
	sub	edx, 8
	mov	ebx, [eax]
	mov	[edx+4], ebx
	mov	ebx, [eax+4]
	mov	[edx], ebx
	jmp	edi

localLong1:
	cmp	ebx, kVarMinusStore
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, [localLongActionTable + ebx*4]
	jmp	ebx

localLongRef:
	sub	edx, 4
	mov	[edx], eax
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	jmp	edi
	
localLongStore:
	mov	ebx, [edx]
	mov	[eax+4], ebx
	mov	ebx, [edx+4]
	mov	[eax], ebx
	add	edx, 8
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	jmp	edi

localLongPlusStore:
	mov	ebx, [eax]
	add	ebx, [edx+4]
	mov	[eax], ebx
	mov	ebx, [eax+4]
	adc	ebx, [edx]
	mov	[eax+4], ebx
	; set var operation back to fetch
	xor	ebx, ebx
	mov	[ebp + FCore.varMode], ebx
	add	edx, 8
	jmp	edi

localLongMinusStore:
	mov	ebx, [eax]
	sub	ebx, [edx+4]
	mov	[eax], ebx
	mov	ebx, [eax+4]
	sbb	ebx, [edx]
	mov	[eax+4], ebx
	; set var operation back to fetch
	xor	ebx, ebx
	mov	[ebp + FCore.varMode], ebx
	add	edx, 8
	jmp	edi

localLongActionTable:
	DD	localLongFetch
	DD	localLongFetch
	DD	localLongRef
	DD	localLongStore
	DD	localLongPlusStore
	DD	localLongMinusStore

entry fieldLongType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz fieldLongType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
fieldLongType1:
	; get ptr to double var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 001FFFFFh
	add	eax, ebx
	jmp	longEntry

entry memberLongType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz memberLongType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
memberLongType1:
	; get ptr to double var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [ebp + FCore.TPtr]
	and	ebx, 001FFFFFh
	add	eax, ebx
	jmp	longEntry

entry localLongArrayType
	; get ptr to double var into eax
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	mov	ebx, [edx]		; add in array index on TOS
	add	edx, 4
	sal	ebx, 3
	add	eax, ebx
	jmp longEntry

entry fieldLongArrayType
	; get ptr to double var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [edx+4]	; eax = index
	sal	eax, 3
	add	eax, [edx]		; add in struct base ptr
	add	edx, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	longEntry

entry memberLongArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [edx]	; eax = index
	sal	eax, 3
	add	eax, [ebp + FCore.TPtr]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	longEntry
	
;-----------------------------------------------
;
; local object ops
;
entry localObjectType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz localObjectType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
localObjectType1:
	; get ptr to Object var into eax
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 001FFFFFh
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
objectEntry:
	mov	ebx, [ebp + FCore.varMode]
	or	ebx, ebx
	jnz	localObject1
	; fetch local Object
localObjectFetch:
	sub	edx, 4
	mov	ebx, [eax]
	mov	[edx], ebx
	jmp	edi

localObject1:
	cmp	ebx, kVarObjectClear
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, [localObjectActionTable + ebx*4]
	jmp	ebx

localObjectRef:
	sub	edx, 4
	mov	[edx], eax
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	jmp	edi
	
localObjectStore:
	; TOS is new object, eax points to destination/old object
	xor	ebx, ebx			; set var operation back to default/fetch
	mov	[ebp + FCore.varMode], ebx
los0:
	mov ebx, [eax]		; ebx = olbObj
	mov ecx, eax		; ecx -> destination
	mov eax, [edx]		; eax = newObj
	cmp eax, ebx
	jz losx				; objects are same, don't change refcount
	; handle newObj refcount
	or eax, eax
	jz los1				; if newObj is null, don't try to increment refcount
	inc dword[eax + Object.refCount]	; increment newObj refcount
	; handle oldObj refcount
los1:
	or ebx, ebx
	jz los2				; if oldObj is null, don't try to decrement refcount
	dec dword[ebx + Object.refCount]
	jz los3
los2:
	mov	[ecx], eax		; var = newObj
losx:
	add	edx, 4
	jmp	edi

	; object var held last reference to oldObj, invoke olbObj.delete method
	; eax = newObj
	; ebx = oldObj
	; [ecx] = var (still holds oldObj)
los3:
	push edi
	push eax
	; TOS is object

	; push this ptr on return stack
	mov	edi, [ebp + FCore.RPtr]
	sub	edi, 4
	mov	[ebp + FCore.RPtr], edi
	mov	eax, [ebp + FCore.TPtr]
	mov	[edi], eax
	
	; set this to oldObj
	mov	[ebp + FCore.TPtr], ebx
	mov	ebx, [ebx]	; ebx = oldObj methods pointer
	mov	ebx, [ebx]	; ebx = oldObj method 0 (delete)

	pop eax
	pop edi
	
	mov	[ecx], eax		; var = newObj
	add	edx, 4

	; execute the delete method opcode which is in ebx
	mov	eax, [ebp + FCore.innerExecute]
	jmp eax

localObjectClear:
	; TOS is new object, eax points to destination/old object
	xor	ebx, ebx			; set var operation back to default/fetch
	mov	[ebp + FCore.varMode], ebx
	sub	edx, 4
	mov [edx], ebx
	; for now, do the clear operation by pushing null on TOS then doing a regular object store
	; later on optimize it since we know source is a null
	jmp	los0

; store object on TOS in variable pointed to by eax
; do not adjust reference counts of old object or new object
localObjectStoreNoRef:
	; TOS is new object, eax points to destination/old object
	xor	ebx, ebx			; set var operation back to default/fetch
	mov	[ebp + FCore.varMode], ebx
	mov	ebx, [edx]
	mov	[eax], ebx
	add	edx, 4
	jmp	edi

; clear object reference, leave on TOS
localObjectUnref:
	; leave object on TOS
	sub	edx, 4
	mov	ebx, [eax]
	mov	[edx], ebx
	; if object var is already null, do nothing else
	or	ebx, ebx
	jz	lou2
	; clear object var
	mov	ecx, eax		; ecx -> object var
	xor	eax, eax
	mov	[ecx], eax
	; set var operation back to fetch
	mov	[ebp + FCore.varMode], eax
	; get object refcount, see if it is already 0
	mov	eax, [ebx + Object.refCount]
	or	eax, eax
	jnz	lou1
	; report refcount negative error
	mov	eax, kForthErrorBadReferenceCount
	jmp	interpLoopErrorExit
lou1:
	; decrement object refcount
	sub	eax, 1
	mov	[ebx + Object.refCount], eax
lou2:
	jmp	edi

	
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
	and	eax, 00E00000h
	jz fieldObjectType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
fieldObjectType1:
	; get ptr to Object var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 001FFFFFh
	add	eax, ebx
	jmp	objectEntry

entry memberObjectType
	mov	eax, ebx
	; see if a varop is specified
	and	eax, 00E00000h
	jz memberObjectType1
	shr	eax, 21
	mov	[ebp + FCore.varMode], eax
memberObjectType1:
	; get ptr to Object var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [ebp + FCore.TPtr]
	and	ebx, 001FFFFFh
	add	eax, ebx
	jmp	objectEntry

entry localObjectArrayType
	; get ptr to Object var into eax
	mov	eax, [ebp + FCore.FPtr]
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	mov	ebx, [edx]		; add in array index on TOS
	add	edx, 4
	sal	ebx, 3
	add	eax, ebx
	jmp objectEntry

entry fieldObjectArrayType
	; get ptr to Object var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [edx+4]	; eax = index
	sal	eax, 3
	add	eax, [edx]		; add in struct base ptr
	add	edx, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	objectEntry

entry memberObjectArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [edx]	; eax = index
	sal	eax, 3
	add	eax, [ebp + FCore.TPtr]
	add	edx, 4
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
	mov	ecx, [ebp + FCore.RPtr]
	sub	ecx, 4
	mov	[ebp + FCore.RPtr], ecx
	mov	eax, [ebp + FCore.TPtr]
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
	mov	eax, [ebp + FCore.innerExecute]
	jmp eax
	
; invoke a method on an object referenced by ptr pair on TOS
entry methodWithTOSType
	; TOS is object
	; ebx is method number
	; push current this on return stack
	mov	ecx, [ebp + FCore.RPtr]
	sub	ecx, 4
	mov	[ebp + FCore.RPtr], ecx
	mov	eax, [ebp + FCore.TPtr]
	mov	[ecx], eax

	; set this ptr from TOS	
	mov	eax, [edx]
	or	eax, eax
	jnz methodTos1
	mov	eax, kForthErrorBadObject
	jmp	interpLoopErrorExit
methodTos1:
	mov	[ebp + FCore.TPtr], eax
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	add	ebx, [eax]
	mov	ebx, [ebx]	; ebx = method opcode
	add	edx, 4
	mov	eax, [ebp + FCore.innerExecute]
	jmp eax
	
; invoke a method on super class of object currently referenced by this ptr pair
entry methodWithSuperType
	; ebx is method number
	mov	ecx, [ebp + FCore.RPtr]
	sub	ecx, 8
	mov	[ebp + FCore.RPtr], ecx
	mov	eax, [ebp + FCore.TPtr]
    push ebx
	; push original methods ptr on return stack
	mov	ebx, [eax]			; ebx is original methods ptr
	mov	[ecx+4], ebx
	; push this on return stack
	mov	[ecx], eax
	
	mov	ecx, [ebx-4]		; ecx -> class vocabulary object
	mov	eax, [ecx+8]		; eax -> class vocabulary
	push edx
    push ecx
    sub esp, 12         ; 16-byte align for OSX
    push eax            ; class vocabulary
	xcall getSuperClassMethods
	; eax -> super class methods table
	mov	ebx, [ebp + FCore.TPtr]
	mov	[ebx], eax		; set this methods ptr to super class methods
	add esp, 16
	pop ecx
	pop edx
	pop ebx
	; ebx is method number
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	add	ebx, eax
	mov	ebx, [ebx]	; ebx = method opcode
	mov	eax, [ebp + FCore.innerExecute]
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
	mov	ecx, [ebp + FCore.TPtr]
	add	ecx, eax						; ecx -> max length field
	and	ebx, 00000FFFh					; ebx = max length
	mov	[ecx], ebx						; set max length
	xor	eax, eax
	mov	[ecx+4], eax					; set current length to 0
	mov	[ecx+8], al						; add terminating null
	jmp	edi

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
	mov	eax, esi
	; pop rstack
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	byteEntry

;-----------------------------------------------
;
; doUByteOp is compiled as the first op in global unsigned byte vars
; the byte data field is immediately after this op
;
entry doUByteBop
	; get ptr to byte var into eax
	mov	eax, esi
	; pop rstack
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	ubyteEntry

;-----------------------------------------------
;
; doByteArrayOp is compiled as the first op in global byte arrays
; the data array is immediately after this op
;
entry doByteArrayBop
	; get ptr to byte var into eax
	mov	eax, esi
	add	eax, [edx]
	add	edx, 4
	; pop rstack
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	byteEntry

entry doUByteArrayBop
	; get ptr to byte var into eax
	mov	eax, esi
	add	eax, [edx]
	add	edx, 4
	; pop rstack
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	ubyteEntry

;-----------------------------------------------
;
; doShortOp is compiled as the first op in global short vars
; the short data field is immediately after this op
;
entry doShortBop
	; get ptr to short var into eax
	mov	eax, esi
	; pop rstack
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	shortEntry

;-----------------------------------------------
;
; doUShortOp is compiled as the first op in global unsigned short vars
; the short data field is immediately after this op
;
entry doUShortBop
	; get ptr to short var into eax
	mov	eax, esi
	; pop rstack
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	ushortEntry

;-----------------------------------------------
;
; doShortArrayOp is compiled as the first op in global short arrays
; the data array is immediately after this op
;
entry doShortArrayBop
	; get ptr to short var into eax
	mov	eax, esi
	mov	ebx, [edx]		; ebx = array index
	add	edx, 4
	sal	ebx, 1
	add	eax, ebx	
	; pop rstack
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	shortEntry

entry doUShortArrayBop
	; get ptr to short var into eax
	mov	eax, esi
	mov	ebx, [edx]		; ebx = array index
	add	edx, 4
	sal	ebx, 1
	add	eax, ebx	
	; pop rstack
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	ushortEntry

;-----------------------------------------------
;
; doIntOp is compiled as the first op in global int vars
; the int data field is immediately after this op
;
entry doIntBop
	; get ptr to int var into eax
	mov	eax, esi
	; pop rstack
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	intEntry

;-----------------------------------------------
;
; doIntArrayOp is compiled as the first op in global int arrays
; the data array is immediately after this op
;
entry doIntArrayBop
	; get ptr to int var into eax
	mov	eax, esi
	mov	ebx, [edx]		; ebx = array index
	add	edx, 4
	sal	ebx, 2
	add	eax, ebx	
	; pop rstack
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	intEntry

;-----------------------------------------------
;
; doFloatOp is compiled as the first op in global float vars
; the float data field is immediately after this op
;
entry doFloatBop
	; get ptr to float var into eax
	mov	eax, esi
	; pop rstack
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	floatEntry

;-----------------------------------------------
;
; doFloatArrayOp is compiled as the first op in global float arrays
; the data array is immediately after this op
;
entry doFloatArrayBop
	; get ptr to float var into eax
	mov	eax, esi
	mov	ebx, [edx]		; ebx = array index
	add	edx, 4
	sal	ebx, 2
	add	eax, ebx	
	; pop rstack
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	floatEntry

;-----------------------------------------------
;
; doDoubleOp is compiled as the first op in global double vars
; the data field is immediately after this op
;
entry doDoubleBop
	; get ptr to double var into eax
	mov	eax, esi
	; pop rstack
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	doubleEntry

;-----------------------------------------------
;
; doDoubleArrayOp is compiled as the first op in global double arrays
; the data array is immediately after this op
;
entry doDoubleArrayBop
	; get ptr to double var into eax
	mov	eax, esi
	mov	ebx, [edx]		; ebx = array index
	add	edx, 4
	sal	ebx, 3
	add	eax, ebx	
	; pop rstack
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	doubleEntry

;-----------------------------------------------
;
; doStringOp is compiled as the first op in global string vars
; the data field is immediately after this op
;
entry doStringBop
	; get ptr to string var into eax
	mov	eax, esi
	; pop rstack
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	stringEntry

;-----------------------------------------------
;
; doStringArrayOp is compiled as the first op in global string arrays
; the data array is immediately after this op
;
entry doStringArrayBop
	; get ptr to string var into eax
	mov	eax, esi		; eax -> maxLen field of string[0]
	mov	ebx, [eax]		; ebx = maxLen
	sar	ebx, 2
	add	ebx, 3			; ebx is element length in longs
	imul	ebx, [edx]	; mult index * element length
	add	edx, 4
	sal	ebx, 2			; ebx is offset in bytes
	add	eax, ebx
	; pop rstack
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp stringEntry

;-----------------------------------------------
;
; doOpOp is compiled as the first op in global op vars
; the op data field is immediately after this op
;
entry doOpBop
	; get ptr to int var into eax
	mov	eax, esi
	; pop rstack
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	opEntry

;-----------------------------------------------
;
; doOpArrayOp is compiled as the first op in global op arrays
; the data array is immediately after this op
;
entry doOpArrayBop
	; get ptr to op var into eax
	mov	eax, esi
	mov	ebx, [edx]		; ebx = array index
	add	edx, 4
	sal	ebx, 2
	add	eax, ebx	
	; pop rstack
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	opEntry

;-----------------------------------------------
;
; doLongOp is compiled as the first op in global int64 vars
; the data field is immediately after this op
;
entry doLongBop
	; get ptr to double var into eax
	mov	eax, esi
	; pop rstack
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	longEntry

;-----------------------------------------------
;
; doLongArrayOp is compiled as the first op in global int64 arrays
; the data array is immediately after this op
;
entry doLongArrayBop
	; get ptr to double var into eax
	mov	eax, esi
	mov	ebx, [edx]		; ebx = array index
	add	edx, 4
	sal	ebx, 3
	add	eax, ebx	
	; pop rstack
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	longEntry

;-----------------------------------------------
;
; doObjectOp is compiled as the first op in global Object vars
; the data field is immediately after this op
;
entry doObjectBop
	; get ptr to Object var into eax
	mov	eax, esi
	; pop rstack
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	objectEntry

;-----------------------------------------------
;
; doObjectArrayOp is compiled as the first op in global Object arrays
; the data array is immediately after this op
;
entry doObjectArrayBop
	; get ptr to Object var into eax
	mov	eax, esi
	mov	ebx, [edx]		; ebx = array index
	add	edx, 4
	sal	ebx, 3
	add	eax, ebx	
	; pop rstack
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	objectEntry

;========================================

entry initStringBop
	;	TOS: len strPtr
	mov	ebx, [edx+4]	; ebx -> first char of string
	xor	eax, eax
	mov	[ebx-4], eax	; set current length = 0
	mov	[ebx], al		; set first char to terminating null
	mov	eax, [edx]		; eax == string length
	mov	[ebx-8], eax	; set maximum length field
	add	edx, 8
	jmp	edi

;========================================

entry strFixupBop
	mov	eax, [edx]
	add	edx, 4
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
	jmp	edi

;========================================

entry doneBop
	mov	eax,kResultDone
	jmp	interpLoopExit

;========================================

entry abortBop
	mov	eax,kForthErrorAbort
	jmp	interpLoopFatalErrorExit

;========================================

entry noopBop
	jmp	edi

;========================================
	
entry plusBop
	mov	eax, [edx]
	add	edx, 4
	add	eax, [edx]
	mov	[edx], eax
	jmp	edi

;========================================
	
entry minusBop
	mov	eax, [edx]
	add	edx, 4
	mov	ebx, [edx]
	sub	ebx, eax
	mov	[edx], ebx
	jmp	edi

;========================================

entry timesBop
	mov	eax, [edx]
	add	edx, 4
	imul	eax, [edx]
	mov	[edx], eax
	jmp	edi

;========================================
	
entry times2Bop
	mov	eax, [edx]
	add	eax, eax
	mov	[edx], eax
	jmp	edi

;========================================
	
entry times4Bop
	mov	eax, [edx]
	sal	eax, 2
	mov	[edx], eax
	jmp	edi

;========================================
	
entry times8Bop
	mov	eax, [edx]
	sal	eax, 3
	mov	[edx], eax
	jmp	edi

;========================================
	
entry divideBop
	; idiv takes 64-bit numerator in edx:eax
	mov	ebx, edx
	mov	eax, [edx+4]	; get numerator
	cdq					; convert into 64-bit in edx:eax
	idiv	DWORD[ebx]		; eax is quotient, edx is remainder
	add	ebx, 4
	mov	[ebx], eax
	mov	edx, ebx
	jmp	edi

;========================================

entry divide2Bop
	mov	eax, [edx]
	sar	eax, 1
	mov	[edx], eax
	jmp	edi
	
;========================================

entry divide4Bop
	mov	eax, [edx]
	sar	eax, 2
	mov	[edx], eax
	jmp	edi
	
;========================================

entry divide8Bop
	mov	eax, [edx]
	sar	eax, 3
	mov	[edx], eax
	jmp	edi
	
;========================================
	
entry divmodBop
	; idiv takes 64-bit numerator in edx:eax
	mov	ebx, edx
	mov	eax, [edx+4]	; get numerator
	cdq					; convert into 64-bit in edx:eax
	idiv	DWORD[ebx]		; eax is quotient, edx is remainder
	mov	[ebx+4], edx
	mov	[ebx], eax
	mov	edx, ebx
	jmp	edi
	
;========================================
	
entry modBop
	; idiv takes 64-bit numerator in edx:eax
	mov	ebx, edx
	mov	eax, [edx+4]	; get numerator
	cdq					; convert into 64-bit in edx:eax
	idiv	DWORD[ebx]		; eax is quotient, edx is remainder
	add	ebx, 4
	mov	[ebx], edx
	mov	edx, ebx
	jmp	edi
	
;========================================
	
entry negateBop
	mov	eax, [edx]
	neg	eax
	mov	[edx], eax
	jmp	edi
	
;========================================
	
entry fplusBop
	fld	DWORD[edx+4]
	fadd	DWORD[edx]
	add	edx,4
	fstp	DWORD[edx]
	jmp	edi
	
;========================================
	
entry fminusBop
	fld	DWORD[edx+4]
	fsub	DWORD[edx]
	add	edx,4
	fstp	DWORD[edx]
	jmp	edi
	
;========================================
	
entry ftimesBop
	fld	DWORD[edx+4]
	fmul	DWORD[edx]
	add	edx,4
	fstp	DWORD[edx]
	jmp	edi
	
;========================================
	
entry fdivideBop
	fld	DWORD[edx+4]
	fdiv	DWORD[edx]
	add	edx,4
	fstp	DWORD[edx]
	jmp	edi
	
;========================================
	
entry faddBlockBop
	; TOS: num pDst pSrcB pSrcA
	push	edx
	mov	eax, [edx+12]
	mov	ebx, [edx+8]
	mov	ecx, [edx]
	mov	edx, [edx+4]
.faddBlockBop1:
	fld	DWORD[eax]
	fadd	DWORD[ebx]
	fstp	DWORD[edx]
	add eax,4
	add ebx,4
	add	edx,4
	sub ecx,1
	jnz .faddBlockBop1
	pop edx
	add	edx,16
	jmp	edi
	
;========================================
	
entry fsubBlockBop
	; TOS: num pDst pSrcB pSrcA
	push	edx
	mov	eax, [edx+12]
	mov	ebx, [edx+8]
	mov	ecx, [edx]
	mov	edx, [edx+4]
.fsubBlockBop1:
	fld	DWORD[eax]
	fsub	DWORD[ebx]
	fstp	DWORD[edx]
	add eax,4
	add ebx,4
	add	edx,4
	sub ecx,1
	jnz .fsubBlockBop1
	pop edx
	add	edx,16
	jmp	edi
	
;========================================
	
entry fmulBlockBop
	; TOS: num pDst pSrcB pSrcA
	push	edx
	mov	eax, [edx+12]
	mov	ebx, [edx+8]
	mov	ecx, [edx]
	mov	edx, [edx+4]
.fmulBlockBop1:
	fld	DWORD[eax]
	fmul	DWORD[ebx]
	fstp	DWORD[edx]
	add eax,4
	add ebx,4
	add	edx,4
	sub ecx,1
	jnz .fmulBlockBop1
	pop edx
	add	edx,16
	jmp	edi
	
;========================================
	
entry fdivBlockBop
	; TOS: num pDst pSrcB pSrcA
	push	edx
	mov	eax, [edx+12]
	mov	ebx, [edx+8]
	mov	ecx, [edx]
	mov	edx, [edx+4]
.fdivBlockBop1:
	fld	DWORD[eax]
	fdiv	DWORD[ebx]
	fstp	DWORD[edx]
	add eax,4
	add ebx,4
	add	edx,4
	sub ecx,1
	jnz .fdivBlockBop1
	pop edx
	add	edx,16
	jmp	edi
	
;========================================
	
entry fscaleBlockBop
	; TOS: num scale pDst pSrc
	mov	eax, [edx+12]	; pSrc
	mov	ebx, [edx+8]	; pDst
	fld	DWORD[edx + 4]	; scale
	mov	ecx, [edx]		; num
.fscaleBlockBop1:
	fld	DWORD[eax]
	fmul	st0, st1
	fstp	DWORD[ebx]
	add eax,4
	add ebx,4
	sub ecx,1
	jnz .fscaleBlockBop1
	ffree	st0
	add	edx,16
	jmp	edi
	
;========================================
	
entry foffsetBlockBop
	; TOS: num scale pDst pSrc
	mov	eax, [edx+12]	; pSrc
	mov	ebx, [edx+8]	; pDst
	fld	DWORD[edx + 4]	; scale
	mov	ecx, [edx]		; num
.foffsetBlockBop1:
	fld	DWORD[eax]
	fadd	st0, st1
	fstp	DWORD[ebx]
	add eax,4
	add ebx,4
	sub ecx,1
	jnz .foffsetBlockBop1
	ffree	st0
	add	edx,16
	jmp	edi
	
;========================================
	
entry fmixBlockBop
	; TOS: num scale pDst pSrc
	mov	eax, [edx+12]	; pSrc
	mov	ebx, [edx+8]	; pDst
	fld	DWORD[edx + 4]	; scale
	mov	ecx, [edx]		; num
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
	add	edx,16
	jmp	edi
	
;========================================
	
entry daddBlockBop
	; TOS: num pDst pSrcB pSrcA
	push	edx
	mov	eax, [edx+12]
	mov	ebx, [edx+8]
	mov	ecx, [edx]
	mov	edx, [edx+4]
.daddBlockBop1:
	fld	QWORD[eax]
	fadd	QWORD[ebx]
	fstp	QWORD[edx]
	add eax,8
	add ebx,8
	add	edx,8
	sub ecx,1
	jnz .daddBlockBop1
	pop edx
	add	edx,16
	jmp	edi
	
;========================================
	
entry dsubBlockBop
	; TOS: num pDst pSrcB pSrcA
	push	edx
	mov	eax, [edx+12]
	mov	ebx, [edx+8]
	mov	ecx, [edx]
	mov	edx, [edx+4]
.dsubBlockBop1:
	fld	QWORD[eax]
	fsub	QWORD[ebx]
	fstp	QWORD[edx]
	add eax,8
	add ebx,8
	add	edx,8
	sub ecx,1
	jnz .dsubBlockBop1
	pop edx
	add	edx,16
	jmp	edi
	
;========================================
	
entry dmulBlockBop
	; TOS: num pDst pSrcB pSrcA
	push	edx
	mov	eax, [edx+12]
	mov	ebx, [edx+8]
	mov	ecx, [edx]
	mov	edx, [edx+4]
.dmulBlockBop1:
	fld	QWORD[eax]
	fmul	QWORD[ebx]
	fstp	QWORD[edx]
	add eax,8
	add ebx,8
	add	edx,8
	sub ecx,1
	jnz .dmulBlockBop1
	pop edx
	add	edx,16
	jmp	edi
	
;========================================
	
entry ddivBlockBop
	; TOS: num pDst pSrcB pSrcA
	push	edx
	mov	eax, [edx+12]
	mov	ebx, [edx+8]
	mov	ecx, [edx]
	mov	edx, [edx+4]
.ddivBlockBop1:
	fld	QWORD[eax]
	fdiv	QWORD[ebx]
	fstp	QWORD[edx]
	add eax,8
	add ebx,8
	add	edx,8
	sub ecx,1
	jnz .ddivBlockBop1
	pop edx
	add	edx,16
	jmp	edi
	
;========================================
	
entry dscaleBlockBop
	; TOS: num scale pDst pSrc
	mov	eax, [edx+16]	; pSrc
	mov	ebx, [edx+12]	; pDst
	fld	QWORD[edx + 4]	; scale
	mov	ecx, [edx]		; num
.dscaleBlockBop1:
	fld	QWORD[eax]
	fmul	st0, st1
	fstp	QWORD[ebx]
	add eax,8
	add ebx,8
	sub ecx,1
	jnz .dscaleBlockBop1
	ffree	st0
	add	edx,20
	jmp	edi
	
;========================================
	
entry doffsetBlockBop
	; TOS: num scale pDst pSrc
	mov	eax, [edx+16]	; pSrc
	mov	ebx, [edx+12]	; pDst
	fld	QWORD[edx + 4]	; scale
	mov	ecx, [edx]		; num
.doffsetBlockBop1:
	fld	QWORD[eax]
	fadd	st0, st1
	fstp	QWORD[ebx]
	add eax,8
	add ebx,8
	sub ecx,1
	jnz .doffsetBlockBop1
	ffree	st0
	add	edx,20
	jmp	edi
	
;========================================
	
entry dmixBlockBop
	; TOS: num scale pDst pSrc
	mov	eax, [edx+16]	; pSrc
	mov	ebx, [edx+12]	; pDst
	fld	QWORD[edx + 4]	; scale
	mov	ecx, [edx]		; num
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
	add	edx,20
	jmp	edi
	
;========================================
	
entry fEquals0Bop
	fldz
	jmp	fEqualsBop1
	
entry fEqualsBop
	fld	DWORD[edx]
	add	edx, 4
fEqualsBop1:
	fld	DWORD[edx]
	xor	ebx, ebx
	fucompp
	fnstsw	ax
	test	ah, 44h
	jp	fEqualsBop2
	dec	ebx
fEqualsBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry fNotEquals0Bop
	fldz
	jmp	fNotEqualsBop1
	
entry fNotEqualsBop
	fld	DWORD[edx]
	add	edx, 4
fNotEqualsBop1:
	fld	DWORD[edx]
	xor	ebx, ebx
	fucompp
	fnstsw	ax
	test	ah, 44h
	jnp	fNotEqualsBop2
	dec	ebx
fNotEqualsBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry fGreaterThan0Bop
	fldz
	jmp	fGreaterThanBop1
	
entry fGreaterThanBop
	fld	DWORD[edx]
	add	edx, 4
fGreaterThanBop1:
	fld	DWORD[edx]
	xor	ebx, ebx
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 41h
	jne	fGreaterThanBop2
	dec	ebx
fGreaterThanBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry fGreaterEquals0Bop
	fldz
	jmp	fGreaterEqualsBop1
	
entry fGreaterEqualsBop
	fld	DWORD[edx]
	add	edx, 4
fGreaterEqualsBop1:
	fld	DWORD[edx]
	xor	ebx, ebx
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 1
	jne	fGreaterEqualsBop2
	dec	ebx
fGreaterEqualsBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry fLessThan0Bop
	fldz
	jmp	fLessThanBop1
	
entry fLessThanBop
	fld	DWORD[edx]
	add	edx, 4
fLessThanBop1:
	fld	DWORD[edx]
	xor	ebx, ebx
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 5
	jp	fLessThanBop2
	dec	ebx
fLessThanBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry fLessEquals0Bop
	fldz
	jmp	fLessEqualsBop1
	
entry fLessEqualsBop
	fld	DWORD[edx]
	add	edx, 4
fLessEqualsBop1:
	fld	DWORD[edx]
	xor	ebx, ebx
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 41h
	jp	fLessEqualsBop2
	dec	ebx
fLessEqualsBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry fWithinBop
	fld	DWORD[edx+4]
	fld	DWORD[edx+8]
	xor	ebx, ebx
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 41h
	jne	fWithinBop2
	fld	DWORD[edx]
	fld	DWORD[edx+8]
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 5
	jp	fWithinBop2
	dec	ebx
fWithinBop2:
	add	edx, 8
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry fMinBop
	fld	DWORD[edx]
	fld	DWORD[edx+4]
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 41h
	jne	fMinBop2
	mov	eax, [edx]
	mov	[edx+4], eax
fMinBop2:
	add	edx, 4
	jmp	edi
	
;========================================
	
entry fMaxBop
	fld	DWORD[edx]
	fld	DWORD[edx+4]
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 5
	jp	fMaxBop2
	mov	eax, [edx]
	mov	[edx+4], eax
fMaxBop2:
	add	edx, 4
	jmp	edi
	
;========================================
	
entry dcmpBop
	fld	QWORD[edx]
	add	edx, 8
	fld	QWORD[edx]
	add	edx, 4
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
	mov	[edx], ebx
	jmp	edi

;========================================
	
entry fcmpBop
	fld	DWORD[edx]
	add	edx, 4
	fld	DWORD[edx]
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
	mov	[edx], ebx
	jmp	edi

;========================================
	
entry dplusBop
	fld	QWORD[edx+8]
	fadd	QWORD[edx]
	add	edx,8
	fstp	QWORD[edx]
	jmp	edi
	
;========================================
	
entry dminusBop
	fld	QWORD[edx+8]
	fsub	QWORD[edx]
	add	edx,8
	fstp	QWORD[edx]
	jmp	edi
	
;========================================
	
entry dtimesBop
	fld	QWORD[edx+8]
	fmul	QWORD[edx]
	add	edx,8
	fstp	QWORD[edx]
	jmp	edi
	
;========================================
	
entry ddivideBop
	fld	QWORD[edx+8]
	fdiv	QWORD[edx]
	add	edx,8
	fstp	QWORD[edx]
	jmp	edi
	
;========================================
	
entry dEquals0Bop
	fldz
	jmp	dEqualsBop1
	
entry dEqualsBop
	fld	QWORD[edx]
	add	edx, 8
dEqualsBop1:
	fld	QWORD[edx]
	add	edx, 4
	xor	ebx, ebx
	fucompp
	fnstsw	ax
	test	ah, 44h
	jp	dEqualsBop2
	dec	ebx
dEqualsBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry dNotEquals0Bop
	fldz
	jmp	dNotEqualsBop1
	
entry dNotEqualsBop
	fld	QWORD[edx]
	add	edx, 8
dNotEqualsBop1:
	fld	QWORD[edx]
	add	edx, 4
	xor	ebx, ebx
	fucompp
	fnstsw	ax
	test	ah, 44h
	jnp	dNotEqualsBop2
	dec	ebx
dNotEqualsBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry dGreaterThan0Bop
	fldz
	jmp	dGreaterThanBop1
	
entry dGreaterThanBop
	fld	QWORD[edx]
	add	edx, 8
dGreaterThanBop1:
	fld	QWORD[edx]
	add	edx, 4
	xor	ebx, ebx
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 41h
	jne	dGreaterThanBop2
	dec	ebx
dGreaterThanBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry dGreaterEquals0Bop
	fldz
	jmp	dGreaterEqualsBop1
	
entry dGreaterEqualsBop
	fld	QWORD[edx]
	add	edx, 8
dGreaterEqualsBop1:
	fld	QWORD[edx]
	add	edx, 4
	xor	ebx, ebx
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 1
	jne	dGreaterEqualsBop2
	dec	ebx
dGreaterEqualsBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry dLessThan0Bop
	fldz
	jmp	dLessThanBop1
	
entry dLessThanBop
	fld	QWORD[edx]
	add	edx, 8
dLessThanBop1:
	fld	QWORD[edx]
	add	edx, 4
	xor	ebx, ebx
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 5
	jp	dLessThanBop2
	dec	ebx
dLessThanBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry dLessEquals0Bop
	fldz
	jmp	dLessEqualsBop1
	
entry dLessEqualsBop
	fld	QWORD[edx]
	add	edx, 8
dLessEqualsBop1:
	fld	QWORD[edx]
	add	edx, 4
	xor	ebx, ebx
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 41h
	jp	dLessEqualsBop2
	dec	ebx
dLessEqualsBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry dWithinBop
	fld	QWORD[edx+8]
	fld	QWORD[edx+16]
	xor	ebx, ebx
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 41h
	jne	dWithinBop2
	fld	QWORD[edx]
	fld	QWORD[edx+16]
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 5
	jp	dWithinBop2
	dec	ebx
dWithinBop2:
	add	edx, 20
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry dMinBop
	fld	QWORD[edx]
	fld	QWORD[edx+8]
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 41h
	jne	dMinBop2
	mov	eax, [edx]
	mov	ebx, [edx+4]
	mov	[edx+8], eax
	mov	[edx+12], ebx
dMinBop2:
	add	edx, 8
	jmp	edi
	
;========================================
	
entry dMaxBop
	fld	QWORD[edx]
	fld	QWORD[edx+8]
	fcomp	st1
	fnstsw	ax
	fstp	st0
	test	ah, 5
	jp	dMaxBop2
	mov	eax, [edx]
	mov	ebx, [edx+4]
	mov	[edx+8], eax
	mov	[edx+12], ebx
dMaxBop2:
	add	edx, 8
	jmp	edi
	

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
	push	edx
	push	esi
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+12]
	push	eax
	mov	eax, [edx+8]
	push	eax
	xcall	atan2
	add	esp, 16
	pop	esi
	pop	edx
    add esp, 4
	add	edx,8
	fstp	QWORD[edx]
	jmp	edi
	
;========================================
	
entry fatan2Bop
    sub esp, 12     ; 16-byte align for mac
	push	edx
	push	esi
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	xcall	atan2f
	add	esp, 8
	pop	esi
	pop	edx
    add esp, 12
	add	edx, 4
	fstp	DWORD[edx]
	jmp	edi
	
;========================================
	
entry dpowBop
	; a^x
    sub esp, 4      ; 16-byte align for mac
	push	edx
	push	esi
	; push x
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx]
	push	eax
	; push a
	mov	eax, [edx+12]
	push	eax
	mov	eax, [edx+8]
	push	eax
	xcall	pow
	add	esp, 16
	pop	esi
	pop	edx
    add esp, 4
	add	edx, 8
	fstp	QWORD[edx]
	jmp	edi
	
;========================================
	
entry fpowBop
	; a^x
    sub esp, 12     ; 16-byte align for mac
	push	edx
	push	esi
	; push x
	mov	eax, [edx]
	push	eax
	; push a
	mov	eax, [edx+4]
	push	eax
	xcall	powf
	add	esp, 8
	pop	esi
	pop	edx
    add esp, 12
	add	edx, 4
	fstp	DWORD[edx]
	jmp	edi
	
;========================================

entry dabsBop
	fld	QWORD[edx]
	fabs
	fstp	QWORD[edx]
	jmp	edi
	
;========================================

entry fabsBop
	fld	DWORD[edx]
	fabs
	fstp	DWORD[edx]
	jmp	edi
	
;========================================

entry dldexpBop
	; ldexp( a, n )
    sub esp, 8      ; 16-byte align for mac
	push	edx
	push	esi
	; TOS is n (int), a (double)
	; get arg n
	mov	eax, [edx]
	push	eax
	; get arg a
	mov	eax, [edx+8]
	push	eax
	mov	eax, [edx+4]
	push	eax
	xcall	ldexp
	add	esp, 12
	pop	esi
	pop	edx
    add esp, 8
	add	edx, 4
	fstp	QWORD[edx]
	jmp	edi
	
;========================================

entry fldexpBop
	; ldexpf( a, n )
    sub esp, 12     ; 16-byte align for mac
	push	edx
	push	esi
	; TOS is n (int), a (float)
	; get arg n
	mov	eax, [edx]
	push	eax
	; get arg a
	mov	eax, [edx+4]
	push	eax
	xcall	ldexpf
	add	esp, 8
	pop	esi
	pop	edx
    add esp, 12
	add	edx, 4
	fstp	DWORD[edx]
	jmp	edi
	
;========================================

entry dfrexpBop
	; frexp( a, ptrToIntExponentReturn )
	sub	edx, 4
    sub esp, 8      ; 16-byte align for mac
	push	edx
	push	esi
	; TOS is a (double)
	; we return TOS: nInt aFrac
	; alloc nInt
	push	edx
	; get arg a
	mov	eax, [edx+8]
	push	eax
	mov	eax, [edx+4]
	push	eax
	xcall	frexp
	add	esp, 12
	pop	esi
	pop	edx
    add esp, 8
	fstp	QWORD[edx+4]
	jmp	edi
	
;========================================

entry ffrexpBop
	; frexpf( a, ptrToIntExponentReturn )
    sub esp, 12     ; 16-byte align for mac
	; get arg a
	mov	eax, [edx]
	sub	edx, 4
	push	edx
	push	esi
	; TOS is a (float)
	; we return TOS: nInt aFrac
	; alloc nInt
	push	edx
	push	eax
	xcall	frexpf
	add	esp, 8
	pop	esi
	pop	edx
    add esp, 12
	fstp	DWORD[edx+4]
	jmp	edi
	
;========================================

entry dmodfBop
	; modf( a, ptrToDoubleWholeReturn )
    sub esp, 8      ; 16-byte align for mac
	mov	eax, edx
	sub	edx, 8
	push	edx
	push	esi
	; TOS is a (double)
	; we return TOS: bFrac aWhole
	; alloc nInt
	push	eax
	; get arg a
	mov	eax, [edx+12]
	push	eax
	mov	eax, [edx+8]
	push	eax
	xcall	modf
	add	esp, 12
	pop	esi
	pop	edx
    add esp, 8
	fstp	QWORD[edx]
	jmp	edi
	
;========================================

entry fmodfBop
	; modf( a, ptrToFloatWholeReturn )
    sub esp, 12     ; 16-byte align for mac
	mov	eax, edx
	sub	edx, 4
	push	edx
	push	esi
	; TOS is a (float)
	; we return TOS: bFrac aWhole
	; alloc nInt
	push	eax
	; get arg a
	mov	eax, [edx+4]
	push	eax
	xcall	modff
	add	esp, 8
	pop	esi
	pop	edx
    add esp, 12
	fstp	DWORD[edx]
	jmp	edi
	
;========================================

entry dfmodBop
    sub esp, 4      ; 16-byte align for mac
	push	edx
	push	esi
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+12]
	push	eax
	mov	eax, [edx+8]
	push	eax
	xcall	fmod
	add	esp, 16
	pop	esi
	pop	edx
    add esp, 4
	add	edx, 8
	fstp	QWORD[edx]
	jmp	edi
	
;========================================

entry ffmodBop
    sub esp, 12     ; 16-byte align for mac
	push	edx
	push	esi
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	xcall	fmodf
	add	esp, 8
	pop	esi
	pop	edx
    add esp, 12
	add	edx, 4
	fstp	DWORD[edx]
	jmp	edi
	
;========================================

entry lplusBop
	mov     ebx, [edx+4]
	mov     eax, [edx]
	add     edx, 8
	add     ebx, [edx+4]
	adc     eax, [edx]
	mov     [edx+4], ebx
	mov     [edx], eax
	jmp     edi

;========================================

entry lminusBop
	mov     ebx, [edx+12]
	mov     eax, [edx+8]
	sub     ebx, [edx+4]
	sbb     eax, [edx]
	add     edx, 8
	mov     [edx+4], ebx
	mov     [edx], eax
	jmp     edi

;========================================

entry ltimesBop
	; based on http://stackoverflow.com/questions/1131833/how-do-you-multiply-two-64-bit-numbers-in-x86-assembler
	push	edi
	push	esi
	; edx always holds hi 32 bits of mul result
	mov	ebx, edx	; so we will use ebx for the forth stack ptr
	; TOS: bhi ebx   blo ebx+4   ahi ebx+8   alo ebx+12
	xor	ecx, ecx	; ecx holds sign flag
	mov	eax, [ebx]

	or	eax, eax
	jge	ltimes1
	; b is negative
	not	ecx
	mov	esi, [ebx+4]
	not	esi
	not	eax
	add	esi, 1
	adc	eax, 0
	mov	[ebx+4], esi
	mov	[ebx], eax
ltimes1:

	mov	eax, [ebx+8]
	mov	esi, [ebx+12]
	or	eax, eax
	jge	ltimes2
	; a is negative
	not	ecx
	not	esi
	not	eax
	add	esi, 1
	adc	eax, 0
	mov	[ebx+12], esi
	mov	[ebx+8], eax
ltimes2:
  
	; alo(esi) * blo
	mov	eax, [ebx+4]
	mul	esi				; edx is hipart, eax is final lopart
	mov	edi, edx		; edi is hipart accumulator
  
	mov	esi, [ebx+12]	; esi = alo
	mov	[ebx+12], eax	; ebx+12 = final lopart

	; alo * bhi
	mov	eax, [ebx]		; eax = bhi
	mul	esi
	add	edi, eax
  
	; ahi * blo
	mov	esi, [ebx+8]	; esi = ahi
	mov	eax, [ebx+4]	; eax = blo
	mul	esi
	add	edi, eax		; edi = hiResult
  
	; invert result if needed
	or	ecx, ecx
	jge	ltimes3
	mov	eax, [ebx+12]	; eax = loResult
	not	eax
	not	edi
	add	eax, 1
	adc	edi, 0
	mov	[ebx+12], eax
ltimes3:

	mov	[ebx+8], edi

	add	ebx, 8
	mov	edx, ebx
	pop	esi
	pop	edi

	jmp     edi

;========================================

entry umtimesBop
	mov	eax, [edx]
	mov	ebx, [edx+4]
	push edx
	mul	ebx		; result hiword in edx, loword in eax
	mov	ebx, edx
	pop	edx
	mov	[edx+4], eax
	mov	[edx], ebx
	jmp	edi

;========================================

entry mtimesBop
	mov	eax, [edx]
	mov	ebx, [edx+4]
	push edx
	imul	ebx		; result hiword in edx, loword in eax
	mov	ebx, edx
	pop	edx
	mov	[edx+4], eax
	mov	[edx], ebx
	jmp	edi

;========================================

entry i2fBop
	fild	DWORD[edx]
	fstp	DWORD[edx]
	jmp	edi	

;========================================

entry i2dBop
	fild	DWORD[edx]
	sub	edx, 4
	fstp	QWORD[edx]
	jmp	edi	

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
    fld     DWORD[edx]
    fistp   DWORD[edx]
	fldcw	WORD[esp + 2]
	add	esp, 4
	jmp	edi

;========================================

entry f2dBop
	fld	DWORD[edx]
	sub	edx, 4
	fstp	QWORD[edx]
	jmp	edi
		
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
    fld     QWORD[edx]
    add edx, 4
    fistp   DWORD[edx]
	fldcw	WORD[esp + 2]
	add	esp, 4
	jmp	edi

;========================================

entry d2fBop
	fld	QWORD[edx]
	add	edx, 4
	fstp	DWORD[edx]
	jmp	edi

;========================================

entry doExitBop
	mov	eax, [ebp + FCore.RPtr]
	mov	ebx, [ebp + FCore.RTPtr]
	cmp	ebx, eax
	jle	doExitBop1
	mov	esi, [eax]
	add	eax, 4
	mov	[ebp + FCore.RPtr], eax
	test	esi, esi
	jz doneBop
	jmp	edi

doExitBop1:
	mov	eax, kForthErrorReturnStackUnderflow
	jmp	interpLoopErrorExit
	
;========================================

entry doExitLBop
    ; rstack: local_var_storage oldFP oldIP
    ; FP points to oldFP
	mov	eax, [ebp + FCore.FPtr]
	mov	esi, [eax]
	mov	[ebp + FCore.FPtr], esi
	add	eax, 4
	mov	ebx, [ebp + FCore.RTPtr]
	cmp	ebx, eax
	jle	doExitBop1
	mov	esi, [eax]
	add	eax, 4
	mov	[ebp + FCore.RPtr], eax
	test	esi, esi
	jz doneBop
	jmp	edi
	
;========================================


entry doExitMBop
    ; rstack: oldIP oldTP
	mov	eax, [ebp + FCore.RPtr]
	mov	ebx, [ebp + FCore.RTPtr]
	add	eax, 8
	cmp	ebx, eax
	jl	doExitBop1
	mov	[ebp + FCore.RPtr], eax
	mov	esi, [eax-8]	; IP = oldIP
	mov	ebx, [eax-4]
	mov	[ebp + FCore.TPtr], ebx
	test	esi, esi
	jz doneBop
	jmp	edi

;========================================

entry doExitMLBop
    ; rstack: local_var_storage oldFP oldIP oldTP
    ; FP points to oldFP
	mov	eax, [ebp + FCore.FPtr]
	mov	esi, [eax]
	mov	[ebp + FCore.FPtr], esi
	add	eax, 12
	mov	ebx, [ebp + FCore.RTPtr]
	cmp	ebx, eax
	jl	doExitBop1
	mov	[ebp + FCore.RPtr], eax
	mov	esi, [eax-8]	; IP = oldIP
	mov	ebx, [eax-4]
	mov	[ebp + FCore.TPtr], ebx
	test	esi, esi
	jz doneBop
	jmp	edi
	
;========================================

entry callBop
	; rpush current IP
	mov	eax, [ebp + FCore.RPtr]
	sub	eax, 4
	mov	[eax], esi
	mov	[ebp + FCore.RPtr], eax
	; pop new IP
	mov	esi, [edx]
	add	edx, 4
	test	esi, esi
	jz doneBop
	jmp	edi
	
;========================================

entry gotoBop
	mov	esi, [edx]
	test	esi, esi
	jz doneBop
	jmp	edi

;========================================
;
; TOS is start-index
; TOS+4 is end-index
; the op right after this one should be a branch just past end of loop (used by leave)
; 
entry doDoBop
	mov	ebx, [ebp + FCore.RPtr]
	sub	ebx, 12
	mov	[ebp + FCore.RPtr], ebx
	; @RP-2 holds top-of-loop-IP
	add	esi, 4    ; skip over loop exit branch right after this op
	mov	[ebx+8], esi
	; @RP-1 holds end-index
	mov	eax, [edx+4]
	mov	[ebx+4], eax
	; @RP holds current-index
	mov	eax, [edx]
	mov	[ebx], eax
	add	edx, 8
	jmp	edi
	
;========================================
;
; TOS is start-index
; TOS+4 is end-index
; the op right after this one should be a branch just past end of loop (used by leave)
; 
entry doCheckDoBop
	mov	eax, [edx]		; eax is start index
	mov	ecx, [edx+4]	; ecx is end index
	add	edx, 8
	cmp	eax,ecx
	jge	doCheckDoBop1
	
	mov	ebx, [ebp + FCore.RPtr]
	sub	ebx, 12
	mov	[ebp + FCore.RPtr], ebx
	; @RP-2 holds top-of-loop-IP
	add	esi, 4    ; skip over loop exit branch right after this op
	mov	[ebx+8], esi
	; @RP-1 holds end-index
	mov	[ebx+4], ecx
	; @RP holds current-index
	mov	[ebx], eax
doCheckDoBop1:
	jmp	edi
	
;========================================

entry doLoopBop
	mov	ebx, [ebp + FCore.RPtr]
	mov	eax, [ebx]
	inc	eax
	cmp	eax, [ebx+4]
	jge	doLoopBop1	; jump if done
	mov	[ebx], eax
	mov	esi, [ebx+8]
	jmp	edi

doLoopBop1:
	add	ebx,12
	mov	[ebp + FCore.RPtr], ebx
	jmp	edi
	
;========================================

entry doLoopNBop
	mov	ebx, [ebp + FCore.RPtr]	; ebp is RP
	mov	eax, [edx]		; pop N into eax
	add	edx, 4
	or	eax, eax
	jl	doLoopNBop1		; branch if increment negative
	add	eax, [ebx]		; eax is new i
	cmp	eax, [ebx+4]
	jge	doLoopBop1		; jump if done
	mov	[ebx], eax		; update i
	mov	esi, [ebx+8]		; branch to top of loop
	jmp	edi

doLoopNBop1:
	add	eax, [ebx]
	cmp	eax, [ebx+4]
	jl	doLoopBop1		; jump if done
	mov	[ebx], eax		; ipdate i
	mov	esi, [ebx+8]		; branch to top of loop
	jmp	edi
	
;========================================

entry iBop
	mov	eax, [ebp + FCore.RPtr]
	mov	ebx, [eax]
	sub	edx,4
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry jBop
	mov	eax, [ebp + FCore.RPtr]
	mov	ebx, [eax+12]
	sub	edx,4
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry unloopBop
	mov	eax, [ebp + FCore.RPtr]
	add	eax, 12
	mov	[ebp + FCore.RPtr], eax
	jmp	edi
	
;========================================

entry leaveBop
	mov	eax, [ebp + FCore.RPtr]
	; point IP at the branch instruction which is just before top of loop
	mov	esi, [eax+8]
	sub	esi, 4
	; drop current index, end index, top-of-loop-IP
	add	eax, 12
	mov	[ebp + FCore.RPtr], eax
	jmp	edi
	
;========================================

entry orBop
	mov	eax, [edx]
	add	edx, 4
	or	[edx], eax
	jmp	edi
	
;========================================

entry andBop
	mov	eax, [edx]
	add	edx, 4
	and	[edx], eax
	jmp	edi
	
;========================================

entry xorBop
	mov	eax, [edx]
	add	edx, 4
	xor	[edx], eax
	jmp	edi
	
;========================================

entry invertBop
	mov	eax,0FFFFFFFFh
	xor	[edx], eax
	jmp	edi
	
;========================================

entry lshiftBop
	mov	ecx, [edx]
	add	edx, 4
	mov	ebx, [edx]
	shl	ebx, cl
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry lshift64Bop
	mov	ecx, [edx]
	add	edx, 4
	mov	ebx, [edx]
	mov	eax, [edx+4]
	shld	ebx, eax, cl
	shl	eax, cl
	mov	[edx], ebx
	mov	[edx+4], eax
	jmp	edi
	
;========================================

entry arshiftBop
	mov	ecx, [edx]
	add	edx, 4
	mov	ebx, [edx]
	sar	ebx, cl
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry rshiftBop
	mov	ecx, [edx]
	add	edx, 4
	mov	ebx, [edx]
	shr	ebx, cl
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry rshift64Bop
	mov	ecx, [edx]
	add	edx, 4
	mov	ebx, [edx]
	mov	eax, [edx+4]
	shrd	eax, ebx, cl
	shr	ebx, cl
	mov	[edx], ebx
	mov	[edx+4], eax
	jmp	edi
	
;========================================

entry rotateBop
	mov	ecx, [edx]
	add	edx, 4
	mov	ebx, [edx]
	and	cl, 01Fh
	rol	ebx, cl
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry rotate64Bop
	mov	ecx, [edx]
	mov	ebx, [edx+4]
	mov	eax, [edx+8]
	push	edx
	; if rotate count is >31, swap lo & hi parts
	bt ecx, 5
	jnc	rotate64Bop_1
	xchg	eax, ebx
rotate64Bop_1:
	and	cl, 01Fh
	mov	edx, ebx
	shld	ebx, eax, cl
	xchg	edx, ebx
	shld	eax, ebx, cl
	mov	ebx, edx
	pop	edx
	add	edx, 4
	mov	[edx], ebx
	mov	[edx+4], eax
	jmp	edi

;========================================
entry reverseBop
    ; Knuth's algorithm
    ; a = (a << 15) | (a >> 17);
	mov	eax, [edx]
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
	mov	[edx], eax
	jmp edi

;========================================

entry countLeadingZerosBop
	mov	eax, [edx]
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
	mov	[edx], ebx
	jmp edi

;========================================

entry countTrailingZerosBop
	mov	eax, [edx]
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
	mov	[edx], ebx
	jmp edi

;========================================

entry archX86Bop
entry trueBop
	mov	eax,0FFFFFFFFh
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry archARMBop
entry falseBop
	xor	eax, eax
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry nullBop
	xor	eax, eax
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry dnullBop
	xor	eax, eax
	sub	edx, 8
	mov	[edx+4], eax
	mov	[edx], eax
	jmp	edi
	
;========================================

entry equals0Bop
	xor	ebx, ebx
	jmp	equalsBop1
	
;========================================

entry equalsBop
	mov	ebx, [edx]
	add	edx, 4
equalsBop1:
	xor	eax, eax
	cmp	ebx, [edx]
	jnz	equalsBop2
	dec	eax
equalsBop2:
	mov	[edx], eax
	jmp	edi
	
;========================================

entry notEquals0Bop
	xor	ebx, ebx
	jmp	notEqualsBop1
	
;========================================

entry notEqualsBop
	mov	ebx, [edx]
	add	edx, 4
notEqualsBop1:
	xor	eax, eax
	cmp	ebx, [edx]
	jz	notEqualsBop2
	dec	eax
notEqualsBop2:
	mov	[edx], eax
	jmp	edi
	
;========================================

entry greaterThan0Bop
	xor	ebx, ebx
	jmp	gtBop1
	
;========================================

entry greaterThanBop
	mov	ebx, [edx]		; ebx = b
	add	edx, 4
gtBop1:
	xor	eax, eax
	cmp	[edx], ebx
	jle	gtBop2
	dec	eax
gtBop2:
	mov	[edx], eax
	jmp	edi

;========================================

entry greaterEquals0Bop
	xor	ebx, ebx
	jmp	geBop1
	
;========================================

entry greaterEqualsBop
	mov	ebx, [edx]
	add	edx, 4
geBop1:
	xor	eax, eax
	cmp	[edx], ebx
	jl	geBop2
	dec	eax
geBop2:
	mov	[edx], eax
	jmp	edi
	

;========================================

entry lessThan0Bop
	xor	ebx, ebx
	jmp	ltBop1
	
;========================================

entry lessThanBop
	mov	ebx, [edx]
	add	edx, 4
ltBop1:
	xor	eax, eax
	cmp	[edx], ebx
	jge	ltBop2
	dec	eax
ltBop2:
	mov	[edx], eax
	jmp	edi
	
;========================================

entry lessEquals0Bop
	xor	ebx, ebx
	jmp	leBop1
	
;========================================

entry lessEqualsBop
	mov	ebx, [edx]
	add	edx, 4
leBop1:
	xor	eax, eax
	cmp	[edx], ebx
	jg	leBop2
	dec	eax
leBop2:
	mov	[edx], eax
	jmp	edi
	
;========================================

entry unsignedGreaterThanBop
	mov	ebx, [edx]
	add	edx, 4
ugtBop1:
	xor	eax, eax
	cmp	[edx], ebx
	jbe	ugtBop2
	dec	eax
ugtBop2:
	mov	[edx], eax
	jmp	edi
	
;========================================

entry unsignedLessThanBop
	mov	ebx, [edx]
	add	edx, 4
ultBop1:
	xor	eax, eax
	cmp	[edx], ebx
	jae	ultBop2
	dec	eax
ultBop2:
	mov	[edx], eax
	jmp	edi
	
;========================================

entry withinBop
	; tos: hiLimit loLimit value
	xor	eax, eax
	mov	ebx, [edx+8]	; ebx = value
withinBop1:
	cmp	[edx], ebx
	jle	withinFail
	cmp	[edx+4], ebx
	jg	withinFail
	dec	eax
withinFail:
	add edx, 8
	mov	[edx], eax		
	jmp	edi
	
;========================================

entry clampBop
	; tos: hiLimit loLimit value
	mov	ebx, [edx+8]	; ebx = value
	mov	eax, [edx]		; eax = hiLimit
	cmp	eax, ebx
	jle clampFail
	mov	eax, [edx+4]
	cmp	eax, ebx
	jg	clampFail
	mov	eax, ebx		; value was within range
clampFail:
	add edx, 8
	mov	[edx], eax		
	jmp	edi
	
;========================================

entry minBop
	mov	ebx, [edx]
	add	edx, 4
	cmp	[edx], ebx
	jl	minBop1
	mov	[edx], ebx
minBop1:
	jmp	edi
	
;========================================

entry maxBop
	mov	ebx, [edx]
	add	edx, 4
	cmp	[edx], ebx
	jg	maxBop1
	mov	[edx], ebx
maxBop1:
	jmp	edi
	
	
;========================================

entry lcmpBop
	xor	eax, eax
	mov ebx, [edx+8]
	sub	ebx, [edx]
	mov ebx, [edx+12]
	sbb	ebx, [edx+4]
	jz	lcmpBop3
	jl	lcmpBop2
	add	eax, 2
lcmpBop2:
	dec	eax
lcmpBop3:
	add edx, 12
	mov	[edx], eax
	jmp	edi

;========================================

entry ulcmpBop
	xor	eax, eax
	mov ebx, [edx+8]
	sub	ebx, [edx]
	mov ebx, [edx+12]
	sbb	ebx, [edx+4]
	jz	ulcmpBop3
	jb	ulcmpBop2
	add	eax, 2
ulcmpBop2:
	dec	eax
ulcmpBop3:
	add edx, 12
	mov	[edx], eax
	jmp	edi

;========================================

entry lEqualsBop
	xor	eax, eax
	mov ebx, [edx+8]
	mov ecx, [edx+12]
	sub	ebx, [edx]
	sbb	ecx, [edx+4]
	jnz	leqBop1
	dec	eax
leqBop1:
	add edx, 12
	mov	[edx], eax
	jmp	edi
	
;========================================

entry lEquals0Bop
	xor eax, eax
	mov ebx, [edx+4]
	or ebx, [edx]
	jnz	leq0Bop1
	dec	eax
leq0Bop1:
	add edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry lNotEqualsBop
	xor	eax, eax
	mov ebx, [edx+8]
	sub	ebx, [edx]
	mov ebx, [edx+12]
	sbb	ebx, [edx+4]
	jz	lneqBop1
	dec	eax
lneqBop1:
	add edx, 12
	mov	[edx], eax
	jmp	edi
	
;========================================

entry lNotEquals0Bop
	xor	eax, eax
	mov ebx, [edx+4]
	or ebx, [edx]
	jz	lneq0Bop1
	dec	eax
lneq0Bop1:
	add edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry lGreaterThanBop
	xor	eax, eax
	mov ebx, [edx+8]
	sub	ebx, [edx]
	mov ebx, [edx+12]
	sbb	ebx, [edx+4]
	jle	lgtBop
	dec	eax
lgtBop:
	add edx, 12
	mov	[edx], eax
	jmp	edi

;========================================

entry lGreaterThan0Bop
	xor	eax, eax
	cmp eax, [edx]
	jl lgt0Bop2		; if hiword is negative, return false
	jg lgt0Bop1		; if hiword is positive, return true
	; hiword was zero, need to check low word (unsigned)
	cmp eax, [edx+4]
	jz lgt0Bop2		; loword also 0, return false
lgt0Bop1:
	dec	eax
lgt0Bop2:
	add	edx, 4
	mov	[edx], eax
	jmp	edi

;========================================

entry lGreaterEqualsBop
	xor	eax, eax
	mov ebx, [edx+8]
	sub	ebx, [edx]
	mov ebx, [edx+12]
	sbb	ebx, [edx+4]
	jl	lgeBop
	dec	eax
lgeBop:
	add edx, 12
	mov	[edx], eax
	jmp	edi

;========================================

entry lGreaterEquals0Bop
	xor	eax, eax
	cmp eax, [edx]
	jl lge0Bop		; if hiword is negative, return false
	dec	eax
lge0Bop:
	add	edx, 4
	mov	[edx], eax
	jmp	edi

;========================================

entry lLessThanBop
	xor	eax, eax
	mov ebx, [edx+8]
	sub	ebx, [edx]
	mov ebx, [edx+12]
	sbb	ebx, [edx+4]
	jge	lltBop
	dec	eax
lltBop:
	add edx, 12
	mov	[edx], eax
	jmp	edi

;========================================

entry lLessThan0Bop
	xor	eax, eax
	cmp eax, [edx]
	jge lt0Bop		; if hiword is negative, return true
	dec	eax
lt0Bop:
	add	edx, 4
	mov	[edx], eax
	jmp	edi

;========================================

entry lLessEqualsBop
	xor	eax, eax
	mov ebx, [edx+8]
	sub	ebx, [edx]
	mov ebx, [edx+12]
	sbb	ebx, [edx+4]
	jg	lleBop
	dec	eax
lleBop:
	add edx, 12
	mov	[edx], eax
	jmp	edi

;========================================

entry lLessEquals0Bop
	xor	eax, eax
	cmp eax, [edx]
	jg lle1Bop		; if hiword is positive, return false
	jl lle0Bop		; if hiword is negative, return true
	; hiword was 0, need to test loword
	cmp eax, [edx+4]
	jnz lle1Bop		; if loword is positive, return false
lle0Bop:
	dec	eax
lle1Bop:
	add	edx, 4
	mov	[edx], eax
	jmp	edi

;========================================

entry icmpBop
	mov	ebx, [edx]		; ebx = b
	add	edx, 4
	xor	eax, eax
	cmp	[edx], ebx
	jz	icmpBop3
	jl	icmpBop2
	add	eax, 2
icmpBop2:
	dec	eax
icmpBop3:
	mov	[edx], eax
	jmp	edi

;========================================

entry uicmpBop
	mov	ebx, [edx]
	add	edx, 4
	xor	eax, eax
	cmp	[edx], ebx
	jz	uicmpBop3
	jb	uicmpBop2
	add	eax, 2
uicmpBop2:
	dec	eax
uicmpBop3:
	mov	[edx], eax
	jmp	edi
	
;========================================

entry rpushBop
	mov	ebx, [edx]
	add	edx, 4
	mov	eax, [ebp + FCore.RPtr]
	sub	eax, 4
	mov	[ebp + FCore.RPtr], eax
	mov	[eax], ebx
	jmp	edi
	
;========================================

entry rpopBop
	mov	eax, [ebp + FCore.RPtr]
	mov	ebx, [eax]
	add	eax, 4
	mov	[ebp + FCore.RPtr], eax
	sub	edx, 4
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry rpeekBop
	mov	eax, [ebp + FCore.RPtr]
	mov	ebx, [eax]
	sub	edx, 4
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry rdropBop
	mov	eax, [ebp + FCore.RPtr]
	add	eax, 4
	mov	[ebp + FCore.RPtr], eax
	jmp	edi
	
;========================================

entry rpBop
	lea	eax, [ebp + FCore.RPtr]
	jmp	intEntry
	
;========================================

entry r0Bop
	mov	eax, [ebp + FCore.RTPtr]
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry dupBop
	mov	eax, [edx]
	sub	edx, 4
	mov	[edx], eax
	jmp	edi

;========================================

entry checkDupBop
	mov	eax, [edx]
	or	eax, eax
	jz	dupNon0Bop1
	sub	edx, 4
	mov	[edx], eax
dupNon0Bop1:
	jmp	edi

;========================================

entry swapBop
	mov	eax, [edx]
	mov	ebx, [edx+4]
	mov	[edx], ebx
	mov	[edx+4], eax
	jmp	edi
	
;========================================

entry dropBop
	add	edx, 4
	jmp	edi
	
;========================================

entry overBop
	mov	eax, [edx+4]
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry rotBop
	mov	eax, [edx]		; tos[0], will go in tos[1]
	mov	ebx, [edx+8]	; tos[2], will go in tos[0]
	mov	[edx], ebx
	mov	ebx, [edx+4]	; tos[1], will go in tos[2]
	mov	[edx+8], ebx
	mov	[edx+4], eax
	jmp	edi
	
;========================================

entry reverseRotBop
	mov	eax, [edx]		; tos[0], will go in tos[2]
	mov	ebx, [edx+4]	; tos[1], will go in tos[0]
	mov	[edx], ebx
	mov	ebx, [edx+8]	; tos[2], will go in tos[1]
	mov	[edx+4], ebx
	mov	[edx+8], eax
	jmp	edi
	
;========================================

entry nipBop
	mov	eax, [edx]
	add	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry tuckBop
	mov	eax, [edx]
	mov	ebx, [edx+4]
	sub	edx, 4
	mov	[edx], eax
	mov	[edx+4], ebx
	mov	[edx+8], eax
	jmp	edi
	
;========================================

entry pickBop
	mov	eax, [edx]
	add   eax, 1
	mov	ebx, [edx+eax*4]
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry spBop
	; this is overkill to make sp look like other vars
	mov	ebx, [ebp + FCore.varMode]
	xor	eax, eax
	mov	[ebp + FCore.varMode], eax
	cmp	ebx, kVarMinusStore
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, [spActionTable + ebx*4]
	jmp	ebx
	
spFetch:
	mov	eax, edx
	sub	edx, 4
	mov	[edx], eax
	jmp	edi

spRef:
	; returns address of SP shadow copy
	lea	eax, [ebp + FCore.SPtr]
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
spStore:
	mov	ebx, [edx]
	mov	edx, ebx
	jmp	edi

spPlusStore:
	mov	eax, [edx]
	add	edx, 4
	add	edx, eax
	jmp	edi

spMinusStore:
	mov	eax, [edx]
	add	edx, 4
	sub	edx, eax
	jmp	edi

spActionTable:
	DD	spFetch
	DD	spFetch
	DD	spRef
	DD	spStore
	DD	spPlusStore
	DD	spMinusStore

	
;========================================

entry s0Bop
	mov	eax, [ebp + FCore.STPtr]
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry fpBop
	lea	eax, [ebp + FCore.FPtr]
	jmp	intEntry
	
;========================================

entry ipBop
	; let the common intVarAction code change the shadow copy of IP,
	; then jump back to ipFixup to copy the shadow copy of IP into IP register (esi)
	push	edi
	mov	[ebp + FCore.IPtr], esi
	lea	eax, [ebp + FCore.IPtr]
	mov	edi, ipFixup
	jmp	intEntry
	
entry	ipFixup	
	mov	esi, [ebp + FCore.IPtr]
	pop	edi
	jmp	edi
	
;========================================

entry ddupBop
	mov	eax, [edx]
	mov	ebx, [edx+4]
	sub	edx, 8
	mov	[edx], eax
	mov	[edx+4], ebx
	jmp	edi
	
;========================================

entry dswapBop
	mov	eax, [edx]
	mov	ebx, [edx+8]
	mov	[edx+8], eax
	mov	[edx], ebx
	mov	eax, [edx+4]
	mov	ebx, [edx+12]
	mov	[edx+12], eax
	mov	[edx+4], ebx
	jmp	edi
	
;========================================

entry ddropBop
	add	edx, 8
	jmp	edi
	
;========================================

entry doverBop
	mov	eax, [edx+8]
	mov	ebx, [edx+12]
	sub	edx, 8
	mov	[edx], eax
	mov	[edx+4], ebx
	jmp	edi
	
;========================================

entry drotBop
	mov	eax, [edx+20]
	mov	ebx, [edx+12]
	mov	[edx+20], ebx
	mov	ebx, [edx+4]
	mov	[edx+12], ebx
	mov	[edx+4], eax
	mov	eax, [edx+16]
	mov	ebx, [edx+8]
	mov	[edx+16], ebx
	mov	ebx, [edx]
	mov	[edx+8], ebx
	mov	[edx], eax
	jmp	edi
	
;========================================

entry startTupleBop
	mov	eax, [ebp + FCore.RPtr]
	sub	eax, 4
	mov	[ebp + FCore.RPtr], eax
	mov	[eax], edx
	jmp	edi
	
;========================================

entry endTupleBop
	mov	eax, [ebp + FCore.RPtr]
	mov	ebx, [eax]
	add	eax, 4
	mov	[ebp + FCore.RPtr], eax
	sub	ebx, edx
	sub	edx, 4
	sar	ebx, 2
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry hereBop
	mov	eax, [ebp + FCore.DictionaryPtr]
    mov	ebx, [eax + ForthMemorySection.pCurrent]
    sub edx, 4
    mov [edx], ebx
    jmp edi

;========================================

entry dpBop
    mov	eax, [ebp + FCore.DictionaryPtr]
    lea	ebx, [eax + ForthMemorySection.pCurrent]
    sub edx, 4
    mov [edx], ebx
    jmp edi

;========================================

entry storeBop
	mov	eax, [edx]
	mov	ebx, [edx+4]
	add	edx, 8
	mov	[eax], ebx
	jmp	edi
	
;========================================

entry ifetchBop
	mov	eax, [edx]
	mov	ebx, [eax]
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry storeNextBop
	mov	eax, [edx]		; eax -> dst ptr
	mov	ecx, [eax]
	mov	ebx, [edx+4]
	add	edx, 8
	mov	[ecx], ebx
	add	ecx, 4
	mov	[eax], ecx
	jmp	edi
	
;========================================

entry fetchNextBop
	mov	eax, [edx]
	mov	ecx, [eax]
	mov	ebx, [ecx]
	mov	[edx], ebx
	add	ecx, 4
	mov	[eax], ecx
	jmp	edi
	
;========================================

entry cstoreBop
	mov	eax, [edx]
	mov	ebx, [edx+4]
	add	edx, 8
	mov	[eax], bl
	jmp	edi
	
;========================================

entry cfetchBop
	mov	eax, [edx]
	xor	ebx, ebx
	mov	bl, [eax]
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry cstoreNextBop
	mov	eax, [edx]		; eax -> dst ptr
	mov	ecx, [eax]
	mov	ebx, [edx+4]
	add	edx, 8
	mov	[ecx], bl
	add	ecx, 1
	mov	[eax], ecx
	jmp	edi
	
;========================================

entry cfetchNextBop
	mov	eax, [edx]
	mov	ecx, [eax]
	xor	ebx, ebx
	mov	bl, [ecx]
	mov	[edx], ebx
	add	ecx, 1
	mov	[eax], ecx
	jmp	edi
	
;========================================

entry scfetchBop
	mov	eax, [edx]
	movsx	ebx, BYTE[eax]
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry c2iBop
	mov	eax, [edx]
	movsx	ebx, al
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry wstoreBop
	mov	eax, [edx]
	mov	bx, [edx+4]
	add	edx, 8
	mov	[eax], bx
	jmp	edi
	
;========================================

entry wstoreNextBop
	mov	eax, [edx]		; eax -> dst ptr
	mov	ecx, [eax]
	mov	ebx, [edx+4]
	add	edx, 8
	mov	[ecx], bx
	add	ecx, 2
	mov	[eax], ecx
	jmp	edi
	
;========================================

entry wfetchBop
	mov	eax, [edx]
	xor	ebx, ebx
	mov	bx, [eax]
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry wfetchNextBop
	mov	eax, [edx]
	mov	ecx, [eax]
	xor	ebx, ebx
	mov	bx, [ecx]
	mov	[edx], ebx
	add	ecx, 2
	mov	[eax], ecx
	jmp	edi
	
;========================================

entry swfetchBop
	mov	eax, [edx]
	movsx	ebx, WORD[eax]
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry w2iBop
	mov	eax, [edx]
	movsx	ebx, ax
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry dstoreBop
	mov	eax, [edx]
	mov	ebx, [edx+4]
	mov	[eax], ebx
	mov	ebx, [edx+8]
	mov	[eax+4], ebx
	add	edx, 12
	jmp	edi
	
;========================================

entry dstoreNextBop
	mov	eax, [edx]		; eax -> dst ptr
	mov	ecx, [eax]
	mov	ebx, [edx+4]
	mov	[ecx], ebx
	mov	ebx, [edx+8]
	mov	[ecx+4], ebx
	add	ecx, 8
	mov	[eax], ecx
	add	edx, 12
	jmp	edi
	
;========================================

entry dfetchBop
	mov	eax, [edx]
	sub	edx, 4
	mov	ebx, [eax]
	mov	[edx], ebx
	mov	ebx, [eax+4]
	mov	[edx+4], ebx
	jmp	edi
	
;========================================

entry dfetchNextBop
	mov	eax, [edx]
	sub	edx, 4
	mov	ecx, [eax]
	mov	ebx, [ecx]
	mov	[edx], ebx
	mov	ebx, [ecx+4]
	mov	[edx+4], ebx
	add	ecx, 8
	mov	[eax], ecx
	jmp	edi
	
;========================================

entry ostoreBop
	mov	eax, [edx]
	mov	ebx, [edx+4]
	mov	[eax+4], ebx
	mov	ebx, [edx+8]
	mov	[eax], ebx
	add	edx, 12
	jmp	edi
	
;========================================

entry ostoreNextBop
	mov	eax, [edx]		; eax -> dst ptr
	mov	ecx, [eax]
	mov	ebx, [edx+4]
	mov	[ecx+4], ebx
	mov	ebx, [edx+8]
	mov	[ecx], ebx
	add	ecx, 8
	mov	[eax], ecx
	add	edx, 12
	jmp	edi
	
;========================================

entry ofetchBop
	mov	eax, [edx]
	sub	edx, 4
	mov	ebx, [eax+4]
	mov	[edx], ebx
	mov	ebx, [eax]
	mov	[edx+4], ebx
	jmp	edi
	
;========================================

entry ofetchNextBop
	mov	eax, [edx]
	sub	edx, 4
	mov	ecx, [eax]
	mov	ebx, [ecx+4]
	mov	[edx], ebx
	mov	ebx, [ecx]
	mov	[edx+4], ebx
	add	ecx, 8
	mov	[eax], ecx
	jmp	edi
	
;========================================

entry moveBop
	;	TOS: nBytes dstPtr srcPtr
	push	edx
	push	esi
    sub esp, 8      ; 16-byte align for mac
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+8]
	push	eax
	mov	eax, [edx+4]
	push	eax
	xcall	memmove
	add	esp, 20
	pop	esi
	pop	edx
	add	edx, 12
	jmp	edi

;========================================

entry memcmpBop
	;	TOS: nBytes mem2Ptr mem1Ptr
	push	edx
	push	esi
    sub esp, 8      ; 16-byte align for mac
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx+8]
	push	eax
	xcall	memcmp
	add	esp, 20
	pop	esi
	pop	edx
	add	edx, 12
	jmp	edi

;========================================

entry fillBop
	;	TOS: nBytes byteVal dstPtr
	push	edx
	push	esi
    sub esp, 8      ; 16-byte align for mac
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx]
	and	eax, 0FFh
	push	eax
	mov	eax, [edx+8]
	push	eax
	xcall	memset
	add	esp, 20
	pop	esi
	pop	edx
	add	edx, 12
	jmp	edi

;========================================

entry fetchBop
	mov	eax, kVarFetch
	mov	[ebp + FCore.varMode], eax
	jmp	edi
	
;========================================

entry intoBop
	mov	eax, kVarStore
	mov	[ebp + FCore.varMode], eax
	jmp	edi
	
;========================================

entry addToBop
	mov	eax, kVarPlusStore
	mov	[ebp + FCore.varMode], eax
	jmp	edi
	
;========================================

entry subtractFromBop
	mov	eax, kVarMinusStore
	mov	[ebp + FCore.varMode], eax
	jmp	edi
	
;========================================

entry oclearBop
	mov	eax, kVarObjectClear
	mov	[ebp + FCore.varMode], eax
	jmp	edi
		
;========================================

entry refBop
	mov	eax, kVarRef
	mov	[ebp + FCore.varMode], eax
	jmp	edi

;========================================

entry setVarActionBop
	mov   eax, [edx]
	add   edx, 4
	mov	[ebp + FCore.varMode], eax
	jmp	edi

;========================================

entry getVarActionBop
	mov	eax, [ebp + FCore.varMode]
	sub   edx, 4
	mov   [edx], eax
	jmp	edi

;========================================

entry byteVarActionBop
	mov	eax,[edx]
	add	edx, 4
	jmp	byteEntry
	
;========================================

entry ubyteVarActionBop
	mov	eax,[edx]
	add	edx, 4
	jmp	ubyteEntry
	
;========================================

entry shortVarActionBop
	mov	eax,[edx]
	add	edx, 4
	jmp	shortEntry
	
;========================================

entry ushortVarActionBop
	mov	eax,[edx]
	add	edx, 4
	jmp	ushortEntry
	
;========================================

entry intVarActionBop
	mov	eax,[edx]
	add	edx, 4
	jmp	intEntry
	
;========================================

entry floatVarActionBop
	mov	eax,[edx]
	add	edx, 4
	jmp	floatEntry
	
;========================================

entry doubleVarActionBop
	mov	eax,[edx]
	add	edx, 4
	jmp	doubleEntry
	
;========================================

entry longVarActionBop
	mov	eax,[edx]
	add	edx, 4
	jmp	longEntry
	
;========================================

entry opVarActionBop
	mov	eax,[edx]
	add	edx, 4
	jmp	opEntry
	
;========================================

entry objectVarActionBop
	mov	eax,[edx]
	add	edx, 4
	jmp	objectEntry
	
;========================================

entry stringVarActionBop
	mov	eax,[edx]
	add	edx, 4
	jmp	stringEntry
	
;========================================

entry strcpyBop
	;	TOS: srcPtr dstPtr
	push	edx
	push	esi
    sub esp, 12     ; 16-byte align for mac
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	xcall	strcpy
	add	esp, 20
	pop	esi
	pop	edx
	add	edx, 8
	jmp	edi

;========================================

entry strncpyBop
	;	TOS: maxBytes srcPtr dstPtr
	push	edx
	push	esi
    sub esp, 8     ; 16-byte align for mac
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx+8]
	push	eax
	xcall	strncpy
	add	esp, 20
	pop	esi
	pop	edx
	add	edx, 12
	jmp	edi

;========================================

entry strlenBop
	mov	eax, [edx]
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
	mov	[edx], eax
	jmp	edi

;========================================

entry strcatBop
	;	TOS: srcPtr dstPtr
	push	edx
	push	esi
    sub esp, 12     ; 16-byte align for mac
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	xcall	strcat
	add	esp, 20
	pop	esi
	pop	edx
	add	edx, 8
	jmp	edi

;========================================

entry strncatBop
	;	TOS: maxBytes srcPtr dstPtr
	push	edx
	push	esi
    sub esp, 8     ; 16-byte align for mac
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx+8]
	push	eax
	xcall	strncat
	add	esp, 20
	pop	esi
	pop	edx
	add	edx, 12
	jmp	edi

;========================================

entry strchrBop
	;	TOS: char strPtr
	push	edx
	push	esi
    sub esp, 12     ; 16-byte align for mac
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	xcall	strchr
	add	esp, 20
	pop	esi
	pop	edx
	add	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry strrchrBop
	;	TOS: char strPtr
	push	edx
	push	esi
    sub esp, 12     ; 16-byte align for mac
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	xcall	strrchr
	add	esp, 20
	pop	esi
	pop	edx
	add	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry strcmpBop
	;	TOS: ptr2 ptr1
	push	edx
	push	esi
    sub esp, 12     ; 16-byte align for mac
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
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
	pop	esi
	pop	edx
	add	edx, 4
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry stricmpBop
	;	TOS: ptr2 ptr1
	push	edx
	push	esi
    sub esp, 12     ; 16-byte align for mac
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
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
	push	edx
	push	esi
    sub esp, 8     ; 16-byte align for mac
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx+8]
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
	pop	esi
	pop	edx
	add	edx, 8
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry strstrBop
	;	TOS: ptr2 ptr1
	push	edx
	push	esi
    sub esp, 12     ; 16-byte align for mac
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	xcall	strstr
	add	esp, 20
	pop	esi
	pop	edx
	add	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry strtokBop
	;	TOS: ptr2 ptr1
	push	edx
	push	esi
    sub esp, 12     ; 16-byte align for mac
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	xcall	strtok
	add	esp, 20
	pop	esi
	pop	edx
	add	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry litBop
entry flitBop
	mov	eax, [esi]
	add	esi, 4
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry dlitBop
	mov	eax, [esi]
	mov	ebx, [esi+4]
	add	esi, 8
	sub	edx, 8
	mov	[edx], eax
	mov	[edx+4], ebx
	jmp	edi
	
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
	mov	eax, [ebp + FCore.RPtr]
	sub	edx, 4
	mov	ebx, [eax]	; ebx points at param field
	mov	[edx], ebx
	add	eax, 4
	mov	[ebp + FCore.RPtr], eax
	jmp	edi
	
;========================================

entry doVariableBop
	; push IP
	sub	edx, 4
	mov	[edx], esi
	; rpop new ip
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	edi
	
;========================================

entry doConstantBop
	; push longword @ IP
	mov	eax, [esi]
	sub	edx, 4
	mov	[edx], eax
	; rpop new ip
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	edi
	
;========================================

entry doDConstantBop
	; push quadword @ IP
	mov	eax, [esi]
	sub	edx, 8
	mov	[edx], eax
	mov	eax, [esi+4]
	mov	[edx+4], eax
	; rpop new ip
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	edi
	
;========================================

entry doStructBop
	; push IP
	sub	edx, 4
	mov	[edx], esi
	; rpop new ip
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	edi

;========================================

entry doStructArrayBop
	; TOS is array index
	; esi -> bytes per element, followed by element 0
	mov	eax, [esi]		; eax = bytes per element
	add	esi, 4			; esi -> element 0
	imul	eax, [edx]
	add	eax, esi		; add in array base addr
	mov	[edx], eax
	; rpop new ip
	mov	ebx, [ebp + FCore.RPtr]
	mov	esi, [ebx]
	add	ebx, 4
	mov	[ebp + FCore.RPtr], ebx
	jmp	edi

;========================================

entry thisBop
	mov	eax, [ebp + FCore.TPtr]
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry unsuperBop
	; pop original pMethods off rstack
	mov	eax, [ebp + FCore.RPtr]
	mov	ebx, [eax]
	add	eax, 4
	mov	[ebp + FCore.RPtr], eax
	; store original pMethods in this.pMethods
	mov	eax, [ebp + FCore.TPtr]
	mov	[eax], ebx
	jmp	edi
	
;========================================

entry executeBop
	mov	ebx, [edx]
	add	edx, 4
	mov	eax, [ebp + FCore.innerExecute]
	jmp eax
	
;========================================

entry	fopenBop
	push	esi
	mov	eax, [edx]	; pop access string
	add	edx, 4
	push	edx
	push	eax
	mov	eax, [edx]	; pop pathname string
	push	eax
	mov	eax, [ebp + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.fileOpen]
	call	eax
	add		sp, 8
	pop	edx
	pop	esi
	mov	[edx], eax	; push fopen result
	jmp	edi
	
;========================================

entry	fcloseBop
	push	esi
	mov	eax, [edx]	; pop file pointer
	push	edx
	push	eax
	mov	eax, [ebp + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.fileClose]
	call	eax
	add	sp,4
	pop	edx
	pop	esi
	mov	[edx], eax	; push fclose result
	jmp	edi
	
;========================================

entry	fseekBop
	push	esi
	push	edx
	mov	eax, [edx]	; pop control
	push	eax
	mov	eax, [edx+4]	; pop offset
	push	eax
	mov	eax, [edx+8]	; pop file pointer
	push	eax
	mov	eax, [ebp + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.fileSeek]
	call	eax
	add		sp, 12
	pop	edx
	pop	esi
	add	edx, 8
	mov	[edx], eax	; push fseek result
	jmp	edi
	
;========================================

entry	freadBop
	push	esi
	push	edx
	mov	eax, [edx]	; pop file pointer
	push	eax
	mov	eax, [edx+4]	; pop numItems
	push	eax
	mov	eax, [edx+8]	; pop item size
	push	eax
	mov	eax, [edx+12]	; pop dest pointer
	push	eax
	mov	eax, [ebp + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.fileRead]
	call	eax
	add		sp, 16
	pop	edx
	pop	esi
	add	edx, 12
	mov	[edx], eax	; push fread result
	jmp	edi
	
;========================================

entry	fwriteBop
	push	esi
	push	edx
	mov	eax, [edx]	; pop file pointer
	push	eax
	mov	eax, [edx+4]	; pop numItems
	push	eax
	mov	eax, [edx+8]	; pop item size
	push	eax
	mov	eax, [edx+12]	; pop dest pointer
	push	eax
	mov	eax, [ebp + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.fileWrite]
	call	eax
	add		sp, 16
	pop	edx
	pop	esi
	add	edx, 12
	mov	[edx], eax	; push fwrite result
	jmp	edi
	
;========================================

entry	fgetcBop
	push	esi
	mov	eax, [edx]	; pop file pointer
	push	edx
	push	eax
	mov	eax, [ebp + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.fileGetChar]
	call	eax
	add	sp, 4
	pop	edx
	pop	esi
	mov	[edx], eax	; push fgetc result
	jmp	edi
	
;========================================

entry	fputcBop
	push	esi
	mov	eax, [edx]	; pop char to put
	add	edx, 4
	push	edx
	push	eax
	mov	eax, [edx]	; pop file pointer
	push	eax
	mov	eax, [ebp + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.filePutChar]
	call	eax
	add		sp, 8
	pop	edx
	pop	esi
	mov	[edx], eax	; push fputc result
	jmp	edi
	
;========================================

entry	feofBop
	push	esi
	mov	eax, [edx]	; pop file pointer
	push	edx
	push	eax
	mov	eax, [ebp + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.fileAtEnd]
	call	eax
	add	sp, 4
	pop	edx
	pop	esi
	mov	[edx], eax	; push feof result
	jmp	edi
	
;========================================

entry	fexistsBop
	push	esi
	mov	eax, [edx]	; pop filename pointer
	push	edx
	push	eax
	mov	eax, [ebp + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.fileExists]
	call	eax
	add	sp, 4
	pop	edx
	pop	esi
	mov	[edx], eax	; push fexists result
	jmp	edi
	
;========================================

entry	ftellBop
	push	esi
	mov	eax, [edx]	; pop file pointer
	push	edx
	push	eax
	mov	eax, [ebp + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.fileTell]
	call	eax
	add	sp, 4
	pop	edx
	pop	esi
	mov	[edx], eax	; push ftell result
	jmp	edi
	
;========================================

entry	flenBop
	push	esi
	mov	eax, [edx]	; pop file pointer
	push	edx
	push	eax
	mov	eax, [ebp + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.fileGetLength]
	call	eax
	add	sp, 4
	pop	edx
	pop	esi
	mov	[edx], eax	; push flen result
	jmp	edi
	
;========================================

entry	fgetsBop
	push	esi
	push	edx
	mov	eax, [edx]	; pop file
	push	eax
	mov	eax, [edx+4]	; pop maxChars
	push	eax
	mov	eax, [edx+8]	; pop buffer
	push	eax
	mov	eax, [ebp + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.fileGetString]
	call	eax
	add		sp, 12
	pop	edx
	pop	esi
	add	edx, 8
	mov	[edx], eax	; push fgets result
	jmp	edi
	
;========================================

entry	fputsBop
	push	esi
	push	edx
	mov	eax, [edx]	; pop file
	push	eax
	mov	eax, [edx+4]	; pop buffer
	push	eax
	mov	eax, [ebp + FCore.FileFuncs]
	mov	eax, [eax + FileFunc.filePutString]
	call	eax
	add		sp, 8
	pop	edx
	pop	esi
	add	edx, 4
	mov	[edx], eax	; push fseek result
	jmp	edi
	
;========================================
entry	setTraceBop
	mov	eax, [edx]
	add	edx, 4
	mov	[ebp + FCore.traceFlags], eax
	mov	[ebp + FCore.SPtr], edx
	mov	[ebp + FCore.IPtr], esi
	jmp interpFunc

;========================================

;extern void fprintfSub( ForthCoreState* pCore );
;extern void snprintfSub( ForthCoreState* pCore );
;extern void fscanfSub( ForthCoreState* pCore );
;extern void sscanfSub( ForthCoreState* pCore );

;========================================

entry fprintfSubCore
    ; TOS: N argN ... arg1 formatStr filePtr       (arg1 to argN are optional)
	mov	edi, [edx]
	add	edi, 2
	add	edx, 4
	mov	esi, edi
fprintfSub1:
	sub	esi, 1
	jl	fprintfSub2
	mov	ebx, [edx]
	add	edx, 4
	push	ebx
	jmp fprintfSub1
fprintfSub2:
	; all args have been moved from parameter stack to PC stack
	mov	[ebp + FCore.SPtr], edx
	xcall	fprintf
	mov	edx, [ebp + FCore.SPtr]
	sub	edx, 4
	mov	[edx], eax		; return result on parameter stack
	mov	[ebp + FCore.SPtr], edx
	; cleanup PC stack
	mov	ebx, edi
	sal	ebx, 2
	add	esp, ebx
	ret
	
; extern void fprintfSub( ForthCoreState* pCore );

entry fprintfSub
;fprintfSub PROC near C public uses ebx esi edx ecx edi ebp,
;	core:PTR
	push ebp
	mov	ebp,esp
	push ebx
	push ecx
	push edx
	push esi
	push edi
	push ebp
	
	mov	ebp, [ebp + 8]
	mov	edx, [ebp + FCore.SPtr]
	call	fprintfSubCore
	
	pop ebp
	pop	edi
	pop	esi
	pop edx
	pop ecx
	pop ebx
	leave
	ret

;========================================

entry snprintfSubCore
    ; TOS: N argN ... arg1 formatStr bufferSize bufferPtr       (arg1 to argN are optional)
	mov	edi, [edx]
	add	edi, 3
	add	edx, 4
	mov	esi, edi
snprintfSub1:
	sub	esi, 1
	jl	snprintfSub2
	mov	ebx, [edx]
	add	edx, 4
	push	ebx
	jmp snprintfSub1
snprintfSub2:
	; all args have been moved from parameter stack to PC stack
	mov	[ebp + FCore.SPtr], edx
%ifdef WIN32
	xcall	_snprintf
%else
    xcall	snprintf
%endif
	mov	edx, [ebp + FCore.SPtr]
	sub	edx, 4
	mov	[edx], eax		; return result on parameter stack
	mov	[ebp + FCore.SPtr], edx
	; cleanup PC stack
	mov	ebx, edi
	sal	ebx, 2
	add	esp, ebx
	ret
	
; extern long snprintfSub( ForthCoreState* pCore );

entry snprintfSub
;snprintfSub PROC near C GLOBAL uses ebx esi edx ecx edi ebp,
;	core:PTR
	push ebp
	mov	ebp,esp
	push ebx
	push ecx
	push edx
	push esi
	push edi
	push ebp
	
	mov	ebp, [ebp + 8]
	mov	edx, [ebp + FCore.SPtr]
	call	snprintfSubCore
	
	pop ebp
	pop	edi
	pop	esi
	pop edx
	pop ecx
	pop ebx
	leave
	ret

;========================================

; extern int oStringFormatSub( ForthCoreState* pCore, char* pBuffer, int bufferSize );
entry oStringFormatSub;
;oStringFormatSub PROC near C public uses ebx esi edx ecx edi ebp,
;	core:PTR,
;	pBuffer:PTR,
;	bufferSize:DWORD
    ; TOS: numArgs argN ... arg1 formatStr (arg1 to argN are optional)

	push ebp
	mov	ebp,esp
	push ebx
	push esi
	push edx
	push ecx
	push edi
	push ebp
	
	mov	ebx, [ebp + 8]      ; ebx -> core

	; store old SP on rstack
	mov	edx, [ebx + FCore.RPtr]
	sub	edx, 4
	mov	[edx], esp
	mov	[ebx + FCore.RPtr], edx

	; copy arg1 ... argN from param stack to PC stack
	mov	edx, [ebx + FCore.SPtr]
	mov	edi, [edx]	; get numArgs
	add	edi, 1		; add one for the format string
	add	edx, 4
oSFormatSub1:
	sub	edi, 1
	jl	oSFormatSub2
	mov	eax, [edx]
	add	edx, 4
	push	eax
	jmp oSFormatSub1
oSFormatSub2:
	; all args have been copied from parameter stack to PC stack
	mov	[ebx + FCore.SPtr], edx
	mov	eax, [ebp + 16]         ; bufferSize
	push eax
	mov	eax, [ebp + 12]         ; pBuffer
	push eax

%ifdef WIN32
    xcall	_snprintf
%else
    xcall	snprintf
%endif

	; cleanup PC stack
	mov	edx, [ebx + FCore.RPtr]
	mov	esp, [edx]
	add	edx, 4
	mov	[ebx + FCore.RPtr], edx

	pop ebp
	pop	edi
	pop ecx
	pop edx
	pop	esi
	pop ebx
	leave
	ret

;========================================

entry fscanfSubCore
	mov	edi, [edx]
	add	edi, 2
	add	edx, 4
	mov	esi, edi
fscanfSub1:
	sub	esi, 1
	jl	fscanfSub2
	mov	ebx, [edx]
	add	edx, 4
	push	ebx
	jmp fscanfSub1
fscanfSub2:
	; all args have been moved from parameter stack to PC stack
	mov	[ebp + FCore.SPtr], edx
	xcall	fscanf
	mov	edx, [ebp + FCore.SPtr]
	sub	edx, 4
	mov	[edx], eax		; return result on parameter stack
	mov	[ebp + FCore.SPtr], edx
	; cleanup PC stack
	mov	ebx, edi
	sal	ebx, 2
	add	esp, ebx
	ret
	
; extern long fscanfSub( ForthCoreState* pCore );

entry fscanfSub
;fscanfSub PROC near C public uses ebx esi edx ecx edi ebp,
;	core:PTR
	push ebp
	mov	ebp,esp
	push ebx
	push ecx
	push edx
	push esi
	push edi
	push ebp
	
	mov	ebp, [ebp + 8]
	mov	edx, [ebp + FCore.SPtr]
	call	fscanfSubCore
	
	pop ebp
	pop	edi
	pop	esi
	pop edx
	pop ecx
	pop ebx
	leave
	ret

;========================================

entry sscanfSubCore
	mov	edi, [edx]
	add	edi, 2
	add	edx, 4
	mov	esi, edi
sscanfSub1:
	sub	esi, 1
	jl	sscanfSub2
	mov	ebx, [edx]
	add	edx, 4
	push	ebx
	jmp sscanfSub1
sscanfSub2:
	; all args have been moved from parameter stack to PC stack
	mov	[ebp + FCore.SPtr], edx
	xcall	sscanf
	mov	edx, [ebp + FCore.SPtr]
	sub	edx, 4
	mov	[edx], eax		; return result on parameter stack
	mov	[ebp + FCore.SPtr], edx
	; cleanup PC stack
	mov	ebx, edi
	sal	ebx, 2
	add	esp, ebx
	ret

; extern long sscanfSub( ForthCoreState* pCore );

entry sscanfSub
;sscanfSub PROC near C public uses ebx esi edx ecx edi ebp,
;	core:PTR
	push ebp
	mov	ebp,esp
	push ebx
	push ecx
	push edx
	push esi
	push edi
	push ebp
	
	mov	ebp, [ebp + 8]
	mov	edx, [ebp + FCore.SPtr]
	call	sscanfSubCore
	
	pop ebp
	pop	edi
	pop	esi
	pop edx
	pop ecx
	pop ebx
	leave
	ret

;========================================
entry dllEntryPointType
	mov	[ebp + FCore.IPtr], esi
	mov	[ebp + FCore.SPtr], edx
	mov	eax, ebx
	and	eax, 0000FFFFh
	cmp	eax, [ebp + FCore.numOps]
	jge	badUserDef
	; push core ptr
	push	ebp
	; push flags
	mov	esi, ebx
	shr	esi, 16
	and	esi, 7
	push	esi
	; push arg count
	mov	esi, ebx
	shr	esi, 19
	and	esi, 1Fh
	push	esi
	; push entry point address
	mov	esi, [ebp + FCore.ops]
	mov	edx, [esi+eax*4]
	push	edx
	xcall	CallDLLRoutine
	add	esp, 12
	pop	ebp
	jmp	interpFunc


;-----------------------------------------------
;
; NUM VAROP OP combo ops
;  
entry nvoComboType
	; ebx: bits 0..10 are signed integer, bits 11..12 are varop-2, bit 13..23 are opcode
	mov	eax, ebx
	sub	edx, 4
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
	mov	[edx], eax
	; set the varop from bits 11-12
	shr	ebx, 11
	mov	eax, ebx
	
	and	eax, 3							; eax = varop - 2
	add	eax, 2
	mov	[ebp + FCore.varMode], eax
	
	; extract the opcode
	shr	ebx, 2
	and	ebx, 0000007FFh			; ebx is 11 bit opcode
	; opcode is in ebx
	mov	eax, [ebp + FCore.innerExecute]
	jmp eax

;-----------------------------------------------
;
; NUM VAROP combo ops
;  
entry nvComboType
	; ebx: bits 0..21 are signed integer, bits 22-23 are varop-2
	mov	eax, ebx
	sub	edx, 4
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
	mov	[edx], eax
	; set the varop from bits 22-23
	shr	ebx, 22							; ebx = varop - 2
	and	ebx, 3
	add	ebx, 2
	mov	[ebp + FCore.varMode], ebx

	jmp edi

;-----------------------------------------------
;
; NUM OP combo ops
;  
entry noComboType
	; ebx: bits 0..12 are signed integer, bits 13..23 are opcode
	mov	eax, ebx
	sub	edx, 4
	and	eax,000001000h
	jnz	noNegative
	; positive constant
	mov	eax, ebx
	and	eax,00FFFh
	jmp	noCombo1

noNegative:
	or	eax, 0FFFFE000h			; sign extend bits 13-31
noCombo1:
	mov	[edx], eax
	; extract the opcode
	mov	eax, ebx
	shr	ebx, 13
	and	ebx, 0000007FFh			; ebx is 11 bit opcode
	; opcode is in ebx
	mov	eax, [ebp + FCore.innerExecute]
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
	mov	[ebp + FCore.varMode], eax
	
	; extract the opcode
	shr	ebx, 2
	and	ebx, 0003FFFFFh			; ebx is 22 bit opcode

	; opcode is in ebx
	mov	eax, [ebp + FCore.innerExecute]
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
	push	edi
	mov	edi, ozbCombo1
	and	ebx, 0FFFh
	; opcode is in ebx
	mov	eax, [ebp + FCore.innerExecute]
	jmp eax
	
ozbCombo1:
	pop	edi
	pop	eax
	mov	ebx, [edx]
	add	edx, 4
	or	ebx, ebx
	jnz	ozbCombo2			; if TOS not 0, don't branch
	mov	ebx, eax
	and	eax, 02000h
	jz	ozbForward
	; backward branch
	or	ebx,0FFFFC000h
ozbForward:
	add	esi, ebx
ozbCombo2:
	jmp	edi
	
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
	push	edi
	mov	edi, onzbCombo1
	and	ebx, 0FFFh
	; opcode is in ebx
	mov	eax, [ebp + FCore.innerExecute]
	jmp eax
	
onzbCombo1:
	pop	edi
	pop	eax
	mov	ebx, [edx]
	add	edx, 4
	or	ebx, ebx
	jz	onzbCombo2			; if TOS 0, don't branch
	mov	ebx, eax
	and	eax, 02000h
	jz	onzbForward
	; backward branch
	or	ebx,0FFFFC000h
onzbForward:
	add	esi, ebx
onzbCombo2:
	jmp	edi
	
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
	push	esi
	mov	eax, ebx
	and	eax, 00800000h
	shl	eax, 8			; sign bit
	
	mov	esi, ebx
	shr	ebx, 18
	and	ebx, 1Fh
	add	ebx, 112
	shl	ebx, 23			; ebx is exponent
	or	eax, ebx
	
	and	esi, 03FFFFh
	shl	esi, 5
	or	eax, esi
	
	sub	edx, 4
	mov	[edx], eax
	pop	esi
	jmp	edi
	

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
	push	esi
	mov	eax, ebx
	and	eax, 00800000h
	shl	eax, 8			; sign bit
	
	mov	esi, ebx
	shr	ebx, 18
	and	ebx, 1Fh
	add	ebx, 1008
	shl	ebx, 20			; ebx is exponent
	or	eax, ebx
	
	and	esi, 03FFFFh
	shl	esi, 2
	or	eax, esi
	
	sub	edx, 4
	mov	[edx], eax
	sub	edx, 4
	; loword of double is all zeros
	xor	eax, eax
	mov	[edx], eax
	pop	esi
	jmp	edi
	

;-----------------------------------------------
;
; squished long literal
;  
entry squishedLongType
	; get low-24 bits of opcode
	mov	eax, ebx
	sub	edx, 8
	and	eax,00800000h
	jnz	longConstantNegative
	; positive constant
	and	ebx,00FFFFFFh
	mov	[edx+4], ebx
	xor	ebx, ebx
	mov	[edx], ebx
	jmp	edi

longConstantNegative:
	or	ebx, 0FF000000h
	mov	[edx+4], ebx
	xor	ebx, ebx
	sub	ebx, 1
	mov	[edx], ebx
	jmp	edi
	

;-----------------------------------------------
;
; LOCALREF OP combo ops
;
entry lroComboType
	; ebx: bits 0..11 are frame offset in longs, bits 12-23 are op
	push	ebx
	and	ebx, 0FFFH
	sal	ebx, 2
	mov	eax, [ebp + FCore.FPtr]
	sub	eax, ebx
	sub	edx, 4
	mov	[edx], eax
	
	pop	ebx
	shr	ebx, 12
	and	ebx, 0FFFH			; ebx is 12 bit opcode
	; opcode is in ebx
	mov	eax, [ebp + FCore.innerExecute]
	jmp eax
	
;-----------------------------------------------
;
; MEMBERREF OP combo ops
;
entry mroComboType
	; ebx: bits 0..11 are member offset in bytes, bits 12-23 are op
	push	ebx
	and	ebx, 0FFFH
	mov	eax, [ebp + FCore.TPtr]
	add	eax, ebx
	sub	edx, 4
	mov	[edx], eax

	pop	ebx
	shr	ebx, 12
	and	ebx, 0FFFH			; ebx is 12 bit opcode
	; opcode is in ebx
	mov	eax, [ebp + FCore.innerExecute]
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
	
;	123 - 149
	DD	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	DD	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
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
	
%endif

