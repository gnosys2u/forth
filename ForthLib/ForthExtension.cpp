//////////////////////////////////////////////////////////////////////
//
// ForthExtension.cpp: support for midi devices
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ForthEngine.h"
#include "ForthExtension.h"

//////////////////////////////////////////////////////////////////////
////
///     ForthExtension
//
//

ForthExtension::ForthExtension()
:   mpEngine( NULL )
{
}


ForthExtension::~ForthExtension()
{
}

void ForthExtension::Initialize( ForthEngine* pEngine )
{
    mpEngine = pEngine;
    Reset();
}


void ForthExtension::Reset()
{
}


void ForthExtension::Shutdown()
{
}


void ForthExtension::ForgetOp( ulong opNumber )
{
}

