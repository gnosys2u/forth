; Copyright (C) 2024 Patrick McElhatton
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the “Software”), to
; deal in the Software without restriction, including without limitation the
; rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
; sell copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
; 
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
; 
; THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
; FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
; DEALINGS IN THE SOFTWARE.

	.section .text
	.align  4
	
	.include	"core_gas.inc"

#	FCore	TYPEDEF	ForthCoreState
#	FileFunc	TYPEDEF	ForthFileInterface


# register usage in a forthOp:
#
#	EAX		free
#	EBX		free
#	ECX		free
#	EDX		SP
#	ESI		IP
#	EDI		inner interp PC (constant)
#	EBP		core ptr (constant)

# when in a opType routine:
#	AL		8-bit opType
#	EBX		full 32-bit opcode (need to mask off top 8 bits)

# remember when calling extern cdecl functions:
# 1) they are free to stomp EAX, EBX, ECX and EDX
# 2) they are free to modify their input params on stack

# if you need more than EAX and EBX in a routine, save ESI/IP & EDX/SP in FCore at start with these instructions:
#	mov	%esi,IPtr(ebp)
#	mov	%edx,SPtr(ebp)
# jump to interpFunc at end - interpFunc will restore ECX, EDX, and EDI and go back to inner loop


	.ifdef ASM_UNDERSCORE_PREFIX
#============== use these macros when subroutine names are prefixed by underscore


# externally accessible subroutine
	.macro subEntry entryLabel
	.globl _\entryLabel
_\entryLabel:
	.endm

	.macro opEntry entryLabel
	.globl _\entryLabel
_\entryLabel:
	.endm

	.macro optypeEntry	opLabel
_\opLabel:
	.endm

	.macro	optypeRef	opLabel
	.long	_\opLabel
	.endm

	.macro	xcall	label
	call	_\label
	.endm

		
	.else
#============== use these macros when subroutine names have no prefix


# externally accessible subroutine
	.macro subEntry entryLabel
	.globl \entryLabel
\entryLabel:
	.endm

	.macro opEntry entryLabel
	.globl \entryLabel
\entryLabel:
	.endm

	.macro optypeEntry	opLabel
\opLabel:
	.endm

	.macro	optypeRef	opLabel
	.long	\opLabel
	.endm

	.macro	xcall	label
	call	\label
	.endm
		

	.endif

#-----------------------------------------------
#
# unaryDoubleFunc is used for dsin, dsqrt, dceil, ...
#
	.macro unaryDoubleFunc	opLabel, func
	opEntry \opLabel
#	.globl _\opLabel
#_\opLabel:
	push	%edx
	push	%ecx
	mov	4(%edx),%eax
	push	%eax
	mov	(%edx),%eax
	push	%eax
	xcall	\func
	add	$8,%esp
	pop	%ecx
	pop	%edx
	fstpl	(%edx)
	jmp	*%edi
	.endm
	
#-----------------------------------------------
#
# unaryFloatFunc is used for fsin, fsqrt, fceil, ...
#
	.macro unaryFloatFunc	opLabel, func
	opEntry \opLabel
#	.globl _\opLabel
#_\opLabel:
	push	%edx
	push	%ecx
	mov	(%edx),%eax
	push	%eax
	xcall	\func
	add	$4,%esp
	pop	%ecx
	pop	%edx
	fstp	(%edx)
	jmp	*%edi
	.endm
	
#========================================

# extern long CallDLLRoutine( DLLRoutine function, long argCount, void *core );

#	CallDLLRoutine	,PROC	near	C	public	uses	%ebx	ecx	%edx	%esi	edi	%ebp
	subEntry	CallDLLRoutine
CallDLLRoutine1:
#	.func	CallDLLRoutine
	push %ebp
	mov	%esp,%ebp
	push %ebx
	push %ecx
	push %edx
	push %esi
	push %edi
	push %ebp
	
	mov	20(%ebp),%eax		# eax = funcAddr
	mov	16(%ebp),%edi		# eax = argCount
	mov	12(%ebp),%esi		# esi = flags
	mov	8(%ebp),%ebp		# ebp -> ForthCore
	mov	SPtr(%ebp),%edx
	mov	%edi,%ecx
CallDLL1:
	sub	$1,%ecx
	jl	CallDLL2
	mov	(%edx),%ebx
	add	$4,%edx
	push	%ebx
	jmp	CallDLL1
CallDLL2:
	# all args have been moved from parameter stack to PC stack
	mov	%edx,SPtr(%ebp)
	
	call	*%eax
	
	# handle void return flag
	mov	%esi,%ecx
	and	$1,%esi
	jnz	CallDLL4
	
	mov	SPtr(%ebp),%ebx
	sub	$4,%ebx
	
	# push high part of result if 64-bit return flag set
	mov	%ecx,%esi
	and	$2,%esi
	jz	CallDLL3
	mov	%edx,(%ebx)		# return high part of result on parameter stack
	sub	$4,%ebx
	
CallDLL3:
	# push low part of result
	mov	%eax,(%ebx)		# return result on parameter stack
	mov	%ebx,SPtr(%ebp)
	
CallDLL4:
	# cleanup PC stack
	mov	%ecx,%esi
	and	$4,%esi		# stdcall calling convention flag
	jnz	CallDLL5
	mov	%edi,%ebx
	sal	$2,%ebx
	add	%ebx,%esp
CallDLL5:	
	pop %ebp
	pop	%edi
	pop	%esi
	pop %edx
	pop %ecx
	pop %ebx
	leave
	ret
#	.endfunc

# extern void NativeAction( ForthCoreState *pCore, ulong opVal );
#-----------------------------------------------
#
# inner interpreter entry point for ops defined in assembler
	
#
	subEntry	NativeAction
#	.func	NativeAction
#NativeAction PROC near C public uses ebx ecx edx esi edi ebp,
#	core:PTR,
#	opVal:DWORD
	push %ebp
	mov	%esp,%ebp
	push %ebx
	push %ecx
	push %edx
	push %esi
	push %edi
	push %ebp
	mov	8(%ebp),%eax		# eax = opVal
	mov	12(%ebp),%ebp		# ebp -> ForthCore
	
	call	native1
	
	pop %ebp
	pop	%edi
	pop	%esi
	pop %edx
	pop %ecx
	pop %ebx
	leave
	ret
#	.endfunc

#	.globl native1
native1:
	mov	IPtr(%ebp),%esi
	mov	SPtr(%ebp),%edx
	mov	ops(%ebp),%ecx
	mov	$nativeActionExit,%edi
	mov	(%ecx,%eax,4),%eax
	jmp	*%eax
nativeActionExit:
	mov	%esi,IPtr(%ebp)
	mov	%edx,SPtr(%ebp)
	ret
	
	.ifdef ASM_INNER_INTERPRETER
#-----------------------------------------------
#
# extOp is used by "builtin" ops which are only defined in C++
#
#	ebx holds the opcode
#
extOp:
	# ebx holds the opcode which was just dispatched, use its low 24-bits as index into builtinOps table of ops defined in C#C++
	mov	%ebx,%eax
	and	$0x00FFFFFF,%eax
	mov	ops(%ebp),%ebx
	mov	(%ebx,%eax,4),%eax				# eax is C routine to dispatch to
	# save current IP and SP	
	mov	%esi,IPtr(%ebp)
	mov	%edx,SPtr(%ebp)
	# we need to push ebp twice - the C compiler feels free to overwrite its input parameters,
	# so the top copy of EBP may be trashed on return from C land
	push	%ebp		# push core ptr (our save)
	push	%ebp		# push core ptr (input to C routine)
	call	*%eax
	add	$4,%esp		# discard inputs to C routine
	pop	%ebp
	# load IP and SP from core, in case C routine modified them
	# NOTE: we can't just jump to interpFunc, since that will replace edi & break single stepping
	mov	IPtr(%ebp),%esi
	mov	SPtr(%ebp),%edx
	mov	state(%ebp),%eax
	or	%eax,%eax
	jnz	extOp1		# if something went wrong
	jmp	*%edi		# if everything is ok

# NOTE: Feb. 14 '07 - doing the right thing here - restoring IP & SP and jumping to
# the interpreter loop exit point - causes an access violation exception ?why?
	#mov	IPtr(ebp),%esi
	#mov	SPtr(ebp),%edx
	#jmp	interpLoopExit	# if something went wrong
extOp1:
	ret

#-----------------------------------------------
#
# extOpType is used to handle optypes which are only defined in C++
#
#	ebx holds the opcode
#
extOpType:
	# get the C routine to handle this optype from optypeAction table in FCore
	mov	%ebx,%eax
	shr	$24,%eax							# eax is 8-bit optype
	mov	optypeAction(%ebp),%ecx
	mov	(%ecx,%eax,4),%eax				# eax is C routine to dispatch to
	# save current IP and SP	
	mov	%esi,IPtr(%ebp)
	mov	%edx,SPtr(%ebp)
	# we need to push ebp twice - the C compiler feels free to overwrite its input parameters,
	# so the top copy of EBP may be trashed on return from C land
	push	%ebp		# push core ptr (our save)
	and	$0x00FFFFFF,%ebx
	push	%ebx		# push 24-bit opVal (input to C routine)
	push	%ebp		# push core ptr (input to C routine)
	call	*%eax
	add	$8,%esp		# discard inputs to C routine 
	pop	%ebp
	# load IP and SP from core, in case C routine modified them
	# NOTE: we can't just jump to interpFunc, since that will replace edi & break single stepping
	mov	IPtr(%ebp),%esi
	mov	SPtr(%ebp),%edx
	mov	state(%ebp),%eax
	or	%eax,%eax
	jnz	extOpType1		# if something went wrong
	jmp	*%edi		# if everything is ok

# NOTE: Feb. 14 '07 - doing the right thing here - restoring IP & SP and jumping to
# the interpreter loop exit point - causes an access violation exception ?why?
	#mov	IPtr(ebp),%esi
	#mov	SPtr(ebp),%edx
	#jmp	interpLoopExit	# if something went wrong
extOpType1:
	ret

#-----------------------------------------------
#
# InitAsmTables - initializes first part of optable, where op positions are referenced by constants
#
	subEntry	InitAsmTables
#	.func	InitAsmTables
#	InitAsmTables	,PROC	near	C	public	uses	%ebx	esi	%edx	%edi	%ebp
	push %ebp
	mov	%esp, %ebp	# 0(ebp) = old_ebp 4(ebp)=return_addr  8(ebp)=ForthCore_ptr
	push %ebx
	push %ecx
	push %edx
	push %edi
	push %ebp

	mov	8(%ebp),%ebp		# ebp -> ForthCore struct
	
	mov	$interpLoopDebug1, %ebx
	mov	%ebx, innerLoop(%ebp)

	pop %ebp
	pop	%edi
	pop %edx
	pop %ecx
	pop %ebx
	leave
	ret
#	.endfunc


#-----------------------------------------------
#
# single step a thread
#
# extern eForthResult InterpretOneOpFast( ForthCoreState *pCore, long op );
	subEntry	InterpretOneOpFast
#	.func	InterpretOneOpFast
	push %ebp
	mov	%esp, %ebp
	push %ebx
	push %ecx
	push %edx
	push %esi
	push %edi
	push %ebp
	
	mov	12(%ebp), %ebx
	mov	8(%ebp), %ebp		# ebp -> ForthCore
	mov	IPtr(%ebp), %esi
	mov	SPtr(%ebp), %edx
	mov	$InterpretOneOpFastExit,%edi
	
	# TODO: should we reset state to OK before every step?
	mov	$kResultOk, %eax
	mov	%eax, state(%ebp)
	# instead of jumping directly to the inner loop, do a call so that
	# error exits which do a return instead of branching to inner loop will work
	call	interpLoopExecuteEntry
	jmp	InterpretOneOpFastExit2

InterpretOneOpFastExit:		# this is exit for state == OK - discard the unused return address from call above
	add	$4, %esp
InterpretOneOpFastExit2:	# this is exit for state != OK
	mov	%esi, IPtr(%ebp)
	mov	%edx, SPtr(%ebp)
	mov	state(%ebp), %eax
	pop %ebp
	pop	%edi
	pop	%esi
	pop %edx
	pop %ecx
	pop %ebx
	leave
	ret
#	.endfunc
	
#-----------------------------------------------
#
# inner interpreter C entry point
#
# extern eForthResult InnerInterpreterFast( ForthCoreState *pCore );
	subEntry	InnerInterpreterFast
#	.func	InnerInterpreterFast
#	InnerInterpreterFast	,PROC	near	C	public	uses	%ebx	ecx	%edx	%esi	edi	%ebp
InnerInterpreterFast:
	push %ebp
	mov	%esp,%ebp
	push %ebx
	push %ecx
	push %edx
	push %esi
	push %edi
	push %ebp
	
	mov	8(%ebp),%ebp		# ebp -> ForthCore
	mov	$kResultOk,%eax
	mov	%eax,state(%ebp)
	
	call	interpFunc
	
	pop %ebp
	pop	%edi
	pop	%esi
	pop %edx
	pop %ecx
	pop %ebx
	leave
	ret
#	.endfunc

#-----------------------------------------------
#
# inner interpreter
#	jump to interpFunc if you nead to reload IP, SP, interpLoop
interpFunc:
	mov	IPtr(%ebp),%esi
	mov	SPtr(%ebp),%edx
	mov	innerLoop(%ebp),%edi
	jmp	*%edi

	.globl interpLoopDebug
interpLoopDebug1:
	# while debugging, store IP,SP in corestate shadow copies after every instruction
	#   so crash stacktrace will be more accurate (off by only one instruction)
	mov	%esi,IPtr(%ebp)
	mov	%edx,SPtr(%ebp)
interpLoop:
	mov	(%esi),%ebx		# eax is opcode
	add	$4,%esi			# advance IP
	# interpLoopExecuteEntry is entry for executeBop - expects opcode in ebx
	.globl	interpLoopExecuteEntry
interpLoopExecuteEntry:
	cmp	numOps(%ebp),%ebx	
	jae	notNative1
	mov	ops(%ebp),%eax
	mov	(%eax,%ebx,4),%ecx
	jmp	*%ecx

	.globl	interpLoopExit
interpLoopExit:
	mov	%eax,state(%ebp)
	mov	%esi,IPtr(%ebp)
	mov	%edx,SPtr(%ebp)
	ret

	.globl	badOptype
badOptype:
	mov	$kForthErrorBadOpcodeType,%eax
	jmp	interpLoopErrorExit

	.globl	badVarOperation
badVarOperation:
	mov	$kForthErrorBadVarOperation,%eax
	jmp	interpLoopErrorExit

	.globl	badOpcode
badOpcode:
	mov	$kForthErrorBadOpcode,%eax


	.globl	interpLoopErrorExit
interpLoopErrorExit:
	# error exit point
	# eax is error code
	mov	%eax,error(%ebp)
	mov	$kResultError,%eax
	jmp	interpLoopExit

interpLoopFatalErrorExit:
	# fatal error exit point
	# eax is error code
	mov	%eax,error(%ebp)
	mov	$kResultFatalError,%eax
	jmp	interpLoopExit

# op (in ebx) is not defined in assembler, dispatch through optype table
	optypeEntry	notNative
notNative1:
	mov	%ebx,%eax			# leave full opcode in ebx
	shr	$24,%eax			# eax is 8-bit optype
	mov	opTypesTable1(,%eax,4),%eax
	jmp	*%eax

	optypeEntry	nativeImmediate
	and	$0x00FFFFFF,%ebx
	cmp	numOps(%ebp),%ebx
	jae	badOpcode
	mov	ops(%ebp),%eax
	mov	(%eax,%ebx,4),%ecx
	jmp	*%ecx

# externalBuiltin is invoked when a builtin op which is outside of range of table is invoked
	optypeEntry	externalBuiltin
	# it should be impossible to get here now
	jmp	badOpcode

	optypeEntry cCodeType
	and	$0x00FFFFFF,%ebx
	# dispatch to C version if valid
	cmp	numOps(%ebp),%ebx
	jae	badOpcode
	jmp	extOp


#-----------------------------------------------
#
# user-defined ops (forth words defined with colon)
#
	optypeEntry	userDefType
	# get low-24 bits of opcode & check validity
	and	$0x00FFFFFF,%ebx
	cmp	numOps(%ebp),%ebx
	jge	badUserDef
	# push IP on rstack
	mov	RPtr(%ebp),%eax
	sub	$4,%eax
	mov	%esi,(%eax)
	mov	%eax,RPtr(%ebp)
	# get new IP
	mov	ops(%ebp),%eax
	mov	(%eax,%ebx,4),%esi
	jmp	*%edi

badUserDef:
	mov	$kForthErrorBadOpcode,%eax
	jmp	interpLoopErrorExit

#-----------------------------------------------
#
# user-defined ops (forth words defined with colon)
#
	optypeEntry	relativeDefType
	# ebx is offset from dictionary base of user definition
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	mov	DictionaryPtr(%ebp),%eax
	add	MemSectionBasePtr(%eax),%ebx
	cmp	MemSectionCurrentPtr(%eax),%ebx
	jge	badUserDef
	# push IP on rstack
	mov	RPtr(%ebp),%eax
	sub	$4,%eax
	mov	%esi,(%eax)
	mov	%eax,RPtr(%ebp)
	mov	%ebx,%esi
	jmp	*%edi

#-----------------------------------------------
#
# unconditional branch ops
#
	optypeEntry	branchType
branchType1:						# for internal branching
	# get low-24 bits of opcode
	mov	%ebx,%eax
	and	$0x00800000,%eax
	jnz	branchBack
	# branch forward
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	add	%ebx,%esi
	jmp	*%edi

branchBack:
	or	$0xFF000000,%ebx
	sal	$2,%ebx
	add	%ebx,%esi
	jmp	*%edi

#-----------------------------------------------
#
# branch-on-zero ops
#
	optypeEntry	branchZType
	mov	(%edx),%eax
	add	$4,%edx
	or	%eax,%eax
	jz	branchType1		# branch taken
	jmp	*%edi	# branch not taken

#-----------------------------------------------
#
# branch-on-notzero ops
#
	optypeEntry	branchNZType
	mov	(%edx),%eax
	add	$4,%edx
	or	%eax,%eax
	jnz	branchType1		# branch taken
	jmp	*%edi	# branch not taken

#-----------------------------------------------
#
# case branch ops
#
	optypeEntry	caseBranchType
    # TOS: this_case_value case_selector
	mov	(%edx),%eax		# eax is this_case_value
	add	$4,%edx
	cmp	(%edx),%eax
	jz	caseMatched
	# case didn't match - branch to next case
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	add	%ebx,%esi
	jmp	*%edi

caseMatched:
	add	$4,%edx
	jmp	*%edi

#-----------------------------------------------
#
# branch around block ops
#
	optypeEntry	pushBranchType
	sub	$4,%edx			# push IP (pointer to block)
	mov	%esi,(%edx)
	and	$0x00FFFFFF,%ebx	# branch around block
	sal	$2,%ebx
	add	%ebx,%esi
	jmp	*%edi


