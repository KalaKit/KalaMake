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

	enum class StartType
	{
		VALIDATE,
		COMPILE
	};

	enum class Version
	{
		V_INVALID,

		V_1_0,
		V_2_0
	};

	//Allowed categories that can be added to any kmake file
	enum class CategoryType
	{
		INVALID,

		//Required version category
		VERSION,
		//Optional references category
		REFERENCES,
		//Required global fields category
		GLOBAL,
		//Optional N amount of profile categories with custom names
		PROFILE
	};

	//Allowed field types that can be added to global and profile categories
	enum class FieldType
	{
		INVALID,

		//What is the target type of the binary,
		//supported by all languages
		BINARY_TYPE,
		//Which compiler launcher runs before the compiler,
		//only for C and C++
		COMPILER_LAUNCHER,
		//Which compiler is used to compile this binary source code,
		//supported by all languages
		COMPILER,
		//Which language standard is used to compile this source code,
		//only for C, C++, Java and Rust
		STANDARD,
		//What is the target platform of the binary,
		//only for C, C++, Zig and Rust
		TARGET_TYPE,
		//How many jobs are allowed to do in parallel for compilation,
		//only for C and C++
		JOBS,

		//What is the name of the the binary,
		//supported by all languages
		BINARY_NAME,
		//Which build type is the binary,
		//only for C, C++, Java, Zig and Rust
		BUILD_TYPE,
		//Where is the binary built to,
		//supported by all languages
		BUILD_PATH,
		//Where are the source code files of the binary located,
		//supported by all languages
		SOURCES,
		//Where are the header files of the binary located,
		//only for C and C++
		HEADERS,
		//What links will be added to the binary,
		//only for C, C++, Java and Rust
		LINKS,
		//What warning level will compilation and linking use, defaults to 'none',
		//only for C and C++
		WARNING_LEVEL,
		//What defines will be added to the binary,
		//only for C, C++, Java (modules) and Rust (--cfg)
		DEFINES,
		//What flags will be passed to the compiler stage,
		//supported by all languages
		COMPILE_FLAGS,
		//What flags will be passed to the link stage,
		//only for C and C++
		LINK_FLAGS,
		//What kalamake-specific flags will trigger extra actions,
		//supported by all languages
		CUSTOM_FLAGS,

		//Pre-build action field, can add as many as you want,
		//supported by all languages
		PRE_BUILD_ACTION,
		//Post-build action field, can add as many as you want,
		//supported by all languages
		POST_BUILD_ACTION
	};

	//Allowed binary types that can be added to the binarytype field
	enum class BinaryType
	{
		INVALID,

		//Creates a runnable executable
		EXECUTABLE,
		//Creates a static library,
		//not used in Java and Python
		STATIC,
		//Creates a shared library,
		//not used in Java and Python
		SHARED
	};

	//Specifies a compiler launcher program that runs before the compiler,
	//only for C and C++
	enum class CompilerLauncherType
	{
		INVALID,

		//Reuses previous compile results
		CCACHE,
		//Same as ccache but supports shared cache servers
		SCCACHE,
		//Sends compile jobs to other servers
		DISTCC,
		//Same as distcc but ships compiler toolchain too
		ICECC
	};

	//Allowed compiler types that can be added to the compiler field
	enum class CompilerType
	{
		INVALID,

		//
		// C/C++
		//

		//C, C++ and Zig language, windows + linux, target-specific flags
		ZIG,

		//Windows only, MSVC-style flags
		CLANG_CL,
		//Windows only, MSVC-style flags
		CL,

		//Windows + linux, GNU flags, defaults to C
		CLANG,
		//Windows + linux, GNU flags, defaults to C++
		CLANGPP,
		//Linux, GNU flags, defaults to C
		GCC,
		//Linux, GNU flags, defaults to C++
		GPP,

		//
		// OTHERS
		//

		//General Java compiler
		JAVA,
		//General python compiler
		PYTHON,
		//General rust compiler
		RUST
	};

	//Allowed standard types that can be added to the standard field,
	//only for C, C++, Java and Rust
	enum class StandardType
	{
		INVALID,

		//
		// C/C++
		//

		C_89,
		C_99,
		C_11,
		C_17,
		C_23,

		CPP_14,
		CPP_17,
		CPP_20,
		CPP_23,
		CPP_26,

		//
		// JAVA
		//

		JAVA_8,
		JAVA_9,
		JAVA_10,
		JAVA_11,
		JAVA_12,
		JAVA_13,
		JAVA_14,
		JAVA_15,
		JAVA_16,
		JAVA_17,
		JAVA_18,
		JAVA_19,
		JAVA_20,
		JAVA_21,
		JAVA_22,
		JAVA_23,
		JAVA_24,
		JAVA_25,
		JAVA_26,

		//
		// RUST
		//

		RUST_15,
		RUST_18,
		RUST_21,
		RUST_24
	};

	//Allowed target types that can be added to the targettype field,
	//only for C, C++, Zig and Rust
	enum class TargetType
	{
		INVALID,

		//Build a linux target on windows or linux (glibc)
		LINUX_GNU,
		//Build a linux target on windows or linux (musl)
		LINUX_MUSL,
		//Build a windows target on linux or windows (mingw)
		WINDOWS_GNU,
		//Build a windows target on linux or windows (msvc)
		WINDOWS_MSVC
	};

	//Allowed build types that can be added to the buildtype field,
	//only for C, C++, Java and Rust
	enum class BuildType
	{
		INVALID,

		//Creates a debug binary
		DEBUG,
		//Creates a release binary
		RELEASE,
		//Creates a release binary with debug flag,
		//same as release on Java
		RELDEBUG,
		//Creates a release binary with smallest size flags,
		//same as release on Java
		MINSIZEREL
	};

	//Allowed warning levels that can be added to the warninglevel field,
	//only for C and C++
	enum class WarningLevel
	{
		INVALID,

		//No warnings
		NONE,
		//Very basic warnings
		BASIC,
		//Common, useful warnings, recommended default
		NORMAL,
		//Strong warnings
		STRONG,
		//Very strict, high signal warnings
		STRICT,
		//All warnings
		ALL
	};

	//Allowed custom flags that can be added to the customflags field.
	//TODO: add vs sln file support here as well
	enum class CustomFlag
	{
		INVALID,

		//Creates compile_commands.json in the same directory as the kmake file
		EXPORT_COMPILE_COMMANDS,
		//Creates .vscode folder with launch.json and tasks.json inside it
		EXPORT_VSCODE_SLN,

		//Treats all warnings as errors,
		//only for C and C++
		WARNINGS_AS_ERRORS,
		//Embed the C/C++ runtime into the binary,
		//dynamic runtime is enabled by default unless this is added,
		//only for C and C++, not used in linux
		MSVC_STATIC_RUNTIME,
		//Creates a .pdb file for debug symbols or embeds the symbols directly to the exe in any build type,
		//only for C and C++
		GENERATE_SYMBOLS,
		//Does not show the console on Windows when launching an executable,
		//only for C and C++
		NO_CONSOLE,

		//Package the created jar file into an executable,
		//only for Java
		PACKAGE_JAR,
		//Attaches the jpackage-made exe to windows console,
		//only for Java
		JAVA_WIN_CONSOLE,
		//Creates .classpath file in project root like compile_commans.json,
		//only for Java
		EXPORT_JAVA_SLN,

		//Pyinstaller bundles everything into a single exe, all files are extracted at each run,
		//otherwise it creates a dir with all content with faster startup.
		//only for Python
		PYTHON_ONE_FILE
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
