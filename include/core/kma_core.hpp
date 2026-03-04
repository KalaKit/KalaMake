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

	constexpr size_t MIN_NAME_LENGTH = 1;
	constexpr size_t MAX_NAME_LENGTH = 20;

	//default build directory path relative to kmaPath if buildpath is not added or filled
	static const path defaultBuildPath = "build";
	//default object directory path relative to kmaPath if objpath is not added or filled
	static const path defaultObjPath = "build/obj";

	enum class TargetState : u8
	{
		S_INVALID = 0,

		S_COMPILE = 1,
		S_GENERATE = 2
	};

	enum class SolutionType : u8
	{
		S_INVALID = 0u,

		//generates ninja solution files
		S_NINJA = 1u,
		//generates visual studio solution files
		S_VS = 2u,
		//generates visual studio code files
		S_VSCODE = 3u
	};

	enum class Version : u8
	{
		V_INVALID = 0u,

		V_1_0 = 1u
	};

	//Allowed categories that can be added to any kmake file
	enum class CategoryType : u8
	{
		C_INVALID = 0u,

		//required version category
		C_VERSION = 1u,

		//optional include paths category
		C_INCLUDE = 2u,

		//required global fields category
		C_GLOBAL = 3u,

		//optional N amount of profile categories with custom names
		C_PROFILE = 4u,

		//optional post-build commands
		C_POST_BUILD = 5u
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
		//what is the target platform of the binary,
		//supports zig, clang, clang++, gcc and g++
		T_TARGET_TYPE = 4u,

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
		//what flags will be passed to the compiler stage, optional
		T_COMPILE_FLAGS = 13u,
		//what flags will be passed to the link stage, optional
		T_LINK_FLAGS = 14u,
		//what kalamake-specific flags will trigger extra actions
		T_CUSTOM_FLAGS = 15u,

		//where a file or folder is moved
		T_MOVE = 16u,
		//where a file or folder is copied
		T_COPY = 17u,
		//where a file or folder is copied and overridden if it already exists
		T_FORCECOPY = 18u,
		//where a new folder is created
		T_CREATE_DIR = 19u,
		//where a file or folder is deleted
		T_DELETE = 20u,
		//what a file or folder will be renamed to
		T_RENAME = 21u
	};

	//Allowed binary types that can be added to the binarytype field
	enum class BinaryType : u8
	{
		B_INVALID = 0u,

		//creates a runnable executable
		B_EXECUTABLE = 1u,

		//creates a linkable .lib on MSVC,
		//creates a linkable .a on GNU
		B_STATIC = 2u,

		//creates a .dll and a linkable .lib on MSVC,
		//creates a .so on GNU, same as runtime-only
		B_SHARED = 3u
	};

	//Allowed compiler types that can be added to the compiler field
	enum class CompilerType : u8
	{
		C_INVALID = 0u,

		//windows + linux, target-specific flags
		C_ZIG = 1u,

		//windows only, MSVC-style flags
		C_CLANG_CL = 2u,
		//windows only, MSVC-style flags
		C_CL = 3u,

		//windows + linux, GNU flags, defaults to C
		C_CLANG = 4u,
		//windows + linux, GNU flags, defaults to C++
		C_CLANGPP = 5u,
		//linux, GNU flags, defaults to C
		C_GCC = 6u,
		//linux, GNU flags, defaults to C++
		C_GPP = 7u
	};

	//Allowed standard types that can be added to the standard field
	enum class StandardType : u8
	{
		S_INVALID = 0u,

		C_89 = 1u,
		C_99 = 2u,
		C_11 = 3u,
		C_17 = 4u,
		C_23 = 5u,
		C_LATEST = 6u,

		CPP_98 = 7u,
		CPP_03 = 8u,
		CPP_11 = 9u,
		CPP_14 = 10u,
		CPP_17 = 11u,
		CPP_20 = 12u,
		CPP_23 = 13u,
		CPP_26 = 14u,
		CPP_LATEST = 15u
	};

	//Allowed target types that can be added to the targettype field
	enum class TargetType : u8
	{
		T_INVALID = 0u,

		T_WINDOWS = 1u,
		T_LINUX = 2u
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

		//will not generate object files for object-compatible languages - compiles and links directly
		F_NO_OBJ = 2u,

		//fails the build if the compiler cannot support the requested standard, ignored on GNU
		F_STANDARD_REQUIRED = 3u,

		//treats all warnings as errors
		F_WARNINGS_AS_ERRORS = 4u,

		//uses msvc instead of the default gnu for cross-compiling linux binary to windows binary,
		//not usable for msvc compilers, not usable outside of linux and c/c++
		F_USE_CLANG_ZIG_MSVC = 5u
	};
	
	struct ProfileData
	{
		//what is the name of this profile
		string profileName{};

		//what is the target type of the binary, required
		BinaryType binaryType{};
		//which compiler is used to compile this binary source code, required
		CompilerType compiler{};
		//which language standard is used to compile this source code,
		//only for C, C++, C#, Rust and Java, required for supported standards
		StandardType standard{};
		//what is the target platform of the binary,
		//supports zig, clang, clang++, gcc and g++
		TargetType targetType{};

		//what is the target type of the binary, required
		string binaryName{};
		//which build type is the binary, required
		BuildType buildType{};
		//where is the binary built to, required
		path buildPath{};
		//where are the source code files of the binary located, required
		vector<path> sources{};
		//where are the header files of the binary located,
		//only for C and C++, optional
		vector<path> headers{};
		//what links will be added to the binary,
		//only for C, C++ and Rust, optional
		vector<path> links{};
		//what warning level will compilation and linking use, defaults to 'none'
		WarningLevel warningLevel;
		//what defines will be added to the binary,
		//only for C, C++ and Rust, optional
		vector<string> defines{};
		//what flags will be passed to the compiler stage, optional
		vector<string> compileFlags{};
		//what flags will be passed to the link stage, optional
		vector<string> linkFlags{};
		//what kalamake-specific flags will trigger extra actions, optional
		vector<CustomFlag> customFlags{};
	};

	struct IncludeData
	{
		string name{};
		path value{};
	};
	
	struct PostBuildAction
	{
		//what build action will be done
		FieldType buildAction{};

		//from where
		path origin{};
		//to where, unused for delete and createdir
		path target{};
	};

	struct GlobalData
	{
		//final mixed data from global and/or target user profile
		ProfileData targetProfile{};

		//what includes are included in this kalamake project
		vector<IncludeData> includes{};
		//what actions will be done after the compilation is complete
		vector<PostBuildAction> postBuildActions{};
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
			const vector<string>& lines,
			SolutionType solutionType);

		static const unordered_map<SolutionType, string_view, EnumHash<SolutionType>>& GetSolutionTypes();
		static const unordered_map<Version,      string_view, EnumHash<Version>>&      GetVersions();
		static const unordered_map<CategoryType, string_view, EnumHash<CategoryType>>& GetCategoryTypes();
		static const unordered_map<FieldType,    string_view, EnumHash<FieldType>>&    GetFieldTypes();

		static const unordered_map<BinaryType,   string_view, EnumHash<BinaryType>>&   GetBinaryTypes();
		static const unordered_map<CompilerType, string_view, EnumHash<CompilerType>>& GetCompilerTypes();
		static const unordered_map<StandardType, string_view, EnumHash<StandardType>>& GetStandardTypes();
		static const unordered_map<TargetType,   string_view, EnumHash<TargetType>>&   GetTargetTypes();
		static const unordered_map<BuildType,    string_view, EnumHash<BuildType>>&    GetBuildTypes();
		static const unordered_map<WarningLevel, string_view, EnumHash<WarningLevel>>& GetWarningLevels();
		static const unordered_map<CustomFlag,   string_view, EnumHash<CustomFlag>>&   GetCustomFlags();

		static void CloseOnError(
			string_view target,
			string_view message);
	};
}