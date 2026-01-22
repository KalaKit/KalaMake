//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <filesystem>
#include <iostream>

#include "KalaHeaders/log_utils.hpp"
#include "KalaHeaders/string_utils.hpp"
#include "KalaHeaders/file_utils.hpp"

#include "KalaCLI/include/core.hpp"

#include "compile.hpp"

using KalaHeaders::KalaLog::Log;
using KalaHeaders::KalaLog::LogType;

using KalaHeaders::KalaFile::ReadLinesFromFile;

using KalaHeaders::KalaString::StartsWith;
using KalaHeaders::KalaString::RemoveAllFromString;
using KalaHeaders::KalaString::SplitString;
using KalaHeaders::KalaString::ContainsString;

using KalaCLI::Core;

using std::string;
using std::to_string;
using std::string_view;
using std::ifstream;
using std::getline;
using std::ostringstream;
using std::vector;
using std::array;
using std::cin;
using std::filesystem::exists;
using std::filesystem::path;
using std::filesystem::current_path;
using std::filesystem::weakly_canonical;
using std::filesystem::is_directory;
using std::filesystem::filesystem_error;

using u8 = uint8_t;
using u32 = uint32_t;

struct CompileData
{
	string compiler{};
	vector<string> scripts{};
	string type{};
	string name{};
};

//kma path is the root directory where the kma file is stored at
static path kmaPath{};

constexpr string_view EXE_VERSION_NUMBER = "1.0";
constexpr string_view KMA_VERSION_NUMBER = "1.0";
constexpr string_view KMA_VERSION_NAME = "#KMA VERSION 1.0";

static const array<string, 4> actionTypes =
{
	"compiler", //what is used to compile this source code
	"scripts",  //what scripts are included
	"type",     //what is the target type of the final file
	"name"      //what is the name of the final file
};

static const array<string, 2> supportedCompilers =
{
	"clang-cl",
	"clang++"
};

static const array<string, 4> supportedScriptExtensions =
{
	".hpp",
	".h",
	".cpp",
	".c"
};

static const array<string, 2> supportedTypes =
{
	"exe",
	"dll"
};

constexpr u8 MIN_NAME_LENGTH = 3u;
constexpr u8 MAX_NAME_LENGTH = 20u;

static void HandleProjectContent(vector<string> fileContent);

static void CompileProject(const CompileData& compileData);

static void PrintError(const string& message)
{
	Log::Print(
		message,
		"KALAMAKE",
		LogType::LOG_ERROR,
		2);
}

namespace KalaMake
{
	void KalaMakeCore::Compile(const vector<string>& params)
	{
		ostringstream details{};

		details
			<< "     | exe version: " << EXE_VERSION_NUMBER.data() << "\n"
			<< "     | kma version: " << KMA_VERSION_NUMBER.data() << "\n"
			<< "\n==========================================================================================\n";

		Log::Print(details.str());

		path projectFile = params[1];

		string& currentDir = Core::GetCurrentDir();
		if (currentDir.empty()) currentDir = current_path().string();

		auto readprojectfile = [](path filePath) -> void
			{
				if (is_directory(filePath))
				{
					PrintError("Failed to compile project because its path '" + filePath.string() + "' leads to a directory!");

					return;
				}

				if (!filePath.has_extension()
					|| filePath.extension() != ".kma")
				{
					PrintError("Failed to compile project because its path '" + filePath.string() + "' has an incorrect extension!");

					return;
				}

				vector<string> content{};

				string result = ReadLinesFromFile(
					filePath,
					content);

				if (!result.empty())
				{
					PrintError("Failed to read project file '" + filePath.string() + "'! Reason: " + result);

					return;
				}

				if (content.empty())
				{
					PrintError("Failed to compile project at '" + filePath.string() + "' because it was empty!");

					return;
				}

				kmaPath = filePath.parent_path();

				HandleProjectContent(content);
			};

		//partial path was found

		path correctTarget{};

		try
		{
			correctTarget = weakly_canonical(path(Core::GetCurrentDir()) / projectFile);
		}
		catch (const filesystem_error&)
		{
			PrintError("Failed to compile project because partial path via '" + projectFile.string() + "' could not be resolved!");

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
			PrintError("Failed to compile project because full path '" + projectFile.string() + "' could not be resolved!");

			return;
		}

		if (exists(correctTarget))
		{
			readprojectfile(correctTarget);

			return;
		}

		PrintError("Failed to compile project because its path '" + projectFile.string() + "' does not exist!");
	}
}

