//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include <sstream>
#include <filesystem>

#include "KalaHeaders/log_utils.hpp"
#include "KalaHeaders/file_utils.hpp"

#include "core.hpp"

#include "core/kma_core.hpp"

using KalaHeaders::KalaLog::Log;
using KalaHeaders::KalaLog::LogType;

using KalaHeaders::KalaFile::ReadLinesFromFile;

using KalaCLI::Core;

using std::ostringstream;
using std::filesystem::exists;
using std::filesystem::path;
using std::filesystem::current_path;
using std::filesystem::weakly_canonical;
using std::filesystem::is_directory;
using std::filesystem::filesystem_error;

namespace KalaMake::Core
{
	constexpr string_view EXE_VERSION_NUMBER = "1.0";
	constexpr string_view KMA_VERSION_NUMBER = "1.0";

	//kma path is the root directory where the kma file is stored at
	static path kmaPath{};

	static const unordered_map<FieldType, string_view, EnumHash<FieldType>> fieldTypes =
	{
		{ FieldType::T_BINARY_TYPE,    "binarytype" },
		{ FieldType::T_COMPILER,       "compiler" },
		{ FieldType::T_STANDARD,       "standard" },
		{ FieldType::T_TARGET_PROFILE, "targetprofile" },

		{ FieldType::T_BINARY_NAME,   "binaryname" },
		{ FieldType::T_BUILD_TYPE,    "buildtype" },
		{ FieldType::T_BUILD_PATH,    "buildpath" },
		{ FieldType::T_SOURCES,       "sources" },
		{ FieldType::T_HEADERS,       "headers" },
		{ FieldType::T_LINKS,         "links" },
		{ FieldType::T_WARNING_LEVEL, "warninglevel" },
		{ FieldType::T_DEFINES,       "defines" },
		{ FieldType::T_FLAGS,         "flags" },
		{ FieldType::T_CUSTOM_FLAGS,  "customflags" }
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
			"Starting to parse the kma file '" + filePath.string() + "'"
			"\n\n==========================================================================================\n",
			"KALAMAKE",
			LogType::LOG_INFO);
	}

	void KalaMakeCore::Generate(
		const path& filePath,
		const vector<string>& lines)
	{
		Log::Print(
			"Starting to generate a solution from the kma file '" + filePath.string() + "'"
			"\n\n==========================================================================================\n",
			"KALAMAKE",
			LogType::LOG_INFO);
	}

	bool KalaMakeCore::IsFieldType(string_view value)
	{
		return ContainsValue(fieldTypes, value);
	}

	bool KalaMakeCore::IsCStandard(string_view value)
	{
		StandardType type{};

		return StringToEnum(value, standardTypes, type)
			&& (type == StandardType::C_89
				|| type == StandardType::C_99
				|| type == StandardType::C_11
				|| type == StandardType::C_17
				|| type == StandardType::C_23
				|| type == StandardType::C_LATEST);
	}
	bool KalaMakeCore::IsCPPStandard(string_view value)
	{
		StandardType type{};

		return StringToEnum(value, standardTypes, type)
			&& (type == StandardType::CPP_11
				|| type == StandardType::CPP_14
				|| type == StandardType::CPP_17
				|| type == StandardType::CPP_20
				|| type == StandardType::CPP_23
				|| type == StandardType::CPP_26
				|| type == StandardType::CPP_LATEST);
	}

	bool KalaMakeCore::IsC_CPPCompiler(string_view value)
	{
		CompilerType type{};

		return StringToEnum(value, compilerTypes, type)
			&& (type == CompilerType::C_CLANG_CL
				|| type == CompilerType::C_CL
				|| type == CompilerType::C_CLANG
				|| type == CompilerType::C_CLANGPP
				|| type == CompilerType::C_GCC
				|| type == CompilerType::C_GPP);
	}
	bool KalaMakeCore::IsMSVCCompiler(string_view value)
	{
		CompilerType type{};

		return StringToEnum(value, compilerTypes, type)
			&& (type == CompilerType::C_CLANG_CL
				|| type == CompilerType::C_CL);
	}
	bool KalaMakeCore::IsGNUCompiler(string_view value)
	{
		CompilerType type{};

		return StringToEnum(value, compilerTypes, type)
			&& (type == CompilerType::C_CLANG
				|| type == CompilerType::C_CLANGPP
				|| type == CompilerType::C_GCC
				|| type == CompilerType::C_GPP);
	}

	bool KalaMakeCore::IsBinaryType(string_view value)
	{
		return ContainsValue(binaryTypes, value);
	}

	bool KalaMakeCore::IsWarningLevel(string_view value)
	{
		return ContainsValue(warningLevels, value);
	}

	bool KalaMakeCore::IsCustomFlag(string_view value)
	{
		return ContainsValue(customFlags, value);
	}

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