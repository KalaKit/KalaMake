//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#ifndef scast
	#define scast static_cast
#endif

#include <filesystem>
#include <string>

using std::filesystem::path;
using std::string_view;

#include "language/kma_language.hpp"

//default build directory path relative to kmaPath if buildpath is not added or filled
static path defaultBuildPath = "build";
//default object directory path relative to kmaPath if objpath is not added or filled
static path defaultObjPath = "build/obj";

constexpr string_view cl_ide_bat_2026 = "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat";
constexpr string_view cl_build_bat_2026 = "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat";
constexpr string_view cl_ide_bat_2022 = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat";
constexpr string_view cl_build_bat_2022 = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat";

static path foundCLPath{};

namespace KalaMake::Language
{
	static const unordered_map<FieldType, string_view, EnumHash<FieldType>> fieldTypes =
	{
		{ FieldType::T_BINARY_TYPE,    "binarytype" },
		{ FieldType::T_COMPILER,       "compiler" },
		{ FieldType::T_STANDARD,       "standard" },
		{ FieldType::T_TARGET_PROFILE, "targetprofile" },

		{ FieldType::T_BINARY_NAME,   "binaryname" },
		{ FieldType::T_BUILD_TYPE,    "buildtype" },
		{ FieldType::T_BUILD_PATH,    "buildpath" },
		{ FieldType::T_SOURCES,       "sources" },
		{ FieldType::T_HEADERS,       "headers" },
		{ FieldType::T_LINKS,         "links" },
		{ FieldType::T_WARNING_LEVEL, "warninglevel" },
		{ FieldType::T_DEFINES,       "defines" },
		{ FieldType::T_FLAGS,         "flags" },
		{ FieldType::T_CUSTOM_FLAGS,  "customflags" }
	};

	static const unordered_map<CompilerType, string_view, EnumHash<CompilerType>> compilerTypes =
	{
		{ CompilerType::C_CLANG_CL, "clang-cl" },
		{ CompilerType::C_CL,       "cl" },

		{ CompilerType::C_CLANG,    "clang" },
		{ CompilerType::C_CLANGPP,  "clang++" },
		{ CompilerType::C_GCC,      "gcc" },
		{ CompilerType::C_GPP,      "g++" }
	};

	static const unordered_map<StandardType, string_view, EnumHash<StandardType>> standardTypes =
	{
		{ StandardType::C_89,     "c89" },
		{ StandardType::C_99,     "c99" },
		{ StandardType::C_11,     "c11" },
		{ StandardType::C_17,     "c17" },
		{ StandardType::C_23,     "c23" },
		{ StandardType::C_LATEST, "clatest" },

		{ StandardType::CPP_11,     "c++11" },
		{ StandardType::CPP_14,     "c++14" },
		{ StandardType::CPP_17,     "c++17" },
		{ StandardType::CPP_20,     "c++20" },
		{ StandardType::CPP_23,     "c++23" },
		{ StandardType::CPP_26,     "c++26" },
		{ StandardType::CPP_LATEST, "c++latest" }
	};

	static const unordered_map<BinaryType, string_view, EnumHash<BinaryType>> binaryTypes =
	{
		{ BinaryType::B_EXECUTABLE,   "executable" },
		{ BinaryType::B_LINK_ONLY,    "link-only" },
		{ BinaryType::B_RUNTIME_ONLY, "runtime-only" },
		{ BinaryType::B_LINK_RUNTIME, "link-runtime" }
	};

	//Same warning types are used for both MSVC and GNU,
	//their true meanings change depending on which OS is used
	static const unordered_map<WarningLevel, string_view, EnumHash<WarningLevel>> warningLevels =
	{
		{ WarningLevel::W_NONE,   "none" },
		{ WarningLevel::W_BASIC,  "basic" },
		{ WarningLevel::W_NORMAL, "normal" },
		{ WarningLevel::W_STRONG, "strong" },
		{ WarningLevel::W_STRICT, "strict" },
		{ WarningLevel::W_ALL,    "all" }
	};

