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
using KalaHeaders::KalaLog::Log;
using KalaHeaders::KalaLog::LogType;
using KalaHeaders::KalaFile::ReadLinesFromFile;
using KalaHeaders::KalaFile::ResolveAnyPath;
using KalaHeaders::KalaFile::ToStringVector;
using KalaHeaders::KalaFile::HasAnyUnsafePathChar;
using KalaHeaders::KalaFile::PathTarget;
using KalaHeaders::KalaString::SplitString;
using KalaHeaders::KalaString::ReplaceAfter;
using KalaHeaders::KalaString::TrimString;
using KalaHeaders::KalaString::HasAnyWhiteSpace;
using KalaHeaders::KalaString::HasAnyUnsafeFieldChar;

using KalaMake::Core::KalaMakeCore;
using KalaMake::Core::GlobalData;
using KalaMake::Core::ProfileData;
using KalaMake::Core::Version;
using KalaMake::Core::CategoryType;
using KalaMake::Core::FieldType;
using KalaMake::Core::CompilerType;
using KalaMake::Core::StandardType;
using KalaMake::Core::BuildType;
using KalaMake::Core::BinaryType;
using KalaMake::Core::WarningLevel;
using KalaMake::Core::CustomFlag;

using std::ostringstream;
using std::filesystem::exists;
using std::filesystem::path;
using std::filesystem::current_path;
using std::filesystem::weakly_canonical;
using std::filesystem::is_directory;
using std::filesystem::is_regular_file;
using std::filesystem::filesystem_error;
using std::string;
using std::string_view;
using std::vector;
using std::unordered_map;

constexpr string_view version_1_0 = "1.0";

constexpr string_view category_version   = "version";
constexpr string_view category_include   = "include";
constexpr string_view category_global    = "global";
constexpr string_view category_profile   = "profile";
constexpr string_view category_postbuild = "postbuild";

constexpr string_view field_binary_type    = "binarytype";
constexpr string_view field_compiler       = "compiler";
constexpr string_view field_standard       = "standard";
constexpr string_view field_target_profile = "targetprofile";
constexpr string_view field_binary_name    = "binaryname";
constexpr string_view field_build_type     = "buildtype";
constexpr string_view field_build_path     = "buildpath";
constexpr string_view field_sources        = "sources";
constexpr string_view field_headers        = "headers";
constexpr string_view field_links          = "links";
constexpr string_view field_warning_level  = "warninglevel";
constexpr string_view field_defines        = "defines";
constexpr string_view field_flags          = "flags";
constexpr string_view field_custom_flags   = "customflags";
constexpr string_view field_move           = "move";
constexpr string_view field_copy           = "copy";
constexpr string_view field_force_copy     = "forcecopy";
constexpr string_view field_create_dir     = "createdir";
constexpr string_view field_delete         = "delete";
constexpr string_view field_rename         = "rename";

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

constexpr string_view build_type_debug      = "debug";
constexpr string_view build_type_release    = "release";
constexpr string_view build_type_reldebug   = "reldebug";
constexpr string_view build_type_minsizerel = "minsizerel";

constexpr string_view binary_type_executable   = "executable";
constexpr string_view binary_type_link_only    = "link-only";
constexpr string_view binary_type_runtime_only = "runtime-only";
constexpr string_view binary_type_link_runtime = "link-runtime";

constexpr string_view warning_level_none   = "none";
constexpr string_view warning_level_basic  = "basic";
constexpr string_view warning_level_normal = "normal";
constexpr string_view warning_level_strong = "strong";
constexpr string_view warning_level_strict = "strict";
constexpr string_view warning_level_all    = "all";

constexpr string_view custom_flag_use_ninja        = "use-ninja";
constexpr string_view custom_flag_no_obj           = "no-obj";
constexpr string_view custom_flag_standard_req     = "standard-required";
constexpr string_view custom_warnings_as_err       = "warnings-as-errors";
constexpr string_view custom_flag_export_comp_comm = "export-compile-commands";

//kma path is the root directory where the kmake file is stored at
static path kmaPath{};

static bool foundVersion{};
static bool foundInclude{};
static bool foundGlobal{};
static bool foundPostBuild{};

