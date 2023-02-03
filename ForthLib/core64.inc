
;
; inner interpreter registers
;
%define rip rsi
%define rnext rdi
%define roptab r9
%define rnumops r10
%define racttab r11
%define rcore r12
%define rfp r13
%define rpsp r14
%define rrp r15

;-----------------------------------------------
;
; the entry macro declares a label and makes it public
;
%macro entry 1
%ifdef ASM_UNDERSCORE_PREFIX
GLOBAL _%1
%else
GLOBAL %1
%endif
_%1:
%1:
%endmacro
	
%macro cextern 1
%ifdef ASM_UNDERSCORE_PREFIX
EXTERN _%1
%else
EXTERN %1
%endif
%endmacro

%macro xcall 1
%ifdef ASM_UNDERSCORE_PREFIX
EXTERN _%1
    call _%1
%else
EXTERN %1
    call %1
%endif
%endmacro

kVarDefaultOp		EQU		0
kVarGet		    	EQU		1
kVarRef				EQU		2
kVarSet		    	EQU		3
kVarSetPlus		    EQU		4
kVarSetMinus	    EQU		5
kVarClear		    EQU		6
kVarPlus            EQU     7
kVarInc             EQU     8
kVarMinus           EQU     9
kVarDec             EQU     10
kVarIncGet          EQU     11
kVarDecGet          EQU     12
kVarGetInc          EQU     13
kVarGetDec          EQU     14

; TODO: maybe have kPtr varops start at 0, not 16, since they will be encoded in ops that way
kPtrAtGet           EQU     16
kPtrAtSet           EQU     17
kPtrAtSetPlus       EQU     18
kPtrAtSetMinus      EQU     19
kPtrAtGetInc        EQU     20
kPtrAtGetDec        EQU     21
kPtrAtSetInc        EQU     22
kPtrAtSetDec        EQU     23
kPtrIncAtGet        EQU     24
kPtrDecAtGet        EQU     25
kPtrIncAtSet        EQU     26
kPtrDecAtSet        EQU     27

kResultOk				EQU		0
kResultDone				EQU		1
kResultExitShell		EQU		2
kResultError			EQU		3
kResultFatalError		EQU		4
kResultException		EQU		5
kResultShutdown			EQU		6
kResultYield			EQU		7


kForthErrorNone						EQU		0
kForthErrorBadOpcode				EQU		1
kForthErrorBadOpcodeType			EQU		2
kForthErrorBadParameter				EQU		3
kForthErrorBadVarOperation			EQU		4
kForthErrorParamStackUnderflow		EQU		5
kForthErrorParamStackOverflow		EQU		6
kForthErrorReturnStackUnderflow		EQU		7
kForthErrorReturnStackOverflow		EQU		8
kForthErrorUnknownSymbol			EQU		9
kForthErrorFileOpen					EQU		10
kForthErrorAbort					EQU		11
kForthErrorForgetBuiltin			EQU		12
kForthErrorBadMethod				EQU		13
kForthErrorException				EQU		14
kForthErrorMissingSize				EQU		15
kForthErrorStruct					EQU		16
kForthErrorUserDefined				EQU		17
kForthErrorBadSyntax				EQU		18
kForthErrorBadPreprocessorDirective	EQU		19
kForthErrorUnimplementedMethod		EQU		20
kForthErrorIllegalMethod			EQU		21
kForthErrorShellStackUnderflow		EQU		22
kForthErrorShellStackOverflow		EQU		23
kForthErrorBadReferenceCount		EQU		24
kForthErrorIO						EQU		25
kForthErrorBadObject				EQU		26
kForthErrorStringOverflow			EQU		27
kForthErrorBadArrayIndex			EQU		28
kForthErrorIllegalOperation			EQU		29
kForthErrorOSException				EQU		30
kForthNumErrors						EQU		31

kPrintSignedDecimal		EQU		0
kPrintAllSigned		EQU		1
kPrintAllUnsigned		EQU		2

