//////////////////////////////////////////////////////////////////////
//
// ExpressionInputStream.cpp: implementation of the ExpressionInputStream class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ExpressionInputStream.h"
#include "Engine.h"
#include "ParseInfo.h"


//////////////////////////////////////////////////////////////////////
////
///
//                     ExpressionInputStream
// 
#define INITIAL_EXPRESSION_STACK_SIZE 2048

ExpressionInputStream::ExpressionInputStream()
	: InputStream(INITIAL_EXPRESSION_STACK_SIZE)
	, mStackSize(INITIAL_EXPRESSION_STACK_SIZE)
{
    // expression input streams shouldn't be deleted when empty since they are
    //  used multiple times
    mbDeleteWhenEmpty = false;

	mpStackBase = static_cast<char *>(__MALLOC(mStackSize));
	mpLeftBase = static_cast<char *>(__MALLOC(mStackSize + 1));
	mpRightBase = static_cast<char *>(__MALLOC(mStackSize + 1));
	ResetStrings();
}

void ExpressionInputStream::ResetStrings()
{
	mpStackTop = mpStackBase + mStackSize;
	mpStackCursor = mpStackTop;
	*--mpStackCursor = '\0';
	*--mpStackCursor = '\0';
	mpLeftCursor = mpLeftBase;
	mpLeftTop = mpLeftBase + mStackSize;
	*mpLeftCursor = '\0';
	*mpLeftTop = '\0';
	mpRightCursor = mpRightBase;
	mpRightTop = mpRightCursor + mStackSize;
	*mpRightCursor = '\0';
	*mpRightTop = '\0';
}

ExpressionInputStream::~ExpressionInputStream()
{
	__FREE(mpStackBase);
	__FREE(mpLeftBase);
	__FREE(mpRightBase);
}

char* topStr = NULL;
char* nextStr = NULL;

#if 0
#define LOG_EXPRESSION(STR) SPEW_SHELL("ExpressionInputStream::%s L:{%s} R:{%s}  (%s)(%s)\n",\
	STR, mpLeftBase, mpRightBase, mpStackCursor, (mpStackCursor + strlen(mpStackCursor) + 1))
#else
#define LOG_EXPRESSION(STR)
#endif

