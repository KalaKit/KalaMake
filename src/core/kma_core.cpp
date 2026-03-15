//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#if _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <sstream>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>
#include <functional>

#include "core_utils.hpp"
#include "log_utils.hpp"
#include "file_utils.hpp"
#include "string_utils.hpp"

#include "kc_core.hpp"

#include "language/kma_language.hpp"
#include "core/kma_core.hpp"

using KalaHeaders::KalaCore::EnumHash;
using KalaHeaders::KalaCore::IsComparable;
using KalaHeaders::KalaCore::IsAssignable;
using KalaHeaders::KalaCore::AnyEnumAndStringMap;
using KalaHeaders::KalaCore::AnyEnum;
using KalaHeaders::KalaCore::StringToEnum;
using KalaHeaders::KalaCore::RemoveDuplicates;

using KalaHeaders::KalaLog::Log;
using KalaHeaders::KalaLog::LogType;

using KalaHeaders::KalaFile::ReadLinesFromFile;
using KalaHeaders::KalaFile::ResolveAnyPath;
using KalaHeaders::KalaFile::ToStringVector;
using KalaHeaders::KalaFile::ToPathVector;
using KalaHeaders::KalaFile::PathTarget;
using KalaHeaders::KalaFile::CreateNewDirectory;

using KalaHeaders::KalaString::SplitString;
using KalaHeaders::KalaString::ReplaceAfter;
using KalaHeaders::KalaString::TrimString;
using KalaHeaders::KalaString::HasAnyWhiteSpace;
using KalaHeaders::KalaString::HasAnyUnsafeFieldChar;
using KalaHeaders::KalaString::HasAnyWhiteSpace;

using KalaMake::Core::KalaMakeCore;
using KalaMake::Core::ReferenceData;
using KalaMake::Core::GlobalData;
using KalaMake::Core::Version;
using KalaMake::Core::CategoryType;
using KalaMake::Core::FieldType;
using KalaMake::Core::CompilerLauncherType;
using KalaMake::Core::CompilerType;
using KalaMake::Core::StandardType;
using KalaMake::Core::TargetType;
using KalaMake::Core::BuildType;
using KalaMake::Core::BinaryType;
using KalaMake::Core::WarningLevel;
using KalaMake::Core::CustomFlag;
using KalaMake::Language::LanguageCore;

using std::ostringstream;
using std::filesystem::exists;
using std::filesystem::path;
using std::filesystem::current_path;
using std::filesystem::weakly_canonical;
using std::filesystem::is_directory;
using std::filesystem::is_regular_file;
using std::filesystem::recursive_directory_iterator;
using std::filesystem::filesystem_error;
using std::string;
using std::string_view;
using std::to_string;
using std::vector;
using std::unordered_map;
using std::function;

using u16 = uint16_t;

constexpr string_view version_1_0 = "1.0";

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

constexpr string_view standard_c89        = "c89";
constexpr string_view standard_c99        = "c99";
constexpr string_view standard_c11        = "c11";
constexpr string_view standard_c17        = "c17";
constexpr string_view standard_c23        = "c23";
constexpr string_view standard_c_latest   = "clatest";
constexpr string_view standard_cpp98      = "c++98";
constexpr string_view standard_cpp03      = "c++03";
constexpr string_view standard_cpp11      = "c++11";
constexpr string_view standard_cpp14      = "c++14";
constexpr string_view standard_cpp17      = "c++17";
constexpr string_view standard_cpp20      = "c++20";
constexpr string_view standard_cpp23      = "c++23";
constexpr string_view standard_cpp26      = "c++26";
constexpr string_view standard_cpp_latest = "c++latest";

constexpr string_view target_type_windows = "windows";
constexpr string_view target_type_linux   = "linux";

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
constexpr string_view custom_clang_zig_msvc      = "use-clang-zig-msvc";
constexpr string_view custom_msvc_static_runtime = "msvc-static-runtime";

//kma path is the root directory where the kmake file is stored at
static path kmaPath{};

static string targetProfile{};

static bool foundVersion{};
static bool foundReferences{};
static bool foundGlobal{};
static bool foundTargetProfile{};

static GlobalData globalData{};

static u16 GetThreadCount()
{
#if _WIN32
	return scast<u16>(GetActiveProcessorCount(ALL_PROCESSOR_GROUPS));
#else
	return scast<u16>(sysconf(_SC_NPROCESSORS_ONLN));
#endif
};

static void CleanEverything()
{
	foundVersion = false;
	foundReferences = false;
	foundGlobal = false;

	globalData = GlobalData{};
}

template<AnyEnumAndStringMap T>
static bool EnumMapContainsValue(
	const T& map,
	string_view value,
	string_view valueName)
{
	if (value.empty())
	{
		KalaMakeCore::CloseOnError(
			"KALAMAKE",
			string(valueName) + " cannot be empty!");

		return false;
	}

	typename T::key_type foundEnum{};

	bool result = StringToEnum(
		value,
		map,
		foundEnum);

	if (!result)
	{
		KalaMakeCore::CloseOnError(
			"KALAMAKE",
			string(valueName) + " did not contain enum that matched requested value '" + string(value) + "'!");

		return false;
	}

	return true;
}

template<AnyEnumAndStringMap T, AnyEnum E>
	requires (
		IsComparable<typename T::mapped_type, string_view>
		&& IsAssignable<E&, typename T::key_type>)
static bool GetEnumFromMap(
	const T& map, 
	string_view value, 
	string_view valueName,
	E& outEnum)
{
	if (value.empty())
	{
		KalaMakeCore::CloseOnError(
			"KALAMAKE",
			string(valueName) + " cannot be empty!");

		return false;
	}

	E foundEnum{};

	bool result = StringToEnum(
		value,
		map,
		foundEnum);

	if (!result)
	{
		KalaMakeCore::CloseOnError(
			"KALAMAKE",
			string(valueName) + " did not contain enum that matched requested value '" + string(value) + "'!");

		return false;
	}

	outEnum = foundEnum;
	return true;
}

