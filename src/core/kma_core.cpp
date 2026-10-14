//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include "core/kma_core.hpp"

#if _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <csignal>
#endif

#include <functional>

#include "log_utils.hpp"
#include "file_utils.hpp"
#include "string_utils.hpp"

#include "language/kma_language.hpp"

using KalaHeaders::KalaCore::ContainsValue;
using KalaHeaders::KalaCore::StringToEnum;
using KalaHeaders::KalaCore::EnumHash;

using KalaHeaders::KalaLog::Log;
using KalaHeaders::KalaLog::LogType;
using KalaHeaders::KalaLog::TimeFormat;
using KalaHeaders::KalaLog::DateFormat;

using KalaHeaders::KalaFile::ReadLinesFromFile;
using KalaHeaders::KalaFile::CreateNewDirectory;

using KalaHeaders::KalaString::IsStringInRange;
using KalaHeaders::KalaString::TrimString;
using KalaHeaders::KalaString::ContainsSpace;
using KalaHeaders::KalaString::SplitString;

using KalaMake::Language::LanguageCore;
using KalaMake::Core::Version;
using KalaMake::Core::CategoryType;
using KalaMake::Core::FieldType;
using KalaMake::Core::BinaryType;
using KalaMake::Core::CompilerLauncherType;
using KalaMake::Core::CompilerType;
using KalaMake::Core::StandardType;
using KalaMake::Core::TargetType;
using KalaMake::Core::BuildType;
using KalaMake::Core::WarningLevel;
using KalaMake::Core::CustomFlag;
using KalaMake::Core::GlobalData;

using std::string;
using std::string_view;
using std::to_string;
using std::vector;
using std::array;
using std::pair;
using std::unordered_map;
using std::filesystem::path;
using std::filesystem::is_directory;
using std::filesystem::is_regular_file;
using std::filesystem::absolute;
using std::filesystem::exists;
using std::filesystem::recursive_directory_iterator;
using std::from_chars;
using std::errc;
using std::function;

constexpr string_view version_1_0 = "1.0";
constexpr string_view version_2_0 = "2.0";

constexpr string_view category_version    = "version";
constexpr string_view category_references = "references";
constexpr string_view category_global     = "global";
constexpr string_view category_profile    = "profile";

constexpr string_view field_binary_type       = "binarytype";
constexpr string_view field_compiler_launcher = "compilerlauncher";
constexpr string_view field_compiler          = "compiler";
constexpr string_view field_standard          = "standard";
constexpr string_view field_target_type       = "targettype";
constexpr string_view field_jobs              = "jobs";
constexpr string_view field_binary_name       = "binaryname";
constexpr string_view field_build_type        = "buildtype";
constexpr string_view field_build_path        = "buildpath";
constexpr string_view field_sources           = "sources";
constexpr string_view field_headers           = "headers";
constexpr string_view field_links             = "links";
constexpr string_view field_warning_level     = "warninglevel";
constexpr string_view field_defines           = "defines";
constexpr string_view field_compile_flags     = "compileflags";
constexpr string_view field_link_flags        = "linkflags";
constexpr string_view field_custom_flags      = "customflags";
constexpr string_view field_pre_build_action  = "prebuildaction";
constexpr string_view field_post_build_action = "postbuildaction";

constexpr string_view binary_type_executable = "executable";
constexpr string_view binary_type_static     = "static";
constexpr string_view binary_type_shared     = "shared";

constexpr string_view compiler_launcher_ccache  = "ccache";
constexpr string_view compiler_launcher_sccache = "sccache";
constexpr string_view compiler_launcher_distcc  = "distcc";
constexpr string_view compiler_launcher_icecc   = "icecc";

constexpr string_view compiler_zig      = "zig";
constexpr string_view compiler_clang_cl = "clang-cl";
constexpr string_view compiler_cl       = "cl";
constexpr string_view compiler_clang    = "clang";
constexpr string_view compiler_clangpp  = "clang++";
constexpr string_view compiler_gcc      = "gcc";
constexpr string_view compiler_gpp      = "g++";
constexpr string_view compiler_java     = "java";
constexpr string_view compiler_python   = "python";
constexpr string_view compiler_rust     = "rust";

constexpr string_view standard_c89    = "c89";
constexpr string_view standard_c99    = "c99";
constexpr string_view standard_c11    = "c11";
constexpr string_view standard_c17    = "c17";
constexpr string_view standard_c23    = "c23";
constexpr string_view standard_cpp14  = "c++14";
constexpr string_view standard_cpp17  = "c++17";
constexpr string_view standard_cpp20  = "c++20";
constexpr string_view standard_cpp23  = "c++23";
constexpr string_view standard_cpp26  = "c++26";
constexpr string_view standard_java8  = "java8";
constexpr string_view standard_java9  = "java9";
constexpr string_view standard_java10 = "java10";
constexpr string_view standard_java11 = "java11";
constexpr string_view standard_java12 = "java12";
constexpr string_view standard_java13 = "java13";
constexpr string_view standard_java14 = "java14";
constexpr string_view standard_java15 = "java15";
constexpr string_view standard_java16 = "java16";
constexpr string_view standard_java17 = "java17";
constexpr string_view standard_java18 = "java18";
constexpr string_view standard_java19 = "java19";
constexpr string_view standard_java20 = "java20";
constexpr string_view standard_java21 = "java21";
constexpr string_view standard_java22 = "java22";
constexpr string_view standard_java23 = "java23";
constexpr string_view standard_java24 = "java24";
constexpr string_view standard_java25 = "java25";
constexpr string_view standard_java26 = "java26";
constexpr string_view standard_rust15 = "rust15";
constexpr string_view standard_rust18 = "rust18";
constexpr string_view standard_rust21 = "rust21";
constexpr string_view standard_rust24 = "rust24";

constexpr string_view target_type_linux_gnu    = "linux-gnu";
constexpr string_view target_type_linux_musl   = "linux-musl";
constexpr string_view target_type_windows_gnu  = "windows-gnu";
constexpr string_view target_type_windows_msvc = "windows-msvc";

constexpr string_view build_type_debug      = "debug";
constexpr string_view build_type_release    = "release";
constexpr string_view build_type_reldebug   = "reldebug";
constexpr string_view build_type_minsizerel = "minsizerel";

constexpr string_view warning_level_none   = "none";
constexpr string_view warning_level_basic  = "basic";
constexpr string_view warning_level_normal = "normal";
constexpr string_view warning_level_strong = "strong";
constexpr string_view warning_level_strict = "strict";
constexpr string_view warning_level_all    = "all";

