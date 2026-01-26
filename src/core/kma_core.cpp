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

#include "core.hpp"

#include "core/kma_core.hpp"
#include "language/kma_language.hpp"
#include "language/kma_language_c.hpp"
#include "language/kma_language_cpp.hpp"

using KalaHeaders::KalaCore::ContainsValue;
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
using KalaHeaders::KalaString::EndsWith;
using KalaHeaders::KalaString::StringToEnum;

using KalaCLI::Core;

using KalaMake::Core::KalaMakeCore;
using KalaMake::Language::LanguageCore;
using KalaMake::Language::Language_C;
using KalaMake::Language::Language_CPP;

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

#ifndef scast
	#define scast static_cast
#endif

//kma path is the root directory where the kma file is stored at
static path kmaPath{};

//default build directory path relative to kmaPath if buildpath is not added or filled
static path defaultBuildPath = "build";
//default object directory path relative to kmaPath if objpath is not added or filled
static path defaultObjPath = "obj";

constexpr string_view cl_ide_bat_2026 = "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat";
constexpr string_view cl_build_bat_2026 = "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat";
constexpr string_view cl_ide_bat_2022 = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat";
constexpr string_view cl_build_bat_2022 = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat";

namespace KalaMake::Core
{
	struct ActionTypeHash
	{
		size_t operator()(ActionType a) const noexcept { return scast<size_t>(a); }
	};

	static const unordered_map<ActionType, string_view, ActionTypeHash> actionTypes =
	{
		{ ActionType::T_NAME,        "name" },
		{ ActionType::T_BINARY_TYPE, "binarytype" },
		{ ActionType::T_COMPILER,    "compiler" },
		{ ActionType::T_STANDARD,    "standard" },
		{ ActionType::T_SOURCES,     "sources" },

		{ ActionType::T_BUILD_PATH,  "buildpath" },
		{ ActionType::T_OBJ_PATH,    "objpath" },
		{ ActionType::T_HEADERS,     "headers" },
		{ ActionType::T_LINKS,       "links" },
		{ ActionType::T_DEBUG_LINKS, "debuglinks" },

		{ ActionType::T_WARNING_LEVEL, "warninglevel" },
		{ ActionType::T_DEFINES,       "defines" },
		{ ActionType::T_EXTENSIONS,    "extensions" },

		{ ActionType::T_FLAGS,        "flags" },
		{ ActionType::T_DEBUG_FLAGS,  "debugflags" },
		{ ActionType::T_CUSTOM_FLAGS, "customflags" }
	};

	struct BinaryTypeHash
	{
		size_t operator()(BinaryType b) const noexcept { return scast<size_t>(b); }
	};

	static const unordered_map<BinaryType, string_view, BinaryTypeHash> binaryTypes =
	{
		{ BinaryType::B_EXECUTABLE,   "executable" },
		{ BinaryType::B_LINK_ONLY,    "link-only" },
		{ BinaryType::B_RUNTIME_ONLY, "runtime-only" },
		{ BinaryType::B_LINK_RUNTIME, "link-runtime" }
	};

	struct WarningLevelHash
	{
		size_t operator()(WarningLevel w) const noexcept { return scast<size_t>(w); }
	};

	//Same warning types are used for both MSVC and GNU,
	//their true meanings change depending on which OS is used
	static const unordered_map<WarningLevel, string_view, WarningLevelHash> warningLevels =
	{
		{ WarningLevel::W_NONE,   "none" },
		{ WarningLevel::W_BASIC,  "basic" },
		{ WarningLevel::W_NORMAL, "normal" },
		{ WarningLevel::W_STRONG, "strong" },
		{ WarningLevel::W_STRICT, "strict" },
		{ WarningLevel::W_ALL,    "all" }
	};

	struct CustomFlagHash
	{
		size_t operator()(CustomFlag c) const noexcept { return scast<size_t>(c); }
	};

