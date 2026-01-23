
#ifndef LEAK_CHECK_H
#define LEAK_CHECK_H

// Suppress C++20 deprecation warnings (e.g., <ciso646> in nlohmann/json)
#ifndef _SILENCE_CXX20_CISO646_REMOVED_WARNING
#define _SILENCE_CXX20_CISO646_REMOVED_WARNING
#endif

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


#ifdef EMSCRIPTEN
// Emscripten: Filesystem operations should be guarded with #ifndef EMSCRIPTEN
// Use a minimal stub namespace - real file ops use Emscripten's virtual FS
#define __stdcall
#include <string>
namespace fs {
    class path {
    public:
        std::string p;
        path() {}
        path(const std::string& s) : p(s) {}
        path(const char* s) : p(s) {}
        std::string string() const { return p; }
        path operator/(const path& other) const { return path(p + "/" + other.p); }
        path operator/(const std::string& other) const { return path(p + "/" + other); }
    };
    inline path current_path() { return path("."); }
    inline bool exists(const path&) { return false; }
    inline bool is_directory(const path&) { return false; }
    inline bool is_regular_file(const path&) { return false; }
    inline bool create_directories(const path&) { return false; }
}
#elif !defined(_WIN32)
// Non-Windows native: Use ghc filesystem
#define __stdcall
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#else
// Windows native: Use standard filesystem
#include <filesystem>
namespace fs = std::filesystem;
#endif


#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC

#ifdef _WIN32
#include <cstdlib>
#include <crtdbg.h>
#define new new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#else
#define new new
#endif

#else

#define new new

#endif

#endif