//////////////////////////////////////////////////////////////////////
//
// ForthParseInfo.cpp: implementation of the ForthParseInfo class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "ForthEngine.h"
#include "ForthParseInfo.h"
#include "ForthShell.h"
#include "ForthInput.h"
#include "ForthVocabulary.h"
#include "ForthExtension.h"

ForthParseInfo::ForthParseInfo(long *pBuffer, int numLongs)
	: mpToken(pBuffer)
	, mMaxChars((numLongs << 2) - 2)
	, mFlags(0)
	, mNumLongs(0)
{
	ASSERT(numLongs > 0);

	// zero length byte, and first char
	*mpToken = 0;
}


ForthParseInfo::~ForthParseInfo()
{
	// NOTE: don't delete mpToken, it doesn't belong to us
}


// copy string to mpToken buffer, set length, and pad with nulls to a longword boundary
// if pSrc is null, just set length and do padding
void
ForthParseInfo::SetToken(const char *pSrc)
{
	int symLen, padChars;
	char *pDst;

	if (pSrc != NULL)
	{

		symLen = strlen(pSrc);
		pDst = (char *)mpToken;

		if (symLen > mMaxChars)
		{
			symLen = mMaxChars;
		}
        mNumChars = symLen;
		// set length byte
#ifdef WIN32
        *pDst++ = min(symLen, 255);
#else
        *pDst++ = std::min(symLen, 255);
#endif

		// make copy of symbol
		memcpy(pDst, pSrc, symLen);
		pDst += symLen;
		*pDst++ = '\0';

	}
	else
	{

		// token has already been copied to mpToken, just set length byte
		symLen = strlen(((char *)mpToken) + 1);
        mNumChars = symLen;
#ifdef WIN32
		*((char *)mpToken) = min(symLen, 255);
#else
		*((char *)mpToken) = std::min(symLen, 255);
#endif
		pDst = ((char *)mpToken) + symLen + 2;
	}

	// in diagram, first char is length byte, 'a' are symbol chars, '0' is terminator, '#' is padding
	//
	//            symLen     padding nLongs
	// 1a0#|        1           1       1
	// 2aa0|        2           0       1
	// 3aaa|0       3           0       1
	// 4aaa|a0##    4           2       2
	// 5aaa|aa0#    5           1       2
	//
	mNumLongs = (symLen + 4) >> 2;


	padChars = (symLen + 2) & 3;
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
ForthParseInfo::BackslashChar(const char*& pSrc)
{
    char c = *pSrc;
    char cResult = c;

    if (c != '\0')
    {
        pSrc++;
        switch (c)
        {

        case 'a':        cResult = '\a';        break;
        case 'b':        cResult = '\b';        break;
        case 'e':        cResult = 0x1b;        break;
        case 'f':        cResult = '\f';        break;
        case 'n':        cResult = '\n';        break;
        case 'r':        cResult = '\r';        break;
        case 't':        cResult = '\t';        break;
        case 'v':        cResult = '\v';        break;
        case '0':        cResult = '\0';        break;
        case '?':        cResult = '\?';        break;
        case '\\':       cResult = '\\';        break;
        case '\'':       cResult = '\'';        break;
        case '\"':       cResult = '\"';        break;

        case 'x':
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

        default:         cResult = c;           break;

        }
    }

	return cResult;
}


const char *
ForthParseInfo::ParseSingleQuote(const char *pSrcIn, const char *pSrcLimit, ForthEngine *pEngine, bool keepBackslashes)
{
	char cc[9];
	bool isQuotedChar = false;
	bool isLongConstant = false;

	if ((pSrcIn[1] != 0) && (pSrcIn[2] != 0))      // there must be at least 2 more chars on line
	{
		const char *pSrc = pSrcIn + 1;
		int iDst = 0;
		int maxChars = pEngine->CheckFeature(kFFMultiCharacterLiterals) ? 8 : 1;
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
ForthParseInfo::ParseDoubleQuote(const char *&pSrc, const char *pSrcLimit, bool keepBackslashes)
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


