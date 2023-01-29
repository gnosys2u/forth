#pragma once
//////////////////////////////////////////////////////////////////////
//
// OMap.h: builtin map related classes
//
//////////////////////////////////////////////////////////////////////


class ClassVocabulary;

namespace OMap
{
	void AddClasses(OuterInterpreter* pOuter);

    oLongMapStruct* createLongMapObject(ClassVocabulary *pClassVocab);

    extern ClassVocabulary* gpLongMapClassVocab;
}