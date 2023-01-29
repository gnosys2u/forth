#pragma once
//////////////////////////////////////////////////////////////////////
//
// OStream.h: builtin stream related classes
//
//////////////////////////////////////////////////////////////////////


class ClassVocabulary;

namespace OStream
{
	void AddClasses(OuterInterpreter* pOuter);

	ForthObject getStdoutObject();
	ForthObject getStderrObject();
}