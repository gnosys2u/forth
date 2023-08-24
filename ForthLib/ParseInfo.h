#pragma once
//////////////////////////////////////////////////////////////////////
//
// ParseInfo.h: interface for the ParseInfo class.
//
//////////////////////////////////////////////////////////////////////

class Engine;

class ParseInfo
{
public:
	ParseInfo(int32_t *pBuffer, int numLongs);
	~ParseInfo();

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

	const char*		ParseSingleQuote(const char *pSrcIn, const char *pSrcLimit, Engine *pEngine, bool keepBackslashes = false);
	void	        ParseDoubleQuote(const char *&pSrc, const char *pSrcLimit, bool keepBackslashes = false);
	
	static int		BackslashChar(const char*& pSrc);

    void            ChopVaropSuffix();
    void            UnchopVaropSuffix();
    VarOperation    CheckVaropSuffix();

    void UpdateLength(size_t symLen);

    static const char* GetVaropSuffix(VarOperation varop);

private:
    
    int32_t*        mpToken;         // pointer to token buffer, first byte is strlen(token)
	int             mFlags;          // flags set by Shell::ParseToken for Engine::ProcessToken
	int             mNumLongs;       // number of longwords for fast comparison algorithm
    int             mNumChars;
	int				mMaxChars;
    char*           mpSuffix;       // null, unless CheckVaropSuffix found a valid suffix
    VarOperation	mSuffixVarop;   // initialized to numVarops to indicate suffix not checked yet
    char            mChoppedChar;   // set by ChopVaropSuffix
};

