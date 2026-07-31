//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include <algorithm>
#include <vector>
#include <filesystem>
#include <fstream>

#include "log_utils.hpp"

#include "language/kma_language.hpp"
#include "core/kma_core.hpp"
#include "core/kma_generate.hpp"

using KalaHeaders::KalaCore::ContainsValue;
using KalaHeaders::KalaCore::RemoveDuplicates;

using KalaHeaders::KalaLog::Log;
using KalaHeaders::KalaLog::LogType;

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
using std::filesystem::is_empty;
using std::filesystem::is_directory;
using std::ifstream;

constexpr string_view tempFolderName = "temp";

static path mainPython{};

static void PreCheck(GlobalData& globalData);

static void Compile_Final(const GlobalData& globalData);

static void GenerateSteps(const GlobalData& globalData)
{
    bool canGenerateVSCodeSln = ContainsValue(globalData.targetProfile.customFlags, CustomFlag::F_EXPORT_VSCODE_SLN);

	if (canGenerateVSCodeSln)
	{
		VSCode_Launch launch
		{
			.name = globalData.targetProfile.profileName,
			.type = "debugpy",
			.program = "${workspaceFolder}/" + relative(mainPython, current_path()).string()
		};

		VSCode_Task task
		{
			.label = globalData.targetProfile.profileName,
			.projectFile = globalData.projectFile.string()
		};

		Generate::GenerateVSCodeSolution(
			globalData.targetProfile.binaryType == BinaryType::B_EXECUTABLE,
			launch,
			task);
	}

	if (canGenerateVSCodeSln)
	{
		Log::Print("===========================================================================\n");
	}
}