constexpr string_view custom_export_comp_comm    = "export-compile-commands";
constexpr string_view custom_export_vscode_sln   = "export-vscode-sln";
constexpr string_view custom_warnings_as_err     = "warnings-as-errors";
constexpr string_view custom_msvc_static_runtime = "msvc-static-runtime";
constexpr string_view custom_generate_symbols    = "generate-symbols";
constexpr string_view custom_no_console          = "no-console";
constexpr string_view custom_package_jar         = "package-jar";
constexpr string_view custom_java_win_console    = "java-win-console";
constexpr string_view custom_export_java_sln     = "export-java-sln";
constexpr string_view custom_python_one_file     = "python-one-file";

//max number of reference expansions allowed to prevent infinite recursion
static constexpr u32 MAX_EXPANSION_LIMIT = 100;

static constexpr u32 MIN_BINARY_NAME_LENGTH = 3;
static constexpr u32 MAX_BINARY_NAME_LENGTH = 50;

static const unordered_map<Version, string_view, EnumHash<Version>> versions =
{
    { Version::V_1_0, version_1_0 },
    { Version::V_2_0, version_2_0 }
};

static const unordered_map<CategoryType, string_view, EnumHash<CategoryType>> categoryTypes =
{
    { CategoryType::C_VERSION,    category_version },
    { CategoryType::C_REFERENCES, category_references },
    { CategoryType::C_GLOBAL,     category_global },
    { CategoryType::C_PROFILE,    category_profile }
};

static const unordered_map<FieldType, string_view, EnumHash<FieldType>> fieldTypes =
{
    { FieldType::T_BINARY_TYPE,       field_binary_type },
    { FieldType::T_COMPILER_LAUNCHER, field_compiler_launcher },
    { FieldType::T_COMPILER,          field_compiler },
    { FieldType::T_STANDARD,          field_standard },
    { FieldType::T_TARGET_TYPE,       field_target_type },
    { FieldType::T_JOBS,              field_jobs },

    { FieldType::T_BINARY_NAME,    field_binary_name },
    { FieldType::T_BUILD_TYPE,     field_build_type },
    { FieldType::T_BUILD_PATH,     field_build_path },
    { FieldType::T_SOURCES,        field_sources },
    { FieldType::T_HEADERS,        field_headers },
    { FieldType::T_LINKS,          field_links },
    { FieldType::T_WARNING_LEVEL,  field_warning_level },
    { FieldType::T_DEFINES,        field_defines },
    { FieldType::T_COMPILE_FLAGS,  field_compile_flags },
    { FieldType::T_LINK_FLAGS,     field_link_flags },
    { FieldType::T_CUSTOM_FLAGS,   field_custom_flags },

    { FieldType::T_PRE_BUILD_ACTION, field_pre_build_action },
    { FieldType::T_POST_BUILD_ACTION, field_post_build_action }
};

static const unordered_map<BinaryType, string_view, EnumHash<BinaryType>> binaryTypes =
{
    { BinaryType::B_EXECUTABLE, binary_type_executable },
    { BinaryType::B_STATIC,     binary_type_static },
    { BinaryType::B_SHARED,     binary_type_shared }
};

static const unordered_map<CompilerLauncherType, string_view, EnumHash<CompilerLauncherType>> compilerLauncherTypes =
{
    { CompilerLauncherType::C_CCACHE,  compiler_launcher_ccache },
    { CompilerLauncherType::C_SCCACHE, compiler_launcher_sccache },
    { CompilerLauncherType::C_DISTCC,  compiler_launcher_distcc },
    { CompilerLauncherType::C_ICECC,   compiler_launcher_icecc }
};

static const unordered_map<CompilerType, string_view, EnumHash<CompilerType>> compilerTypes =
{
    { CompilerType::C_ZIG,      compiler_zig },
    { CompilerType::C_CLANG_CL, compiler_clang_cl },
    { CompilerType::C_CL,       compiler_cl },

    { CompilerType::C_CLANG,   compiler_clang },
    { CompilerType::C_CLANGPP, compiler_clangpp },
    { CompilerType::C_GCC,     compiler_gcc },
    { CompilerType::C_GPP,     compiler_gpp },

    { CompilerType::C_JAVA,   compiler_java },
    { CompilerType::C_PYTHON, compiler_python },
    { CompilerType::C_RUST,   compiler_rust }
};

static const unordered_map<StandardType, string_view, EnumHash<StandardType>> standardTypes =
{
    { StandardType::C_89, standard_c89 },
    { StandardType::C_99, standard_c99 },
    { StandardType::C_11, standard_c11 },
    { StandardType::C_17, standard_c17 },
    { StandardType::C_23, standard_c23 },

    { StandardType::CPP_14, standard_cpp14 },
    { StandardType::CPP_17, standard_cpp17 },
    { StandardType::CPP_20, standard_cpp20 },
    { StandardType::CPP_23, standard_cpp23 },
    { StandardType::CPP_26, standard_cpp26 },

    { StandardType::JAVA_8,  standard_java8 },
    { StandardType::JAVA_9,  standard_java9 },
    { StandardType::JAVA_10, standard_java10 },
    { StandardType::JAVA_11, standard_java11 },
    { StandardType::JAVA_12, standard_java12 },
    { StandardType::JAVA_13, standard_java13 },
    { StandardType::JAVA_14, standard_java14 },
    { StandardType::JAVA_15, standard_java15 },
    { StandardType::JAVA_16, standard_java16 },
    { StandardType::JAVA_17, standard_java17 },
    { StandardType::JAVA_18, standard_java18 },
    { StandardType::JAVA_19, standard_java19 },
    { StandardType::JAVA_20, standard_java20 },
    { StandardType::JAVA_21, standard_java21 },
    { StandardType::JAVA_22, standard_java22 },
    { StandardType::JAVA_23, standard_java23 },
    { StandardType::JAVA_24, standard_java24 },
    { StandardType::JAVA_25, standard_java25 },
    { StandardType::JAVA_26, standard_java26 },

    { StandardType::RUST_15, standard_rust15 },
    { StandardType::RUST_18, standard_rust18 },
    { StandardType::RUST_21, standard_rust21 },
    { StandardType::RUST_24, standard_rust24 }
};

static const unordered_map<TargetType, string_view, EnumHash<TargetType>> targetTypes =
{
    { TargetType::T_LINUX_GNU,    target_type_linux_gnu },
    { TargetType::T_LINUX_MUSL,   target_type_linux_musl },
    { TargetType::T_WINDOWS_GNU,  target_type_windows_gnu },
    { TargetType::T_WINDOWS_MSVC, target_type_windows_msvc }
};

static const unordered_map<BuildType, string_view, EnumHash<BuildType>> buildTypes =
{
    { BuildType::B_DEBUG,      build_type_debug },
    { BuildType::B_RELEASE,    build_type_release },
    { BuildType::B_RELDEBUG,   build_type_reldebug },
    { BuildType::B_MINSIZEREL, build_type_minsizerel }
};

