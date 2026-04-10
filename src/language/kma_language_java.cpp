//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include <algorithm>
#include <vector>
#include <filesystem>
#include <fstream>

#include "log_utils.hpp"
#include "file_utils.hpp"

#include "language/kma_language.hpp"
#include "core/kma_core.hpp"
#include "core/kma_generate.hpp"

using KalaHeaders::KalaCore::EnumToString;
using KalaHeaders::KalaCore::ContainsValue;
using KalaHeaders::KalaCore::RemoveDuplicates;

using KalaHeaders::KalaLog::Log;
using KalaHeaders::KalaLog::LogType;

using KalaHeaders::KalaFile::CreateNewDirectory;
using KalaHeaders::KalaFile::DeletePath;
using KalaHeaders::KalaFile::CopyPath;

using KalaMake::Core::KalaMakeCore;
using KalaMake::Language::GlobalData;
using KalaMake::Core::BinaryType;
using KalaMake::Core::CompilerLauncherType;
using KalaMake::Core::StandardType;
using KalaMake::Core::TargetType;
using KalaMake::Core::BuildType;
using KalaMake::Core::WarningLevel;
using KalaMake::Core::CustomFlag;
using KalaMake::Core::Generate;
using KalaMake::Core::JavaClassPath;
using KalaMake::Core::VSCode_Launch;
using KalaMake::Core::VSCode_Task;

using std::string;
using std::string_view;
using std::find;
using std::vector;
using std::filesystem::path;
using std::filesystem::current_path;
using std::filesystem::file_time_type;
using std::filesystem::last_write_time;
using std::filesystem::directory_iterator;
using std::filesystem::recursive_directory_iterator;
using std::filesystem::is_empty;
using std::filesystem::is_directory;
using std::ifstream;
using std::getline;

using u16 = uint16_t;

constexpr string_view classFolderName = "class";

static file_time_type kmakeTime{};
static file_time_type jarTime{};

static path mainJava{};
static path mainClass{};
static string mainClassValue{};

static void PreCheck(GlobalData& globalData);

static void Compile_Final(const GlobalData& globalData);

static void GenerateSteps(const GlobalData& globalData)
{
	bool isMSVC =
#ifdef _WIN32
		true;
#else
		false;
#endif

	bool canGenerateExportSln = ContainsValue(globalData.targetProfile.customFlags, CustomFlag::F_EXPORT_JAVA_SLN);
	bool canGenerateVSCodeSln = ContainsValue(globalData.targetProfile.customFlags, CustomFlag::F_EXPORT_VSCODE_SLN);

	if (canGenerateExportSln)
	{
		string package = mainClassValue.find('.') != string::npos
			? mainClassValue.substr(0, mainClassValue.rfind('.'))
			: "";
		size_t depth = count(package.begin(), package.end(), '.') + 1;

		path srcRoot = mainJava.parent_path();
		for (size_t i = 0; i < depth; i++)
		{
			srcRoot = srcRoot.parent_path();
		}

		JavaClassPath jcp
		{
			.binaryName = globalData.targetProfile.binaryName,
			.srcDir = relative(srcRoot, current_path()),
			.outputDir = relative(globalData.targetProfile.buildPath, current_path())
		};

		Generate::GenerateJavaClassPath(jcp);

		if (canGenerateVSCodeSln) Log::Print(" ");
	}

	if (canGenerateVSCodeSln)
	{
		path relativeBuildPath = relative(globalData.targetProfile.buildPath, current_path());
		path programPath = relativeBuildPath / globalData.targetProfile.binaryName;

		VSCode_Launch launch
		{
			.name = globalData.targetProfile.profileName,
			.type = "java",
			.program = "${workspaceFolder}/" + programPath.string(),
			.mainClass = mainClassValue
		};

		VSCode_Task task
		{
			.label = globalData.targetProfile.profileName,
			.projectFile = globalData.projectFile.string()
		};

		Generate::GenerateVSCodeSolution(
			isMSVC,
			globalData.targetProfile.binaryType == BinaryType::B_EXECUTABLE,
			launch,
			task);
	}

	if (canGenerateExportSln
		|| canGenerateVSCodeSln)
	{
		Log::Print("===========================================================================\n");
	}
}

