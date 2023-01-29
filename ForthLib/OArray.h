#pragma once
//////////////////////////////////////////////////////////////////////
//
// OArray.h: builtin array related classes
//
//////////////////////////////////////////////////////////////////////


class ClassVocabulary;

namespace OArray
{
	void AddClasses(OuterInterpreter* pOuter);

    oArrayStruct* createArrayObject(ClassVocabulary *pClassVocab);

    extern ClassVocabulary* gpArrayClassVocab;
}