#-----------------------------------------------
#
# 24-bit constant ops
#
	optypeEntry	constantType
	# get low-24 bits of opcode
	mov	%ebx,%eax
	sub	$4,%edx
	and	$0x00800000,%eax
	jnz	constantNegative
	# positive constant
	and	$0x00FFFFFF,%ebx
	mov	%ebx,(%edx)
	jmp	*%edi

constantNegative:
	or	$0xFF000000,%ebx
	mov	%ebx,(%edx)
	jmp	*%edi

#-----------------------------------------------
#
# 24-bit offset ops
#
	optypeEntry	offsetType
	# get low-24 bits of opcode
	mov	%ebx,%eax
	and	$0x00800000,%eax
	jnz	offsetNegative
	# positive constant
	and	$0x00FFFFFF,%ebx
	add	%ebx,(%edx)
	jmp	*%edi

offsetNegative:
	or	$0xFF000000,%ebx
	add	%ebx,(%edx)
	jmp	*%edi

#-----------------------------------------------
#
# 24-bit offset fetch ops
#
	optypeEntry	offsetFetchType
	# get low-24 bits of opcode
	mov	%ebx,%eax
	and	$0x00800000,%eax
	jnz	offsetFetchNegative
	# positive constant
	and	$0x00FFFFFF,%ebx
	add	(%edx),%ebx
	mov	(%ebx),%eax
	mov	%eax,(%edx)
	jmp	*%edi

offsetFetchNegative:
	or	$0xFF000000,%ebx
	add	(%edx),%ebx
	mov	(%ebx),%eax
	mov	%eax,(%edx)
	jmp	*%edi

#-----------------------------------------------
#
# array offset ops
#
	optypeEntry	arrayOffsetType
	# get low-24 bits of opcode
	and	$0x00FFFFFF,%ebx		# ebx is size of one element
	# TOS is array base, tos-1 is index
	imul	4(%edx),%ebx	# multiply index by element size
	add	(%edx),%ebx			# add array base addr
	add	$4,%edx
	mov	%ebx,(%edx)
	jmp	*%edi

#-----------------------------------------------
#
# local struct array ops
#
	optypeEntry	localStructArrayType
   # bits 0..11 are padded struct length in bytes, bits 12..23 are frame offset in longs
   # multiply struct length by TOS, add in (negative) frame offset, and put result on TOS
	mov	$0x0000FFF,%eax
	and	%ebx,%eax                # eax is padded struct length in bytes
	imul	(%edx),%eax              # multiply index * length
	add	FPtr(%ebp),%eax
	and	$0x00FFF000,%ebx
	sar	$10,%ebx							# ebx = frame offset in bytes of struct[0]
	sub	%ebx,%eax						# eax -> struct[N]
	mov	%eax,(%edx)
	jmp	*%edi

#-----------------------------------------------
#
# string constant ops
#
	optypeEntry	constantStringType
	# IP points to beginning of string
	# low 24-bits of ebx is string len in longs
	sub	$4,%edx
	mov	%esi,(%edx)		# push string ptr
	# get low-24 bits of opcode
	and	$0x00FFFFFF,%ebx
	shl	$2,%ebx
	# advance IP past string
	add	%ebx,%esi
	jmp	*%edi

#-----------------------------------------------
#
# local stack frame allocation ops
#
	optypeEntry	allocLocalsType
	# rpush old FP
	mov	FPtr(%ebp),%ecx
	mov	RPtr(%ebp),%eax
	sub	$4,%eax
	mov	%ecx,(%eax)
	# set FP = RP, points at old FP
	mov	%eax,FPtr(%ebp)
	# allocate amount of storage specified by low 24-bits of op on rstack
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	sub	%ebx,%eax
	mov	%eax,RPtr(%ebp)
	# clear out allocated storage
	mov	%eax,%ecx
	xor	%eax,%eax
alt1:
	mov	%eax,(%ecx)
	add	$4,%ecx
	sub	$4,%ebx
	jnz	alt1
	jmp	*%edi

#-----------------------------------------------
#
# local string init ops
#
	optypeEntry	initLocalStringType
   # bits 0..11 are string length in bytes, bits 12..23 are frame offset in longs
   # init the current & max length fields of a local string
	mov	$0x00FFF000,%eax
	and	%ebx,%eax
	sar	$10,%eax							# eax = frame offset in bytes
	mov	FPtr(%ebp),%ecx
	sub	%eax,%ecx						# esi -> max length field
	and	$0x00000FFF,%ebx					# ebx = max length
	mov	%ebx,(%ecx)						# set max length
	xor	%eax,%eax
	mov	%eax,4(%ecx)					# set current length to 0
	movb	%al,5(%ecx)						# add terminating null
	jmp	*%edi

#-----------------------------------------------
#
# local reference ops
#
	optypeEntry	localRefType
	# push local reference - ebx is frame offset in longs
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	sub	%ebx,%eax
	sub	$4,%edx
	mov	%eax,(%edx)
	jmp	*%edi

#-----------------------------------------------
#
# member reference ops
#
	optypeEntry	memberRefType
	# push member reference - ebx is member offset in bytes
	mov	TDPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	sub	$4,%edx
	mov	%eax,(%edx)
	jmp	*%edi

#-----------------------------------------------
#
# local byte ops
#
	optypeEntry	localByteType
	# get ptr to byte var into eax
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	sub	%ebx,%eax
	# see if it is a fetch
byteEntry:
	mov	varMode(%ebp),%ebx
	or	%ebx,%ebx
	jnz	localByte1
	# fetch local byte
localByteFetch:
	sub	$4,%edx
	movsxb	(%eax),%ebx
	mov	%ebx,(%edx)
	jmp	*%edi

	optypeEntry	localUByteType
	# get ptr to byte var into eax
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	sub	%ebx,%eax
	# see if it is a fetch
ubyteEntry:
	mov	varMode(%ebp),%ebx
	or	%ebx,%ebx
	jnz	localByte1
	# fetch local unsigned byte
localUByteFetch:
	sub	$4,%edx
	xor	%ebx,%ebx
	movb	(%eax),%bl
	mov	%ebx,(%edx)
	jmp	*%edi

localByteRef:
	sub	$4,%edx
	mov	%eax,(%edx)
	# set var operation back to fetch
	xor	%eax,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

localByteStore:
	mov	(%edx),%ebx
	movb	%bl,(%eax)
	add	$4,%edx
	# set var operation back to fetch
	xor	%eax,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

localBytePlusStore:
	xor	%ebx,%ebx
	movb	(%eax),%bl
	add	(%edx),%ebx
	movb	%bl,(%eax)
	add	$4,%edx
	# set var operation back to fetch
	xor	%eax,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

localByteMinusStore:
	xor	%ebx,%ebx
	movb	(%eax),%bl
	sub	(%edx),%ebx
	movb	%bl,(%eax)
	add	$4,%edx
	# set var operation back to fetch
	xor	%eax,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

localByteActionTable:
	.long	localByteFetch
	.long	localByteFetch
	.long	localByteRef
	.long	localByteStore
	.long	localBytePlusStore
	.long	localByteMinusStore

localUByteActionTable:
	.long	localUByteFetch
	.long	localUByteFetch
	.long	localByteRef
	.long	localByteStore
	.long	localBytePlusStore
	.long	localByteMinusStore

localByte1:
	cmp	$kVarMinusStore,%ebx
	jg	badVarOperation
	# dispatch based on value in ebx
	mov	localByteActionTable(,%ebx,4),%ebx
	jmp	*%ebx

	optypeEntry	fieldByteType
	# get ptr to byte var into eax
	# TOS is base ptr, ebx is field offset in bytes
	mov	(%edx),%eax
	add	$4,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	byteEntry

	optypeEntry	fieldUByteType
	# get ptr to byte var into eax
	# TOS is base ptr, ebx is field offset in bytes
	mov	(%edx),%eax
	add	$4,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	ubyteEntry

	optypeEntry	memberByteType
	# get ptr to byte var into eax
	# this data ptr is base ptr, ebx is field offset in bytes
	mov	TDPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	byteEntry

	optypeEntry	memberUByteType
	# get ptr to byte var into eax
	# this data ptr is base ptr, ebx is field offset in bytes
	mov	TDPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	ubyteEntry

	optypeEntry	localByteArrayType
	# get ptr to byte var into eax
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	sub	%ebx,%eax
	add	(%edx),%eax		# add in array index on TOS
	add	$4,%edx
	jmp	byteEntry

	optypeEntry	localUByteArrayType
	# get ptr to byte var into eax
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	sub	%ebx,%eax
	add	(%edx),%eax		# add in array index on TOS
	add	$4,%edx
	jmp	ubyteEntry

	optypeEntry	fieldByteArrayType
	# get ptr to byte var into eax
	# TOS is struct base ptr, NOS is index
	# ebx is field offset in bytes
	mov	(%edx),%eax
	add	4(%edx),%eax
	add	$8,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	byteEntry

	optypeEntry	fieldUByteArrayType
	# get ptr to byte var into eax
	# TOS is struct base ptr, NOS is index
	# ebx is field offset in bytes
	mov	(%edx),%eax
	add	4(%edx),%eax
	add	$8,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	ubyteEntry

	optypeEntry	memberByteArrayType
	# get ptr to byte var into eax
	# this data ptr is base ptr, TOS is index
	# ebx is field offset in bytes
	mov	TDPtr(%ebp),%eax
	add	(%edx),%eax
	add	$4,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	byteEntry

	optypeEntry	memberUByteArrayType
	# get ptr to byte var into eax
	# this data ptr is base ptr, TOS is index
	# ebx is field offset in bytes
	mov	TDPtr(%ebp),%eax
	add	(%edx),%eax
	add	$4,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	ubyteEntry

#-----------------------------------------------
#
# local short ops
#
	optypeEntry	localShortType
	# get ptr to short var into eax
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	sub	%ebx,%eax
	# see if it is a fetch
shortEntry:
	mov	varMode(%ebp),%ebx
	or	%ebx,%ebx
	jnz	localShort1
	# fetch local short
localShortFetch:
	sub	$4,%edx
	movsxw	(%eax),%ebx
	mov	%ebx,(%edx)
	jmp	*%edi

	optypeEntry	localUShortType
	# get ptr to short var into eax
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	sub	%ebx,%eax
	# see if it is a fetch
ushortEntry:
	mov	varMode(%ebp),%ebx
	or	%ebx,%ebx
	jnz	localShort1
	# fetch local unsigned short
localUShortFetch:
	sub	$4,%edx
	movsxw	(%eax),%ebx
	xor	%ebx,%ebx
	movw	(%eax),%bx
	mov	%ebx,(%edx)
	jmp	*%edi

localShortRef:
	sub	$4,%edx
	mov	%eax,(%edx)
	# set var operation back to fetch
	xor	%eax,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

localShortStore:
	mov	(%edx),%ebx
	movw	%bx,(%eax)
	add	$4,%edx
	# set var operation back to fetch
	xor	%eax,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

localShortPlusStore:
	movsxw	(%eax),%ebx
	add	(%edx),%ebx
	movw	%bx,(%eax)
	add	$4,%edx
	# set var operation back to fetch
	xor	%eax,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

localShortMinusStore:
	movsxw	(%eax),%ebx
	sub	(%edx),%ebx
	movw	%bx,(%eax)
	add	$4,%edx
	# set var operation back to fetch
	xor	%eax,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

localShortActionTable:
	.long	localShortFetch
	.long	localShortFetch
	.long	localShortRef
	.long	localShortStore
	.long	localShortPlusStore
	.long	localShortMinusStore

localUShortActionTable:
	.long	localUShortFetch
	.long	localUShortFetch
	.long	localShortRef
	.long	localShortStore
	.long	localShortPlusStore
	.long	localShortMinusStore

localShort1:
	cmp	$kVarMinusStore,%ebx
	jg	badVarOperation
	# dispatch based on value in ebx
	mov	localShortActionTable(,%ebx,4),%ebx
	jmp	*%ebx

	optypeEntry	fieldShortType
	# get ptr to short var into eax
	# TOS is base ptr, ebx is field offset in bytes
	mov	(%edx),%eax
	add	$4,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	shortEntry

	optypeEntry	fieldUShortType
	# get ptr to unsigned short var into eax
	# TOS is base ptr, ebx is field offset in bytes
	mov	(%edx),%eax
	add	$4,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	ushortEntry

	optypeEntry	memberShortType
	# get ptr to short var into eax
	# this data ptr is base ptr, ebx is field offset in bytes
	mov	TDPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	shortEntry

	optypeEntry	memberUShortType
	# get ptr to unsigned short var into eax
	# this data ptr is base ptr, ebx is field offset in bytes
	mov	TDPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	ushortEntry

	optypeEntry	localShortArrayType
	# get ptr to int var into eax
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx	# ebx is frame offset in longs
	sal	$2,%ebx
	sub	%ebx,%eax
	mov	(%edx),%ebx		# add in array index on TOS
	add	$4,%edx
	sal	$1,%ebx
	add	%ebx,%eax
	jmp	shortEntry

	optypeEntry	localUShortArrayType
	# get ptr to int var into eax
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx	# ebx is frame offset in longs
	sal	$2,%ebx
	sub	%ebx,%eax
	mov	(%edx),%ebx		# add in array index on TOS
	add	$4,%edx
	sal	$1,%ebx
	add	%ebx,%eax
	jmp	ushortEntry

	optypeEntry	fieldShortArrayType
	# get ptr to short var into eax
	# TOS is struct base ptr, NOS is index
	# ebx is field offset in bytes
	mov	4(%edx),%eax	# eax = index
	sal	$1,%eax
	add	(%edx),%eax		# add in struct base ptr
	add	$8,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax		# add in field offset
	jmp	shortEntry

	optypeEntry	fieldUShortArrayType
	# get ptr to short var into eax
	# TOS is struct base ptr, NOS is index
	# ebx is field offset in bytes
	mov	4(%edx),%eax	# eax = index
	sal	$1,%eax
	add	(%edx),%eax		# add in struct base ptr
	add	$8,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax		# add in field offset
	jmp	ushortEntry

	optypeEntry	memberShortArrayType
	# get ptr to short var into eax
	# this data ptr is base ptr, TOS is index
	# ebx is field offset in bytes
	mov	(%edx),%eax	# eax = index
	sal	$1,%eax
	add	TDPtr(%ebp),%eax
	add	$4,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax		# add in field offset
	jmp	shortEntry

	optypeEntry	memberUShortArrayType
	# get ptr to short var into eax
	# this data ptr is base ptr, TOS is index
	# ebx is field offset in bytes
	mov	(%edx),%eax	# eax = index
	sal	$1,%eax
	add	TDPtr(%ebp),%eax
	add	$4,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax		# add in field offset
	jmp	ushortEntry

#-----------------------------------------------
#
# local int ops
#
	optypeEntry	localIntType
	# get ptr to int var into eax
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	sub	%ebx,%eax
	# see if it is a fetch
intEntry:
	mov	varMode(%ebp),%ebx
	or	%ebx,%ebx
	jnz	localInt1
	# fetch local int
localIntFetch:
	sub	$4,%edx
	mov	(%eax),%ebx
	mov	%ebx,(%edx)
	jmp	*%edi

localIntRef:
	sub	$4,%edx
	mov	%eax,(%edx)
	# set var operation back to fetch
	xor	%eax,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

localIntStore:
	mov	(%edx),%ebx
	mov	%ebx,(%eax)
	add	$4,%edx
	# set var operation back to fetch
	xor	%eax,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

localIntPlusStore:
	mov	(%eax),%ebx
	add	(%edx),%ebx
	mov	%ebx,(%eax)
	add	$4,%edx
	# set var operation back to fetch
	xor	%eax,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

localIntMinusStore:
	mov	(%eax),%ebx
	sub	(%edx),%ebx
	mov	%ebx,(%eax)
	add	$4,%edx
	# set var operation back to fetch
	xor	%eax,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

localIntActionTable:
	.long	localIntFetch
	.long	localIntFetch
	.long	localIntRef
	.long	localIntStore
	.long	localIntPlusStore
	.long	localIntMinusStore

localInt1:
	cmp	$kVarMinusStore,%ebx
	jg	badVarOperation
	# dispatch based on value in ebx
	mov	localIntActionTable(,%ebx,4),%ebx
	jmp	*%ebx

	optypeEntry	fieldIntType
	# get ptr to int var into eax
	# TOS is base ptr, ebx is field offset in bytes
	mov	(%edx),%eax
	add	$4,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	intEntry

	optypeEntry	memberIntType
	# get ptr to int var into eax
	# this data ptr is base ptr, ebx is field offset in bytes
	mov	TDPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	intEntry

	optypeEntry	localIntArrayType
	# get ptr to int var into eax
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	sub	(%edx),%ebx		# add in array index on TOS
	add	$4,%edx
	sal	$2,%ebx
	sub	%ebx,%eax
	jmp	intEntry

	optypeEntry	fieldIntArrayType
	# get ptr to int var into eax
	# TOS is struct base ptr, NOS is index
	# ebx is field offset in bytes
	mov	4(%edx),%eax	# eax = index
	sal	$2,%eax
	add	(%edx),%eax		# add in struct base ptr
	add	$8,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax		# add in field offset
	jmp	intEntry

	optypeEntry	memberIntArrayType
	# get ptr to short var into eax
	# this data ptr is base ptr, TOS is index
	# ebx is field offset in bytes
	mov	(%edx),%eax	# eax = index
	sal	$2,%eax
	add	TDPtr(%ebp),%eax
	add	$4,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax		# add in field offset
	jmp	intEntry

#-----------------------------------------------
#
# local float ops
#
	optypeEntry	localFloatType
	# get ptr to float var into eax
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	sub	%ebx,%eax
	# see if it is a fetch
floatEntry:
	mov	varMode(%ebp),%ebx
	or	%ebx,%ebx
	jnz	localFloat1
	# fetch local float
localFloatFetch:
	sub	$4,%edx
	mov	(%eax),%ebx
	mov	%ebx,(%edx)
	jmp	*%edi

localFloatRef:
	sub	$4,%edx
	mov	%eax,(%edx)
	# set var operation back to fetch
	xor	%eax,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

localFloatStore:
	mov	(%edx),%ebx
	mov	%ebx,(%eax)
	add	$4,%edx
	# set var operation back to fetch
	xor	%eax,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

localFloatPlusStore:
	flds	(%eax)
	# set var operation back to fetch
	xor	%ebx,%ebx
	mov	%ebx,varMode(%ebp)
	fadds	(%edx)
	add	$4,%edx
	fstps	(%eax)
	jmp	*%edi

localFloatMinusStore:
	flds	(%eax)
	# set var operation back to fetch
	xor	%ebx,%ebx
	mov	%ebx,varMode(%ebp)
	fsubs	(%edx)
	add	$4,%edx
	fstps	(%eax)
	jmp	*%edi

localFloatActionTable:
	.long	localFloatFetch
	.long	localFloatFetch
	.long	localFloatRef
	.long	localFloatStore
	.long	localFloatPlusStore
	.long	localFloatMinusStore

