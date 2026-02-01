//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include <sstream>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>

#include "KalaHeaders/core_utils.hpp"
#include "KalaHeaders/log_utils.hpp"
#include "KalaHeaders/file_utils.hpp"
#include "KalaHeaders/string_utils.hpp"

#include "core.hpp"

#include "core/kma_core.hpp"

using KalaHeaders::KalaCore::EnumHash;
using KalaHeaders::KalaCore::ContainsValue;
using KalaHeaders::KalaCore::IsComparable;
using KalaHeaders::KalaCore::IsAssignable;
using KalaHeaders::KalaCore::AnyEnumAndStringMap;
using KalaHeaders::KalaCore::AnyEnum;
using KalaHeaders::KalaCore::StringToEnum;
using KalaHeaders::KalaCore::RemoveDuplicates;
using KalaHeaders::KalaCore::StringToEnum;
using KalaHeaders::KalaLog::Log;
using KalaHeaders::KalaLog::LogType;
using KalaHeaders::KalaFile::ReadLinesFromFile;
using KalaHeaders::KalaFile::ResolveAnyPath;
using KalaHeaders::KalaString::ContainsString;
using KalaHeaders::KalaString::ReplaceAfter;
using KalaHeaders::KalaString::TrimString;

using KalaMake::Core::KalaMakeCore;
using KalaMake::Core::GlobalData;
using KalaMake::Core::ProfileData;
using KalaMake::Core::CategoryType;
using KalaMake::Core::Version;

using std::ostringstream;
using std::filesystem::exists;
using std::filesystem::path;
using std::filesystem::current_path;
using std::filesystem::weakly_canonical;
using std::filesystem::is_directory;
using std::filesystem::filesystem_error;
using std::string;
using std::string_view;
using std::vector;
using std::unordered_map;

//kma path is the root directory where the kmake file is stored at
static path kmaPath{};

static bool foundVersion{};
static bool foundGlobal{};
static bool foundInclude{};

template<AnyEnumAndStringMap T>
static bool EnumMapContainsValue(
	const T& map,
	string_view value,
	string_view valueName)
{
	if (value.empty())
	{
		KalaMakeCore::PrintError(string(valueName) + " cannot be empty!");

		return false;
	}

	typename T::key_type foundEnum{};

	bool result = StringToEnum(
		value,
		map,
		foundEnum);

	if (!result)
	{
		KalaMakeCore::PrintError(string(valueName) + " did not contain enum that matched requested value '" + string(value) + "'!");

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
		KalaMakeCore::PrintError(string(valueName) + " cannot be empty!");

		return false;
	}

	E foundEnum{};

	bool result = StringToEnum(
		value,
		map,
		foundEnum);

	if (!result)
	{
		KalaMakeCore::PrintError(string(valueName) + " did not contain enum that matched requested value '" + string(value) + "'!");

		return false;
	}

	outEnum = foundEnum;
	return true;
}

static bool ExtractCategoryData(
	const string& line,
	string& outCategoryName,
	string& outCategoryValue)
{
	string newLine = line;
	newLine.erase(0,  1);
	if (newLine.empty())
	{
		KalaMakeCore::PrintError(
			"Failed to resolve category '" + line + "' because it had no field name");

		return false;
	}

	//fast exit for include and global category
	if (newLine == "include"
		|| newLine == "global")
	{
		outCategoryName = std::move(newLine);

		return true;
	}

	size_t spacePos = newLine.find(' ');
	if (spacePos == string::npos)
	{
		KalaMakeCore::PrintError(
			"Failed to resolve category '" + line + "' because its type had no value!");

		return false;
	}

	string name = newLine.substr(0, spacePos);

	CategoryType type{};
	if (!KalaMakeCore::ResolveCategory(name, type)
		|| type == CategoryType::C_INVALID)
	{
		KalaMakeCore::PrintError(
			"Failed to resolve category '" + line + "' because its type '" + name + "' was not found!");

		return false;
	}

	size_t valueStart = newLine.find_first_not_of(' ', spacePos + 1);
	if (valueStart == string::npos)
	{
		KalaMakeCore::PrintError(
			"Failed to resolve category '" + line + "' because its type had no value!");

		return false;
	}
	
	string value = newLine.substr(valueStart);

	outCategoryName = std::move(name);
	outCategoryValue = std::move(value);

	return true;
}