namespace KalaMake::Language
{
	void LanguageCore::Compile_Python(GlobalData& globalData)
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
			"LANGUAGE_PYTHON",
			"Python only supports executables!");
    }
    if (globalData.targetProfile.compilerLauncher != CompilerLauncherType::C_INVALID)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_PYTHON",
			"Field 'compilerlauncher' is not supported in Python!");
    }
    if (globalData.targetProfile.standard != StandardType::S_INVALID)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_PYTHON",
			"Field 'standard' is not supported in Python!");
    }
    if (globalData.targetProfile.targetType != TargetType::T_INVALID)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_PYTHON",
			"Field 'targettype' is not supported in Python!");
    }
    if (globalData.targetProfile.buildType != BuildType::B_INVALID)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_PYTHON",
			"Field 'buildtype' is not supported in Python!");
    }
    if (!globalData.targetProfile.headers.empty())
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_PYTHON",
			"Field 'headers' is not supported in Python!");
    }
    if (!globalData.targetProfile.links.empty())
	{
		KalaMakeCore::CloseOnError(
			"LANGUAGE_PYTHON",
			"Field 'links' is not supported in Python!");
	}
    if (globalData.targetProfile.warningLevel != WarningLevel::W_INVALID)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_PYTHON",
			"Field 'warninglevel' is not supported in Python!");
    }
    if (!globalData.targetProfile.linkFlags.empty())
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_PYTHON",
			"Field 'linkflags' is not supported in Python!");
    }

    if (!globalData.targetProfile.defines.empty())
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_PYTHON",
			"Field 'defines' is not supported in Python!");
    }
    if (!globalData.targetProfile.links.empty())
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_PYTHON",
			"Field 'links' is not supported in Python!");
    }
    if (globalData.targetProfile.jobs != 0)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_PYTHON",
			"Field 'jobs' is not supported in Python!");
    }

	if (ContainsValue(
		globalData.targetProfile.customFlags, 
		CustomFlag::F_EXPORT_COMPILE_COMMANDS))
	{
        KalaMakeCore::CloseOnError(
		    "LANGUAGE_PYTHON",
		    "Custom flag 'export-compile-commands' is not supported in Python!");
	}
	if (ContainsValue(
		globalData.targetProfile.customFlags, 
		CustomFlag::F_WARNINGS_AS_ERRORS))
	{
        KalaMakeCore::CloseOnError(
			"LANGUAGE_PYTHON",
			"Custom flag 'warnings-as-errors' is not supported in Python!");
	}
    if (ContainsValue(
		globalData.targetProfile.customFlags, 
		CustomFlag::F_MSVC_STATIC_RUNTIME))
	{
        KalaMakeCore::CloseOnError(
			"LANGUAGE_PYTHON",
			"Custom flag 'msvc-static-runtime' is not supported in Python!");
	}
	if (ContainsValue(
		globalData.targetProfile.customFlags, 
		CustomFlag::F_GENERATE_SYMBOLS))
	{
        KalaMakeCore::CloseOnError(
			"LANGUAGE_PYTHON",
			"Custom flag 'generate-symbols' is not supported in Python!");
	}
	if (ContainsValue(
		globalData.targetProfile.customFlags, 
		CustomFlag::F_NO_CONSOLE))
	{
        KalaMakeCore::CloseOnError(
			"LANGUAGE_PYTHON",
			"Custom flag 'no-console' is not supported in Java!");
	}
	if (ContainsValue(
		globalData.targetProfile.customFlags, 
		CustomFlag::F_PACKAGE_JAR))
	{
        KalaMakeCore::CloseOnError(
			"LANGUAGE_PYTHON",
			"Custom flag 'package-jar' is not supported in Python!");
	}
	if (ContainsValue(
		globalData.targetProfile.customFlags, 
		CustomFlag::F_JAVA_WIN_CONSOLE))
	{
        KalaMakeCore::CloseOnError(
			"LANGUAGE_PYTHON",
			"Custom flag 'java-win-console' is not supported in Python!");
	}
	if (ContainsValue(
		globalData.targetProfile.customFlags, 
		CustomFlag::F_EXPORT_JAVA_SLN))
	{
        KalaMakeCore::CloseOnError(
			"LANGUAGE_PYTHON",
			"Custom flag 'export-java-sln' is not supported in Python!");
	}

    //
	// FILTER OUT BAD SOURCE FILES 
	//

	for (const auto& p : globalData.targetProfile.sources)
	{
		if ((p.stem() == "Main"
			|| p.stem() == "main"))
		{
			if (!mainPython.empty())
			{
                KalaMakeCore::CloseOnError(
				    "LANGUAGE_PYTHON",
				    "Cannot have more than one main Python script! Please ensure you only have one Main.py or main.py script, and not both.");
			}

            if (is_empty(p))
            {
                KalaMakeCore::CloseOnError(
				    "LANGUAGE_PYTHON",
				    "Main Python script was empty!");
            }

            mainPython = p;
		}
	}

    if (mainPython.empty())
    {
        KalaMakeCore::CloseOnError(
            "LANGUAGE_PYTHON",
            "Did not find main Python script! Please ensure Main.py or main.py is added to sources.");
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

			return target.extension() != ".py";
		};

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
						"LANGUAGE_PYTHON",
						"Cannot ignore target '" + target.string() + "' if it hasn't already been added to sources list!");
				}
			}

			exclusions.push_back(withoutExclamation);

			Log::Print(
				"Ignoring excluded source script path '" + (*itMatch).string() + "'.",
				"LANGUAGE_PYTHON",
				LogType::LOG_INFO);

			foundInvalid = true;
			continue;
		}
		if (should_remove(target))
		{
			Log::Print(
				"Ignoring invalid source script path '" + target.string() + "'",
				"LANGUAGE_PYTHON",
				LogType::LOG_INFO);

			foundInvalid = true;
			continue;
		}

		path canonicalPath = weakly_canonical(target);
		if (!exists(canonicalPath))
		{
			Log::Print(
				"Ignoring non-existing source script path '" + target.string() + "'",
				"LANGUAGE_PYTHON",
				LogType::LOG_INFO);

			foundInvalid = true;
			continue;
		}
		if (!is_regular_file(canonicalPath))
		{
			Log::Print(
				"Ignoring non-file source script path '" + target.string() + "'",
				"LANGUAGE_PYTHON",
				LogType::LOG_INFO);

			foundInvalid = true;
			continue;
		}

		finalSources.push_back(target);
	}

	if (finalSources.empty())
	{
		KalaMakeCore::CloseOnError(
			"LANGUAGE_PYTHON",
			"No sources were remaining after cleaning source scripts list!");
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
			"LANGUAGE_PYTHON",
			LogType::LOG_INFO);

		for (const auto& a : globalData.targetProfile.preBuildActions)
		{
			Log::Print("\naction: " + a);

			if (system(a.c_str()) != 0)
			{
				KalaMakeCore::CloseOnError(
					"LANGUAGE_PYTHON",
					"Failed to run pre build action '" + a + "'!");
			}
		}

		Log::Print(" ");

		Log::Print(
			"Finished all pre build actions!",
			"LANGUAGE_PYTHON",
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

    auto check_pyinstaller = []() -> void
        {
            string command = "pyinstaller --version";

#ifdef _WIN32
            command += " > NUL 2>&1";
#else
            command += " > /dev/null 2>&1";
#endif

            if (system(command.c_str()) != 0)
            {
                KalaMakeCore::CloseOnError(
                    "LANGUAGE_PYTHON",
                    "Cannot compile because pyinstaller was not found!");
            }
        };

	auto compile = [&globalData]() -> void
        {
            string command{};

            //set compiler

            command = "pyinstaller";

            //set main python script

            command += " \"" + mainPython.string() + "\"";

            //set name

            command += " --name " + globalData.targetProfile.binaryName;

            //set compile flags

            vector<string> finalFlags = globalData.targetProfile.compileFlags;

            if (ContainsValue(globalData.targetProfile.customFlags, CustomFlag::F_PYTHON_ONE_FILE))
            {
                finalFlags.push_back("--onefile");
            }

            for (auto& f : finalFlags)
            {
                if (!f.starts_with("--")) f = "--" + f;
            }
            RemoveDuplicates(finalFlags);
            for (const auto& f : finalFlags) command += " " + f;

            //set build dir

            command += " --distpath \"" + globalData.targetProfile.buildPath.string() + "\"";

            //set spec file dir

            command += " --specpath \"" + globalData.targetProfile.buildPath.string() + "\"";

            //set temp file dir

            command += " --workpath \"" + (globalData.targetProfile.buildPath / string(tempFolderName)).string() + "\"";
        
            //compile

            bool isAnyNewer{};

            file_time_type kmakeTime = last_write_time(globalData.projectFile);
            file_time_type buildTime = last_write_time(globalData.targetProfile.buildPath);

            for (const auto& f : globalData.targetProfile.sources)
            {
                file_time_type fileTime = last_write_time(f);

                if (kmakeTime > buildTime
                    || fileTime > buildTime)
                {
                    isAnyNewer = true;
                    break;
                }
            }

            if (isAnyNewer
                || is_empty(globalData.targetProfile.buildPath))
            {
                //always add noconfirm for folder and one-file to skip all confirmations
                command += " --noconfirm";

				Log::Print(
					"Starting to compile via '" + command + "'.",
					"LANGUAGE_PYTHON",
					LogType::LOG_INFO);

                Log::Print(" ");

                if (system(command.c_str()) != 0)
                {
					KalaMakeCore::CloseOnError(
						"LANGUAGE_PYTHON",
						"Failed to compile '" + globalData.targetProfile.buildPath.string() + "'!");
                }

				Log::Print(
					"Finished compiling to output '" + globalData.targetProfile.buildPath.string() + "'!",
					"LANGUAGE_PYTHON",
					LogType::LOG_SUCCESS);
            }
            else
            {
				Log::Print(
					"Skipping compiling to output '" + globalData.targetProfile.buildPath.string() + "' because there are no new source files.",
					"LANGUAGE_PYTHON",
					LogType::LOG_INFO);
            }
        };

    check_pyinstaller();
    compile();

	//
	// POST BUILD ACTIONS
	//

	if (!globalData.targetProfile.postBuildActions.empty())
	{
		Log::Print("\n===========================================================================\n");

		Log::Print(
			"Starting to run post build actions.",
			"LANGUAGE_PYTHON",
			LogType::LOG_INFO);

		for (const auto& a : globalData.targetProfile.postBuildActions)
		{
			Log::Print("\naction: " + a);

			if (system(a.c_str()) != 0)
			{
				KalaMakeCore::CloseOnError(
					"LANGUAGE_PYTHON",
					"Failed to run post build action '" + a + "'!");
			}
		}

		Log::Print(" ");

		Log::Print(
			"Finished all post build actions!",
			"LANGUAGE_PYTHON",
			LogType::LOG_SUCCESS);
	}
}