localFloat1:
	cmp	$kVarMinusStore,%ebx
	jg	badVarOperation
	# dispatch based on value in ebx
	mov	localFloatActionTable(,%ebx,4),%ebx
	jmp	*%ebx

	optypeEntry	fieldFloatType
	# get ptr to float var into eax
	# TOS is base ptr, ebx is field offset in bytes
	mov	(%edx),%eax
	add	$4,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	floatEntry

	optypeEntry	memberFloatType
	# get ptr to float var into eax
	# this data ptr is base ptr, ebx is field offset in bytes
	mov	TDPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	floatEntry

	optypeEntry	localFloatArrayType
	# get ptr to float var into eax
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	sub	(%edx),%ebx		# add in array index on TOS
	add	$4,%edx
	sal	$2,%ebx
	sub	%ebx,%eax
	jmp	floatEntry

	optypeEntry	fieldFloatArrayType
	# get ptr to float var into eax
	# TOS is struct base ptr, NOS is index
	# ebx is field offset in bytes
	mov	4(%edx),%eax	# eax = index
	sal	$2,%eax
	add	(%edx),%eax		# add in struct base ptr
	add	$8,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax		# add in field offset
	jmp	floatEntry

	optypeEntry	memberFloatArrayType
	# get ptr to short var into eax
	# this data ptr is base ptr, TOS is index
	# ebx is field offset in bytes
	mov	(%edx),%eax	# eax = index
	sal	$2,%eax
	add	TDPtr(%ebp),%eax
	add	$4,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax		# add in field offset
	jmp	floatEntry

#-----------------------------------------------
#
# local double ops
#
	optypeEntry	localDoubleType
	# get ptr to double var into eax
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	sub	%ebx,%eax
	# see if it is a fetch
doubleEntry:
	mov	varMode(%ebp),%ebx
	or	%ebx,%ebx
	jnz	localDouble1
	# fetch local double
localDoubleFetch:
	sub	$8,%edx
	mov	(%eax),%ebx
	mov	%ebx,(%edx)
	mov	4(%eax),%ebx
	mov	%ebx,4(%edx)
	jmp	*%edi

localDoubleRef:
	sub	$4,%edx
	mov	%eax,(%edx)
	# set var operation back to fetch
	xor	%eax,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

localDoubleStore:
	mov	(%edx),%ebx
	mov	%ebx,(%eax)
	mov	4(%edx),%ebx
	mov	%ebx,4(%eax)
	add	$8,%edx
	# set var operation back to fetch
	xor	%eax,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

localDoublePlusStore:
	fldl	(%eax)
	# set var operation back to fetch
	xor	%ebx,%ebx
	mov	%ebx,varMode(%ebp)
	faddl	(%edx)
	add	$8,%edx
	fstpl	(%eax)
	jmp	*%edi

localDoubleMinusStore:
	fldl	(%eax)
	# set var operation back to fetch
	xor	%ebx,%ebx
	mov	%ebx,varMode(%ebp)
	fsubl	(%edx)
	add	$8,%edx
	fstpl	(%eax)
	jmp	*%edi

localDoubleActionTable:
	.long	localDoubleFetch
	.long	localDoubleFetch
	.long	localDoubleRef
	.long	localDoubleStore
	.long	localDoublePlusStore
	.long	localDoubleMinusStore

localDouble1:
	cmp	$kVarMinusStore,%ebx
	jg	badVarOperation
	# dispatch based on value in ebx
	mov	localDoubleActionTable(,%ebx,4),%ebx
	jmp	*%ebx

	optypeEntry	fieldDoubleType
	# get ptr to double var into eax
	# TOS is base ptr, ebx is field offset in bytes
	mov	(%edx),%eax
	add	$4,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	doubleEntry

	optypeEntry	memberDoubleType
	# get ptr to double var into eax
	# this data ptr is base ptr, ebx is field offset in bytes
	mov	TDPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	doubleEntry

	optypeEntry	localDoubleArrayType
	# get ptr to double var into eax
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	sub	%ebx,%eax
	mov	(%edx),%ebx		# add in array index on TOS
	add	$4,%edx
	sal	$3,%ebx
	add	%ebx,%eax
	jmp	doubleEntry

	optypeEntry	fieldDoubleArrayType
	# get ptr to double var into eax
	# TOS is struct base ptr, NOS is index
	# ebx is field offset in bytes
	mov	4(%edx),%eax	# eax = index
	sal	$3,%eax
	add	(%edx),%eax		# add in struct base ptr
	add	$8,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax		# add in field offset
	jmp	doubleEntry

	optypeEntry	memberDoubleArrayType
	# get ptr to short var into eax
	# this data ptr is base ptr, TOS is index
	# ebx is field offset in bytes
	mov	(%edx),%eax	# eax = index
	sal	$3,%eax
	add	TDPtr(%ebp),%eax
	add	$4,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax		# add in field offset
	jmp	doubleEntry

#-----------------------------------------------
#
# local string ops
#
#	PUBLIC	stringEntry,localStringFetch,localStringStore,localStringAppend,localStringType
	optypeEntry	localStringType
	# get ptr to string var into eax
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	sub	%ebx,%eax
	# see if it is a fetch
stringEntry:
	mov	varMode(%ebp),%ebx
	or	%ebx,%ebx
	jnz	localString1
	# fetch local string
localStringFetch:
	sub	$4,%edx
	add	$8,%eax		# skip maxLen & currentLen fields
	mov	%eax,(%edx)
	# set var operation back to fetch
	xor	%eax,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

# ref on a string returns the address of maxLen field, not the string chars
localStringRef:
	sub	$4,%edx
	mov	%eax,(%edx)
	# set var operation back to fetch
	xor	%eax,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

localStringStore:
	# eax -> dest string maxLen field
	# TOS is src string addr
	mov	%esi,IPtr(%ebp)	# IP will get stomped
	mov	(%edx),%ecx			# ecx -> chars of src string
	add	$4,%edx
	mov	%edx,SPtr(%ebp)
	lea	8(%eax),%edi		# edi -> chars of dst string
	push	%ecx
	xcall	strlen
	add	$4,%esp
	# eax is src string length

	# figure how many chars to copy
	mov	-8(%edi),%ebx		# ebx = max string length
	cmp	%ebx,%eax
	jle	lsStore1
	mov	%ebx,%eax
lsStore1:
	# set current length field
	mov	%eax,-4(%edi)

	# do the copy
	push	%eax		# push numBytes
	push	%eax		# and save a copy in case strncpy modifies its stack inputs
	push	%ecx		# srcPtr
	push	%edi		# dstPtr
	xcall	strncpy
	add	$12,%esp
	pop	%esi			# esi = numBytes

	# add the terminating null
	xor	%eax,%eax
	movb	%al,(%edi,%esi)

	# set var operation back to fetch
	mov	%eax,varMode(%ebp)
	jmp	interpFunc

localStringAppend:
	# eax -> dest string maxLen field
	# TOS is src string addr
	mov	%esi,IPtr(%ebp)	# IP will get stomped
	mov	(%edx),%ecx			# ecx -> chars of src string
	add	$4,%edx
	mov	%edx,SPtr(%ebp)
	lea	8(%eax),%edi		# edi -> chars of dst string
	push	%ecx
	xcall	strlen
	add	$4,%esp
	# eax is src string length

	# figure how many chars to copy
	mov	-8(%edi),%ebx		# ebx = max string length
	mov	-4(%edi),%esi		# esi = cur string length
	add	%eax,%esi
	cmp	%ebx,%esi
	jle	lsAppend1
	mov	%ebx,%eax
	sub	-4(%edi),%eax		# #bytes to copy = maxLen - curLen
	mov	%ebx,%esi			# new curLen = maxLen
lsAppend1:
	# set current length field
	mov	%esi,-4(%edi)

	# do the append
	push	%eax		# push numBytes
	push	%ecx		# srcPtr
	push	%edi		# dstPtr
	# don't need to worry about stncat stomping registers since we jump to interpFunc
	xcall	strncat
	add	$12,%esp

	# add the terminating null
	xor	%eax,%eax
	mov	-4(%edi),%esi
	movb	%al,(%edi,%esi)

	# set var operation back to fetch
	mov	%eax,varMode(%ebp)
	jmp	interpFunc

localStringActionTable:
	.long	localStringFetch
	.long	localStringFetch
	.long	localStringRef
	.long	localStringStore
	.long	localStringAppend

localString1:
	cmp	$kVarPlusStore,%ebx
	jg	badVarOperation
	# dispatch based on value in ebx
	mov	localStringActionTable(,%ebx,4),%ebx
	jmp	*%ebx

	optypeEntry	fieldStringType
	# get ptr to byte var into eax
	# TOS is base ptr, ebx is field offset in bytes
	mov	(%edx),%eax
	add	$4,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	stringEntry

	optypeEntry	memberStringType
	# get ptr to byte var into eax
	# this data ptr is base ptr, ebx is field offset in bytes
	mov	TDPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	stringEntry

	optypeEntry	localStringArrayType
	# get ptr to int var into eax
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	sub	%ebx,%eax		# eax -> maxLen field of string[0]
	mov	(%eax),%ebx
	sar	$2,%ebx
	add	$3,%ebx			# ebx is element length in longs
	imul	(%edx),%ebx	# mult index * element length
	add	$4,%edx
	sal	$2,%ebx			# ebx is offset in bytes
	add	%ebx,%eax
	jmp	stringEntry

	optypeEntry	fieldStringArrayType
	# get ptr to string var into eax
	# TOS is struct base ptr, NOS is index
	# ebx is field offset in bytes
	and	$0x00FFFFFF,%ebx
	add	(%edx),%ebx		# ebx -> maxLen field of string[0]
	mov	(%ebx),%eax		# eax = maxLen
	sar	$2,%eax
	add	$3,%eax			# eax is element length in longs
	imul	4(%edx),%eax	# mult index * element length
	sal	$2,%eax
	add	%ebx,%eax		# eax -> maxLen field of string[N]
	add	$8,%edx
	jmp	opEntry

	optypeEntry	memberStringArrayType
	# get ptr to string var into eax
	# this data ptr is base ptr, TOS is index
	# ebx is field offset in bytes
	and	$0x00FFFFFF,%ebx
	add	TDPtr(%ebp),%ebx	# ebx -> maxLen field of string[0]
	mov	(%ebx),%eax		# eax = maxLen
	sar	$2,%eax
	add	$3,%eax			# eax is element length in longs
	imul	(%edx),%eax	# mult index * element length
	sal	$2,%eax
	add	%ebx,%eax		# eax -> maxLen field of string[N]
	add	$4,%edx
	jmp	opEntry

#-----------------------------------------------
#
# local op ops
#
	optypeEntry	localOpType
	# get ptr to op var into ebx
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	sub	%ebx,%eax
	# see if it is a fetch (execute for ops)
opEntry:
	mov	varMode(%ebp),%ebx
	or	%ebx,%ebx
	jnz	localOp1
	# execute local op
localOpExecute:
	mov	(%eax),%ebx
	jmp	interpLoopExecuteEntry


localOpActionTable:
	.long	localOpExecute
	.long	localIntFetch
	.long	localIntRef
	.long	localIntStore

localOp1:
	cmp	$kVarStore,%ebx
	jg	badVarOperation
	# dispatch based on value in ebx
	mov	localOpActionTable(,%ebx,4),%ebx
	jmp	*%ebx

	optypeEntry	fieldOpType
	# get ptr to op var into eax
	# TOS is base ptr, ebx is field offset in bytes
	mov	(%edx),%eax
	add	$4,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	opEntry

	optypeEntry	memberOpType
	# get ptr to op var into eax
	# this data ptr is base ptr, ebx is field offset in bytes
	mov	TDPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	opEntry

	optypeEntry	localOpArrayType
	# get ptr to op var into eax
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	sub	(%edx),%ebx		# add in array index on TOS
	add	$4,%edx
	sal	$2,%ebx
	sub	%ebx,%eax
	jmp	opEntry

	optypeEntry	fieldOpArrayType
	# get ptr to op var into eax
	# TOS is struct base ptr, NOS is index
	# ebx is field offset in bytes
	mov	4(%edx),%eax	# eax = index
	sal	$2,%eax
	add	(%edx),%eax		# add in struct base ptr
	add	$8,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax		# add in field offset
	jmp	opEntry

	optypeEntry	memberOpArrayType
	# get ptr to short var into eax
	# this data ptr is base ptr, TOS is index
	# ebx is field offset in bytes
	mov	(%edx),%eax	# eax = index
	sal	$2,%eax
	add	TDPtr(%ebp),%eax
	add	$4,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax		# add in field offset
	jmp	opEntry

#-----------------------------------------------
#
# local long (int64) ops
#
	optypeEntry	localLongType
	# get ptr to long var into eax
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	sub	%ebx,%eax
	# see if it is a fetch
longEntry:
	mov	varMode(%ebp),%ebx
	or	%ebx,%ebx
	jnz	localLong1
	# fetch local double
localLongFetch:
	sub	$8,%edx
	mov	(%eax),%ebx
	mov	%ebx,4(%edx)
	mov	4(%eax),%ebx
	mov	%ebx,(%edx)
	jmp	*%edi

localLongRef:
	sub	$4,%edx
	mov	%eax,(%edx)
	# set var operation back to fetch
	xor	%eax,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

localLongStore:
	mov	(%edx),%ebx
	mov	%ebx,4(%eax)
	mov	4(%edx),%ebx
	mov	%ebx,(%eax)
	add	$8,%edx
	# set var operation back to fetch
	xor	%eax,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

localLongPlusStore:
	mov	(%eax),%ebx
	add	4(%edx),%ebx
	mov	%ebx,(%eax)
	mov	4(%eax),%ebx
	adc	(%edx),%ebx
	mov	%ebx,4(%eax)
	# set var operation back to fetch
	xor	%ebx,%ebx
	mov	%ebx,varMode(%ebp)
	add	$8,%edx
	jmp	*%edi

localLongMinusStore:
	mov	(%eax),%ebx
	sub	4(%edx),%ebx
	mov	%ebx,(%eax)
	mov	4(%eax),%ebx
	sbb	(%edx),%ebx
	mov	%ebx,4(%eax)
	# set var operation back to fetch
	xor	%ebx,%ebx
	mov	%ebx,varMode(%ebp)
	add	$8,%edx
	jmp	*%edi

localLongActionTable:
	.long	localLongFetch
	.long	localLongFetch
	.long	localLongRef
	.long	localLongStore
	.long	localLongPlusStore
	.long	localLongMinusStore

localLong1:
	cmp	$kVarMinusStore,%ebx
	jg	badVarOperation
	# dispatch based on value in ebx
	mov	localLongActionTable(,%ebx,4),%ebx
	jmp	*%ebx

	optypeEntry	fieldLongType
	# get ptr to double var into eax
	# TOS is base ptr, ebx is field offset in bytes
	mov	(%edx),%eax
	add	$4,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	longEntry

	optypeEntry	memberLongType
	# get ptr to double var into eax
	# this data ptr is base ptr, ebx is field offset in bytes
	mov	TDPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	longEntry

	optypeEntry	localLongArrayType
	# get ptr to double var into eax
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	sub	%ebx,%eax
	mov	(%edx),%ebx		# add in array index on TOS
	add	$4,%edx
	sal	$3,%ebx
	add	%ebx,%eax
	jmp	longEntry

	optypeEntry	fieldLongArrayType
	# get ptr to double var into eax
	# TOS is struct base ptr, NOS is index
	# ebx is field offset in bytes
	mov	4(%edx),%eax	# eax = index
	sal	$3,%eax
	add	(%edx),%eax		# add in struct base ptr
	add	$8,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax		# add in field offset
	jmp	longEntry

	optypeEntry	memberLongArrayType
	# get ptr to short var into eax
	# this data ptr is base ptr, TOS is index
	# ebx is field offset in bytes
	mov	(%edx),%eax	# eax = index
	sal	$3,%eax
	add	TDPtr(%ebp),%eax
	add	$4,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax		# add in field offset
	jmp	longEntry

#-----------------------------------------------
#
# local object ops
#
	optypeEntry	localObjectType
	# get ptr to Object var into eax
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	sub	%ebx,%eax
	# see if it is a fetch
objectEntry:
	mov	varMode(%ebp),%ebx
	or	%ebx,%ebx
	jnz	localObject1
	# fetch local Object
localObjectFetch:
	sub	$8,%edx
	mov	(%eax),%ebx
	mov	%ebx,(%edx)
	mov	4(%eax),%ebx
	mov	%ebx,4(%edx)
	jmp	*%edi

localObjectRef:
	sub	$4,%edx
	mov	%eax,(%edx)
	# set var operation back to fetch
	xor	%eax,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

localObjectStore:
	# TOS is new object, eax points to destination#old object
	xor	%ebx,%ebx			# set var operation back to default#fetch
	mov	%ebx,varMode(%ebp)
	mov	%eax,%ecx		# ecx -> destination
	mov	4(%edx),%eax	# eax = newObj data
	mov	4(%ecx),%ebx	# ebx = olbObj data
	cmp	%ebx,%eax
	jz	losx				# objects have same data ptr, don't change refcount
	# handle newObj refcount
	or	%eax,%eax
	jz	los1				# if newObj data ptr is null, don't try to increment refcount
	incb	(%eax)	# increment newObj refcount
	# handle oldObj refcount
los1:
	or	%ebx,%ebx
	jz	los2				# if oldObj data ptr is null, don't try to decrement refcount
	decb	(%ebx)
	jz	los3
los2:
	mov	%eax,4(%ecx)	# oldObj.data = newObj.data
losx:
	mov	(%edx),%ebx		# ebx = newObj methods
	mov	%ebx,(%ecx)		# var.methods = newObj.methods
	add	$8,%edx
	jmp	*%edi

	# object var held last reference to oldObj, invoke olbObj.delete method
	# eax = newObj.data
	# ebx = oldObj.data
	# [ecx] = var.methods
los3:
	push	%edi
	push	%eax
	# TOS is object vtable, NOS is object data ptr
	# ebx is method number

	# push this ptr pair on return stack
	mov	RPtr(%ebp),%edi
	sub	$8,%edi
	mov	%edi,RPtr(%ebp)
	mov	TDPtr(%ebp),%eax
	mov	%eax,4(%edi)
	mov	TMPtr(%ebp),%eax
	mov	%eax,(%edi)

	mov	%ebx,TDPtr(%ebp)
	mov	(%ecx),%ebx
	mov	%ebx,TMPtr(%ebp)

	mov	(%ebx),%ebx	# ebx = method 0 (delete) opcode

	pop	%eax
	pop	%edi

	mov	%eax,4(%ecx)	# var.data = newObj.data
	mov	(%edx),%eax		# ebx = newObj methods
	mov	%eax,(%ecx)		# var.methods = newObj.methods
	add	$8,%edx

	# execute the delete method opcode which is in ebx
	jmp	interpLoopExecuteEntry

