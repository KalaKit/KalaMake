//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#pragma once

#include <vector>
#include <string>

#include "core/kma_core.hpp"

#ifndef scast
	#define scast static_cast
#endif

namespace KalaMake::Language
{
	using KalaMake::Core::BinaryType;
	using KalaMake::Core::WarningLevel;
	using KalaMake::Core::CustomFlag;

	using std::vector;
	using std::string;
	using std::string_view;

	using u8 = uint8_t;

	constexpr string_view cl_ide_bat_2026 = "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat";
	constexpr string_view cl_build_bat_2026 = "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat";
	constexpr string_view cl_ide_bat_2022 = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat";
	constexpr string_view cl_build_bat_2022 = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat";

	struct CompileData
	{
		string name{};
		BinaryType type{};
		string standard{};
		string compiler{};
		vector<string> sources{};

		string buildPath{};
		string objPath{};
		vector<string> headers{};
		vector<string> links{};
		vector<string> debuglinks{};

		WarningLevel warningLevel;
		vector<string> defines{};
		vector<string> extensions{};

		vector<string> flags{};
		vector<string> debugflags{};
		vector<CustomFlag> customFlags{};
	};

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
		CompileData data{};
	};
}