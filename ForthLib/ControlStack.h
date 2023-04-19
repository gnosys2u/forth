#pragma once
//////////////////////////////////////////////////////////////////////
//
// ControlStack.h: interface for the ControlStack class.
//
//////////////////////////////////////////////////////////////////////

#include "Forth.h"

enum ControlStackTag
{
    kCSTagNothing       = 0x00000001,
    kCSTagDo            = 0x00000002,
    kCSTagBegin         = 0x00000004,
    kCSTagWhile         = 0x00000008,
    kCSTagCase          = 0x00000010,
    kCSTagIf            = 0x00000020,
    kCSTagElse          = 0x00000040,
    kCSTagParen         = 0x00000080,
    kCSTagString        = 0x00000100,
    kCSTagDefColon      = 0x00000200,
    kCSTagPoundIf       = 0x00000400,
    kCSTagOf            = 0x00000800,
    kCSTagOfIf          = 0x00001000,
    kCSTagAndIf         = 0x00002000,
    kCSTagOrIf          = 0x00004000,
    kCSTagElif          = 0x00008000,
    kCSTagTry           = 0x00010000,
    kCSTagCatcher       = 0x00020000,
    kCSTagFinally       = 0x00040000,
    kCSTagDefNoName     = 0x00080000,
    kCSTagDefInterface  = 0x00100000,
    kCSTagDefStruct     = 0x00200000,
    kCSTagDefClass      = 0x00400000,
    kCSTagDefEnum       = 0x00800000,
    kCSTagDefFunction   = 0x01000000,
    kCSTagDefMethod     = 0x02000000,
    kCSTagLastTag = kCSTagDefMethod  // update this when you add a new tag
   // if you add tags, remember to update TagStrings in Shell.cpp
};

typedef struct ControlStackEntry
{
    const char* name;
    void* address;
    ucell op;
    ControlStackTag tag;
};

class ControlStack
{
public:
   ControlStack( int stackEntries = 256 );
   virtual ~ControlStack();

   ControlStackEntry* Peek(int index = 0);
   void Drop();
   void Push(ControlStackTag tag, void* address = nullptr, const char* name = nullptr, ucell op = 0);
   inline ucell        GetSize(void)         { return mCSLen; };
   inline cell         GetDepth(void)        { return mDepth; };
   inline void         EmptyStack(void)      { mDepth = 0; };
   
   void GetTagString(ucell tags, char* pBuffer, size_t bufferSize);

#if 0
   // push tag telling what control structure we are compiling (if/else/for/...)
   void         PushTag(ControlStackTag tag);
   void         PushAddress(forthop* val);
   void         Push(ucell val);
   ControlStackTag PopTag(void);
   forthop*     PopAddress(void);
   ucell         Pop(void);

   ControlStackTag    PeekTag(int index = 0);
   forthop*     PeekAddress(int index = 0);
   cell        Peek(int index = 0);

   // push a string, this should be followed by a PushTag of a tag which uses this string (such as paren)
   void                PushString(const char *pString);
   // return true IFF item on top of control stack is a string
   bool                PopString(char *pString, int maxLen);
#endif

   void					ShowStack();

protected:
    ucell mDepth;
    ControlStackEntry* mCSB;       // control stack base
    ControlStackEntry* mCST;       // empty control stack pointer
	ucell              mCSLen;     // size of control stack in longwords
	Engine         *mpEngine;
};

