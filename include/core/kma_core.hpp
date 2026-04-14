//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>

#include "core_utils.hpp"

namespace KalaMake::Core
{
	using KalaHeaders::KalaCore::EnumHash;

	using std::vector;
	using std::string;
	using std::string_view;
	using std::filesystem::path;
	using std::unordered_map;

	using u8 = uint8_t;
	using u16 = uint16_t;

	enum class StartType : u8
	{
		S_INVALID = 0u,
		S_COMPILE = 1u,
		S_CLEAN = 2u,
		S_LIST_PROFILES = 3u
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

		//optional references category
		C_REFERENCES = 2u,

		//required global fields category
		C_GLOBAL = 3u,

		//optional N amount of profile categories with custom names
		C_PROFILE = 4u
	};

	//Allowed field types that can be added to global and profile categories
	enum class FieldType : u8
	{
		T_INVALID = 0u,

		//what is the target type of the binary,
		//supported by all languages
		T_BINARY_TYPE = 1u,
		//which compiler launcher runs before the compiler,
		//only for C and C++
		T_COMPILER_LAUNCHER = 2u,
		//which compiler is used to compile this binary source code,
		//supported by all languages
		T_COMPILER = 3u,
		//which language standard is used to compile this source code,
		//only for C, C++, Java and Rust
		T_STANDARD = 4u,
		//what is the target platform of the binary,
		//only for C, C++, Zig and Rust
		T_TARGET_TYPE = 5u,
		//how many jobs are allowed to do in parallel for compilation,
		//only for C and C++
		T_JOBS = 6u,

		//what is the name of the the binary,
		//supported by all languages
		T_BINARY_NAME = 7u,
		//which build type is the binary,
		//only for C, C++, Java, Zig and Rust
		T_BUILD_TYPE = 8u,
		//where is the binary built to,
		//supported by all languages
		T_BUILD_PATH = 9u,
		//where are the source code files of the binary located,
		//supported by all languages
		T_SOURCES = 10u,
		//where are the header files of the binary located,
		//only for C and C++
		T_HEADERS = 11u,
		//what links will be added to the binary,
		//only for C, C++ and Java
		T_LINKS = 12u,
		//what warning level will compilation and linking use, defaults to 'none',
		//only for C and C++
		T_WARNING_LEVEL = 13u,
		//what defines will be added to the binary,
		//only for C, C++, Java (modules) and Rust (--cfg)
		T_DEFINES = 14u,
		//what flags will be passed to the compiler stage,
		//supported by all languages
		T_COMPILE_FLAGS = 15u,
		//what flags will be passed to the link stage,
		//only for C and C++
		T_LINK_FLAGS = 16u,
		//what kalamake-specific flags will trigger extra actions,
		//supported by all languages
		T_CUSTOM_FLAGS = 17u,

		//pre-build action field, can add as many as you want,
		//supported by all languages
		T_PRE_BUILD_ACTION = 18u,

		//post-build action field, can add as many as you want,
		//supported by all languages
		T_POST_BUILD_ACTION = 19u
	};

	//Allowed binary types that can be added to the binarytype field
	enum class BinaryType : u8
	{
		B_INVALID = 0u,

		//creates a runnable executable
		B_EXECUTABLE = 1u,

		//creates a static library,
		//not used in Java and Python
		B_STATIC = 2u,

		//creates a shared library,
		//not used in Java and Python
		B_SHARED = 3u
	};

	//Specifies a compiler launcher program that runs before the compiler,
	//only for C and C++
	enum class CompilerLauncherType : u8
	{
		C_INVALID = 0u,

		//reuses previous compile results
		C_CCACHE = 1u,
		//same as ccache but supports shared cache servers
		C_SCCACHE = 2u,
		//sends compile jobs to other servers
		C_DISTCC = 3u,
		//same as distcc but ships compiler toolchain too
		C_ICECC = 4u
	};

