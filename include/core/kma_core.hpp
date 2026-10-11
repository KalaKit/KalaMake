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
		S_VALIDATE = 0,
		S_LIST_PROFILES = 1,
		S_CLEAN = 2,
		S_COMPILE = 3
	};

	enum class Version : u8
	{
		V_INVALID = 0,

		V_1_0 = 1,
		V_2_0 = 2
	};

	//Allowed categories that can be added to any kmake file
	enum class CategoryType : u8
	{
		C_INVALID = 0,

		//required version category
		C_VERSION = 1,
		//optional references category
		C_REFERENCES = 2,
		//required global fields category
		C_GLOBAL = 3,
		//optional N amount of profile categories with custom names
		C_PROFILE = 4
	};

	//Allowed field types that can be added to global and profile categories
	enum class FieldType : u8
	{
		T_INVALID = 0,

		//what is the target type of the binary,
		//supported by all languages
		T_BINARY_TYPE = 1,
		//which compiler launcher runs before the compiler,
		//only for C and C++
		T_COMPILER_LAUNCHER = 2,
		//which compiler is used to compile this binary source code,
		//supported by all languages
		T_COMPILER = 3,
		//which language standard is used to compile this source code,
		//only for C, C++, Java and Rust
		T_STANDARD = 4,
		//what is the target platform of the binary,
		//only for C, C++, Zig and Rust
		T_TARGET_TYPE = 5,
		//how many jobs are allowed to do in parallel for compilation,
		//only for C and C++
		T_JOBS = 6,

		//what is the name of the the binary,
		//supported by all languages
		T_BINARY_NAME = 7,
		//which build type is the binary,
		//only for C, C++, Java, Zig and Rust
		T_BUILD_TYPE = 8,
		//where is the binary built to,
		//supported by all languages
		T_BUILD_PATH = 9,
		//where are the source code files of the binary located,
		//supported by all languages
		T_SOURCES = 10,
		//where are the header files of the binary located,
		//only for C and C++
		T_HEADERS = 11,
		//what links will be added to the binary,
		//only for C, C++, Java and Rust
		T_LINKS = 12,
		//what warning level will compilation and linking use, defaults to 'none',
		//only for C and C++
		T_WARNING_LEVEL = 13,
		//what defines will be added to the binary,
		//only for C, C++, Java (modules) and Rust (--cfg)
		T_DEFINES = 14,
		//what flags will be passed to the compiler stage,
		//supported by all languages
		T_COMPILE_FLAGS = 15,
		//what flags will be passed to the link stage,
		//only for C and C++
		T_LINK_FLAGS = 16,
		//what kalamake-specific flags will trigger extra actions,
		//supported by all languages
		T_CUSTOM_FLAGS = 17,

		//pre-build action field, can add as many as you want,
		//supported by all languages
		T_PRE_BUILD_ACTION = 18,
		//post-build action field, can add as many as you want,
		//supported by all languages
		T_POST_BUILD_ACTION = 19
	};

	//Allowed binary types that can be added to the binarytype field
	enum class BinaryType : u8
	{
		B_INVALID = 0,

		//creates a runnable executable
		B_EXECUTABLE = 1,
		//creates a static library,
		//not used in Java and Python
		B_STATIC = 2,
		//creates a shared library,
		//not used in Java and Python
		B_SHARED = 3
	};

	//Specifies a compiler launcher program that runs before the compiler,
	//only for C and C++
	enum class CompilerLauncherType : u8
	{
		C_INVALID = 0,

		//reuses previous compile results
		C_CCACHE = 1,
		//same as ccache but supports shared cache servers
		C_SCCACHE = 2,
		//sends compile jobs to other servers
		C_DISTCC = 3,
		//same as distcc but ships compiler toolchain too
		C_ICECC = 4
	};

	//Allowed compiler types that can be added to the compiler field
	enum class CompilerType : u8
	{
		C_INVALID = 0,

		//
		// C/C++
		//

		//C, C++ and Zig language, windows + linux, target-specific flags
		C_ZIG = 1,

		//windows only, MSVC-style flags
		C_CLANG_CL = 2,
		//windows only, MSVC-style flags
		C_CL = 3,

		//windows + linux, GNU flags, defaults to C
		C_CLANG = 4,
		//windows + linux, GNU flags, defaults to C++
		C_CLANGPP = 5,
		//linux, GNU flags, defaults to C
		C_GCC = 6,
		//linux, GNU flags, defaults to C++
		C_GPP = 7,

		//
		// OTHERS
		//

		//general Java compiler
		C_JAVA = 8,
		//general python compiler
		C_PYTHON = 9,
		//general rust compiler
		C_RUST = 10
	};

	//Allowed standard types that can be added to the standard field,
	//only for C, C++, Java and Rust
	enum class StandardType : u8
	{
		S_INVALID = 0,

		//
		// C/C++
		//

		C_89 = 1,
		C_99 = 2,
		C_11 = 3,
		C_17 = 4,
		C_23 = 5,

		CPP_14 = 6,
		CPP_17 = 7,
		CPP_20 = 8,
		CPP_23 = 9,
		CPP_26 = 10,

		//
		// JAVA
		//

		JAVA_8  = 11,
		JAVA_9  = 12,
		JAVA_10 = 13,
		JAVA_11 = 14,
		JAVA_12 = 15,
		JAVA_13 = 16,
		JAVA_14 = 17,
		JAVA_15 = 18,
		JAVA_16 = 19,
		JAVA_17 = 20,
		JAVA_18 = 21,
		JAVA_19 = 22,
		JAVA_20 = 23,
		JAVA_21 = 24,
		JAVA_22 = 25,
		JAVA_23 = 26,
		JAVA_24 = 27,
		JAVA_25 = 28,
		JAVA_26 = 29,

		//
		// RUST
		//

		RUST_15 = 30,
		RUST_18 = 31,
		RUST_21 = 32,
		RUST_24 = 33
	};

	//Allowed target types that can be added to the targettype field,
	//only for C, C++, Zig and Rust
	enum class TargetType : u8
	{
		T_INVALID = 0,

		//build a linux target on windows or linux (glibc)
		T_LINUX_GNU = 1,
		//build a linux target on windows or linux (musl)
		T_LINUX_MUSL = 2,
		//build a windows target on linux or windows (mingw)
		T_WINDOWS_GNU = 3,
		//build a windows target on linux or windows (msvc)
		T_WINDOWS_MSVC = 4
	};

	//Allowed build types that can be added to the buildtype field,
	//only for C, C++, Java and Rust
	enum class BuildType : u8
	{
		B_INVALID = 0,

		//creates a debug binary
		B_DEBUG = 1,
		//creates a release binary
		B_RELEASE = 2,
		//creates a release binary with debug flag,
		//same as release on Java
		B_RELDEBUG = 3,
		//creates a release binary with smallest size flags,
		//same as release on Java
		B_MINSIZEREL = 4
	};

	//Allowed warning levels that can be added to the warninglevel field,
	//only for C and C++
	enum class WarningLevel : u8
	{
		W_INVALID = 0,

		//no warnings
		W_NONE = 1,
		//very basic warnings
		W_BASIC = 2,
		//common, useful warnings, recommended default
		W_NORMAL = 3,
		//strong warnings
		W_STRONG = 4,
		//very strict, high signal warnings
		W_STRICT = 5,
		//all warnings
		W_ALL = 6
	};

	//Allowed custom flags that can be added to the customflags field.
	//TODO: add vs sln file support here as well
	enum class CustomFlag : u8
	{
		F_INVALID = 0,

		//creates compile_commands.json in the same directory as the kmake file
		F_EXPORT_COMPILE_COMMANDS = 1,
		//creates .vscode folder with launch.json and tasks.json inside it
		F_EXPORT_VSCODE_SLN = 2,

		//treats all warnings as errors,
		//only for C and C++
		F_WARNINGS_AS_ERRORS = 3,
		//embed the C/C++ runtime into the binary,
		//dynamic runtime is enabled by default unless this is added,
		//only for C and C++, not used in linux
		F_MSVC_STATIC_RUNTIME = 4,
		//creates a .pdb file for debug symbols or embeds the symbols directly to the exe in any build type,
		//only for C and C++
		F_GENERATE_SYMBOLS = 5,
		//does not show the console on Windows when launching an executable,
		//only for C and C++
		F_NO_CONSOLE = 6,

		//package the created jar file into an executable,
		//only for Java
		F_PACKAGE_JAR = 7,
		//attaches the jpackage-made exe to windows console,
		//only for Java
		F_JAVA_WIN_CONSOLE = 8,
		//creates .classpath file in project root like compile_commans.json,
		//only for Java
		F_EXPORT_JAVA_SLN = 9,

		//pyinstaller bundles everything into a single exe, all files are extracted at each run,
		//otherwise it creates a dir with all content with faster startup.
		//only for Python
		F_PYTHON_ONE_FILE = 10
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
		static bool IsInHeadlessMode();
		static void SetHeadlessModeState(bool newValue);

		static void OpenFile(
			StartType startType,
			const vector<string>& params);

		KNODISCARD
		static const unordered_map<Version,      string_view, EnumHash<Version>>&      GetVersions();
		KNODISCARD
		static const unordered_map<CategoryType, string_view, EnumHash<CategoryType>>& GetCategoryTypes();
		KNODISCARD
		static const unordered_map<FieldType,    string_view, EnumHash<FieldType>>&    GetFieldTypes();

		KNODISCARD
		static const unordered_map<BinaryType,           string_view, EnumHash<BinaryType>>&           GetBinaryTypes();
		KNODISCARD
		static const unordered_map<CompilerLauncherType, string_view, EnumHash<CompilerLauncherType>>& GetCompilerLauncherTypes();
		KNODISCARD
		static const unordered_map<CompilerType,         string_view, EnumHash<CompilerType>>&         GetCompilerTypes();
		KNODISCARD
		static const unordered_map<StandardType,         string_view, EnumHash<StandardType>>&         GetStandardTypes();
		KNODISCARD
		static const unordered_map<TargetType,           string_view, EnumHash<TargetType>>&           GetTargetTypes();
		KNODISCARD
		static const unordered_map<BuildType,            string_view, EnumHash<BuildType>>&            GetBuildTypes();
		KNODISCARD
		static const unordered_map<WarningLevel,         string_view, EnumHash<WarningLevel>>&         GetWarningLevels();
		KNODISCARD
		static const unordered_map<CustomFlag,           string_view, EnumHash<CustomFlag>>&           GetCustomFlags();

		static void ForceClose(
			string_view target,
			string_view message);
	};
}