static bool ResolvePathVector(
	const vector<string>& value,
	string_view valueName,
	const vector<string>& extensions,
	vector<path>& outValues)
{
	auto path_exists = [&valueName, &extensions](string_view value, vector<path>& outValues) -> bool
		{
			if (value.empty())
			{
				KalaMakeCore::PrintError(string(valueName) + " '" + string(value) + "' cannot be empty!");

				return false;
			}

			path p{ value };

			if (!exists(p)) p = kmaPath / p;
			if (!exists(p))
			{
				KalaMakeCore::PrintError(string(valueName) + " '" + string(value) + "' could not be resolved! Did you assign the local or full path correctly?");

				return false;
			}

			if (!is_regular_file(p))
			{
				KalaMakeCore::PrintError(string(valueName) + " '" + string(value) + "' is not a regular file so its extension can't be checked!");

				return false;
			}

			if (!p.has_extension())
			{
				KalaMakeCore::PrintError(string(valueName) + " '" + string(value) + "' has no extension!");

				return false;
			}

			string ext = p.extension().string();

			if (!ContainsValue(extensions, ext))
			{
				KalaMakeCore::PrintError(string(valueName) + " '" + string(value) + "' has an unsupported extension '" + string(ext) + "'!");

				return false;
			}

			try
			{
				p = weakly_canonical(p);
			}
			catch (const filesystem_error&)
			{
				KalaMakeCore::PrintError("Failed to resolve target path '" + string(value) + "'!");

				return false;
			}

			outValues.push_back(p);

			return true;
		};

	if (value.empty())
	{
		KalaMakeCore::PrintError(string(valueName) + " has no values!");

		return false;
	}

	vector<path> cleanedValues{};

	for (const auto& v : value) if (!path_exists(v, cleanedValues)) return false;

	RemoveDuplicates(cleanedValues);

	outValues = std::move(cleanedValues);

	return true;
}

static GlobalData FirstParse(const vector<string>& lines);

namespace KalaMake::Core
{
	constexpr string_view EXE_VERSION_NUMBER = "1.0";
	constexpr string_view KMA_VERSION_NUMBER = "1.0";

	static const unordered_map<Version, string_view, EnumHash<Version>> versions =
	{
		{ Version::V_1_0, "1.0" }
	};

	static const unordered_map<CategoryType, string_view, EnumHash<CategoryType>> categoryTypes =
	{
		{ CategoryType::C_VERSION, "#version " },
		{ CategoryType::C_INCLUDE, "#include" },
		{ CategoryType::C_GLOBAL, "#global" },
		{ CategoryType::C_PROFILE, "#profile " }
	};

