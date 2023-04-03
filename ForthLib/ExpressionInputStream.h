#pragma once
//////////////////////////////////////////////////////////////////////
//
// ExpressionInputStream.h: interface for the ExpressionInputStream class.
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