//Same warning types are used for both MSVC and GNU,
//their true meanings change depending on which OS is used
static const unordered_map<WarningLevel, string_view, EnumHash<WarningLevel>> warningLevels =
{
    { WarningLevel::W_NONE,   warning_level_none },
    { WarningLevel::W_BASIC,  warning_level_basic },
    { WarningLevel::W_NORMAL, warning_level_normal },
    { WarningLevel::W_STRONG, warning_level_strong },
    { WarningLevel::W_STRICT, warning_level_strict },
    { WarningLevel::W_ALL,    warning_level_all }
};

//Same warning types are used for both MSVC and GNU,
//their true meanings change depending on which OS is used
static const unordered_map<CustomFlag, string_view, EnumHash<CustomFlag>> customFlags =
{
    { CustomFlag::F_EXPORT_COMPILE_COMMANDS, custom_export_comp_comm },
    { CustomFlag::F_EXPORT_VSCODE_SLN,       custom_export_vscode_sln },
    { CustomFlag::F_WARNINGS_AS_ERRORS,      custom_warnings_as_err },
    { CustomFlag::F_MSVC_STATIC_RUNTIME,     custom_msvc_static_runtime },
    { CustomFlag::F_GENERATE_SYMBOLS,        custom_generate_symbols },
    { CustomFlag::F_NO_CONSOLE,              custom_no_console },
    { CustomFlag::F_PACKAGE_JAR,             custom_package_jar },
    { CustomFlag::F_JAVA_WIN_CONSOLE,        custom_java_win_console },
    { CustomFlag::F_EXPORT_JAVA_SLN,         custom_export_java_sln },
    { CustomFlag::F_PYTHON_ONE_FILE,         custom_python_one_file }
};

static constexpr array<string_view, 19> availableFields =
{
    field_compiler_launcher,
    field_compiler,
    field_standard,
    field_binary_name,
    field_build_type,
    field_target_type,
    field_binary_type,
    field_build_path,
    field_warning_level,
    field_jobs,
    field_sources,
    field_headers,
    field_links,
    field_defines,
    field_compile_flags,
    field_link_flags,
    field_custom_flags,
    field_pre_build_action,
    field_post_build_action
};

static constexpr array<string_view, 7> requiredFields = 
{
    field_compiler,
    field_standard,
    field_binary_name,
    field_build_type,
    field_binary_type,
    field_build_path,
    field_sources  
};

static constexpr array<string_view, 12> singleValueFields = 
{
    field_compiler_launcher,
    field_compiler,
    field_standard,
    field_binary_name,
    field_build_type,
    field_target_type,
    field_binary_type,
    field_build_path,
    field_warning_level,
    field_jobs,
    field_pre_build_action,
    field_post_build_action
};

static bool isInHeadlessMode{};
static GlobalData globalData{};
static vector<string> userParams{};

static u16 GetThreadCount()
{
#if _WIN32
	return scast<u16>(GetActiveProcessorCount(ALL_PROCESSOR_GROUPS));
#else
	return scast<u16>(sysconf(_SC_NPROCESSORS_ONLN));
#endif
};

static string GetGlobalData(const vector<string>& content);
static string ListProfiles();
static string CleanBuildDirs();

namespace KalaMake::Core
{
    bool KalaMakeCore::IsInHeadlessMode() { return isInHeadlessMode; }
    void KalaMakeCore::SetHeadlessModeState(bool newValue) { isInHeadlessMode = newValue; }

    void KalaMakeCore::OpenFile(
        StartType startType,
        const vector<string>& params)
    {
        userParams = params;

        if (userParams[1] == "version"
            || userParams[1] == "references")
        {
            ForceClose(
                "KalaMake core error",
                "Failed to parse kmake file '" + userParams[0] 
                + " because the target profile '" + userParams[1] + "' was invalid!");
        }

        vector<string> kmakeContent{};
        string err = ReadLinesFromFile(userParams[0], kmakeContent);
        if (!err.empty())
        {
            Log::Print(
                "Failed to parse kmake file '" + userParams[0] + "'! Reason: " + err,
                "KMAKE_CORE",
                LogType::LOG_ERROR,
                2);

            return;
        }

        err = GetGlobalData(kmakeContent);
        if (startType == StartType::S_VALIDATE
            || !err.empty())
        {
            if (!err.empty())
            {
                ForceClose(
                    "KalaMake core error",
                    "Failed to get data from kmake file '" + userParams[0] + "'! Reason: " + err);
            }

            if (isInHeadlessMode) exit(0);
            return;
        }

        if (startType == StartType::S_LIST_PROFILES)
        {
            err = ListProfiles();
            if (!err.empty())
            {
                ForceClose(
                    "KalaMake core error",
                    "Failed to list profiles from kmake file '" + userParams[0] + "'! Reason: " + err);
            }

            if (isInHeadlessMode) exit(0);
            return;
        }

        if (startType == StartType::S_CLEAN)
        {
            err = CleanBuildDirs();
            if (!err.empty())
            {
                ForceClose(
                    "KalaMake core error",
                    "Failed to clean build dirs from kmake file '" + userParams[0] + "'! Reason: " + err);
            }

            if (isInHeadlessMode) exit(0);
            return;
        }

        CompilerType c = globalData.targetProfile.compiler;
        switch (c)
        {
        default:
        case CompilerType::C_ZIG:
        case CompilerType::C_CL:
        case CompilerType::C_CLANG_CL:
        case CompilerType::C_CLANG:
        case CompilerType::C_CLANGPP:
        case CompilerType::C_GCC:
        case CompilerType::C_GPP:
        {
            if (c == CompilerType::C_ZIG
                && globalData.targetProfile.standard == StandardType::S_INVALID)
            {
                LanguageCore::Compile_Zig(globalData);
            }
            else LanguageCore::Compile_C_CPP(globalData);

            if (isInHeadlessMode) exit(0);
            return;
        }
        case CompilerType::C_JAVA:
        {
            LanguageCore::Compile_Java(globalData);

            if (isInHeadlessMode) exit(0);
            return;
        }
        case CompilerType::C_PYTHON:
        {
            LanguageCore::Compile_Python(globalData);

            if (isInHeadlessMode) exit(0);
            return;
        }
        case CompilerType::C_RUST:
        {
            LanguageCore::Compile_Rust(globalData);

            if (isInHeadlessMode) exit(0);
            return;
        }
        }
    }

	const unordered_map<Version,      string_view, EnumHash<Version>>&      KalaMakeCore::GetVersions()      { return versions; }
	const unordered_map<CategoryType, string_view, EnumHash<CategoryType>>& KalaMakeCore::GetCategoryTypes() { return categoryTypes; }
	const unordered_map<FieldType,    string_view, EnumHash<FieldType>>&    KalaMakeCore::GetFieldTypes()    { return fieldTypes; }

