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
	string compiler{};
	vector<string> sources{};
	vector<string> headers{};
	vector<string> links{};
	string warningLevel{};
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

constexpr string_view cl_ide_bat_2026 = "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat";
constexpr string_view cl_build_bat_2026 = "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat";
constexpr string_view cl_ide_bat_2022 = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat";
constexpr string_view cl_build_bat_2022 = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat";

static path foundCLPath{};

static const array<string_view, 14> actionTypes =
{
	"compiler",     //what is used to compile this source code
	"sources",      //what source files are compiled
	"type",         //what is the target type of the final file
	"name",         //what is the name of the final file

	"standard",     //what is the language standard

	//optional fields

	"warninglevel", //what warning level should the compiler use, defaults to strong if unset

	"links",        //what libraries will be linked to the binary

	"headers",      //what header files are included (for C and C++)

	"defines",      //what compile-time defines will be linked to the binary
	"buildpath",    //where the binary will be built to
	"objpath",      //where the object files live at during compilation

	"extensions",   //what language standard extensions will be used
	"flags",        //what flags will be passed to the compiler
	"customflags"   //what KalaMake-specific flags will trigger extra actions
};

static const array<string_view, 3> supportedCompilers =
{
	"clang-cl", //for windows only, MSVC toolchain
	"clang++",  //for windows and linux, GNU toolchain for mingw, msys2 and similar

	"cl"        //windows-only MSVC compiler
};

static const array<string_view, 3> supportedTypes =
{
	"executable", //creates a runnable executable
	"shared-lib", //creates .dll + .lib (.so + .a on Posix)
	"static-lib"  //creates .lib only (.a only on Posix)
};

static const array<string_view, 5> supportedCStandards =
{
	"c89",
	"c99",
	"c11",
	"c17",
	"c23"
};
static const array<string_view, 6> supportedCPPStandards =
{
	"c++11",
	"c++14",
	"c++17",
	"c++20",
	"c++23",
	"c++26"
};

//same warning types are used for both Windows and Linux,
//their true meanings change depending on which OS is used
static const array<string_view, 6> supportedWarningTypes =
{
	//no warnings
	//  Windows: -W0
	//  Linux:   -w
	"none",

	//very basic warnings
	//  Windows: -W1
	//  Linux:   -Wall
	"basic",

	//common, useful warnings
	//  Windows: -W2
	//  Linux:   -Wall
	//           -Wextra
	"normal",

	//strong warnings, recommended default
	//  Windows: -W3
	//  Linux:   -Wall
	//           -Wextra
	//           -Wpedantic
	"strong",

	//very strict, high signal warnings
	//  Windows: -W4
	//  Linux:   -Wall
	//           -Wextra
	//           -Wpedantic
	//           -Wshadow
	//           -Wconversion
	//           -Wsign-conversion
	"strict",

	//everything
	//  Windows:       -Wall
	//  Linux (clang): -Wall
	//                 -Wextra
	//                 -Wpedantic
	//                 -Weverything
	//  Linux (GCC):   -Wall
	//                 -Wextra
	//                 -Wpedantic
	//                 -Wshadow
	//                 -Wconversion
	//                 -Wsign-conversion
	//                 -Wcast-align
	//                 -Wnull-dereference
	//                 -Wdouble-promotion
	//                 -Wformat=2
	"all"
};