namespace KalaMake::Language
{
	void LanguageCore::Compile_Java(GlobalData& globalData)
	{
		PreCheck(globalData);
		Compile_Final(globalData);
	}
}

void PreCheck(GlobalData& globalData)
{
    //
    // ENSURE UNSUPPORTED FIELDS ARE NOT USED
    //

    if (globalData.targetProfile.binaryType != BinaryType::B_EXECUTABLE)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_JAVA",
			"Java only supports executables!");
    }
    if (globalData.targetProfile.compilerLauncher != CompilerLauncherType::C_INVALID)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_JAVA",
			"Field 'compilerlauncher' is not supported in Java!");
    }
    if (globalData.targetProfile.standard == StandardType::S_INVALID)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_JAVA",
			"Field 'standard' must be assigned in Java!");
    }
    if (globalData.targetProfile.targetType != TargetType::T_INVALID)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_JAVA",
			"Field 'targettype' is not supported in Java!");
    }
    if (!globalData.targetProfile.headers.empty())
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_JAVA",
			"Field 'headers' is not supported in Java!");
    }
    if (globalData.targetProfile.warningLevel != WarningLevel::W_INVALID)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_JAVA",
			"Field 'warninglevel' is not supported in Java!");
    }
    if (!globalData.targetProfile.linkFlags.empty())
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_JAVA",
			"Field 'linkflags' is not supported in Java!");
    }

    if (globalData.targetProfile.jobs != 0)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_JAVA",
			"Field 'jobs' is not supported in Java!");
    }

	if (ContainsValue(
			globalData.targetProfile.customFlags, 
			CustomFlag::F_EXPORT_COMPILE_COMMANDS))
	{
        KalaMakeCore::CloseOnError(
			"LANGUAGE_JAVA",
			"Custom flag 'export-compile-commands' is not supported in Java!");
	}

	if (!globalData.targetProfile.links.empty())
	{
		if (globalData.targetProfile.links.size() > 1)
		{
			KalaMakeCore::CloseOnError(
				"LANGUAGE_JAVA",
				"Field 'links' only allows one value in Java!");
		}
		if (!is_directory(globalData.targetProfile.links[0]))
		{
			KalaMakeCore::CloseOnError(
				"LANGUAGE_JAVA",
				"Field 'links' value must be a directory!");
		}

		bool foundJar{};
		for (const auto& f : directory_iterator(globalData.targetProfile.links[0]))
		{
			if (is_regular_file(path(f))
				&& path(f).extension() == ".jar")
			{
				foundJar = true;
				break;
			}
		}
		if (!foundJar)
		{
			KalaMakeCore::CloseOnError(
				"LANGUAGE_JAVA",
				"Field 'links' directory did not contain any jar files!");
		}
	}

    //
	// FILTER OUT BAD SOURCE FILES 
	//

	for (const auto& j : globalData.targetProfile.sources)
	{
		if ((j.stem() == "Main"
			|| j.stem() == "main")
			&& is_empty(j))
		{
			KalaMakeCore::CloseOnError(
				"LANGUAGE_JAVA",
				"Main.java was empty!");
		}
	}

	auto should_exclude = [](const path& target) -> bool
		{
			return (target.string().starts_with('!'));
		};

	auto should_remove = [](
		const path& target) -> bool
		{
			if (!exists(target)
				|| is_directory(target)
				|| !is_regular_file(target)
				|| !target.has_extension())
			{
				return true;
			}

			return target.extension() != ".java";
		};

	bool foundMain{};
	bool foundInvalid{};
	vector<path>& sources = globalData.targetProfile.sources;
	vector<path> exclusions{};
	vector<path> finalSources{};

	for (const auto& target : sources)
	{
		if (should_exclude(target))
		{
			string withoutExclamation = target.string().substr(1);

			auto itMatch = find(
				sources.begin(),
				sources.end(),
				path((withoutExclamation)));

			if (itMatch == sources.end())
			{
				withoutExclamation = (current_path() / withoutExclamation).string();

				itMatch = find(
					sources.begin(),
					sources.end(),
					path((withoutExclamation)));

				if (itMatch == sources.end())
				{
					KalaMakeCore::CloseOnError(
						"LANGUAGE_JAVA",
						"Cannot ignore target '" + target.string() + "' if it hasn't already been added to sources list!");
				}
			}

			exclusions.push_back(withoutExclamation);

			Log::Print(
				"Ignoring excluded source script path '" + (*itMatch).string() + "'.",
				"LANGUAGE_JAVA",
				LogType::LOG_INFO);

			foundInvalid = true;
			continue;
		}
		if (should_remove(target))
		{
			Log::Print(
				"Ignoring invalid source script path '" + target.string() + "'",
				"LANGUAGE_JAVA",
				LogType::LOG_INFO);

			foundInvalid = true;
			continue;
		}

		path canonicalPath = weakly_canonical(target);
		if (!exists(canonicalPath))
		{
			Log::Print(
				"Ignoring non-existing source script path '" + target.string() + "'",
				"LANGUAGE_JAVA",
				LogType::LOG_INFO);

			foundInvalid = true;
			continue;
		}
		if (!is_regular_file(canonicalPath))
		{
			Log::Print(
				"Ignoring non-file source script path '" + target.string() + "'",
				"LANGUAGE_JAVA",
				LogType::LOG_INFO);

			foundInvalid = true;
			continue;
		}
		
		if (target.stem() == "Main"
			|| target.stem() == "main")
		{
			if (foundMain)
			{
				KalaMakeCore::CloseOnError(
					"LANGUAGE_JAVA",
					"Cannot have more than one main Java script! Make sure more than one or both Main.java and main.java don't exist.");
			}

			foundMain = true;
			mainJava = target;

			auto get_package_name = []() -> string
				{
					ifstream file(mainJava);
					if (!file.is_open())
					{
						KalaMakeCore::CloseOnError(
							"LANGUAGE_JAVA",
							"Failed to open main class for package retrieval!");
					}

					string line{};

					while (getline(file, line))
					{
						if (line.rfind("package ", 0) == 0)
						{
							string package = line.substr(8);
							package = package.substr(0, package.find(';'));
							return package + ".";
						}
					}

					return "";
				};

			mainClassValue = get_package_name() + mainJava.stem().string();
		}

		finalSources.push_back(target);
	}

	if (finalSources.empty())
	{
		KalaMakeCore::CloseOnError(
			"LANGUAGE_JAVA",
			"No sources were remaining after cleaning source scripts list!");
	}

	if (!foundMain)
	{
		KalaMakeCore::CloseOnError(
			"LANGUAGE_JAVA",
			"Did not find main java script! There must be one script named Main.java or main.java.");
	}

	if (foundInvalid) 
	{
		if (!exclusions.empty())
		{
			finalSources.erase(
				remove_if(
					finalSources.begin(),
					finalSources.end(),
					[&exclusions](const path& p)
					{
						return find(exclusions.begin(), exclusions.end(), p) != exclusions.end();
					}),
				finalSources.end());
		}

		globalData.targetProfile.sources = std::move(finalSources);

		Log::Print("\n===========================================================================\n");
	}
}

