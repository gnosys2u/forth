#pragma once
//////////////////////////////////////////////////////////////////////
//
// OSystem.h: builtin system class
//
//////////////////////////////////////////////////////////////////////


class ForthClassVocabulary;

namespace OSystem
{
	extern ForthClassVocabulary* gpOSystemClassVocab;

	struct oSystemStruct
	{
        forthop*        pMethods;
        REFCOUNTER      refCount;
        ForthObject     namedObjects;
        ForthObject     args;
        ForthObject     env;
        ForthObject     shellStack;
    };

    void AddClasses(ForthOuterInterpreter* pOuter);
    void Shutdown(ForthEngine* pEngine);
}
