// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the FORTHDLL_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// FORTHDLL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef FORTHDLL_EXPORTS
#define FORTHDLL_API __declspec(dllexport)
#else
#define FORTHDLL_API __declspec(dllimport)
#endif

#if 0
// This class is exported from the dll
class FORTHDLL_API CForthDLL {
public:
	CForthDLL(void);
	// TODO: add your methods here.
};

extern FORTHDLL_API int nForthDLL;

FORTHDLL_API int fnForthDLL(void);
#endif

#include "ForthEngine.h"

FORTHDLL_API Engine* CreateForthEngine();
FORTHDLL_API Shell* CreateForthShell(int argc, const char** argv, const char** envp,
    Engine* pEngine = NULL, Extension* pExtension = NULL, int shellStackLongs = 1024);

FORTHDLL_API BufferInputStream* CreateForthBufferInputStream(
    const char* pDataBuffer, int dataBufferLen, int bufferLen = DEFAULT_INPUT_BUFFER_LEN, bool deleteWhenEmpty = true);
FORTHDLL_API ConsoleInputStream* CreateForthConsoleInputStream(
    int bufferLen = DEFAULT_INPUT_BUFFER_LEN, bool deleteWhenEmpty = true);
FORTHDLL_API FileInputStream* CreateForthFileInputStream(
    FILE* pInFile, const char* pFilename, int bufferLen, bool deleteWhenEmpty = true);

FORTHDLL_API void DeleteForthEngine(Engine* pEngine);
FORTHDLL_API void DeleteForthShell(Shell* pShell);
FORTHDLL_API void DeleteForthInputStream(InputStream* pStream);

