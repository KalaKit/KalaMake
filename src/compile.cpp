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
using KalaHeaders::KalaString::RemoveFromString;
using KalaHeaders::KalaString::SplitString;
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
	vector<string> sources{};
	vector<string> headers{};
	vector<string> links{};
	string type{};
	string name{};
	string standard{};
	vector<string> defines{};
	string buildPath{};
	string objPath{};
	vector<string> extensions{};
	vector<string> flags{};
	vector<string> customFlags{};
};

//kma path is the root directory where the kma file is stored at
static path kmaPath{};

//default build directory path relative to kmaPath if buildpath is not added or filled
static path defaultBuildPath = "build";
//default object directory path relative to kmaPath if objpath is not added or filled
static path defaultObjPath = "obj";

constexpr string_view EXE_VERSION_NUMBER = "1.0";
constexpr string_view KMA_VERSION_NUMBER = "1.0";
constexpr string_view KMA_VERSION_NAME = "#KMA VERSION 1.0";

static const array<string_view, 13> actionTypes =
{
	"compiler",   //what is used to compile this source code
	"sources",    //what source files are compiled
	"type",       //what is the target type of the final file
	"name",       //what is the name of the final file

	"standard",   //what is the language standard

	//optional fields

	"links",      //what libraries will be linked to the binary

	"headers",    //what header files are included (for C and C++)

	"defines",    //what compile-time defines will be linked to the binary
	"buildpath",  //where the binary will be built to
	"objpath",    //where the object files live at during compilation

	"extensions", //what language standard extensions will be used
	"flags",      //what flags will be passed to the compiler
	"customflags" //what KalaMake-specific flags will trigger extra actions
};

static const array<string_view, 3> supportedCompilers =
{
	"clang-cl", //for windows only, MSVC toolchain
	"clang++",  //for windows and linux, GNU toolchain for mingw, msys2 and similar

	"cl"        //windows-only MSVC compiler
};

static const array<string_view, 2> supportedSourceExtensions =
{
	".cpp",
	".c"
};
static const array<string_view, 2> supportedHeaderExtensions =
{
	".hpp",
	".h"
};

static const array<string_view, 3> supportedTypes =
{
	"executable", //creates a runnable executable
	"shared-lib", //creates .dll + .lib (or .so + .a on linux)
	"static-lib"  //creates .lib only (or .a only on linux)
};

