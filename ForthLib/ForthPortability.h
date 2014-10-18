#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthPortability.h: macros to hide system dependancies
//
//////////////////////////////////////////////////////////////////////


#ifdef WIN32

#define SNPRINTF sprintf_s

#else

#define SNPRINTF snprintf

#endif