static void CleanFoundFlags()
{
	foundVersion = false;
	foundInclude = false;
	foundGlobal = false;
	foundPostBuild = false;
}

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

static void ResolvePathVector(
	const vector<string>& value,
	string_view valueName,
	const vector<string>& extensions,
	vector<path>& outValues)
{
	auto check_path = [&valueName, &extensions](string_view value, vector<path>& outValues) -> bool
		{
			if (value.empty())
			{
				KalaMakeCore::PrintError(string(valueName) + " '" + string(value) + "' cannot be empty!");

				exit(1);
			}

			path p{ value };

			if (!exists(p)) p = kmaPath / p;
			if (!exists(p))
			{
				KalaMakeCore::PrintError(string(valueName) + " '" + string(value) + "' could not be resolved! Did you assign the local or full path correctly?");

				exit(1);
			}

			if (!is_regular_file(p))
			{
				KalaMakeCore::PrintError(string(valueName) + " '" + string(value) + "' is not a regular file so its extension can't be checked!");

				exit(1);
			}

			if (!p.has_extension())
			{
				KalaMakeCore::PrintError(string(valueName) + " '" + string(value) + "' has no extension!");

				exit(1);
			}

			string ext = p.extension().string();

			if (!ContainsValue(extensions, ext))
			{
				KalaMakeCore::PrintError(string(valueName) + " '" + string(value) + "' has an unsupported extension '" + string(ext) + "'!");

				exit(1);
			}

			try
			{
				p = weakly_canonical(p);
			}
			catch (const filesystem_error&)
			{
				KalaMakeCore::PrintError("Failed to resolve target path '" + string(value) + "'!");

				exit(1);
			}

			outValues.push_back(p);

			return true;
		};

	if (value.empty())
	{
		KalaMakeCore::PrintError(string(valueName) + " has no values!");

		exit(1);
	}

	vector<path> cleanedValues{};

	for (const auto& v : value) check_path(v, cleanedValues);

	RemoveDuplicates(cleanedValues);

	outValues = std::move(cleanedValues);
}

static void ExtractCategoryData(
	const string& line,
	string& outCategoryName,
	string& outCategoryValue)
{
	string newLine = line;
	newLine.erase(0,  1);
	if (newLine.empty())
	{
		KalaMakeCore::PrintError(
			"Failed to resolve category '" + line + "' because it had no type or value!");

		exit(1);
	}

	if (newLine == category_include)
	{
		outCategoryName = category_include;
		return;
	}
	if (newLine == category_global)
	{
		outCategoryName = category_global;
		return;
	}
	if (newLine == category_postbuild)
	{
		outCategoryName = category_postbuild;
		return;
	}

	if (newLine.starts_with(category_include)
		|| newLine.starts_with(category_global)
		|| newLine.starts_with(category_postbuild))
	{
		KalaMakeCore::PrintError(
			"Failed to resolve category line '" + line + "' because it is not allowed to have a value after its name!");

		exit(1);
	}

	size_t spacePos = newLine.find(' ');
	if (spacePos == string::npos)
	{
		KalaMakeCore::PrintError(
			"Failed to resolve category line '" + line + "' because its value was empty!");

		exit(1);
	}

	string name = newLine.substr(0, spacePos);

	CategoryType type{};
	if (!KalaMakeCore::ResolveCategory(name, type)
		|| type == CategoryType::C_INVALID)
	{
		KalaMakeCore::PrintError(
			"Failed to resolve category line '" + line + "' because its type '" + name + "' was not found!");

		exit(1);
	}

	size_t valueStart = newLine.find_first_not_of(' ', spacePos + 1);
	if (valueStart == string::npos)
	{
		KalaMakeCore::PrintError(
			"Failed to resolve category line '" + line + "' because its value was empty!");

		exit(1);
	}
	
	string value = newLine.substr(valueStart);

	outCategoryName = name;
	outCategoryValue = value;
}