static const array<string_view, 7> supportedCustomFlags =
{
	//works on clang and cl, uses the multithreaded benefits of ninja for faster compilation
	"useninja",

	//fails the build if the compiler cannot support the requested standard
	//  cl:        /permissive-
	//  clang-cl:  nothing
	//  clang-c++: nothing
	//  gcc:       nothing
	"standardrequired",

	//treats all warnings as errors
	//  cl:        /WX
	//  clang-cl:  /WX
	//  clang-c++: -Werror
	//  gcc:       -Werror
	"warningsaserrors",

	//
	// build types (do not add more than one of these)
	//

	//only create debug build
	//  cl:        /Od /Zi
	//  clang-cl:  /Od /Zi
	//  clang-c++: -O0 -g
	//  gcc:       -O0 -g
	"debug",

	//only create standard release build
	//  cl:        /O2
	//  clang-cl:  /O2
	//  clang-c++: -O2
	//  gcc:       -O2
	"release",

	//only create release with debug symbols
	//  cl:        /O2 /Zi
	//  clang-cl:  /O2 /Zi
	//  clang-c++: -O2 -g
	//  gcc:       -O2 -g
	"reldebug",

	//only create minimum size release build
	//  cl:        /O1
	//  clang-cl:  /O1
	//  clang-c++: -Os
	//  gcc:       -Os
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
	string compiler{};
	vector<string> sources{};
	vector<string> headers{};
	vector<string> links{};
	string warningLevel;
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
	auto IsWarningLevel = [](string_view value) -> bool
		{
			return StartsWith(value, "warninglevel: ");
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
	auto IsSupportedHeaderExtension = [](string_view standard, const path& value) -> bool
		{
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

	auto IsSupportedType = [](string_view value) -> bool
		{
			for (const auto& t : supportedTypes)
			{
				if (value == t) return true;
			}
			
			PrintError("Failed to compile project because build type '" + string(value) + "' is not supported!");

			return false;
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

	auto IsSupportedBuildPath = [](
		string_view compiler,
		string_view type,
		string_view standard,
		const path& value) -> bool
		{
			if (!exists(value))
			{
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
						if (value.extension() != ".exe"
							&& (compiler == "clang-cl"
								|| compiler == "cl"))
						{
							PrintError("Failed to compile project because passed build path '" + value.string() + "' executable extension on Windows is not valid!");

							return false;
						}
					}
					else if (type == "shared-lib")
					{
						if (value.extension() != ".lib"
							&& (compiler == "clang-cl"
								|| compiler == "cl"))
						{
							PrintError("Failed to compile project because passed build path '" + value.string() + "' shared library extension on Windows is not valid!");

							return false;
						}
						else if (value.extension() != ".so"
							&& (compiler == "clang-c++"
								|| compiler == "gcc"))
						{
							PrintError("Failed to compile project because passed build path '" + value.string() + "' shared library extension on Posix is not valid!");

							return false;
						}
					}
					else if (type == "static-lib")
					{
						if (value.extension() != ".lib"
							&& (compiler == "clang-cl"
								|| compiler == "cl"))
						{
							PrintError("Failed to compile project because passed build path '" + value.string() + "' static library extension on Windows is not valid!");

							return false;
						}
						else if (value.extension() != ".a"
							&& (compiler == "clang-c++"
								|| compiler == "gcc"))
						{
							PrintError("Failed to compile project because passed build path '" + value.string() + "' static library extension on Posix is not valid!");

							return false;
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
						PrintError("Failed to compile project because passed obj path '" + value.string() + "' extension on Windows is not valid!");

						return false;
					}
					else if (value.extension() != ".o"
						&& (compiler == "clang-c++"
							|| compiler == "gcc"))
					{
						PrintError("Failed to compile project because passed obj path '" + value.string() + "' extension on Posix is not valid!");

						return false;
					}
				}

				//room for future expansion
				else return true;
			}

			return true;
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

		if (IsCompiler(line))
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
			vector<string> asteriskSources{};
			if (ContainsDuplicates(foundSources)) RemoveDuplicates(foundSources);

			bool failedSourceSearch{};

			for (auto& s : foundSources)
			{
				auto is_valid_source = [](string& h, bool silent = false) -> bool
					{
						path sourcePath = exists(h)
							? h
							: (kmaPath / h);

						if (!exists(sourcePath))
						{
							if (!silent) PrintError(
								"Failed to compile project because the source '" + sourcePath.string() 
								+ "' was not found!");

							return false;
						}

						try
						{
							h = path(weakly_canonical(sourcePath)).string();
						}
						catch (const filesystem_error&)
						{
							if (!silent) PrintError(
								"Failed to compile project because the source path '" + sourcePath.string() 
								+ "' could not be resolved!");

							return false;
						}

						return true;
					};

				auto add_to_asterisk_container = [is_valid_source](
					const path& dir,
					vector<string>& target,
					bool recursive = false)
					{
						if (recursive)
						{
							for (const auto& p : recursive_directory_iterator(dir))
							{
								string updatedValue = path(p).string();
								if (is_valid_source(updatedValue, true))
								{
									target.push_back(updatedValue);
								}
							}
						}
						else
						{
							for (const auto& p : directory_iterator(dir))
							{
								string updatedValue = path(p).string();
								if (is_valid_source(updatedValue, true))
								{
									target.push_back(updatedValue);
								}
							}
						}
					};

				if (ContainsString(s, "/*.")
					|| ContainsString(s, "\\*.")
					|| ContainsString(s, "/**.")
					|| ContainsString(s, "\\**."))
				{
					vector<string> split = SplitString(s, ".");

					if (split.size() == 2)
					{
						if (ContainsString(s, "/*.")
							|| ContainsString(s, "\\*."))
						{
							path full = s;
							path baseDir = full.parent_path();
							string pattern = full.filename().string();

							if (!exists(baseDir)) baseDir = path(kmaPath / baseDir).string();
							if (!exists(baseDir)
								|| !is_directory(baseDir))
							{
								PrintError(
									"Failed to compile project because the source path with asterisk '" + s
									+ "' does not exist or does not lead to a directory!");

								failedSourceSearch = true;

								break;
							}

							vector<path> foundPaths{};
							string result = GetRelativeFiles(baseDir, pattern, foundPaths);

							if (!result.empty())
							{
								PrintError(
									"Failed to compile project because the source path with asterisk '" + s
									+ "' failed to be resolved! Reason: " + result);

								failedSourceSearch = true;

								break;
							}

							for (auto& p : foundPaths)
							{
								string updatedValue = p.string();
								if (is_valid_source(updatedValue, true))
								{
									asteriskSources.push_back(updatedValue);
								}
							}
						}
						else if (ContainsString(s, "/**.")
							|| ContainsString(s, "\\**."))
						{
							path full = s;
							path baseDir = full.parent_path();
							string pattern = full.filename().string();

							if (!exists(baseDir)) baseDir = path(kmaPath / baseDir).string();
							if (!exists(baseDir)
								|| !is_directory(baseDir))
							{
								PrintError(
									"Failed to compile project because the source path with asterisk '" + s
									+ "' does not exist or does not lead to a directory!");

								failedSourceSearch = true;

								break;
							}

							vector<path> foundPaths{};
							string result = GetRelativeFiles(baseDir, pattern, foundPaths, true);

							if (!result.empty())
							{
								PrintError(
									"Failed to compile project because the source path with asterisk '" + s
									+ "' failed to be resolved! Reason: " + result);

								failedSourceSearch = true;

								break;
							}

							for (auto& p : foundPaths)
							{
								string updatedValue = p.string();
								if (is_valid_source(updatedValue, true))
								{
									asteriskSources.push_back(updatedValue);
								}
							}
						}
					}
					else
					{
						PrintError(
							"Failed to compile project because the source path with asterisk '" + s
							+ "' structure is invalid");

						failedSourceSearch = true;

						break;
					}
				}
				else if (s == "*")  add_to_asterisk_container(kmaPath, asteriskSources);
				else if (s == "**") add_to_asterisk_container(kmaPath, asteriskSources, true);
				else if (EndsWith(s, "/*")
					|| EndsWith(s, "\\*"))
				{
					string baseDir = s.substr(0, s.size() - 2);
					if (!exists(baseDir)) baseDir = path(kmaPath / baseDir).string();
					if (!exists(baseDir)
						|| !is_directory(baseDir))
					{
						PrintError(
							"Failed to compile project because source path with asterisk '" + s 
							+ "' does not exist or does not lead to a directory!");

						failedSourceSearch = true;

						break;
					}

					add_to_asterisk_container(baseDir, asteriskSources);
				}
				else if (EndsWith(s, "/**")
					|| EndsWith(s, "\\**"))
				{
					string baseDir = s.substr(0, s.size() - 3);
					if (!exists(baseDir)) baseDir = path(kmaPath / baseDir).string();
					if (!exists(baseDir)
						|| !is_directory(baseDir))
					{
						PrintError(
							"Failed to compile project because the source path with asterisk '" + s 
							+ "' does not exist or does not lead to a directory!");

						failedSourceSearch = true;

						break;
					}

					add_to_asterisk_container(baseDir, asteriskSources, true);
				}
				else
				{
					if (!is_valid_source(s))
					{
						failedSourceSearch = true;

						break;
					}

				}
			}

			if (failedSourceSearch) return;

			//move asterisk sources content to found sources container end
			foundSources.insert(
				foundSources.end(),
				make_move_iterator(asteriskSources.begin()),
				make_move_iterator(asteriskSources.end()));

			//move found sources to sources container
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

			//ignore empty headers field
			if (value.empty()) continue;

			vector<string> foundHeaders = SplitString(value, ", ");
			vector<string> asteriskHeaders{};
			if (ContainsDuplicates(foundHeaders)) RemoveDuplicates(foundHeaders);

			bool failedHeaderSearch{};

			for (auto& h : foundHeaders)
			{
				auto is_valid_header = [](string& h, bool silent = false) -> bool
					{
						path sourcePath = exists(h)
							? h
							: (kmaPath / h);

						if (!exists(sourcePath))
						{
							if (!silent) PrintError(
								"Failed to compile project because the header '" + sourcePath.string() 
								+ "' was not found!");

							return false;
						}

						try
						{
							h = path(weakly_canonical(sourcePath)).string();
						}
						catch (const filesystem_error&)
						{
							if (!silent) PrintError(
								"Failed to compile project because the header path '" + sourcePath.string() 
								+ "' could not be resolved!");

							return false;
						}

						return true;
					};

				auto add_to_asterisk_container = [is_valid_header](
					const path& dir, 
					vector<string>& target,
					bool recursive = false)
					{
						if (recursive)
						{
							for (const auto& p : recursive_directory_iterator(dir))
							{
								string updatedValue = path(p).string();
								if (is_valid_header(updatedValue, true))
								{
									target.push_back(updatedValue);
								}
							}
						}
						else
						{
							for (const auto& p : directory_iterator(dir))
							{
								string updatedValue = path(p).string();
								if (is_valid_header(updatedValue, true))
								{
									target.push_back(updatedValue);
								}
							}
						}
					};

				if (ContainsString(h, "/*.")
					|| ContainsString(h, "\\*.")
					|| ContainsString(h, "/**.")
					|| ContainsString(h, "\\**."))
				{
					vector<string> split = SplitString(h, ".");

					if (split.size() == 2)
					{
						if (ContainsString(h, "/*.")
							|| ContainsString(h, "\\*."))
						{
							path full = h;
							path baseDir = full.parent_path();
							string pattern = full.filename().string();

							if (!exists(baseDir)) baseDir = path(kmaPath / baseDir).string();
							if (!exists(baseDir)
								|| !is_directory(baseDir))
							{
								PrintError(
									"Failed to compile project because the header path with asterisk '" + h
									+ "' does not exist or does not lead to a directory!");

								failedHeaderSearch = true;

								break;
							}

							vector<path> foundPaths{};
							string result = GetRelativeFiles(baseDir, pattern, foundPaths);

							if (!result.empty())
							{
								PrintError(
									"Failed to compile project because the header path with asterisk '" + h
									+ "' failed to be resolved! Reason: " + result);

								failedHeaderSearch = true;

								break;
							}

							for (auto& p : foundPaths)
							{
								string updatedValue = p.string();
								if (is_valid_header(updatedValue, true))
								{
									asteriskHeaders.push_back(updatedValue);
								}
							}
						}
						else if (ContainsString(h, "/**.")
							|| ContainsString(h, "\\**."))
						{
							path full = h;
							path baseDir = full.parent_path();
							string pattern = full.filename().string();

							if (!exists(baseDir)) baseDir = path(kmaPath / baseDir).string();
							if (!exists(baseDir)
								|| !is_directory(baseDir))
							{
								PrintError(
									"Failed to compile project because the header path with asterisk '" + h
									+ "' does not exist or does not lead to a directory!");

								failedHeaderSearch = true;

								break;
							}

							vector<path> foundPaths{};
							string result = GetRelativeFiles(baseDir, pattern, foundPaths, true);

							if (!result.empty())
							{
								PrintError(
									"Failed to compile project because the header path with asterisk '" + h
									+ "' failed to be resolved! Reason: " + result);

								failedHeaderSearch = true;

								break;
							}

							for (auto& p : foundPaths)
							{
								string updatedValue = p.string();
								if (is_valid_header(updatedValue, true))
								{
									asteriskHeaders.push_back(updatedValue);
								}
							}
						}
					}
					else
					{
						PrintError(
							"Failed to compile project because the header path with asterisk '" + h
							+ "' structure is invalid");

						failedHeaderSearch = true;

						break;
					}
				}
				else if (h == "*")  add_to_asterisk_container(kmaPath, asteriskHeaders);
				else if (h == "**") add_to_asterisk_container(kmaPath, asteriskHeaders, true);
				else if (EndsWith(h, "/*")
						 || EndsWith(h, "\\*"))
				{
					string baseDir = h.substr(0, h.size() - 2);
					if (!exists(baseDir)) baseDir = path(kmaPath / baseDir).string();
					if (!exists(baseDir)
						|| !is_directory(baseDir))
					{
						PrintError(
							"Failed to compile project because header path with asterisk '" + h 
							+ "' does not exist or does not lead to a directory!");

						failedHeaderSearch = true;

						break;
					}

					add_to_asterisk_container(baseDir, asteriskHeaders);
				}
				else if (EndsWith(h, "/**")
						 || EndsWith(h, "\\**"))
				{
					string baseDir = h.substr(0, h.size() - 3);
					if (!exists(baseDir)) baseDir = path(kmaPath / baseDir).string();
					if (!exists(baseDir)
						|| !is_directory(baseDir))
					{
						PrintError(
							"Failed to compile project because header path with asterisk '" + h 
							+ "' does not exist or does not lead to a directory!");

						failedHeaderSearch = true;

						break;
					}

					add_to_asterisk_container(baseDir, asteriskHeaders, true);
				}
				else
				{
					if (!is_valid_header(h))
					{
						failedHeaderSearch = true;

						break;
					}

				}
			}

			if (failedHeaderSearch) return;

			//move asterisk headers content to found headers container end
			foundHeaders.insert(
				foundHeaders.end(),
				make_move_iterator(asteriskHeaders.begin()),
				make_move_iterator(asteriskHeaders.end()));

			//move found sources to headers container
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

			bool failedLinkSearch{};

			//resolve all relative link files with extensions to project kma file path
			for (auto& l : foundLinks)
			{
				if ((EndsWith(l, ".lib")
					|| EndsWith(l, ".so")
					|| EndsWith(l, ".a"))
					&& !exists(l))
				{
					l = path(kmaPath / l).string();
				}

				if (exists(l))
				{
					try
					{
						l = path(weakly_canonical(l)).string();
					}
					catch (const filesystem_error&)
					{
						PrintError("Failed to compile project because link path '" + l + "' could not be resolved!");

						failedLinkSearch = true;

						return;
					}
				}
			}

			if (failedLinkSearch) return;

			links = std::move(foundLinks);
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
		else if (IsName(line))
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

			if (!exists(value)) value = path(kmaPath / buildPath).string();
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

			if (!exists(value)) value = path(kmaPath / objPath).string();
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
	// VERIFY SOURCES AND HEADERS AGAINST STANDARD
	//

	vector<string> cleanedSources{};
	vector<string> cleanedHeaders{};

	for (const auto& s : sources)
	{
		if (is_directory(s))
		{
			for (const auto& d : recursive_directory_iterator(s))
			{
				if (!is_regular_file(d)) continue;

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
	// ASSIGN DEFAULT VALUES
	//

	if (warningLevel.empty()) warningLevel = "strong";
	if (buildPath.empty()) buildPath = path(kmaPath / defaultBuildPath).string();
	if (objPath.empty()) objPath = path(kmaPath / defaultObjPath).string();

	if (ContainsDuplicates(cleanedSources)) RemoveDuplicates(cleanedSources);
	if (ContainsDuplicates(cleanedHeaders)) RemoveDuplicates(cleanedHeaders);
	if (ContainsDuplicates(links)) RemoveDuplicates(links);
	if (ContainsDuplicates(defines)) RemoveDuplicates(defines);
	if (ContainsDuplicates(extensions)) RemoveDuplicates(extensions);
	if (ContainsDuplicates(flags)) RemoveDuplicates(flags);
	if (ContainsDuplicates(customFlags)) RemoveDuplicates(customFlags);

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
		buildPath))
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
		<< "compiler: " << compiler << "\n"
		<< "standard: " << standard << "\n"
		<< "sources:\n";

	for (const auto& s : cleanedSources)
	{
		oss << "  " << s << "\n";
	}

	if (!headers.empty())
	{
		oss << "headers:\n";

		for (const auto& h : cleanedHeaders)
		{
			oss << "  " << h << "\n";
		}
	}
	if (!links.empty())
	{
		oss << "links:\n";

		for (const auto& l : links)
		{
			oss << "  " << l << "\n";
		}
	}

	oss << "warning level: " << warningLevel << "\n";

	oss << "build path: " << buildPath << "\n";
	oss << "obj path: " << objPath << "\n";

	if (!flags.empty())
	{
		oss << "flags:\n";

		for (const auto& f : flags)
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

	if (!extensions.empty())
	{
		oss << "extensions:\n";

		for (const auto& e : extensions)
		{
			oss << "  " << e << "\n";
		}
	}

	Log::Print(
		oss.str(),
		"KALAMAKE",
		LogType::LOG_INFO);

	Log::Print("==========================================================================================\n");

	CompileProject(
		{
			.compiler = compiler,
			.sources = std::move(cleanedSources),
			.headers = std::move(cleanedHeaders),
			.links = std::move(links),
			.warningLevel = warningLevel,
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
	vector<string> finalFlags = std::move(compileData.flags);

	//
	// BINARY TYPE
	//

	path buildPath{};

	if (compileData.type == "executable")
	{
		if (compileData.compiler == "clang-cl"
			|| compileData.compiler == "cl")
		{
			buildPath = path(compileData.buildPath) / string(compileData.name + ".exe");

			finalFlags.push_back("/Fe:\"" + buildPath.string() + "\"");
		}
		else if (compileData.compiler == "clang-c++"
			|| compileData.compiler == "gcc")
		{
			buildPath = path(compileData.buildPath) / compileData.name;

			finalFlags.push_back("-o \"" + buildPath.string() + "\"");
		}
	}
	else if (compileData.type == "shared-lib")
	{
		if (compileData.compiler == "clang-cl"
			|| compileData.compiler == "cl")
		{
			buildPath = path(compileData.buildPath) / string(compileData.name + ".dll");
			path libPath = path(compileData.buildPath) / string(compileData.name + ".lib");

			finalFlags.push_back("/LD");
			finalFlags.push_back("/link");
			finalFlags.push_back("/OUT:\"" + buildPath.string() + "\"");
			finalFlags.push_back("/IMPLIB:\"" + libPath.string() + "\"");
		}
		else if (compileData.compiler == "clang-c++"
			|| compileData.compiler == "gcc")
		{
			buildPath = path(compileData.buildPath) / string(compileData.name + ".so");

			finalFlags.push_back("-fPIC");
			finalFlags.push_back("-shared");
			finalFlags.push_back("-o \"" + buildPath.string() + "\"");
		}
	}
	else if (compileData.type == "static-lib")
	{
		//continue to static lib compilation function
		//since its very different from exe and shared lib
		CompileStaticLib(compileData);

		return;
	}

	//
	// STANDARD
	//

	if (compileData.compiler == "clang-cl"
		|| compileData.compiler == "cl")
	{
		finalFlags.push_back("/std:" + compileData.standard);
	}
	else if (compileData.compiler == "clang-c++"
		|| compileData.compiler == "gcc")
	{
		finalFlags.push_back("-std=" + compileData.standard);
	}

	//
	// EXTENSIONS
	//

	//...

	//
	// SOURCES
	//

	if (compileData.compiler == "clang-cl"
		|| compileData.compiler == "cl"
		|| compileData.compiler == "clang-c++"
		|| compileData.compiler == "gcc")
	{
		for (const auto& s : compileData.sources)
		{
			finalFlags.push_back("\"" + s + "\"");
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
				finalFlags.push_back("/I\"" + s + "\"");
			}
		}
		else if (compileData.compiler == "clang-c++"
				 || compileData.compiler == "gcc")
		{
			for (const auto& s : compileData.headers)
			{
				finalFlags.push_back("-I\"" + s + "\"");
			}
		}
	}

	//
	// LINKS
	//

	if (!compileData.links.empty())
	{
		if (compileData.compiler == "clang-cl"
			|| compileData.compiler == "cl")
		{
			for (const auto& l : compileData.links)
			{
				if (EndsWith(l, ".lib")) finalFlags.push_back("\"" + l + "\"");
				else                     finalFlags.push_back(l);                   
			}
		}
		else if (compileData.compiler == "clang-c++"
				 || compileData.compiler == "gcc")
		{
			for (const auto& l : compileData.links)
			{
				if (EndsWith(l, ".so")
					|| EndsWith(l, ".a"))
				{
					finalFlags.push_back("\"" + l + "\"");
				}
				else finalFlags.push_back("-l" + l);
			}
		}
	}

	//
	// WARNING LEVEL
	//

	if (compileData.compiler == "clang-cl"
		|| compileData.compiler == "cl")
	{
		if (compileData.warningLevel == "none")        finalFlags.push_back("/W0");
		else if (compileData.warningLevel == "basic")  finalFlags.push_back("/W1");
		else if (compileData.warningLevel == "normal") finalFlags.push_back("/W2");
		else if (compileData.warningLevel == "strong") finalFlags.push_back("/W3");
		else if (compileData.warningLevel == "strict") finalFlags.push_back("/W4");
		else if (compileData.warningLevel == "all")    finalFlags.push_back("/Wall");
	}
	else if (compileData.compiler == "clang-c++"
		|| compileData.compiler == "gcc")
	{
		if (compileData.warningLevel == "none")        finalFlags.push_back("-w");
		else if (compileData.warningLevel == "basic")  finalFlags.push_back("-Wall");
		else if (compileData.warningLevel == "normal")
		{
			finalFlags.push_back("-Wall");
			finalFlags.push_back("-Wextra");
		}
		else if (compileData.warningLevel == "strong")
		{
			finalFlags.push_back("-Wall");
			finalFlags.push_back("-Wextra");
			finalFlags.push_back("-Wpedantic");
		}
		else if (compileData.warningLevel == "strict")
		{
			finalFlags.push_back("-Wall");
			finalFlags.push_back("-Wextra");
			finalFlags.push_back("-Wpedantic");
			finalFlags.push_back("-Wshadow");
			finalFlags.push_back("-Wconversion");
			finalFlags.push_back("-Wsign-conversion");
		}
		else if (compileData.warningLevel == "all")
		{
			if (compileData.compiler == "clang-c++")
			{
				finalFlags.push_back("-Wall");
				finalFlags.push_back("-Wextra");
				finalFlags.push_back("-Wpedantic");
				finalFlags.push_back("-Weverything");
			}
			else
			{
				finalFlags.push_back("-Wall");
				finalFlags.push_back("-Wextra");
				finalFlags.push_back("-Wpedantic");
				finalFlags.push_back("-Wshadow");
				finalFlags.push_back("-Wconversion");
				finalFlags.push_back("-Wsign-conversion");
				finalFlags.push_back("-Wcast-align");
				finalFlags.push_back("-Wnull-dereference");
				finalFlags.push_back("-Wdouble-promotion");
				finalFlags.push_back("-Wformat=2");
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
			|| compileData.compiler == "clang-c++"
			|| compileData.compiler == "gcc")
		{
			for (const auto& d : compileData.defines)
			{
				finalFlags.push_back("-D" + d);
			}
		}
	}

	//
	// CUSTOM FLAGS
	//

	for (const auto& f : compileData.customFlags)
	{
		if (f == "standardrequired"
			&& compileData.compiler == "cl")
		{
			finalFlags.push_back("/permissive-");

			continue;
		}

		if (f == "warningsaserrors")
		{
			if (compileData.compiler == "clang-cl"
				|| compileData.compiler == "cl")
			{
				finalFlags.push_back("/WX");
			}
			else if (compileData.compiler == "clang-c++"
				|| compileData.compiler == "gcc")
			{
				finalFlags.push_back("-Werror");
			}

			continue;
		}

		//build types

		if (f == "debug")
		{
			if (compileData.compiler == "clang-cl"
				|| compileData.compiler == "cl")
			{
				finalFlags.push_back("/Od");
				finalFlags.push_back("/Zi");
			}
			else if (compileData.compiler == "clang-c++"
					 || compileData.compiler == "gcc")
			{
				finalFlags.push_back("-O0");
				finalFlags.push_back("-g");
			}

			continue;
		}
		if (f == "release")
		{
			if (compileData.compiler == "clang-cl"
				|| compileData.compiler == "cl")
			{
				finalFlags.push_back("/O2");
			}
			if (compileData.compiler == "clang-c++"
				|| compileData.compiler == "gcc")
			{
				finalFlags.push_back("-O2");
			}

			continue;
		}
		if (f == "relwithdebinfo")
		{
			if (compileData.compiler == "clang-cl"
				|| compileData.compiler == "cl")
			{
				finalFlags.push_back("/O2");
				finalFlags.push_back("/Zi");
			}
			else if (compileData.compiler == "clang-c++"
					 || compileData.compiler == "gcc")
			{
				finalFlags.push_back("-O2");
				finalFlags.push_back("-g");
			}

			continue;
		}
		if (f == "minsizerel")
		{
			if (compileData.compiler == "clang-cl"
				|| compileData.compiler == "cl")
			{
				finalFlags.push_back("/O1");
			}
			else if (compileData.compiler == "clang-c++"
					 || compileData.compiler == "gcc")
			{
				finalFlags.push_back("-O1");
			}

			continue;
		}
	}

	//
	// BUILD THE PROJECT
	//

	if (ContainsDuplicates(finalFlags)) RemoveDuplicates(finalFlags);

	ostringstream oss{};

	for (int i = 0; i < finalFlags.size(); i++)
	{
		if (i > 0) oss << ' ';
		oss << finalFlags[i];
	}

	Log::Print(
		"Flags passed to compiler " + compileData.compiler + ":\n\n" + oss.str(),
		"KALAMAKE",
		LogType::LOG_INFO);

	Log::Print("\n==========================================================================================\n");

	string finalValue{};
	if (compileData.compiler == "cl")
	{
		finalValue = 
			"cmd /c \"\"" 
			+ foundCLPath.string() 
			+ "\" && cl "
			+ oss.str() + "\"";
	}
	else if (compileData.compiler == "clang-cl")   finalValue = "clang-cl " + oss.str();
	else if (compileData.compiler == "clang-c++")  finalValue = "clang-c++ " + oss.str();
	else if (compileData.compiler == "gcc")        finalValue = "gcc " + oss.str();

	if (system(finalValue.c_str()) != 0)
	{
		PrintError("Compilation failed!");
	}
}

void CompileStaticLib(const CompileData& compileData)
{

}