	const unordered_map<BinaryType,           string_view, EnumHash<BinaryType>>&           KalaMakeCore::GetBinaryTypes()           { return binaryTypes; }
	const unordered_map<CompilerLauncherType, string_view, EnumHash<CompilerLauncherType>>& KalaMakeCore::GetCompilerLauncherTypes() { return compilerLauncherTypes; }
	const unordered_map<CompilerType,         string_view, EnumHash<CompilerType>>&         KalaMakeCore::GetCompilerTypes()         { return compilerTypes; }
	const unordered_map<StandardType,         string_view, EnumHash<StandardType>>&         KalaMakeCore::GetStandardTypes()         { return standardTypes; }
	const unordered_map<TargetType,           string_view, EnumHash<TargetType>>&           KalaMakeCore::GetTargetTypes()           { return targetTypes; }
	const unordered_map<BuildType,            string_view, EnumHash<BuildType>>&            KalaMakeCore::GetBuildTypes()            { return buildTypes; }
	const unordered_map<WarningLevel,         string_view, EnumHash<WarningLevel>>&         KalaMakeCore::GetWarningLevels()         { return warningLevels; }
	const unordered_map<CustomFlag,           string_view, EnumHash<CustomFlag>>&           KalaMakeCore::GetCustomFlags()           { return customFlags; }

    void KalaMakeCore::ForceClose(
		string_view target,
		string_view reason)
	{
		Log::Print(
			"\n================"
			"\nFORCE CLOSE"
			"\n================\n",
			true);

		Log::Print(
			reason,
			target,
			LogType::LOG_ERROR,
			2,
			true,
			TimeFormat::TIME_NONE,
			DateFormat::DATE_NONE);

#if defined(KWIN_ANY)
		__debugbreak();
#else
		raise(SIGTRAP);
#endif

		_Exit(1);
	}
}