static void ExtractCategoryData(
	const string& line,
	string& outCategoryName,
	string& outCategoryValue)
{
	if (!line.starts_with('#')) return;

	string newLine = line;
	
	newLine.erase(0,  1);
	if (newLine.empty())
	{
		KalaMakeCore::CloseOnError(
			"KALAMAKE",
			"Failed to resolve category '" + line + "' because it had no type or value!");
	}

	size_t spacePos = newLine.find(' ');
	string name = newLine.substr(0, spacePos);

	auto is_valid_category = [](string_view name) -> bool
	{
		CategoryType type{};
		return 
			(GetEnumFromMap(KalaMakeCore::GetCategoryTypes(), name, "Category", type)
			&& type != CategoryType::C_INVALID);
	};

	if (!is_valid_category(name))
	{
		KalaMakeCore::CloseOnError(
			"KALAMAKE",
			"Failed to resolve category '" + line + "' because it does not exist!");
	}

	//early exit for valueless ref and global categories
	if (newLine == category_references
		|| newLine == category_global)
	{
		outCategoryName = newLine;
		return;
	}

	//space must exist after '#profile' and '#version'
	if (spacePos == string::npos)
	{
		KalaMakeCore::CloseOnError(
			"KALAMAKE",
			"Failed to resolve category '" + line + "' because its value was empty!");
	}

	//must contain non-space value after '#profile ' and '#version '
	size_t valueStart = newLine.find_first_not_of(' ', spacePos + 1);
	if (valueStart == string::npos)
	{
		KalaMakeCore::CloseOnError(
			"KALAMAKE",
			"Failed to resolve category '" + line + "' because its value was empty!");
	}

	outCategoryName = name;
	outCategoryValue = newLine.substr(valueStart);
}

static void ExtractFieldData(
	const string& line,
	string& outFieldName,
	vector<string>& outFieldValues,
	bool isReference = false);

static void FirstParse(const vector<string>& lines);

static string TranslateReferences(string_view value);

namespace KalaMake::Core
{
	static const unordered_map<Version, string_view, EnumHash<Version>> versions =
	{
		{ Version::V_1_0, version_1_0 }
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

		{ CompilerType::C_CLANG,    compiler_clang },
		{ CompilerType::C_CLANGPP,  compiler_clangpp },
		{ CompilerType::C_GCC,      compiler_gcc },
		{ CompilerType::C_GPP,      compiler_gpp }
	};

	static const unordered_map<StandardType, string_view, EnumHash<StandardType>> standardTypes =
	{
		{ StandardType::C_89,     standard_c89 },
		{ StandardType::C_99,     standard_c99 },
		{ StandardType::C_11,     standard_c11 },
		{ StandardType::C_17,     standard_c17 },
		{ StandardType::C_23,     standard_c23 },
		{ StandardType::C_LATEST, standard_c_latest },

		{ StandardType::CPP_98,      standard_cpp98 },
		{ StandardType::CPP_03,      standard_cpp03 },
		{ StandardType::CPP_11,      standard_cpp11 },
		{ StandardType::CPP_14,      standard_cpp14 },
		{ StandardType::CPP_17,     standard_cpp17 },
		{ StandardType::CPP_20,     standard_cpp20 },
		{ StandardType::CPP_23,     standard_cpp23 },
		{ StandardType::CPP_26,     standard_cpp26 },
		{ StandardType::CPP_LATEST, standard_cpp_latest }
	};

	static const unordered_map<TargetType, string_view, EnumHash<TargetType>> targetTypes =
	{
		{ TargetType::T_WINDOWS, target_type_windows },
		{ TargetType::T_LINUX,   target_type_linux }
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
		{ CustomFlag::F_EXPORT_COMPILE_COMMANDS,     custom_export_comp_comm },
		{ CustomFlag::F_EXPORT_VSCODE_SLN,           custom_export_vscode_sln },
		{ CustomFlag::F_WARNINGS_AS_ERRORS,          custom_warnings_as_err },
		{ CustomFlag::F_USE_CLANG_ZIG_MSVC,          custom_clang_zig_msvc },
		{ CustomFlag::F_MSVC_STATIC_RUNTIME,         custom_msvc_static_runtime }
	};

	void KalaMakeCore::OpenFile(const vector<string>& params)
	{
		ostringstream details{};

		Log::Print(details.str());

		path projectFile = params[1];
		targetProfile = params[2];

		string& currentDir = KalaCLI::Core::GetCurrentDir();
		if (currentDir.empty()) currentDir = current_path().string();

		auto handle_state = [](path filePath) -> void
			{
				if (is_directory(filePath))
				{
					KalaMakeCore::CloseOnError(
						"KALAMAKE",
						"Project path '" + filePath.string() + "' leads to a directory!");
				}

				if (!filePath.has_extension()
					|| filePath.extension() != ".kmake")
				{
					KalaMakeCore::CloseOnError(
						"KALAMAKE",
						"Project path '" + filePath.string() + "' has an incorrect extension!");
				}

				vector<string> content{};

				string result = ReadLinesFromFile(
					filePath,
					content);

				if (!result.empty())
				{
					KalaMakeCore::CloseOnError(
						"KALAMAKE",
						"Project '" + filePath.string() + "' could not be compiled! Reason: " + result);
				}

				if (content.empty())
				{
					KalaMakeCore::CloseOnError(
						"KALAMAKE",
						"Project '" + filePath.string() + "' was empty!");
				}

				kmaPath = filePath.parent_path();
				globalData.projectFile = filePath;

				Compile(filePath, content);
			};

		//partial path was found

		path correctTarget{};

		try
		{
			correctTarget = weakly_canonical(path(KalaCLI::Core::GetCurrentDir()) / projectFile);
		}
		catch (const filesystem_error&)
		{
			KalaMakeCore::CloseOnError(
				"KALAMAKE",
				"Project partial path via '" + projectFile.string() + "' could not be resolved!");
		}

		if (exists(correctTarget))
		{
			handle_state(correctTarget);

			return;
		}

		//full path was found

		try
		{
			correctTarget = weakly_canonical(projectFile);
		}
		catch (const filesystem_error&)
		{
			KalaMakeCore::CloseOnError(
				"KALAMAKE",
				"Project full path '" + projectFile.string() + "' could not be resolved!");
		}

		if (exists(correctTarget))
		{
			handle_state(correctTarget);

			return;
		}

		KalaMakeCore::CloseOnError(
			"KALAMAKE",
			"Project path '" + projectFile.string() + "' does not exist!");
	}