	static const unordered_map<FieldType, string_view, EnumHash<FieldType>> fieldTypes =
	{
		{ FieldType::T_BINARY_TYPE,    "binarytype: " },
		{ FieldType::T_COMPILER,       "compiler: " },
		{ FieldType::T_STANDARD,       "standard: " },
		{ FieldType::T_TARGET_PROFILE, "targetprofile: " },

		{ FieldType::T_BINARY_NAME,   "binaryname: " },
		{ FieldType::T_BUILD_TYPE,    "buildtype: " },
		{ FieldType::T_BUILD_PATH,    "buildpath: " },
		{ FieldType::T_SOURCES,       "sources: " },
		{ FieldType::T_HEADERS,       "headers: " },
		{ FieldType::T_LINKS,         "links: " },
		{ FieldType::T_WARNING_LEVEL, "warninglevel: " },
		{ FieldType::T_DEFINES,       "defines: " },
		{ FieldType::T_FLAGS,         "flags: " },
		{ FieldType::T_CUSTOM_FLAGS,  "customflags: " }
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

	static const unordered_map<BuildType, string_view, EnumHash<BuildType>> buildTypes =
	{
		{ BuildType::B_DEBUG,      "debug" },
		{ BuildType::B_RELEASE,    "release" },
		{ BuildType::B_RELDEBUG,   "reldebug" },
		{ BuildType::B_MINSIZEREL, "minsizerel" }
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

	void KalaMakeCore::OpenFile(
		const vector<string>& params, 
		TargetState state)
	{
		ostringstream details{};

		details
			<< "     | exe version: " << EXE_VERSION_NUMBER.data() << "\n"
			<< "     | kma version: " << KMA_VERSION_NUMBER.data() << "\n";

		Log::Print(details.str());

		path projectFile = params[1];

		string& currentDir = KalaCLI::Core::GetCurrentDir();
		if (currentDir.empty()) currentDir = current_path().string();

		auto readprojectfile = [&state](path filePath) -> void
			{
				if (is_directory(filePath))
				{
					KalaMakeCore::PrintError("Failed to compile project because its path '" + filePath.string() + "' leads to a directory!");

					return;
				}

				if (!filePath.has_extension()
					|| filePath.extension() != ".kmake")
				{
					KalaMakeCore::PrintError("Failed to compile project because its path '" + filePath.string() + "' has an incorrect extension!");

					return;
				}

				vector<string> content{};

				string result = ReadLinesFromFile(
					filePath,
					content);

				if (!result.empty())
				{
					KalaMakeCore::PrintError("Failed to read project file '" + filePath.string() + "'! Reason: " + result);

					return;
				}

				if (content.empty())
				{
					KalaMakeCore::PrintError("Failed to compile project at '" + filePath.string() + "' because it was empty!");

					return;
				}

				kmaPath = filePath.parent_path();

				if (state == TargetState::S_COMPILE) { Compile(filePath, content); }
				else if (state == TargetState::S_GENERATE) { Generate(filePath, content); }
				else
				{
					KalaMakeCore::PrintError("Failed to compile project at '" + filePath.string() + "' because an unknown target state was passed!");

					return;
				}
			};

		//partial path was found

		path correctTarget{};

		try
		{
			correctTarget = weakly_canonical(path(KalaCLI::Core::GetCurrentDir()) / projectFile);
		}
		catch (const filesystem_error&)
		{
			KalaMakeCore::PrintError("Failed to compile project because partial path via '" + projectFile.string() + "' could not be resolved!");

			return;
		}

		if (exists(correctTarget))
		{
			readprojectfile(correctTarget);

			return;
		}

		//full path was found

		try
		{
			correctTarget = weakly_canonical(projectFile);
		}
		catch (const filesystem_error&)
		{
			KalaMakeCore::PrintError("Failed to compile project because full path '" + projectFile.string() + "' could not be resolved!");

			return;
		}

		if (exists(correctTarget))
		{
			readprojectfile(correctTarget);

			return;
		}

		KalaMakeCore::PrintError("Failed to compile project because its path '" + projectFile.string() + "' does not exist!");
	}

	void KalaMakeCore::Compile(
		const path& filePath,
		const vector<string>& lines)
	{
		Log::Print(
			"Starting to parse the kalamake file '" + filePath.string() + "'"
			"\n\n==========================================================================================\n",
			"KALAMAKE",
			LogType::LOG_INFO);

		GlobalData data = FirstParse(lines);
	}

	void KalaMakeCore::Generate(
		const path& filePath,
		const vector<string>& lines)
	{
		Log::Print(
			"Starting to generate a solution from the kalamake file '" + filePath.string() + "'"
			"\n\n==========================================================================================\n",
			"KALAMAKE",
			LogType::LOG_INFO);

		GlobalData data = FirstParse(lines);
	}

	bool KalaMakeCore::ResolveFieldReference(
		const vector<path>& currentProjectIncludes,
		const vector<ProfileData>& currentProjectProfiles,
		string_view value)
	{
		//TODO: fill

		return true;
	}

	bool KalaMakeCore::ResolveProfileReference(
		const vector<path>& currentProjectIncludes,
		const vector<ProfileData>& currentProjectProfiles,
		string_view value)
	{
		//TODO: fill

		return true;
	}

	bool KalaMakeCore::IsValidVersion(string_view value) { return EnumMapContainsValue(versions, value, "Version"); }

	bool KalaMakeCore::ResolveCategory(
		string_view value,
		CategoryType& outValue) 
	{ 
		return GetEnumFromMap(categoryTypes, value, "Category", outValue);
	}

	bool KalaMakeCore::ResolveField(
		string_view value,
		FieldType& outValue) 
	{ 
		return GetEnumFromMap(fieldTypes, value, "Field", outValue);
	}

	bool KalaMakeCore::ResolveBinaryType(
		string_view value,
		BinaryType& outValue)
	{ 
		return GetEnumFromMap(binaryTypes, value, "Binary type", outValue);
	}
	bool KalaMakeCore::ResolveCompiler(
		string_view value,
		CompilerType& outValue)
	{
		return GetEnumFromMap(compilerTypes, value, "Compiler", outValue);
	}
	bool KalaMakeCore::ResolveStandard(
		string_view value,
		StandardType& outValue)
	{
		return GetEnumFromMap(standardTypes, value, "Standard", outValue);
	}
	bool KalaMakeCore::IsValidTargetProfile(
		string_view value,
		const vector<string>& targetProfiles)
	{
		if (value.empty())
		{
			KalaMakeCore::PrintError("Target profile list has no values!");

			return false;
		}

		//TODO: fill out

		return false;
	}

	bool KalaMakeCore::IsValidBinaryName(string_view value)
	{
		if (value.empty())
		{
			KalaMakeCore::PrintError("Binary name cannot be empty!");

			return false;
		}

		if (value.size() < MIN_NAME_LENGTH)
		{
			KalaMakeCore::PrintError("Binary name length is too short!");

			return false;
		}
		if (value.size() > MAX_NAME_LENGTH)
		{
			KalaMakeCore::PrintError("Binary name length is too long!");

			return false;
		}

		return true;
	}
	bool KalaMakeCore::ResolveBuildType(
		string_view value,
		BuildType& outValue)
	{
		return GetEnumFromMap(buildTypes, value, "Build type", outValue);
	}
	bool KalaMakeCore::ResolveBuildPath(
		string_view value,
		path& outValue)
	{
		if (value.empty())
		{
			KalaMakeCore::PrintError("Build path cannot be empty!");

			return false;
		}

		if (ContainsString(value, "*")
			|| ContainsString(value, "**"))
		{
			KalaMakeCore::PrintError("Build path '" + string(value) + "' is not allowed to use wildcards!");

			return false;
		}

		path p{ value };

		if (!exists(p)) p = kmaPath / p;
		if (!exists(p))
		{
			KalaMakeCore::PrintError("Build path '" + string(value) + "' could not be resolved! Did you assign the local or full path correctly?");

			return false;
		}

		if (!is_directory(p))
		{
			KalaMakeCore::PrintError("Build path '" + string(value) + "' must lead to a directory!");

			return false;
		}

		try
		{
			p = weakly_canonical(p);
		}
		catch (const filesystem_error&)
		{
			KalaMakeCore::PrintError("Failed to resolve build path '" + string(value)  + "'!");

			return false;
		}

		outValue = p;

		return true;
	}
	bool KalaMakeCore::ResolveSources(
		const vector<string>& value,
		const vector<string>& correctExtensions,
		vector<path>& outValues)
	{
		return ResolvePathVector(
			value,
			"Source scripts list",
			correctExtensions,
			outValues);
	}
	bool KalaMakeCore::ResolveHeaders(
		const vector<string>& value,
		const vector<string>& correctExtensions,
		vector<path>& outValues)
	{
		return ResolvePathVector(
			value,
			"Header scripts list",
			correctExtensions,
			outValues);
	}
	bool KalaMakeCore::ResolveLinks(
		const vector<string>& value,
		const vector<string>& correctExtensions,
		vector<path>& outValues)
	{
		return ResolvePathVector(
			value,
			"Link list",
			correctExtensions,
			outValues);
	}
	bool KalaMakeCore::ResolveWarningLevel(
		string_view value,
		WarningLevel& outValue)
	{ 
		return GetEnumFromMap(warningLevels, value, "Warning level", outValue);
	}
	bool KalaMakeCore::ResolveCustomFlags(
		const vector<string>& value,
		vector<CustomFlag>& outValues)
	{
		vector<CustomFlag> foundValues{};

		for (const auto& v : value)
		{
			CustomFlag f{};

			if (!GetEnumFromMap(customFlags, v, "Custom flag list", f)) return false;

			foundValues.push_back(f);
		}

		outValues = std::move(foundValues);
		return true;
	}

	const unordered_map<CategoryType, string_view, EnumHash<CategoryType>>& KalaMakeCore::GetCategoryTypes() { return categoryTypes; }
	const unordered_map<Version, string_view, EnumHash<Version>>& KalaMakeCore::GetVersions() { return versions; }
	const unordered_map<FieldType, string_view, EnumHash<FieldType>>& KalaMakeCore::GetFieldTypes() { return fieldTypes; }
	const unordered_map<CompilerType, string_view, EnumHash<CompilerType>>& KalaMakeCore::GetCompilerTypes() { return compilerTypes; }
	const unordered_map<StandardType, string_view, EnumHash<StandardType>>& KalaMakeCore::GetStandardTypes() { return standardTypes; }
	const unordered_map<BinaryType, string_view, EnumHash<BinaryType>>& KalaMakeCore::GetBinaryTypes() { return binaryTypes; }
	const unordered_map<WarningLevel, string_view, EnumHash<WarningLevel>>& KalaMakeCore::GetWarningLevels() { return warningLevels; }
	const unordered_map<CustomFlag, string_view, EnumHash<CustomFlag>>& KalaMakeCore::GetCustomFlags() { return customFlags; }

    void KalaMakeCore::PrintError(string_view message)
	{
		Log::Print(
			message,
			"KALAMAKE",
			LogType::LOG_ERROR,
			2);
	}
}

GlobalData FirstParse(const vector<string>& lines)
{
	GlobalData data{};

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

	for (const string& l : lines)
	{
		if (l.empty()
			|| l.starts_with("//"))
		{
			continue;
		}

		if (!l.starts_with('#')) continue;

		string cleanedLine = TrimString(ReplaceAfter(l, "//"));
		
		string categoryName{};
		string categoryValue{};

		if (!ExtractCategoryData(
			cleanedLine, 
			categoryName,
			categoryValue))
		{
			exit(1);
		}

		const unordered_map<
			CategoryType, 
			string_view, 
			EnumHash<CategoryType>>& categoryTypes 
			= KalaMakeCore::GetCategoryTypes();

		CategoryType categoryType{};
		bool convertCategory = StringToEnum(
			categoryName, 
			categoryTypes, 
			categoryType);

		if (!convertCategory
			|| categoryType == CategoryType::C_INVALID)
		{
			KalaMakeCore::PrintError(
				"Failed to compile project because category type '" + categoryName  + "' is invalid!");
			
			exit(1);
		}

		if (categoryType == CategoryType::C_VERSION)
		{
			if (foundVersion)
			{
				KalaMakeCore::PrintError(
					"Failed to compile project because version category was passed more than once!");

				exit(1);
			}

			const unordered_map<
				Version, 
				string_view, 
				EnumHash<Version>>& versions 
				= KalaMakeCore::GetVersions();

			Version v{};
			bool convertVersion = StringToEnum(
				categoryValue, 
				versions, 
				v);

			if (!convertVersion
				|| v == Version::V_INVALID)
			{
				KalaMakeCore::PrintError(
					"Failed to compile project because version '" + categoryValue  + "' is invalid!");

				exit(1);
			}

			foundVersion = true;
			continue;
		}
		else if (categoryType == CategoryType::C_INCLUDE)
		{
			if (foundInclude)
			{
				KalaMakeCore::PrintError(
					"Failed to compile project because include category was used more than once!");

				exit(1);
			}

			foundInclude = true;

			vector<string> content = get_all_category_content(categoryName);

			vector<path> resolvedIncludes{};
			
			for (const string& c : content)
			{
				vector<path> foundIncludes{};
				string result = ResolveAnyPath(c, ".", foundIncludes);

				if (!result.empty())
				{
					KalaMakeCore::PrintError(
					"Failed to compile project because include path '" + c + "' could not be resolved! Reason: " + result);

					exit(1);
				}
				
				resolvedIncludes.insert(
					resolvedIncludes.end(),
					make_move_iterator(foundIncludes.begin()),
					make_move_iterator(foundIncludes.end()));
			}

			RemoveDuplicates(resolvedIncludes);

			data.includes.insert(
				data.includes.end(),
				make_move_iterator(resolvedIncludes.begin()),
				make_move_iterator(resolvedIncludes.end()));
		}
		else if (categoryType == CategoryType::C_GLOBAL)
		{
			if (foundGlobal)
			{
				KalaMakeCore::PrintError(
					"Failed to compile project because global category was used more than once!");

				exit(1);
			}

			foundGlobal = true;

			vector<string> content = get_all_category_content(categoryName);

			data.profiles.push_back(
				{
					.profileName = categoryValue
				});
		}
		else if (categoryType == CategoryType::C_PROFILE)
		{
			for (const ProfileData& p : data.profiles)
			{
				if (p.profileName == categoryValue)
				{
					KalaMakeCore::PrintError(
					"Failed to compile project because profile name '" + categoryValue  + "' was used more than once!");

					exit(1);
				}
			}

			vector<string> content = get_all_category_content(categoryName, categoryValue);

			data.profiles.push_back(
				{
					.profileName = categoryValue
				});
		}
	}

	return data;
}