	//Same warning types are used for both MSVC and GNU,
	//their true meanings change depending on which OS is used
	static const unordered_map<CustomFlag, string_view, EnumHash<CustomFlag>> customFlags =
	{
		{ CustomFlag::F_USE_NINJA,               "use-ninja" },
		{ CustomFlag::F_NO_OBJ,                  "no-obj" },
		{ CustomFlag::F_STANDARD_REQUIRED,       "standard-required" },
		{ CustomFlag::F_WARNINGS_AS_ERRORS,      "warnings-as-errors" },
		{ CustomFlag::F_EXPORT_COMPILE_COMMANDS, "export-compile-commands" }
	};

	bool LanguageCore::IsFieldType(string_view value)
	{
		return ContainsValue(fieldTypes, value);
	}

	bool LanguageCore::IsCStandard(string_view value)
	{
		StandardType type{};

		return StringToEnum(value, standardTypes, type)
			&& (type == StandardType::C_89
			|| type == StandardType::C_99
			|| type == StandardType::C_11
			|| type == StandardType::C_17
			|| type == StandardType::C_23
			|| type == StandardType::C_LATEST);
	}
	bool LanguageCore::IsCPPStandard(string_view value)
	{
		StandardType type{};

		return StringToEnum(value, standardTypes, type)
			&& (type == StandardType::CPP_11
			|| type == StandardType::CPP_14
			|| type == StandardType::CPP_17
			|| type == StandardType::CPP_20
			|| type == StandardType::CPP_23
			|| type == StandardType::CPP_26
			|| type == StandardType::CPP_LATEST);
	}

	bool LanguageCore::IsC_CPPCompiler(string_view value)
	{
		CompilerType type{};

		return StringToEnum(value, compilerTypes, type)
			&& (type == CompilerType::C_CLANG_CL
			|| type == CompilerType::C_CL
			|| type == CompilerType::C_CLANG
			|| type == CompilerType::C_CLANGPP
			|| type == CompilerType::C_GCC
			|| type == CompilerType::C_GPP);
	}
	bool LanguageCore::IsMSVCCompiler(string_view value)
	{
		CompilerType type{};

		return StringToEnum(value, compilerTypes, type)
			&& (type == CompilerType::C_CLANG_CL
			|| type == CompilerType::C_CL);
	}
	bool LanguageCore::IsGNUCompiler(string_view value)
	{
		CompilerType type{};

		return StringToEnum(value, compilerTypes, type)
			&& (type == CompilerType::C_CLANG
			|| type == CompilerType::C_CLANGPP
			|| type == CompilerType::C_GCC
			|| type == CompilerType::C_GPP);
	}

	bool LanguageCore::IsBinaryType(string_view value)
	{
		return ContainsValue(binaryTypes, value);
	}

	bool LanguageCore::IsWarningLevel(string_view value)
	{
		return ContainsValue(warningLevels, value);
	}

	bool LanguageCore::IsCustomFlag(string_view value)
	{
		return ContainsValue(customFlags, value);
	}

	const unordered_map<FieldType, string_view, EnumHash<FieldType>>& LanguageCore::GetFieldTypes() { return fieldTypes; }
	const unordered_map<CompilerType, string_view, EnumHash<CompilerType>>& LanguageCore::GetCompilerTypes() { return compilerTypes; }
	const unordered_map<StandardType, string_view, EnumHash<StandardType>>& LanguageCore::GetStandardTypes() { return standardTypes; }
	const unordered_map<BinaryType, string_view, EnumHash<BinaryType>>& LanguageCore::GetBinaryTypes() { return binaryTypes; }
	const unordered_map<WarningLevel, string_view, EnumHash<WarningLevel>>& LanguageCore::GetWarningLevels() { return warningLevels; }
	const unordered_map<CustomFlag, string_view, EnumHash<CustomFlag>>& LanguageCore::GetCustomFlags() { return customFlags; }
}