	//Same warning types are used for both MSVC and GNU,
	//their true meanings change depending on which OS is used
	static const unordered_map<CustomFlag, string_view, CustomFlagHash> customFlags =
	{
		{ CustomFlag::F_USE_NINJA,               "use-ninja" },
		{ CustomFlag::F_NO_OBJ,                  "no-obj" },
		{ CustomFlag::F_STANDARD_REQUIRED,       "standard-required" },
		{ CustomFlag::F_WARNINGS_AS_ERRORS,      "warnings-as-errors" },
		{ CustomFlag::F_EXPORT_COMPILE_COMMANDS, "export-compile-commands" },

		{ CustomFlag::F_BUILD_DEBUG,      "build-debug" },
		{ CustomFlag::F_BUILD_RELEASE,    "build-release" },
		{ CustomFlag::F_BUILD_RELDEBUG,   "build-reldebug" },
		{ CustomFlag::F_BUILD_MINSIZEREL, "build-minsizerel" }
	};

	void KalaMakeCore::Initialize(const vector<string>& params)
	{
		ostringstream details{};

		details
			<< "     | exe version: " << EXE_VERSION_NUMBER.data() << "\n"
			<< "     | kma version: " << KMA_VERSION_NUMBER.data() << "\n";

		Log::Print(details.str());

		path projectFile = params[1];

		string& currentDir = KalaCLI::Core::GetCurrentDir();
		if (currentDir.empty()) currentDir = current_path().string();

		auto readprojectfile = [](path filePath) -> void
			{
				if (is_directory(filePath))
				{
					KalaMakeCore::PrintError("Failed to compile project because its path '" + filePath.string() + "' leads to a directory!");

					return;
				}

				if (!filePath.has_extension()
					|| filePath.extension() != ".kma")
				{
					KalaMakeCore::PrintError("Failed to compile project because its path '" + filePath.string() + "' has an incorrect extension!");

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
					KalaMakeCore::PrintError("Failed to read project file '" + filePath.string() + "'! Reason: " + result);

					return;
				}

				if (content.empty())
				{
					KalaMakeCore::PrintError("Failed to compile project at '" + filePath.string() + "' because it was empty!");

					return;
				}

				kmaPath = filePath.parent_path();

				CompileData data = Parse(content);
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

	CompileData KalaMakeCore::Parse(const vector<string>& lines)
	{
		string name{};
		string binaryType{};
		string standard{};
		string compiler{};
		vector<string> sources{};

		string buildPath{};
		string objPath{};
		vector<string> headers{};
		vector<string> links{};
		vector<string> debugLinks{};

		string warningLevel;
		vector<string> defines{};
		vector<string> extensions{};

		vector<string> flags{};
		vector<string> debugFlags{};
		vector<string> localCustomFlags{};

		auto IsName = [](string_view value) -> bool
			{
				return StartsWith(value, "name: ");
			};
		auto IsBinaryType = [](string_view value) -> bool
			{
				return StartsWith(value, "binarytype: ");
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
		auto IsLink = [](string_view value) -> bool
			{
				return StartsWith(value, "links: ");
			};
		auto IsDebugLink = [](string_view value) -> bool
			{
				return StartsWith(value, "debuglinks: ");
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

		auto IsFlag = [](string_view value) -> bool
			{
				return StartsWith(value, "flags: ");
			};
		auto IsDebugFlag = [](string_view value) -> bool
			{
				return StartsWith(value, "debugflags: ");
			};
		auto IsCustomFlag = [](string_view value) -> bool
			{
				return StartsWith(value, "customflags: ");
			};

		auto IsSupportedName = [](string_view value) -> bool
			{
				if (value.size() < MIN_NAME_LENGTH)
				{
					KalaMakeCore::PrintError("Failed to compile project because name is too short!");

					return false;
				}
				if (value.size() > MAX_NAME_LENGTH)
				{
					KalaMakeCore::PrintError("Failed to compile project because name is too long!");

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

					KalaMakeCore::PrintError("Failed to compile project because name contains illegal characters!");

					return false;
				}

				return true;
			};
		auto IsSupportedBinaryType = [](string_view value) -> bool
			{
				bool isSupported = 
					LanguageCore::IsMSVCCompiler(value)
					|| LanguageCore::IsGNUCompiler(value);

				if (isSupported) return true;

				KalaMakeCore::PrintError("Failed to compile project because binary type '" + string(value) + "' is not supported!");

				return false;
			};
		auto IsSupportedStandard = [](string_view value) -> bool
			{
				if (Language_CPP::IsCPPStandard(value)
					|| Language_C::IsCStandard(value))
				{
					return true;
				}

				KalaMakeCore::PrintError("Failed to compile project because standard '" + string(value) + "' is not supported!");

				return false;
			};
		auto IsSupportedCompiler = [](string_view value) -> bool
			{
				bool isSupported =
					LanguageCore::IsMSVCCompiler(value)
					|| LanguageCore::IsGNUCompiler(value);

				if (isSupported) return true;

				KalaMakeCore::PrintError("Failed to compile project because compiler '" + string(value) + "' is not supported!");

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

				if (Language_C::IsCStandard(standard)
					&& value.extension() != ".c")
				{
					Log::Print(
						"Skipped invalid source file '" + value.string() + "' because it is not supported by the C standard.",
						"KALAMAKE",
						LogType::LOG_WARNING);

					return false;
				}

				if (Language_CPP::IsCPPStandard(standard)
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

					KalaMakeCore::PrintError("Failed to compile project because passed build path '" + value.string() + "' does not exist!");

					return false;
				}

				if (is_directory(value)) return true;
				else
				{
					if (Language_CPP::IsCPPStandard(standard)
						|| Language_C::IsCStandard(standard))
					{
						if (type == "executable")
						{
							if ((compiler == "clang-cl"
								|| compiler == "cl")
								&& value.extension() != ".exe")
							{
								KalaMakeCore::PrintError("Failed to compile project because passed build path '" + value.string() + "' executable extension is not valid!");

								return false;
							}
							else if (compiler == "clang"
								|| compiler == "clang++"
								|| compiler == "gcc"
								|| compiler == "g++")
							{
#ifdef _WIN32
								if (value.extension() != ".exe")
								{
									KalaMakeCore::PrintError("Failed to compile project because passed build path '" + value.string() + "' executable extension is not valid!");

									return false;
								}
#else
								if (value.has_extension())
								{
									KalaMakeCore::PrintError("Failed to compile project because passed build path '" + value.string() + "' executable extension is not valid!");

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
								KalaMakeCore::PrintError("Failed to compile project because passed build path '" + value.string() + "' shared library extension is not valid!");

								return false;
							}
							else if (compiler == "clang"
								|| compiler == "clang++"
								|| compiler == "gcc"
								|| compiler == "g++")
							{
#ifdef _WIN32
								if (value.extension() != ".dll")
								{
									KalaMakeCore::PrintError("Failed to compile project because passed build path '" + value.string() + "' shared library extension is not valid!");

									return false;
								}
#else
								if (value.extension() != ".so")
								{
									KalaMakeCore::PrintError("Failed to compile project because passed build path '" + value.string() + "' shared library extension is not valid!");

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
								KalaMakeCore::PrintError("Failed to compile project because passed build path '" + value.string() + "' static library extension is not valid!");

								return false;
							}
							else if (compiler == "clang"
								|| compiler == "clang++"
								|| compiler == "gcc"
								|| compiler == "g++")
							{
#ifdef _WIN32
								if (value.extension() != ".lib")
								{
									KalaMakeCore::PrintError("Failed to compile project because passed build path '" + value.string() + "' static library extension is not valid!");

									return false;
								}
#else
								if (value.extension() != ".a")
								{
									KalaMakeCore::PrintError("Failed to compile project because passed build path '" + value.string() + "' static library extension is not valid!");

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
								KalaMakeCore::PrintError("Failed to compile project because passed build path '" + value.string() + "' runtime library extension is not valid!");

								return false;
							}
							else if (compiler == "clang"
								|| compiler == "clang++"
								|| compiler == "gcc"
								|| compiler == "g++")
							{
#ifdef _WIN32
								if (value.extension() != ".dll")
								{
									KalaMakeCore::PrintError("Failed to compile project because passed build path '" + value.string() + "' runtime library extension is not valid!");

									return false;
								}
#else
								if (value.extension() != ".so")
								{
									KalaMakeCore::PrintError("Failed to compile project because passed build path '" + value.string() + "' runtime library extension is not valid!");

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

					KalaMakeCore::PrintError("Failed to compile project because passed obj path '" + value.string() + "' does not exist!");

					return false;
				}

				if (is_directory(value)) return true;
				else
				{
					if (Language_CPP::IsCPPStandard(standard)
						|| Language_C::IsCStandard(standard))
					{
						if (value.extension() != ".obj"
							&& (compiler == "clang-cl"
								|| compiler == "cl"))
						{
							KalaMakeCore::PrintError("Failed to compile project because passed obj path '" + value.string() + "' extension is not valid!");

							return false;
						}
						else if (compiler == "clang"
							|| compiler == "clang++"
							|| compiler == "gcc"
							|| compiler == "g++")
						{
#ifdef _WIN32
							if (value.extension() != ".obj")
							{
								KalaMakeCore::PrintError("Failed to compile project because passed obj path '" + value.string() + "' extension is not valid!");

								return false;
							}
#else
							if (value.extension() != ".o")
							{
								KalaMakeCore::PrintError("Failed to compile project because passed obj path '" + value.string() + "' extension is not valid!");

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

				if (Language_C::IsCStandard(standard)
					&& value.extension() != ".h")
				{
					Log::Print(
						"Skipped invalid header file '" + value.string() + "' because it is not supported by the C standard.",
						"KALAMAKE",
						LogType::LOG_WARNING);

					return false;
				}

				if (Language_CPP::IsCPPStandard(standard)
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

		auto IsSupportedWarningLevel = [IsWarningLevel](string_view value) -> bool
			{
				if (IsWarningLevel(value)) return true;

				KalaMakeCore::PrintError("Failed to compile project because warning level '" + string(value) + "' is not supported!");

				return false;
			};
		auto IsSupportedCustomFlag = [IsCustomFlag](string_view value) -> bool
			{
				if (IsCustomFlag(value)) return true;

				KalaMakeCore::PrintError("Failed to compile project because custom flag '" + string(value) + "' is not supported!");

				return false;
			};

		if (lines[0] != KMA_VERSION_NAME)
		{
			KalaMakeCore::PrintError("Failed to compile project because kma version field value is malformed!");

			return{};
		}

		//
		// READ FILE CONTENT IN KMA FILE
		//

		for (size_t i = 0; i < lines.size(); i++)
		{
			const string_view& line = lines[i];

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
					KalaMakeCore::PrintError("Failed to compile project because more than one name line was passed!");

					return{};
				}

				string value = RemoveFromString(line, "name: ");

				if (value.empty())
				{
					KalaMakeCore::PrintError("Failed to compile project because no name value was passed!");

					return{};
				}

				if (!IsSupportedName(value)) return{};

				name = value;
			}
			else if (IsBinaryType(line))
			{
				if (!binaryType.empty())
				{
					KalaMakeCore::PrintError("Failed to compile project because more than one binarytype line was passed!");

					return{};
				}

				string value = RemoveFromString(line, "binarytype: ");

				if (value.empty())
				{
					KalaMakeCore::PrintError("Failed to compile project because no binarytype value was passed!");

					return{};
				}

				if (!IsSupportedBinaryType(value)) return{};

				binaryType = value;
			}
			else if (IsStandard(line))
			{
				if (!standard.empty())
				{
					KalaMakeCore::PrintError("Failed to compile project because more than one standard line was passed!");

					return{};
				}

				string value = RemoveFromString(line, "standard: ");

				if (value.empty())
				{
					KalaMakeCore::PrintError("Failed to compile project because no standard value was passed!");

					return{};
				}

				if (!IsSupportedStandard(value)) return{};

				standard = value;
			}
			else if (IsCompiler(line))
			{
				if (!compiler.empty())
				{
					KalaMakeCore::PrintError("Failed to compile project because more than one compiler line was passed!");

					return{};
				}

				string value = RemoveFromString(line, "compiler: ");

				if (value.empty())
				{
					KalaMakeCore::PrintError("Failed to compile project because no compiler value was passed!");

					return{};
				}

				if (!IsSupportedCompiler(value)) return{};

				if (value == "cl")
				{
					if (exists(cl_ide_bat_2026))        foundCLPath = cl_ide_bat_2026;
					else if (exists(cl_build_bat_2026)) foundCLPath = cl_build_bat_2026;
					else if (exists(cl_ide_bat_2022))   foundCLPath = cl_ide_bat_2022;
					else if (exists(cl_build_bat_2022)) foundCLPath = cl_build_bat_2022;
					else
					{
						KalaMakeCore::PrintError("Failed to compile project because no 'vcvars64.bat' for cl compiler was found!");

						return{};
					}
				}

				compiler = value;
			}
			else if (IsSource(line))
			{
				if (!sources.empty())
				{
					KalaMakeCore::PrintError("Failed to compile project because more than one sources line was passed!");

					return{};
				}

				string value = RemoveFromString(line, "sources: ");

				if (value.empty())
				{
					KalaMakeCore::PrintError("Failed to compile project because there were no sources passed!");

					return{};
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
								KalaMakeCore::PrintError(
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
								KalaMakeCore::PrintError(
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
							KalaMakeCore::PrintError(
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

				if (failedSourceSearch) return{};

				//move found sources to sources container
				sources = std::move(foundSources);
			}

			else if (IsBuildPath(line))
			{
				if (!buildPath.empty())
				{
					KalaMakeCore::PrintError("Failed to compile project because more than one build path line was passed!");

					return{};
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
					KalaMakeCore::PrintError("Failed to compile project because build path '" + value + "' could not be resolved!");
				}

				buildPath = value;
			}
			else if (IsObjPath(line))
			{
				if (!objPath.empty())
				{
					KalaMakeCore::PrintError("Failed to compile project because more than one obj path line was passed!");

					return{};
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
					KalaMakeCore::PrintError("Failed to compile project because obj path '" + value + "' could not be resolved!");
				}

				objPath = value;
			}
			else if (IsHeader(line))
			{
				if (!headers.empty())
				{
					KalaMakeCore::PrintError("Failed to compile project because more than one headers line was passed!");

					return{};
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
								KalaMakeCore::PrintError(
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
								KalaMakeCore::PrintError(
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
							KalaMakeCore::PrintError(
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

				if (failedHeaderSearch) return{};

				//move found sources to headers container
				headers = std::move(foundHeaders);
			}
			else if (IsLink(line))
			{
				if (!links.empty())
				{
					KalaMakeCore::PrintError("Failed to compile project because more than one links line was passed!");

					return{};
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
							KalaMakeCore::PrintError("Failed to compile project because reklink path '" + l + "' could not be resolved!");

							failedLinkSearch = true;

							return{};
						}
					}
				}

				if (failedLinkSearch) return{};

				links = std::move(foundLinks);
			}
			else if (IsDebugLink(line))
			{
				if (!debugLinks.empty())
				{
					KalaMakeCore::PrintError("Failed to compile project because more than one debuglinks line was passed!");

					return{};
				}

				string value = RemoveFromString(line, "debuglinks: ");

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
							KalaMakeCore::PrintError("Failed to compile project because deblink path '" + l + "' could not be resolved!");

							failedLinkSearch = true;

							return{};
						}
					}
				}

				if (failedLinkSearch) return{};

				debugLinks = std::move(foundLinks);
			}

			else if (IsWarningLevel(line))
			{
				if (!warningLevel.empty())
				{
					KalaMakeCore::PrintError("Failed to compile project because more than one warning level line was passed!");

					return{};
				}

				string value = RemoveFromString(line, "warninglevel: ");

				//ignore empty warning level field
				if (value.empty()) continue;

				if (!IsSupportedWarningLevel(value)) return{};

				warningLevel = value;
			}
			else if (IsDefine(line))
			{
				if (!defines.empty())
				{
					KalaMakeCore::PrintError("Failed to compile project because more than one defines line was passed!");

					return{};
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
					KalaMakeCore::PrintError("Failed to compile project because more than one extensions line was passed!");

					return{};
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
					KalaMakeCore::PrintError("Failed to compile project because more than one flags line was passed!");

					return{};
				}

				string value = RemoveFromString(line, "flags: ");

				//ignore empty flags field
				if (value.empty()) continue;

				vector<string> tempFlags = SplitString(value, ", ");

				flags = std::move(tempFlags);
			}
			else if (IsDebugFlag(line))
			{
				if (!debugFlags.empty())
				{
					KalaMakeCore::PrintError("Failed to compile project because more than one debugflags line was passed!");

					return{};
				}

				string value = RemoveFromString(line, "debugflags: ");

				//ignore empty flags field
				if (value.empty()) continue;

				vector<string> tempFlags = SplitString(value, ", ");

				debugFlags = std::move(tempFlags);
			}
			else if (IsCustomFlag(line))
			{
				if (!customFlags.empty())
				{
					KalaMakeCore::PrintError("Failed to compile project because more than one customflags line was passed!");

					return{};
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

				if (failedCustomFlagsSearch) return{};

				localCustomFlags = std::move(tempCustomflags);
			}
			else
			{
				KalaMakeCore::PrintError("Failed to compile project because unknown field '" + string(line) + "' was passed to the project file!");

				return{};
			}
		}

		//
		// VERIFY SOURCES, HEADERS AND LINKS
		//

		vector<string> cleanedSources{};
		vector<string> cleanedHeaders{};
		vector<string> cleanedLinks{};
		vector<string> cleanedDebugLinks{};

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
		if (!links.empty())
		{
			for (const auto& l : links)
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

				cleanedLinks.push_back(l);
			}
		}
		if (!debugLinks.empty())
		{
			for (const auto& l : debugLinks)
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

				cleanedDebugLinks.push_back(l);
			}
		}

		//
		// POST-READ EMPTY CHECKS
		//

		if (name.empty())
		{
			KalaMakeCore::PrintError("Failed to compile project because name has no value!");

			return{};
		}
		if (binaryType.empty())
		{
			KalaMakeCore::PrintError("Failed to compile project because binarytype has no value!");

			return{};
		}
		if (standard.empty())
		{
			KalaMakeCore::PrintError("Failed to compile project because standard has no value!");

			return{};
		}
		if (compiler.empty())
		{
			KalaMakeCore::PrintError("Failed to compile project because compiler has no value!");

			return{};
		}
		if (cleanedSources.empty())
		{
			KalaMakeCore::PrintError("Failed to compile project because sources have no value!");

			return{};
		}

		//
		// CLEAN UP VALUES
		//

		if (warningLevel.empty()) warningLevel = "none";
		if (buildPath.empty()) buildPath = path(kmaPath / defaultBuildPath).string();
		if (objPath.empty()) objPath = path(kmaPath / defaultObjPath).string();

		if (ContainsDuplicates(cleanedSources))    RemoveDuplicates(cleanedSources);
		if (ContainsDuplicates(cleanedHeaders))    RemoveDuplicates(cleanedHeaders);
		if (ContainsDuplicates(cleanedLinks))      RemoveDuplicates(cleanedLinks);
		if (ContainsDuplicates(cleanedDebugLinks)) RemoveDuplicates(cleanedDebugLinks);
		if (ContainsDuplicates(defines))           RemoveDuplicates(defines);
		if (ContainsDuplicates(extensions))        RemoveDuplicates(extensions);
		if (ContainsDuplicates(flags))             RemoveDuplicates(flags);
		if (ContainsDuplicates(debugFlags))        RemoveDuplicates(debugFlags);
		if (ContainsDuplicates(localCustomFlags))  RemoveDuplicates(localCustomFlags);

		for (auto& f : flags)
		{
			if (compiler == "clang-cl"
				|| compiler == "cl")
			{
				if (!f.starts_with('/')) f = "/" + f;
			}
			else if (compiler == "clang"
				|| compiler == "clang++"
				|| compiler == "gcc"
				|| compiler == "g++")
			{
				if (!f.starts_with('-')) f = "-" + f;
			}
		}
		for (auto& f : debugFlags)
		{
			if (compiler == "clang-cl"
				|| compiler == "cl")
			{
				if (!f.starts_with('/')) f = "/" + f;
			}
			else if (compiler == "clang"
				|| compiler == "clang++"
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
			binaryType,
			standard,
			buildPath))
		{
			return{};
		}

		if (!IsSupportedObjPath(
			compiler,
			standard,
			objPath))
		{
			return{};
		}

		//
		// PRINT VALUES
		//

		ostringstream oss{};

		oss << "Parsed data passed to KalaMake:\n\n"

			<< "name: " << name << "\n"
			<< "binary type: " << binaryType << "\n"
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
		if (!cleanedLinks.empty())
		{
			oss << "links:\n";

			for (const auto& l : cleanedLinks)
			{
				oss << "  " << l << "\n";
			}
		}
		if (!cleanedDebugLinks.empty())
		{
			oss << "debug links:\n";

			for (const auto& l : cleanedDebugLinks)
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

		if (!flags.empty())
		{
			oss << "flags:\n";

			for (const auto& f : flags)
			{
				oss << "  " << f << "\n";
			}
		}
		if (!debugFlags.empty())
		{
			oss << "debug flags:\n";

			for (const auto& f : debugFlags)
			{
				oss << "  " << f << "\n";
			}
		}
		if (!customFlags.empty())
		{
			oss << "custom flags:\n";

			for (const auto& cf : localCustomFlags)
			{
				oss << "  " << cf << "\n";
			}
		}

		Log::Print(
			oss.str(),
			"KALAMAKE",
			LogType::LOG_INFO);

		Log::Print("==========================================================================================\n");

		BinaryType bType{};
		bool canConvertToBType = StringToEnum(binaryType, binaryTypes, bType);
		if (!canConvertToBType)
		{
			KalaMakeCore::PrintError("Failed to compile project because binary type couldn't be converted to enum!");

			return {};
		}

		WarningLevel wLevel{};
		bool canConvertToWLevel = StringToEnum(warningLevel, warningLevels, wLevel);
		if (!canConvertToWLevel)
		{
			KalaMakeCore::PrintError("Failed to compile project because warning level couldn't be converted to enum!");

			return {};
		}

		vector<CustomFlag> cFlags{};
		for (const string& f : localCustomFlags)
		{
			CustomFlag cFlag{};
			bool canConvertToCFlag = StringToEnum(f, customFlags, cFlag);
			if (!canConvertToCFlag)
			{
				KalaMakeCore::PrintError("Failed to compile project because warning level couldn't be converted to enum!");

				return {};
			}

			cFlags.push_back(cFlag);
		}

		return 
		{
			.name = name,
			.binaryType = bType,
			.standard = standard,
			.compiler = compiler,
			.sources = std::move(cleanedSources),

			.buildPath = buildPath,
			.objPath = objPath,
			.headers = std::move(cleanedHeaders),
			.links = std::move(cleanedLinks),
			.debuglinks = std::move(cleanedDebugLinks),

			.warningLevel = wLevel,
			.defines = std::move(defines),
			.extensions = std::move(extensions),

			.flags = std::move(flags),
			.debugflags = std::move(debugFlags),
			.customFlags = std::move(cFlags)
		};
	}

	bool KalaMakeCore::IsActionType(string_view value)
	{
		return ContainsValue(actionTypes, value);
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

	const path& KalaMakeCore::GetFoundCLPath() { return foundCLPath; }

    void KalaMakeCore::PrintError(string_view message)
	{
		Log::Print(
			message,
			"KALAMAKE",
			LogType::LOG_ERROR,
			2);
	}
}