#pragma once
//////////////////////////////////////////////////////////////////////
//
// OMap.h: builtin map related classes
//
//////////////////////////////////////////////////////////////////////


class ForthClassVocabulary;

namespace OMap
{
	void AddClasses(ForthOuterInterpreter* pOuter);

    oLongMapStruct* createLongMapObject(ForthClassVocabulary *pClassVocab);

    extern ForthClassVocabulary* gpLongMapClassVocab;
}