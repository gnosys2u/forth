// forth internal definitions
autoforget forth_internals

features
kFFRegular -> features

: forth_internals ;

enum: eBaseType
  kBTByte				// 0
  kBTUByte
  kBTShort
  kBTUShort
  kBTInt				// 4
  kBTUInt	
  kBTLong
  kBTULong
  kBTFloat				// 8
  kBTDouble
  kBTString
  kBTOp
  kBTObject				// 12
  kBTStruct
  kBTUserDef
  kBTVoid
;enum

enum: eShellTag
  kSTNothing	// 0
  kSTDo
  kSTBegin
  kSTWhile
  kSTCase		// 4
  kSTIf
  kSTElse
  kSTParen
  kSTString	// 8
  kSTColon
  kSTPoundIf
  kSTOf
  kSTOfIf		// 12
;enum
 
// these are the results of running the inner interpreter
enum: eInterpreterResult
  kIROk          // no need to exit
  kIRDone        // exit because of "done" opcode
  kIRExitShell   // exit because of a "bye" opcode
  kIRError       // exit because of error
  kIRFatalError  // exit because of fatal error
  kIRException   // exit because of uncaught exception
  kIRShutdown    // exit because of a "shutdown" opcode
  kIRYield       // exit because of a stopThread/yield/sleepThread opcode
;enum

enum: eForthError
  kFENone
  kFEBadOpcode
  kFEBadOpcodeType
  kFEBadParameter
  kFEBadVarOperation
  kFEParamStackUnderflow
  kFEParamStackOverflow
  kFEReturnStackUnderflow
  kFEReturnStackOverflow
  kFEUnknownSymbol
  kFEFileOpen
  kFEAbort
  kFEForgetBuiltin
  kFEBadMethod
  kFEException
  kFEMissingSize
  kFEStruct
  kFEUserDefined
  kFEBadSyntax
  kFEBadPreprocessorDirective
  kFENumErrors
;enum

// how sign should be handled while printing integers
enum: eSignedPrintMode
  kSPMSignedDecimal				// only decimal numbers are signed
  kSPMAllSigned					// all numbers are signed
  kSPMAllUnsigned				// all numbers are unsigned
;enum

enum: eVarOp
  kVODefault
  kVOFetch
  kVORef
  kVOStore
  kVOPlusStore
  kVOMinusStore
;enum

struct: ForthCoreState
  ptrTo int     optypeAction           // optypeActionRoutine*

  int           numBuiltinOps

  ptrTo int     ops	                // **long
  int           numOps
  int           maxOps             	// current size of table at pUserOps

  ptrTo int	pEngine

  ptrTo int     IP                     // interpreter pointer

  ptrTo int     SP                     // parameter stack pointer
    
  ptrTo int     RP                     // return stack pointer

  ptrTo int     FP                     // frame pointer
    
  ptrTo int     TPM                    // this pointer (methods)
  ptrTo int     TPD                    // this pointer (data)

  int           varMode                // operation to perform on variables

  int           state                  // inner loop state - ok/done/error

  int           error

  ptrTo int     SB                     // param stack base
  ptrTo int     ST                     // empty parameter stack pointer

  int           SLen                   // size of param stack in longwords

  ptrTo int     RB                     // return stack base
  ptrTo int     RT                     // empty return stack pointer

  int           RLen                   // size of return stack in longwords
  
  ptrTo int     pThread                // ForthThread*

  ptrTo int     pDictionary            // ForthMemorySection*
  ptrTo int     pFileFuncs             // ForthFileInterface*

  ptrTo int	innerLoop              // inner interpreter re-entry point for assembler
  
  ptrTo byte    pConOutData            // void*
  int           consoleOut             // consoleOutRoutine
  int		consoleOutOp

  ptrTo int     pDefaultOutFile        // FILE*
  ptrTo int     pDefaultInFile         // FILE*
  int           base                   // output base
  int           signedPrintMode        // if numers are printed as signed/unsigned

  int           traceFlags             // trace flag bits

  ptrTo int     pExceptionFrame        // points to current exception handler frame in rstack

;struct

-> features

loaddone

: testy
  5 3+ 7 * %d
;

new oThread -> oThread threadA
threadA.init( 64 64 ' testy )

: showThreadState   // pThread
  <oThread>.getState -> ptrTo ForthCoreState pState
  "IP " %s pState.IP %x
  "  SP " %s pState.SP %x
  "  RP " %s pState.RP %x %nl
;


: ss
  threadA.step "Result : " %s %x "   " %s
  threadA showThreadState
;
