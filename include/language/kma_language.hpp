//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#pragma once

#include <vector>
#include <string>

#include "core/kma_core.hpp"

namespace KalaMake::Language
{
	using KalaMake::Core::CompileData;
	using KalaMake::Core::BinaryType;
	using KalaMake::Core::WarningLevel;
	using KalaMake::Core::CustomFlag;

	using std::vector;
	using std::string;
	using std::string_view;

	using u8 = uint8_t;

	enum class CompileFlag : u8
	{
		C_INVALID = 0,

		//builds with ninja
		C_NINJA = 1,
		
		//skips generating obj files
		C_NO_OBJ = 2,

		//creates a static lib only
		C_LINK_ONLY = 3
	};

	enum class CompilerType : u8
	{
		C_INVALID = 0,

		//windows only, MSVC-style flags
		C_CLANG_CL = 1,
		//windows only, MSVC-style flags
		C_CL = 2,

		//windows + linux, GNU flags, defaults to C
		C_CLANG = 3,
		//windows + linux, GNU flags, defaults to C++
		C_CLANGPP = 4,
		//linux, GNU flags, defaults to C
		C_GCC = 5,
		//linux, GNU flags, defaults to C++
		C_GPP = 6
	};

	class LanguageCore
	{
	public:
		static bool IsMSVCCompiler(string_view value);
		static bool IsGNUCompiler(string_view value);

		virtual bool Compile(vector<CompileFlag> compileFlags = {}) = 0;

		virtual ~LanguageCore() = default;
	protected:
		CompileData compileData{};
	};
}