static void ExtractFieldData(
	const string& line,
	string& outFieldName,
	vector<string>& outFieldValues,
	bool isInclude = false)
{
	if (line.find(": ") == string::npos)
	{
		KalaMakeCore::PrintError(
			"Failed to resolve field '" + line + "' because it is missing its name and value separator!");

		exit(1);
	}

	vector<string> split = SplitString(line, ": ");

	if (split.size() > 2)
	{
		KalaMakeCore::PrintError(
			"Failed to resolve field '" + line + "' because it has more than one name and value separator!");

		exit(1);
	}

	string name = split[0];
	string value = split[1];

	if (HasAnyWhiteSpace(name))
	{
		KalaMakeCore::PrintError(
			"Field name '" + name + "' cannot have spaces!");

		exit(1);
	}
	if (HasAnyUnsafeFieldChar(name))
	{
		KalaMakeCore::PrintError(
			"Field name '" + name + "' must only contain A-Z, a-z, 0-9, _ or -!");

		exit(1);
	}

	FieldType t{};
	bool searchSuccess = StringToEnum(name, KalaMakeCore::GetFieldTypes(), t);

	if (!isInclude
		&& (!searchSuccess
		|| t == FieldType::T_INVALID))
	{
		KalaMakeCore::PrintError("Field '" + name  + "' is invalid!");
				
		exit(1);
	}
	else if (isInclude
			 && searchSuccess
			 && t != FieldType::T_INVALID)
	{
		KalaMakeCore::PrintError("Field '" + name  + "' cannot be used for include field names!");
				
		exit(1);
	}

	auto require_quotes = [](const string& input) -> string
		{
			if (input.empty())
			{
				KalaMakeCore::PrintError("Failed to parse path! Cannot remove '\"' from empty path.");
			
				exit(1);
			}

			if (input.size() <= 2)
			{
				KalaMakeCore::PrintError("Failed to parse path! Input path '" + input + "' was too small.");
			
				exit(1);
			}

			if (input.front() != '"'
				|| input.back() != '"')
			{
				KalaMakeCore::PrintError("Failed to parse path! Input path '" + input + "' did not have the '\"' symbol at the front or back.");
			
				exit(1);
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
		if (value.empty())
		{
			KalaMakeCore::PrintError(
				"Build path must have a value!");

			exit(1);
		}

		if (value.find(',') != string::npos)
		{
			KalaMakeCore::PrintError("Build path '" + name  + "' is not allowed to have more than one path!");
				
			exit(1);
		}
		if (value.find('*') != string::npos)
		{
			KalaMakeCore::PrintError("Build path '" + name  + "' is not allowed to use wildcards!");
				
			exit(1);
		}

		if ((!value.starts_with('"')
			&& value.ends_with('"'))
			|| (value.starts_with('"')
			&& !value.ends_with('"')))
		{
			KalaMakeCore::PrintError(
				"Partial quoting for build path '" + value + "' is not allowed!");

			exit(1);
		}

		if (!value.starts_with('"')
			&& !value.ends_with('"'))
		{
			KalaMakeCore::PrintError(
				"Build path value '" + value + "' must have quotes!");

			exit(1);
		}

		string cleanedValue = require_quotes(value);

		if (HasAnyUnsafePathChar(cleanedValue))
		{
			KalaMakeCore::PrintError(
				"Build path value '" + cleanedValue + "' must not contain illegal path characters!");

			exit(1);
		}

		vector<path> resolvedPaths{};
		string errorMsg = ResolveAnyPath(
			cleanedValue, 
			kmaPath.string(), 
			resolvedPaths);

		if (!errorMsg.empty())
		{
			KalaMakeCore::PrintError(
				"Build path value '" + cleanedValue + "' could not be resolved! Reason: " + errorMsg);

			exit(1);
		}

		vector<string> result{};
		ToStringVector(resolvedPaths, result);

		outFieldName = name;
		outFieldValues = result;

		return;
	}
	else if (name == field_sources
			 || name == field_headers)
	{
		//early exit for empty value
		if (value.empty())
		{
			outFieldName = name;
			return;
		}

		vector<string> splitPaths = SplitString(value, ", ");

		vector<string> result{};

		for (const string& l : splitPaths)
		{
			if ((!l.starts_with('"')
				&& l.ends_with('"'))
				|| (l.starts_with('"')
				&& !l.ends_with('"')))
			{
				KalaMakeCore::PrintError(
					"Partial quoting for source or header value '" + l + "' is not allowed!");

				exit(1);
			}

			if (!l.starts_with('"')
				&& !l.ends_with('"'))
			{
				KalaMakeCore::PrintError(
					"Source or header value '" + l + "' must have quotes!");

				exit(1);
			}

			string cleanedValue = require_quotes(l);

			if (HasAnyUnsafePathChar(cleanedValue))
			{
				KalaMakeCore::PrintError(
					"Source or header value '" + cleanedValue + "' must not contain illegal path characters!");

				exit(1);
			}

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
					KalaMakeCore::PrintError(
						"Source value '" + cleanedValue + "' could not be resolved! Reason: " + errorMsg);

					exit(1);
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
					KalaMakeCore::PrintError(
						"Header value '" + cleanedValue + "' could not be resolved! Reason: " + errorMsg);

					exit(1);
				}
			}

			ToStringVector(resolvedPaths, resolvedStringPaths);

			result.insert(
				result.end(),
				make_move_iterator(resolvedStringPaths.begin()),
				make_move_iterator(resolvedStringPaths.end()));
		}

		RemoveDuplicates(result);

		outFieldName = name;
		outFieldValues = result;

		return;
	}
	else if (name == field_links)
	{
		//early exit for empty value
		if (value.empty())
		{
			outFieldName = name;
			return;
		}

		vector<string> splitPaths = SplitString(value, ", ");

		vector<string> result{};

		for (const string& l : splitPaths)
		{
			if (!l.starts_with('"')
				&& !l.ends_with('"'))
			{
				if (!l.ends_with(".so")
					&& !l.ends_with(".a")
					&& !l.ends_with(".lib"))
				{
					KalaMakeCore::PrintError(
						"Relative link value '" + l + "' does not have an extension!");

					exit(1);
				}

				result.push_back(l);
			}
			else 
			{
				if ((!l.starts_with('"')
					&& l.ends_with('"'))
					|| (l.starts_with('"')
					&& !l.ends_with('"')))
				{
					KalaMakeCore::PrintError(
						"Partial quoting for link '" + l + "' is not allowed!");

					exit(1);
				}

				if (!l.starts_with('"')
					&& !l.ends_with('"'))
				{
					KalaMakeCore::PrintError(
						"Link value '" + l + "' must have quotes!");

					exit(1);
				}

				string cleanedValue = require_quotes(l);

				if (HasAnyUnsafePathChar(cleanedValue))
				{
					KalaMakeCore::PrintError(
						"Link value '" + cleanedValue + "' must not contain illegal path characters!");

					exit(1);
				}

				vector<string> resolvedStringPaths{};
				vector<path> resolvedPaths{};

				string errorMsg = ResolveAnyPath(
						cleanedValue, 
						kmaPath.string(), 
						resolvedPaths,
						PathTarget::P_FILE_ONLY);

				if (!errorMsg.empty())
				{
					KalaMakeCore::PrintError(
						"Link path '" + cleanedValue + "' could not be resolved! Reason: " + errorMsg);

					exit(1);
				}

				ToStringVector(resolvedPaths, resolvedStringPaths);

				result.insert(
					result.end(),
					make_move_iterator(resolvedStringPaths.begin()),
					make_move_iterator(resolvedStringPaths.end()));
			}
		}

		RemoveDuplicates(result);

		outFieldName = name;
		outFieldValues = result;

		return;
	}
	//any field name in includes
	else if (isInclude)
	{
		if (value.empty())
		{
			KalaMakeCore::PrintError(
				"Include field '" + name + "' must have a value!");

			exit(1);
		}

		if (value.find(',') != string::npos)
		{
			KalaMakeCore::PrintError("Include field '" + name  + "' is not allowed to have more than one path!");
				
			exit(1);
		}

		vector<string> splitPaths = SplitString(value, ", ");

		vector<string> result{};

		for (const string& l : splitPaths)
		{
			if ((!l.starts_with('"')
				&& l.ends_with('"'))
				|| (l.starts_with('"')
				&& !l.ends_with('"')))
			{
				KalaMakeCore::PrintError(
					"Partial quoting for include value '" + l + "' is not allowed!");

				exit(1);
			}

			if (!l.starts_with('"')
				&& !l.ends_with('"'))
			{
				KalaMakeCore::PrintError(
					"Include value '" + l + "' must have quotes!");

				exit(1);
			}

			string cleanedValue = require_quotes(l);

			if (HasAnyUnsafePathChar(cleanedValue))
			{
				KalaMakeCore::PrintError(
					"Field '" + name + "' value '" + cleanedValue + "' must not contain illegal path characters!");

				exit(1);
			}

			vector<string> resolvedStringPaths{};
			vector<path> resolvedPaths{};

			string errorMsg = ResolveAnyPath(
					cleanedValue, 
					kmaPath.string(), 
					resolvedPaths);

			if (!errorMsg.empty())
			{
				KalaMakeCore::PrintError(
					"Include value '" + cleanedValue + "' could not be resolved! Reason: " + errorMsg);

				exit(1);
			}

			ToStringVector(resolvedPaths, resolvedStringPaths);

			result.insert(
				result.end(),
				make_move_iterator(resolvedStringPaths.begin()),
				make_move_iterator(resolvedStringPaths.end()));
		}

		RemoveDuplicates(result);

		outFieldName = name;
		outFieldValues = result;
	}
	else if (name == field_move
			 || name == field_copy
			 || name == field_force_copy
			 || name == field_delete
			 || name == field_create_dir
			 || name == field_rename)
	{
		vector<string> splitPaths = SplitString(value, ", ");
		if (splitPaths.size() > 2)
		{
			KalaMakeCore::PrintError(
				"Field '" + name + "' must only contain origin and target path!");

			exit(1);
		}
		if (splitPaths.size() <= 1)
		{
			KalaMakeCore::PrintError(
				"Field '" + name + "' must contain origin and target path!");

			exit(1);
		}

		if (splitPaths[0].find('*') != string::npos)
		{
			KalaMakeCore::PrintError("Include field '" + name  + "' origin path is not allowed to use wildcards!");
						
			exit(1);
		}
		if (splitPaths[1].find('*') != string::npos)
		{
			KalaMakeCore::PrintError("Include field '" + name  + "' target path is not allowed to use wildcards!");
						
			exit(1);
		}

		vector<string> result{};

		for (const string& l : splitPaths)
		{
			if ((!l.starts_with('"')
				&& l.ends_with('"'))
				|| (l.starts_with('"')
				&& !l.ends_with('"')))
			{
				KalaMakeCore::PrintError(
					"Partial quoting for field '" + name + "' value '" + l + "' is not allowed!");

				exit(1);
			}

			if (!l.starts_with('"')
				&& !l.ends_with('"'))
			{
				KalaMakeCore::PrintError(
					"Field '" + name + "' value '" + l + "' must have quotes!");

				exit(1);
			}

			string cleanedValue = require_quotes(l);

			if (HasAnyUnsafePathChar(cleanedValue))
			{
				KalaMakeCore::PrintError(
					"Field '" + name + "' value '" + cleanedValue + "' must not contain illegal path characters!");

				exit(1);
			}

			vector<string> resolvedStringPaths{};
			vector<path> resolvedPaths{};

			string errorMsg = ResolveAnyPath(
					cleanedValue, 
					kmaPath.string(), 
					resolvedPaths);

			if (!errorMsg.empty())
			{
				KalaMakeCore::PrintError(
					"Field '" + name + "' value '" + cleanedValue + "' could not be resolved! Reason: " + errorMsg);

				exit(1);
			}

			ToStringVector(resolvedPaths, resolvedStringPaths);

			result.insert(
				result.end(),
				make_move_iterator(resolvedStringPaths.begin()),
				make_move_iterator(resolvedStringPaths.end()));
		}

		RemoveDuplicates(result);

		outFieldName = name;
		outFieldValues = result;
	}
	//all other standard fields with no paths
	else 
	{
		if (value.find('"') != string::npos)
		{
			KalaMakeCore::PrintError("Field '" + name + "' is not allowed to use quotes or paths!");
						
			exit(1);
		}
		if (value.find('*') != string::npos)
		{
			KalaMakeCore::PrintError("Field '" + name + "' is not allowed to use wildcards!");
						
			exit(1);
		}

		//these fields must have a value
		if ((name == field_binary_type
			|| name == field_build_type
			|| name == field_compiler
			|| name == field_standard
			|| name == field_target_profile)
			&& value.empty())
		{
			KalaMakeCore::PrintError(
				"Field '" + name + "' must have a value!");

			exit(1);
		}

		if ((name == field_binary_type
			|| name == field_compiler
			|| name == field_standard
			|| name == field_binary_name
			|| name == field_warning_level)
			&& value.find(",") != string::npos)
		{
			KalaMakeCore::PrintError("Field '" + name + "' is not allowed to have more than one value!");
							
			exit(1);
		}

		if (name == field_binary_type)
		{
			const auto& binaryTypes = KalaMakeCore::GetBinaryTypes();

			BinaryType binaryType{};
			if (!StringToEnum(value, binaryTypes, binaryType)
				|| binaryType == BinaryType::B_INVALID)
			{
				KalaMakeCore::PrintError("Binary type '" + value + "' is invalid!");
				
				exit(1);
			}
		}
		if (name == field_build_type)
		{
			const auto& buildTypes = KalaMakeCore::GetBuildTypes();

			BuildType buildType{};
			if (!StringToEnum(value, buildTypes, buildType)
				|| buildType == BuildType::B_INVALID)
			{
				KalaMakeCore::PrintError("Build type '" + value + "' is invalid!");
				
				exit(1);
			}
		}
		if (name == field_compiler)
		{
			const auto& compilerTypes = KalaMakeCore::GetCompilerTypes();

			CompilerType compilerType{};
			if (!StringToEnum(value, compilerTypes, compilerType)
				|| compilerType == CompilerType::C_INVALID)
			{
				KalaMakeCore::PrintError("Compiler type '" + value + "' is invalid!");
				
				exit(1);
			}
		}
		if (name == field_standard)
		{
			const auto& standardTypes = KalaMakeCore::GetStandardTypes();

			StandardType standardType{};
			if (!StringToEnum(value, standardTypes, standardType)
				|| standardType == StandardType::S_INVALID)
			{
				KalaMakeCore::PrintError("Standard type '" + value + "' is invalid!");
				
				exit(1);
			}
		}

		vector<string> splitPaths = SplitString(value, ", ");

		vector<string> result{};

		RemoveDuplicates(result);

		outFieldName = name;
		outFieldValues = result;
	}	
}