	void KalaMakeCore::Compile(
		const path& filePath,
		const vector<string>& lines)
	{
		Log::Print(
			"Starting to parse the kalamake file '" + filePath.string() + "'"
			"\n\n===========================================================================\n",
			"KALAMAKE",
			LogType::LOG_INFO);

		FirstParse(lines);

		if (globalData.targetProfile.binaryType == BinaryType::B_INVALID)
		{
			KalaMakeCore::CloseOnError(
				"KALAMAKE",
				"No binary type was passed!");
		}
		if (globalData.targetProfile.compiler == CompilerType::C_INVALID)
		{
			KalaMakeCore::CloseOnError(
				"KALAMAKE",
				"No compiler was passed!");
		}
		if (globalData.targetProfile.standard == StandardType::S_INVALID)
		{
			KalaMakeCore::CloseOnError(
				"KALAMAKE",
				"No standard was passed!");
		}
		
		if (globalData.targetProfile.binaryName.empty())
		{
			KalaMakeCore::CloseOnError(
				"KALAMAKE",
				"No binary name was passed!");
		}
		if (globalData.targetProfile.buildType == BuildType::B_INVALID)
		{
			KalaMakeCore::CloseOnError(
				"KALAMAKE",
				"No build type was passed!");
		}
		if (globalData.targetProfile.buildPath.empty())
		{
			KalaMakeCore::CloseOnError(
				"KALAMAKE",
				"No build path was passed!");
		}
		if (globalData.targetProfile.sources.empty())
		{
			KalaMakeCore::CloseOnError(
				"KALAMAKE",
				"No sources were passed!");
		}

		//assign cpu thread count if none was assigned
		if (globalData.targetProfile.jobs == 0) globalData.targetProfile.jobs = GetThreadCount();

		Log::Print(
			"Using '" + to_string(globalData.targetProfile.jobs) + "' jobs for compilation.\n",
			"KALAMAKE",
			LogType::LOG_INFO);

		Log::Print("===========================================================================\n");

		Log::Print(
			"Finished first parse! Cleaning up parsed data and parsing for compiler.\n",
			"KALAMAKE",
			LogType::LOG_SUCCESS);

		LanguageCore::Compile(globalData);
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

    void KalaMakeCore::CloseOnError(
		string_view target,
		string_view message)
	{
		Log::Print(
			message,
			target,
			LogType::LOG_ERROR,
			2);

		exit(1);
	}
}

void ExtractFieldData(
	const string& line,
	string& outFieldName,
	vector<string>& outFieldValues,
	bool isReference)
{
	if (line.find(": ") == string::npos)
	{
		KalaMakeCore::CloseOnError(
			"KALAMAKE",
			"Failed to resolve field '" + line + "' because it is missing its name and value separator!");
	}

	vector<string> split = SplitString(line, ": ");

	if (split.size() > 2)
	{
		KalaMakeCore::CloseOnError(
			"KALAMAKE",
			"Failed to resolve field '" + line + "' because it has more than one name and value separator!");
	}

	string name = split[0];
	string trimmedValue = TrimString(split[1]);

	if (HasAnyWhiteSpace(name))
	{
		KalaMakeCore::CloseOnError(
			"KALAMAKE",
			"Field name '" + name + "' cannot have spaces!");
	}
	if (HasAnyUnsafeFieldChar(name))
	{
		KalaMakeCore::CloseOnError(
			"KALAMAKE",
			"Field name '" + name + "' must only contain A-Z, a-z, 0-9, _ or -!");
	}

	FieldType t{};
	bool searchSuccess = StringToEnum(name, KalaMakeCore::GetFieldTypes(), t);

	if (!isReference
		&& (!searchSuccess
		|| t == FieldType::T_INVALID))
	{
		KalaMakeCore::CloseOnError(
			"KALAMAKE",
			"Field '" + name  + "' is invalid!");
	}
	else if (isReference
			 && searchSuccess
			 && t != FieldType::T_INVALID)
	{
		KalaMakeCore::CloseOnError(
			"KALAMAKE",
			"Field '" + name  + "' cannot be used for reference field names!");
	}

	auto require_quotes = [](const string& input) -> string
		{
			if (input.empty())
			{
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"Failed to parse path! Cannot remove '\"' from empty path.");
			}

			if (input.size() <= 2)
			{
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"Failed to parse path! Input path '" + input + "' was too small.");
			}

