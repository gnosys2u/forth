#pragma once
//////////////////////////////////////////////////////////////////////
//
// OArray.h: builtin array related classes
//
//////////////////////////////////////////////////////////////////////


class ForthClassVocabulary;

namespace OArray
{
	void AddClasses(ForthEngine* pEngine);

    oArrayStruct* createArrayObject(ForthClassVocabulary *pClassVocab);

    extern ForthClassVocabulary* gpArrayClassVocab;
}