void Compile_Final(const GlobalData& globalData)
{
	//
	// PRE BUILD ACTIONS
	//

	if (!globalData.targetProfile.preBuildActions.empty())
	{
		Log::Print(
			"Starting to run pre build actions.",
			"LANGUAGE_JAVA",
			LogType::LOG_INFO);

		for (const auto& a : globalData.targetProfile.preBuildActions)
		{
			Log::Print("\naction: " + a);

			if (system(a.c_str()) != 0)
			{
				KalaMakeCore::CloseOnError(
					"LANGUAGE_JAVA",
					"Failed to run pre build action '" + a + "'!");
			}
		}

		Log::Print(" ");

		Log::Print(
			"Finished all pre build actions!",
			"LANGUAGE_JAVA",
			LogType::LOG_SUCCESS);

		Log::Print("\n===========================================================================\n");
	}

	//
	// GENERATE STEPS
	//

	GenerateSteps(globalData);

	//
	// COMPILE
	//

	auto compile = [&globalData]() -> vector<path>
		{
			string command{};

			//set compiler

			command = "javac ";

			//set standard

			string_view standard{};
			EnumToString(globalData.targetProfile.standard, KalaMakeCore::GetStandardTypes(), standard);

			command += "--release " + string(standard).erase(0, 4);

			//set module path

			if (!globalData.targetProfile.links.empty())
			{
				command += " --module-path \"" + globalData.targetProfile.links[0].string() + "\"";
			}

			//add modules

			if (!globalData.targetProfile.defines.empty())
			{
				command += " --add-modules ";
				for (const auto& d : globalData.targetProfile.defines)
				{
					command += d + ",";
				}
				command.pop_back();
			}

			//set flags

			vector<string> finalFlags = globalData.targetProfile.compileFlags;

			if (ContainsValue(
				globalData.targetProfile.customFlags, 
				CustomFlag::F_WARNINGS_AS_ERRORS))
			{
				finalFlags.push_back("-Werror");
			}

			switch (globalData.targetProfile.buildType)
			{
			case BuildType::B_DEBUG:
			{
				finalFlags.push_back("-g");
				break;
			}
			case BuildType::B_RELEASE:
			case BuildType::B_MINSIZEREL:
			case BuildType::B_RELDEBUG:
			{
				finalFlags.push_back("-g:none");
				break;
			}
			default: break;
			}

			RemoveDuplicates(finalFlags);

			for (const auto& f : finalFlags)
			{
				command += " " + f;
			}

			//compile

			path classDir = globalData.targetProfile.buildPath / classFolderName;

			if (!exists(classDir))
			{
				string errorMsg = CreateNewDirectory(classDir);
				if (!errorMsg.empty())
				{
					KalaMakeCore::CloseOnError(
						"LANGUAGE_JAVA",
						"Failed to create new class dir for compilation! Reason: " + errorMsg);
				}
			}

			kmakeTime = last_write_time(globalData.projectFile);

			command += " -d \"" + classDir.string() + "\"";

			bool isAnyNewer{};

			if (is_empty(classDir)) isAnyNewer = true;
			else
			{
				for (const auto& j : globalData.targetProfile.sources)
				{
					for (const auto& c : recursive_directory_iterator(classDir))
					{
						if (last_write_time(j) > last_write_time(c)
							|| kmakeTime > last_write_time(c))
						{
							isAnyNewer = true;
							break;
						}
					}
				}
			}

			if (isAnyNewer)
			{
				for (const auto& j : globalData.targetProfile.sources)
				{
					command += " \"" + j.string() + "\"";
				}

				Log::Print(
					"Starting to compile via '" + command + "'.",
					"LANGUAGE_JAVA",
					LogType::LOG_INFO);

				if (system(command.c_str()) != 0)
				{
					KalaMakeCore::CloseOnError(
						"LANGUAGE_JAVA",
						"Failed to compile class files!");
				}
			}
			else
			{
				Log::Print(
					"Skipping compilation of class files because they are newer than their source files.\n",
					"LANGUAGE_JAVA",
					LogType::LOG_INFO);
			}

			vector<path> compiledClasses{};
			for (const auto& c : recursive_directory_iterator(classDir))
			{
				if (is_directory(c)) continue;
				compiledClasses.push_back(c);

				if (c.path().stem() == "Main"
					|| c.path().stem() == "main")
				{
					mainClass = c.path();
				}
			}

			if (mainClass.empty())
			{
				KalaMakeCore::CloseOnError(
					"LANGUAGE_JAVA",
					"Failed to find main class!");
			}

			return compiledClasses;
		};
		
	//
	// CREATE JAR FILE
	//

	auto create_jar = [&globalData](const vector<path>& compiledClasses) -> path
		{
			string command = "jar --create";

			//set jar path

			path jarPath = globalData.targetProfile.buildPath / (globalData.targetProfile.binaryName + ".jar");
			command += " --file \"" + jarPath.string() + "\"";

			//set main class

			command += " --main-class " + mainClassValue;

			//add class file dir

			path classDir = globalData.targetProfile.buildPath / classFolderName;
			command += " -C \"" + classDir.string() + "\" .";

			//create the jar file

			auto needs_jar = [](
				const path& output,
				const vector<path>& classes) -> bool
				{
					if (!exists(output)) return true;

					jarTime = last_write_time(output);

					if (jarTime < kmakeTime) return true;

					for (const auto& c : classes)
					{
						if (!exists(c)
							|| last_write_time(c) > jarTime)
						{
							return true;
						}
					}

					return false;
				};

			Log::Print("===========================================================================\n");

			if (needs_jar(jarPath, compiledClasses))
			{
				Log::Print(
					"Starting to create jar file via '" + command + "'.",
					"LANGUAGE_JAVA",
					LogType::LOG_INFO);

				if (system(command.c_str()) != 0)
				{
					KalaMakeCore::CloseOnError(
						"LANGUAGE_JAVA",
						"Failed to create jar file '" + jarPath.string() + "'!");
				}

				Log::Print(
					"Finished creating jar file '" + jarPath.string() + "'!",
					"LANGUAGE_JAVA",
					LogType::LOG_SUCCESS);
			}
			else
			{
				Log::Print(
					"Skipping creating jar file '" + jarPath.string() + "' because there are no new class files.",
					"LANGUAGE_JAVA",
					LogType::LOG_INFO);
			}

			return jarPath;
		};

	//
	// PACKAGE JAR FILE
	//

	auto package_jar = [&globalData](const path& jarPath) -> void
		{
			string command = "jpackage";
			string jarName = jarPath.filename().string();

			path jarDir = 
				globalData.targetProfile.buildPath 
				/ globalData.targetProfile.binaryName;

			path buildPath = 
#ifdef _WIN32
				globalData.targetProfile.buildPath 
				/ globalData.targetProfile.binaryName
				/ (globalData.targetProfile.binaryName + ".exe");
#else
				globalData.targetProfile.buildPath 
				/ globalData.targetProfile.binaryName 
				/ "bin" 
				/ globalData.targetProfile.binaryName;
#endif

			//set input

			auto needs_copy = [](
				const path& oldFile,
				const path& newFile) -> bool
				{
					return last_write_time(oldFile) > last_write_time(newFile);
				};

			if (!globalData.targetProfile.links.empty())
			{
				vector<path> toBeCopied{};
				vector<path> targetDoesNotContain{};

				for (const auto& oldFile : directory_iterator(globalData.targetProfile.links[0]))
				{
					path of = path(oldFile);
					if (of.stem() == "Main"
						|| of.stem() == "main"
						|| is_directory(of)
						|| (is_regular_file(of)
						&& of.extension() != ".jar"))
					{
						continue;
					}

					bool foundFile{};

					for (const auto& newFile : directory_iterator(jarDir))
					{
						path nf = path(newFile);
						if (nf.stem() == "Main"
							|| nf.stem() == "main")
						{
							continue;
						}

						if (of.stem() == nf.stem())
						{
							foundFile = true;

							if (needs_copy(oldFile, nf))
							{
								toBeCopied.push_back(oldFile);
							}

							break;
						}
					}

					if (!foundFile) targetDoesNotContain.push_back(oldFile);
					foundFile = false;
				}

				for (const auto& f : targetDoesNotContain)
				{
					string errorMsg = CopyPath(f, jarDir / path(f).filename());
					if (!errorMsg.empty())
					{
						KalaMakeCore::CloseOnError(
							"LANGUAGE_JAVA",
							"Failed to copy jar file to target dir! Reason: " + errorMsg);
					}
				}
				for (const auto& f : toBeCopied)
				{
					string errorMsg = CopyPath(f, jarDir / path(f).filename(), true);
					if (!errorMsg.empty())
					{
						KalaMakeCore::CloseOnError(
							"LANGUAGE_JAVA",
							"Failed to overwrite jar file at target dir! Reason: " + errorMsg);
					}
				}
			}

			path inputPath = globalData.targetProfile.buildPath;
			command += " --input \"" + inputPath.string() + "\"";

			//set main jar

			command += " --main-jar \"" + jarName + "\"";

			//set package destination

			command += " --dest \"" + globalData.targetProfile.buildPath.string() + "\"";

			//set package name and type

			command += " --name " + globalData.targetProfile.binaryName + " --type app-image";

			//allow win-console

			if (ContainsValue(
				globalData.targetProfile.customFlags, 
				CustomFlag::F_JAVA_WIN_CONSOLE))
			{
				command += " --win-console";
			}

			//package jar file

			auto needs_package = [](
				const path& jarPath,
				const path& packagePath) -> bool
				{
					if (!exists(packagePath)) return true;

					file_time_type packageTime = last_write_time(packagePath);

					if (packageTime < kmakeTime) return true;

					if (!exists(jarPath)
						|| last_write_time(jarPath) > packageTime)
					{
						return true;
					}

					return false;
				};

			Log::Print("===========================================================================\n");

			if (needs_package(jarPath, buildPath))
			{
				Log::Print(
					"Starting to package jar file via '" + command + "'.",
					"LANGUAGE_JAVA",
					LogType::LOG_INFO);

				path packageDir = globalData.targetProfile.buildPath / globalData.targetProfile.binaryName;
				if (exists(packageDir))
				{
					string errorMsg = DeletePath(packageDir);
					if (!errorMsg.empty())
					{
						KalaMakeCore::CloseOnError(
							"LANGUAGE_JAVA",
							"Failed to delete package dir for repacking! Reason: " + errorMsg);
					}
				}

				if (system(command.c_str()) != 0)
				{
					KalaMakeCore::CloseOnError(
						"LANGUAGE_JAVA",
						"Failed to package jar file '" + jarName + "'!");
				}

				Log::Print(
					"Finished packing jar file to path '" + buildPath.string() + "'!",
					"LANGUAGE_JAVA",
					LogType::LOG_SUCCESS);
			}
			else
			{
				Log::Print(
					"Skipping packing jar file to path '" + buildPath.string() + "' because it is newer than its jar file.",
					"LANGUAGE_JAVA",
					LogType::LOG_INFO);
			}
		};

	//
	// COMPILE, CREATE JAR AND PACKAGE JAR
	//

	vector<path> classFiles = compile();
	path jarPath = create_jar(classFiles);

	if (ContainsValue(
		globalData.targetProfile.customFlags, 
		CustomFlag::F_PACKAGE_JAR))
	{
		package_jar(jarPath);
	}

	//
	// POST BUILD ACTIONS
	//

	if (!globalData.targetProfile.postBuildActions.empty())
	{
		Log::Print("\n===========================================================================\n");

		Log::Print(
			"Starting to run post build actions.",
			"LANGUAGE_JAVA",
			LogType::LOG_INFO);

		for (const auto& a : globalData.targetProfile.postBuildActions)
		{
			Log::Print("\naction: " + a);

			if (system(a.c_str()) != 0)
			{
				KalaMakeCore::CloseOnError(
					"LANGUAGE_JAVA",
					"Failed to run post build action '" + a + "'!");
			}
		}

		Log::Print(" ");

		Log::Print(
			"Finished all post build actions!",
			"LANGUAGE_JAVA",
			LogType::LOG_SUCCESS);
	}
}