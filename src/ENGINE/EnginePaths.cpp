// Kept in its own file: <windows.h> is only safe to include where nothing else
// from the engine is (its macros collide with engine names).
#include "EnginePaths.h"
#include <filesystem>
#include <cstdlib>
#include <iostream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace
{
	std::string AsDir(const fs::path& p)
	{
		std::string s = p.generic_string();
		if (!s.empty() && s.back() != '/')
			s += '/';
		return s;
	}

	bool IsDir(const fs::path& p)
	{
		std::error_code ec;
		return fs::is_directory(p, ec);
	}

	std::string FindEngineShaderDir()
	{
		if (const char* env = std::getenv("KINJO_ENGINE_SHADERS"))
		{
			if (IsDir(env))
				return AsDir(env);
			std::cout << "WARNING: KINJO_ENGINE_SHADERS=" << env << " is not a directory" << std::endl;
		}

#ifdef __EMSCRIPTEN__
		return "engine/shaders/";
#else
		// <root>/src/ENGINE/EnginePaths.cpp -> <root>/shaders
		const fs::path source = fs::path(__FILE__).parent_path().parent_path().parent_path() / "shaders";
		if (IsDir(source))
			return AsDir(source);

#ifdef _WIN32
		HMODULE module = nullptr;
		if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCSTR>(&FindEngineShaderDir), &module))
		{
			char buffer[MAX_PATH] = { 0 };
			const DWORD length = GetModuleFileNameA(module, buffer, MAX_PATH);
			if (length > 0 && length < MAX_PATH)
			{
				const fs::path besideModule = fs::path(buffer).parent_path() / "shaders";
				if (IsDir(besideModule))
					return AsDir(besideModule);
			}
		}
#endif
		return "";
#endif
	}
}

const std::string& EngineShaderDir()
{
	static const std::string dir = []()
	{
		std::string found = FindEngineShaderDir();
		if (found.empty())
			std::cout << "WARNING: engine shader folder not found; only the game's data/shaders will be used" << std::endl;
		else
			std::cout << "Engine shaders: " << found << std::endl;
		return found;
	}();
	return dir;
}