static GlobalData FirstParse(const vector<string>& lines);

namespace KalaMake::Core
{
	constexpr string_view EXE_VERSION_NUMBER = "1.0";
	constexpr string_view KMA_VERSION_NUMBER = "1.0";

	static const unordered_map<Version, string_view, EnumHash<Version>> versions =
	{
		{ Version::V_1_0, version_1_0 }
	};

	static const unordered_map<CategoryType, string_view, EnumHash<CategoryType>> categoryTypes =
	{
		{ CategoryType::C_VERSION, category_version },
		{ CategoryType::C_INCLUDE, category_include },
		{ CategoryType::C_GLOBAL, category_global },
		{ CategoryType::C_PROFILE, category_profile },
		{ CategoryType::C_POST_BUILD, category_postbuild }
	};

	static const unordered_map<FieldType, string_view, EnumHash<FieldType>> fieldTypes =
	{
		{ FieldType::T_BINARY_TYPE,    field_binary_type },
		{ FieldType::T_COMPILER,       field_compiler },
		{ FieldType::T_STANDARD,       field_standard },
		{ FieldType::T_TARGET_PROFILE, field_target_profile },

		{ FieldType::T_BINARY_NAME,    field_binary_name },
		{ FieldType::T_BUILD_TYPE,     field_build_type },
		{ FieldType::T_BUILD_PATH,     field_build_path },
		{ FieldType::T_SOURCES,        field_sources },
		{ FieldType::T_HEADERS,        field_headers },
		{ FieldType::T_LINKS,          field_links },
		{ FieldType::T_WARNING_LEVEL, field_warning_level },
		{ FieldType::T_DEFINES,       field_defines },
		{ FieldType::T_FLAGS,         field_flags },
		{ FieldType::T_CUSTOM_FLAGS,  field_custom_flags },

		{ FieldType::T_MOVE,       field_move },
		{ FieldType::T_COPY,       field_copy },
		{ FieldType::T_FORCECOPY,  field_force_copy },
		{ FieldType::T_CREATE_DIR, field_create_dir },
		{ FieldType::T_DELETE,     field_delete },
		{ FieldType::T_RENAME,     field_rename }
	};