# clear object reference, leave on TOS
localObjectUnref:
	# leave object on TOS
	sub	$8,%edx
	mov	(%eax),%ebx
	mov	%ebx,(%edx)
	mov	4(%eax),%ebx	# ebx -> object refcount
	mov	%ebx,4(%edx)
	# if object var is already null, do nothing else
	or	%ebx, %ebx
	jz	lou2
	# clear object var
	mov	%eax,%ecx		# ecx -> object var
	xor	%eax,%eax
	mov	%eax,(%ecx)
	mov	%eax,4(%ecx)
	# set var operation back to fetch
	mov	%eax,varMode(%ebp)
	# get object refcount, see if it is already 0
	mov	(%ebx),%eax
	or	%eax,%eax
	jnz	lou1
	# report refcount negative error
	mov	$kForthErrorBadReferenceCount,%eax
	jmp	interpLoopErrorExit
lou1:
	# decrement object refcount
	sub	$1,%eax
	mov	%eax,(%ebx)
lou2:
	jmp	*%edi


localObjectActionTable:
	.long	localObjectFetch
	.long	localObjectFetch
	.long	localObjectRef
	.long	localObjectStore
	.long	badVarOperation
	.long	localObjectUnref

localObject1:
	cmp	$kVarStore,%ebx
	jg	badVarOperation
	# dispatch based on value in ebx
	mov	localObjectActionTable(,%ebx,4),%ebx
	jmp	*%ebx

	optypeEntry	fieldObjectType
	# get ptr to Object var into eax
	# TOS is base ptr, ebx is field offset in bytes
	mov	(%edx),%eax
	add	$4,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	objectEntry

	optypeEntry	memberObjectType
	# get ptr to Object var into eax
	# this data ptr is base ptr, ebx is field offset in bytes
	mov	TDPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax
	jmp	objectEntry

	optypeEntry	localObjectArrayType
	# get ptr to Object var into eax
	mov	FPtr(%ebp),%eax
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	sub	%ebx,%eax
	mov	(%edx),%ebx		# add in array index on TOS
	add	$4,%edx
	sal	$3,%ebx
	add	%ebx,%eax
	jmp	objectEntry

	optypeEntry	fieldObjectArrayType
	# get ptr to Object var into eax
	# TOS is struct base ptr, NOS is index
	# ebx is field offset in bytes
	mov	4(%edx),%eax	# eax = index
	sal	$3,%eax
	add	(%edx),%eax		# add in struct base ptr
	add	$8,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax		# add in field offset
	jmp	objectEntry

	optypeEntry	memberObjectArrayType
	# get ptr to short var into eax
	# this data ptr is base ptr, TOS is index
	# ebx is field offset in bytes
	mov	(%edx),%eax	# eax = index
	sal	$3,%eax
	add	TDPtr(%ebp),%eax
	add	$4,%edx
	and	$0x00FFFFFF,%ebx
	add	%ebx,%eax		# add in field offset
	jmp	objectEntry

#-----------------------------------------------
#
# method invocation ops
#

# invoke a method on object currently referenced by this ptr pair

	optypeEntry	methodWithThisType
	# ebx is method number
	# push this ptr pair on return stack
	mov	RPtr(%ebp),%ecx
	sub	$8,%ecx
	mov	%ecx,RPtr(%ebp)
	mov	TDPtr(%ebp),%eax
	mov	%eax,4(%ecx)
	mov	TMPtr(%ebp),%eax
	mov	%eax,(%ecx)

	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	add	%eax,%ebx
	mov	(%ebx),%ebx	# ebx = method opcode
	jmp	interpLoopExecuteEntry

# invoke a method on an object referenced by ptr pair on TOS
	optypeEntry	methodWithTOSType
	# TOS is object vtable, NOS is object data ptr
	# ebx is method number
	# push this ptr pair on return stack
	mov	RPtr(%ebp),%ecx
	sub	$8,%ecx
	mov	%ecx,RPtr(%ebp)
	mov	TDPtr(%ebp),%eax
	mov	%eax,4(%ecx)
	mov	TMPtr(%ebp),%eax
	mov	%eax,(%ecx)

	# set data ptr from TOS	
	mov	4(%edx),%eax
	mov	%eax,TDPtr(%ebp)
	# set vtable ptr from TOS
	mov	(%edx),%eax
	mov	%eax,TMPtr(%ebp)
	and	$0x00FFFFFF,%ebx
	sal	$2,%ebx
	add	%eax,%ebx
	mov	(%ebx),%ebx	# ebx = method opcode
	add	$8,%edx
	jmp	interpLoopExecuteEntry

#-----------------------------------------------
#
# member string init ops
#
	optypeEntry	memberStringInitType
   # bits 0..11 are string length in bytes, bits 12..23 are member offset in longs
   # init the current & max length fields of a member string
	mov	$0x00FFF000,%eax
	and	%ebx,%eax
	sar	$10,%eax							# eax = member offset in bytes
	mov	TDPtr(%ebp),%ecx
	add	%eax,%ecx						# esi -> max length field
	and	$0x00000FFF,%ebx					# ebx = max length
	mov	%ebx,(%ecx)						# set max length
	xor	%eax,%eax
	mov	%eax,4(%ecx)					# set current length to 0
	movb	%al,8(%ecx)						# add terminating null
	jmp	*%edi

#-----------------------------------------------
#
# builtinOps code
#

#-----------------------------------------------
#
# doByteOp is compiled as the first op in global byte vars
# the byte data field is immediately after this op
#
	opEntry	doByteBop
	# get ptr to byte var into eax
	mov	%esi,%eax
	# pop rstack
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	byteEntry

#-----------------------------------------------
#
# doUByteOp is compiled as the first op in global unsigned byte vars
# the byte data field is immediately after this op
#
	opEntry	doUByteBop
	# get ptr to byte var into eax
	mov	%esi,%eax
	# pop rstack
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	ubyteEntry

#-----------------------------------------------
#
# doByteArrayOp is compiled as the first op in global byte arrays
# the data array is immediately after this op
#
	opEntry	doByteArrayBop
	# get ptr to byte var into eax
	mov	%esi,%eax
	add	(%edx),%eax
	add	$4,%edx
	# pop rstack
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	byteEntry

	opEntry	doUByteArrayBop
	# get ptr to byte var into eax
	mov	%esi,%eax
	add	(%edx),%eax
	add	$4,%edx
	# pop rstack
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	ubyteEntry

#-----------------------------------------------
#
# doShortOp is compiled as the first op in global short vars
# the short data field is immediately after this op
#
	opEntry	doShortBop
	# get ptr to short var into eax
	mov	%esi,%eax
	# pop rstack
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	shortEntry

#-----------------------------------------------
#
# doUShortOp is compiled as the first op in global unsigned short vars
# the short data field is immediately after this op
#
	opEntry	doUShortBop
	# get ptr to short var into eax
	mov	%esi,%eax
	# pop rstack
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	ushortEntry

#-----------------------------------------------
#
# doShortArrayOp is compiled as the first op in global short arrays
# the data array is immediately after this op
#
	opEntry	doShortArrayBop
	# get ptr to short var into eax
	mov	%esi,%eax
	mov	(%edx),%ebx		# ebx = array index
	add	$4,%edx
	sal	$1,%ebx
	add	%ebx,%eax
	# pop rstack
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	shortEntry

	opEntry	doUShortArrayBop
	# get ptr to short var into eax
	mov	%esi,%eax
	mov	(%edx),%ebx		# ebx = array index
	add	$4,%edx
	sal	$1,%ebx
	add	%ebx,%eax
	# pop rstack
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	ushortEntry

#-----------------------------------------------
#
# doIntOp is compiled as the first op in global int vars
# the int data field is immediately after this op
#
	opEntry	doIntBop
	# get ptr to int var into eax
	mov	%esi,%eax
	# pop rstack
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	intEntry

#-----------------------------------------------
#
# doIntArrayOp is compiled as the first op in global int arrays
# the data array is immediately after this op
#
	opEntry	doIntArrayBop
	# get ptr to int var into eax
	mov	%esi,%eax
	mov	(%edx),%ebx		# ebx = array index
	add	$4,%edx
	sal	$2,%ebx
	add	%ebx,%eax
	# pop rstack
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	intEntry

#-----------------------------------------------
#
# doFloatOp is compiled as the first op in global float vars
# the float data field is immediately after this op
#
	opEntry	doFloatBop
	# get ptr to float var into eax
	mov	%esi,%eax
	# pop rstack
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	floatEntry

#-----------------------------------------------
#
# doFloatArrayOp is compiled as the first op in global float arrays
# the data array is immediately after this op
#
	opEntry	doFloatArrayBop
	# get ptr to float var into eax
	mov	%esi,%eax
	mov	(%edx),%ebx		# ebx = array index
	add	$4,%edx
	sal	$2,%ebx
	add	%ebx,%eax
	# pop rstack
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	floatEntry

#-----------------------------------------------
#
# doDoubleOp is compiled as the first op in global double vars
# the data field is immediately after this op
#
	opEntry	doDoubleBop
	# get ptr to double var into eax
	mov	%esi,%eax
	# pop rstack
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	doubleEntry

#-----------------------------------------------
#
# doDoubleArrayOp is compiled as the first op in global double arrays
# the data array is immediately after this op
#
	opEntry	doDoubleArrayBop
	# get ptr to double var into eax
	mov	%esi,%eax
	mov	(%edx),%ebx		# ebx = array index
	add	$4,%edx
	sal	$3,%ebx
	add	%ebx,%eax
	# pop rstack
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	doubleEntry

#-----------------------------------------------
#
# doStringOp is compiled as the first op in global string vars
# the data field is immediately after this op
#
	opEntry	doStringBop
	# get ptr to string var into eax
	mov	%esi,%eax
	# pop rstack
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	stringEntry

#-----------------------------------------------
#
# doStringArrayOp is compiled as the first op in global string arrays
# the data array is immediately after this op
#
	opEntry	doStringArrayBop
	# get ptr to string var into eax
	mov	%esi,%eax		# eax -> maxLen field of string[0]
	mov	(%eax),%ebx		# ebx = maxLen
	sar	$2,%ebx
	add	$3,%ebx			# ebx is element length in longs
	imul	(%edx),%ebx	# mult index * element length
	add	$4,%edx
	sal	$2,%ebx			# ebx is offset in bytes
	add	%ebx,%eax
	# pop rstack
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	stringEntry

#-----------------------------------------------
#
# doOpOp is compiled as the first op in global op vars
# the op data field is immediately after this op
#
	opEntry	doOpBop
	# get ptr to int var into eax
	mov	%esi,%eax
	# pop rstack
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	opEntry

#-----------------------------------------------
#
# doOpArrayOp is compiled as the first op in global op arrays
# the data array is immediately after this op
#
	opEntry	doOpArrayBop
	# get ptr to op var into eax
	mov	%esi,%eax
	mov	(%edx),%ebx		# ebx = array index
	add	$4,%edx
	sal	$2,%ebx
	add	%ebx,%eax
	# pop rstack
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	opEntry

#-----------------------------------------------
#
# doLongOp is compiled as the first op in global int64 vars
# the data field is immediately after this op
#
	opEntry	doLongBop
	# get ptr to double var into eax
	mov	%esi,%eax
	# pop rstack
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	longEntry

#-----------------------------------------------
#
# doLongArrayOp is compiled as the first op in global int64 arrays
# the data array is immediately after this op
#
	opEntry	doLongArrayBop
	# get ptr to double var into eax
	mov	%esi,%eax
	mov	(%edx),%ebx		# ebx = array index
	add	$4,%edx
	sal	$3,%ebx
	add	%ebx,%eax
	# pop rstack
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	longEntry

#-----------------------------------------------
#
# doObjectOp is compiled as the first op in global Object vars
# the data field is immediately after this op
#
	opEntry	doObjectBop
	# get ptr to Object var into eax
	mov	%esi,%eax
	# pop rstack
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	objectEntry

#-----------------------------------------------
#
# doObjectArrayOp is compiled as the first op in global Object arrays
# the data array is immediately after this op
#
	opEntry	doObjectArrayBop
	# get ptr to Object var into eax
	mov	%esi,%eax
	mov	(%edx),%ebx		# ebx = array index
	add	$4,%edx
	sal	$3,%ebx
	add	%ebx,%eax
	# pop rstack
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	objectEntry

#========================================

	opEntry	initStringBop
	#	TOS: strPtr len
	mov	(%edx),%ebx		# ebx -> first char of string
	xor	%eax,%eax
	mov	%eax,-4(%ebx)	# set current length = 0
	movb	%al,(%ebx)		# set first char to terminating null
	mov	4(%edx),%eax	# eax == string length
	mov	%eax,-8(%ebx)	# set maximum length field
	add	$8,%edx
	jmp	*%edi

#========================================

	opEntry	doneBop
	mov	$kResultDone,%eax
	jmp	interpLoopExit

#========================================

	opEntry	abortBop
	mov	$kForthErrorAbort,%eax
	jmp	interpLoopFatalErrorExit

#========================================

	opEntry	plusBop
	mov	(%edx),%eax
	add	$4,%edx
	add	(%edx),%eax
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	minusBop
	mov	(%edx),%eax
	add	$4,%edx
	mov	(%edx),%ebx
	sub	%eax,%ebx
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	timesBop
	mov	(%edx),%eax
	add	$4,%edx
	imul	(%edx),%eax
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	utimesBop
	mov	(%edx),%eax
	mov	4(%edx),%ebx
	push	%edx
	mul	%ebx			# result hiword in edx, loword in eax
	mov	%edx,%ebx
	pop	%edx
	mov	%ebx,4(%edx)
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	times2Bop
	mov	(%edx),%eax
	add	%eax,%eax
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	times4Bop
	mov	(%edx),%eax
	sal	$2,%eax
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	times8Bop
	mov	(%edx),%eax
	sal	$3,%eax
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	divideBop
	# idiv takes 64-bit numerator in edx:eax
	mov	%edx,%ebx
	mov	4(%edx),%eax	# get numerator
	cltd					# convert into 64-bit in edx:eax
	idivl	(%ebx)		# eax is quotient, edx is remainder
	add	$4,%ebx
	mov	%eax,(%ebx)
	mov	%ebx,%edx
	jmp	*%edi

#========================================

	opEntry	divide2Bop
	mov	(%edx),%eax
	sar	$1,%eax
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	divide4Bop
	mov	(%edx),%eax
	sar	$2,%eax
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	divide8Bop
	mov	(%edx),%eax
	sar	$3,%eax
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	divmodBop
	# idiv takes 64-bit numerator in edx:eax
	mov	%edx,%ebx
	mov	4(%edx),%eax	# get numerator
	cltd				# convert into 64-bit in edx:eax
	idivl	(%ebx)		# eax is quotient, edx is remainder
	mov	%edx,4(%ebx)
	mov	%eax,(%ebx)
	mov	%ebx,%edx
	jmp	*%edi

#========================================

	opEntry	modBop
	# idiv takes 64-bit numerator in edx:eax
	mov	%edx,%ebx
	mov	4(%edx),%eax	# get numerator
	cltd				# convert into 64-bit in edx:eax
	idivl	(%ebx)		# eax is quotient, edx is remainder
	add	$4,%ebx
	mov	%edx,(%ebx)
	mov	%ebx,%edx
	jmp	*%edi

#========================================

	opEntry	negateBop
	mov	(%edx),%eax
	neg	%eax
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	fplusBop
	flds	4(%edx)
	fadds	(%edx)
	add	$4,%edx
	fstps	(%edx)
	jmp	*%edi

#========================================

	opEntry	fminusBop
	flds	4(%edx)
	fsubs	(%edx)
	add	$4,%edx
	fstp	(%edx)
	jmp	*%edi

#========================================

	opEntry	ftimesBop
	flds	4(%edx)
	fmuls	(%edx)
	add	$4,%edx
	fstps	(%edx)
	jmp	*%edi

#========================================

	opEntry	fdivideBop
	flds	4(%edx)
	fdivs	(%edx)
	add	$4,%edx
	fstps	(%edx)
	jmp	*%edi

#========================================

	opEntry	fEquals0Bop
	fldz
	jmp	fEqualsBop1

	opEntry	fEqualsBop
	flds	(%edx)
	add	$4,%edx
fEqualsBop1:
	flds	(%edx)
	xor	%ebx,%ebx
	fucompp
	fnstsw	%ax
	testb	$0x44,%ah
	jp	fEqualsBop2
	dec	%ebx
fEqualsBop2:
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	fNotEquals0Bop
	fldz
	jmp	fNotEqualsBop1

	opEntry	fNotEqualsBop
	flds	(%edx)
	add	$4,%edx
fNotEqualsBop1:
	fld	(%edx)
	xor	%ebx,%ebx
	fucompp
	fnstsw	%ax
	testb	$0x44,%ah
	jnp	fNotEqualsBop2
	dec	%ebx
fNotEqualsBop2:
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	fGreaterThan0Bop
	fldz
	jmp	fGreaterThanBop1

	opEntry	fGreaterThanBop
	flds	(%edx)
	add	$4,%edx
fGreaterThanBop1:
	flds	(%edx)
	xor	%ebx,%ebx
	fcomp	%st(1)
	fnstsw	%ax
	fstp	%st(0)
	testb	$0x41,%ah
	jne	fGreaterThanBop2
	dec	%ebx
fGreaterThanBop2:
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	fGreaterEquals0Bop
	fldz
	jmp	fGreaterEqualsBop1

	opEntry	fGreaterEqualsBop
	flds	(%edx)
	add	$4,%edx
fGreaterEqualsBop1:
	flds	(%edx)
	xor	%ebx,%ebx
	fcomp	%st(1)
	fnstsw	%ax
	fstp	%st(0)
	testb	$1,%ah
	jne	fGreaterEqualsBop2
	dec	%ebx
fGreaterEqualsBop2:
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	fLessThan0Bop
	fldz
	jmp	fLessThanBop1

	opEntry	fLessThanBop
	flds	(%edx)
	add	$4,%edx
fLessThanBop1:
	flds	(%edx)
	xor	%ebx,%ebx
	fcomp	%st(1)
	fnstsw	%ax
	fstp	%st(0)
	testb	$5,%ah
	jp	fLessThanBop2
	dec	%ebx
fLessThanBop2:
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	fLessEquals0Bop
	fldz
	jmp	fLessEqualsBop1

	opEntry	fLessEqualsBop
	flds	(%edx)
	add	$4,%edx
fLessEqualsBop1:
	flds	(%edx)
	xor	%ebx,%ebx
	fcomp	%st(1)
	fnstsw	%ax
	fstp	%st(0)
	testb	$0x41,%ah
	jp	fLessEqualsBop2
	dec	%ebx
fLessEqualsBop2:
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	fWithinBop
	flds	4(%edx)
	flds	8(%edx)
	xor	%ebx,%ebx
	fcomp	%st(1)
	fnstsw	%ax
	fstp	%st(0)
	testb	$0x41,%ah
	jne	fWithinBop2
	flds	(%edx)
	flds	8(%edx)
	fcomp	%st(1)
	fnstsw	%ax
	fstp	%st(0)
	testb	$5,%ah
	jp	fWithinBop2
	dec	%ebx
fWithinBop2:
	add	$8,%edx
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	fMinBop
	fld	(%edx)
	flds	4(%edx)
	fcomp	%st(1)
	fnstsw	%ax
	fstp	%st(0)
	testb	$0x41,%ah
	jne	fMinBop2
	mov	(%edx),%eax
	mov	%eax,4(%edx)
