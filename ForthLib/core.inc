
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
kVarFetch			EQU		1
kVarRef				EQU		2
kVarStore			EQU		3
kVarPlusStore		EQU		4
kVarMinusStore		EQU		5
kVarObjectClear		EQU		6

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
    .optypeAction		RESD 1
    .numBuiltinOps		RESD 1
    .ops				RESD 1
    .numOps				RESD 1
    .maxOps				RESD 1
    .pEngine			RESD 1
    .IPtr				RESD 1
    .SPtr				RESD 1
    .RPtr				RESD 1
    .FPtr				RESD 1
    .TPtr				RESD 1
    .varMode			RESD 1
    .state				RESD 1
    .error				RESD 1
    .SBPtr				RESD 1
    .STPtr				RESD 1
    .SLen				RESD 1
    .RBPtr				RESD 1
    .RTPtr				RESD 1
    .RLen				RESD 1
    .ThreadPtr			RESD 1
    .DictionaryPtr		RESD 1
    .FileFuncs			RESD 1
    .innerLoop			RESD 1
    .innerExecute		RESD 1
    .consoleOutStream	RESD 1
    .base				RESD 1
    .signedPrintMode	RESD 1
    .traceFlags			RESD 1
	.ExceptionPtr		RESD 1
    .scratch            RESD 4
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