			if (input.front() != '"'
				|| input.back() != '"')
			{
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"Failed to parse path! Input path '" + input + "' did not have the '\"' symbol at the front or back.");
			}

			string result = input;

			result.erase(0, 1);
			result.pop_back();

			return result;
		};

	//
	// PARSE FIELD
	//

	if (name == field_build_path)
	{
		if (trimmedValue.empty())
		{
			KalaMakeCore::CloseOnError(
				"KALAMAKE",
				"Build path must have a value!");
		}

		if (trimmedValue.find(',') != string::npos)
		{
			KalaMakeCore::CloseOnError(
				"KALAMAKE",
				"Build path '" + trimmedValue  + "' is not allowed to have more than one path!");
		}
		if (trimmedValue.find('*') != string::npos)
		{
			KalaMakeCore::CloseOnError(
				"KALAMAKE",
				"Build path '" + trimmedValue + "' is not allowed to use wildcards!");
		}

		if (trimmedValue.starts_with('"'))
		{
			if (!trimmedValue.ends_with('"'))
			{
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"Build path '" + trimmedValue + "' must end with quotes!");
			}

			trimmedValue = TranslateReferences(require_quotes(trimmedValue));

			vector<path> resolvedPaths{};
			if (trimmedValue.find('*') == string::npos
				&& !exists(trimmedValue))
			{
				string errorMsg = CreateNewDirectory(trimmedValue);
							
				if (!errorMsg.empty())
				{
					KalaMakeCore::CloseOnError(
						"KALAMAKE",
						"Build path '" + trimmedValue + "' could not be created! Reason: " + errorMsg);
				}

				resolvedPaths = { trimmedValue };
			}
			else
			{
				string errorMsg = ResolveAnyPath(
					trimmedValue, 
					kmaPath.string(), 
					resolvedPaths);

				if (!errorMsg.empty())
				{
					KalaMakeCore::CloseOnError(
						"KALAMAKE",
						"Build path '" + trimmedValue + "' could not be resolved! Reason: " + errorMsg);
				}
			}

			vector<string> result{};
			ToStringVector(resolvedPaths, result);

			outFieldName = name;
			outFieldValues = result;
		}
		else
		{
			KalaMakeCore::CloseOnError(
				"KALAMAKE",
					
				"Build path '" + trimmedValue + "' has an illegal structure!");
		}
	}
	else if (name == field_sources
			 || name == field_headers)
	{
		//early exit for empty value
		if (trimmedValue.empty())
		{
			outFieldName = name;
			return;
		}

		vector<string> splitPaths = SplitString(TranslateReferences(trimmedValue), ", ");

		vector<string> result{};

		for (const string& l : splitPaths)
		{
			string trimmedLine = TrimString(l);

			if (trimmedLine.starts_with('"'))
			{
				if (!trimmedLine.ends_with('"'))
				{
					KalaMakeCore::CloseOnError(
						"KALAMAKE",
						"Source or header path '" + trimmedLine + "' must end with quotes!");
				}

				string cleanedValue = require_quotes(trimmedLine);

				vector<string> resolvedStringPaths{};
				vector<path> resolvedPaths{};

				if (name == field_sources)
				{
					string errorMsg = ResolveAnyPath(
						cleanedValue, 
						kmaPath.string(), 
						resolvedPaths,
						PathTarget::P_FILE_ONLY);

					if (!errorMsg.empty())
					{
						KalaMakeCore::CloseOnError(
							"KALAMAKE",
							"Source value '" + cleanedValue + "' could not be resolved! Reason: " + errorMsg);
					}
				}
				else 
				{
					string errorMsg = ResolveAnyPath(
						cleanedValue, 
						kmaPath.string(), 
						resolvedPaths);

					if (!errorMsg.empty())
					{
						KalaMakeCore::CloseOnError(
							"KALAMAKE",
							"Header value '" + cleanedValue + "' could not be resolved! Reason: " + errorMsg);
					}

					for (const auto& p : resolvedPaths)
					{
						if (!is_directory(p))
						{
							KalaMakeCore::CloseOnError(
								"KALAMAKE",
								"Header value '" + p.string() + "' must be a directory!");
						}
					}
				}

				if (name == field_sources)
				{
					auto dir_to_scripts = [](const path& dir) -> vector<path>
						{
							vector<path> out{};

							for (const auto& f : recursive_directory_iterator(dir))
							{
								if (is_regular_file(f)) out.push_back(f);
							}

							return out;
						};

					vector<path> sourceFiles{};

					for (const auto& p : resolvedPaths)
					{
						if (is_directory(p))
						{
							vector<path> localSrc = dir_to_scripts(p);
							sourceFiles.insert(
								sourceFiles.end(),
								make_move_iterator(localSrc.begin()),
								make_move_iterator(localSrc.end()));
						}
						else result.push_back(p.string());
					}

					vector<string> stringSourceFiles{};
					ToStringVector(sourceFiles, stringSourceFiles);

					result.insert(
						result.end(),
						make_move_iterator(stringSourceFiles.begin()),
						make_move_iterator(stringSourceFiles.end()));
				}
				else
				{
					ToStringVector(resolvedPaths, resolvedStringPaths);

					result.insert(
						result.end(),
						make_move_iterator(resolvedStringPaths.begin()),
						make_move_iterator(resolvedStringPaths.end()));
				}
			}
			else
			{
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"Source or header value '" + trimmedLine + "' has an illegal structure!");
			}
		}

		RemoveDuplicates(result);

		outFieldName = name;
		outFieldValues = result;
	}
	else if (name == field_links)
	{
		//early exit for empty value
		if (trimmedValue.empty())
		{
			outFieldName = name;
			return;
		}

		vector<string> splitPaths = SplitString(TranslateReferences(trimmedValue), ", ");

		vector<string> result{};

		auto resolve_line = [require_quotes](string& trimmedLine) -> vector<string>
			{
				if (trimmedLine.starts_with('"'))
				{
					if (!trimmedLine.ends_with('"'))
					{
						KalaMakeCore::CloseOnError(
							"KALAMAKE",
							"Link path '" + trimmedLine + "' must end with quotes!");
					}

					trimmedLine = require_quotes(trimmedLine);

					vector<string> resolvedStringPaths{};
					vector<path> resolvedPaths{};

					string errorMsg = ResolveAnyPath(
							trimmedLine, 
							kmaPath.string(), 
							resolvedPaths,
							PathTarget::P_FILE_ONLY);

					if (!errorMsg.empty())
					{
						KalaMakeCore::CloseOnError(
							"KALAMAKE",
							"Link path '" + trimmedLine + "' could not be resolved! Reason: " + errorMsg);
					}

					ToStringVector(resolvedPaths, resolvedStringPaths);

					return resolvedStringPaths;
				}
				else
				{
#ifdef __linux__
					if (trimmedLine.starts_with("lib"))
					{
						KalaMakeCore::CloseOnError(
							"KALAMAKE",
							"Link system path '" + trimmedLine + "' must not start with 'lib'!");
					}
#endif

					if (path(trimmedLine).has_extension())
					{
						KalaMakeCore::CloseOnError(
							"KALAMAKE",
							"Link system path '" + trimmedLine + "' is not allowed to use extensions!");
					}

					return { trimmedLine };
				}

				return {};
			};

		for (const string& l : splitPaths)
		{
			string trimmedLine = TrimString(l);

			vector<string> cleanedStrings = resolve_line(trimmedLine);

			result.insert(
				result.end(),
				make_move_iterator(cleanedStrings.begin()),
				make_move_iterator(cleanedStrings.end()));
		}

		RemoveDuplicates(result);

		outFieldName = name;
		outFieldValues = result;
	}
	//any field in references category
	else if (isReference)
	{
		if (trimmedValue.empty())
		{
			KalaMakeCore::CloseOnError(
				"KALAMAKE",
				"Reference '" + name + "' must have a value!");
		}

		outFieldName = name;
		outFieldValues = { trimmedValue };
	}
	else if (name == field_post_build_action)
	{
		if (trimmedValue.find(',') != string::npos)
		{
			KalaMakeCore::CloseOnError(
				"KALAMAKE",
				"Post build action '" + name  + "' is not allowed to have more than one value!");
		}

		outFieldName = name;
		outFieldValues = { TranslateReferences(trimmedValue) };
	}
	//all other standard fields with no paths
	else 
	{
		if (name != field_defines
			&& trimmedValue.find('"') != string::npos)
		{
			KalaMakeCore::CloseOnError(
				"KALAMAKE",
				"Field '" + name + "' is not allowed to use quotes!");
		}
		if (trimmedValue.find('*') != string::npos)
		{
			KalaMakeCore::CloseOnError(
				"KALAMAKE",
				"Field '" + name + "' is not allowed to use wildcards!");
		}

		//these fields must have a value
		if ((name == field_binary_type
			|| name == field_build_type
			|| name == field_compiler
			|| name == field_standard)
			&& trimmedValue.empty())
		{
			KalaMakeCore::CloseOnError(
				"KALAMAKE",
				"Field '" + name + "' must have a value!");
		}

		if ((name == field_binary_type
			|| name == field_compiler_launcher
			|| name == field_compiler
			|| name == field_standard
			|| name == field_target_type
			|| name == field_binary_name
			|| name == field_warning_level)
			&& trimmedValue.find(",") != string::npos)
		{
			KalaMakeCore::CloseOnError(
				"KALAMAKE",
				"Field '" + name + "' is not allowed to have more than one value!");
		}

		string cleanValue = TranslateReferences(trimmedValue);

		if (name == field_jobs
			&& !cleanValue.empty())
		{
			unsigned long parsed{};

			try
			{
				parsed = scast<int>(stoul(cleanValue));
			}
			catch (...)
			{
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"Jobs value must contain a valid unsigned integer!");
			}
			
			int jobs = scast<int>(parsed);

			if (jobs <= 0)
			{
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"Jobs value count must be 1 or greater!");
			}
			if (jobs > UINT16_MAX)
			{
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"Jobs value count must be less than 65536!");
			}

			if (jobs > (GetThreadCount() * 2))
			{
				Log::Print(
					"Jobs count exceeds double the thread count, issues may occur during compilation.",
					"KALAMAKE",
					LogType::LOG_WARNING);
			}
		}

		if (name == field_binary_type)
		{
			const auto& binaryTypes = KalaMakeCore::GetBinaryTypes();

			BinaryType binaryType{};
			if (!StringToEnum(cleanValue, binaryTypes, binaryType)
				|| binaryType == BinaryType::B_INVALID)
			{
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"Binary type '" + trimmedValue + "' is invalid!");
			}
		}
		if (name == field_compiler_launcher)
		{
			const auto& compilerLauncherTypes = KalaMakeCore::GetCompilerLauncherTypes();

			CompilerLauncherType compilerLauncherType{};
			if (!StringToEnum(cleanValue, compilerLauncherTypes, compilerLauncherType)
				|| compilerLauncherType == CompilerLauncherType::C_INVALID)
			{
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"Compiler launcher type '" + trimmedValue + "' is invalid!");
			}
		}
		if (name == field_compiler)
		{
			const auto& compilerTypes = KalaMakeCore::GetCompilerTypes();

			CompilerType compilerType{};
			if (!StringToEnum(cleanValue, compilerTypes, compilerType)
				|| compilerType == CompilerType::C_INVALID)
			{
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"Compiler type '" + trimmedValue + "' is invalid!");
			}
		}
		if (name == field_standard)
		{
			const auto& standardTypes = KalaMakeCore::GetStandardTypes();

			StandardType standardType{};
			if (!StringToEnum(cleanValue, standardTypes, standardType)
				|| standardType == StandardType::S_INVALID)
			{
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"Standard type '" + trimmedValue + "' is invalid!");
			}
		}
		if (name == field_target_type)
		{
			const auto& targetTypes = KalaMakeCore::GetTargetTypes();

			TargetType targetType{};
			if (!StringToEnum(cleanValue, targetTypes, targetType)
				|| targetType == TargetType::T_INVALID)
			{
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"Target type '" + trimmedValue + "' is invalid!");
			}
		}
		if (name == field_build_type)
		{
			const auto& buildTypes = KalaMakeCore::GetBuildTypes();

			BuildType buildType{};
			if (!StringToEnum(cleanValue, buildTypes, buildType)
				|| buildType == BuildType::B_INVALID)
			{
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"Build type '" + trimmedValue + "' is invalid!");
			}
		}
		if (name == field_warning_level)
		{
			const auto& warningLevels = KalaMakeCore::GetWarningLevels();

			WarningLevel warningLevel{};
			if (!StringToEnum(cleanValue, warningLevels, warningLevel)
				|| warningLevel == WarningLevel::W_INVALID)
			{
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"Warning level '" + trimmedValue + "' is invalid!");
			}
		}

		vector<string> result{};
		if (cleanValue.find(',') != string::npos)
		{
			result = SplitString(cleanValue, ", ");
		}
		else result.push_back(cleanValue);

		RemoveDuplicates(result);

		if (name == field_custom_flags)
		{
			const auto& customFlags = KalaMakeCore::GetCustomFlags();

			for (const auto& r : result)
			{
				CustomFlag customFlag{};
				if (!StringToEnum(r, customFlags, customFlag)
					|| customFlag == CustomFlag::F_INVALID)
				{
					KalaMakeCore::CloseOnError(
						"KALAMAKE",
						"Custom flag '" + trimmedValue + "' is invalid!");
				}
			}
		}

		outFieldName = name;
		outFieldValues = result;
	}	

	if (outFieldValues.empty())
	{
		Log::Print(
			"Field '" + outFieldName + "' was parsed correctly and had no values.",
			"KALAMAKE",
			LogType::LOG_INFO);
	}
	else
	{
		string msg = "Field '" + outFieldName + "' was parsed correctly, found values:\n";
		for (size_t i = 0; i < outFieldValues.size(); ++i)
		{
			string_view v = outFieldValues[i];

			msg += "    " + string(v);
			if (i != outFieldValues.size() - 1) msg += "\n";
		}

		Log::Print(
			msg,
			"KALAMAKE",
			LogType::LOG_INFO);
	}
}