fMinBop2:
	add	$4,%edx
	jmp	*%edi

#========================================

	opEntry	fMaxBop
	flds	(%edx)
	flds	4(%edx)
	fcomp	%st(1)
	fnstsw	%ax
	fstp	%st(0)
	testb	$5,%ah
	jp	fMaxBop2
	mov	(%edx),%eax
	mov	%eax,4(%edx)
fMaxBop2:
	add	$4,%edx
	jmp	*%edi

#========================================

	opEntry	dcmpBop
	fldl	(%edx)
	add	$8,%edx
	fldl	(%edx)
	add	$4,%edx
	xor	%ebx, %ebx
	fcomp	%st(1)
	fnstsw	%ax
	fstp	%st(0)
	sahf
	jz	dcmpBop3
	jb	dcmpBop2
	add	$2,%ebx
dcmpBop2:
	dec	%ebx
dcmpBop3:
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	fcmpBop
	flds	(%edx)
	add	$4,%edx
	flds	(%edx)
	add	$4,%edx
	xor	%ebx, %ebx
	fcomp	%st(1)
	fnstsw	%ax
	fstp	%st(0)
	sahf
	jz	fcmpBop3
	jb	fcmpBop2
	add	$2,%ebx
fcmpBop2:
	dec	%ebx
fcmpBop3:
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	dplusBop
	fldl	8(%edx)
	faddl	(%edx)
	add	$8,%edx
	fstpl	(%edx)
	jmp	*%edi

#========================================

	opEntry	dminusBop
	fldl	8(%edx)
	fsubl	(%edx)
	add	$8,%edx
	fstpl	(%edx)
	jmp	*%edi

#========================================

	opEntry	dtimesBop
	fldl	8(%edx)
	fmull	(%edx)
	add	$8,%edx
	fstpl	(%edx)
	jmp	*%edi

#========================================

	opEntry	ddivideBop
	fldl	8(%edx)
	fdivl	(%edx)
	add	$8,%edx
	fstpl	(%edx)
	jmp	*%edi

#========================================

	opEntry	dEquals0Bop
	fldz
	jmp	dEqualsBop1

	opEntry	dEqualsBop
	fldl	(%edx)
	add	$8,%edx
dEqualsBop1:
	fldl	(%edx)
	add	$4,%edx
	xor	%ebx,%ebx
	fucompp
	fnstsw	%ax
	testb	$0x44,%ah
	jp	dEqualsBop2
	dec	%ebx
dEqualsBop2:
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	dNotEquals0Bop
	fldz
	jmp	dNotEqualsBop1

	opEntry	dNotEqualsBop
	fldl	(%edx)
	add	$8,%edx
dNotEqualsBop1:
	fldl	(%edx)
	add	$4,%edx
	xor	%ebx,%ebx
	fucompp
	fnstsw	%ax
	testb	$0x44,%ah
	jnp	dNotEqualsBop2
	dec	%ebx
dNotEqualsBop2:
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	dGreaterThan0Bop
	fldz
	jmp	dGreaterThanBop1

	opEntry	dGreaterThanBop
	fldl	(%edx)
	add	$8,%edx
dGreaterThanBop1:
	fldl	(%edx)
	add	$4,%edx
	xor	%ebx,%ebx
	fcomp	%st(1)
	fnstsw	%ax
	fstp	%st(0)
	testb	$0x41,%ah
	jne	dGreaterThanBop2
	dec	%ebx
dGreaterThanBop2:
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	dGreaterEquals0Bop
	fldz
	jmp	dGreaterEqualsBop1

	opEntry	dGreaterEqualsBop
	fldl	(%edx)
	add	$8,%edx
dGreaterEqualsBop1:
	fldl	(%edx)
	add	$4,%edx
	xor	%ebx,%ebx
	fcomp	%st(1)
	fnstsw	%ax
	fstp	%st(0)
	testb	$1,%ah
	jne	dGreaterEqualsBop2
	dec	%ebx
dGreaterEqualsBop2:
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	dLessThan0Bop
	fldz
	jmp	dLessThanBop1

	opEntry	dLessThanBop
	fldl	(%edx)
	add	$8,%edx
dLessThanBop1:
	fldl	(%edx)
	add	$4,%edx
	xor	%ebx,%ebx
	fcomp	%st(1)
	fnstsw	%ax
	fstp	%st(0)
	testb	$5,%ah
	jp	dLessThanBop2
	dec	%ebx
dLessThanBop2:
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	dLessEquals0Bop
	fldz
	jmp	dLessEqualsBop1

	opEntry	dLessEqualsBop
	fldl	(%edx)
	add	$8,%edx
dLessEqualsBop1:
	fldl	(%edx)
	add	$4,%edx
	xor	%ebx,%ebx
	fcomp	%st(1)
	fnstsw	%ax
	fstp	%st(0)
	testb	$0x41,%ah
	jp	dLessEqualsBop2
	dec	%ebx
dLessEqualsBop2:
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	dWithinBop
	fldl	8(%edx)
	fldl	16(%edx)
	xor	%ebx,%ebx
	fcomp	%st(1)
	fnstsw	%ax
	fstp	%st(0)
	testb	$0x41,%ah
	jne	dWithinBop2
	fldl	(%edx)
	fldl	16(%edx)
	fcomp	%st(1)
	fnstsw	%ax
	fstp	%st(0)
	testb	$5,%ah
	jp	dWithinBop2
	dec	%ebx
dWithinBop2:
	add	$20,%edx
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	dMinBop
	fldl	(%edx)
	fldl	8(%edx)
	fcomp	%st(1)
	fnstsw	%ax
	fstp	%st(0)
	testb	$0x41,%ah
	jne	dMinBop2
	mov	(%edx),%eax
	mov	4(%edx),%ebx
	mov	%eax,8(%edx)
	mov	%ebx,12(%edx)
dMinBop2:
	add	$8,%edx
	jmp	*%edi

#========================================

	opEntry	dMaxBop
	fldl	(%edx)
	fldl	8(%edx)
	fcomp	%st(1)
	fnstsw	%ax
	fstp	%st(0)
	testb	$5,%ah
	jp	dMaxBop2
	mov	(%edx),%eax
	mov	4(%edx),%ebx
	mov	%eax,8(%edx)
	mov	%ebx,12(%edx)
dMaxBop2:
	add	$8,%edx
	jmp	*%edi


#========================================
	
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

#========================================

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

#========================================

	opEntry	datan2Bop
	push	%edx
	push	%esi
	mov	4(%edx),%eax
	push	%eax
	mov	(%edx),%eax
	push	%eax
	mov	12(%edx),%eax
	push	%eax
	mov	8(%edx),%eax
	push	%eax
	xcall	atan2
	add	$16,%esp
	pop	%esi
	pop	%edx
	add	$8,%edx
	fstpl	(%edx)
	jmp	*%edi

#========================================
	
	opEntry fatan2Bop
	push	%edx
	push	%esi
	mov	(%edx),%eax
	push	%eax
	mov	4(%edx),%eax
	push	%eax
	xcall	atan2f
	add	$8,%esp
	pop	%esi
	pop	%edx
	add	$4,%edx
	fstp	(%edx)
	jmp	*%edi

#========================================

	opEntry	dpowBop
	# a^x
	push	%edx
	push	%esi
	# push x
	mov	4(%edx),%eax
	push	%eax
	mov	(%edx),%eax
	push	%eax
	# push a
	mov	12(%edx),%eax
	push	%eax
	mov	8(%edx),%eax
	push	%eax
	xcall	pow
	add	$16,%esp
	pop	%esi
	pop	%edx
	add	$8,%edx
	fstpl	(%edx)
	jmp	*%edi

#========================================
	
	opEntry fpowBop
	# a^x
	push	%edx
	push	%esi
	# push x
	mov	(%edx),%eax
	push	%eax
	# push a
	mov	4(%edx),%eax
	push	%eax
	xcall	powf
	add	$8,%esp
	pop	%esi
	pop	%edx
	add	$4,%edx
	fstp	(%edx)
	jmp	*%edi
	
#========================================

	opEntry	dabsBop
	fldl	(%edx)
	fabs
	fstpl	(%edx)
	jmp	*%edi

#========================================

	opEntry	fabsBop
	fld	(%edx)
	fabs
	fstp	(%edx)
	jmp	*%edi

#========================================

	opEntry	dldexpBop
	# ldexp( a, n )
	push	%edx
	push	%esi
	# TOS is n (int), a (double)
	# get arg n
	mov	(%edx),%eax
	push	%eax
	# get arg a
	mov	8(%edx),%eax
	push	%eax
	mov	4(%edx),%eax
	push	%eax
	xcall	ldexp
	add	$12,%esp
	pop	%esi
	pop	%edx
	add	$4,%edx
	fstpl	(%edx)
	jmp	*%edi

#========================================

	opEntry	fldexpBop
	# ldexpf( a, n )
	push	%edx
	push	%esi
	# TOS is n (int), a (float)
	# get arg n
	mov	(%edx),%eax
	push	%eax
	# get arg a
	mov	4(%edx),%eax
	push	%eax
	xcall	ldexpf
	add	$8,%esp
	pop	%esi
	pop	%edx
	add	$4,%edx
	fstp	(%edx)
	jmp	*%edi

#========================================

	opEntry	dfrexpBop
	# frexp( a, ptrToIntExponentReturn )
	sub	$4,%edx
	push	%edx
	push	%esi
	# TOS is a (double)
	# we return TOS: nInt aFrac
	# alloc nInt
	push	%edx
	# get arg a
	mov	8(%edx),%eax
	push	%eax
	mov	4(%edx),%eax
	push	%eax
	xcall	frexp
	add	$12,%esp
	pop	%esi
	pop	%edx
	fstpl	4(%edx)
	jmp	*%edi

#========================================

	opEntry	ffrexpBop
	# frexpf( a, ptrToIntExponentReturn )
	# get arg a
	mov	(%edx),%eax
	sub	$4,%edx
	push	%edx
	push	%esi
	# TOS is a (float)
	# we return TOS: nInt aFrac
	# alloc nInt
	push	%edx
	push	%eax
	xcall	frexpf
	add	$8,%esp
	pop	%esi
	pop	%edx
	fstp	4(%edx)
	jmp	*%edi

#========================================

	opEntry	dmodfBop
	# modf( a, ptrToDoubleWholeReturn )
	mov	%edx,%eax
	sub	$8,%edx
	push	%edx
	push	%esi
	# TOS is a (double)
	# we return TOS: bFrac aWhole
	# alloc nInt
	push	%eax
	# get arg a
	mov	12(%edx),%eax
	push	%eax
	mov	8(%edx),%eax
	push	%eax
	xcall	modf
	add	$12,%esp
	pop	%esi
	pop	%edx
	fstpl	(%edx)
	jmp	*%edi

#========================================

	opEntry	fmodfBop
	# modf( a, ptrToFloatWholeReturn )
	mov	%edx,%eax
	sub	$4,%edx
	push	%edx
	push	%esi
	# TOS is a (float)
	# we return TOS: bFrac aWhole
	# alloc nInt
	push	%eax
	# get arg a
	mov	4(%edx),%eax
	push	%eax
	xcall	modff
	add	$8,%esp
	pop	%esi
	pop	%edx
	fstp	(%edx)
	jmp	*%edi

#========================================

	opEntry	dfmodBop
	push	%edx
	push	%esi
	mov	4(%edx),%eax
	push	%eax
	mov	(%edx),%eax
	push	%eax
	mov	12(%edx),%eax
	push	%eax
	mov	8(%edx),%eax
	push	%eax
	xcall	fmod
	add	$16,%esp
	pop	%esi
	pop	%edx
	add	$8,%edx
	fstpl	(%edx)
	jmp	*%edi

#========================================

	opEntry	ffmodBop
	push	%edx
	push	%esi
	mov	(%edx),%eax
	push	%eax
	mov	4(%edx),%eax
	push	%eax
	xcall	fmodf
	add	$8,%esp
	pop	%esi
	pop	%edx
	add	$4,%edx
	fstp	(%edx)
	jmp	*%edi

#========================================

	opEntry	i2fBop
	fild	(%edx)
	fstp	(%edx)
	jmp	*%edi

#========================================

	opEntry	i2dBop
	fild	(%edx)
	sub	$4,%edx
	fstpl	(%edx)
	jmp	*%edi

#========================================

	opEntry	f2iBop
	flds	(%edx)
	fistp	(%edx)
	jmp	*%edi
	
#========================================

	opEntry	f2dBop
	flds	(%edx)
	sub	$4,%edx
	fstpl	(%edx)
	jmp	*%edi

#========================================

	opEntry	d2iBop
	fldl	(%edx)
	add	$4,%edx
	fistp	(%edx)
	jmp	*%edi

#========================================

	opEntry	d2fBop
	fldl	(%edx)
	add	$4,%edx
	fstp	(%edx)
	jmp	*%edi

#========================================

	opEntry	doExitBop
	mov	RPtr(%ebp),%eax
	mov	RTPtr(%ebp),%ebx
	cmp	%eax,%ebx
	jle	doExitBop1
	mov	(%eax),%esi
	add	$4,%eax
	mov	%eax,RPtr(%ebp)
	jmp	*%edi

doExitBop1:
	mov	$kForthErrorReturnStackUnderflow,%eax
	jmp	interpLoopErrorExit

#========================================

	opEntry	doExitLBop
    # rstack: local_var_storage oldFP oldIP
    # FP points to oldFP
	mov	FPtr(%ebp),%eax
	mov	(%eax),%esi
	mov	%esi,FPtr(%ebp)
	add	$4,%eax
	mov	RTPtr(%ebp),%ebx
	cmp	%eax,%ebx
	jle	doExitBop1
	mov	(%eax),%ecx
	add	$4,%eax
	mov	%eax,RPtr(%ebp)
	jmp	*%edi

#========================================


	opEntry	doExitMBop
    # rstack: oldIP oldTPV oldTPD
	mov	RPtr(%ebp),%eax
	mov	RTPtr(%ebp),%ebx
	add	$12,%eax
	cmp	%eax,%ebx
	jl	doExitBop1
	mov	%eax,RPtr(%ebp)
	mov	-12(%eax),%esi	# IP = oldIP
	mov	-8(%eax),%ebx
	mov	%ebx,TMPtr(%ebp)
	mov	-4(%eax),%ebx
	mov	%ebx,TDPtr(%ebp)
	jmp	*%edi

#========================================

	opEntry	doExitMLBop
    # rstack: local_var_storage oldFP oldIP oldTPV oldTPD
    # FP points to oldFP
	mov	FPtr(%ebp),%eax
	mov	(%eax),%esi
	mov	%esi,FPtr(%ebp)
	add	$16,%eax
	mov	RTPtr(%ebp),%ebx
	cmp	%eax,%ebx
	jl	doExitBop1
	mov	%eax,RPtr(%ebp)
	mov	-12(%eax),%esi	# IP = oldIP
	mov	-8(%eax),%ebx
	mov	%ebx,TMPtr(%ebp)
	mov	-4(%eax),%ebx
	mov	%ebx,TDPtr(%ebp)
	jmp	*%edi

#========================================

	opEntry	callBop
	# rpush current IP
	mov	RPtr(%ebp),%eax
	sub	$4,%eax
	mov	%esi,(%eax)
	mov	%eax,RPtr(%ebp)
	# pop new IP
	mov	(%edx),%esi
	add	$4,%edx
	jmp	*%edi

#========================================

	opEntry	gotoBop
	mov	(%edx),%esi
	jmp	*%edi

#========================================
#
# TOS is start-index
# TOS+4 is end-index
# the op right after this one should be a branch
# 
	opEntry	doDoBop
	mov	RPtr(%ebp),%ebx
	sub	$12,%ebx
	mov	%ebx,RPtr(%ebp)
	# @RP-2 holds top-of-loop-IP
	add	$4,%esi    # skip over loop exit branch right after this op
	mov	%esi,8(%ebx)
	# @RP-1 holds end-index
	mov	4(%edx),%eax
	mov	%eax,4(%ebx)
	# @RP holds current-index
	mov	(%edx),%eax
	mov	%eax,(%ebx)
	add	$8,%edx
	jmp	*%edi

#========================================
#
# TOS is start-index
# TOS+4 is end-index
# the op right after this one should be a branch just past end of loop (used by leave)
# 
	opEntry	doCheckDoBop
	mov	(%edx),%eax		# eax is start index
	mov	4(%edx),%ecx	# esi is end index
	add	$8,%edx
	cmp	%ecx,%eax
	jge	doCheckDoBop1
	
	mov	RPtr(%ebp),%ebx
	sub	$12,%ebx
	mov	%ebx,RPtr(%ebp)
	# @RP-2 holds top-of-loop-IP
	add	$4,%esi    # skip over loop exit branch right after this op
	mov	%esi,8(%ebx)
	# @RP-1 holds end-index
	mov	%ecx,4(%ebx)
	# @RP holds current-index
	mov	%eax,(%ebx)
doCheckDoBop1:
	jmp	*%edi
	
#========================================

	opEntry	doLoopBop
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%eax
	inc	%eax
	cmp	4(%ebx),%eax
	jge	doLoopBop1	# jump if done
	mov	%eax,(%ebx)
	mov	8(%ebx),%esi
	jmp	*%edi

doLoopBop1:
	add	$12,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	*%edi

#========================================

	opEntry	doLoopNBop
	mov	RPtr(%ebp),%ebx	# ebp is RP
	mov	(%edx),%eax		# pop N into eax
	add	$4,%edx
	or	%eax,%eax
	jl	doLoopNBop1		# branch if increment negative
	add	(%ebx),%eax		# eax is new i
	cmp	4(%ebx),%eax
	jge	doLoopBop1		# jump if done
	mov	%eax,(%ebx)		# update i
	mov	8(%ebx),%esi		# branch to top of loop
	jmp	*%edi

doLoopNBop1:
	add	(%ebx),%eax
	cmp	4(%ebx),%eax
	jl	doLoopBop1		# jump if done
	mov	%eax,(%ebx)		# ipdate i
	mov	8(%ebx),%esi		# branch to top of loop
	jmp	*%edi

#========================================

	opEntry	iBop
	mov	RPtr(%ebp),%eax
	mov	(%eax),%ebx
	sub	$4,%edx
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	jBop
	mov	RPtr(%ebp),%eax
	mov	12(%eax),%ebx
	sub	$4,%edx
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	unloopBop
	mov	RPtr(%ebp),%eax
	add	$12,%eax
	mov	%eax,RPtr(%ebp)
	jmp	*%edi

#========================================

	opEntry	leaveBop
	mov	RPtr(%ebp),%eax
	# point IP at the branch instruction which is just before top of loop
	mov	8(%eax),%esi
	sub	$4,%esi
	# drop current index, end index, top-of-loop-IP
	add	$12,%eax
	mov	%eax,RPtr(%ebp)
	jmp	*%edi

