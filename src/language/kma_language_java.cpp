//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include <algorithm>
#include <vector>
#include <filesystem>

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

using KalaMake::Core::KalaMakeCore;
using KalaMake::Language::GlobalData;
using KalaMake::Core::BinaryType;
using KalaMake::Core::CompilerLauncherType;
using KalaMake::Core::TargetType;
using KalaMake::Core::BuildType;
using KalaMake::Core::WarningLevel;
using KalaMake::Core::CustomFlag;
using KalaMake::Core::CompileCommand;
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
using std::filesystem::is_empty;

using u16 = uint16_t;

constexpr string_view classFolderName = "class";

static vector<CompileCommand> commands{};

static file_time_type kmakeTime{};
static file_time_type jarTime{};
static path mainJava{};
static path mainClass{};

static void PreCheck(GlobalData& globalData);

static void Compile_Final(const GlobalData& globalData);

namespace KalaMake::Language
{
	void LanguageCore::Compile_Java(GlobalData& globalData)
	{
		commands.clear();

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
    if (!globalData.targetProfile.links.empty())
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_JAVA",
			"Field 'links' is not supported in Java!");
    }
    if (globalData.targetProfile.warningLevel != WarningLevel::W_INVALID)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_JAVA",
			"Field 'warninglevel' is not supported in Java!");
    }
    if (!globalData.targetProfile.defines.empty())
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_JAVA",
			"Field 'defines' is not supported in Java!");
    }
    if (!globalData.targetProfile.linkFlags.empty())
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_JAVA",
			"Field 'linkflags' is not supported in Java!");
    }

	if (ContainsValue(
			globalData.targetProfile.customFlags, 
			CustomFlag::F_EXPORT_COMPILE_COMMANDS))
	{
        KalaMakeCore::CloseOnError(
			"LANGUAGE_JAVA",
			"Custom flag 'export-compile-commands' is not supported in Java!");
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
			foundMain = true;
			mainJava = target;
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
					for (const auto& c : directory_iterator(classDir))
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
			for (const auto& c : directory_iterator(classDir))
			{
				compiledClasses.push_back(c);

				if (c.path().stem() == "Main"
					|| c.path().stem() == "main")
				{
					mainClass = c.path();
				}
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

			command += " --main-class " + mainClass.stem().string();

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