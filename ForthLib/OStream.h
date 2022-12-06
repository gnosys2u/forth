#pragma once
//////////////////////////////////////////////////////////////////////
//
// OStream.h: builtin stream related classes
//
//////////////////////////////////////////////////////////////////////


class ForthClassVocabulary;

namespace OStream
{
	void AddClasses(ForthOuterInterpreter* pOuter);

	ForthObject getStdoutObject();
	ForthObject getStderrObject();
}