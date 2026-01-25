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
using KalaHeaders::KalaFile::GetRelativeFiles;
using KalaHeaders::KalaFile::CreateNewDirectory;

using KalaHeaders::KalaString::ContainsString;
using KalaHeaders::KalaString::StartsWith;
using KalaHeaders::KalaString::RemoveFromString;
using KalaHeaders::KalaString::SplitString;
using KalaHeaders::KalaString::MakeViews;
using KalaHeaders::KalaString::EndsWith;

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
using std::filesystem::directory_iterator;
using std::filesystem::recursive_directory_iterator;
using std::filesystem::is_regular_file;

struct CompileData
{
	string name{};
	string type{};
	string standard{};
	string compiler{};
	vector<string> sources{};

	string buildPath{};
	string objPath{};
	vector<string> headers{};
	vector<string> rellinks{};
	vector<string> deblinks{};

	string warningLevel;
	vector<string> defines{};
	vector<string> extensions{};

	vector<string> relflags{};
	vector<string> debflags{};
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

constexpr string_view cl_ide_bat_2026 = "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat";
constexpr string_view cl_build_bat_2026 = "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat";
constexpr string_view cl_ide_bat_2022 = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat";
constexpr string_view cl_build_bat_2022 = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat";

static path foundCLPath{};

static const array<string_view, 16> actionTypes =
{
	"name",         //what is the name of the final file
	"type",         //what is the target type of the final file
	"compiler",     //what is used to compile this source code
	"standard",     //what is the language standard
	"sources",      //what source files are compiled
	
	//optional fields

	"buildpath",    //where the binary will be built to
	"objpath",      //where the object files live at when linked
	"headers",      //what header files are included (for C and C++)
	"rellinks",     //what release libraries will be linked to the release binary
	"deblinks",     //what debug libraries will be linked to the debug binary
	
	"warninglevel", //what warning level should the compiler use, defaults to strong if unset
	"defines",      //what compile-time defines will be linked to the binary
	"extensions",   //what language standard extensions will be used

	"relflags", //what flags will be passed to the compiler in any release build
	"debflags",   //what flags will be passed to the compiler in the debug build
	"customflags"   //what KalaMake-specific flags will trigger extra actions
};

static const array<string_view, 5> supportedCompilers =
{
	"clang-cl", //windows only, MSVC-style flags
	"cl",       //windows only, MSVC-style flags

	"clang++",  //windows + linux, GNU flags
	"gcc",      //linux, GNU flags, used for C
	"g++"       //linux, GNU flags, used for C++
};

static const array<string_view, 4> supportedTypes =
{
	//creates a runnable executable
	"executable",

	//creates a linkable .lib on windows,
	//creates a linkable .a on linux
	"link-only",

	//creates a .dll on windows,
	//creates a .so on linux
	"runtime-only",

	//creates a .dll and a linkable .lib on windows,
	//creates a .so on linux, same as runtime-only
	"link-runtime"
};

static const array<string_view, 6> supportedCStandards =
{
	"c89",
	"c99",
	"c11",
	"c17",
	"c23",
	"clatest"
};
static const array<string_view, 7> supportedCPPStandards =
{
	"c++11",
	"c++14",
	"c++17",
	"c++20",
	"c++23",
	"c++26",
	"c++latest",
};

//same warning types are used for both Windows and Linux,
//their true meanings change depending on which OS is used
static const array<string_view, 6> supportedWarningTypes =
{
	//no warnings
	//  Windows: /W0
	//  Linux:   -w
	"none",

	//very basic warnings
	//  Windows: /W1
	//  Linux:   -Wall
	"basic",

	//common, useful warnings
	//  Windows: /W2
	//  Linux:   -Wall
	//           -Wextra
	"normal",

	//strong warnings, recommended default
	//  Windows: /W3
	//  Linux:   -Wall
	//           -Wextra
	//           -Wpedantic
	"strong",

	//very strict, high signal warnings
	//  Windows: /W4
	//  Linux:   -Wall
	//           -Wextra
	//           -Wpedantic
	//           -Wshadow
	//           -Wconversion
	//           -Wsign-conversion
	"strict",

	//everything
	//  Windows: (cl + clang-cl): /Wall
	// 
	//  Windows/Linux (clang++):  -Wall
	//                            -Wextra
	//                            -Wpedantic
	//                            -Weverything
	// 
	//  Linux (GCC + G++):        -Wall
	//                            -Wextra
	//                            -Wpedantic
	//                            -Wshadow
	//                            -Wconversion
	//                            -Wsign-conversion
	//                            -Wcast-align
	//                            -Wnull-dereference
	//                            -Wdouble-promotion
	//                            -Wformat=2
	"all"
};

static const array<string_view, 8> supportedCustomFlags =
{
	//works on clang and cl, uses the multithreaded benefits of ninja for faster compilation
	"use-ninja",

	//should object files be kept or not? works only for languages
	//that support object files for faster recompilation
	"keep-obj",

	//fails the build if the compiler cannot support the requested standard
	//  cl + clang-cl:       nothing
	//  gcc + g++ + clang++: nothing
	"standard-required",

	//treats all warnings as errors
	//  cl + clang-cl:       /WX
	//  gcc + g++ + clang++: -Werror
	"warnings-as-errors",

	//
	// build types
	//

	//only create debug build
	//  cl + clang-cl:       /Od /Zi
	//  gcc + g++ + clang++: -O0 -g
	"debug",

	//only create standard release build
	//  cl + clang-cl:       /O2
	//  gcc + g++ + clang++: -O2
	"release",

	//only create release with debug symbols
	//  cl + clang-cl:       /O2 /Zi
	//  gcc + g++ + clang++: -O2 -g
	"reldebug",

	//only create minimum size release build
	//  cl + clang-cl:       /O1
	//  gcc + g++ + clang++: -Os
	"minsizerel"
};

constexpr size_t MIN_NAME_LENGTH = 1;
constexpr size_t MAX_NAME_LENGTH = 20;

static void HandleProjectContent(const vector<string>& fileContent);

static void CompileProject(const CompileData& compileData);

static void CompileStaticLib(const CompileData& compileData);

static void PrintError(const string& message)
{
	Log::Print(
		message,
		"KALAMAKE",
		LogType::LOG_ERROR,
		2);
}

static bool IsCStandard(string_view value)
{
	return ContainsString(supportedCStandards, value);

	return find(
		supportedCStandards.begin(), 
		supportedCStandards.end(), 
		value) 
		!= supportedCStandards.end();
}
static bool IsCPPStandard(string_view value)
{
	return find(
		supportedCPPStandards.begin(),
		supportedCPPStandards.end(),
		value)
		!= supportedCPPStandards.end();
}

namespace KalaMake
{
	void KalaMakeCore::Initialize(const vector<string>& params)
	{
		ostringstream details{};

		details
			<< "     | exe version: " << EXE_VERSION_NUMBER.data() << "\n"
			<< "     | kma version: " << KMA_VERSION_NUMBER.data() << "\n";

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

				Log::Print(
					"Starting to parse kma file '" + filePath.string() + "'"
					"\n\n==========================================================================================\n",
					"KALAMAKE",
					LogType::LOG_INFO);

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
	string name{};
	string type{};
	string standard{};
	string compiler{};
	vector<string> sources{};

	string buildPath{};
	string objPath{};
	vector<string> headers{};
	vector<string> rellinks{};
	vector<string> deblinks{};

	string warningLevel;
	vector<string> defines{};
	vector<string> extensions{};

	vector<string> relflags{};
	vector<string> debflags{};
	vector<string> customFlags{};

	auto IsName = [](string_view value) -> bool
		{
			return StartsWith(value, "name: ");
		};
	auto IsType = [](string_view value) -> bool
		{
			return StartsWith(value, "type: ");
		};
	auto IsCompiler = [](string_view value) -> bool
		{
			return StartsWith(value, "compiler: ");
		};
	auto IsStandard = [](string_view value) -> bool
		{
			return StartsWith(value, "standard: ");
		};
	auto IsSource = [](string_view value) -> bool
		{
			return StartsWith(value, "sources: ");
		};

	auto IsBuildPath = [](string_view value) -> bool
		{
			return StartsWith(value, "buildpath: ");
		};
	auto IsObjPath = [](string_view value) -> bool
		{
			return StartsWith(value, "objpath: ");
		};
	auto IsHeader = [](string_view value) -> bool
		{
			return StartsWith(value, "headers: ");
		};
	auto IsRelLink = [](string_view value) -> bool
		{
			return StartsWith(value, "rellinks: ");
		};
	auto IsDebLink = [](string_view value) -> bool
		{
			return StartsWith(value, "deblinks: ");
		};

	auto IsWarningLevel = [](string_view value) -> bool
		{
			return StartsWith(value, "warninglevel: ");
		};
	auto IsDefine = [](string_view value) -> bool
		{
			return StartsWith(value, "defines: ");
		};
	auto IsExtension = [](string_view value) -> bool
		{
			return StartsWith(value, "extensions: ");
		};

	auto IsRelFlag = [](string_view value) -> bool
		{
			return StartsWith(value, "relflags: ");
		};
	auto IsDebFlag = [](string_view value) -> bool
		{
			return StartsWith(value, "debflags: ");
		};
	auto IsCustomFlag = [](string_view value) -> bool
		{
			return StartsWith(value, "customflags: ");
		};

	auto IsSupportedName = [](string_view value) -> bool
		{
			if (value.size() < MIN_NAME_LENGTH)
			{
				PrintError("Failed to compile project because name is too short!");

				return false;
			}
			if (value.size() > MAX_NAME_LENGTH)
			{
				PrintError("Failed to compile project because name is too long!");

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

				PrintError("Failed to compile project because name contains illegal characters!");

				return false;
			}

			return true;
		};
	auto IsSupportedType = [](string_view value) -> bool
		{
			for (const auto& t : supportedTypes)
			{
				if (value == t) return true;
			}

			PrintError("Failed to compile project because build type '" + string(value) + "' is not supported!");

			return false;
		};
	auto IsSupportedStandard = [](string_view value) -> bool
		{
			if (IsCPPStandard(value)
				|| IsCStandard(value))
			{
				return true;
			}

			PrintError("Failed to compile project because standard '" + string(value) + "' is not supported!");

			return false;
		};
	auto IsSupportedCompiler = [](string_view value) -> bool
		{
			for (const auto& e : supportedCompilers)
			{
				if (value == e) return true;
			}

			PrintError("Failed to compile project because compiler '" + string(value) + "' is not supported!");

			return false;
		};
	auto IsSupportedSourceExtension = [](string_view standard, const path& value) -> bool
		{
			if (!exists(value))
			{
				Log::Print(
					"Skipped invalid source file '" + value.string() + "' because it was not found.",
					"KALAMAKE",
					LogType::LOG_WARNING);

				return false;
			}

			if (is_directory(value))
			{
				Log::Print(
					"Skipped invalid source file '" + value.string() + "' because it is a directory.",
					"KALAMAKE",
					LogType::LOG_WARNING);

				return false;
			}

			if (IsCStandard(standard)
				&& value.extension() != ".c")
			{
				Log::Print(
					"Skipped invalid source file '" + value.string() + "' because it is not supported by the C standard.",
					"KALAMAKE",
					LogType::LOG_WARNING);

				return false;
			}

			if (IsCPPStandard(standard)
				&& value.extension() != ".cpp")
			{
				Log::Print(
					"Skipped invalid source file '" + value.string() + "' because it is not supported by the C++ standard.",
					"KALAMAKE",
					LogType::LOG_WARNING);

				return false;
			}

			return true;
		};

	auto IsSupportedBuildPath = [](
		string_view compiler,
		string_view type,
		string_view standard,
		const path& value) -> bool
		{
			if (!exists(value))
			{
				//treat all extensionless values as dirs and return true
				if (!value.has_extension()) return true;

				PrintError("Failed to compile project because passed build path '" + value.string() + "' does not exist!");

				return false;
			}

			if (is_directory(value)) return true;
			else
			{
				if (IsCPPStandard(standard)
					|| IsCStandard(standard))
				{
					if (type == "executable")
					{
						if ((compiler == "clang-cl"
							|| compiler == "cl")
							&& value.extension() != ".exe")
						{
							PrintError("Failed to compile project because passed build path '" + value.string() + "' executable extension is not valid!");

							return false;
						}
						else if (compiler == "clang++"
								 || compiler == "gcc"
								 || compiler == "g++")
						{	
#ifdef _WIN32
							if (value.extension() != ".exe")
							{
								PrintError("Failed to compile project because passed build path '" + value.string() + "' executable extension is not valid!");

								return false;
							}
#else
							if (value.has_extension())
							{
								PrintError("Failed to compile project because passed build path '" + value.string() + "' executable extension is not valid!");

								return false;
							}
#endif						
						}
					}
					else if (type == "link-runtime")
					{
						if ((compiler == "clang-cl"
							|| compiler == "cl")
							&& value.extension() != ".dll")
						{
							PrintError("Failed to compile project because passed build path '" + value.string() + "' shared library extension is not valid!");

							return false;
						}
						else if (compiler == "clang++"
								 || compiler == "gcc"
								 || compiler == "g++")
						{
#ifdef _WIN32
							if (value.extension() != ".dll")
							{
								PrintError("Failed to compile project because passed build path '" + value.string() + "' shared library extension is not valid!");

								return false;
							}
#else
							if (value.extension() != ".so")
							{
								PrintError("Failed to compile project because passed build path '" + value.string() + "' shared library extension is not valid!");

								return false;
							}
#endif
						}
					}
					else if (type == "link-only")
					{
						if ((compiler == "clang-cl"
							|| compiler == "cl")
							&& value.extension() != ".lib")
						{
							PrintError("Failed to compile project because passed build path '" + value.string() + "' static library extension is not valid!");

							return false;
						}
						else if (compiler == "clang++"
								 || compiler == "gcc"
								 || compiler == "g++")
						{
#ifdef _WIN32
							if (value.extension() != ".lib")
							{
								PrintError("Failed to compile project because passed build path '" + value.string() + "' static library extension is not valid!");

								return false;
							}
#else
							if (value.extension() != ".a")
							{
								PrintError("Failed to compile project because passed build path '" + value.string() + "' static library extension is not valid!");

								return false;
							}
#endif
						}
					}
					else if (type == "runtime-only")
					{
						if ((compiler == "clang-cl"
							|| compiler == "cl")
							&& value.extension() != ".dll")
						{
							PrintError("Failed to compile project because passed build path '" + value.string() + "' runtime library extension is not valid!");

							return false;
						}
						else if (compiler == "clang++"
								 || compiler == "gcc"
								 || compiler == "g++")
						{
#ifdef _WIN32
							if (value.extension() != ".dll")
							{
								PrintError("Failed to compile project because passed build path '" + value.string() + "' runtime library extension is not valid!");

								return false;
							}
#else
							if (value.extension() != ".so")
							{
								PrintError("Failed to compile project because passed build path '" + value.string() + "' runtime library extension is not valid!");

								return false;
							}
#endif
						}
					}
				}

				//room for future expansion
				else return true;
			}

			return true;
		};
	auto IsSupportedObjPath = [](
		string_view compiler,
		string_view standard,
		const path& value) -> bool
		{
			if (!exists(value))
			{
				//treat all extensionless values as dirs and return true
				if (!value.has_extension()) return true;

				PrintError("Failed to compile project because passed obj path '" + value.string() + "' does not exist!");

				return false;
			}

			if (is_directory(value)) return true;
			else
			{
				if (IsCPPStandard(standard)
					|| IsCStandard(standard))
				{
					if (value.extension() != ".obj"
						&& (compiler == "clang-cl"
							|| compiler == "cl"))
					{
						PrintError("Failed to compile project because passed obj path '" + value.string() + "' extension is not valid!");

						return false;
					}
					else if (compiler == "clang++"
							 || compiler == "gcc"
							 || compiler == "g++")
					{
#ifdef _WIN32
						if (value.extension() != ".obj")
						{
							PrintError("Failed to compile project because passed obj path '" + value.string() + "' extension is not valid!");

							return false;
						}
#else
						if (value.extension() != ".o")
						{
							PrintError("Failed to compile project because passed obj path '" + value.string() + "' extension is not valid!");

							return false;
						}
#endif
					}
				}

				//room for future expansion
				else return true;
			}

			return true;
		};
	auto IsSupportedHeaderExtension = [](string_view standard, const path& value) -> bool
		{
			if (!exists(value))
			{
				Log::Print(
					"Skipped invalid header file '" + value.string() + "' because it was not found.",
					"KALAMAKE",
					LogType::LOG_WARNING);

				return false;
			}

			//directories are allowed
			if (is_directory(value)) return true;

			if (IsCStandard(standard)
				&& value.extension() != ".h")
			{
				Log::Print(
					"Skipped invalid header file '" + value.string() + "' because it is not supported by the C standard.",
					"KALAMAKE",
					LogType::LOG_WARNING);

				return false;
			}

			if (IsCPPStandard(standard)
				&& value.extension() != ".hpp"
				&& value.extension() != ".h")
			{
				Log::Print(
					"Skipped invalid header file '" + value.string() + "' because it is not supported by the C++ standard.",
					"KALAMAKE",
					LogType::LOG_WARNING);

				return false;
			}

			return true;
		};

	auto IsSupportedWarningLevel = [](string_view value) -> bool
		{
			for (const auto& t : supportedWarningTypes)
			{
				if (value == t) return true;
			}

			PrintError("Failed to compile project because warning level '" + string(value) + "' is not supported!");

			return false;
		};
	auto IsSupportedCustomFlag = [](string_view value) -> bool
		{
			for (const auto& f : supportedCustomFlags)
			{
				if (value == f) return true;
			}

			PrintError("Failed to compile project because custom flag '" + string(value) + "' is not supported!");

			return false;
		};

	if (fileContent[0] != KMA_VERSION_NAME)
	{
		PrintError("Failed to compile project because kma version field value is malformed!");

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

		if (IsName(line))
		{
			if (!name.empty())
			{
				PrintError("Failed to compile project because more than one name line was passed!");

				return;
			}

			string value = RemoveFromString(line, "name: ");

			if (value.empty())
			{
				PrintError("Failed to compile project because no name value was passed!");

				return;
			}

			if (!IsSupportedName(value)) return;

			name = value;
		}
		else if (IsType(line))
		{
			if (!type.empty())
			{
				PrintError("Failed to compile project because more than one type line was passed!");

				return;
			}

			string value = RemoveFromString(line, "type: ");

			if (value.empty())
			{
				PrintError("Failed to compile project because no type value was passed!");

				return;
			}

			if (!IsSupportedType(value)) return;

			type = value;
		}
		else if (IsStandard(line))
		{
			if (!standard.empty())
			{
				PrintError("Failed to compile project because more than one standard line was passed!");

				return;
			}

			string value = RemoveFromString(line, "standard: ");

			if (value.empty())
			{
				PrintError("Failed to compile project because no standard value was passed!");

				return;
			}

			if (!IsSupportedStandard(value)) return;

			standard = value;
		}
		else if (IsCompiler(line))
		{
			if (!compiler.empty())
			{
				PrintError("Failed to compile project because more than one compiler line was passed!");

				return;
			}

			string value = RemoveFromString(line, "compiler: ");

			if (value.empty())
			{
				PrintError("Failed to compile project because no compiler value was passed!");

				return;
			}

			if (!IsSupportedCompiler(value)) return;

			if (value == "cl")
			{
				if (exists(cl_ide_bat_2026))        foundCLPath = cl_ide_bat_2026;
				else if (exists(cl_build_bat_2026)) foundCLPath = cl_build_bat_2026;
				else if (exists(cl_ide_bat_2022))   foundCLPath = cl_ide_bat_2022;
				else if (exists(cl_build_bat_2022)) foundCLPath = cl_build_bat_2022;
				else
				{
					PrintError("Failed to compile project because no 'vcvars64.bat' for cl compiler was found!");

					return;
				}
			}

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
				PrintError("Failed to compile project because there were no sources passed!");

				return;
			}

			vector<string> foundSources = SplitString(value, ", ");
			if (ContainsDuplicates(foundSources)) RemoveDuplicates(foundSources);

			bool failedSourceSearch{};

			for (auto& s : foundSources)
			{
				auto is_valid_source = [](string& s) -> bool
					{
						path sourcePath = exists(s)
							? s
							: (kmaPath / s);

						if (!exists(sourcePath))
						{
							PrintError(
								"Failed to compile project because the source '" + sourcePath.string() 
								+ "' was not found!");

							return false;
						}

						try
						{
							s = path(weakly_canonical(sourcePath)).string();
						}
						catch (const filesystem_error&)
						{
							PrintError(
								"Failed to compile project because the source path '" + sourcePath.string() 
								+ "' could not be resolved!");

							return false;
						}

						return true;
					};

				//always allow directories
				if (!path(s).has_extension())
				{
					path sourcePath = exists(s)
						? s
						: (kmaPath / s);

					try
					{
						s = path(weakly_canonical(sourcePath)).string();
					}
					catch (const filesystem_error&)
					{
						PrintError(
							"Failed to compile project because the source path '" + sourcePath.string()
							+ "' could not be resolved!");

						failedSourceSearch = true;

						break;
					}

					continue;
				}
				//check files with extensions
				if (!is_valid_source(s))
				{
					failedSourceSearch = true;

					break;
				}
			}

			if (failedSourceSearch) return;

			//move found sources to sources container
			sources = std::move(foundSources);
		}

		else if (IsBuildPath(line))
		{
			if (!buildPath.empty())
			{
				PrintError("Failed to compile project because more than one build path line was passed!");

				return;
			}

			string value = RemoveFromString(line, "buildpath: ");

			//ignore empty build path field
			if (value.empty()) continue;

			//assume path is relative if it doesn't already exist
			if (!exists(value))
			{
				if (path(value).has_extension()) value = path(kmaPath / buildPath).string();
				else                             value = path(kmaPath / value).string();
			}

			try
			{
				value = path(weakly_canonical(value)).string();
			}
			catch (const filesystem_error&)
			{
				PrintError("Failed to compile project because build path '" + value + "' could not be resolved!");
			}

			buildPath = value;
		}
		else if (IsObjPath(line))
		{
			if (!objPath.empty())
			{
				PrintError("Failed to compile project because more than one obj path line was passed!");

				return;
			}

			string value = RemoveFromString(line, "objpath: ");

			//ignore empty obj path field
			if (value.empty()) continue;

			//assume path is relative if it doesn't already exist
			if (!exists(value))
			{
				if (path(value).has_extension()) value = path(kmaPath / buildPath).string();
				else                             value = path(kmaPath / value).string();
			}

			try
			{
				value = path(weakly_canonical(value)).string();
			}
			catch (const filesystem_error&)
			{
				PrintError("Failed to compile project because obj path '" + value + "' could not be resolved!");
			}

			objPath = value;
		}
		else if (IsHeader(line))
		{
			if (!headers.empty())
			{
				PrintError("Failed to compile project because more than one headers line was passed!");

				return;
			}

			string value = RemoveFromString(line, "headers: ");

			//ignore empty headers field
			if (value.empty()) continue;

			vector<string> foundHeaders = SplitString(value, ", ");
			if (ContainsDuplicates(foundHeaders)) RemoveDuplicates(foundHeaders);

			bool failedHeaderSearch{};

			for (auto& h : foundHeaders)
			{
				auto is_valid_header = [](string& h) -> bool
					{
						path headerPath = exists(h)
							? h
							: (kmaPath / h);

						if (!exists(headerPath))
						{
							PrintError(
								"Failed to compile project because the header '" + headerPath.string()
								+ "' was not found!");

							return false;
						}

						try
						{
							h = path(weakly_canonical(headerPath)).string();
						}
						catch (const filesystem_error&)
						{
							PrintError(
								"Failed to compile project because the header path '" + headerPath.string()
								+ "' could not be resolved!");

							return false;
						}

						return true;
					};

				//always allow directories
				if (!path(h).has_extension())
				{
					path headerPath = exists(h)
						? h
						: (kmaPath / h);

					try
					{
						h = path(weakly_canonical(headerPath)).string();
					}
					catch (const filesystem_error&)
					{
						PrintError(
							"Failed to compile project because the header path '" + headerPath.string()
							+ "' could not be resolved!");

						failedHeaderSearch = true;

						break;
					}

					continue;
				}
				//check files with extensions
				if (!is_valid_header(h))
				{
					failedHeaderSearch = true;

					break;
				}
			}

			if (failedHeaderSearch) return;

			//move found sources to headers container
			headers = std::move(foundHeaders);
		}
		else if (IsRelLink(line))
		{
			if (!rellinks.empty())
			{
				PrintError("Failed to compile project because more than one links line was passed!");

				return;
			}

			string value = RemoveFromString(line, "rellinks: ");

			//ignore empty links field
			if (value.empty()) continue;

			vector<string> foundLinks = SplitString(value, ", ");
			if (ContainsDuplicates(foundLinks)) RemoveDuplicates(foundLinks);

			bool failedLinkSearch{};

			//resolve all relative link files with extensions to project kma file path
			for (auto& l : foundLinks)
			{
				if (ContainsString("/", l)
					|| ContainsString("\\", l))
				{
					l = exists(path(l))
						? l
						: path(kmaPath / l).string();
				}

				if (exists(l))
				{
					try
					{
						l = path(weakly_canonical(l)).string();
					}
					catch (const filesystem_error&)
					{
						PrintError("Failed to compile project because reklink path '" + l + "' could not be resolved!");

						failedLinkSearch = true;

						return;
					}
				}
			}

			if (failedLinkSearch) return;

			rellinks = std::move(foundLinks);
		}
		else if (IsDebLink(line))
		{
			if (!deblinks.empty())
			{
				PrintError("Failed to compile project because more than one deblinks line was passed!");

				return;
			}

			string value = RemoveFromString(line, "deblinks: ");

			//ignore empty links field
			if (value.empty()) continue;

			vector<string> foundLinks = SplitString(value, ", ");
			if (ContainsDuplicates(foundLinks)) RemoveDuplicates(foundLinks);

			bool failedLinkSearch{};

			//resolve all relative link files with extensions to project kma file path
			for (auto& l : foundLinks)
			{
				if (ContainsString("/", l)
					|| ContainsString("\\", l))
				{
					l = exists(path(l))
						? l
						: path(kmaPath / l).string();
				}

				if (exists(l))
				{
					try
					{
						l = path(weakly_canonical(l)).string();
					}
					catch (const filesystem_error&)
					{
						PrintError("Failed to compile project because deblink path '" + l + "' could not be resolved!");

						failedLinkSearch = true;

						return;
					}
				}
			}

			if (failedLinkSearch) return;

			deblinks = std::move(foundLinks);
		}

		else if (IsWarningLevel(line))
		{
			if (!warningLevel.empty())
			{
				PrintError("Failed to compile project because more than one warning level line was passed!");

				return;
			}

			string value = RemoveFromString(line, "warninglevel: ");

			//ignore empty warning level field
			if (value.empty()) continue;

			if (!IsSupportedWarningLevel(value)) return;

			warningLevel = value;
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

			defines = std::move(tempDefines);
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

			extensions = std::move(tempExtensions);
		}

		else if (IsRelFlag(line))
		{
			if (!relflags.empty())
			{
				PrintError("Failed to compile project because more than one relflags line was passed!");

				return;
			}

			string value = RemoveFromString(line, "relflags: ");

			//ignore empty flags field
			if (value.empty()) continue;

			vector<string> tempFlags = SplitString(value, ", ");

			relflags = std::move(tempFlags);
		}
		else if (IsDebFlag(line))
		{
			if (!debflags.empty())
			{
				PrintError("Failed to compile project because more than one debflags line was passed!");

				return;
			}

			string value = RemoveFromString(line, "debflags: ");

			//ignore empty flags field
			if (value.empty()) continue;

			vector<string> tempFlags = SplitString(value, ", ");

			debflags = std::move(tempFlags);
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
				if (!IsSupportedCustomFlag(f))
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
	// VERIFY SOURCES, HEADERS AND LINKS
	//

	vector<string> cleanedSources{};
	vector<string> cleanedHeaders{};
	vector<string> cleanedRelLinks{};
	vector<string> cleanedDebLinks{};

	for (const auto& s : sources)
	{
		//scan source folder
		if (is_directory(s))
		{
			for (const auto& d : recursive_directory_iterator(s))
			{
				if (is_directory(d)) continue;

				path p = path(d);
				if (IsSupportedSourceExtension(standard, p)) cleanedSources.push_back(p.string());
			}
		}
		else
		{
			path p = path(s);
			if (!IsSupportedSourceExtension(standard, p)) continue;

			cleanedSources.push_back(s);
		}
	}
	if (!headers.empty())
	{
		for (const auto& h : headers)
		{
			path p = path(h);
			if (!IsSupportedHeaderExtension(standard, p)) continue;

			cleanedHeaders.push_back(h);
		}
	}
	if (!rellinks.empty())
	{
		for (const auto& l : rellinks)
		{
			if (!EndsWith(l, ".lib")
				&& !EndsWith(l, ".a"))
			{
				Log::Print(
					"Skipped invalid link file '" + l + "' because it has no extension.",
					"KALAMAKE",
					LogType::LOG_WARNING);

				continue;
			}

			if ((ContainsString("/", l)
				|| ContainsString("\\", l))
				&& !exists(l))
			{
				Log::Print(
					"Skipped invalid link file '" + l + "' because it does not exist.",
					"KALAMAKE",
					LogType::LOG_WARNING);

				continue;
			}

			cleanedRelLinks.push_back(l);
		}
	}
	if (!deblinks.empty())
	{
		for (const auto& l : deblinks)
		{
			if (!EndsWith(l, ".lib")
				&& !EndsWith(l, ".a"))
			{
				Log::Print(
					"Skipped invalid link file '" + l + "' because it has no extension.",
					"KALAMAKE",
					LogType::LOG_WARNING);

				continue;
			}

			if ((ContainsString("/", l)
				|| ContainsString("\\", l))
				&& !exists(l))
			{
				Log::Print(
					"Skipped invalid link file '" + l + "' because it does not exist.",
					"KALAMAKE",
					LogType::LOG_WARNING);

				continue;
			}

			cleanedDebLinks.push_back(l);
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
	if (cleanedSources.empty())
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

	//
	// CLEAN UP VALUES
	//

	if (warningLevel.empty()) warningLevel = "strong";
	if (buildPath.empty()) buildPath = path(kmaPath / defaultBuildPath).string();
	if (objPath.empty()) objPath = path(kmaPath / defaultObjPath).string();

	if (ContainsDuplicates(cleanedSources))  RemoveDuplicates(cleanedSources);
	if (ContainsDuplicates(cleanedHeaders))  RemoveDuplicates(cleanedHeaders);
	if (ContainsDuplicates(cleanedRelLinks)) RemoveDuplicates(cleanedDebLinks);
	if (ContainsDuplicates(defines))         RemoveDuplicates(defines);
	if (ContainsDuplicates(extensions))      RemoveDuplicates(extensions);
	if (ContainsDuplicates(relflags))        RemoveDuplicates(relflags);
	if (ContainsDuplicates(debflags))        RemoveDuplicates(debflags);
	if (ContainsDuplicates(customFlags))     RemoveDuplicates(customFlags);

	for (auto& f : relflags)
	{
		if (compiler == "clang-cl"
			|| compiler == "cl")
		{
			if (!f.starts_with('/')) f = "/" + f;
		}
		else if (compiler == "clang++"
			|| compiler == "gcc"
			|| compiler == "g++")
		{
			if (!f.starts_with('-')) f = "-" + f;
		}
	}
	for (auto& f : debflags)
	{
		if (compiler == "clang-cl"
			|| compiler == "cl")
		{
			if (!f.starts_with('/')) f = "/" + f;
		}
		else if (compiler == "clang++"
			|| compiler == "gcc"
			|| compiler == "g++")
		{
			if (!f.starts_with('-')) f = "-" + f;
		}
	}

	//
	// VERIFY BUILD AND OBJ PATHS
	//

	if (!IsSupportedBuildPath(
		compiler,
		type,
		standard,
		buildPath))
	{
		return;
	}

	if (!IsSupportedObjPath(
		compiler,
		standard,
		objPath))
	{
		return;
	}

	//
	// PRINT VALUES
	//

	ostringstream oss{};

	oss << "Parsed data passed to KalaMake:\n\n"

		<< "name: " << name << "\n"
		<< "type: " << type << "\n"
		<< "standard: " << standard << "\n"
		<< "compiler: " << compiler << "\n"
		<< "sources:\n";

	for (const auto& s : cleanedSources)
	{
		oss << "  " << s << "\n";
	}

	oss << "build path: " << buildPath << "\n";
	oss << "obj path: " << objPath << "\n";
	if (!headers.empty())
	{
		oss << "headers:\n";

		for (const auto& h : cleanedHeaders)
		{
			oss << "  " << h << "\n";
		}
	}
	if (!cleanedRelLinks.empty())
	{
		oss << "release links:\n";

		for (const auto& l : cleanedRelLinks)
		{
			oss << "  " << l << "\n";
		}
	}
	if (!cleanedDebLinks.empty())
	{
		oss << "debug links:\n";

		for (const auto& l : cleanedDebLinks)
		{
			oss << "  " << l << "\n";
		}
	}

	oss << "warning level: " << warningLevel << "\n";
	if (!defines.empty())
	{
		oss << "defines:\n";

		for (const auto& e : defines)
		{
			oss << "  " << e << "\n";
		}
	}
	if (!extensions.empty())
	{
		oss << "extensions:\n";

		for (const auto& e : extensions)
		{
			oss << "  " << e << "\n";
		}
	}

	if (!relflags.empty())
	{
		oss << "release flags:\n";

		for (const auto& f : relflags)
		{
			oss << "  " << f << "\n";
		}
	}
	if (!debflags.empty())
	{
		oss << "debug flags:\n";

		for (const auto& f : debflags)
		{
			oss << "  " << f << "\n";
		}
	}
	if (!customFlags.empty())
	{
		oss << "custom flags:\n";

		for (const auto& cf : customFlags)
		{
			oss << "  " << cf << "\n";
		}
	}

	Log::Print(
		oss.str(),
		"KALAMAKE",
		LogType::LOG_INFO);

	Log::Print("==========================================================================================\n");

	CompileProject(
		{
			.name = name,
			.type = type,
			.standard = standard,
			.compiler = compiler,
			.sources = std::move(cleanedSources),

			.buildPath = buildPath,
			.objPath = objPath,
			.headers = std::move(cleanedHeaders),
			.rellinks = std::move(cleanedRelLinks),
			.deblinks = std::move(cleanedDebLinks),

			.warningLevel = warningLevel,
			.defines = std::move(defines),
			.extensions = std::move(extensions),

			.relflags = std::move(relflags),
			.debflags = std::move(debflags),
			.customFlags = std::move(customFlags),
		});
}

void CompileProject(const CompileData& compileData)
{
	if (compileData.type == "link-only")
	{
		//continue to static lib compilation function
		//since its very different from exe and shared lib
		CompileStaticLib(compileData);

		return;
	}

	vector<string> finalRelFlags = std::move(compileData.relflags);
	vector<string> finalDebFlags = std::move(compileData.debflags);

	//
	// SOURCES
	//

	if (compileData.compiler == "clang-cl"
		|| compileData.compiler == "cl"
		|| compileData.compiler == "clang++"
		|| compileData.compiler == "gcc"
		|| compileData.compiler == "g++")
	{
		for (const auto& s : compileData.sources)
		{
			finalRelFlags.push_back("\"" + s + "\"");
			finalDebFlags.push_back("\"" + s + "\"");
		}
	}

	//
	// HEADERS
	//

	if (!compileData.headers.empty())
	{
		if (compileData.compiler == "clang-cl"
			|| compileData.compiler == "cl")
		{
			for (const auto& s : compileData.headers)
			{
				finalRelFlags.push_back("/I\"" + s + "\"");
				finalDebFlags.push_back("/I\"" + s + "\"");
			}
		}
		else if (compileData.compiler == "clang++"
				 || compileData.compiler == "gcc"
				 || compileData.compiler == "g++")
		{
			for (const auto& s : compileData.headers)
			{
				finalRelFlags.push_back("-I\"" + s + "\"");
				finalDebFlags.push_back("-I\"" + s + "\"");
			}
		}
	}

	//
	// LINKS
	//

	if (!compileData.rellinks.empty())
	{
		if (compileData.compiler == "clang-cl"
			|| compileData.compiler == "cl")
		{
			for (const auto& l : compileData.rellinks)
			{
				if (EndsWith(l, ".lib")) finalRelFlags.push_back("\"" + l + "\"");
				else                     finalRelFlags.push_back(l);                   
			}
		}
		else if (compileData.compiler == "clang++"
				 || compileData.compiler == "gcc"
				 || compileData.compiler == "g++")
		{
			for (const auto& l : compileData.rellinks)
			{
				if (EndsWith(l, ".lib")
					|| EndsWith(l, ".a"))
				{
					finalRelFlags.push_back("\"" + l + "\"");
				}
				else finalRelFlags.push_back("-l" + l);
			}
		}
	}
	if (!compileData.deblinks.empty())
	{
		if (compileData.compiler == "clang-cl"
			|| compileData.compiler == "cl")
		{
			for (const auto& l : compileData.deblinks)
			{
				if (EndsWith(l, ".lib")) finalDebFlags.push_back("\"" + l + "\"");
				else                     finalDebFlags.push_back(l);
			}
		}
		else if (compileData.compiler == "clang++"
			|| compileData.compiler == "gcc"
			|| compileData.compiler == "g++")
		{
			for (const auto& l : compileData.deblinks)
			{
				if (EndsWith(l, ".lib")
					|| EndsWith(l, ".a"))
				{
					finalDebFlags.push_back("\"" + l + "\"");
				}
				else finalDebFlags.push_back("-l" + l);
			}
		}
	}

	//
	// STANDARD
	//

	if (compileData.compiler == "clang-cl"
		|| compileData.compiler == "cl")
	{
		finalRelFlags.push_back("/std:" + compileData.standard);
		finalDebFlags.push_back("/std:" + compileData.standard);
	}
	else if (compileData.compiler == "clang++"
			 || compileData.compiler == "gcc"
			 || compileData.compiler == "g++")
	{
		finalRelFlags.push_back("-std=" + compileData.standard);
		finalDebFlags.push_back("-std=" + compileData.standard);
	}

	//
	// EXTENSIONS
	//

	//...

	//
	// WARNING LEVEL
	//

	if (compileData.compiler == "clang-cl"
		|| compileData.compiler == "cl")
	{
		if (compileData.warningLevel == "none")
		{
			finalRelFlags.push_back("/W0");
			finalDebFlags.push_back("/W0");
		}
		else if (compileData.warningLevel == "basic")
		{
			finalRelFlags.push_back("/W1");
			finalDebFlags.push_back("/W1");
		}
		else if (compileData.warningLevel == "normal")
		{
			finalRelFlags.push_back("/W2");
			finalDebFlags.push_back("/W2");
		}
		else if (compileData.warningLevel == "strong")
		{
			finalRelFlags.push_back("/W3");
			finalDebFlags.push_back("/W3");
		}
		else if (compileData.warningLevel == "strict")
		{
			finalRelFlags.push_back("/W4");
			finalDebFlags.push_back("/W4");
		}
		else if (compileData.warningLevel == "all")
		{
			finalRelFlags.push_back("/Wall");
			finalDebFlags.push_back("/Wall");
		}
	}
	else if (compileData.compiler == "clang++"
			 || compileData.compiler == "gcc"
			 || compileData.compiler == "g++")
	{
		if (compileData.warningLevel == "none")
		{
			finalRelFlags.push_back("-w");
			finalDebFlags.push_back("-w");
		}
		else if (compileData.warningLevel == "basic")
		{
			finalRelFlags.push_back("-Wall");
			finalDebFlags.push_back("-Wall");
		}
		else if (compileData.warningLevel == "normal")
		{
			finalRelFlags.push_back("-Wall");
			finalRelFlags.push_back("-Wextra");

			finalDebFlags.push_back("-Wall");
			finalDebFlags.push_back("-Wextra");
		}
		else if (compileData.warningLevel == "strong")
		{
			finalRelFlags.push_back("-Wall");
			finalRelFlags.push_back("-Wextra");
			finalRelFlags.push_back("-Wpedantic");

			finalDebFlags.push_back("-Wall");
			finalDebFlags.push_back("-Wextra");
			finalDebFlags.push_back("-Wpedantic");
		}
		else if (compileData.warningLevel == "strict")
		{
			finalRelFlags.push_back("-Wall");
			finalRelFlags.push_back("-Wextra");
			finalRelFlags.push_back("-Wpedantic");
			finalRelFlags.push_back("-Wshadow");
			finalRelFlags.push_back("-Wconversion");
			finalRelFlags.push_back("-Wsign-conversion");

			finalDebFlags.push_back("-Wall");
			finalDebFlags.push_back("-Wextra");
			finalDebFlags.push_back("-Wpedantic");
			finalDebFlags.push_back("-Wshadow");
			finalDebFlags.push_back("-Wconversion");
			finalDebFlags.push_back("-Wsign-conversion");
		}
		else if (compileData.warningLevel == "all")
		{
			if (compileData.compiler == "clang++")
			{
				finalRelFlags.push_back("-Wall");
				finalRelFlags.push_back("-Wextra");
				finalRelFlags.push_back("-Wpedantic");
				finalRelFlags.push_back("-Weverything");

				finalDebFlags.push_back("-Wall");
				finalDebFlags.push_back("-Wextra");
				finalDebFlags.push_back("-Wpedantic");
				finalDebFlags.push_back("-Weverything");
			}
			else
			{
				finalRelFlags.push_back("-Wall");
				finalRelFlags.push_back("-Wextra");
				finalRelFlags.push_back("-Wpedantic");
				finalRelFlags.push_back("-Wshadow");
				finalRelFlags.push_back("-Wconversion");
				finalRelFlags.push_back("-Wsign-conversion");
				finalRelFlags.push_back("-Wcast-align");
				finalRelFlags.push_back("-Wnull-dereference");
				finalRelFlags.push_back("-Wdouble-promotion");
				finalRelFlags.push_back("-Wformat=2");

				finalDebFlags.push_back("-Wall");
				finalDebFlags.push_back("-Wextra");
				finalDebFlags.push_back("-Wpedantic");
				finalDebFlags.push_back("-Wshadow");
				finalDebFlags.push_back("-Wconversion");
				finalDebFlags.push_back("-Wsign-conversion");
				finalDebFlags.push_back("-Wcast-align");
				finalDebFlags.push_back("-Wnull-dereference");
				finalDebFlags.push_back("-Wdouble-promotion");
				finalDebFlags.push_back("-Wformat=2");
			}
		}
	}

	//
	// DEFINES
	//

	if (!compileData.defines.empty())
	{
		if (compileData.compiler == "clang-cl"
			|| compileData.compiler == "cl"
			|| compileData.compiler == "clang++"
			|| compileData.compiler == "gcc"
			|| compileData.compiler == "g++")
		{
			for (const auto& d : compileData.defines)
			{
				finalRelFlags.push_back("-D" + d);
				finalDebFlags.push_back("-D" + d);
			}
		}
	}

	//
	// CUSTOM FLAGS
	//

	for (const auto& f : compileData.customFlags)
	{
		if (f == "standard-required"
			&& compileData.compiler == "cl")
		{
			finalRelFlags.push_back("/permissive-");
			finalDebFlags.push_back("/permissive-");

			continue;
		}

		if (f == "warnings-as-errors")
		{
			if (compileData.compiler == "clang-cl"
				|| compileData.compiler == "cl")
			{
				finalRelFlags.push_back("/WX");
				finalDebFlags.push_back("/WX");
			}
			else if (compileData.compiler == "clang++"
					 || compileData.compiler == "gcc"
					 || compileData.compiler == "g++")
			{
				finalRelFlags.push_back("-Werror");
				finalDebFlags.push_back("-Werror");
			}

			continue;
		}
	}

	//
	// BUILD TYPES
	//

	vector<string> buildFlags{};

	for (const auto& f : compileData.customFlags)
	{
		if (f == "debug")      buildFlags.push_back("debug");
		if (f == "release")    buildFlags.push_back("release");
		if (f == "reldebug")   buildFlags.push_back("reldebug");
		if (f == "minsizerel") buildFlags.push_back("minsizerel");
	}

	//enable standard C++ extensions for MSVC
	if (IsCPPStandard(compileData.standard)
		&& (compileData.compiler == "cl"
		|| compileData.compiler == "clang-cl"))
	{
		finalRelFlags.push_back("/EHsc");
		finalDebFlags.push_back("/EHsc");
	}

	//
	// BUILD PROJECT
	//

	auto get_build_path = [](
		string_view name,
		string_view compiler,
		string_view type,
		const path& buildPath) -> string
		{
			if (type == "executable")
			{
				if (compiler == "clang-cl"
					|| compiler == "cl")
				{
					path newBuildPath = buildPath / string(string(name) + ".exe");
					path parentDir = newBuildPath.parent_path();

					if (!exists(parentDir))
					{
						string result = CreateNewDirectory(parentDir);
						if (!result.empty()) return "Reason: " + result;
					}

					return "/Fe:\"" + newBuildPath.string() + "\"";
				}
				else if (compiler == "clang++"
						 || compiler == "gcc"
						 || compiler == "g++")
				{
#ifdef _WIN32
					path newBuildPath = buildPath / string(string(name) + ".exe");
#else
					path newBuildPath = buildPath / name;
#endif
					path parentDir = newBuildPath.parent_path();

					if (!exists(parentDir))
					{
						string result = CreateNewDirectory(parentDir);
						if (!result.empty()) return "Reason: " + result;
					}

					return "-o \"" + newBuildPath.string() + "\"";
				}
			}
			else if (type == "link-runtime")
			{
				if (compiler == "clang-cl"
					|| compiler == "cl")
				{
					path newBuildPath = buildPath / string(string(name) + ".dll");
					path libPath = buildPath / string(string(name) + ".lib");

					path parentDir = newBuildPath.parent_path();

					if (!exists(parentDir))
					{
						string result = CreateNewDirectory(parentDir);
						if (!result.empty()) return "Reason: " + result;
					}

					return
						"/LD /link "
						"/OUT:\"" + newBuildPath.string() + "\" "
						"/IMPLIB:\"" + libPath.string() + "\"";
				}
				else if (compiler == "clang++"
						 || compiler == "gcc"
						 || compiler == "g++")
				{
#ifdef _WIN32
					path newBuildPath = buildPath / string(string(name) + ".dll");
					path libPath = buildPath / string("lib" + string(name) + ".dll.a");

					path parentDir = newBuildPath.parent_path();

					if (!exists(parentDir))
					{
						string result = CreateNewDirectory(parentDir);
						if (!result.empty()) return "Reason: " + result;
					}

					return
						"-shared "
						"-o \"" + newBuildPath.string() + "\" "
						"-Wl,--out-implib,\"" + libPath.string() + "\"";
#else
					path newBuildPath = buildPath / string(string(name) + ".so");
					path parentDir = newBuildPath.parent_path();

					if (!exists(parentDir))
					{
						string result = CreateNewDirectory(parentDir);
						if (!result.empty()) return "Reason: " + result;
					}

					return "-fPIC -shared "
						"-o \"" + newBuildPath.string() + "\"";
#endif
				}
			}
			else if (type == "runtime-only")
			{
				if (compiler == "clang-cl"
					|| compiler == "cl")
				{
					path newBuildPath = buildPath / string(string(name) + ".dll");
					path parentDir = newBuildPath.parent_path();

					if (!exists(parentDir))
					{
						string result = CreateNewDirectory(parentDir);
						if (!result.empty()) return "Reason: " + result;
					}

					return
						"/LD /link "
						"/OUT:\"" + newBuildPath.string() + "\"";
				}
				else if (compiler == "clang++"
						 || compiler == "gcc"
						 || compiler == "g++")
				{
#ifdef _WIN32
					path newBuildPath = buildPath / string(string(name) + ".dll");
					path parentDir = newBuildPath.parent_path();

					if (!exists(parentDir))
					{
						string result = CreateNewDirectory(parentDir);
						if (!result.empty()) return "Reason: " + result;
					}

					return
						"-shared "
						"-o \"" + newBuildPath.string() + "\"";
#else
					path newBuildPath = buildPath / string(string(name) + ".so");
					path parentDir = newBuildPath.parent_path();

					if (!exists(parentDir))
					{
						string result = CreateNewDirectory(parentDir);
						if (!result.empty()) return "Reason: " + result;
					}

					return "-fPIC -shared "
						"-o \"" + newBuildPath.string() + "\"";
#endif
				}
			}

			return "Reason: Invalid target type.";
		};

	auto get_build_type = [](
		string_view flagsType,
		string_view compiler,
		const vector<string>& flags) -> string
		{
			if (compiler == "clang-cl"
				|| compiler == "cl")
			{
				if (flagsType == "debug"
					&& ContainsString(flags, "/Od")
					&& ContainsString(flags, "/Zi"))
				{
					return "debug";
				}

				if (flagsType == "release")
				{
					if (ContainsString(flags, "/O2")
						&& ContainsString(flags, "/Zi"))
					{
						return "reldebug";
					}
					else if (ContainsString(flags, "/O2"))
					{
						return "release";
					}
					else if (ContainsString(flags, "/O1"))
					{
						return "minsizerel";
					}
				}

				if (flagsType == "release")
				{
					Log::Print(
						"Failed to find valid combination of release build flags, assuming 'release' as default.",
						"KALAMAKE",
						LogType::LOG_WARNING);

					return "release";
				}
				else
				{
					Log::Print(
						"Failed to find valid combination of debug build flags, skipping build in debug.",
						"KALAMAKE",
						LogType::LOG_WARNING);

					return "none";
				}
			}
			else if (compiler == "clang++"
					 || compiler == "gcc"
					 || compiler == "g++")
			{
				if (flagsType == "debug"
					&& ContainsString(flags, "-O0")
					&& ContainsString(flags, "-g"))
				{
					return "debug";
				}

				if (flagsType == "release")
				{
					if (ContainsString(flags, "-O2")
						&& ContainsString(flags, "-g"))
					{
						return "reldebug";
					}
					else if (ContainsString(flags, "-O2"))
					{
						return "release";
					}
					else if (ContainsString(flags, "-Os"))
					{
						return "minsizerel";
					}
				}

				if (flagsType == "release")
				{
					Log::Print(
						"Failed to find valid combination of release build flags, assuming 'release' as default.",
						"KALAMAKE",
						LogType::LOG_WARNING);

					return "release";
				}
				else
				{
					Log::Print(
						"Failed to find valid combination of debug build flags, skipping build in debug.",
						"KALAMAKE",
						LogType::LOG_WARNING);

					return "none";
				}
			}

			if (flagsType == "release")
			{
				Log::Print(
					"Failed to find valid combination of release build flags, assuming 'release' as default.",
					"KALAMAKE",
					LogType::LOG_WARNING);

				return "release";
			}
			else
			{
				Log::Print(
					"Failed to find valid combination of debug build flags, skipping build in debug.",
					"KALAMAKE",
					LogType::LOG_WARNING);

				return "none";
			}
		};

	auto build_project = [&compileData](
		string_view compiler,
		const path& buildPath,
		vector<string> finalFlags) -> bool
		{
			finalFlags.push_back(buildPath.string());

			if (ContainsDuplicates(finalFlags)) RemoveDuplicates(finalFlags);

			ostringstream oss{};

			for (int i = 0; i < finalFlags.size(); i++)
			{
				if (i > 0) oss << ' ';
				oss << finalFlags[i];
			}

			Log::Print(
				"Flags passed to compiler " + string(compiler) + ":\n\n" + oss.str(),
				"KALAMAKE",
				LogType::LOG_INFO);

			Log::Print("\n==========================================================================================\n");

			string finalValue{};
			if (compiler == "cl")
			{
				Log::Print(
					"Found valid vcvars64.bat from '" + foundCLPath.string() + "'.",
					"KALAMAKE",
					LogType::LOG_INFO);

				finalValue =
					"cmd /c \"\""
					+ foundCLPath.string()
					+ "\" && cl "
					+ oss.str() + "\"";
			}
			else if (compiler == "clang-cl") finalValue = "clang-cl " + oss.str();
			else if (compiler == "clang++") finalValue = "clang++ " + oss.str();
			else if (compiler == "gcc")       finalValue = "gcc " + oss.str();
			else if (compiler == "g++")       finalValue = "g++ " + oss.str();

			if (system(finalValue.c_str()) != 0)
			{
				PrintError("Compilation failed!");

				Log::Print("\n==========================================================================================\n");

				return false;
			}
			else
			{
				Log::Print(
					"Compilation succeeded!",
					"KALAMAKE",
					LogType::LOG_SUCCESS);
			}

			Log::Print("\n==========================================================================================\n");

			return true;
		};

	if (!buildFlags.empty())
	{
		for (const auto& b : buildFlags)
		{
			//adds build flag as build folder between build path and binary name
			string finalBuildPath = get_build_path(
				compileData.name,
				compileData.compiler,
				compileData.type,
				path(compileData.buildPath) / b);

			if (ContainsString(finalBuildPath, "Result: "))
			{
				PrintError("Failed to create new directory for compiled file target path missing parent path! " + finalBuildPath);
				return;
			}

			if (b != "debug")
			{
				if (compileData.compiler == "clang-cl"
					|| compileData.compiler == "cl")
				{
					if (b == "release")
					{
						if (!ContainsString(finalRelFlags, "/O2")) finalRelFlags.push_back("/O2");
					}
					else if (b == "reldebug")
					{
						if (!ContainsString(finalRelFlags, "/O2")) finalRelFlags.push_back("/O2");
						if (!ContainsString(finalRelFlags, "/Zi")) finalRelFlags.push_back("/Zi");
					}
					else if (b == "minsizerel")
					{
						if (!ContainsString(finalRelFlags, "/O1")) finalRelFlags.push_back("/O1");
					}

					if (!ContainsString(finalRelFlags, "/MD")) finalRelFlags.push_back("/MD");
				}
				else if (compileData.compiler == "clang++"
						 || compileData.compiler == "gcc"
						 || compileData.compiler == "g++")
				{
					if (b == "release")
					{
						if (!ContainsString(finalRelFlags, "-O2")) finalRelFlags.push_back("-O2");
					}
					else if (b == "reldebug")
					{
						if (!ContainsString(finalRelFlags, "-O2")) finalRelFlags.push_back("-O2");
						if (!ContainsString(finalRelFlags, "-g")) finalRelFlags.push_back("-g");
					}
					else if (b == "minsizerel")
					{
						if (!ContainsString(finalRelFlags, "-Os")) finalRelFlags.push_back("-Os");
					}
				}

				if (!build_project(
					compileData.compiler,
					finalBuildPath,
					finalRelFlags))
				{
					return;
				}
			}
			else
			{
				if (compileData.compiler == "clang-cl"
					|| compileData.compiler == "cl")
				{
					if (!ContainsString(finalDebFlags, "/Od")) finalDebFlags.push_back("/Od");
					if (!ContainsString(finalDebFlags, "/Zi")) finalDebFlags.push_back("/Zi");
					if (!ContainsString(finalDebFlags, "/MDd")) finalDebFlags.push_back("/MDd");
				}
				else if (compileData.compiler == "clang++"
						 || compileData.compiler == "gcc"
						 || compileData.compiler == "g++")
				{
					if (!ContainsString(finalDebFlags, "-O0")) finalDebFlags.push_back("-O0");
					if (!ContainsString(finalDebFlags, "-g")) finalDebFlags.push_back("-g");
				}

				if (!build_project(
					compileData.compiler,
					finalBuildPath,
					finalDebFlags))
				{
					return;
				}
			}
		}
	}
	else
	{
		string relbuildType = get_build_type(
			"release",
			compileData.compiler,
			compileData.relflags);

		//build in any release mode
		{
			//adds build flag as build folder between build path and binary name
			string finalBuildPath = get_build_path(
				compileData.name,
				compileData.compiler,
				compileData.type,
				path(compileData.buildPath) / relbuildType);

			if (ContainsString(finalBuildPath, "Result: "))
			{
				PrintError("Failed to create new directory for compiled file target path missing parent path! " + finalBuildPath);
				return;
			}

			if (!build_project(
				compileData.compiler,
				finalBuildPath,
				finalRelFlags))
			{
				return;
			}
		}

		if (!compileData.debflags.empty())
		{
			string debbuildType = get_build_type(
				"debug",
				compileData.compiler,
				compileData.debflags);

			if (debbuildType != "none")
			{
				//adds build flag as build folder between build path and binary name
				string finalBuildPath = get_build_path(
					compileData.name,
					compileData.compiler,
					compileData.type,
					path(compileData.buildPath) / debbuildType);

				if (ContainsString(finalBuildPath, "Result: "))
				{
					PrintError("Failed to create new directory for compiled file target path missing parent path! " + finalBuildPath);
					return;
				}

				if (!build_project(
					compileData.compiler,
					finalBuildPath,
					finalDebFlags))
				{
					return;
				}
			}
		}
	}
}

void CompileStaticLib(const CompileData& compileData)
{

}