	//Allowed compiler types that can be added to the compiler field
	enum class CompilerType : u8
	{
		C_INVALID = 0u,

		//
		// C/C++
		//

		//C, C++ and Zig language, windows + linux, target-specific flags
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
		C_GPP = 7u,

		//
		// OTHERS
		//

		//general Java compiler
		C_JAVA = 8u,

		//general python compiler
		C_PYTHON = 9u,

		//general rust compiler
		C_RUST = 10u
	};

	//Allowed standard types that can be added to the standard field,
	//only for C, C++, Java and Rust
	enum class StandardType : u8
	{
		S_INVALID = 0u,

		//
		// C/C++
		//

		C_89 = 1u,
		C_99 = 2u,
		C_11 = 3u,
		C_17 = 4u,
		C_23 = 5u,

		CPP_14 = 6u,
		CPP_17 = 7u,
		CPP_20 = 8u,
		CPP_23 = 9u,
		CPP_26 = 10u,

		//
		// JAVA
		//

		JAVA_8  = 11u,
		JAVA_9  = 12u,
		JAVA_10 = 13u,
		JAVA_11 = 14u,
		JAVA_12 = 15u,
		JAVA_13 = 16u,
		JAVA_14 = 17u,
		JAVA_15 = 18u,
		JAVA_16 = 19u,
		JAVA_17 = 20u,
		JAVA_18 = 21u,
		JAVA_19 = 22u,
		JAVA_20 = 23u,
		JAVA_21 = 24u,
		JAVA_22 = 25u,
		JAVA_23 = 26u,
		JAVA_24 = 27u,
		JAVA_25 = 28u,
		JAVA_26 = 29u,

		//
		// RUST
		//

		RUST_15 = 30u,
		RUST_18 = 31u,
		RUST_21 = 32u,
		RUST_24 = 33u
	};

	//Allowed target types that can be added to the targettype field,
	//only for C, C++, Zig and Rust
	enum class TargetType : u8
	{
		T_INVALID = 0u,

		//build a linux target on windows or linux (glibc)
		T_LINUX_GNU = 1u,

		//build a linux target on windows or linux (musl)
		T_LINUX_MUSL = 2u,

		//build a windows target on linux or windows (mingw)
		T_WINDOWS_GNU = 3u
	};

	//Allowed build types that can be added to the buildtype field,
	//only for C, C++, Java and Rust
	enum class BuildType : u8
	{
		B_INVALID = 0u,

		//creates a debug binary
		B_DEBUG = 1u,

		//creates a release binary
		B_RELEASE = 2u,

		//creates a release binary with debug flag,
		//same as release on Java
		B_RELDEBUG = 3u,

		//creates a release binary with smallest size flags,
		//same as release on Java
		B_MINSIZEREL = 4u
	};

	//Allowed warning levels that can be added to the warninglevel field,
	//only for C and C++
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

	//Allowed custom flags that can be added to the customflags field.
	//TODO: add vs sln file support here as well
	enum class CustomFlag : u8
	{
		F_INVALID = 0u,

		//creates compile_commands.json in the same directory as the kmake file
		F_EXPORT_COMPILE_COMMANDS = 1u,

		//creates .vscode folder with launch.json and tasks.json inside it
		F_EXPORT_VSCODE_SLN = 2u,

		//treats all warnings as errors,
		//only for C and C++
		F_WARNINGS_AS_ERRORS = 3u,

		//embed the C/C++ runtime into the binary,
		//dynamic runtime is enabled by default unless this is added,
		//only for C and C++, not used in linux
		F_MSVC_STATIC_RUNTIME = 4u,

		//package the created jar file into an executable,
		//only for Java
		F_PACKAGE_JAR = 5u,

		//attaches the jpackage-made exe to windows console,
		//only for Java
		F_JAVA_WIN_CONSOLE = 6u,

		//creates .classpath file in project root like compile_commans.json,
		//only for Java
		F_EXPORT_JAVA_SLN = 7u,

