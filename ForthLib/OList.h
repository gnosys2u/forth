#pragma once
//////////////////////////////////////////////////////////////////////
//
// OList.h: builtin list related classes
//
//////////////////////////////////////////////////////////////////////


class ClassVocabulary;

namespace OList
{
	struct oListIterStruct
	{
        forthop*        pMethods;
		REFCOUNTER      refCount;
		ForthObject		parent;
		oListElement*	cursor;
	};

	void AddClasses(OuterInterpreter* pOuter);
}