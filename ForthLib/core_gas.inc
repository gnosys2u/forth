




kVarDefaultOp			=		0
kVarFetch				=		1
kVarRef					=		2
kVarStore				=		3
kVarPlusStore			=		4
kVarMinusStore			=		5

kVocabSetCurrent		=	0
kVocabNewestEntry		=	1
kVocabRef				=	2
kVocabFindEntry			=	3
kVocabFindEntryValue	=	4
kVocabAddEntry			=	5
kVocabRemoveEntry		=	6
kVocabEntryLength		=	7
kVocabNumEntries		=	8
kVocabGetClass			=	9

kResultOk				=		0
kResultDone				=		1
kResultExitShell		=		2
kResultError			=		3
kResultFatalError		=		4
kResultException		=		5
kResultShutdown			=		6
kResultYield			=		7

kForthErrorNone						=		0
kForthErrorBadOpcode				=		1
kForthErrorBadOpcodeType			=		2
kForthErrorBadParameter				=		3
kForthErrorBadVarOperation			=		4
kForthErrorParamStackUnderflow		=		5
kForthErrorParamStackOverflow		=		6
kForthErrorReturnStackUnderflow		=		7
kForthErrorReturnStackOverflow		=		8
kForthErrorUnknownSymbol			=		9
kForthErrorFileOpen					=		10
kForthErrorAbort					=		11
kForthErrorForgetBuiltin			=		12
kForthErrorBadMethod				=		13
kForthErrorException				=		14
kForthErrorMissingSize				=		15
kForthErrorStruct					=		16
kForthErrorUserDefined				=		17
kForthErrorBadSyntax				=		18
kForthErrorBadPreprocessorDirective	=		19
kForthErrorUnimplementedMethod		=		20
kForthErrorShellStackUnderflow		=		21
kForthErrorShellStackOverflow		=		22
kForthErrorBadReferenceCount		=		23
kForthErrorIO						=		24
kForthErrorBadObject				=		25
kForthNumErrors						=		26

kPrintSignedDecimal		=		0
kPrintAllSigned			=		1
kPrintAllUnsigned		=		2

	

# ForthMemorySection
MemSectionCurrentPtr	=	0
MemSectionBasePtr		=	4
MemSectionLength		=	8

#	ForthCoreState	STRUCT	2t
#
#	core (EBP) offsets:
#
optypeAction		=	0   	 	#	table of C opType action routines
numBuiltinOps		=	4			#	number of builtin ops
ops					=	8			#	table of ops
numOps				=	12			#	number of ops
maxOps				=	16			#	current size of user defined op table
pEngine				=	20			#	ForthEngine pointer
IPtr				=	24			#	IP - interpreter pointer (r5)
SPtr				=	28			#	SP - forth stack pointer (r6)
RPtr				=	32			#	RP - forth return stack pointer
FPtr				=	36			#	FP - frame pointer
TMPtr				=	40			#	TPM - this pointer (methods)
TDPtr				=	44			#	TPD - this pointer (data)
varMode				=	48			#	varMode - fetch/store/plusStore/minusStore
state				=	52			#	state - ok/done/error
error				=	56			#	error code
SBPtr				=	60			#	empty parameter stack pointer
STPtr				=	64			#	empty parameter stack pointer
SLen				=	68			#	size of param stack in longwords
RBPtr				=	72			#	empty return stack pointer
RTPtr				=	76			#	empty return stack pointer
RLen				=	80			#	size of return stack in longwords
ThreadPtr			=	84			#	current ForthThread pointer
DictionaryPtr		=	88			#	dictionary pointer
FileFuncs			=	92			#	shell file functions
innerLoop			=	96			#	inner loop reentry point
consoleOutStream	=	100
base				=	108
signedPrintMode		=	112

#	ForthFileInterface
fileOpen			=	0
fileClose			=	4
fileRead			=	8
fileWrite			=	12
fileGetChar			=	16
filePutChar			=	20
fileAtEnd			=	24
fileExists			=	28
fileSeek			=	32
fileTell			=	36
fileGetLength		=	40
fileGetString		=	44
filePutString		=	48