kLogStack					EQU 1
kLogOuterInterpreter		EQU 2
kLogInnerInterpreter		EQU 4
kLogShell					EQU 8
kLogStructs					EQU 16
kLogVocabulary				EQU 32
kLogIO						EQU 64
kLogEngine					EQU 128
kLogToConsole				EQU 256
kLogCompilation				EQU 512
kLogProfiler                EQU 1024

; the kOp definitions are all out of date, bogus and luckily unused
;kOpNative		EQU		0
;kOpUserDef		EQU		1
;kOpBranch		EQU		2
;kOpBranchNZ		EQU		3
;kOpBranchZ		EQU		4
;kOpCaseBranch		EQU		5
;kOpConstant		EQU		6
;kOpOffset		EQU		7
;kOpArrayOffset		EQU		8
;kOpConstantString		EQU		9
;kOpAllocLocals		EQU		10
;kOpLocalStringInit		EQU		11
;kOpLocalInt		EQU		12
;kOpLocalFloat		EQU		13
;kOpLocalDouble		EQU		14
;kOpLocalString		EQU		15
;kOpFieldInt		EQU		16
;kOpFieldFloat		EQU		17
;kOpFieldDouble		EQU		18
;kOpFieldString		EQU		19
;kOpMethodWithThis		EQU		20
;kOpMemberInt		EQU		21
;kOpMemberFloat		EQU		22
;kOpMemberDouble		EQU		23
;kOpMemberString		EQU		24
;kOpLocalUserDefined		EQU		25
;kOpMaxLocalUserDefined		EQU		127
;kOpUserMethods		EQU		128

kOpRelativeDef			EQU 6

STRUC MemorySection
    .pCurrent: RESQ 1
    .pBase				RESQ 1
    .len					RESQ 1
ENDSTRUC 

; STRUC ForthCoreState
STRUC FCore
    .optypeAction		RESQ 1  ; 0
    .numBuiltinOps		RESQ 1  ; 8
    .ops				RESQ 1  ; 16
    .numOps				RESQ 1  ; 24
    .maxOps				RESQ 1  ; 32
    .pEngine			RESQ 1  ; 40
    .IPtr				RESQ 1  ; 48
    .SPtr				RESQ 1  ; 56
    .RPtr				RESQ 1  ; 64
    .FPtr				RESQ 1  ; 72
    .TPtr				RESQ 1  ; 80
    .varMode			RESQ 1  ; 88
    .state				RESQ 1  ; 96
    .error				RESQ 1  ; 104
    .SBPtr				RESQ 1  ; 112
    .STPtr				RESQ 1  ; 120
    .SLen				RESQ 1  ; 128
    .RBPtr				RESQ 1  ; 136
    .RTPtr				RESQ 1  ; 144
    .RLen				RESQ 1  ; 152
    .ThreadPtr			RESQ 1  ; 160
    .DictionaryPtr		RESQ 1  ; 168
    .FileFuncs			RESQ 1  ; 176
    .innerLoop			RESQ 1  ; 184
    .innerExecute		RESQ 1  ; 192
    .consoleOutStream	RESQ 1  ; 200
    .base				RESQ 1  ; 208
    .signedPrintMode	RESQ 1  ; 216
    .traceFlags			RESQ 1  ; 224
	.ExceptionPtr		RESQ 1  ; 232
    .scratch            RESQ 4  ; 240
ENDSTRUC 

STRUC Object
	.pMethods		RESQ 1
	.refCount		RESQ 1
ENDSTRUC 

;STRUC ForthFileInterface
STRUC FileFunc
    .fileOpen		RESQ 1
    .fileClose		RESQ 1
    .fileRead		RESQ 1
    .fileWrite		RESQ 1
    .fileGetChar		RESQ 1
    .filePutChar		RESQ 1
    .fileAtEnd		RESQ 1
    .fileExists		RESQ 1
    .fileSeek		RESQ 1
    .fileTell		RESQ 1
    .fileGetLength	RESQ 1
    .fileGetString	RESQ 1
    .filePutString	RESQ 1
ENDSTRUC 


; End of file core.h