void FirstParse(const vector<string>& lines)
{
	//always a fresh build
	CleanEverything();

	auto get_all_category_content = [&lines](
		string_view categoryName,
		string_view categoryValue = {}) -> vector<string>
		{
			bool collecting{};
			vector<string> collected{};

			for (const string& li : lines)
			{
				if (li.empty()
					|| li.starts_with("//"))
				{
					continue;
				}

				string cli = TrimString(ReplaceAfter(li, "//"));
				if (cli.empty()) continue;

				if (!collecting)
				{
					if (categoryValue.empty())
					{
						if (cli == "#" + string(categoryName)) collecting = true;
					}
					else if (cli == "#" + string(categoryName) + " " + string(categoryValue)) collecting = true;
					
					continue;
				}

				if (cli[0] == '#') break;

				collected.push_back(cli);
			}

			return collected;
		};

	const auto& categoryTypes = KalaMakeCore::GetCategoryTypes();
	const auto& versions = KalaMakeCore::GetVersions();

	auto clean_line = [&categoryTypes](
		string& line, 
		string& name, 
		string& value, 
		CategoryType& type) -> void
		{
			if (line.empty()
				|| line.starts_with("//")
				|| !line.starts_with('#'))
			{
				return;
			}

			line = TrimString(ReplaceAfter(line, "//"));

			ExtractCategoryData(
				line, 
				name,
				value);

			if (!StringToEnum(name, categoryTypes, type)
				|| type == CategoryType::C_INVALID)
			{
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"Category type '" + name + "' is invalid!");
			}
		};

	string correctTargetProfile{};
	for (const string& l : lines)
	{
		string line = l;
		string name{};
		string value{};

		CategoryType type{};
		clean_line(line, name, value, type);

		if (type == CategoryType::C_PROFILE)
		{
			if (value == "global")
			{
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"User profile name is not allowed to be 'global'!");
			}

			if (HasAnyUnsafeFieldChar(value))
			{
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"User profile name '" + value + "' must only contain A-Z, a-z, 0-9, _ or -!");
			}

			if (value != targetProfile) continue;
			else
			{
				foundTargetProfile = true;
				correctTargetProfile = value;

				break;
			}
		}
	}
	if (!foundTargetProfile)
	{
		for (const string& l : lines)
		{
			string line = l;
			string name{};
			string value{};

			CategoryType type{};
			clean_line(line, name, value, type);

			if (type == CategoryType::C_GLOBAL
				&& targetProfile == "global")
			{
				foundTargetProfile = true;
				correctTargetProfile = targetProfile;
				break;
			}
		}
	}

	if (!foundTargetProfile)
	{
		KalaMakeCore::CloseOnError(
			"KALAMAKE",
			"Target profile '" + targetProfile + "' was not found!");
	}

	if (!foundVersion)
	{
		for (const string& l : lines)
		{
			string line = l;
			string name{};
			string value{};

			CategoryType type{};
			clean_line(line, name, value, type);

			if (type == CategoryType::C_VERSION)
			{
				if (foundVersion)
				{
					KalaMakeCore::CloseOnError(
						"KALAMAKE",
						"Version category was passed more than once!");
				}

				Version v{};
				bool convertVersion = StringToEnum(
					value, 
					versions, 
					v);

				if (!convertVersion
					|| v == Version::V_INVALID)
				{
					KalaMakeCore::CloseOnError(
						"KALAMAKE",
						"Version '" + value  + "' is invalid!");
				}

				Log::Print(
					"Found valid version '" + value + "'",
					"KALAMAKE",
					LogType::LOG_INFO);

				foundVersion = true;

				break;
			}
		}

		if (!foundVersion)
		{
			KalaMakeCore::CloseOnError(
				"KALAMAKE",
				"Failed to find version!");
		}
	}

	if (!foundReferences)
	{
		for (const string& l : lines)
		{
			string line = l;
			string name{};
			string value{};

			CategoryType type{};
			clean_line(line, name, value, type);

			if (type == CategoryType::C_REFERENCES)
			{
				Log::Print(
					"\n---------------------------------------------------------------------------"
					"\n# Starting to parse references category\n"
					"---------------------------------------------------------------------------\n");

				if (foundReferences)
				{
					KalaMakeCore::CloseOnError(
						"KALAMAKE",
						"References category was used more than once!");
				}

				vector<string> content = get_all_category_content(name);

				unordered_map<string, path> fields{};
				for (const auto& c : content)
				{
					string fieldName{};
					vector<string> fieldValues{};
					ExtractFieldData(
						c, 
						fieldName, 
						fieldValues,
						true);

					if (fields.contains(fieldName))
					{
						KalaMakeCore::CloseOnError(
							"KALAMAKE",
							"Reference field '" + fieldName + "' was duplicated!");
					}

					fields[fieldName] = fieldValues[0];
				}

				globalData.references.reserve(globalData.references.size() + fields.size());

				for (const auto& [k, v] : fields)
				{
					ReferenceData ref
					{
						.name = k,
						.value = v.string()
					};

					globalData.references.push_back(ref);	
				}

				foundReferences = true;

				break;
			}
		}
	}

	if (!foundGlobal)
	{
		for (const string& l : lines)
		{
			string line = l;
			string name{};
			string value{};

			CategoryType type{};
			clean_line(line, name, value, type);

			if (type == CategoryType::C_GLOBAL)
			{
				Log::Print(
					"\n---------------------------------------------------------------------------"
					"\n# Starting to parse global profile\n"
					"---------------------------------------------------------------------------\n");

				if (foundGlobal)
				{
					KalaMakeCore::CloseOnError(
						"KALAMAKE",
						"Global category was used more than once!");
				}
				
				vector<string> content = get_all_category_content(name);

				unordered_map<string, vector<string>> fields{};
				for (const auto& c : content)
				{
					string fieldName{};
					vector<string> fieldValues{};
					ExtractFieldData(
						c, 
						fieldName, 
						fieldValues);

					if (fields.contains(fieldName)
						&& fieldName != field_post_build_action)
					{
						KalaMakeCore::CloseOnError(
							"KALAMAKE",
							"Field '" + fieldName + "' was duplicated!");
					}

					if (fieldName != field_post_build_action) fields[fieldName] = fieldValues;
					else
					{
						if (!fields.contains(string(field_post_build_action)))
						{
							fields[fieldName] = fieldValues;
						}
						else fields[fieldName].push_back(fieldValues[0]);
					}
				}

				globalData.targetProfile.profileName = value;
				if (fields.contains(string(field_binary_type)))
				{
					const vector<string>& values = fields[string(field_binary_type)];

					BinaryType result{};
					StringToEnum(values.front(), KalaMake::Core::binaryTypes, result);
					globalData.targetProfile.binaryType = result;
				}
				if (fields.contains(string(field_compiler_launcher)))
				{
					const vector<string>& values = fields[string(field_compiler_launcher)];

					CompilerLauncherType result{};
					StringToEnum(values.front(), KalaMake::Core::compilerLauncherTypes, result);
					globalData.targetProfile.compilerLauncher = result;
				}
				if (fields.contains(string(field_compiler)))
				{
					const vector<string>& values = fields[string(field_compiler)];

					CompilerType result{};
					StringToEnum(values.front(), KalaMake::Core::compilerTypes, result);
					globalData.targetProfile.compiler = result;
				}
				if (fields.contains(string(field_standard)))
				{
					const vector<string>& values = fields[string(field_standard)];

					StandardType result{};
					StringToEnum(values.front(), KalaMake::Core::standardTypes, result);
					globalData.targetProfile.standard = result;
				}
				if (fields.contains(string(field_target_type)))
				{
					const vector<string>& values = fields[string(field_target_type)];

					TargetType result{};
					StringToEnum(values.front(), KalaMake::Core::targetTypes, result);
					globalData.targetProfile.targetType = result;
#if _WIN32
					if (globalData.targetProfile.targetType == TargetType::T_WINDOWS) globalData.targetProfile.targetType = TargetType::T_INVALID;
#else
					if (globalData.targetProfile.targetType == TargetType::T_LINUX) globalData.targetProfile.targetType = TargetType::T_INVALID;
#endif
				}
				if (fields.contains(string(field_jobs)))
				{
					const vector<string>& values = fields[string(field_jobs)];
					globalData.targetProfile.jobs = scast<u16>(stoul(values[0]));
				}

				if (fields.contains(string(field_binary_name)))
				{
					globalData.targetProfile.binaryName = fields[string(field_binary_name)][0];
				}
				if (fields.contains(string(field_build_type)))
				{
					const vector<string>& values = fields[string(field_build_type)];

					BuildType result{};
					StringToEnum(values.front(), KalaMake::Core::buildTypes, result);
					globalData.targetProfile.buildType = result;
				}
				if (fields.contains(string(field_build_path)))
				{
					globalData.targetProfile.buildPath = fields[string(field_build_path)][0];
				}
				if (fields.contains(string(field_sources)))
				{
					vector<path> pathResult{};
					ToPathVector(fields[string(field_sources)], pathResult);

					globalData.targetProfile.sources = std::move(pathResult);
				}
				if (fields.contains(string(field_headers)))
				{
					vector<path> pathResult{};
					ToPathVector(fields[string(field_headers)], pathResult);

					globalData.targetProfile.headers = std::move(pathResult);
				}
				if (fields.contains(string(field_links)))
				{
					vector<path> pathResult{};
					ToPathVector(fields[string(field_links)], pathResult);

					globalData.targetProfile.links = std::move(pathResult);
				}
				if (fields.contains(string(field_warning_level)))
				{
					const vector<string>& values = fields[string(field_warning_level)];

					WarningLevel result{};
					StringToEnum(values.front(), KalaMake::Core::warningLevels, result);
					globalData.targetProfile.warningLevel = result;
				}
				if (fields.contains(string(field_defines)))
				{
					globalData.targetProfile.defines = std::move(fields[string(field_defines)]);
				}
				if (fields.contains(string(field_compile_flags)))
				{
					globalData.targetProfile.compileFlags = std::move(fields[string(field_compile_flags)]);
				}
				if (fields.contains(string(field_link_flags)))
				{
					globalData.targetProfile.linkFlags = std::move(fields[string(field_link_flags)]);
				}
				if (fields.contains(string(field_custom_flags)))
				{
					const vector<string>& values = fields[string(field_custom_flags)];
					vector<CustomFlag> customFlags{};

					for (const auto& cf : values)
					{
						CustomFlag result{};
						StringToEnum(cf, KalaMake::Core::customFlags, result);
						customFlags.push_back(result);
					}
					globalData.targetProfile.customFlags = std::move(customFlags);
				}
				if (fields.contains(string(field_post_build_action)))
				{
					globalData.targetProfile.postBuildActions = std::move(fields[string(field_post_build_action)]);
				}

				foundGlobal = true;

				break;
			}
		}

		if (!foundGlobal)
		{
			KalaMakeCore::CloseOnError(
				"KALAMAKE",
				"Failed to find global profile!");
		}
	}

	if (foundTargetProfile
		&& correctTargetProfile != "global")
	{
		for (const string& l : lines)
		{
			string line = l;
			string name{};
			string value{};

			CategoryType type{};
			clean_line(line, name, value, type);

			if (type == CategoryType::C_PROFILE)
			{
				if (value != correctTargetProfile) continue;

				Log::Print(
					"\n---------------------------------------------------------------------------"
					"\n# Starting to parse user profile '" + value + "'\n"
					"---------------------------------------------------------------------------\n");

				vector<string> content = get_all_category_content(name, value);

				unordered_map<string, vector<string>> fields{};
				for (const auto& c : content)
				{
					string fieldName{};
					vector<string> fieldValues{};
					ExtractFieldData(
						c, 
						fieldName, 
						fieldValues);

					if (fields.contains(fieldName)
						&& fieldName != field_post_build_action)
					{
						KalaMakeCore::CloseOnError(
							"KALAMAKE",
							"Field '" + fieldName + "' was duplicated!");
					}

					if (fieldName != field_post_build_action) fields[fieldName] = fieldValues;
					else
					{
						if (!fields.contains(string(field_post_build_action)))
						{
							fields[fieldName] = fieldValues;
						}
						else fields[fieldName].push_back(fieldValues[0]);
					}
				}

				globalData.targetProfile.profileName = value;
				if (fields.contains(string(field_binary_type)))
				{
					const vector<string>& values = fields[string(field_binary_type)];

					BinaryType result{};
					StringToEnum(values.front(), KalaMake::Core::binaryTypes, result);
					globalData.targetProfile.binaryType = result;
				}
				if (fields.contains(string(field_compiler)))
				{
					const vector<string>& values = fields[string(field_compiler)];

					CompilerType result{};
					StringToEnum(values.front(), KalaMake::Core::compilerTypes, result);
					globalData.targetProfile.compiler = result;
				}
				if (fields.contains(string(field_compiler_launcher)))
				{
					const vector<string>& values = fields[string(field_compiler_launcher)];

					CompilerLauncherType result{};
					StringToEnum(values.front(), KalaMake::Core::compilerLauncherTypes, result);
					globalData.targetProfile.compilerLauncher = result;
				}
				if (fields.contains(string(field_standard)))
				{
					const vector<string>& values = fields[string(field_standard)];

					StandardType result{};
					StringToEnum(values.front(), KalaMake::Core::standardTypes, result);
					globalData.targetProfile.standard = result;
				}
				if (fields.contains(string(field_target_type)))
				{
					const vector<string>& values = fields[string(field_target_type)];

					TargetType result{};
					StringToEnum(values.front(), KalaMake::Core::targetTypes, result);
					globalData.targetProfile.targetType = result;
#if _WIN32
					if (globalData.targetProfile.targetType == TargetType::T_WINDOWS) globalData.targetProfile.targetType = TargetType::T_INVALID;
#else
					if (globalData.targetProfile.targetType == TargetType::T_LINUX) globalData.targetProfile.targetType = TargetType::T_INVALID;
#endif
				}
				if (fields.contains(string(field_jobs)))
				{
					const vector<string>& values = fields[string(field_jobs)];
					globalData.targetProfile.jobs = scast<u16>(stoul(values[0]));
				}

				if (fields.contains(string(field_binary_name)))
				{
					globalData.targetProfile.binaryName = fields[string(field_binary_name)][0];
				}
				if (fields.contains(string(field_build_type)))
				{
					const vector<string>& values = fields[string(field_build_type)];

					BuildType result{};
					StringToEnum(values.front(), KalaMake::Core::buildTypes, result);
					globalData.targetProfile.buildType = result;
				}
				if (fields.contains(string(field_build_path)))
				{
					globalData.targetProfile.buildPath = fields[string(field_build_path)][0];
				}
				if (fields.contains(string(field_sources)))
				{
					vector<path> pathResult{};
					ToPathVector(fields[string(field_sources)], pathResult);

					globalData.targetProfile.sources.reserve(
						globalData.targetProfile.sources.size()
						+ pathResult.size());

					globalData.targetProfile.sources.insert(
						globalData.targetProfile.sources.end(),
						pathResult.begin(),
						pathResult.end());

					RemoveDuplicates(globalData.targetProfile.sources);
				}
				if (fields.contains(string(field_headers)))
				{
					vector<path> pathResult{};
					ToPathVector(fields[string(field_headers)], pathResult);

					globalData.targetProfile.headers.reserve(
						globalData.targetProfile.headers.size()
						+ pathResult.size());

					globalData.targetProfile.headers.insert(
						globalData.targetProfile.headers.end(),
						pathResult.begin(),
						pathResult.end());

					RemoveDuplicates(globalData.targetProfile.headers);
				}
				if (fields.contains(string(field_links)))
				{
					vector<path> pathResult{};
					ToPathVector(fields[string(field_links)], pathResult);

					globalData.targetProfile.links.reserve(
						globalData.targetProfile.links.size()
						+ pathResult.size());

					globalData.targetProfile.links.insert(
						globalData.targetProfile.links.end(),
						pathResult.begin(),
						pathResult.end());

					RemoveDuplicates(globalData.targetProfile.links);
				}
				if (fields.contains(string(field_warning_level)))
				{
					vector<string>& values = fields[string(field_warning_level)];

					WarningLevel result{};
					StringToEnum(values.front(), KalaMake::Core::warningLevels, result);
					globalData.targetProfile.warningLevel = result;
				}
				if (fields.contains(string(field_defines)))
				{
					vector<string>& defines = fields[string(field_defines)];

					globalData.targetProfile.defines.reserve(
						globalData.targetProfile.defines.size()
						+ defines.size());

					globalData.targetProfile.defines.insert(
						globalData.targetProfile.defines.end(),
						defines.begin(),
						defines.end());

					RemoveDuplicates(globalData.targetProfile.defines);
				}
				if (fields.contains(string(field_compile_flags)))
				{
					vector<string>& flags = fields[string(field_compile_flags)];

					globalData.targetProfile.compileFlags.reserve(
						globalData.targetProfile.compileFlags.size()
						+ flags.size());

					globalData.targetProfile.compileFlags.insert(
						globalData.targetProfile.compileFlags.end(),
						flags.begin(),
						flags.end());

					RemoveDuplicates(globalData.targetProfile.compileFlags);
				}
				if (fields.contains(string(field_link_flags)))
				{
					vector<string>& flags = fields[string(field_link_flags)];

					globalData.targetProfile.linkFlags.reserve(
						globalData.targetProfile.linkFlags.size()
						+ flags.size());

					globalData.targetProfile.linkFlags.insert(
						globalData.targetProfile.linkFlags.end(),
						flags.begin(),
						flags.end());

					RemoveDuplicates(globalData.targetProfile.linkFlags);
				}
				if (fields.contains(string(field_custom_flags)))
				{
					const vector<string>& values = fields[string(field_custom_flags)];
					vector<CustomFlag> customFlags{};

					for (const auto& cf : values)
					{
						CustomFlag result{};
						StringToEnum(cf, KalaMake::Core::customFlags, result);
						customFlags.push_back(result);
					}

					globalData.targetProfile.customFlags.reserve(
						globalData.targetProfile.customFlags.size()
						+ customFlags.size());

					globalData.targetProfile.customFlags.insert(
						globalData.targetProfile.customFlags.end(),
						customFlags.begin(),
						customFlags.end());

					RemoveDuplicates(globalData.targetProfile.customFlags);
				}
				if (fields.contains(string(field_post_build_action)))
				{
					const vector<string>& values = fields[string(field_post_build_action)];

					globalData.targetProfile.postBuildActions.reserve(
						globalData.targetProfile.postBuildActions.size()
						+ values.size());

					globalData.targetProfile.postBuildActions.insert(
						globalData.targetProfile.postBuildActions.end(),
						values.begin(),
						values.end());
				}

				if (value == correctTargetProfile) break;
			}
		}
	}
}