		//pyinstaller bundles everything into a single exe, all files are extracted at each run,
		//otherwise it creates a dir with all content with faster startup.
		//only for Python
		F_PYTHON_ONE_FILE = 8u
	};
	
	struct ProfileData
	{
		//what is the name of this profile
		string profileName{};

		//what is the target type of the binary,
		//supported by all languages
		BinaryType binaryType{};
		//which compiler launcher runs before the compiler,
		//only for C and C++
		CompilerLauncherType compilerLauncher{};
		//which compiler is used to compile this binary source code,
		//supported by all languages
		CompilerType compiler{};
		//which language standard is used to compile this source code,
		//only for C, C++, Java and Zig
		StandardType standard{};
		//what is the target platform of the binary,
		//only for C, C++, Zig and Rust
		TargetType targetType{};
		//how many parallel compilation jobs are allowed,
		//only for C and C++
		u16 jobs{};

		//what is the target type of the binary,
		//supported by all languages
		string binaryName{};
		//which build type is the binary,
		//only for C, C++, Java and Rust
		BuildType buildType{};
		//where is the binary built to,
		//supported by all languages
		path buildPath{};
		//where are the source code files of the binary located,
		//supported by all languages
		vector<path> sources{};
		//where are the header files of the binary located,
		//only for C and C++
		vector<path> headers{};
		//what links will be added to the binary,
		//only for C, C++ and Java
		vector<path> links{};
		//what warning level will compilation and linking use, defaults to 'none',
		//only for C and C++
		WarningLevel warningLevel;
		//what defines will be added to the binary,
		//only for C, C++, Java (modules) and Rust (--cfg)
		vector<string> defines{};
		//what flags will be passed to the compiler stage,
		//supported by all languages
		vector<string> compileFlags{};
		//what flags will be passed to the link stage,
		//only for C and C++
		vector<string> linkFlags{};
		//what kalamake-specific flags will trigger extra actions,
		//supported by all languages
		vector<CustomFlag> customFlags{};

		//what actions will be done before generation, compilation and linking starts
		vector<string> preBuildActions{};

		//what actions will be done after generation, compilation and linking is complete
		vector<string> postBuildActions{};
	};

	struct ReferenceData
	{
		string name{};
		string value{};
	};

	struct GlobalData
	{
		//which kmake file was used
		path projectFile{};

		//final mixed data from global and/or target user profile
		ProfileData targetProfile{};

		//what references are included in this kalamake project
		vector<ReferenceData> references{};
	};

	class KalaMakeCore
	{
	public:
		static void OpenFile(
			StartType type,
			const vector<string>& params);

		static const unordered_map<Version,      string_view, EnumHash<Version>>&      GetVersions();
		static const unordered_map<CategoryType, string_view, EnumHash<CategoryType>>& GetCategoryTypes();
		static const unordered_map<FieldType,    string_view, EnumHash<FieldType>>&    GetFieldTypes();

		static const unordered_map<BinaryType,           string_view, EnumHash<BinaryType>>&           GetBinaryTypes();
		static const unordered_map<CompilerLauncherType, string_view, EnumHash<CompilerLauncherType>>& GetCompilerLauncherTypes();
		static const unordered_map<CompilerType,         string_view, EnumHash<CompilerType>>&         GetCompilerTypes();
		static const unordered_map<StandardType,         string_view, EnumHash<StandardType>>&         GetStandardTypes();
		static const unordered_map<TargetType,           string_view, EnumHash<TargetType>>&           GetTargetTypes();
		static const unordered_map<BuildType,            string_view, EnumHash<BuildType>>&            GetBuildTypes();
		static const unordered_map<WarningLevel,         string_view, EnumHash<WarningLevel>>&         GetWarningLevels();
		static const unordered_map<CustomFlag,           string_view, EnumHash<CustomFlag>>&           GetCustomFlags();

		static void CloseOnError(
			string_view target,
			string_view message);
	};
}