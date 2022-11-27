
;-----------------------------------------------
;
; the entry macro declares a label and makes it public
;
%macro entry 1
%ifdef LINUX
GLOBAL %1
%else
GLOBAL _%1
%endif
_%1:
%1:
%endmacro
	
%macro cextern 1
%ifdef LINUX
EXTERN %1
%else
EXTERN _%1
%endif
%endmacro

%macro xcall 1
%ifdef LINUX
EXTERN %1
    call %1
%else
EXTERN _%1
    call _%1
%endif
%endmacro

kVarDefaultOp		EQU		0
kVarGet			    EQU		1
kVarRef				EQU		2
kVarSet			    EQU		3
kVarSetPlus		    EQU		4
kVarSetMinus	    EQU		5
kVarClear		    EQU		6

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

STRUC ForthMemorySection
    .pCurrent: RESD 1
    .pBase				RESD 1
    .len					RESD 1
ENDSTRUC 

; STRUC ForthCoreState
STRUC FCore
    .optypeAction		RESD 1  ; 0
    .numBuiltinOps		RESD 1  ; 4
    .ops				RESD 1  ; 8
    .numOps				RESD 1  ; 12
    .maxOps				RESD 1  ; 16
    .pEngine			RESD 1  ; 20
    .IPtr				RESD 1  ; 24
    .SPtr				RESD 1  ; 28
    .RPtr				RESD 1  ; 32
    .FPtr				RESD 1  ; 36
    .TPtr				RESD 1  ; 40
    .varMode			RESD 1  ; 44
    .state				RESD 1  ; 48
    .error				RESD 1  ; 52
    .SBPtr				RESD 1  ; 56
    .STPtr				RESD 1  ; 60
    .SLen				RESD 1  ; 64
    .RBPtr				RESD 1  ; 68
    .RTPtr				RESD 1  ; 72
    .RLen				RESD 1  ; 76
    .ThreadPtr			RESD 1  ; 80
    .DictionaryPtr		RESD 1  ; 84
    .FileFuncs			RESD 1  ; 88
    .innerLoop			RESD 1  ; 92
    .innerExecute		RESD 1  ; 96
    .consoleOutStream	RESD 1  ; 100
    .base				RESD 1  ; 104
    .signedPrintMode	RESD 1  ; 108
    .traceFlags			RESD 1  ; 112
	.ExceptionPtr		RESD 1  ; 116
    .scratch            RESD 4  ; 120
ENDSTRUC 

STRUC Object
	.pMethods		RESD 1
	.refCount		RESD 1
ENDSTRUC 

;STRUC ForthFileInterface
STRUC FileFunc
    .fileOpen		RESD 1
    .fileClose		RESD 1
    .fileRead		RESD 1
    .fileWrite		RESD 1
    .fileGetChar		RESD 1
    .filePutChar		RESD 1
    .fileAtEnd		RESD 1
    .fileExists		RESD 1
    .fileSeek		RESD 1
    .fileTell		RESD 1
    .fileGetLength	RESD 1
    .fileGetString	RESD 1
    .filePutString	RESD 1
ENDSTRUC 


; End of file core.h