bool
ExpressionInputStream::ProcessExpression(InputStream* pInputStream)
{
	// z	(a,b) -> (a,bz)
	// _	(a,b) -> (ab_,)		underscore represents space, tab or EOL
	// )	(a,b)(c,d)  -> (c,ab_d)
	// (	(a,b) -> (,)(a,b)

	bool result = true;

	int nestingDepth = 0;
	ResetStrings();
	const char* pSrc = pInputStream->GetBufferPointer();
	const char* pNewSrc = pSrc;
    char c = 0;
	char previousChar = '\0';
	bool danglingPeriod = false;	 // to allow ")." at end of line to force continuation to next line
	ParseInfo parseInfo((int32_t *)mpBufferBase, mBufferLen);
	Engine* pEngine = Engine::GetInstance();

	bool done = false;
	bool atEOL = false;
	while (!done)
	{
		const char* pSrcLimit = pInputStream->GetBufferBasePointer() + pInputStream->GetWriteOffset();
		if (atEOL || (pSrc >= pSrcLimit))
		{
			if ((nestingDepth != 0) || danglingPeriod)
			{
				// input buffer is empty
				pSrc = pInputStream->GetLine("expression>");
				pSrcLimit = pInputStream->GetBufferBasePointer() + pInputStream->GetWriteOffset();
				// TODO: skip leading whitespace
			}
			else
			{
				done = true;
			}
		}
		if (pSrc != nullptr)
		{
			c = *pSrc++;
			//SPEW_SHELL("process character {%c} 0x%x\n", c, c);
#if 0
            if (c == '\\')
			{
                c = ParseInfo::BackslashChar(pSrc);
			}
#endif
			pInputStream->SetBufferPointer(pSrc);
			switch (c)
			{
				case ' ':
				case '\t':
					// whitespace completes the token sitting in right string
					if (mpRightCursor != mpRightBase)
					{
						CombineRightIntoLeft();
					}
					done = (nestingDepth == 0);
					break;

				case '\0':
					atEOL = true;
					break;

				case '(':
					PushStrings();
					nestingDepth++;
					break;

				case ')':
					PopStrings();
					nestingDepth--;
					break;

				case '/':
					if (*pSrc == '/')
					{
						// this is an end-of-line comment
						atEOL = true;
					}
					else
					{
						AppendCharToRight(c);
					}
					break;

#if 0
				case '\'':
					if (mpRightCursor != mpRightBase)
					{
						CombineRightIntoLeft();
					}
					pNewSrc = parseInfo.ParseSingleQuote(pSrc - 1, pSrcLimit, pEngine, true);
					if ((pNewSrc == (pSrc - 1)) && ((*pSrc == ' ') || (*pSrc == '\t')))
					{
						// this is tick operator
						AppendCharToRight(c);
						AppendCharToRight(*pSrc++);
						CombineRightIntoLeft();
					}
					else
					{
						AppendCharToRight(c);
						if (parseInfo.GetFlags() & PARSE_FLAG_QUOTED_CHARACTER)
						{
							const char* pChars = (char *)parseInfo.GetToken();
							while (*pChars != '\0')
							{
								char cc = *pChars++;
								/*
								if (cc == ' ')
								{
								// need to prefix spaces in character constants with backslash
								AppendCharToRight('\\');
								}
								*/
								AppendCharToRight(cc);
							}
							pSrc = pNewSrc;
							AppendCharToRight(c);
							if (parseInfo.GetFlags() & PARSE_FLAG_FORCE_LONG)
							{
								AppendCharToRight('L');
							}
							CombineRightIntoLeft();
						}
					}
					break;
#endif

				case '\"':
					if (mpRightCursor != mpRightBase)
					{
						CombineRightIntoLeft();
                    }
                    pNewSrc = pSrc - 1;   // point back at the quote
                    parseInfo.ParseDoubleQuote(pNewSrc, pSrcLimit, true);
					if (pNewSrc == (pSrc - 1))
					{
						// TODO: report error
					}
					else
					{
						AppendCharToRight(c);
						AppendStringToRight((char *)parseInfo.GetToken());
						pSrc = pNewSrc;
						AppendCharToRight(c);
						CombineRightIntoLeft();
					}
					break;

				default:
					if ((previousChar == ')') && (c != '.'))
					{
						// this seems hokey, but it fixes cases like "a(b(1)2)" becoming "1 b2 a" instead of "1 b 2 a"
						//   and doesn't break "a(1).b(2)" like some other fixes
						CombineRightIntoLeft();
					}
					AppendCharToRight(c);
					break;
			}
		}
		else
		{
			AppendCharToRight(' ');		// TODO: is this necessary?
			CombineRightIntoLeft();
			done = true;
		}
		danglingPeriod = (previousChar == ')') && (c == '.');
		previousChar = c;
	}

	strcpy(mpBufferBase, mpLeftBase);
	strcat(mpBufferBase, " ");
	strcat(mpBufferBase, mpRightBase);
	mReadOffset = 0;
	mWriteOffset = strlen(mpBufferBase);
	SPEW_SHELL("ExpressionInputStream::ProcessExpression  result:{%s}\n", mpBufferBase);
	return result;
}

cell ExpressionInputStream::GetSourceID()
{
	return -1;
}

char* ExpressionInputStream::GetLine(const char *pPrompt)
{
	return NULL;
}

const char* ExpressionInputStream::GetType(void)
{
	return "Expression";
}

void ExpressionInputStream::SeekToLineEnd()
{

}

cell* ExpressionInputStream::GetInputState()
{
	// TODO: error!
	return nullptr;
}

bool
ExpressionInputStream::SetInputState(cell* pState)
{
	// TODO: error!
	return false;
}

void ExpressionInputStream::PushString(char *pString, int numBytes)
{
	char* pNewBase = mpStackCursor - (numBytes + 1);
	if (pNewBase > mpStackBase)
	{
		memcpy(pNewBase, pString, numBytes + 1);
		mpStackCursor = pNewBase;
	}
	else
	{
		// TODO: report stack overflow
	}
}

void ExpressionInputStream::PushStrings()
{
	//SPEW_SHELL("ExpressionInputStream::PushStrings  left:{%s}  right:{%s}\n", mpLeftBase, mpRightBase);
	PushString(mpRightBase, mpRightCursor - mpRightBase);
	nextStr = mpStackCursor;
	PushString(mpLeftBase, mpLeftCursor - mpLeftBase);
	mpRightCursor = mpRightBase;
	*mpRightCursor = '\0';
	mpLeftCursor = mpLeftBase;
	*mpLeftCursor = '\0';
	LOG_EXPRESSION("PushStrings");
}