#========================================

	opEntry	orBop
	mov	(%edx),%eax
	add	$4,%edx
	or	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	andBop
	mov	(%edx),%eax
	add	$4,%edx
	and	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	xorBop
	mov	(%edx),%eax
	add	$4,%edx
	xor	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	invertBop
	mov	$0x0FFFFFFFF,%eax
	xor	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	lshiftBop
	mov	(%edx),%ecx
	add	$4,%edx
	mov	(%edx),%ebx
	shl	%cl,%ebx
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	arshiftBop
	mov	(%edx),%ecx
	add	$4,%edx
	mov	(%edx),%ebx
	sar	%cl,%ebx
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	rshiftBop
	mov	(%edx),%ecx
	add	$4,%edx
	mov	(%edx),%ebx
	shr	%cl,%ebx
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	notBop
	mov	$0xFFFFFFFF,%eax
	xor	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	archX86Bop
	opEntry	trueBop
	mov	$0xFFFFFFFF,%eax
	sub	$4,%edx
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	archARMBop
	opEntry	falseBop
	xor	%eax,%eax
	sub	$4,%edx
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	nullBop
	xor	%eax,%eax
	sub	$4,%edx
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	oclearBop
	mov	$kVarStore,%eax
	mov	%eax,varMode(%ebp)
	# fall thru to dnull
	
	opEntry	dnullBop
	xor	%eax,%eax
	sub	$8,%edx
	mov	%eax,4(%edx)
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	equals0Bop
	xor	%ebx,%ebx
	jmp	equalsBop1

#========================================

	opEntry	equalsBop
	mov	(%edx),%ebx
	add	$4,%edx
equalsBop1:
	xor	%eax,%eax
	cmp	(%edx),%ebx
	jnz	equalsBop2
	dec	%eax
equalsBop2:
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	notEquals0Bop
	xor	%ebx,%ebx
	jmp	notEqualsBop1

#========================================

	opEntry	notEqualsBop
	mov	(%edx),%ebx
	add	$4,%edx
notEqualsBop1:
	xor	%eax,%eax
	cmp	(%edx),%ebx
	jz	notEqualsBop2
	dec	%eax
notEqualsBop2:
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	greaterThan0Bop
	xor	%ebx,%ebx
	jmp	gtBop1

#========================================

	opEntry	greaterThanBop
	mov	(%edx),%ebx		# ebx = b
	add	$4,%edx
gtBop1:
	xor	%eax,%eax
	cmp	%ebx,(%edx)
	jle	gtBop2
	dec	%eax
gtBop2:
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	greaterEquals0Bop
	xor	%ebx,%ebx
	jmp	geBop1

#========================================

	opEntry	greaterEqualsBop
	mov	(%edx),%ebx
	add	$4,%edx
geBop1:
	xor	%eax,%eax
	cmp	%ebx,(%edx)
	jl	geBop2
	dec	%eax
geBop2:
	mov	%eax,(%edx)
	jmp	*%edi


#========================================

	opEntry	lessThan0Bop
	xor	%ebx,%ebx
	jmp	ltBop1

#========================================

	opEntry	lessThanBop
	mov	(%edx),%ebx
	add	$4,%edx
ltBop1:
	xor	%eax,%eax
	cmp	%ebx,(%edx)
	jge	ltBop2
	dec	%eax
ltBop2:
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	lessEquals0Bop
	xor	%ebx,%ebx
	jmp	leBop1

#========================================

	opEntry	lessEqualsBop
	mov	(%edx),%ebx
	add	$4,%edx
leBop1:
	xor	%eax,%eax
	cmp	%ebx,(%edx)
	jg	leBop2
	dec	%eax
leBop2:
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	unsignedGreaterThanBop
	mov	(%edx),%ebx
	add	$4,%edx
ugtBop1:
	xor	%eax,%eax
	cmp	%ebx,(%edx)
	jbe	ugtBop2
	dec	%eax
ugtBop2:
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	unsignedLessThanBop
	mov	(%edx),%ebx
	add	$4,%edx
ultBop1:
	xor	%eax,%eax
	cmp	%ebx,(%edx)
	jae	ultBop2
	dec	%eax
ultBop2:
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	withinBop
	# tos: hiLimit loLimit value
	xor	%eax,%eax
	mov	8(%edx),%ebx	# ebx = value
withinBop1:
	cmp	%ebx,(%edx)
	jle	withinFail
	cmp	%ebx,4(%edx)
	jg	withinFail
	dec	%eax
withinFail:
	add	$8,%edx
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	minBop
	mov	(%edx),%ebx
	add	$4,%edx
	cmp	%ebx,(%edx)
	jl	minBop1
	mov	%ebx,(%edx)
minBop1:
	jmp	*%edi

#========================================

	opEntry	maxBop
	mov	(%edx),%ebx
	add	$4,%edx
	cmp	%ebx,(%edx)
	jg	maxBop1
	mov	%ebx,(%edx)
maxBop1:
	jmp	*%edi

#========================================

	opEntry	icmpBop
	mov	(%edx),%ebx		# ebx = b
	add	$4,%edx
	xor	%eax, %eax
	cmp	%ebx,(%edx)
	jz	icmpBop3
	jl	icmpBop2
	add	$2,%eax
icmpBop2:
	dec	%eax
icmpBop3:
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	uicmpBop
	mov	(%edx),%ebx		# ebx = b
	add	$4,%edx
	xor	%eax, %eax
	cmp	%ebx,(%edx)
	jz	uicmpBop3
	jb	uicmpBop2
	add	$2,%eax
uicmpBop2:
	dec	%eax
uicmpBop3:
	mov	%eax,(%edx)
	jmp	*%edi


#========================================

	opEntry	rpushBop
	mov	(%edx),%ebx
	add	$4,%edx
	mov	RPtr(%ebp),%eax
	sub	$4,%eax
	mov	%eax,RPtr(%ebp)
	mov	%ebx,(%eax)
	jmp	*%edi

#========================================

	opEntry	rpopBop
	mov	RPtr(%ebp),%eax
	mov	(%edx),%ebx
	add	$4,%eax
	mov	%eax,RPtr(%ebp)
	sub	$4,%edx
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	rpeekBop
	mov	RPtr(%ebp),%eax
	mov	(%eax),%ebx
	sub	$4,%edx
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	rdropBop
	mov	RPtr(%ebp),%eax
	add	$4,%eax
	mov	%eax,RPtr(%ebp)
	jmp	*%edi

#========================================

	opEntry	rpBop
	lea	RPtr(%ebp),%eax
	jmp	intEntry

#========================================

	opEntry	r0Bop
	mov	RTPtr(%ebp),%eax
	sub	$4,%edx
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	dupBop
	mov	(%edx),%eax
	sub	$4,%edx
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	checkDupBop
	mov	(%edx),%eax
	or	%eax,%eax
	jz	dupNon0Bop1
	sub	$4,%edx
	mov	%eax,(%edx)
dupNon0Bop1:
	jmp	*%edi

#========================================

	opEntry	swapBop
	mov	(%edx),%eax
	mov	4(%edx),%ebx
	mov	%ebx,(%edx)
	mov	%eax,4(%edx)
	jmp	*%edi

#========================================

	opEntry	dropBop
	add	$4,%edx
	jmp	*%edi

#========================================

	opEntry	overBop
	mov	4(%edx),%eax
	sub	$4,%edx
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	rotBop
	mov	(%edx),%eax		# tos[0], will go in tos[1]
	mov	8(%edx),%ebx	# tos[2], will go in tos[0]
	mov	%ebx,(%edx)
	mov	4(%edx),%ebx	# tos[1], will go in tos[2]
	mov	%ebx,8(%edx)
	mov	%eax,4(%edx)
	jmp	*%edi

#========================================

	opEntry	reverseRotBop
	mov	(%edx),%eax		# tos[0], will go in tos[2]
	mov	4(%edx),%ebx	# tos[1], will go in tos[0]
	mov	%ebx,(%edx)
	mov	8(%edx),%ebx	# tos[2], will go in tos[1]
	mov	%ebx,4(%edx)
	mov	%eax,8(%edx)
	jmp	*%edi

#========================================

	opEntry	nipBop
	mov	(%edx),%eax
	add	$4,%edx
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	tuckBop
	mov	(%edx),%eax
	mov	4(%edx),%ebx
	sub	$4,%edx
	mov	%eax,(%edx)
	mov	%ebx,4(%edx)
	mov	%eax,8(%edx)
	jmp	*%edi

#========================================

	opEntry	pickBop
	mov	(%edx),%eax
	add	$1,%eax
	mov	(%edx,%eax,4),%ebx
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	spBop
	mov	%edx,%eax
	sub	$4,%edx
	mov	%eax,(%edx)
	jmp	*%edi

	# this is overkill to make sp look like other vars
	mov	varMode(%ebp),%ebx
	xor	%eax,%eax
	mov	%eax,varMode(%ebp)
	cmp	kVarMinusStore,%ebx
	jg	badVarOperation
	# dispatch based on value in ebx
	mov	spActionTable(,%ebx,4),%ebx
	jmp	*%ebx
	
spFetch:
	mov	%edx,%eax
	sub	$4,%edx
	mov	%eax,(%edx)
	jmp	*%edi

spRef:
	# returns address of SP shadow copy
	lea	SPtr(%ebp),%eax
	sub	$4,%edx
	mov	%eax,(%edx)
	jmp	*%edi
	
spStore:
	mov	(%edx),%ebx
	mov	%ebx,%edx
	jmp	*%edi

spPlusStore:
	mov	(%edx),%eax
	add	$4,%edx
	add	%eax,%edx
	jmp	*%edi

spMinusStore:
	mov	(%edx),%eax
	add	$4,%edx
	sub	%eax,%edx
	jmp	*%edi

spActionTable:
	.long	spFetch
	.long	spFetch
	.long	spRef
	.long	spStore
	.long	spPlusStore
	.long	spMinusStore


#========================================

	opEntry	s0Bop
	mov	STPtr(%ebp),%eax
	sub	$4,%edx
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	fpBop
	lea	FPtr(%ebp),%eax
	jmp	intEntry

#========================================

	opEntry	ipBop
	# let the common intVarAction code change the shadow copy of IP,
	# then jump back to ipFixup to copy the shadow copy of IP into IP register (esi)
	push	%edi
	mov	%esi,IPtr(%ebp)
	lea	IPtr(%ebp),%eax
	mov	$ipFixup1,%edi
	jmp	intEntry
	
	opEntry	ipFixup
ipFixup1:
	mov	IPtr(%ebp),%esi
	pop	%edi
	jmp	*%edi
	
#========================================

	opEntry	ddupBop
	mov	(%edx),%eax
	mov	4(%edx),%ebx
	sub	$8,%edx
	mov	%eax,(%edx)
	mov	%ebx,4(%edx)
	jmp	*%edi

#========================================

	opEntry	dswapBop
	mov	(%edx),%eax
	mov	8(%edx),%ebx
	mov	%eax,8(%edx)
	mov	%ebx,(%edx)
	mov	4(%edx),%eax
	mov	12(%edx),%ebx
	mov	%eax,12(%edx)
	mov	%ebx,4(%edx)
	jmp	*%edi

#========================================

	opEntry	ddropBop
	add	$8,%edx
	jmp	*%edi

#========================================

	opEntry	doverBop
	mov	8(%edx),%eax
	mov	12(%edx),%ebx
	sub	$8,%edx
	mov	%eax,(%edx)
	mov	%ebx,4(%edx)
	jmp	*%edi

#========================================

	opEntry	drotBop
	mov	20(%edx),%eax
	mov	12(%edx),%ebx
	mov	%ebx,20(%edx)
	mov	4(%edx),%ebx
	mov	%ebx,12(%edx)
	mov	%eax,4(%edx)
	mov	16(%edx),%eax
	mov	8(%edx),%ebx
	mov	%ebx,16(%edx)
	mov	(%edx),%ebx
	mov	%ebx,8(%edx)
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	startTupleBop
	mov	RPtr(%ebp),%eax
	sub	$4,%eax
	mov	%eax,RPtr(%ebp)
	mov	%edx,(%eax)
	jmp	*%edi

#========================================

	opEntry	endTupleBop
	mov	RPtr(%ebp),%eax
	mov	(%eax),%ebx
	add	$4,%eax
	mov	%eax,RPtr(%ebp)
	sub	%edx,%ebx
	sub	$4,%edx
	sar	$2,%ebx
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	hereBop
	mov	DictionaryPtr(%ebp),%eax
	mov	MemSectionCurrentPtr(%eax),%eax
	sub	$4,%edx
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	storeBop
	mov	(%edx),%eax
	mov	4(%edx),%ebx
	add	$8,%edx
	mov	%ebx,(%eax)
	jmp	*%edi

#========================================

	opEntry	fetchBop
	mov	(%edx),%eax
	mov	(%eax),%ebx
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	storeNextBop
	mov	(%edx),%eax
	mov (%eax),%ecx
	mov	4(%edx),%ebx
	add	$8,%edx
	mov	%ebx,(%ecx)
	add $4,%ecx
	mov %ecx,(%eax)
	jmp	*%edi

#========================================

	opEntry	fetchNextBop
	mov	(%edx),%eax
	mov (%eax),%ecx
	mov	(%ecx),%ebx
	mov	%ebx,(%edx)
	add $4,%ecx
	mov %ecx,(%eax)
	jmp	*%edi

#========================================

	opEntry	cstoreBop
	mov	(%edx),%eax
	mov	4(%edx),%ebx
	add	$8,%edx
	movb	%bl,(%eax)
	jmp	*%edi

#========================================

	opEntry	cfetchBop
	mov	(%edx),%eax
	xor	%ebx,%ebx
	movb	(%eax),%bl
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	cstoreNextBop
	mov	(%edx),%eax
	mov (%eax),%ecx
	mov	4(%edx),%ebx
	add	$8,%edx
	movb	%bl,(%ecx)
	add $1,%ecx
	mov %ecx,(%eax)
	jmp	*%edi

#========================================

	opEntry	cfetchNextBop
	mov	(%edx),%eax
	mov (%eax),%ecx
	xor	%ebx,%ebx
	movb	(%ecx),%bl
	mov	%ebx,(%edx)
	add $1,%ecx
	mov %ecx,(%eax)
	jmp	*%edi

#========================================

	opEntry	scfetchBop
	mov	(%edx),%eax
	movsxb	(%eax),%ebx
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	c2iBop
	mov	(%edx),%eax
	movsxb	%al,%ebx
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	wstoreBop
	mov	(%edx),%eax
	movw	4(%edx),%bx
	add	$8,%edx
	movw	%bx,(%eax)
	jmp	*%edi

#========================================

	opEntry	wstoreNextBop
	mov	(%edx),%eax
	mov (%eax),%ecx
	mov	4(%edx),%ebx
	add	$8,%edx
	movw	%bx,(%ecx)
	add $2,%ecx
	mov %ecx,(%eax)
	jmp	*%edi

#========================================

	opEntry	wfetchBop
	mov	(%edx),%eax
	xor	%ebx,%ebx
	movw	(%eax),%bx
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	wfetchNextBop
	mov	(%edx),%eax
	mov (%eax),%ecx
	xor	%ebx,%ebx
	movw	(%ecx),%bx
	mov	%ebx,(%edx)
	add $2,%ecx
	mov %ecx,(%eax)
	jmp	*%edi

#========================================

	opEntry	swfetchBop
	mov	(%edx),%eax
	movsxw	(%eax),%ebx
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	w2iBop
	mov	(%edx),%eax
	movsxw	%ax,%ebx
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	dstoreBop
	mov	(%edx),%eax
	mov	4(%edx),%ebx
	mov	%ebx,(%eax)
	mov	8(%edx),%ebx
	mov	%ebx,4(%eax)
	add	$12,%edx
	jmp	*%edi

#========================================

	opEntry	dstoreNextBop
	mov	(%edx),%eax
	mov (%eax),%ecx
	mov	4(%edx),%ebx
	mov	%ebx,(%ecx)
	mov	8(%edx),%ebx
	mov	%ebx,4(%ecx)
	add $8,%ecx
	mov %ecx,(%eax)
	add	$12,%edx
	jmp	*%edi

#========================================

	opEntry	dfetchBop
	mov	(%edx),%eax
	sub	$4,%edx
	mov	(%eax),%ebx
	mov	%ebx,(%edx)
	mov	4(%eax),%ebx
	mov	%ebx,4(%edx)
	jmp	*%edi

#========================================

	opEntry	dfetchNextBop
	mov	(%edx),%eax
	sub	$4,%edx
	mov (%eax),%ecx
	mov	(%ecx),%ebx
	mov	%ebx,(%edx)
	mov	4(%ecx),%ebx
	mov	%ebx,4(%edx)
	add $8,%ecx
	mov %ecx,(%eax)
	jmp	*%edi

#========================================

	opEntry	ostoreBop
	mov	(%edx),%eax
	mov	4(%edx),%ebx
	mov	%ebx,4(%eax)
	mov	8(%edx),%ebx
	mov	%ebx,(%eax)
	add	$12,%edx
	jmp	*%edi

#========================================

	opEntry	ostoreNextBop
	mov	(%edx),%eax
	mov (%eax),%ecx
	mov	4(%edx),%ebx
	mov	%ebx,4(%ecx)
	mov	8(%edx),%ebx
	mov	%ebx,(%ecx)
	add $8,%ecx
	mov %ecx,(%eax)
	add	$12,%edx
	jmp	*%edi

#========================================

	opEntry	ofetchBop
	mov	(%edx),%eax
	sub	$4,%edx
	mov	4(%eax),%ebx
	mov	%ebx,(%edx)
	mov	(%eax),%ebx
	mov	%ebx,4(%edx)
	jmp	*%edi

#========================================

	opEntry	ofetchNextBop
	mov	(%edx),%eax
	sub	$4,%edx
	mov (%eax),%ecx
	mov	4(%ecx),%ebx
	mov	%ebx,(%edx)
	mov	(%ecx),%ebx
	mov	%ebx,4(%edx)
	add $8,%ecx
	mov %ecx,(%eax)
	jmp	*%edi

#========================================

	opEntry	moveBop
	#	TOS: nBytes dstPtr srcPtr
	#	TOS: nBytes srcPtr dstPtr
	push	%edx
	push	%esi
	mov	(%edx),%eax
	push	%eax
	mov	8(%edx),%eax
	push	%eax
	mov	4(%edx),%eax
	push	%eax
	xcall	memmove
	add	$12,%esp
	pop	%esi
	pop	%edx
	add	$12,%edx
	jmp	*%edi

#========================================

	opEntry	fillBop
	#	TOS: nBytes byteVal dstPtr
	push	%edx
	push	%esi
	mov	4(%edx),%eax
	push	%eax
	mov	(%edx),%eax
	and	$0x0FF,%eax
	push	%eax
	mov	8(%edx),%eax
	push	%eax
	xcall	memset
	add	$12,%esp
	pop	%esi
	pop	%edx
	add	$12,%edx
	jmp	*%edi

#========================================

	opEntry	intoBop
	mov	$kVarStore,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

#========================================

	opEntry	addToBop
	mov	$kVarPlusStore,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

#========================================

	opEntry	subtractFromBop
	mov	$kVarMinusStore,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

#========================================

	opEntry	addressOfBop
	mov	$kVarRef,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

#========================================

	opEntry	removeEntryBop
	mov	$kVocabRemoveEntry,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

