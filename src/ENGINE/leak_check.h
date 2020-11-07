#ifndef MAKE_DLL_H
 #define MAKE_DLL_H
 #ifdef MAKEDLL
 #  define KINJO_API __declspec(dllexport)
 #else
  #ifdef TESTGAME
  #  define KINJO_API 
  #else
  #  define KINJO_API __declspec(dllimport)
  #endif
 #endif
#endif

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC

#ifndef LEAK_CHECK_H
#define LEAK_CHECK_H

#ifdef _WIN32
#include <cstdlib>
#include <crtdbg.h>
#define neww new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
// allocations to be of _CLIENT_BLOCK type
#else
#define neww new
#endif

#endif

#else
	#define neww new
#endif