#pragma once
//////////////////////////////////////////////////////////////////////
//
// ExpressionInputStream.h: interface for the ExpressionInputStream class.
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

//#include "InputStream.h"
#include "InputStream.h"

class ParseInfo;

class ExpressionInputStream : public InputStream
{
public:
	ExpressionInputStream();
	virtual ~ExpressionInputStream();

	// returns true IFF expression was processed successfully
	bool ProcessExpression(InputStream* pInputStream);

	virtual cell    GetSourceID() const;
	virtual char*   GetLine(const char *pPrompt);
    virtual char*   AddLine();
    virtual bool    IsInteractive(void) { return false; };
	virtual InputStreamType GetType(void) const;
    virtual const char* GetName(void) const;

	virtual void    SeekToLineEnd();

	virtual cell*   GetInputState();
	virtual bool    SetInputState(cell* pState);

	virtual bool	IsGenerated();

protected:
	void			PushStrings();
	void			PushString(char *pString, int numBytes);
	void			PopStrings();
	void			AppendCharToRight(char c);
	void			AppendStringToRight(const char* pString);
	void			CombineRightIntoLeft();
	void			ResetStrings();
	inline bool		StackEmpty() { return mpStackCursor == mpStackTop; }

	uint32_t		mStackSize;
	char*				mpStackBase;
	char*				mpStackTop;
	char*				mpStackCursor;
	char*				mpLeftBase;
	char*				mpLeftCursor;
	char*				mpLeftTop;
	char*				mpRightBase;
	char*				mpRightCursor;
	char*				mpRightTop;
	ParseInfo*		mpParseInfo;
};

