//////////////////////////////////////////////////////////////////////
//
// ParseInfo.cpp: implementation of the ParseInfo class.
//
// Copyright (C) 2024 Patrick McElhatton
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the �Software�), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED �AS IS�, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "Engine.h"
#include "OuterInterpreter.h"
#include "ParseInfo.h"
#include "Shell.h"
#include "InputStream.h"
#include "Vocabulary.h"
#include "Extension.h"

ParseInfo::ParseInfo(int32_t *pBuffer, int numLongs)
	: mpToken(pBuffer)
	, mMaxChars((numLongs << 2) - 2)
	, mFlags(0)
	, mNumLongs(0)
	, mNumChars(0)
    , mpSuffix(nullptr)
    , mSuffixVarop(VarOperation::numVarops)
{
	ASSERT(numLongs > 0);

	// zero length byte, and first char
	*mpToken = 0;
}


ParseInfo::~ParseInfo()
{
	// NOTE: don't delete mpToken, it doesn't belong to us
}


// copy string to mpToken buffer, set length, and pad with nulls to a longword boundary
// if pSrc is null, just set length and do padding
void ParseInfo::SetToken(const char* pSrc)
{
    size_t symLen, padChars;
    char* pDst;

    if (pSrc != NULL)
    {
        symLen = strlen(pSrc);
        pDst = ((char*)mpToken) + 1;

        if (symLen > mMaxChars)
        {
            symLen = mMaxChars;
        }
        // make copy of symbol
        memcpy(pDst, pSrc, symLen);
        *(pDst + symLen) = '\0';

    }
    else
    {
        // token has already been copied to mpToken, just set length byte
        symLen = strlen(((char*)mpToken) + 1);
    }

    UpdateLength(symLen);
}

void ParseInfo::UpdateLength(size_t symLen)
{
    // set length byte
    char* pDst = ((char*)mpToken) + symLen + 2;
#ifdef WIN32
    * ((char*)mpToken) = (char)(min(symLen, 255));
#else
    * ((char*)mpToken) = std::min(symLen, (size_t)255);
#endif
    mNumChars = (int)symLen;
    // in diagram, first char is length byte, 'a' are symbol chars, '0' is terminator, '#' is padding
	//
	//            symLen     padding nLongs
	// 1a0#|        1           1       1
	// 2aa0|        2           0       1
	// 3aaa|0       3           0       1
	// 4aaa|a0##    4           2       2
	// 5aaa|aa0#    5           1       2
	//
	mNumLongs = (int)((symLen + 4) >> 2);

    size_t padChars = (symLen + 2) & 3;
	if (padChars > 1)
	{
		padChars = 4 - padChars;
		while (padChars > 0)
		{
			*pDst++ = '\0';
			padChars--;
		}
	}
}

// return -1 for not a valid hexadecimal char
int hexValue(char c)
{
    int result = -1;
    
    c = tolower(c);
    if ((c >= '0') && (c <= '9'))
    {
        result = c - '0';
    }
    else if ((c >= 'a') && (c <= 'f'))
    {
        result = 10 + (c - 'a');
    }

    return result;
}

int ParseInfo::BackslashChar(const char*& pSrc)
{
    char c = *pSrc;
    int cResult = c;

    if (c != '\0')
    {
        pSrc++;
        switch (c)
        {

        case 'a':        cResult = '\a';        break;		// x07	bell
        case 'b':        cResult = '\b';        break;		// x08	backspace
        case 'e':        cResult = 0x1b;        break;		// x1b	escape
        case 'f':        cResult = '\f';        break;		// x0b	formfeed
        case 'l':        cResult = '\n';        break;		// x0a	linefeed (AKA newline)
        case 'm':        cResult = 0x0a0d;      break;		// 0x0d,x0a	carriage return + linefeed
        case 'n':        cResult = '\n';        break;		// x0a	newline
        case 'q':        cResult = '\"';        break;		// x22	double quote
        case 'r':        cResult = '\r';        break;		// x0d	carriage return
        case 's':        cResult = '\'';        break;		// x27	single quote
        case '_':        cResult = ' ';         break;		// x20	space (not ANSI, only needed in multi-character literals)
        case 't':        cResult = '\t';        break;		// x09	tab (horizontal)
        case 'v':        cResult = '\v';        break;		// x0b	vertical tab
        case 'z':        cResult = '\0';        break;		// x00	nul
        case '\\':       cResult = '\\';        break;		// x5c	backslash
        case '\"':       cResult = '\"';        break;		// x22	double quote

        case 'x':											// hex-value
        {
            int val = hexValue(*pSrc);
            if (val >= 0)
            {
                cResult = val << 4;
                val = hexValue(pSrc[1]);
                if (val >= 0)
                {
                    cResult += val;
                    pSrc += 2;
                }
            }
            break;
        }

        default:
            pSrc--;
            return '\\';
        }
    }

	return cResult;
}


