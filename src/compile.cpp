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

#include "KalaHeaders/core_utils.hpp"
#include "KalaHeaders/log_utils.hpp"
#include "KalaHeaders/string_utils.hpp"
#include "KalaHeaders/file_utils.hpp"

#include "KalaCLI/include/core.hpp"

#include "compile.hpp"

using KalaHeaders::KalaCore::ContainsDuplicates;
using KalaHeaders::KalaCore::RemoveDuplicates;

using KalaHeaders::KalaLog::Log;
using KalaHeaders::KalaLog::LogType;

using KalaHeaders::KalaFile::ReadLinesFromFile;

using KalaHeaders::KalaString::StartsWith;
using KalaHeaders::KalaString::RemoveAllFromString;
using KalaHeaders::KalaString::SplitString;
using KalaHeaders::KalaString::ContainsString;
using KalaHeaders::KalaString::MakeViews;

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
	string standard{};
	vector<string> defines{};
	string targetDir{};
	string objDir{};
	vector<string> extensions{};
	vector<string> flags{};
	vector<string> customFlags{};
};

//kma path is the root directory where the kma file is stored at
static path kmaPath{};

static path defaultTargetDir = "build";
static path defaultObjDir = "obj";

constexpr string_view EXE_VERSION_NUMBER = "1.0";
constexpr string_view KMA_VERSION_NUMBER = "1.0";
constexpr string_view KMA_VERSION_NAME = "#KMA VERSION 1.0";

static const array<string_view, 11> actionTypes =
{
	"compiler",   //what is used to compile this source code
	"scripts",    //what scripts are included
	"type",       //what is the target type of the final file
	"name",       //what is the name of the final file

	"standard",   //what is the language standard

	//optional fields

	"defines",    //compile-time defines linked to the binary
	"targetdir",  //where the binary will be built to, defaults to /build/release if using standard release etc
	"objdir",     //where the intermediate files live at during compilation

	"extensions", //what language standard extensions will be used
	"flags",      //what flags will be passed to the compiler
	"customflags" //KalaMake-specific flags that trigger extra actions
};

static const array<string_view, 3> supportedCompilers =
{
	"clang-cl", //for windows, MSVC toolchain
	"clang++",  //for windows, GNU toolchain for mingw, msys2 and similar

	"cl"        //windows MSVC compiler
};

static const array<string_view, 4> supportedScriptExtensions =
{
	".hpp",
	".h",
	".cpp",
	".c"
};

static const array<string_view, 3> supportedTypes =
{
	"executable", //creates a runnable executable
	"shared-lib", //creates dll + lib (or .so + .a on linux)
	"static-lib"  //creates lib only (or .a only on linux)
};

static const array<string_view, 9> supportedStandards =
{
	"cpp11",
	"cpp14",
	"cpp17",
	"cpp20",
	"cpp23",

	"c98",
	"c11",
	"c17",
	"c23"
};

static const array<string_view, 9> supportedCustomFlags =
{
	"useninja",         //works on clang and cl, uses the multithreaded benefits of ninja for faster compilation

	"standardrequired", //fails the build if the compiler cannot support the requested standard

	//build types (do not add more than one of these)

	"debug",            //only create debug build
	"release",          //only create regular release build
	"reldebug",         //only create release with debug symbols
	"minsizerel",       //only create minimum size release build
	"debug-release",    //create debug + regular release
	"debug-reldebug",   //create debug + release with debug symbols
	"debug-minsizerel"  //create debug + minimum size release
};

constexpr u8 MIN_NAME_LENGTH = 3u;
constexpr u8 MAX_NAME_LENGTH = 20u;

static void HandleProjectContent(const vector<string>& fileContent);

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

