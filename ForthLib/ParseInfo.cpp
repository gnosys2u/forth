//////////////////////////////////////////////////////////////////////
//
// ParseInfo.cpp: implementation of the ParseInfo class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "Engine.h"
#include "OuterInterpreter.h"
#include "ParseInfo.h"
#include "Shell.h"
#include "Input.h"
#include "Vocabulary.h"
#include "Extension.h"

ParseInfo::ParseInfo(int32_t *pBuffer, int numLongs)
	: mpToken(pBuffer)
	, mMaxChars((numLongs << 2) - 2)
	, mFlags(0)
	, mNumLongs(0)
	, mNumChars(0)
    , mpSuffix(nullptr)
    , mSuffixVarop(VarOperation::kNumVarops)
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
void
ParseInfo::SetToken(const char* pSrc)
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

char
ParseInfo::BackslashChar(const char*& pSrc)
{
    char c = *pSrc;
    char cResult = c;

    if (c != '\0')
    {
        pSrc++;
        switch (c)
        {

        case 'a':        cResult = '\a';        break;		// x07	bell
        case 'b':        cResult = '\b';        break;		// x08	backspace
        case 'e':        cResult = 0x1b;        break;		// x1b	escape
        case 'f':        cResult = '\f';        break;		// x0b	formfeed
        case 'n':        cResult = '\n';        break;		// x0a	newline
        case 'r':        cResult = '\r';        break;		// x0d	carriage return
        case 't':        cResult = '\t';        break;		// x09	tab (horizontal)
        case 'v':        cResult = '\v';        break;		// x0b	vertical tab
        case '0':        cResult = '\0';        break;		// x00	nul

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

		// this handles \'  \"  \\  \` among others
        default:         cResult = c;           break;

        }
    }

	return cResult;
}


const char *
ParseInfo::ParseSingleQuote(const char *pSrcIn, const char *pSrcLimit, Engine *pEngine, bool keepBackslashes)
{
	char cc[9];
	bool isQuotedChar = false;
	bool isLongConstant = false;

	if ((pSrcIn[1] != 0) && (pSrcIn[2] != 0))      // there must be at least 2 more chars on line
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
                ch = BackslashChar(pSrc);
                cc[iDst++] = ch;
			}
			else if (ch == '`')
			{
				cc[iDst++] = '\0';
				isQuotedChar = true;
				if (pSrc < pSrcLimit)
				{
					ch = *pSrc;
					if ((ch == 'l') || (ch == 'L'))
					{
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
			if (*pSrc == '`')
			{
				pSrc++;
				cc[iDst++] = '\0';
				isQuotedChar = true;
			}
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
	return pSrcIn;
}


void
ParseInfo::ParseDoubleQuote(const char *&pSrc, const char *pSrcLimit, bool keepBackslashes)
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
				*pDst++ = '\\';
			}
            pSrc++;
			ch = BackslashChar(pSrc);
			break;

		default:
			ch = *pSrc++;
			break;

		}

        *pDst++ = ch;
	}
	*pDst = 0;
	SetToken();
}

VarOperation ParseInfo::CheckVaropSuffix()
{
    VarOperation varop = VarOperation::kVarDefaultOp;

    mpSuffix = nullptr;
    char* pToken = ((char*)mpToken) + 1;
    char* pLastChar = pToken + (mNumChars - 1);

    switch (*pLastChar)
    {
    case '&':
        varop = VarOperation::kVarRef;
        break;

    case '~':
        if (pLastChar[-1] == '@')
        {
            pLastChar--;
            varop = VarOperation::kVarUnref;        // v@~ - push obj on TOS, clear var, decrement obj refcount but don't delete if 0
        }
        else
        {
            varop = VarOperation::kVarClear;        // v~ - clear var and decrement obj refcount, delete if 0
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
                    varop = VarOperation::kPtrDecAtGet;     // p--@@
                }
                break;

            case '+':
                if (pLastChar[-2] == '+')
                {
                    pLastChar -= 2;
                    varop = VarOperation::kPtrIncAtGet;     // p++@@
                }
                break;

            default:
                varop = VarOperation::kPtrAtGet;            // p@@
                break;
            }
            break;

        case '-':
            if (pLastChar[-1] == '-')
            {
                pLastChar -= 2;
                varop = VarOperation::kVarDecGet;     // v--@
            }
            break;

        case '+':
            if (pLastChar[-1] == '+')
            {
                pLastChar -= 2;
                varop = VarOperation::kVarIncGet;     // v++@
            }
            break;

        default:
            varop = VarOperation::kVarGet;            // v@
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
                    varop = VarOperation::kPtrDecAtSet;     // p--@!
                }
                break;

            case '+':
                if (pLastChar[-2] == '+')
                {
                    pLastChar -= 2;
                    varop = VarOperation::kPtrIncAtSet;     // p++@!
                }
                break;

            default:
                varop = VarOperation::kPtrAtSet;            // p@!
                break;
            }
            break;

        default:
            varop = VarOperation::kVarSet;            // v!
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
                    varop = VarOperation::kPtrAtGetDec;     // p@@--
                }
                else
                {
                    varop = VarOperation::kVarGetDec;     // v@--
                }
                break;

            case '!':
                if (pLastChar[-2] == '@')
                {
                    pLastChar -= 2;
                    varop = VarOperation::kPtrAtSetDec;     // p@!--
                }
                break;

            default:
                varop = VarOperation::kVarDec;            // v--
                break;
            }
            break;

        case '!':
            pLastChar--;
            if (pLastChar[-1] == '@')
            {
                pLastChar--;
                varop = VarOperation::kPtrAtSetMinus;            // p@!-
            }
            else
            {
                varop = VarOperation::kVarSetMinus;             // v!-
            }
            break;

        case'@':
            pLastChar--;
            varop = VarOperation::kVarMinus;                    // v@-
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
                    varop = VarOperation::kPtrAtGetDec;     // p@@++
                }
                else
                {
                    varop = VarOperation::kVarGetInc;     // v@++
                }
                break;

            case '!':
                if (pLastChar[-2] == '@')
                {
                    pLastChar -= 2;
                    varop = VarOperation::kPtrAtSetInc;     // p@!++
                }
                break;

            default:
                varop = VarOperation::kVarInc;            // v++
                break;
            }
            break;

        case '!':
            pLastChar--;
            if (pLastChar[-1] == '@')
            {
                pLastChar--;
                varop = VarOperation::kPtrAtSetPlus;            // p@!+
            }
            else
            {
                varop = VarOperation::kVarSetPlus;            // v!+
            }
            break;

        case'@':
            pLastChar--;
            varop = VarOperation::kVarPlus;                    // v@+
            break;

        default:
            break;
        }
        break;


    case 'o':
        if (pLastChar[-1] == '!')
        {
            pLastChar--;
            varop = VarOperation::kVarSetPlus;            // v!o - set object, don't inc refcount
        }
        break;
    }

    if (pLastChar == pToken)
    {
        varop = VarOperation::kVarDefaultOp;
    }
    else
    {
        if (varop != VarOperation::kVarDefaultOp)
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

    return (varop >= VarOperation::kVarDefaultOp && varop < VarOperation::kNumVarops)
        ? varopNames[(int)varop] : "_UNKNOWN_VAROP";
}

