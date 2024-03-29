//////////////////////////////////////////////////////////////////////
//
// Extension.cpp: support for midi devices
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "Engine.h"
#include "Extension.h"

//////////////////////////////////////////////////////////////////////
////
///     Extension
//
//

Extension::Extension()
:   mpEngine( NULL )
{
}


Extension::~Extension()
{
}

void Extension::Initialize( Engine* pEngine )
{
    mpEngine = pEngine;
    Reset();
}


void Extension::Reset()
{
}


void Extension::Shutdown()
{
}


void Extension::ForgetOp( uint32_t opNumber )
{
}