#========================================

	opEntry	entryLengthBop
	mov	$kVocabEntryLength,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

#========================================

	opEntry	numEntriesBop
	mov	$kVocabNumEntries,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

#========================================

	opEntry	vocabToClassBop
	mov	$kVocabGetClass,%eax
	mov	%eax,varMode(%ebp)
	jmp	*%edi

#========================================

	opEntry	setVarActionBop
	mov	(%edx),%eax
	add	$4,%edx
	mov	%eax,varMode(%ebp)
	jmp	*%edi

#========================================

	opEntry	getVarActionBop
	mov	varMode(%ebp),%eax
	sub	$4,%edx
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	byteVarActionBop
	mov	(%edx),%eax
	add	$4,%edx
	jmp	byteEntry
	
#========================================

	opEntry	ubyteVarActionBop
	mov	(%edx),%eax
	add	$4,%edx
	jmp	ubyteEntry
	
#========================================

	opEntry	shortVarActionBop
	mov	(%edx),%eax
	add	$4,%edx
	jmp	shortEntry
	
#========================================

	opEntry	ushortVarActionBop
	mov	(%edx),%eax
	add	$4,%edx
	jmp	ushortEntry
	
#========================================

	opEntry	intVarActionBop
	mov	(%edx),%eax
	add	$4,%edx
	jmp	intEntry
	
#========================================

	opEntry	floatVarActionBop
	mov	(%edx),%eax
	add	$4,%edx
	jmp	floatEntry
	
#========================================

	opEntry	doubleVarActionBop
	mov	(%edx),%eax
	add	$4,%edx
	jmp	doubleEntry
	
#========================================

	opEntry	longVarActionBop
	mov	(%edx),%eax
	add	$4,%edx
	jmp	longEntry
	
#========================================

	opEntry	opVarActionBop
	mov	(%edx),%eax
	add	$4,%edx
	jmp	opEntry
	
#========================================

	opEntry	objectVarActionBop
	mov	(%edx),%eax
	add	$4,%edx
	jmp	objectEntry
	
#========================================

	opEntry	stringVarActionBop
	mov	(%edx),%eax
	add	$4,%edx
	jmp	stringEntry
	
#========================================

	opEntry	strcpyBop
	#	TOS: srcPtr dstPtr
	push	%edx
	push	%esi
	mov	(%edx),%eax
	push	%eax
	mov	4(%edx),%eax
	push	%eax
	xcall	strcpy
	add	$8,%esp
	pop	%esi
	pop	%edx
	add	$8,%edx
	jmp	*%edi

#========================================

	opEntry	strncpyBop
	#	TOS: maxBytes srcPtr dstPtr
	push	%edx
	push	%esi
	mov	(%edx),%eax
	push	%eax
	mov	4(%edx),%eax
	push	%eax
	mov	8(%edx),%eax
	push	%eax
	xcall	strncpy
	add	$12,%esp
	pop	%esi
	pop	%edx
	add	$12,%edx
	jmp	*%edi

#========================================

	opEntry	strlenBop
	mov	(%edx),%eax
	xor	%ebx,%ebx
strlenBop1:
	movb	(%eax),%bh
	testb	$255,%bh
	jz	strlenBop2
	addb	$1,%bl
	add	$1,%eax
	jmp	strlenBop1

strlenBop2:
	xorb	%bh,%bh
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	strcatBop
	#	TOS: srcPtr dstPtr
	push	%edx
	push	%esi
	mov	(%edx),%eax
	push	%eax
	mov	4(%edx),%eax
	push	%eax
	xcall	strcat
	add	$8,%esp
	pop	%esi
	pop	%edx
	add	$8,%edx
	jmp	*%edi

#========================================

	opEntry	strncatBop
	#	TOS: maxBytes srcPtr dstPtr
	push	%edx
	push	%esi
	mov	(%edx),%eax
	push	%eax
	mov	4(%edx),%eax
	push	%eax
	mov	8(%edx),%eax
	push	%eax
	xcall	strncat
	add	$12,%esp
	pop	%esi
	pop	%edx
	add	$12,%edx
	jmp	*%edi

#========================================

	opEntry	strchrBop
	#	TOS: char strPtr
	push	%edx
	push	%esi
	mov	(%edx),%eax
	push	%eax
	mov	4(%edx),%eax
	push	%eax
	xcall	strchr
	add	$8,%esp
	pop	%esi
	pop	%edx
	add	$4,%edx
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	strrchrBop
	#	TOS: char strPtr
	push	%edx
	push	%esi
	mov	(%edx),%eax
	push	%eax
	mov	4(%edx),%eax
	push	%eax
	xcall	strrchr
	add	$8,%esp
	pop	%esi
	pop	%edx
	add	$4,%edx
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	strcmpBop
	#	TOS: ptr2 ptr1
	push	%edx
	push	%esi
	mov	(%edx),%eax
	push	%eax
	mov	4(%edx),%eax
	push	%eax
	xcall	strcmp
strcmp1:
	xor	%ebx,%ebx
	cmp	%ebx,%eax
	jz	strcmp3		# if strings equal, return 0
	jg	strcmp2
	sub	$2,%ebx
strcmp2:
	inc	%ebx
strcmp3:
	add	$8,%esp
	pop	%esi
	pop	%edx
	add	$4,%edx
	mov	%ebx,(%edx)
	jmp	*%edi

#========================================

	opEntry	stricmpBop
	#	TOS: ptr2 ptr1
	push	%edx
	push	%esi
	mov	(%edx),%eax
	push	%eax
	mov	4(%edx),%eax
	push	%eax
	xcall	strcasecmp
	jmp	strcmp1

#========================================

	opEntry	strstrBop
	#	TOS: ptr2 ptr1
	push	%edx
	push	%esi
	mov	(%edx),%eax
	push	%eax
	mov	4(%edx),%eax
	push	%eax
	xcall	strstr
	add	$8,%esp
	pop	%esi
	pop	%edx
	add	$4,%edx
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	strtokBop
	#	TOS: ptr2 ptr1
	push	%edx
	push	%esi
	mov	(%edx),%eax
	push	%eax
	mov	4(%edx),%eax
	push	%eax
	xcall	strtok
	add	$8,%esp
	pop	%esi
	pop	%edx
	add	$4,%edx
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	litBop
	opEntry	flitBop
	mov	(%esi),%eax
	add	$4,%esi
	sub	$4,%edx
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	dlitBop
	mov	(%esi),%eax
	mov	4(%esi),%ebx
	add	$8,%esi
	sub	$8,%edx
	mov	%eax,(%edx)
	mov	%ebx,4(%edx)
	jmp	*%edi

#========================================

# doDoes is executed while executing the user word
# it puts the parameter address of the user word on TOS
# top of rstack is parameter address
#
# : plusser builds , does @ + ;
# 5 plusser plus5
#
# the above 2 lines generates 3 new ops:
#	plusser
#	unnamed op
#	plus5
#
# code generated for above:
#
# plusser userOp(100) starts here
#	0	op(builds)
#	4	op(comma)
#	8	op(endBuilds)		compiled by "does"
#	12	101					compiled by "does"
# unnamed userOp(101) starts here
#	16	op(doDoes)			compiled by "does"
#	20	op(fetch)
#	24	op(plus)
#	28	op(doExit)
#
# plus5 userOp(102) starts here
#	32	userOp(101)
#	36	5
#
# ...
#	68	intCons(7)
#	72	userOp(102)		"plus5"
#	76	op(%d)
#
# we are executing some userOp when we hit the plus5 at 72
#	IP		next op			PS		RS
#--------------------------------------------
#	68		intCons(7)		()
#	72		userOp(102)		(7)		()
#	32		userOp(101)		(7)		(76)
#	16		op(doDoes)		(7)		(36,76)
#	20		op(fetch)		(36,7)	(76)
#	24		op(plus)		(5,7)	(76)
#	28		op(doExit)		(12)	(76)
#	76		op(%d)			(12)	()
#
	opEntry	doDoesBop
	mov	RPtr(%ebp),%eax
	sub	$4,%edx
	mov	(%eax),%ebx	# ebx points at param field
	mov	%ebx,(%edx)
	add	$4,%eax
	mov	%eax,RPtr(%ebp)
	jmp	*%edi

#========================================

	opEntry	doVariableBop
	# push IP
	sub	$4,%edx
	mov	%esi,(%edx)
	# rpop new ip
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	*%edi

#========================================

	opEntry	doConstantBop
	# push longword @ IP
	mov	(%esi),%eax
	sub	$4,%edx
	mov	%eax,(%edx)
	# rpop new ip
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	*%edi

#========================================

	opEntry	doDConstantBop
	# push quadword @ IP
	mov	(%esi),%eax
	sub	$8,%edx
	mov	%eax,(%edx)
	mov	4(%esi),%eax
	mov	%eax,4(%edx)
	# rpop new ip
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	*%edi

#========================================

	opEntry	doStructBop
	# push IP
	sub	$4,%edx
	mov	%esi,(%edx)
	# rpop new ip
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	*%edi

#========================================

	opEntry	doStructArrayBop
	# TOS is array index
	# esi -> bytes per element, followed by element 0
	mov	(%esi),%eax		# eax = bytes per element
	add	$4,%esi			# ecx -> element 0
	imul	(%edx),%eax
	add	%esi,%eax		# add in array base addr
	mov	%eax,(%edx)
	# rpop new ip
	mov	RPtr(%ebp),%ebx
	mov	(%ebx),%esi
	add	$4,%ebx
	mov	%ebx,RPtr(%ebp)
	jmp	*%edi

#========================================

	opEntry	thisBop
	mov	TMPtr(%ebp),%eax
	sub	$8,%edx
	mov	%eax,(%edx)
	mov	TDPtr(%ebp),%eax
	mov	%eax,4(%edx)
	jmp	*%edi

#========================================

	opEntry	thisDataBop
	mov	TDPtr(%ebp),%eax
	sub	$4,%edx
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	thisMethodsBop
	mov	TMPtr(%ebp),%eax
	sub	$4,%edx
	mov	%eax,(%edx)
	jmp	*%edi

#========================================

	opEntry	executeBop
	mov	(%edx),%ebx
	add	$4,%edx
	jmp	interpLoopExecuteEntry

#========================================

	opEntry	fopenBop
	push	%esi
	mov	(%edx),%eax	# pop access string
	add	$4,%edx
	push	%edx
	push	%eax
	mov	(%edx),%eax	# pop pathname string
	push	%eax
	mov	FileFuncs(%ebp),%eax
	mov	fileOpen(%eax),%eax
	call	*%eax
	addw	$8,%sp
	pop	%edx
	pop	%esi
	mov	%eax,(%edx)	# push fopen result
	jmp	*%edi

#========================================

	opEntry	fcloseBop
	push	%esi
	mov	(%edx),%eax	# pop file pointer
	push	%edx
	push	%eax
	mov	FileFuncs(%ebp),%eax
	mov	fileClose(%eax),%eax
	call	*%eax
	addw	$4,%sp
	pop	%edx
	pop	%esi
	mov	%eax,(%edx)	# push fclose result
	jmp	*%edi

#========================================

	opEntry	fseekBop
	push	%esi
	push	%edx
	mov	(%edx),%eax	# pop control
	push	%eax
	mov	4(%edx),%eax	# pop offset
	push	%eax
	mov	8(%edx),%eax	# pop file pointer
	push	%eax
	mov	FileFuncs(%ebp),%eax
	mov	fileSeek(%eax),%eax
	call	*%eax
	addw	$12,%sp
	pop	%edx
	pop	%esi
	add	$8,%edx
	mov	%eax,(%edx)	# push fseek result
	jmp	*%edi

#========================================

	opEntry	freadBop
	push	%esi
	push	%edx
	mov	(%edx),%eax	# pop file pointer
	push	%eax
	mov	4(%edx),%eax	# pop numItems
	push	%eax
	mov	8(%edx),%eax	# pop item size
	push	%eax
	mov	12(%edx),%eax	# pop dest pointer
	push	%eax
	mov	FileFuncs(%ebp),%eax
	mov	fileRead(%eax),%eax
	call	*%eax
	addw	$16,%sp
	pop	%edx
	pop	%esi
	add	$12,%edx
	mov	%eax,(%edx)	# push fread result
	jmp	*%edi

#========================================

	opEntry	fwriteBop
	push	%esi
	push	%edx
	mov	(%edx),%eax	# pop file pointer
	push	%eax
	mov	4(%edx),%eax	# pop numItems
	push	%eax
	mov	8(%edx),%eax	# pop item size
	push	%eax
	mov	12(%edx),%eax	# pop dest pointer
	push	%eax
	mov	FileFuncs(%ebp),%eax
	mov	fileWrite(%eax),%eax
	call	*%eax
	addw	$16,%sp
	pop	%edx
	pop	%esi
	add	$12,%edx
	mov	%eax,(%edx)	# push fwrite result
	jmp	*%edi

#========================================

	opEntry	fgetcBop
	push	%esi
	mov	(%edx),%eax	# pop file pointer
	push	%edx
	push	%eax
	mov	FileFuncs(%ebp),%eax
	mov	fileGetChar(%eax),%eax
	call	*%eax
	addw	$4,%sp
	pop	%edx
	pop	%esi
	mov	%eax,(%edx)	# push fgetc result
	jmp	*%edi

#========================================

	opEntry	fputcBop
	push	%esi
	mov	(%edx),%eax	# pop char to put
	add	$4,%edx
	push	%edx
	push	%eax
	mov	(%edx),%eax	# pop file pointer
	push	%eax
	mov	FileFuncs(%ebp),%eax
	mov	filePutChar(%eax),%eax
	call	*%eax
	addw	$8,%sp
	pop	%edx
	pop	%esi
	mov	%eax,(%edx)	# push fputc result
	jmp	*%edi

#========================================

	opEntry	feofBop
	push	%esi
	mov	(%edx),%eax	# pop file pointer
	push	%edx
	push	%eax
	mov	FileFuncs(%ebp),%eax
	mov	fileAtEnd(%eax),%eax
	call	*%eax
	addw	$4,%sp
	pop	%edx
	pop	%esi
	mov	%eax,(%edx)	# push feof result
	jmp	*%edi

#========================================

	opEntry	fexistsBop
	push	%esi
	mov	(%edx),%eax	# pop filename pointer
	push	%edx
	push	%eax
	mov	FileFuncs(%ebp),%eax
	mov	fileExists(%eax),%eax
	call	*%eax
	addw	$4,%sp
	pop	%edx
	pop	%esi
	mov	%eax,(%edx)	# push fexists result
	jmp	*%edi

#========================================

	opEntry	ftellBop
	push	%esi
	mov	(%edx),%eax	# pop file pointer
	push	%edx
	push	%eax
	mov	FileFuncs(%ebp),%eax
	mov	fileTell(%eax),%eax
	call	*%eax
	addw	$4,%sp
	pop	%edx
	pop	%esi
	mov	%eax,(%edx)	# push ftell result
	jmp	*%edi

#========================================

	opEntry	flenBop
	push	%esi
	mov	(%edx),%eax	# pop file pointer
	push	%edx
	push	%eax
	mov	FileFuncs(%ebp),%eax
	mov	fileGetLength(%eax),%eax
	call	*%eax
	addw	$4,%sp
	pop	%edx
	pop	%esi
	mov	%eax,(%edx)	# push flen result
	jmp	*%edi

#========================================

	opEntry	fgetsBop
	push	%esi
	push	%edx
	mov	(%edx),%eax	# pop file
	push	%eax
	mov	4(%edx),%eax	# pop maxChars
	push	%eax
	mov	8(%edx),%eax	# pop buffer
	push	%eax
	mov	FileFuncs(%ebp),%eax
	mov	fileGetString(%eax),%eax
	call	*%eax
	addw	$12,%sp
	pop	%edx
	pop	%esi
	add	$8,%edx
	mov	%eax,(%edx)	# push fgets result
	jmp	*%edi

#========================================

	opEntry	fputsBop
	push	%esi
	push	%edx
	mov	(%edx),%eax	# pop file
	push	%eax
	mov	4(%edx),%eax	# pop buffer
	push	%eax
	mov	FileFuncs(%ebp),%eax
	mov	filePutString(%eax),%eax
	call	*%eax
	addw	$8,%sp
	pop	%edx
	pop	%esi
	add	$4,%edx
	mov	%eax,(%edx)	# push fseek result
	jmp	*%edi

#========================================

#extern void sprintfSub( ForthCoreState* pCore );
#extern void fscanfSub( ForthCoreState* pCore );
#extern void sscanfSub( ForthCoreState* pCore );

#========================================

fprintfSubCore:
	mov	(%edx),%edi
	add	$2,%edi
	add	$4,%edx
	mov	%edi,%esi
fprintfSub1:
	sub	$1,%esi
	jl	fprintfSub2
	mov	(%edx),%ebx
	add	$4,%edx
	push	%ebx
	jmp	fprintfSub1
fprintfSub2:
	# all args have been moved from parameter stack to PC stack
	mov	%edx,SPtr(%ebp)
	xcall	fprintf
	mov	SPtr(%ebp),%edx
	sub	$4,%edx
	mov	%eax,(%edx)		# return result on parameter stack
	mov	%edx,SPtr(%ebp)
	# cleanup PC stack
	mov	%edi,%ebx
	sal	$2,%ebx
	add	%ebx,%esp
	ret

# extern void fprintfSub( ForthCoreState* pCore );

#	fprintfSub	,PROC	near	C	public	uses	%ebx	ecx	%edx	%esi	edi	%ebp
	subEntry	fprintfSub
	push %ebp
	mov	%esp,%ebp
	push %ebx
	push %ecx
	push %edx
	push %esi
	push %edi
	push %ebp
	
	mov	8(%ebp),%ebp		# ebp -> ForthCore
	mov	SPtr(%ebp),%edx
	call	fprintfSubCore
	
	pop %ebp
	pop	%edi
	pop	%esi
	pop %edx
	pop %ecx
	pop %ebx
	leave
	ret

#========================================

sprintfSubCore:
	mov	(%edx),%edi
	add	$2,%edi
	add	$4,%edx
	mov	%edi,%esi
sprintfSub1:
	sub	$1,%esi
	jl	sprintfSub2
	mov	(%edx),%ebx
	add	$4,%edx
	push	%ebx
	jmp	sprintfSub1
sprintfSub2:
	# all args have been moved from parameter stack to PC stack
	mov	%edx,SPtr(%ebp)
	xcall	sprintf
	mov	SPtr(%ebp),%edx
	sub	$4,%edx
	mov	%eax,(%edx)		# return result on parameter stack
	mov	%edx,SPtr(%ebp)
	# cleanup PC stack
	mov	%edi,%ebx
	sal	$2,%ebx
	add	%ebx,%esp
	ret

# extern long sprintfSub( ForthCoreState* pCore );

#	sprintfSub	,PROC	near	C	public	uses	%ebx	ecx	%edx	%esi	edi	%ebp
	subEntry	sprintfSub
	push %ebp
	mov	%esp,%ebp
	push %ebx
	push %ecx
	push %edx
	push %esi
	push %edi
	push %ebp
	
	mov	8(%ebp),%ebp		# ebp -> ForthCore
	mov	SPtr(%ebp),%edx
	call	sprintfSubCore
	
	pop %ebp
	pop	%edi
	pop	%esi
	pop %edx
	pop %ecx
	pop %ebx
	leave
	ret