static const array<string_view, 10> supportedStandards =
{
	"cpp11",
	"cpp14",
	"cpp17",
	"cpp20",
	"cpp23",

	"c89",
	"c99",
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

constexpr u8 MIN_NAME_LENGTH = 1u;
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
	vector<string> sources{};
	vector<string> headers{};
	vector<string> links{};
	string type{};
	string name{};
	string standard{};
	
	vector<string> defines{};
	string buildPath{};
	string objPath{};
	vector<string> extensions{};
	vector<string> flags{};
	vector<string> customFlags{};

	auto IsCompiler = [](string_view value) -> bool
		{
			return StartsWith(value, "compiler: ");
		};
	auto IsSource = [](string_view value) -> bool
		{
			return StartsWith(value, "sources: ");
		};
	auto IsHeader = [](string_view value) -> bool
		{
			return StartsWith(value, "headers: ");
		};
	auto IsLink = [](string_view value) -> bool
		{
			return StartsWith(value, "links: ");
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
	auto IsBuildPath = [](string_view value) -> bool
		{
			return StartsWith(value, "buildpath: ");
		};
	auto IsObjPath = [](string_view value) -> bool
		{
			return StartsWith(value, "objpath: ");
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

	auto IsAllowedSourceExtension = [](path value) -> bool
		{
			if (!value.has_extension()) return false;

			for (const auto& e : supportedSourceExtensions)
			{
				if (value.extension() == e) return true;
			}

			return false;
		};
	auto IsAllowedHeaderExtension = [](path value) -> bool
		{
			if (!value.has_extension()) return false;

			for (const auto& e : supportedHeaderExtensions)
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

	for (size_t i = 0; i < fileContent.size(); i++)
	{
		const string_view& line = fileContent[i];

		//ignore version line, empty lines and comments
		if (i == 0
			|| line.empty()
			|| StartsWith(line, "//"))
		{
			continue;
		}

		if (IsCompiler(line))
		{
			if (!compiler.empty())
			{
				PrintError("Failed to compile project because more than one compiler line was passed!");

				return;
			}

			string value = RemoveFromString(line, "compiler: ");

			if (!IsAllowedCompiler(value)) return;

			compiler = value;
		}
		else if (IsSource(line))
		{
			if (!sources.empty())
			{
				PrintError("Failed to compile project because more than one sources line was passed!");

				return;
			}

			string value = RemoveFromString(line, "sources: ");

			if (value.empty())
			{
				PrintError("Failed to compile project because no sources were passed!");

				return;
			}

			vector<string> foundSources = SplitString(value, ", ");
			if (ContainsDuplicates(foundSources)) RemoveDuplicates(foundSources);

			bool failedSourceSearch{};

			for (const auto& s : foundSources)
			{
				path sourcePath{}; 
				
				try
				{
					sourcePath = weakly_canonical(kmaPath / s);
				}
				catch (const filesystem_error&)
				{
					PrintError("Failed to compile project because source '" + s + "' could not be resolved!");

					failedSourceSearch = true;

					break;
				}

				if (!exists(sourcePath))
				{
					PrintError("Failed to compile project because source '" + sourcePath.string() + "' was not found!");

					failedSourceSearch = true;

					break;
				}

				if (!IsAllowedSourceExtension(sourcePath))
				{
					PrintError("Failed to compile project because source '" + sourcePath.string() + "' had an unsupported extension!");

					failedSourceSearch = true;

					break;
				}
			}

			if (failedSourceSearch) return;

			sources = std::move(foundSources);
		}
		else if (IsHeader(line))
		{
			if (!headers.empty())
			{
				PrintError("Failed to compile project because more than one headers line was passed!");

				return;
			}

			string value = RemoveFromString(line, "headers: ");

			if (value.empty())
			{
				PrintError("Failed to compile project because no headers were passed!");

				return;
			}

			vector<string> foundHeaders = SplitString(value, ", ");
			if (ContainsDuplicates(foundHeaders)) RemoveDuplicates(foundHeaders);

			bool failedHeaderSearch{};

			for (const auto& s : foundHeaders)
			{
				path headerPath{};

				try
				{
					headerPath = weakly_canonical(kmaPath / s);
				}
				catch (const filesystem_error&)
				{
					PrintError("Failed to compile project because header '" + s + "' could not be resolved!");

					failedHeaderSearch = true;

					break;
				}

				if (!exists(headerPath))
				{
					PrintError("Failed to compile project because header '" + headerPath.string() + "' was not found!");

					failedHeaderSearch = true;

					break;
				}

				if (!IsAllowedHeaderExtension(headerPath))
				{
					PrintError("Failed to compile project because header '" + headerPath.string() + "' had an unsupported extension!");

					failedHeaderSearch = true;

					break;
				}
			}

			if (failedHeaderSearch) return;

			headers = std::move(foundHeaders);
		}
		else if (IsLink(line))
		{
			if (!links.empty())
			{
				PrintError("Failed to compile project because more than one links line was passed!");

				return;
			}

			string value = RemoveFromString(line, "links: ");

			//ignore empty links field
			if (value.empty()) continue;

			vector<string> foundLinks = SplitString(value, ", ");
			if (ContainsDuplicates(foundLinks)) RemoveDuplicates(foundLinks);

			links = std::move(foundLinks);
		}
		else if (IsType(line))
		{
			if (!type.empty())
			{
				PrintError("Failed to compile project because more than one type line was passed!");

				return;
			}

			string value = RemoveFromString(line, "type: ");

			if (!IsAllowedType(value)) return;

			type = value;
		}
		else if (IsName(line))
		{
			if (!name.empty())
			{
				PrintError("Failed to compile project because more than one name line was passed!");

				return;
			}

			string value = RemoveFromString(line, "name: ");

			if (!IsAllowedName(value)) return;

			name = value;
		}
		else if (IsStandard(line))
		{
			if (!standard.empty())
			{
				PrintError("Failed to compile project because more than one standard line was passed!");

				return;
			}

			string value = RemoveFromString(line, "standard: ");

			if (!IsAllowedStandard(value)) return;

			standard = value;
		}
		else if (IsDefine(line))
		{
			if (!defines.empty())
			{
				PrintError("Failed to compile project because more than one defines line was passed!");

				return;
			}

			string value = RemoveFromString(line, "defines: ");

			//ignore empty defines field
			if (value.empty()) continue;

			vector<string> tempDefines = SplitString(value, ", ");
			if (ContainsDuplicates(tempDefines)) RemoveDuplicates(tempDefines);

			defines = std::move(tempDefines);
		}
		else if (IsBuildPath(line))
		{
			if (!buildPath.empty())
			{
				PrintError("Failed to compile project because more than one build path line was passed!");

				return;
			}

			path tempBuildPath = kmaPath / RemoveFromString(line, "buildpath: ");

			if (!exists(tempBuildPath))
			{
				PrintError("Failed to compile project because passed build path '" + tempBuildPath.string() + "' does not exist!");

				return;
			}

			if (!is_directory(tempBuildPath))
			{
				PrintError("Failed to compile project because passed build path '" + tempBuildPath.string() + "' does not lead to a directory!");

				return;
			}

			if (tempBuildPath.string() == objPath)
			{
				PrintError("Failed to compile project because passed build path '" + tempBuildPath.string() + "' is the same as obj path!");

				return;
			}

			buildPath = tempBuildPath.string();
		}
		else if (IsObjPath(line))
		{
			if (!objPath.empty())
			{
				PrintError("Failed to compile project because more than one obj path line was passed!");

				return;
			}

			path tempObjPath = kmaPath / RemoveFromString(line, "objpath: ");

			if (!exists(tempObjPath))
			{
				PrintError("Failed to compile project because passed obj path '" + tempObjPath.string() + "' does not exist!");

				return;
			}

			if (!is_directory(tempObjPath))
			{
				PrintError("Failed to compile project because passed obj path '" + tempObjPath.string() + "' does not lead to a directory!");

				return;
			}

			if (tempObjPath.string() == buildPath)
			{
				PrintError("Failed to compile project because passed obj path '" + tempObjPath.string() + "' is the same as build path!");

				return;
			}

			objPath = tempObjPath.string();
		}
		else if (IsExtension(line))
		{
			if (!extensions.empty())
			{
				PrintError("Failed to compile project because more than one extensions line was passed!");

				return;
			}

			string value = RemoveFromString(line, "extensions: ");

			//ignore empty customflags field
			if (value.empty()) continue;

			vector<string> tempExtensions = SplitString(value, ", ");
			if (ContainsDuplicates(tempExtensions)) RemoveDuplicates(tempExtensions);

			extensions = std::move(tempExtensions);
		}
		else if (IsFlag(line))
		{
			if (!flags.empty())
			{
				PrintError("Failed to compile project because more than one flags line was passed!");

				return;
			}

			string value = RemoveFromString(line, "flags: ");

			//ignore empty flags field
			if (value.empty()) continue;

			vector<string> tempFlags = SplitString(value, ", ");
			if (ContainsDuplicates(tempFlags)) RemoveDuplicates(tempFlags);

			flags = std::move(tempFlags);
		}
		else if (IsCustomFlag(line))
		{
			if (!customFlags.empty())
			{
				PrintError("Failed to compile project because more than one customflags line was passed!");

				return;
			}

			string value = RemoveFromString(line, "customflags: ");

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
			PrintError("Failed to compile project because unknown field '" + string(line) + "' was passed to the project file!");

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
	if (sources.empty())
	{
		PrintError("Failed to compile project because sources have no value!");

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
			.sources = std::move(sources),
			.headers = std::move(headers),
			.links = std::move(links),
			.type = type,
			.name = name,
			.standard = standard,
			.defines = std::move(defines),
			.buildPath = buildPath,
			.objPath = objPath,
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