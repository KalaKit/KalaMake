//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>

#include "KalaHeaders/core_utils.hpp"

namespace KalaMake::Core
{
	using KalaHeaders::KalaCore::EnumHash;

	using std::vector;
	using std::string;
	using std::string_view;
	using std::filesystem::path;
	using std::unordered_map;

	using u8 = uint8_t;

	enum class TargetState : u8
	{
		S_INVALID = 0,

		S_COMPILE = 1,
		S_GENERATE = 2
	};

	constexpr size_t MIN_NAME_LENGTH = 1;
	constexpr size_t MAX_NAME_LENGTH = 20;

	//Allowed categories that can be added to any kmake file
	enum class CategoryType : u8
	{
		//required version category
		C_VERSION,

		//optional include paths category
		C_INCLUDE,

		//required global fields category
		C_GLOBAL,

		//optional N amount of profile categories with custom names
		C_PROFILE
	};

	//Allowed field types that can be added to global and profile categories
	enum class FieldType : u8
	{
		T_INVALID = 0u,

		//what is the target type of the binary
		T_BINARY_TYPE = 1u,
		//which compiler is used to compile this binary source code
		T_COMPILER = 2u,
		//which language standard is used to compile this source code,
		//only for C, C++, C#, Rust and Java
		T_STANDARD = 3u,
		//which profiles will be built
		T_TARGET_PROFILE = 4u,

		//what is the name of the the binary
		T_BINARY_NAME = 5u,
		//which build type is the binary
		T_BUILD_TYPE = 6u,
		//where is the binary built to
		T_BUILD_PATH = 7u,
		//where are the source code files of the binary located
		T_SOURCES = 8u,
		//where are the header files of the binary located,
		//only for C and C++
		T_HEADERS = 9u,
		//what links will be added to the binary,
		//only for C, C++ and Rust
		T_LINKS = 10u,
		//what warning level will compilation and linking use, defaults to 'none'
		T_WARNING_LEVEL = 11u,
		//what defines will be added to the binary,
		//only for C, C++ and Rust
		T_DEFINES = 12u,
		//what flags will be passed to the compiler
		T_FLAGS = 13u,
		//what kalamake-specific flags will trigger extra actions
		T_CUSTOM_FLAGS = 14u
	};

	//Allowed compiler types that can be added to the compiler field
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

	//Allowed standard types that can be added to the standard field
	enum class StandardType : u8
	{
		S_INVALID = 0,

		C_89 = 1,
		C_99 = 2,
		C_11 = 3,
		C_17 = 4,
		C_23 = 5,
		C_LATEST = 6,

		CPP_11 = 7,
		CPP_14 = 8,
		CPP_17 = 9,
		CPP_20 = 10,
		CPP_23 = 11,
		CPP_26 = 12,
		CPP_LATEST = 13
	};

	//Allowed build types that can be added to the buildtype field
	enum class BuildType : u8
	{
		B_INVALID = 0u,

		//creates a debug binary
		B_DEBUG = 1u,

		//creates a release binary
		B_RELEASE = 2u,

		//creates a release binary with debug flag
		B_RELDEBUG = 3u,

		//creates a release binary with smallest size flags
		B_MINSIZEREL = 4u
	};

	//Allowed binary types that can be added to the binarytype field
	enum class BinaryType : u8
	{
		B_INVALID = 0u,

		//creates a runnable executable
		B_EXECUTABLE = 1u,

		//creates a linkable .lib on MSVC,
		//creates a linkable .a on GNU
		B_LINK_ONLY = 2u,

		//creates a .dll on MSVC,
		//creates a .so on GNU
		B_RUNTIME_ONLY = 3u,

		//creates a .dll and a linkable .lib on MSVC,
		//creates a .so on GNU, same as runtime-only
		B_LINK_RUNTIME = 4u
	};

	//Allowed warning levels that can be added to the warninglevel field
	enum class WarningLevel : u8
	{
		W_INVALID = 0u,

		//no warnings
		W_NONE = 1u,

		//very basic warnings
		W_BASIC = 2u,

		//common, useful warnings, recommended default
		W_NORMAL = 3u,

		//strong warnings
		W_STRONG = 4u,

		//very strict, high signal warnings
		W_STRICT = 5u,

		//all warnings
		W_ALL = 6u
	};

	//Allowed custom flags that can be added to the customflags field
	enum class CustomFlag : u8
	{
		F_INVALID = 0u,

