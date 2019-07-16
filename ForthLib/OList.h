#pragma once
//////////////////////////////////////////////////////////////////////
//
// OList.h: builtin list related classes
//
//////////////////////////////////////////////////////////////////////


class ForthClassVocabulary;

namespace OList
{
	struct oListIterStruct
	{
        forthop*        pMethods;
        ucell           refCount;
		ForthObject		parent;
		oListElement*	cursor;
	};

	void AddClasses(ForthEngine* pEngine);
}