const char * ParseInfo::ParseSingleQuote(const char *pSrcIn, const char *pSrcLimit, Engine *pEngine, bool keepBackslashes)
{
	char cc[9];
	bool isQuotedChar = false;
	bool isLongConstant = false;

    // pSrcIn[0] is a single quote char
	if (pSrcIn[1] != 0)      // there must be at least 1 more char on line
	{
		const char *pSrc = pSrcIn + 1;
		int iDst = 0;
		int maxChars = pEngine->GetOuterInterpreter()->CheckFeature(kFFMultiCharacterLiterals) ? 8 : 1;
		while ((iDst < maxChars) && (pSrc < pSrcLimit))
		{
			char ch = *pSrc++;
			if (ch == '\0')
			{
                break;
			}
            else if (ch == ' ' || ch == '\t')
            {
                // this is the tick operator
				break;
			}
            else if (ch == '\\')
			{
				if (keepBackslashes)
				{
					cc[iDst++] = ch;
				}

				if (*pSrc == '\0')
				{
					break;
				}
                int ich = BackslashChar(pSrc);
                while (ich > 0xFF)
                {
                    cc[iDst++] = ich & 0xFF;
                    ich >>= 8;
                }
                cc[iDst++] = ich;
			}
            else if (ch == '\'')
            {
                // we need to support 2 ugly special cases for ANSI 2012 compatablity:
                //  '''    - single quote character constant
                //  ''<anythingButSingleQuote> - also single quote character constant (gforth does this)
                // in a multi-character literal, single quote must be done with \'
                isQuotedChar = true;
                if (iDst == 0)
                {
                    cc[iDst++] = '\'';
                    if (*pSrc == '\'')
                    {
                        pSrc++;
                    }
                }
                cc[iDst++] = '\0';

				if (pSrc < pSrcLimit)
				{
					ch = *pSrc;
					if ((ch == 'l') || (ch == 'L'))
					{
                        // trailing L after closing quote forces this to be treated as a long value
						isLongConstant = true;
						ch = *++pSrc;
					}

					if (pSrc < pSrcLimit)
					{
						// if there are still more chars after closing quote, next char must be a delimiter
						if ((ch != '\0') && (ch != ' ') && (ch != '\t') && (ch != ')'))
						{
							isQuotedChar = false;
							isLongConstant = false;
						}
					}
				}
				break;
			}
			else
			{
				cc[iDst++] = ch;
			}
		}

		if (iDst == maxChars)
		{
			if (*pSrc == '\'')
			{
				pSrc++;
				cc[iDst++] = '\0';
				isQuotedChar = true;
			}
		}

        if (iDst == 1)
        {
            // make tokens with no trailing single quote like "'a" work
            cc[iDst] = '\0';
            isQuotedChar = true;
        }
        
        if (isQuotedChar)
		{
			SetFlag(PARSE_FLAG_QUOTED_CHARACTER);
			if (isLongConstant)
			{
				SetFlag(PARSE_FLAG_FORCE_LONG);
			}
			SetToken(cc);
			pSrcIn = pSrc;
		}
	}
    else if (pSrcIn[1] == '\'')
    {
        // handle the {''} case - return one single quote (is this a gforth only thing?)
        SetFlag(PARSE_FLAG_QUOTED_CHARACTER);
        SetToken("\'");
        pSrcIn += 2;
    }
	return pSrcIn;
}