		//uses the multithreaded benefits of ninja for faster compilation
		F_USE_NINJA = 1u,

		//will not generate obj files for obj-compatible languages, compiles and links directly
		F_NO_OBJ = 2u,

		//fails the build if the compiler cannot support the requested standard, ignored on GNU
		F_STANDARD_REQUIRED = 3u,

		//treats all warnings as errors
		F_WARNINGS_AS_ERRORS = 4u,

		//used only for the --generate command in kalamake,
		//exports the compilation commands to your solution file type you selected
		F_EXPORT_COMPILE_COMMANDS = 5u
	};

	//Overridable per-profile field definitions for kalamake action types
	struct ProfileData
	{
		//what is the target type of the binary
		string overrideBinaryName{};
		//which build type is the binary
		BuildType overrideBuildType{};
		//where is the binary built to
		string overrideBuildPath{};
		//where are the source code files of the binary located
		vector<string> overrideSources{};
		//where are the header files of the binary located,
		//only for C and C++
		vector<string> overrideHeaders{};
		//what links will be added to the binary,
		//only for C, C++ and Rust
		vector<string> overrideLinks{};
		//what warning level will compilation and linking use, defaults to 'none'
		WarningLevel overrideWarningLevel;
		//what defines will be added to the binary,
		//only for C, C++ and Rust
		vector<string> overrideDefines{};
		//what flags will be passed to the compiler
		vector<string> overrideFlags{};
		//what kalamake-specific flags will trigger extra actions
		vector<CustomFlag> overrideCustomFlags{};
	};

	//Default field definitions for all kalamake action types
	struct GlobalData
	{
		//cannot be overridden, global only

		//what is the target type of the binary
		BinaryType binaryType{};
		//which compiler is used to compile this binary source code
		CompilerType compiler{};
		//which language standard is used to compile this source code,
		//only for C, C++, C#, Rust and Java
		StandardType standard{};
		//which profiles will be built
		string targetProfile{};

		//overridable

		//what is the name of the the binary
		string binaryName{};
		//which build type is the binary
		BuildType buildType{};
		//where is the binary built to
		string buildPath{};
		//where are the source code files of the binary located
		vector<string> sources{};
		//where are the header files of the binary located,
		//only for C and C++
		vector<string> headers{};
		//what links will be added to the binary,
		//only for C, C++ and Rust
		vector<string> links{};
		//what warning level will compilation and linking use, defaults to 'none'
		WarningLevel warningLevel;
		//what defines will be added to the binary,
		//only for C, C++ and Rust
		vector<string> defines{};
		//what flags will be passed to the compiler
		vector<string> flags{};
		//what kalamake-specific flags will trigger extra actions
		vector<CustomFlag> customFlags{};

		//what profiles are included in this kalamake project
		vector<ProfileData> projects{};
		//what includes are included in this kalamake project
		vector<string> includes{};
	};

	class KalaMakeCore
	{
	public:
		static void OpenFile(
			const vector<string>& params, 
			TargetState state);

		static void Compile(
			const path& filePath,
			const vector<string>& lines);

		static void Generate(
			const path& filePath,
			const vector<string>& lines);

		static bool IsCategoryType(string_view value);

		static bool IsFieldType(string_view value);

		static bool IsCStandard(string_view value);
		static bool IsCPPStandard(string_view value);

		static bool IsC_CPPCompiler(string_view value);
		static bool IsMSVCCompiler(string_view value);
		static bool IsGNUCompiler(string_view value);

		static bool IsBinaryType(string_view value);

		static bool IsWarningLevel(string_view value);

		static bool IsCustomFlag(string_view value);

		static const unordered_map<FieldType, string_view, EnumHash<FieldType>>& GetFieldTypes();
		static const unordered_map<CompilerType, string_view, EnumHash<CompilerType>>& GetCompilerTypes();
		static const unordered_map<StandardType, string_view, EnumHash<StandardType>>& GetStandardTypes();
		static const unordered_map<BinaryType, string_view, EnumHash<BinaryType>>& GetBinaryTypes();
		static const unordered_map<WarningLevel, string_view, EnumHash<WarningLevel>>& GetWarningLevels();
		static const unordered_map<CustomFlag, string_view, EnumHash<CustomFlag>>& GetCustomFlags();

		static void PrintError(string_view message);
	};
}