#========================================

fscanfSubCore:
	mov	(%edx),%edi
	add	$2,%edi
	add	$4,%edx
	mov	%edi,%esi
fscanfSub1:
	sub	$1,%esi
	jl	fscanfSub2
	mov	(%edx),%ebx
	add	$4,%edx
	push	%ebx
	jmp	fscanfSub1
fscanfSub2:
	# all args have been moved from parameter stack to PC stack
	mov	%edx,SPtr(%ebp)
	xcall	fscanf
	mov	SPtr(%ebp),%edx
	sub	$4,%edx
	mov	%eax,(%edx)		# return result on parameter stack
	mov	%edx,SPtr(%ebp)
	# cleanup PC stack
	mov	%edi,%ebx
	sal	$2,%ebx
	add	%ebx,%esp
	ret

# extern long fscanfSub( ForthCoreState* pCore );

#	fscanfSub	,PROC	near	C	public	uses	%ebx	ecx	%edx	%esi	edi	%ebp
	subEntry	fscanfSub
	push %ebp
	mov	%esp,%ebp
	push %ebx
	push %ecx
	push %edx
	push %esi
	push %edi
	push %ebp
	
	mov	8(%ebp),%ebp		# ebp -> ForthCore
	mov	SPtr(%ebp),%edx
	call	fscanfSubCore

	pop %ebp
	pop	%edi
	pop	%esi
	pop %edx
	pop %ecx
	pop %ebx
	leave
	ret
	
#========================================

sscanfSubCore:
	mov	(%edx),%edi
	add	$2,%edi
	add	$4,%edx
	mov	%edi,%esi
sscanfSub1:
	sub	$1,%esi
	jl	sscanfSub2
	mov	(%edx),%ebx
	add	$4,%edx
	push	%ebx
	jmp	sscanfSub1
sscanfSub2:
	# all args have been moved from parameter stack to PC stack
	mov	%edx,SPtr(%ebp)
	xcall	sscanf
	mov	SPtr(%ebp),%edx
	sub	$4,%edx
	mov	%eax,(%edx)		# return result on parameter stack
	mov	%edx,SPtr(%ebp)
	# cleanup PC stack
	mov	%edi,%ebx
	sal	$2,%ebx
	add	%ebx,%esp
	ret

# extern long sscanfSub( ForthCoreState* pCore );

#	sscanfSub	,PROC	near	C	public	uses	%ebx	ecx	%edx	%esi	edi	%ebp
	subEntry	sscanfSub
	push %ebp
	mov	%esp,%ebp
	push %ebx
	push %ecx
	push %edx
	push %esi
	push %edi
	push %ebp
	
	mov	8(%ebp),%ebp		# ebp -> ForthCore
	mov	SPtr(%ebp),%edx
	call	sscanfSubCore
	
	pop %ebp
	pop	%edi
	pop	%esi
	pop %edx
	pop %ecx
	pop %ebx
	leave
	ret

#========================================
	optypeEntry	dllEntryPointType
	mov	%esi,IPtr(%ebp)
	mov	%edx,SPtr(%ebp)
	mov	%ebx,%eax
	and	$0x0000FFFF,%eax
	cmp	numOps(%ebp),%eax
	jge	badUserDef
	# push core ptr
	push	%ebp
	# push flags
	mov	%ebx,%esi
	shr	$15,%esi
	and	$7,%esi
	push	%esi
	# push arg count
	mov	%ebx,%esi
	and	$0x00F80000,%esi
	sar	$19,%esi
	and	$0x1F,%esi
	push	%esi
	# push entry point address
	mov	ops(%ebp),%esi
	mov	(%esi,%eax,4),%edx
	push	%edx
	xcall	CallDLLRoutine
	add	$12,%esp
	pop	%ebp
	jmp	interpFunc


#-----------------------------------------------
#
# NUM VAROP OP combo ops
#  
	optypeEntry	nvoComboType
	# ebx: bits 0..10 are signed integer, bits 11..12 are varop-2, bit 13..23 are opcode
	mov	%ebx,%eax
	sub	$4,%edx
	and	$0x00000400,%eax
	jnz	nvoNegative
	# positive constant
	mov	%ebx,%eax
	and	$0x003FF,%eax
	jmp	nvoCombo1

nvoNegative:
	mov	%ebx,%eax
	or	$0x0FFFFF800,%ebx			# sign extend bits 11-31
nvoCombo1:
	mov	%eax,(%edx)
	# set the varop from bits 11-12
	shr	$11,%ebx
	mov	%ebx,%eax
	
	and	$3,%eax							# eax = varop - 2
	add	$2,%eax
	mov	%eax,varMode(%ebp)

	# extract the opcode
	shr	$2,%ebx
	and	$0x0000007FF,%ebx			# ebx is 11 bit opcode
	jmp	interpLoopExecuteEntry

#-----------------------------------------------
#
# NUM VAROP combo ops
#  
	optypeEntry	nvComboType
	# bits 0..21 are signed integer, bits 22-23 are varop-2
	# ebx holds low-24 bits of opcode
	mov	%ebx,%eax
	sub	$4,%edx
	and	$0x00020000,%eax
	jnz	nvNegative
	# positive constant
	mov	%ebx,%eax
	and	$0x001FFFFF,%eax
	jmp	nvCombo1

nvNegative:
	mov	%ebx,%eax
	or	$0x0FFE00000,%ebx			# sign extend bits 22-31
nvCombo1:
	mov	%eax,(%edx)
	# set the varop from bits 22-23
	shr	$22,%ebx							# ebx = varop - 2
	mov	$0x000C00000,%eax
	and	$3,%ebx
	add	$2,%ebx
	mov	%ebx,varMode(%ebp)

	jmp	*%edi

#-----------------------------------------------
#
# NUM OP combo ops
#  
	optypeEntry	noComboType
	# ebx: bits 0..12 are signed integer, bits 13..23 are opcode
	mov	%ebx,%eax
	sub	$4,%edx
	and	$0x000001000,%eax
	jnz	noNegative
	# positive constant
	mov	%ebx,%eax
	and	$0x00FFF,%eax
	jmp	noCombo1

noNegative:
	or	$0xFFFFE000,%eax			# sign extend bits 13-31
noCombo1:
	mov	%eax,(%edx)
	# extract the opcode
	mov	%ebx,%eax
	shl	$13,%ebx
	and	$0x0000007FF,%ebx			# ebx is 11 bit opcode
	# opcode is in ebx
	jmp	interpLoopExecuteEntry

#-----------------------------------------------
#
# VAROP OP combo ops
#  
	optypeEntry	voComboType
	# bits 0-1 are varop-2, bits 2-23 are opcode
	# set the varop from bits 0-1
	mov	$0x000000003,%eax
	and	%ebx,%eax
	add	$2,%eax
	mov	%eax,varMode(%ebp)

	# extract the opcode
	shr	$2,%ebx
	and	$0x0003FFFFF,%ebx			# ebx is 22 bit opcode

	# opcode is in ebx
	jmp	interpLoopExecuteEntry

#-----------------------------------------------
#
# OP ZBRANCH combo ops
#  
	optypeEntry	ozbComboType
	# ebx: bits 0..11 are opcode, bits 12-23 are signed integer branch offset in longs
	mov	%ebx,%eax
	shr	$12,%eax
	and	$0x00000FFF,%eax
	push	%eax
	push	%edi
	mov	$ozbCombo1,%edi
	and	$0x00000FFF,%ebx
	# opcode is in ebx
	jmp	interpLoopExecuteEntry
	
ozbCombo1:
	pop	%edi
	pop	%eax
	mov	(%edx),%ebx
	add	$4,%edx
	or	%ebx,%ebx
	jnz	ozbCombo2			# if TOS not 0, don't branch
	mov	%eax,%ebx
	and	$0x00000800,%eax
	jnz	ozbNegative
	# positive branch
	or	$0xFFFFC000,%ebx

ozbNegative:
	add	%ebx,%esi
ozbCombo2:
	jmp	*%edi
	
#-----------------------------------------------
#
# OP BRANCH combo ops
#  
	optypeEntry obComboType
	# ebx: bits 0..11 are opcode, bits 12-23 are signed integer branch offset in longs
	mov	%ebx,%eax
	shr	$12,%eax
	and	$0xFFF,%eax
	push	%eax
	push	%edi
	mov	$obCombo1,%edi
	and	$0x0FFF,%ebx
	# opcode is in ebx
	jmp	interpLoopExecuteEntry
	
obCombo1:
	pop	%edi
	pop	%eax
	mov	%eax,%ebx
	and	$0x0800,%eax
	jnz	obNegative
	# positive branch
	or	$0xFFFFC000,%ebx

obNegative:
	add	%ebx,%esi
	jmp	*%edi
	

#-----------------------------------------------
#
# squished float literal
#  
	optypeEntry squishedFloatType
	# ebx: bit 23 is sign, bits 22..18 are exponent, bits 17..0 are mantissa
	# to unsquish a float:
	#   sign = (inVal & 0x800000) << 8
	#   exponent = (((inVal >> 18) & 0x1f) + (127 - 15)) << 23
	#   mantissa = (inVal & 0x3ffff) << 5
	#   outVal = sign | exponent | mantissa
	push	%esi
	mov	%ebx,%eax
	and	$0x00800000,%eax
	shl	$8,%eax			# sign bit
	
	mov	%ebx,%esi
	shr	$18,%ebx
	and	$0x1F,%ebx
	add	$112,%ebx
	shl	$23,%ebx			# ebx is exponent
	or	%ebx,%eax
	
	and	$0x03FFFF,%esi
	shl	$5,%esi
	or	%esi,%eax
	
	sub	$4,%edx
	mov	%eax,(%edx)
	pop	%esi
	jmp	*%edi
	

#-----------------------------------------------
#
# squished double literal
#  
	optypeEntry squishedDoubleType
	# ebx: bit 23 is sign, bits 22..18 are exponent, bits 17..0 are mantissa
	# to unsquish a double:
	#   sign = (inVal & 0x800000) << 8
	#   exponent = (((inVal >> 18) & 0x1f) + (1023 - 15)) << 20
	#   mantissa = (inVal & 0x3ffff) << 2
	#   outVal = (sign | exponent | mantissa) << 32
	push	%esi
	mov	%ebx,%eax
	and	$0x00800000,%eax
	shl	$8,%eax			# sign bit
	
	mov	%ebx,%esi
	shr	$18,%ebx
	and	$0x1F,%ebx
	add	$0x1008,%ebx
	shl	$0x20,%ebx			# ebx is exponent
	or	%ebx,%eax
	
	and	$0x03FFFF,%esi
	shl	$2,%esi
	or	%esi,%eax
	
	sub	$4,%edx
	mov	%eax,(%edx)
	sub	$4,%edx
	# loword of double is all zeros
	xor	%eax,%eax
	mov	%eax,(%edx)
	pop	%esi
	jmp	*%edi
	

#-----------------------------------------------
#
# squished long literal
#  
	optypeEntry squishedLongType
	# get low-24 bits of opcode
	mov	%ebx,%eax
	sub	$8,%edx
	and	$0x00800000,%eax
	jnz	longConstantNegative
	# positive constant
	and	$0x00FFFFFF,%ebx
	mov	%ebx,(%edx)
	xor	%ebx,%ebx
	mov	%ebx,4(%edx)
	jmp	*%edi

longConstantNegative:
	or	$0xFF000000,%ebx
	mov	%ebx,(%edx)
	xor	%ebx,%ebx
	sub	$1,%ebx
	mov	%ebx,4(%edx)
	jmp	*%edi
	

#-----------------------------------------------
#
# LOCALREF OP combo ops
#
	optypeEntry lroComboType
	# ebx: bits 0..11 are frame offset in longs, bits 12-23 are op
	push	%ebx
	and	$0x0FFF,%ebx
	sal	$2,%ebx
	mov	FPtr(%ebp),%eax
	sub	%ebx,%eax
	sub	$4,%edx
	mov	%eax,(%edx)
	
	pop	%ebx
	shr	$12,%ebx
	and	$0x0FFF,%ebx			# ebx is 12 bit opcode
	# opcode is in ebx
	jmp	interpLoopExecuteEntry
	
#-----------------------------------------------
#
# MEMBERREF OP combo ops
#
	optypeEntry mroComboType
	# ebx: bits 0..11 are member offset in bytes, bits 12-23 are op
	push	%ebx
	and	$0x0FFF,%ebx
	mov	TDPtr(%ebp),%eax
	add	%ebx,%eax
	sub	$4,%edx
	mov	%eax,(%edx)
	
	pop	%ebx
	shr	$12,%ebx
	and	$0x0FFF,%ebx			# ebx is 12 bit opcode
	# opcode is in ebx
	jmp	interpLoopExecuteEntry


#=================================================================================================
#
#                                    opType table
#  
#=================================================================================================
	.align  4
	opEntry	opTypesTable
opTypesTable1:
# TBD: check the order of these
# TBD: copy these into base of ForthCoreState, fill unused slots with badOptype
#	00 - 09
	optypeRef	externalBuiltin			# kOpNative = 0,
	optypeRef	nativeImmediate			# kOpNativeImmediate,
	optypeRef	userDefType				# kOpUserDef,
	optypeRef	userDefType				# kOpUserDefImmediate,
	optypeRef	cCodeType				# kOpCCode,         
	optypeRef	cCodeType				# kOpCCodeImmediate,
	optypeRef	relativeDefType			# kOpRelativeDef,
	optypeRef	relativeDefType			# kOpRelativeDefImmediate,
	optypeRef	dllEntryPointType		# kOpDLLEntryPoint,
	.long	extOpType
#	10 - 19
	optypeRef	branchType				# kOpBranch = 10,
	optypeRef	branchNZType			# kOpBranchNZ,
	optypeRef	branchZType				# kOpBranchZ,
	optypeRef	caseBranchType			# kOpCaseBranch,
	optypeRef	pushBranchType			# kOpPushBranch,
	.long	extOpType
	.long	extOpType
	.long	extOpType
	.long	extOpType
	.long	extOpType
#	20 - 29
	optypeRef	constantType			# kOpConstant = 20,   
	optypeRef	constantStringType		# kOpConstantString,	
	optypeRef	offsetType				# kOpOffset,          
	optypeRef	arrayOffsetType			# kOpArrayOffset,     
	optypeRef	allocLocalsType			# kOpAllocLocals,     
	optypeRef	localRefType			# kOpLocalRef,
	optypeRef	initLocalStringType		# kOpInitLocalString, 
	optypeRef	localStructArrayType	# kOpLocalStructArray,
	optypeRef	offsetFetchType			# kOpOffsetFetch,          
	optypeRef	memberRefType			# kOpMemberRef,	

#	30 - 39
	optypeRef	localByteType			# 30 - 42 : local variables
	optypeRef	localUByteType
	optypeRef	localShortType
	optypeRef	localUShortType
	optypeRef	localIntType
	optypeRef	localIntType
	optypeRef	localLongType
	optypeRef	localLongType
	optypeRef	localFloatType
	optypeRef	localDoubleType

#	40 - 49
	optypeRef	localStringType
	optypeRef	localOpType
	optypeRef	localObjectType
	optypeRef	localByteArrayType		# 43 - 55 : local arrays
	optypeRef	localUByteArrayType
	optypeRef	localShortArrayType
	optypeRef	localUShortArrayType
	optypeRef	localIntArrayType
	optypeRef	localIntArrayType
	optypeRef	localLongArrayType

#	50 - 59
	optypeRef	localLongArrayType
	optypeRef	localFloatArrayType
	optypeRef	localDoubleArrayType
	optypeRef	localStringArrayType
	optypeRef	localOpArrayType
	optypeRef	localObjectArrayType
	optypeRef	fieldByteType			# 56 - 68 : field variables
	optypeRef	fieldUByteType
	optypeRef	fieldShortType
	optypeRef	fieldUShortType

#	60 - 69
	optypeRef	fieldIntType
	optypeRef	fieldIntType
	optypeRef	fieldLongType
	optypeRef	fieldLongType
	optypeRef	fieldFloatType
	optypeRef	fieldDoubleType
	optypeRef	fieldStringType
	optypeRef	fieldOpType
	optypeRef	fieldObjectType
	optypeRef	fieldByteArrayType		# 69 - 81 : field arrays

#	70 - 79
	optypeRef	fieldUByteArrayType
	optypeRef	fieldShortArrayType
	optypeRef	fieldUShortArrayType
	optypeRef	fieldIntArrayType
	optypeRef	fieldIntArrayType
	optypeRef	fieldLongArrayType
	optypeRef	fieldLongArrayType
	optypeRef	fieldFloatArrayType
	optypeRef	fieldDoubleArrayType
	optypeRef	fieldStringArrayType

#	80 - 89
	optypeRef	fieldOpArrayType
	optypeRef	fieldObjectArrayType
	optypeRef	memberByteType			# 82 - 94 : member variables
	optypeRef	memberUByteType
	optypeRef	memberShortType
	optypeRef	memberUShortType
	optypeRef	memberIntType
	optypeRef	memberIntType
	optypeRef	memberLongType
	optypeRef	memberLongType

#	90 - 99
	optypeRef	memberFloatType
	optypeRef	memberDoubleType
	optypeRef	memberStringType
	optypeRef	memberOpType
	optypeRef	memberObjectType
	optypeRef	memberByteArrayType	# 95 - 107 : member arrays
	optypeRef	memberUByteArrayType
	optypeRef	memberShortArrayType
	optypeRef	memberUShortArrayType
	optypeRef	memberIntArrayType

#	100 - 109
	optypeRef	memberIntArrayType
	optypeRef	memberLongArrayType
	optypeRef	memberLongArrayType
	optypeRef	memberFloatArrayType
	optypeRef	memberDoubleArrayType
	optypeRef	memberStringArrayType
	optypeRef	memberOpArrayType
	optypeRef	memberObjectArrayType
	optypeRef	methodWithThisType
	optypeRef	methodWithTOSType

#	110 - 119
	optypeRef	memberStringInitType
	optypeRef	nvoComboType
	optypeRef	nvComboType
	optypeRef	noComboType
	optypeRef	voComboType
	optypeRef	ozbComboType
	optypeRef	obComboType

	optypeRef	squishedFloatType
	optypeRef	squishedDoubleType
	optypeRef	squishedLongType
	
#	120 - 121
	optypeRef	lroComboType
	optypeRef	mroComboType
	
#	122 - 149
	.long	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType
	.long	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType
	.long	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType
#	150 - 199
	.long	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType
	.long	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType
	.long	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType
	.long	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType
	.long	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType
#	200 - 249
	.long	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType
	.long	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType
	.long	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType
	.long	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType
	.long	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType
#	250 - 255
	.long	extOpType,	extOpType,	extOpType,	extOpType,	extOpType,	extOpType

endOpTypesTable:
	.long 0
	.endif	# end .ifdef ASM_INNER_ASSEMBLER

	.end