void ParseInfo::ParseDoubleQuote(const char *&pSrc, const char *pSrcLimit, bool keepBackslashes)
{
	char  *pDst = GetToken();

	SetFlag(PARSE_FLAG_QUOTED_STRING);

	pSrc++;  // skip first double-quote
	while ((*pSrc != '\0') && (pSrc < pSrcLimit))
	{
        char ch = '\0';

		switch (*pSrc)
		{

		case '"':
			*pDst = 0;
			// set token length byte
			SetToken();
            pSrc++;
			return;

		case '\\':
			if (keepBackslashes)
			{
                *pDst++ = *pSrc++;
                *pDst++ = *pSrc++;
            }
            else
            {
                pSrc++;
                int ich = BackslashChar(pSrc);
                while (ich > 0xFF)
                {
                    *pDst++ = ich & 0xFF;
                    ich >>= 8;
                }
                *pDst++ = ich;
            }
			break;

		default:
			ch = *pSrc++;
            *pDst++ = ch;
            break;
		}

	}
	*pDst = 0;
	SetToken();
}

VarOperation ParseInfo::CheckVaropSuffix()
{
    VarOperation varop = VarOperation::varDefaultOp;

    mpSuffix = nullptr;
    char* pToken = ((char*)mpToken) + 1;
    char* pLastChar = pToken + (mNumChars - 1);

    switch (*pLastChar)
    {
    case '&':
        varop = VarOperation::varRef;              // v& - push addr of var on TOS
        break;

    case '~':
        if (pLastChar[-1] == '@')
        {
            pLastChar--;
            varop = VarOperation::objUnref;        // v@~ - push obj on TOS, clear var, decrement obj refcount but don't delete if 0
        }
        else
        {
            varop = VarOperation::varClear;        // v~ - clear var and decrement obj refcount, delete if 0
        }
        break;

    case '@':
        switch (pLastChar[-1])
        {

        case '@':
            pLastChar--;
            switch (pLastChar[-1])
            {
            case '-':
                if (pLastChar[-2] == '-')
                {
                    pLastChar -= 2;
                    varop = VarOperation::ptrDecAtGet;     // p--@@
                }
                break;

            case '+':
                if (pLastChar[-2] == '+')
                {
                    pLastChar -= 2;
                    varop = VarOperation::ptrIncAtGet;     // p++@@
                }
                break;

            default:
                varop = VarOperation::ptrAtGet;            // p@@
                break;
            }
            break;

        case '-':
            if (pLastChar[-1] == '-')
            {
                pLastChar -= 2;
                varop = VarOperation::varDecGet;     // v--@
            }
            break;

        case '+':
            if (pLastChar[-1] == '+')
            {
                pLastChar -= 2;
                varop = VarOperation::varIncGet;     // v++@
            }
            break;

        default:
            varop = VarOperation::varGet;            // v@
            break;
        }
        break;

    case '!':
        switch (pLastChar[-1])
        {

        case '@':
            pLastChar--;
            switch (pLastChar[-1])
            {
            case '-':
                if (pLastChar[-2] == '-')
                {
                    pLastChar -= 2;
                    varop = VarOperation::ptrDecAtSet;     // p--@!
                }
                break;

            case '+':
                if (pLastChar[-2] == '+')
                {
                    pLastChar -= 2;
                    varop = VarOperation::ptrIncAtSet;     // p++@!
                }
                break;

            default:
                varop = VarOperation::ptrAtSet;            // p@!
                break;
            }
            break;

        default:
            varop = VarOperation::varSet;            // v!
            break;
        }
        break;

    case '-':
        switch (pLastChar[-1])
        {

        case '-':
            pLastChar--;
            switch (pLastChar[-1])
            {
            case '@':
                pLastChar--;
                if (pLastChar[-1] == '@')
                {
                    pLastChar--;
                    varop = VarOperation::ptrAtGetDec;     // p@@--
                }
                else
                {
                    varop = VarOperation::varGetDec;     // v@--
                }
                break;

            case '!':
                if (pLastChar[-2] == '@')
                {
                    pLastChar -= 2;
                    varop = VarOperation::ptrAtSetDec;     // p@!--
                }
                break;

            default:
                varop = VarOperation::varDec;            // v--
                break;
            }
            break;

        case '!':
            pLastChar--;
            if (pLastChar[-1] == '@')
            {
                pLastChar--;
                varop = VarOperation::ptrAtSetMinus;            // p@!-
            }
            else
            {
                varop = VarOperation::varSetMinus;             // v!-
            }
            break;

        case'@':
            pLastChar--;
            varop = VarOperation::varMinus;                    // v@-
            break;

        default:
            break;
        }
        break;

    case '+':
        switch (pLastChar[-1])
        {

        case '+':
            pLastChar--;
            switch (pLastChar[-1])
            {
            case '@':
                pLastChar--;
                if (pLastChar[-1] == '@')
                {
                    pLastChar--;
                    varop = VarOperation::ptrAtGetInc;     // p@@++
                }
                else
                {
                    varop = VarOperation::varGetInc;     // v@++
                }
                break;

            case '!':
                if (pLastChar[-2] == '@')
                {
                    pLastChar -= 2;
                    varop = VarOperation::ptrAtSetInc;     // p@!++
                }
                break;

            default:
                varop = VarOperation::varInc;            // v++
                break;
            }
            break;

        case '!':
            pLastChar--;
            if (pLastChar[-1] == '@')
            {
                pLastChar--;
                varop = VarOperation::ptrAtSetPlus;            // p@!+
            }
            else
            {
                varop = VarOperation::varSetPlus;            // v!+
            }
            break;

        case'@':
            pLastChar--;
            varop = VarOperation::varPlus;                    // v@+
            break;

        default:
            break;
        }
        break;


    case 'o':
        if (pLastChar[-1] == '!')
        {
            pLastChar--;
            varop = VarOperation::objStoreNoRef;            // v!o - set object, don't inc refcount
        }
        break;
    }

    if (pLastChar == pToken)
    {
        varop = VarOperation::varDefaultOp;
    }
    else
    {
        if (varop != VarOperation::varDefaultOp)
        {
            mpSuffix = pLastChar;
        }
    }

    return varop;
}