string GetGlobalData(const vector<string>& content)
{
    struct CategoryBlock
    {
        string categoryName{};
        vector<string> categoryWords{};
    };

    auto get_categories = [&](vector<CategoryBlock>& outResult) -> string
        {
            vector<vector<string>> cleanedLines{};

            for (const string& line : content)
            {
                if (line.empty()) continue;

                string trimmed{};
                string err = TrimString(line, trimmed);

                if (!err.empty())
                {
                    return "Failed to trim line '" + line + "'! Reason: " + err;
                }

                vector<string> split{};
                string word{};
                bool inQuotes{};
                bool escaped{};
                
                for (char c : trimmed)
                {
                    if ((c == ' '
                        || c == '\t')
                        && !inQuotes)
                    {
                        if (!word.empty())
                        {
                            if (word.starts_with("//")) break;

                            split.push_back(word);
                            word.clear();
                        }

                        continue;
                    }

                    word += c;

                    if (escaped)
                    {
                        escaped = false;
                        continue;
                    }

                    if (c == '\\'
                        && inQuotes)
                    {
                        escaped = true;
                        continue;
                    }

                    if (c == '"') inQuotes = !inQuotes;
                }

                if (inQuotes)
                {
                    return "Failed to parse line '" + line + "' because it is a quoted value and it was not closed!";
                }

                if (!word.empty()
                    && !word.starts_with("//"))
                {
                    split.push_back(word);
                }

                //ignore empty lines after all trimming
                if (split.empty()) continue;

                cleanedLines.push_back(split);
            }

            if (cleanedLines.empty())
            {
                return "Failed to get categories because there was nothing left after initial cleanup!";
            }

            CategoryBlock catBlock{};

            //group by category
            for (size_t i = 0; i < cleanedLines.size(); ++i)
            {
                const vector<string>& cleanLine = cleanedLines[i];

                for (size_t j = 0; j < cleanLine.size(); ++j)
                {
                    const string& cleanWord = cleanLine[j];

                    if (cleanWord.starts_with('#'))
                    {
                        string categoryName = cleanWord.substr(1);
                        if (categoryName == "profile")
                        {
                            if (!catBlock.categoryName.empty())
                            {
                                outResult.push_back(catBlock);
                                catBlock = {};
                            }

                            string profileName{};

                            if (j + 1 < cleanLine.size())
                            {
                                profileName = cleanLine[j + 1];
                                j++;
                            }
                            else return "Failed to get categories because custom profile had no name on the same line!";

                            if (profileName.starts_with('#')
                                || profileName.ends_with(':')
                                || (profileName.starts_with('"')
                                && (profileName.ends_with('"')
                                || profileName.ends_with("\","))))
                            {
                                return "Failed to get categories because custom profile had no valid name!";
                            }

                            if (profileName == "version"
                                || profileName == "references"
                                || profileName == "global")
                            {
                                return "Failed to get categories because custom profile name '" + profileName + "' is reserved!";
                            }

                            catBlock.categoryName = cleanWord + " " + profileName;
                            continue;
                        }
                        else
                        {
                            if (!catBlock.categoryName.empty())
                            {
                                outResult.push_back(catBlock);
                                catBlock = {};
                            }
                            catBlock.categoryName = cleanWord;

                            continue;
                        }
                    }
                    else
                    {
                        //only version can contain value with no : or '"'
                        if (catBlock.categoryName == "#version")
                        {
                            if (cleanWord.ends_with(':')
                                || cleanWord.starts_with('"')
                                || (cleanWord.ends_with('"')
                                || cleanWord.ends_with("\",")))
                            {
                                return 
                                    "Failed to get categories because value '" + cleanWord 
                                    + "' structure was invalid in category '" + catBlock.categoryName + "'!";
                            }
                        }
                        else
                        {
                            //each value not in version category must start and end with '"' or end with :
                            bool validValue = 
                                ((cleanWord.ends_with(':')
                                && cleanWord.find('"') == string::npos)
                                || (cleanWord.starts_with('"')
                                && (cleanWord.ends_with('"')
                                || cleanWord.ends_with("\","))));
                            
                            if (!validValue)
                            {
                                return 
                                    "Failed to get categories because value '" + cleanWord 
                                    + "' structure was invalid in category '" + catBlock.categoryName + "'!";
                            }
                        }

                        catBlock.categoryWords.push_back(cleanWord);
                    }
                }
            }

            //also push last category block
            outResult.push_back(catBlock);

            return "";
        };

    vector<CategoryBlock> categories{};
    string err = get_categories(categories);
    if (!err.empty()) return err;

    vector<string> foundCategoryNames{};

    //find empty categories and find duplicate category names
    for (const CategoryBlock& block : categories)
    {
        if (ContainsValue(foundCategoryNames, block.categoryName))
        {
            return "Failed to get categories because category name '" + block.categoryName + "' was used more than once!";
        }

        if (block.categoryWords.empty())
        {
            return "Failed to get categories because category '" + block.categoryName + "' has no values!";
        }

        foundCategoryNames.push_back(block.categoryName);
    }

    string targetCategory = userParams[1] == "global"
        ? "#global"
        : "#profile " + userParams[1];

    if (!ContainsValue(foundCategoryNames, targetCategory))
    {
        return "Failed to get categories because category '" + userParams[1] + "' was not found!";
    }

    //find missing required categories
    if (!ContainsValue(foundCategoryNames, "#version"))
    {
        return "Failed to verify categories because required category 'version' is missing!";
    }
    if (!ContainsValue(foundCategoryNames, "#global"))
    {
        return "Failed to verify categories because required category 'global' is missing!";
    }

    //ensure version category only has one value
    for (const CategoryBlock& cat : categories)
    {
        if (cat.categoryName == "#version")
        {
            if (cat.categoryWords.size() > 1)
            {
                return "Failed to verify categories because category 'version' has more than one value!";
            }
            if (cat.categoryWords[0] != version_1_0
                && cat.categoryWords[0] != version_2_0)
            {
                return "Failed to verify categories because category 'version' has an invalid version value!";
            }
            if (cat.categoryWords[0] == version_1_0)
            {
                return "Failed to verify categories because category 'version' has an unsupported version!";
            }
        }
    }

    struct FieldBlock
    {
        string fieldName{};
        vector<string> fieldValues{};
    };

    auto get_field_data = [](
        const CategoryBlock& categoryBlock,
        vector<FieldBlock>& outResult) -> string
        {
            FieldBlock fieldBlock{};

            for (const string& blockWord : categoryBlock.categoryWords)
            {
                if (categoryBlock.categoryName == "#version")
                {
                    outResult.push_back({ "", { blockWord } });
                    break;
                }

                if (!blockWord.ends_with(':')
                    && fieldBlock.fieldName.empty())
                {
                    return "Failed to parse value '" + blockWord + "' because it was not preceded by a field!";
                }

                if (blockWord.ends_with(':'))
                {
                    if (!fieldBlock.fieldName.empty())
                    {
                        outResult.push_back(fieldBlock);
                        fieldBlock = {};
                    }
                    fieldBlock.fieldName = blockWord;

                    continue;
                }

                fieldBlock.fieldValues.push_back(blockWord);
            }

            //push last block if its not empty
            if (!fieldBlock.fieldName.empty()) outResult.push_back(fieldBlock);

            return "";
        };

    vector<pair<string, vector<FieldBlock>>> gatheredData{};
    for (const CategoryBlock& cat : categories)
    {
        vector<FieldBlock> result{};
        err = get_field_data(
            cat,
            result);

        if (!err.empty()) return err;

        gatheredData.push_back({ cat.categoryName, std::move(result) });
    }

    for (size_t i = 0; i < gatheredData.size(); ++i)
    {
        pair<string, vector<FieldBlock>>& catBlock = gatheredData[i];

        //remove starting '#' from categories
        catBlock.first = catBlock.first.substr(1);
        //remove starting 'profile ' from categories
        string toBeErased = "profile ";
        if (catBlock.first.starts_with(toBeErased))
        {
            catBlock.first = catBlock.first.substr(toBeErased.size());
        }

        for (size_t j = 0; j < catBlock.second.size(); ++j)
        {
            FieldBlock& fieldBlock = catBlock.second[j];

            if (fieldBlock.fieldValues.empty())
            {
                return 
                    "Failed to parse field '" + fieldBlock.fieldName 
                    + "' in category '" + catBlock.first + "' because it had no values!";
            }

            bool hasMoreThanOneValue = fieldBlock.fieldValues.size() > 1;

            if (catBlock.first != "version")
            {
                for (size_t k = 0; k < fieldBlock.fieldValues.size(); ++k)
                {
                    const string& fieldValue = fieldBlock.fieldValues[k];
                    
                    if (hasMoreThanOneValue
                        && k != fieldBlock.fieldValues.size() - 1
                        && !fieldValue.ends_with("\","))
                    {
                        return 
                            "Failed to parse value in field '" + fieldBlock.fieldName 
                            + "' in category '" + catBlock.first + "' because the value must end with a comma!";
                    }

                    //last field value must not end with '",'
                    if (k == fieldBlock.fieldValues.size() - 1
                        && fieldValue.ends_with("\","))
                    {
                        return 
                            "Failed to parse last value in field '" + fieldBlock.fieldName 
                            + "' in category '" + catBlock.first + "' because the last value must not end with a comma!";
                    }
                }

                //remove ending ':' from fields
                fieldBlock.fieldName = fieldBlock.fieldName.substr(0, fieldBlock.fieldName.size() - 1);

                if (catBlock.first != "references")
                {
                    if (!ContainsValue(availableFields, fieldBlock.fieldName))
                    {
                        return 
                            "Failed to parse field '" + fieldBlock.fieldName 
                            + "' in category '" + catBlock.first + "' because it is unsupported!"; 
                    }
                }
            }

            if (hasMoreThanOneValue
                && ContainsValue(singleValueFields, fieldBlock.fieldName))
            {
                return 
                    "Failed to parse field '" + fieldBlock.fieldName 
                    + "' in category '" + catBlock.first + "' because it cannot have more than one value!";
            }

            for (string& val : fieldBlock.fieldValues)
            {
                if (val.starts_with('"'))
                {
                    //remove starting '"' from values
                    val = val.substr(1);

                    //remove ending '"' from values
                    if (val.ends_with('"')) val = val.substr(0, val.size() - 1);
                    //remove ending '"' from values where they end as '",'
                    else val = val.substr(0, val.size() - 2);
                }
            }
        }
    }

    vector<FieldBlock> foundReferences{};
    vector<FieldBlock> foundContentInGlobal{};
    pair<string, vector<FieldBlock>> foundContentInCustomProfile{};

    for (const pair<string, vector<FieldBlock>>& catBlock : gatheredData)
    {
        vector<string> foundFields{};
        for (const FieldBlock& fieldBlock : catBlock.second)
        {
            bool isRepeatable = 
                fieldBlock.fieldName == "prebuildaction"
                || fieldBlock.fieldName == "postbuildaction";

            //non-references category allows to have more than one repeatable field,
            //references category does not allow to have more
            if (ContainsValue(foundFields, fieldBlock.fieldName)
                && (!isRepeatable
                || catBlock.first == "references"))
            {
                return 
                    "Failed to parse field '" + fieldBlock.fieldName 
                    + "' in category '" + catBlock.first + "' because it was added more than once to the category!";
            }

            foundFields.push_back(fieldBlock.fieldName);
        }
    }

    //store references
    for (const pair<string, vector<FieldBlock>>& catBlock : gatheredData)
    {
        if (catBlock.first == "references")
        {
            foundReferences = catBlock.second;
            break;
        }
    }

    //store global content
    for (const pair<string, vector<FieldBlock>>& catBlock : gatheredData)
    {
        if (catBlock.first == "global")
        {
            foundContentInGlobal = catBlock.second;
            break;
        }
    }

    //store custom profile content
    if (userParams[1] != "global")
    {
        for (const pair<string, vector<FieldBlock>>& catBlock : gatheredData)
        {
            if (catBlock.first == userParams[1])
            {
                foundContentInCustomProfile = { catBlock.first, catBlock.second };
                break;
            }
        }
    }

    //get found required fields global already has
    vector<string> foundGlobalRequiredFields{};

    for (const FieldBlock& fieldBlock : foundContentInGlobal)
    {
        if (ContainsValue(requiredFields, fieldBlock.fieldName))
        {
            foundGlobalRequiredFields.push_back(fieldBlock.fieldName);
        }
    }

    if (userParams[1] != "global")
    {
        //check if each custom profile has required fields global does not have
        vector<string> foundFields{};
        for (const FieldBlock& fieldBlock : foundContentInCustomProfile.second)
        {
            if (ContainsValue(requiredFields, fieldBlock.fieldName))
            {
                foundFields.push_back(fieldBlock.fieldName);
            }
        }

        for (const string_view requiredField : requiredFields)
        {
            if (!ContainsValue(foundGlobalRequiredFields, requiredField)
                && !ContainsValue(foundFields, requiredField))
            {
                return 
                    "Failed to parse field '" + string(requiredField) 
                    + "' because it is required and it was not found in "
                    "'global' and '" + foundContentInCustomProfile.first + "' profile!";
            }
        }
    }
    else
    {
        for (const string_view requiredField : requiredFields)
        {
            if (!ContainsValue(foundGlobalRequiredFields, requiredField))
            {
                return 
                    "Failed to parse field '" + string(requiredField) 
                    + "' because it is required and it was not found in 'global' profile!";
            }
        }
    }

    pair<string, vector<FieldBlock>> finalCategoryContent{};
    vector<FieldBlock> globalPrePostBuildActions{};

    if (userParams[1] != "global")
    {
        for (const FieldBlock& globalBlock : foundContentInGlobal)
        {
            bool found{};
            for (FieldBlock& customBlock : foundContentInCustomProfile.second)
            {
                if (customBlock.fieldName == globalBlock.fieldName)
                {
                    found = true;

                    //append multi-value field values from global to custom profile
                    if (!ContainsValue(singleValueFields, customBlock.fieldName))
                    {
                        //append non-existing multivalue
                        for (const string& value : globalBlock.fieldValues)
                        {
                            if (!ContainsValue(customBlock.fieldValues, value))
                            {
                                customBlock.fieldValues.push_back(value);
                            }
                        }
                    }
                }
            }

            //add missing fields from global to custom
            if (!found
                || globalBlock.fieldName == field_pre_build_action
                || globalBlock.fieldName == field_post_build_action)
            {
                foundContentInCustomProfile.second.push_back(globalBlock);
            }
        }

        finalCategoryContent = std::move(foundContentInCustomProfile);
    }
    else finalCategoryContent = { "global", std::move(foundContentInGlobal) };

    string compiler{};
    for (const FieldBlock& fieldBlock : finalCategoryContent.second)
    {
        if (fieldBlock.fieldName == field_compiler)
        {
            compiler = fieldBlock.fieldValues[0];
            break;
        }
    }

    bool foundJobs{};
    for (const FieldBlock& fieldBlock : finalCategoryContent.second)
    {
        if (fieldBlock.fieldName == field_jobs)
        {
            foundJobs = true;
            break;
        }
    }

    //only c/c++ compilers can auto-assign job if not assigned
    if (compiler == compiler_clang_cl
        || compiler == compiler_cl
        || compiler == compiler_clang
        || compiler == compiler_clangpp
        || compiler == compiler_gcc
        || compiler == compiler_gpp
        || compiler == compiler_zig)
    {
        if (!foundJobs)
        {
            string jobsCount = to_string(GetThreadCount());
            finalCategoryContent.second.push_back(
                { 
                    "jobs", 
                    { jobsCount } 
                });

            Log::Print(
                "Set jobs count to '" + jobsCount + "' because it was not assigned.",
                "KMAKE_CORE",
                LogType::LOG_INFO);
        }
    }
    else
    {
        if (foundJobs)
        {
            return
                "Failed to parse field 'jobs' because it was assigned to compiler '" + compiler +
                "' that does not support multiple jobs!";
        }
    }

    auto translate_reference = [&](
        string inValue,
        string& outValue) -> string
        {
            //value did not contain any references, skip
            if (inValue.find('$') == string_view::npos)
            {
                outValue = inValue;
                return "";
            }

            size_t replacementCount{};
            function<string(size_t)> replace_ref;

            replace_ref = [&](size_t start) -> string
                {
                    size_t refBracketPos = start + 1;
                    size_t refBracketEndPos = string::npos;
                    size_t depth = 1;

                    for (size_t i = refBracketPos + 1; i < inValue.size(); i++)
                    {
                        if (i + 1 < inValue.size()
                            && inValue[i] == '$'
                            && inValue[i + 1] == '{')
                        {
                            depth++;
                            i++;
                        }
                        else if (inValue[i] == '}')
                        {
                            depth--;
                            if (depth == 0)
                            {
                                refBracketEndPos = i;
                                break;
                            }
                        }
                    }

                    if (refBracketEndPos == string::npos)
                    {
                        return
                            "Failed to find dereference values in field value '" + inValue +
                            "' because a reference value was not closed by a closing bracket!";
                    }

                    //find ref start here before the closing bracket and jump to child
                    size_t childRef = inValue.find("${", refBracketPos + 1);
                    while (childRef != string::npos
                        && childRef < refBracketEndPos)
                    {
                        string err = replace_ref(childRef);
                        if (!err.empty()) return err;

                        //recalculate closing bracket after child replacement
                        refBracketEndPos = inValue.find('}', refBracketPos);

                        childRef = inValue.find("${", refBracketPos + 1);
                    }

                    if (refBracketEndPos == refBracketPos + 1)
                    {
                        return
                            "Failed to find dereference values in field value '" + inValue 
                            + "' because a reference had no value between brackets!";
                    }

                    string refValue = inValue.substr(
                        refBracketPos + 1,
                        refBracketEndPos - refBracketPos - 1);

                    if (ContainsSpace(refValue))
                    {
                        return
                            "Failed to find dereference values in field value '" + inValue 
                            + "' because reference '" + refValue + "' contains whitespace characters!";
                    }

                    string replacement{};
                    for (const auto& r : globalData.references)
                    {
                        if (r.name == refValue)
                        {
                            replacement = r.value;
                            break;
                        }
                    }
                    if (replacement.empty())
                    {
                        return
                            "Failed to find dereference value '" + refValue + "' in field value '" 
                            + inValue + "' because it has not been added as a reference!";
                    }

                    if (++replacementCount > MAX_EXPANSION_LIMIT)
                    {
                        return
                            "Failed to translate references in field value '" + inValue 
                            + "' because the maximum reference replacement limit was exceeded!" 
                            "Possible circular reference.";
                    }

                    inValue.replace(
                        start,
                        refBracketEndPos - start + 1,
                        replacement);

                    return "";
                };

            for (size_t i = 0; i < inValue.size();)
            {
                if (i + 1 < inValue.size()
                    && inValue[i] == '$'
                    && inValue[i + 1] == '{')
                {
                    string err = replace_ref(i);
                    if (!err.empty()) return err;
                }
                else i++;
            }

            outValue = inValue;
            return "";
        };

    for (const FieldBlock& fieldBlock : foundReferences)
    {
        string combined{};
        for (const string& value : fieldBlock.fieldValues)
        {
            string translated{};
            string err = translate_reference(value, translated);
            if (!err.empty()) return err;

            if (!combined.empty()) combined += ", ";
            combined += translated;
        }

        globalData.references.push_back(
            {
                .name = fieldBlock.fieldName,
                .value = std::move(combined)
            });
    }

    auto try_parse_jobs = [](string_view text) -> bool
        {
            u16 jobs{};

            auto [ptr, ec] = from_chars(
                text.data(),
                text.data() + text.size(),
                jobs);

            if (ec != errc{}
                || ptr != text.data() + text.size())
            {
                return false;
            }

            return true;
        };

    globalData.projectFile = absolute(userParams[0]);
    globalData.targetProfile.profileName = userParams[1];

    for (FieldBlock& fieldBlock : finalCategoryContent.second)
    {
        Log::Print("--field '" + fieldBlock.fieldName + "'", true);

        if (fieldBlock.fieldName == field_compiler_launcher
            || fieldBlock.fieldName == field_compiler
            || fieldBlock.fieldName == field_standard
            || fieldBlock.fieldName == field_target_type
            || fieldBlock.fieldName == field_binary_type
            || fieldBlock.fieldName == field_build_type
            || fieldBlock.fieldName == field_warning_level
            || fieldBlock.fieldName == field_binary_name
            || fieldBlock.fieldName == field_build_path
            || fieldBlock.fieldName == field_jobs
            || fieldBlock.fieldName == field_pre_build_action
            || fieldBlock.fieldName == field_post_build_action)
        {
            string value = fieldBlock.fieldValues[0];
            string expanded{};
            string err = translate_reference(value, expanded);
            if (!err.empty()) return err;

            vector<string> translated{};
            if (expanded.find(", ") != string::npos)
            {
                string err = SplitString(expanded, ", ", translated);
                if (!err.empty())
                {
                    return
                        "Failed to parse field '" + fieldBlock.fieldName
                        + "' value '" + expanded + "' in category '" 
                        + finalCategoryContent.first + "' because its reference couldn't be expanded! Reason: " + err;
                }
            }
            else translated.push_back(expanded);

            if (translated.empty()
                || translated[0].empty())
            {
                return
                    "Failed to parse field '" + fieldBlock.fieldName
                    + "' value '" + expanded + "' in category '" 
                    + finalCategoryContent.first + "' because its expanded reference was empty!";
            }
            if (translated.size() > 1)
            {
                return
                    "Failed to parse field '" + fieldBlock.fieldName
                    + "' value '" + expanded + "' in category '" 
                    + finalCategoryContent.first + "' because its expanded reference contained more than one value!";
            }
            expanded = translated[0];

            if ((fieldBlock.fieldName == field_compiler_launcher
                && !ContainsValue(compilerLauncherTypes, expanded))
                || (fieldBlock.fieldName == field_compiler
                && !ContainsValue(compilerTypes, expanded))
                || (fieldBlock.fieldName == field_standard
                && !ContainsValue(standardTypes, expanded))
                || (fieldBlock.fieldName == field_target_type
                && !ContainsValue(targetTypes, expanded))
                || (fieldBlock.fieldName == field_binary_type
                && !ContainsValue(binaryTypes, expanded))
                || (fieldBlock.fieldName == field_build_type
                && !ContainsValue(buildTypes, expanded))
                || (fieldBlock.fieldName == field_warning_level
                && !ContainsValue(warningLevels, expanded)))
            {
                return 
                    "Failed to parse field '" + fieldBlock.fieldName
                    + "' value '" + expanded + "' in category '" 
                    + finalCategoryContent.first + "' because it is invalid!";
            }

            if (fieldBlock.fieldName == field_binary_name
                && !IsStringInRange(expanded, MIN_BINARY_NAME_LENGTH, MAX_BINARY_NAME_LENGTH))
            {
                return 
                    "Failed to parse field '" + fieldBlock.fieldName
                    + "' value '" + expanded + "' in category '" 
                    + finalCategoryContent.first + "' because it is out of range!";
            }

            if (fieldBlock.fieldName == field_build_path)
            {
                path buildPath = absolute(expanded);

                if (exists(buildPath))
                {
                    if (!is_directory(buildPath))
                    {
                        return 
                            "Failed to parse field '" + fieldBlock.fieldName
                            + "' value '" + expanded + "' in category '" 
                            + finalCategoryContent.first + "' because it does not lead to a directory!";
                    }
                }
                else
                {
                    string err = CreateNewDirectory(buildPath);
                    if (!err.empty())
                    {
                        return 
                            "Failed to parse field '" + fieldBlock.fieldName
                            + "' value '" + expanded + "' in category '" 
                            + finalCategoryContent.first + "' because it could not be created as a new build path!";
                    }

                    Log::Print(
                        "Created build directory '" + expanded + "' because it did not exist.",
                        "KMAKE_CORE",
                        LogType::LOG_INFO);
                }
            }

            if (fieldBlock.fieldName == field_jobs)
            {
                bool errState = try_parse_jobs(expanded);
                if (!errState)
                {
                    return 
                        "Failed to parse field '" + fieldBlock.fieldName
                        + "' value '" + expanded + "' in category '" 
                        + finalCategoryContent.first + "' because it could not be cast to u16!";
                }

                if (expanded.starts_with('0'))
                {
                    return 
                        "Failed to parse field '" + fieldBlock.fieldName
                        + "' value '" + expanded + "' in category '" 
                        + finalCategoryContent.first + "' because it was set to 0 or started with 0!";
                }
            }

            if (fieldBlock.fieldName == field_binary_type)
            {
                string _ = StringToEnum(
                    expanded, 
                    binaryTypes, 
                    globalData.targetProfile.binaryType);
            }
            if (fieldBlock.fieldName == field_compiler_launcher)
            {
                string _ = StringToEnum(
                    expanded, 
                    compilerLauncherTypes, 
                    globalData.targetProfile.compilerLauncher);
            }
            if (fieldBlock.fieldName == field_compiler)
            {
                string _ = StringToEnum(
                    expanded, 
                    compilerTypes, 
                    globalData.targetProfile.compiler);
            }
            if (fieldBlock.fieldName == field_standard)
            {
                string _ = StringToEnum(
                    expanded, 
                    standardTypes, 
                    globalData.targetProfile.standard);
            }
            if (fieldBlock.fieldName == field_target_type)
            {
                string _ = StringToEnum(
                    expanded, 
                    targetTypes, 
                    globalData.targetProfile.targetType);
            }
            if (fieldBlock.fieldName == field_jobs)
            {
                globalData.targetProfile.jobs = scast<u16>(stoul(expanded));
            }
            if (fieldBlock.fieldName == field_binary_name)
            {
                globalData.targetProfile.binaryName = expanded;
            }
            if (fieldBlock.fieldName == field_build_type)
            {
                string _ = StringToEnum(
                    expanded, 
                    buildTypes, 
                    globalData.targetProfile.buildType);
            }
            if (fieldBlock.fieldName == field_build_path)
            {
                globalData.targetProfile.buildPath = expanded;
            }
            if (fieldBlock.fieldName == field_warning_level)
            {
                string _ = StringToEnum(
                    expanded, 
                    warningLevels, 
                    globalData.targetProfile.warningLevel);
            }
            if (fieldBlock.fieldName == field_pre_build_action)
            {
                size_t pos{};
                while ((pos = expanded.find("\\\"", pos)) != string::npos)
                {
                    expanded.replace(pos, 2, "\"");
                    ++pos;
                }

                globalData.targetProfile.preBuildActions.push_back(expanded);
            }
            if (fieldBlock.fieldName == field_post_build_action)
            {
                size_t pos{};
                while ((pos = expanded.find("\\\"", pos)) != string::npos)
                {
                    expanded.replace(pos, 2, "\"");
                    ++pos;
                }

                globalData.targetProfile.postBuildActions.push_back(expanded);
            }

            Log::Print("----value '" + expanded + "'", true);
        }

        vector<string> finalValues{};
        for (size_t i = 0; i < fieldBlock.fieldValues.size(); i++)
        {
            const string& value = fieldBlock.fieldValues[i];

            if (fieldBlock.fieldName == field_sources
                || fieldBlock.fieldName == field_headers
                || fieldBlock.fieldName == field_links
                || fieldBlock.fieldName == field_custom_flags
                || fieldBlock.fieldName == field_compile_flags
                || fieldBlock.fieldName == field_link_flags
                || fieldBlock.fieldName == field_defines)
            {
                string expanded{};
                string err = translate_reference(value, expanded);
                if (!err.empty()) return err;

                vector<string> translated{};
                if (expanded.find(", ") != string::npos)
                {
                    string err = SplitString(expanded, ", ", translated);
                    if (!err.empty())
                    {
                        return
                            "Failed to parse field '" + fieldBlock.fieldName
                            + "' value '" + expanded + "' in category '" 
                            + finalCategoryContent.first + "' because its reference couldn't be expanded! Reason: " + err;
                    }
                }
                else translated.push_back(expanded);

                if (translated.empty())
                {
                    return
                        "Failed to parse field '" + fieldBlock.fieldName
                        + "' value '" + expanded + "' in category '" 
                        + finalCategoryContent.first + "' because its expanded reference was empty!";
                }
                for (const string& val : translated)
                {
                    if (val.empty())
                    {
                        return
                            "Failed to parse field '" + fieldBlock.fieldName
                            + "' value '" + val + "' in category '" 
                            + finalCategoryContent.first + "' because it was empty!";
                    }

                    //sources must always exist
                    if (fieldBlock.fieldName == field_sources
                        && !exists(absolute(val)))
                    {
                        return 
                            "Failed to parse field '" + fieldBlock.fieldName
                            + "' value '" + val + "' in category '" 
                            + finalCategoryContent.first + "' because it does not exist!";
                    }

                    //headers must be directories and always exist
                    if (fieldBlock.fieldName == field_headers)
                    {
                        path absPath = absolute(val);
                        if (!exists(absPath))
                        {
                            return 
                                "Failed to parse field '" + fieldBlock.fieldName
                                + "' value '" + val + "' in category '" 
                                + finalCategoryContent.first + "' because it does not exist!";
                        }
                        if (!is_directory(absPath))
                        {
                            return 
                                "Failed to parse field '" + fieldBlock.fieldName
                                + "' value '" + val + "' in category '" 
                                + finalCategoryContent.first + "' because it is not a directory!";
                        }
                    }

                    //links must lead to full path if not system lib
                    if (fieldBlock.fieldName == field_links
                        && path(val).has_parent_path()
                        && !is_regular_file(absolute(val)))
                    {
                        return 
                            "Failed to parse field '" + fieldBlock.fieldName
                            + "' value '" + val + "' in category '" 
                            + finalCategoryContent.first + "' because it does not exist!";
                    }

                    if (fieldBlock.fieldName == field_custom_flags
                        && !ContainsValue(customFlags, val))
                    {
                        return 
                            "Failed to parse field '" + fieldBlock.fieldName
                            + "' value '" + val + "' in category '" 
                            + finalCategoryContent.first + "' because it is invalid!";
                    }

                    Log::Print("----value '" + val + "'", true);
                    finalValues.push_back(val);
                }
            }
        }
        fieldBlock.fieldValues = std::move(finalValues);

        if (fieldBlock.fieldName == field_sources)
        {
            for (const string& value : fieldBlock.fieldValues)
            {
                if (is_directory(value))
                {
                    for (const auto& entry : recursive_directory_iterator(value))
                    {
                        if (!entry.is_regular_file()) continue;

                        const path& file = entry.path();
                        string ext = file.extension().string();

                        if (ext == ".c"
                            || ext == ".cc"
                            || ext == ".cpp")
                        {
                            globalData.targetProfile.sources.push_back(file);
                        }
                    }
                }
                else globalData.targetProfile.sources.push_back(path(value));
            }
        }
        if (fieldBlock.fieldName == field_headers)
        {
            for (const string& value : fieldBlock.fieldValues)
            {
                globalData.targetProfile.headers.push_back(path(value));
            }
        }
        if (fieldBlock.fieldName == field_links)
        {
            for (const string& value : fieldBlock.fieldValues)
            {
                globalData.targetProfile.links.push_back(path(value));
            }
        }
        if (fieldBlock.fieldName == field_custom_flags)
        {
            for (const string& value : fieldBlock.fieldValues)
            {
                CustomFlag flag{};

                string _ = StringToEnum(
                    value, 
                    customFlags, 
                    flag);

                globalData.targetProfile.customFlags.push_back(flag);
            }
        }
        if (fieldBlock.fieldName == field_compile_flags)
        {
            globalData.targetProfile.compileFlags = std::move(fieldBlock.fieldValues);
        }
        if (fieldBlock.fieldName == field_link_flags)
        {
            globalData.targetProfile.linkFlags = std::move(fieldBlock.fieldValues);
        }
        if (fieldBlock.fieldName == field_defines)
        {
            globalData.targetProfile.defines = std::move(fieldBlock.fieldValues);
        }
    }

    return "";
}

string ListProfiles()
{
    return "";
}

string CleanBuildDirs()
{
    return "";
}