#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthParseInfo.h: interface for the ForthParseInfo class.
//
//////////////////////////////////////////////////////////////////////

class ForthEngine;

class ForthParseInfo
{
public:
	ForthParseInfo(int32_t *pBuffer, int numLongs);
	~ForthParseInfo();

	// SetToken copies symbol to token buffer (if pSrc not NULL), sets the length byte,
	//   sets mNumLongs and pads end of token buffer with nuls to next longword boundary
	// call with no argument or NULL if token has already been copied to mpToken
	void            SetToken(const char *pSrc = NULL);

	inline int      GetFlags(void) { return mFlags; };
	inline void     SetAllFlags(int flags) { mFlags = flags; };
	inline void     SetFlag(int flag) { mFlags |= flag; };

	inline char *   GetToken(void) { return ((char *)mpToken) + 1; };
    inline int32_t *   GetTokenAsLong(void) { return mpToken; };
    inline int      GetTokenLength(void) { return mNumChars; };
	inline int      GetNumLongs(void) { return mNumLongs; };
	inline int		GetMaxChars(void) const { return mMaxChars; };

	const char*		ParseSingleQuote(const char *pSrcIn, const char *pSrcLimit, ForthEngine *pEngine, bool keepBackslashes = false);
	void	        ParseDoubleQuote(const char *&pSrc, const char *pSrcLimit, bool keepBackslashes = false);
	
	static char		BackslashChar(const char*& pSrc);

private:
	int32_t *      mpToken;         // pointer to token buffer, first byte is strlen(token)
	int         mFlags;          // flags set by ForthShell::ParseToken for ForthEngine::ProcessToken
	int         mNumLongs;       // number of longwords for fast comparison algorithm
    int         mNumChars;
	int         mMaxChars;
};