void HandleProjectContent(const vector<string>& fileContent)
{
	string compiler{};
	vector<string> scripts{};
	string type{};
	string name{};
	string standard{};
	
	vector<string> defines{};
	string targetDir{};
	string objDir{};
	vector<string> extensions{};
	vector<string> flags{};
	vector<string> customFlags{};

	auto IsCompiler = [](string_view value) -> bool
		{
			return StartsWith(value, "compiler: ");
		};
	auto IsScript = [](string_view value) -> bool
		{
			return StartsWith(value, "scripts: ");
		};
	auto IsType = [](string_view value) -> bool
		{
			return StartsWith(value, "type: ");
		};
	auto IsName = [](string_view value) -> bool
		{
			return StartsWith(value, "name: ");
		};
	auto IsStandard = [](string_view value) -> bool
		{
			return StartsWith(value, "standard: ");
		};
	auto IsDefine = [](string_view value) -> bool
		{
			return StartsWith(value, "defines: ");
		};
	auto IsTargetDir = [](string_view value) -> bool
		{
			return StartsWith(value, "targetdir: ");
		};
	auto IsObjDir = [](string_view value) -> bool
		{
			return StartsWith(value, "objdir: ");
		};
	auto IsExtension = [](string_view value) -> bool
		{
			return StartsWith(value, "extensions: ");
		};
	auto IsFlag = [](string_view value) -> bool
		{
			return StartsWith(value, "flags: ");
		};
	auto IsCustomFlag = [](string_view value) -> bool
		{
			return StartsWith(value, "customflags: ");
		};

	auto IsAllowedCompiler = [](string_view value) -> bool
		{
			return value == "clang-cl"
				|| value == "clang++";
		};

	auto IsAllowedScriptExtension = [](path value) -> bool
		{
			if (!value.has_extension()) return false;

			for (const auto& e : supportedScriptExtensions)
			{
				if (value.extension() == e) return true;
			}

			return false;
		};

	auto IsAllowedType = [](string_view value) -> bool
		{
			for (const auto& t : supportedTypes)
			{
				if (value == t) return true;
			}
			
			PrintError("Failed to compile project because type '" + string(value) + "' was not found!");

			return false;
		};

	auto IsAllowedName = [](string_view value) -> bool
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

	auto IsAllowedStandard = [](string_view value) -> bool
		{
			for (const auto& f : supportedStandards)
			{
				if (value == f) return true;
			}

			PrintError("Failed to compile project because standard '" + string(value) + "' was not found!");

			return false;
		};

	auto IsAllowedCustomFlag = [](string_view value) -> bool
		{
			for (const auto& f : supportedCustomFlags)
			{
				if (value == f) return true;
			}

			PrintError("Failed to compile project because custom flag '" + string(value) + "' was not found!");

			return false;
		};

	if (fileContent[0] != KMA_VERSION_NAME)
	{
		PrintError("Failed to compile project because kma version field value is incorrect or missing on the first line!");

		return;
	}

	//
	// READ FILE CONTENT IN KMA FILE
	//

	for (int i = 0; i < fileContent.size(); i++)
	{
		const string_view& c = fileContent[i];

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
					PrintError("Failed to compile project because script '" + scriptPath.string() + "' was not found!");

					failedScriptSearch = true;

					break;
				}

				if (!IsAllowedScriptExtension(scriptPath))
				{
					PrintError("Failed to compile project because script '" + scriptPath.string() + "' had an unsupported extension!");

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
		else if (IsStandard(c))
		{
			if (!standard.empty())
			{
				PrintError("Failed to compile project because more than one standard line was passed!");

				return;
			}

			string value = RemoveAllFromString(c, "standard: ");

			if (!IsAllowedStandard(c)) return;

			standard = value;
		}
		else if (IsDefine(c))
		{
			if (!defines.empty())
			{
				PrintError("Failed to compile project because more than one defines line was passed!");

				return;
			}

			string value = RemoveAllFromString(c, "defines: ");

			//ignore empty defines field
			if (value.empty()) continue;

			vector<string> tempDefines = SplitString(value, ", ");

			if (ContainsDuplicates(tempDefines)) RemoveDuplicates(tempDefines);

			defines = std::move(tempDefines);
		}
		else if (IsTargetDir(c))
		{
			if (!targetDir.empty())
			{
				PrintError("Failed to compile project because more than one target dir line was passed!");

				return;
			}

			path tempTargetDir = kmaPath / c;

			if (!exists(tempTargetDir))
			{
				PrintError("Failed to compile project because passed target dir path '" + tempTargetDir.string() + "' does not exist!");

				return;
			}

			if (!is_directory(tempTargetDir))
			{
				PrintError("Failed to compile project because passed target dir path '" + tempTargetDir.string() + "' does not lead to a directory!");

				return;
			}

			if (tempTargetDir.string() == objDir)
			{
				PrintError("Failed to compile project because passed target dir path '" + tempTargetDir.string() + "' is the same as obj dir path!");

				return;
			}

			targetDir = tempTargetDir.string();
		}
		else if (IsObjDir(c))
		{
			if (!objDir.empty())
			{
				PrintError("Failed to compile project because more than one obj dir line was passed!");

				return;
			}

			path tempObjDir = kmaPath / c;

			if (!exists(tempObjDir))
			{
				PrintError("Failed to compile project because passed obj dir path '" + tempObjDir.string() + "' does not exist!");

				return;
			}

			if (!is_directory(tempObjDir))
			{
				PrintError("Failed to compile project because passed obj dir path '" + tempObjDir.string() + "' does not lead to a directory!");

				return;
			}

			if (tempObjDir.string() == targetDir)
			{
				PrintError("Failed to compile project because passed obj dir path '" + tempObjDir.string() + "' is the same as target dir path!");

				return;
			}

			objDir = tempObjDir.string();
		}
		else if (IsExtension(c))
		{
			if (!extensions.empty())
			{
				PrintError("Failed to compile project because more than one extensions line was passed!");

				return;
			}

			string value = RemoveAllFromString(c, "extensions: ");

			//ignore empty customflags field
			if (value.empty()) continue;

			vector<string> tempExtensions = SplitString(value, ", ");

			if (ContainsDuplicates(tempExtensions)) RemoveDuplicates(tempExtensions);

			extensions = std::move(extensions);
		}
		else if (IsFlag(c))
		{
			if (!flags.empty())
			{
				PrintError("Failed to compile project because more than one flags line was passed!");

				return;
			}

			string value = RemoveAllFromString(c, "flags: ");

			//ignore empty flags field
			if (value.empty()) continue;

			vector<string> tempFlags = SplitString(value, ", ");

			if (ContainsDuplicates(tempFlags)) RemoveDuplicates(tempFlags);

			flags = std::move(tempFlags);
		}
		else if (IsCustomFlag(c))
		{
			if (!customFlags.empty())
			{
				PrintError("Failed to compile project because more than one customflags line was passed!");

				return;
			}

			string value = RemoveAllFromString(c, "customflags: ");

			//ignore empty customflags field
			if (value.empty()) continue;

			vector<string> tempCustomflags = SplitString(value, ", ");

			if (ContainsDuplicates(tempCustomflags)) RemoveDuplicates(tempCustomflags);

			bool failedCustomFlagsSearch{};

			for (const auto& f : tempCustomflags)
			{
				if (!IsAllowedCustomFlag(f))
				{
					failedCustomFlagsSearch = true;

					break;
				}
			}

			if (failedCustomFlagsSearch) return;

			customFlags = std::move(tempCustomflags);
		}
		else
		{
			PrintError("Failed to compile project because unknown field '" + string(c) + "' was passed to the project file!");

			return;
		}
	}

	//
	// POST-READ EMPTY CHECKS
	//

	if (compiler.empty())
	{
		PrintError("Failed to compile project because compiler has no value!");

		return;
	}
	if (scripts.empty())
	{
		PrintError("Failed to compile project because scripts have no value!");

		return;
	}
	if (type.empty())
	{
		PrintError("Failed to compile project because type has no value!");

		return;
	}
	if (name.empty())
	{
		PrintError("Failed to compile project because name has no value!");

		return;
	}
	if (standard.empty())
	{
		PrintError("Failed to compile project because standard has no value!");

		return;
	}

	CompileProject(
		{
			.compiler = compiler,
			.scripts = std::move(scripts),
			.type = type,
			.name = name,
			.standard = standard,
			.defines = std::move(defines),
			.targetDir = targetDir,
			.objDir = objDir,
			.extensions = std::move(extensions),
			.flags = std::move(flags),
			.customFlags = std::move(customFlags),
		});
}

void CompileProject(const CompileData& compileData)
{
	Log::Print(
		"Passed all kma parsing checks and starting to compile.",
		"KALAMAKE",
		LogType::LOG_INFO);
}