void ExpressionInputStream::PopStrings()
{
	// at start, leftString is a, rightString is b, top of string stack is c, next on stack is d
	// (a,b)(c,d)  -> (c,ab_d)   OR  rightString = leftString + rightString + space + stack[1], leftString = stack[0]
	if (mpStackCursor < mpStackTop)
	{
		int lenA = mpLeftCursor - mpLeftBase;
		int lenB = mpRightCursor - mpRightBase;
		int lenStackTop = strlen(mpStackCursor);
		char* pStackNext = mpStackCursor + lenStackTop + 1;
		int lenStackNext = strlen(pStackNext);
		// TODO: check that ab_d will fit in right string
		// append right to left
		if (lenB > 0)
		{
			if (lenA > 0)
			{
				*mpLeftCursor++ = ' ';
			}
			memcpy(mpLeftCursor, mpRightBase, lenB + 1);
			mpLeftCursor += lenB;
		}
		// append second stacked string to left
		if (lenStackNext > 0)
		{
			if (mpLeftCursor != mpLeftBase)
			{
				*mpLeftCursor++ = ' ';
			}
			memcpy(mpLeftCursor, pStackNext, lenStackNext + 1);
			mpLeftCursor += lenStackNext;
		}
		lenA = mpLeftCursor - mpLeftBase;
		if (lenA > 0)
		{
			memcpy(mpRightBase, mpLeftBase, lenA + 1);
		}
		mpRightCursor = mpRightBase + lenA;
		//AppendCharToRight(' ');

		// copy top of stack to left
		memcpy(mpLeftBase, mpStackCursor, lenStackTop + 1);
		mpLeftCursor = mpLeftBase + lenStackTop;

		// remove both strings from stack
		mpStackCursor = pStackNext + lenStackNext + 1;
		// TODO: check that stack cursor is not above stackTop
		LOG_EXPRESSION("PopStrings");
		//SPEW_SHELL("ExpressionInputStream::PopStrings  left:{%s}  right:{%s}\n", mpLeftBase, mpRightBase);
	}
	else
	{
		// TODO: report pop of empty stack
	}
	nextStr = mpStackCursor + strlen(mpStackCursor) + 1;
}

void ExpressionInputStream::AppendStringToRight(const char* pString)
{
	int len = strlen(pString);
	if ((mpRightCursor + len) < mpRightTop)
	{
		memcpy(mpRightCursor, pString, len + 1);
		mpRightCursor += len;
		LOG_EXPRESSION("AppendStringToRight");
		//SPEW_SHELL("ExpressionInputStream::AppendStringToRight  left:{%s}  right:{%s}\n", mpLeftBase, mpRightBase);
	}
	else
	{
		// TODO: report right string overflow
	}
}


void ExpressionInputStream::AppendCharToRight(char c)
{
	if (mpRightCursor < mpRightTop)
	{
		*mpRightCursor++ = c;
		*mpRightCursor = '\0';
		LOG_EXPRESSION("AppendCharToRight");
		//SPEW_SHELL("ExpressionInputStream::AppendCharToRight  left:{%s}  right:{%s}\n", mpLeftBase, mpRightBase);
	}
	else
	{
		// TODO: report right string overflow
	}
}

void ExpressionInputStream::CombineRightIntoLeft()
{
	int spaceRemainingInLeft = mpLeftTop - mpLeftCursor;
	int rightLen = mpRightCursor - mpRightBase;
	if (spaceRemainingInLeft > rightLen)
	{
		if (mpLeftCursor != mpLeftBase)
		{
			*mpLeftCursor++ = ' ';
		}
		memcpy(mpLeftCursor, mpRightBase, rightLen + 1);
		mpLeftCursor += rightLen;
	}
	mpRightCursor = mpRightBase;
	*mpRightCursor = '\0';
	LOG_EXPRESSION("CombineRightIntoLeft");
	//SPEW_SHELL("ExpressionInputStream::CombineRightIntoLeft  left:{%s}  right:{%s}\n", mpLeftBase, mpRightBase);
}


bool ExpressionInputStream::IsGenerated()
{
	return true;
}
