#pragma once
//////////////////////////////////////////////////////////////////////
//
// OSystem.h: builtin system class
//
//////////////////////////////////////////////////////////////////////


class ClassVocabulary;

namespace OSystem
{
	extern ClassVocabulary* gpOSystemClassVocab;

	struct oSystemStruct
	{
        forthop*        pMethods;
        REFCOUNTER      refCount;
        ForthObject     namedObjects;
        ForthObject     args;
        ForthObject     env;
        ForthObject     controlStack;
        ForthObject     searchStack;
    };

    void AddClasses(OuterInterpreter* pOuter);
    void Shutdown(Engine* pEngine);
}