string TranslateReferences(string_view value)
{
	//value did not contain any references, skip
	if (value.find('$') == string_view::npos) return string(value);

	string result = string(value);

	function<void(size_t)> replace_ref;

	replace_ref = [&](size_t start)
		{
			size_t refBracketPos = result.find('{', start);

			if (refBracketPos == string::npos)
			{
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"Failed to find dereference values in field value '" + result + "' because a reference was not followed by a starting bracket!");
			}

			//find ref start here before the closing bracket and jump to child
			size_t childRef = result.find('$', refBracketPos + 1);
			while (childRef != string::npos
				   && childRef < result.find('}', refBracketPos))
			{
				replace_ref(childRef);
				childRef = result.find('$', refBracketPos + 1);
			}

			size_t refBracketEndPos = result.find('}', refBracketPos);
			if (refBracketEndPos == string::npos)
			{
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"Failed to find dereference values in field value '" + result + "' because a reference value was not closed by a closing bracket!");
			}

			if (refBracketEndPos == refBracketPos + 1)
			{
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"Failed to find dereference values in field value '" + result + "' because a reference had no value between brackets!");
			}

			string refValue = result.substr(
				refBracketPos + 1,
				refBracketEndPos - refBracketPos - 1);

			if (HasAnyWhiteSpace(refValue))
			{
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"Failed to find dereference values in field value '" + result + "' because reference '" + refValue + "' contains whitespace characters!");
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
				KalaMakeCore::CloseOnError(
					"KALAMAKE",
					"Failed to find dereference value '" + refValue + "' in field value '" + result + "' because it has not been added as a reference!");
			}

			result.replace(
				start,
				refBracketEndPos - start + 1,
				replacement);
		};

	size_t refPos;

	while ((refPos = result.find('$')) != string::npos)
	{
		replace_ref(refPos);
	}

	return result;
}