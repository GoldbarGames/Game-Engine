
#ifndef LEAK_CHECK_H
#define LEAK_CHECK_H


#if defined(_MSC_VER)
//  Microsoft 
#define DLL_EXPORT __declspec(dllexport)
#define DLL_IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
//  GCC
#define DLL_EXPORT __attribute__((visibility("default")))
#define DLL_IMPORT
#else
//  do nothing and hope for the best?
#define DLL_EXPORT
#define DLL_IMPORT
#pragma warning Unknown dynamic link import/export semantics.
#endif

#if MAKEDLL
#   define KINJO_API DLL_EXPORT
#else
#   define KINJO_API DLL_IMPORT
#endif


#ifndef _WIN32
#define __stdcall
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif


#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC

#ifdef _WIN32
#include <cstdlib>
#include <crtdbg.h>
#define neww new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#else
#define neww new
#endif

#else

#define neww new

#endif

#endif