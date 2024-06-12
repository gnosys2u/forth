#pragma once
//////////////////////////////////////////////////////////////////////
//
// ParseInfo.h: interface for the ParseInfo class.
//
// Copyright (C) 2024 Patrick McElhatton
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
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