	static const unordered_map<CompilerType, string_view, EnumHash<CompilerType>> compilerTypes =
	{
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

	static const unordered_map<BuildType, string_view, EnumHash<BuildType>> buildTypes =
	{
		{ BuildType::B_DEBUG,      build_type_debug },
		{ BuildType::B_RELEASE,    build_type_release },
		{ BuildType::B_RELDEBUG,   build_type_reldebug },
		{ BuildType::B_MINSIZEREL, build_type_minsizerel }
	};

	static const unordered_map<BinaryType, string_view, EnumHash<BinaryType>> binaryTypes =
	{
		{ BinaryType::B_EXECUTABLE,   binary_type_executable },
		{ BinaryType::B_LINK_ONLY,    binary_type_link_only },
		{ BinaryType::B_RUNTIME_ONLY, binary_type_runtime_only },
		{ BinaryType::B_LINK_RUNTIME, binary_type_link_runtime }
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
		{ CustomFlag::F_USE_NINJA,               custom_flag_use_ninja },
		{ CustomFlag::F_NO_OBJ,                  custom_flag_no_obj },
		{ CustomFlag::F_STANDARD_REQUIRED,       custom_flag_standard_req },
		{ CustomFlag::F_WARNINGS_AS_ERRORS,      custom_warnings_as_err },
		{ CustomFlag::F_EXPORT_COMPILE_COMMANDS, custom_flag_export_comp_comm }
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
					KalaMakeCore::PrintError("its path '" + filePath.string() + "' leads to a directory!");

					return;
				}

				if (!filePath.has_extension()
					|| filePath.extension() != ".kmake")
				{
					KalaMakeCore::PrintError("its path '" + filePath.string() + "' has an incorrect extension!");

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
			KalaMakeCore::PrintError("partial path via '" + projectFile.string() + "' could not be resolved!");

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
			KalaMakeCore::PrintError("full path '" + projectFile.string() + "' could not be resolved!");

			return;
		}

		if (exists(correctTarget))
		{
			readprojectfile(correctTarget);

			return;
		}

		KalaMakeCore::PrintError("its path '" + projectFile.string() + "' does not exist!");
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

		CleanFoundFlags();
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

		CleanFoundFlags();
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

		if (value.find('*') != string::npos)
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
		vector<path> result{};

		ResolvePathVector(
			value,
			"Source scripts list",
			correctExtensions,
			result);

		if (result.empty()) return false;

		outValues = result;

		return true;
	}
	bool KalaMakeCore::ResolveHeaders(
		const vector<string>& value,
		const vector<string>& correctExtensions,
		vector<path>& outValues)
	{
		vector<path> result{};

		ResolvePathVector(
			value,
			"Header scripts list",
			correctExtensions,
			outValues);

		if (result.empty()) return false;

		outValues = result;

		return true;
	}
	bool KalaMakeCore::ResolveLinks(
		const vector<string>& value,
		const vector<string>& correctExtensions,
		vector<path>& outValues)
	{
		vector<path> result{};
	
		ResolvePathVector(
			value,
			"Link list",
			correctExtensions,
			outValues);

		if (result.empty()) return false;

		outValues = result;

		return true;
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

	const unordered_map<Version, string_view, EnumHash<Version>>& KalaMakeCore::GetVersions() { return versions; }
	const unordered_map<CategoryType, string_view, EnumHash<CategoryType>>& KalaMakeCore::GetCategoryTypes() { return categoryTypes; }
	const unordered_map<FieldType, string_view, EnumHash<FieldType>>& KalaMakeCore::GetFieldTypes() { return fieldTypes; }
	const unordered_map<CompilerType, string_view, EnumHash<CompilerType>>& KalaMakeCore::GetCompilerTypes() { return compilerTypes; }
	const unordered_map<StandardType, string_view, EnumHash<StandardType>>& KalaMakeCore::GetStandardTypes() { return standardTypes; }
	const unordered_map<BinaryType, string_view, EnumHash<BinaryType>>& KalaMakeCore::GetBinaryTypes() { return binaryTypes; }
	const unordered_map<BuildType, string_view, EnumHash<BuildType>>& KalaMakeCore::GetBuildTypes() { return buildTypes; }
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

	const auto& categoryTypes = KalaMakeCore::GetCategoryTypes();
	const auto& versions = KalaMakeCore::GetVersions();

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

		ExtractCategoryData(
			cleanedLine, 
			categoryName,
			categoryValue);

		CategoryType categoryType{};
		if (!StringToEnum(categoryName, categoryTypes, categoryType)
			|| categoryType == CategoryType::C_INVALID)
		{
			KalaMakeCore::PrintError("Category type '" + categoryName  + "' is invalid!");
			
			exit(1);
		}

		if (categoryType == CategoryType::C_VERSION)
		{
			if (foundVersion)
			{
				KalaMakeCore::PrintError("Version category was passed more than once!");

				exit(1);
			}

			Version v{};
			bool convertVersion = StringToEnum(
				categoryValue, 
				versions, 
				v);

			if (!convertVersion
				|| v == Version::V_INVALID)
			{
				KalaMakeCore::PrintError("Version '" + categoryValue  + "' is invalid!");

				exit(1);
			}

			foundVersion = true;
			continue;
		}
		else if (categoryType == CategoryType::C_INCLUDE)
		{
			if (foundInclude)
			{
				KalaMakeCore::PrintError("Include category was used more than once!");

				exit(1);
			}

			foundInclude = true;

			vector<string> content = get_all_category_content(categoryName);

			unordered_map<string, vector<string>> fields{};
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
					KalaMakeCore::PrintError("Include name '" + fieldName + "' was duplicated!");

					exit(1);
				}

				fields[fieldName] = fieldValues;
			}

