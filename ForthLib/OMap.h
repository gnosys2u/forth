#pragma once
//////////////////////////////////////////////////////////////////////
//
// OMap.h: builtin map related classes
//
//////////////////////////////////////////////////////////////////////


class ForthClassVocabulary;

namespace OMap
{
	void AddClasses(ForthEngine* pEngine);

    oLongMapStruct* createLongMapObject(ForthClassVocabulary *pClassVocab);

    extern ForthClassVocabulary* gpLongMapClassVocab;
}