void HandleProjectContent(vector<string> fileContent)
{
	string compiler{};
	vector<string> scripts{};
	string type{};
	string name{};

	auto IsCompiler = [](const string& value) -> bool
		{
			return StartsWith(value, "compiler: ");
		};
	auto IsScript = [](const string& value) -> bool
		{
			return StartsWith(value, "scripts: ");
		};
	auto IsType = [](const string& value) -> bool
		{
			return StartsWith(value, "type: ");
		};
	auto IsName = [](const string& value) -> bool
		{
			return StartsWith(value, "name: ");
		};

	auto IsAllowedCompiler = [](const string& value) -> bool
		{
			return value == "clang-cl"
				|| value == "clang++";
		};

	auto HasAllowedExtension = [](path value) -> bool
		{
			return value.has_extension()
				&& (value.extension() == ".hpp"
				|| value.extension() == ".h"
				|| value.extension() == ".cpp"
				|| value.extension() == ".c");
		};

	auto IsAllowedType = [](const string& value) -> bool
		{
			return value == "exe"
				|| value == "dll";
		};

	auto IsAllowedName = [](const string& value) -> bool
		{
			if (value.size() < MIN_NAME_LENGTH)
			{
				PrintError("Failed to compile project because name value is too short!");

				return false;
			}
			if (value.size() > MAX_NAME_LENGTH)
			{
				PrintError("Failed to compile project because name value is too long!");

				return false;
			}

			for (unsigned char c : value)
			{
				if ((c >= 'a' && c <= 'z')
					|| (c >= 'A' && c <= 'Z')
					|| (c >= '0' && c <= '9')
					|| c == '.'
					|| c == '-'
					|| c == '_')
				{
					continue;
				}

				PrintError("Failed to compile project because name value contains illegal characters!");

				return false;
			}

			return true;
		};

	if (fileContent[0] != KMA_VERSION_NAME)
	{
		PrintError("Failed to compile project because kma version field value is incorrect or missing on the first line!");

		return;
	}

	for (int i = 0; i < fileContent.size(); i++)
	{
		const string& c = fileContent[i];

		//ignore version line, empty lines and comments
		if (i == 0
			|| c.empty()
			|| StartsWith(c, "//"))
		{
			continue;
		}

		if (IsCompiler(c))
		{
			if (!compiler.empty())
			{
				PrintError("Failed to compile project because more than one compiler line was passed!");

				return;
			}

			string value = RemoveAllFromString(c, "compiler: ");

			if (!IsAllowedCompiler(value)) return;

			compiler = value;
		}
		else if (IsScript(c))
		{
			if (!scripts.empty())
			{
				PrintError("Failed to compile project because more than one scripts line was passed!");

				return;
			}

			string value = RemoveAllFromString(c, "scripts: ");

			if (value.empty())
			{
				PrintError("Failed to compile project because no scripts were passed!");

				return;
			}

			vector<string> foundScripts = SplitString(value, ", ");

			bool failedScriptSearch{};

			for (const auto& s : foundScripts)
			{
				if (!HasAllowedExtension(s))
				{
					PrintError("Failed to compile project because script '" + s + "' had an unsupported extension!");

					failedScriptSearch = true;

					break;
				}

				path scriptPath{}; 
				
				try
				{
					scriptPath = weakly_canonical(kmaPath / s);
				}
				catch (const filesystem_error&)
				{
					PrintError("Failed to compile project because script '" + s + "' could not be resolved!");

					failedScriptSearch = true;

					break;
				}

				if (!exists(scriptPath))
				{
					PrintError("Failed to compile project because script '" + s + "' was not found!");

					failedScriptSearch = true;

					break;
				}
			}

			if (failedScriptSearch) return;

			scripts = std::move(foundScripts);
		}
		else if (IsType(c))
		{
			if (!type.empty())
			{
				PrintError("Failed to compile project because more than one type line was passed!");

				return;
			}

			string value = RemoveAllFromString(c, "type: ");

			if (!IsAllowedType(value)) return;

			type = value;
		}
		else if (IsName(c))
		{
			if (!name.empty())
			{
				PrintError("Failed to compile project because more than one name line was passed!");

				return;
			}

			string value = RemoveAllFromString(c, "name: ");

			if (!IsAllowedName(value)) return;

			name = value;
		}
	}

	if (compiler.empty())
	{
		PrintError("Failed to compile project because no compiler value was passed!");

		return;
	}
	if (scripts.empty())
	{
		PrintError("Failed to compile project because no scripts were passed!");

		return;
	}
	if (type.empty())
	{
		PrintError("Failed to compile project because no type value was passed!");

		return;
	}
	if (name.empty())
	{
		PrintError("Failed to compile project because no name value was passed!");

		return;
	}

	CompileData data =
	{
		.compiler = compiler,
		.scripts = std::move(scripts),
		.type = type,
		.name = name
	};

	CompileProject(data);
}

void CompileProject(const CompileData& compileData)
{
	Log::Print(
		"Passed all kma parsing checks and starting to compile.",
		"KALAMAKE",
		LogType::LOG_INFO);
}