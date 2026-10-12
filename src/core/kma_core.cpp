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

#include "log_utils.hpp"
#include "file_utils.hpp"
#include "string_utils.hpp"

#include "language/kma_language.hpp"

using KalaHeaders::KalaCore::ContainsValue;
using KalaHeaders::KalaCore::EnumHash;

using KalaHeaders::KalaLog::Log;
using KalaHeaders::KalaLog::LogType;
using KalaHeaders::KalaLog::TimeFormat;
using KalaHeaders::KalaLog::DateFormat;

using KalaHeaders::KalaFile::ReadLinesFromFile;

using KalaHeaders::KalaString::TrimString;
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
using KalaMake::Core::ProfileData;
using KalaMake::Core::ReferenceData;
using KalaMake::Core::GlobalData;

using std::string;
using std::string_view;
using std::vector;
using std::pair;
using std::unordered_map;

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

    { StandardType::RUST_15,     standard_rust15 },
    { StandardType::RUST_18,     standard_rust18 },
    { StandardType::RUST_21,     standard_rust21 },
    { StandardType::RUST_24,     standard_rust24 }
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

static bool isInHeadlessMode{};
static GlobalData globalData{};

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
        vector<string> kmakeContent{};
        string err = ReadLinesFromFile(params[0], kmakeContent);
        if (!err.empty())
        {
            Log::Print(
                "Failed to parse kmake file '" + params[0] + "'! Reason: " + err,
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
                    "Failed to get data from kmake file '" + params[0] + "'! Reason: " + err);
            }

            if (isInHeadlessMode) exit(0);
            return;
        }

        //TEMPORARY EXIT, REMOVE LATER
        exit(0);

        if (startType == StartType::S_LIST_PROFILES)
        {
            err = ListProfiles();
            if (!err.empty())
            {
                ForceClose(
                    "KalaMake core error",
                    "Failed to list profiles from kmake file '" + params[0] + "'! Reason: " + err);
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
                    "Failed to clean build dirs from kmake file '" + params[0] + "'! Reason: " + err);
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
        if (cat.categoryName == "#version"
            && cat.categoryWords.size() > 1)
        {
            return "Failed to verify categories because category 'version' has more than one value!";
        }
    }

    struct FieldBlock
    {
        string fieldName{};
        vector<string> fieldValues{};
    };

    /*
    TODO: ADD THIS ORDER
    
    Validate field repetition: Is this field allowed to appear multiple times within the same category?
    Validate value counts: Is this field allowed to contain multiple values in a single declaration?
    Validate field names: Does each field actually exist in KalaMake's supported fields?
    Validate actual values: Are those values valid for the specific field?
    Assign: Store the verified configuration, with custom profiles overriding inherited global settings where appropriate.
    */

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

    for (pair<string, vector<FieldBlock>>& catBlock : gatheredData)
    {
        //remove starting '#' from categories
        catBlock.first = catBlock.first.substr(1);
        //remove starting 'profile ' from categories
        string toBeErased = "profile ";
        if (catBlock.first.starts_with(toBeErased))
        {
            catBlock.first = catBlock.first.substr(toBeErased.size());
        }

        vector<string> fields{};

        for (FieldBlock& fieldBlock : catBlock.second)
        {
            if (fieldBlock.fieldValues.empty())
            {
                return 
                    "Failed to parse field '" + fieldBlock.fieldName 
                    + "' in category '" + catBlock.first + "' because it had no values!";
            }

            if (fieldBlock.fieldValues[fieldBlock.fieldValues.size() - 1].ends_with("\","))
            {
                return 
                    "Failed to parse last value in field '" + fieldBlock.fieldName 
                    + "' in category '" + catBlock.first + "' because the last value must not end with a comma!";
            }

            //remove ending ':' from fields
            if (!fieldBlock.fieldName.empty())
            {
                fieldBlock.fieldName = fieldBlock.fieldName.substr(0, fieldBlock.fieldName.size() - 1);
            }

            //non-references category allows to have more than one postbuildcation,
            //references category does not allow to have more
            if (ContainsValue(fields, fieldBlock.fieldName)
                && (fieldBlock.fieldName != "postbuildaction"
                || (catBlock.first == "references"
                && fieldBlock.fieldName == "postbuildaction")))
            {
                return 
                    "Failed to parse field '" + fieldBlock.fieldName 
                    + "' in category '" + catBlock.first + "' because it was added more than once to the category!";
            }

            fields.push_back(fieldBlock.fieldName);

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