			size_t total = 0;
			for (const auto& [_, v] : fields) total += v.size();
			data.includes.reserve(data.includes.size() + total);

			for (const auto& [_, v] : fields)
			{
				data.includes.insert(
					data.includes.end(),
					make_move_iterator(v.begin()),
					make_move_iterator(v.end()));	
			}
		}
		else if (categoryType == CategoryType::C_GLOBAL)
		{
			if (foundGlobal)
			{
				KalaMakeCore::PrintError("Global category was used more than once!");

				exit(1);
			}

			foundGlobal = true;

			vector<string> content = get_all_category_content(categoryName);

			unordered_map<string, vector<string>> fields{};
			for (const auto& c : content)
			{
				string fieldName{};
				vector<string> fieldValues{};
				ExtractFieldData(
					c, 
					fieldName, 
					fieldValues);

				if (fields.contains(fieldName))
				{
					KalaMakeCore::PrintError("Include name '" + fieldName + "' was duplicated!");

					exit(1);
				}

				fields[fieldName] = fieldValues;
			}
		}
		else if (categoryType == CategoryType::C_PROFILE)
		{
			for (const ProfileData& p : data.profiles)
			{
				if (p.profileName == categoryValue)
				{
					KalaMakeCore::PrintError("Profile name '" + categoryValue  + "' was used more than once!");

					exit(1);
				}
			}

			vector<string> content = get_all_category_content(categoryName, categoryValue);

			unordered_map<string, vector<string>> fields{};
			for (const auto& c : content)
			{
				string fieldName{};
				vector<string> fieldValues{};
				ExtractFieldData(
					c, 
					fieldName, 
					fieldValues);

				if (fields.contains(fieldName))
				{
					KalaMakeCore::PrintError("Include name '" + fieldName + "' was duplicated!");

					exit(1);
				}

				fields[fieldName] = fieldValues;
			}

			data.profiles.push_back(
				{
					.profileName = categoryValue
				});
		}
		else if (categoryType == CategoryType::C_POST_BUILD)
		{
			if (foundPostBuild)
			{
				KalaMakeCore::PrintError("Post build category was used more than once!");

				exit(1);
			}

			foundPostBuild = true;

			vector<string> content = get_all_category_content(categoryName, categoryValue);
		}
	}

	return data;
}