void ParseInfo::ChopVaropSuffix()
{
    if (mpSuffix != nullptr)
    {
        int newLen = mpSuffix - (((char *)mpToken) + 1);
        if (newLen != 0)
        {
            mChoppedChar = *mpSuffix;
            *mpSuffix = 0;
            UpdateLength(newLen);
        }
    }
}

void ParseInfo::UnchopVaropSuffix()
{
    if (mpSuffix != nullptr)
    {
        *mpSuffix = mChoppedChar;
        UpdateLength(strlen(((char*)mpToken) + 1));
    }
}


const char* ParseInfo::GetVaropSuffix(VarOperation varop)
{
    const static char* varopNames[] =
    {
        "",     // kVarDefaultOp,
        "@",    // kVarGet,
        "&",    // kVarRef,
        "!",    // kVarSet,
        "!+",   // kVarSetPlus,
        "!-",   // kVarSetMinus,
        "~",    // kVarClear,
        "@+",   // kVarPlus,
        "++",   // kVarInc,
        "@-",   // kVarMinus,
        "--",   // kVarDec,
        "++@",  // kVarIncGet,
        "--@",  // kVarDecGet,
        "@++",  // kVarGetInc,
        "@--",  // kVarGetDec,
        "@@",   // kPtrAtGet,
        "@!",   // kPtrAtSet,
        "@!+,"  // kPtrAtSetPlus,
        "@!-",  // kPtrAtSetMinus,
        "@@++", // kPtrAtGetInc,
        "@@--", // kPtrAtGetDec,
        "@!++", // kPtrAtSetInc,
        "@!--", // kPtrAtSetDec,
        "++@@", // kPtrIncAtGet,
        "--@@", // kPtrDecAtGet,
        "++@!", // kPtrIncAtSet,
        "--@!"  // kPtrDecAtSet,
    };

    return (varop >= VarOperation::varDefaultOp && varop < VarOperation::numVarops)
        ? varopNames[(int)varop] : "_UNKNOWN_VAROP";
}

