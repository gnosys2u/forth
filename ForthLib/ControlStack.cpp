//////////////////////////////////////////////////////////////////////
//
// ControlStack.cpp: implementation of the ControlStack class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "Engine.h"
#include "ControlStack.h"
#include "OuterInterpreter.h"

//////////////////////////////////////////////////////////////////////
////
///
//                     ControlStack
// 

// this is the number of extra longs to allocate at top and
//    bottom of stacks
#define GAURD_AREA 4

ControlStack::ControlStack( int stackEntries)
: mCSLen(stackEntries)
, mDepth(0)
{
   mCSB = new ControlStackEntry[mCSLen + (GAURD_AREA * 2)];
   mCSB += GAURD_AREA;
   mCST = mCSB + mCSLen;
   EmptyStack();
   mpEngine = Engine::GetInstance();
}

ControlStack::~ControlStack()
{
   delete [] (mCSB - GAURD_AREA);
}

ControlStackEntry* ControlStack::Peek(int index)
{
    if (index < mDepth)
    {
        return &(mCSB[mDepth - (index + 1)]);
    }

}

void ControlStack::Drop()
{
    if (mDepth > 0)
    {
        mDepth--;
    }
    else
    {
        SPEW_SHELL("Failed to pop control stack\n");
        mpEngine->SetError(ForthError::controlStackUnderflow);
    }
}

void ControlStack::Push(ControlStackTag tag, void* address, const char* name, ucell op)
{
    if (mDepth >= mCSLen)
    {
        SPEW_SHELL("Control stack overflow!\n");
        mpEngine->SetError(ForthError::controlStackOverflow);
        return;
    }

    ControlStackEntry& entry = mCSB[mDepth];
    entry.tag = tag;
    entry.address = address;
    if (name == nullptr)
    {
        entry.name = nullptr;
    }
    else
    {
        OuterInterpreter* pOuter = mpEngine->GetOuterInterpreter();
        entry.name = pOuter->AddTempString(name);
    }
    entry.op = op;

#ifdef TRACE_SHELL
    char tagString[256];
    if (mpEngine->GetTraceFlags() & kLogShell)
    {
        GetTagString(tag, tagString, sizeof(tagString));
        const char* logName = (name == nullptr) ? "" : name;
        SPEW_SHELL("ControlStack[%d]: pushed %s %p {%s} 0x%x\n", mDepth, tagString, address, logName, op);
    }
#endif
    mDepth++;
}

void ControlStack::ShowStack()
{
    char buff[512];

	mpEngine->ConsoleOut("control stack:\n");
    cell ix = mDepth - 1;
    while (ix >= 0)
    {
        ControlStackEntry& entry = mCSB[ix];
        sprintf(buff, "[%d] ", ix);
        mpEngine->ConsoleOut(buff);
        
        GetTagString((cell)entry.tag, buff, sizeof(buff));
        mpEngine->ConsoleOut(buff);
        
        if (entry.name != nullptr)
        {
            sprintf(buff, "  {%s}", entry.name);
            mpEngine->ConsoleOut(buff);
        }

        sprintf(buff, "  addr:%p  op:0x%x", entry.address, entry.op);
        mpEngine->ConsoleOut(buff);
        ix--;
    }
}

void ControlStack::GetTagString(ucell tags, char* pBuffer, size_t bufferSize)
{
    bool foundOne = false;
    int mask = 1;
    int index = 0;
    pBuffer[0] = '\0';

    const char* TagStrings[] =
    {
        "NOTHING",
        "do",
        "begin",
        "while",
        "case/of",
        "if",
        "else",
        "paren",
        "string",
        "colon",
        "poundDirective",
        "of",
        "ofif",
        "andif",
        "orif",
        "elif",
        "try",
        "catch",
        "finally",
        "colonNoName",
        "interface",
        "struct",
        "class",
        "enum",
        "function",
        "method",
        nullptr
    };

    while ((tags != 0) && (TagStrings[index] != nullptr))
    {
        if ((mask & tags) != 0)
        {
            if (foundOne)
            {
                strcat(pBuffer, "|");
            }
            strcat(pBuffer, TagStrings[index]);
            foundOne = true;
        }
        index++;
        tags >>= 1;
    }

    if (!foundOne)
    {
        sprintf(pBuffer, "UNKNOWN TAG 0x%x", tags);
    }
}

