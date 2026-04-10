//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include <string>
#include <vector>
#include <filesystem>

#include "language/kma_language.hpp"
#include "core/kma_core.hpp"
#include "core/kma_generate.hpp"

#include "log_utils.hpp"

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
using std::vector;
using std::filesystem::path;
using std::filesystem::current_path;
using std::filesystem::exists;
using std::filesystem::file_time_type;
using std::filesystem::last_write_time;

// zig + linux-gnu
constexpr string_view target_type_zig_linux_gnu = "x86_64-linux-gnu";
// zig + linux-musl
constexpr string_view target_type_zig_linux_musl = "x86_64-linux-musl";
// zig + windows-gnu
constexpr string_view target_type_zig_windows_gnu = "x86_64-windows-gnu";

static path mainZig{};

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

	bool canGenerateVSCodeSln = ContainsValue(globalData.targetProfile.customFlags, CustomFlag::F_EXPORT_VSCODE_SLN);

	if (canGenerateVSCodeSln)
	{
		path relativeBuildPath = relative(globalData.targetProfile.buildPath, current_path());
		path programPath = relativeBuildPath / globalData.targetProfile.binaryName;

		VSCode_Launch launch
		{
			.name = globalData.targetProfile.profileName,
			.type = "lldb",
			.program = "${workspaceFolder}/" + programPath.string()
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

	if (canGenerateVSCodeSln)
	{
		Log::Print("===========================================================================\n");
	}
}

namespace KalaMake::Language
{
	void LanguageCore::Compile_Zig(GlobalData& globalData)
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

    if (globalData.targetProfile.standard != StandardType::S_INVALID)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_ZIG",
			"Field 'standard' is not supported in Zig!");
    }
    if (globalData.targetProfile.compilerLauncher != CompilerLauncherType::C_INVALID)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_ZIG",
			"Field 'compilerlauncher' is not supported in Zig!");
    }
    if (!globalData.targetProfile.headers.empty())
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_ZIG",
			"Field 'headers' is not supported in Zig!");
    }
    if (globalData.targetProfile.warningLevel != WarningLevel::W_INVALID)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_ZIG",
			"Field 'warninglevel' is not supported in Zig!");
    }
    if (!globalData.targetProfile.linkFlags.empty())
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_ZIG",
			"Field 'linkflags' is not supported in Zig!");
    }

    if (!globalData.targetProfile.defines.empty())
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_ZIG",
			"Field 'defines' is not supported in Zig!");
    }
    if (globalData.targetProfile.jobs != 0)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_ZIG",
			"Field 'jobs' is not supported in Zig!");
    }

	if (ContainsValue(
			globalData.targetProfile.customFlags, 
			CustomFlag::F_EXPORT_COMPILE_COMMANDS))
	{
        KalaMakeCore::CloseOnError(
			"LANGUAGE_ZIG",
			"Custom flag 'export-compile-commands' is not supported in Zig!");
	}
	if (ContainsValue(
			globalData.targetProfile.customFlags, 
			CustomFlag::F_WARNINGS_AS_ERRORS))
	{
        KalaMakeCore::CloseOnError(
			"LANGUAGE_ZIG",
			"Custom flag 'warnings-as-errors' is not supported in Zig!");
	}
	if (ContainsValue(
		globalData.targetProfile.customFlags, 
		CustomFlag::F_PACKAGE_JAR))
	{
        KalaMakeCore::CloseOnError(
			"LANGUAGE_ZIG",
			"Custom flag 'package-jar' is not supported in Zig!");
	}
	if (ContainsValue(
		globalData.targetProfile.customFlags, 
		CustomFlag::F_JAVA_WIN_CONSOLE))
	{
        KalaMakeCore::CloseOnError(
			"LANGUAGE_ZIG",
			"Custom flag 'java-win-console' is not supported in Zig!");
	}
	if (ContainsValue(
		globalData.targetProfile.customFlags, 
		CustomFlag::F_EXPORT_JAVA_SLN))
	{
        KalaMakeCore::CloseOnError(
			"LANGUAGE_ZIG",
			"Custom flag 'export-java-sln' is not supported in Zig!");
	}

    //
	// FILTER OUT BAD SOURCE FILES 
	//

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

			return target.extension() != ".zig";
		};

	bool foundInvalid{};
	vector<path>& sources = globalData.targetProfile.sources;
	vector<path> exclusions{};
	vector<path> finalSources{};

	bool foundMain{};

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
						"LANGUAGE_ZIG",
						"Cannot ignore target '" + target.string() + "' if it hasn't already been added to sources list!");
				}
			}

			exclusions.push_back(withoutExclamation);

			Log::Print(
				"Ignoring excluded source script path '" + (*itMatch).string() + "'.",
				"LANGUAGE_ZIG",
				LogType::LOG_INFO);

			foundInvalid = true;
			continue;
		}
		if (should_remove(target))
		{
			Log::Print(
				"Ignoring invalid source script path '" + target.string() + "'",
				"LANGUAGE_ZIG",
				LogType::LOG_INFO);

			foundInvalid = true;
			continue;
		}

		path canonicalPath = weakly_canonical(target);
		if (!exists(canonicalPath))
		{
			Log::Print(
				"Ignoring non-existing source script path '" + target.string() + "'",
				"LANGUAGE_ZIG",
				LogType::LOG_INFO);

			foundInvalid = true;
			continue;
		}
		if (!is_regular_file(canonicalPath))
		{
			Log::Print(
				"Ignoring non-file source script path '" + target.string() + "'",
				"LANGUAGE_ZIG",
				LogType::LOG_INFO);

			foundInvalid = true;
			continue;
		}

		if (target.stem() == "main"
			|| target.stem() == "root")
		{
            if (foundMain)
			{
				KalaMakeCore::CloseOnError(
					"LANGUAGE_ZIG",
					"Cannot have more than one main Zig script! Make sure more than one or both main.zig and root.zig don't exist.");
			}

			foundMain = true;
			mainZig = target;
		}

		finalSources.push_back(target);
	}

	if (finalSources.empty())
	{
		KalaMakeCore::CloseOnError(
			"LANGUAGE_ZIG",
			"No sources were remaining after cleaning source scripts list!");
	}

	if (!foundMain)
	{
		KalaMakeCore::CloseOnError(
			"LANGUAGE_ZIG",
			"Did not find main Zig script! There must be one script named main.zig or root.zig.");
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
			"LANGUAGE_ZIG",
			LogType::LOG_INFO);

		for (const auto& a : globalData.targetProfile.preBuildActions)
		{
			Log::Print("\naction: " + a);

			if (system(a.c_str()) != 0)
			{
				KalaMakeCore::CloseOnError(
					"LANGUAGE_ZIG",
					"Failed to run pre build action '" + a + "'!");
			}
		}

		Log::Print(" ");

		Log::Print(
			"Finished all pre build actions!",
			"LANGUAGE_ZIG",
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

    auto compile = [&globalData]() -> void
        {
            string command{};

            //set compiler

            command = "zig";

            //set binary type

            switch(globalData.targetProfile.binaryType)
            {
            case BinaryType::B_EXECUTABLE:
            {
                command += " build-exe";

#ifdef __linux__
                //add missing libc flag on linux
                command += " -lc";
#endif

                break;
            }
            case BinaryType::B_SHARED:
            {
                command += " build-lib -dynamic";

#ifdef __linux__
                //use the correct soname for linux
                command += " --name " + globalData.targetProfile.binaryName;

                //add missing libc flag on linux
                command += " -lc";
#endif

                break;
            }
            case BinaryType::B_STATIC:
            {
                command += " build-lib";
                break;
            }
            default: break;
            }

            //set flags

            vector<string> finalFlags = globalData.targetProfile.compileFlags;

            switch (globalData.targetProfile.buildType)
            {
            case BuildType::B_DEBUG:
            {
                finalFlags.push_back("-O Debug");
                break;
            }
            case BuildType::B_RELEASE:
            {
                finalFlags.push_back("-O ReleaseFast");
				finalFlags.push_back("-fstrip");
				break;
            }
            case BuildType::B_MINSIZEREL:
            {
                finalFlags.push_back("-O ReleaseSmall");
				finalFlags.push_back("-fstrip");
				break;
            }
            case BuildType::B_RELDEBUG:
            {
                finalFlags.push_back("-O ReleaseSafe");
                break;
            }
            default: break;
            }

            RemoveDuplicates(finalFlags);

			for (const auto& f : finalFlags)
			{
				command += " " + f;
			}

            //set target type

            switch(globalData.targetProfile.targetType)
            {
            case TargetType::T_LINUX_GNU:
            {
                command += " -target " + string(target_type_zig_linux_gnu);
                break;
            }
            case TargetType::T_LINUX_MUSL:
            {
                command += " -target " + string(target_type_zig_linux_musl);
                break;
            }
            case TargetType::T_WINDOWS_GNU:
            {
                command += " -target " + string(target_type_zig_windows_gnu);
                break;
            }
            default: break;
            }

            //set main source

            command += " \"" + mainZig.string() + "\"";

            //set links

            for (const auto& l : globalData.targetProfile.links)
            {
                if (exists(l)) command += " \"" + path(l).string() + "\"";
                else             command += " -l" + path(l).string();
            }

            //set output data

            path buildPath = globalData.targetProfile.buildPath;
			string binaryName = globalData.targetProfile.binaryName;

			bool isOnLinux{};
#ifdef __linux__
			isOnLinux = true;
#endif

            string extension{};
			if (globalData.targetProfile.binaryType == BinaryType::B_EXECUTABLE)
			{
				if ((isOnLinux
					&& globalData.targetProfile.targetType == TargetType::T_INVALID)
					|| globalData.targetProfile.targetType == TargetType::T_LINUX_GNU
					|| globalData.targetProfile.targetType == TargetType::T_LINUX_MUSL)
				{
					
					if (binaryName.ends_with(".exe")) binaryName.resize(binaryName.size() - 4);
				}
				else
				{
					if (!binaryName.ends_with(".exe")) extension = ".exe";
				}
			}
			else if (globalData.targetProfile.binaryType == BinaryType::B_SHARED)
			{
				if ((isOnLinux
					&& globalData.targetProfile.targetType == TargetType::T_INVALID)
					|| globalData.targetProfile.targetType == TargetType::T_LINUX_GNU
					|| globalData.targetProfile.targetType == TargetType::T_LINUX_MUSL)
				{
					if (!binaryName.ends_with(".so")) extension = ".so";
                    if (!binaryName.starts_with("lib")) binaryName = "lib" + binaryName;
				}
				else
				{
					if (!binaryName.ends_with(".dll")) extension = ".dll";
				}
			}
			else if (globalData.targetProfile.binaryType == BinaryType::B_STATIC)
			{
				if ((isOnLinux
					&& globalData.targetProfile.targetType == TargetType::T_INVALID)
					|| globalData.targetProfile.targetType == TargetType::T_LINUX_GNU
					|| globalData.targetProfile.targetType == TargetType::T_LINUX_MUSL
					|| globalData.targetProfile.targetType == TargetType::T_WINDOWS_GNU)
				{
					if (!binaryName.ends_with(".a")) extension = ".a";
                    if (!binaryName.starts_with("lib")) binaryName = "lib" + binaryName;
				}
				else
				{
					if (!binaryName.ends_with(".lib")) extension = ".lib";
				}
			}

			path outputPath = path(buildPath / string(binaryName + extension));

            //set output path

            command += " -femit-bin=\"" + outputPath.string() + "\"";

            //compile

            file_time_type kmakeTime = last_write_time(globalData.projectFile);

            bool isAnyNewer{};

            for (const auto& s : globalData.targetProfile.sources)
            {
                if (kmakeTime > last_write_time(s))
                {
                    isAnyNewer = true;
                    break;
                }
            }

            if (!isAnyNewer)
            {
                for (const auto& l : globalData.targetProfile.links)
                {
                    if (exists(l)
                        && kmakeTime > last_write_time(l))
                    {
                        isAnyNewer = true;
                        break;
                    }
                }
            }

            if (!isAnyNewer
                && exists(outputPath))
            {
                file_time_type outputWriteTime = last_write_time(outputPath);

                for (const auto& s : globalData.targetProfile.sources)
                {
                    if (outputWriteTime < last_write_time(s))
                    {
                        isAnyNewer = true;
                        break;
                    }
                }

                if (!isAnyNewer)
                {
                    for (const auto& l : globalData.targetProfile.links)
                    {
                        if (exists(l)
                            && outputWriteTime < last_write_time(l))
                        {
                            isAnyNewer = true;
                            break;
                        }
                    }
                }
            }

            if (isAnyNewer
                || !exists(outputPath))
            {
				Log::Print(
					"Starting to compile via '" + command + "'.",
					"LANGUAGE_ZIG",
					LogType::LOG_INFO);

                Log::Print(" ");

                if (system(command.c_str()) != 0)
                {
					KalaMakeCore::CloseOnError(
						"LANGUAGE_ZIG",
						"Failed to compile '" + outputPath.string() + "'!");
                }

				Log::Print(
					"Finished compiling to output '" + outputPath.string() + "'!",
					"LANGUAGE_ZIG",
					LogType::LOG_SUCCESS);
            }
            else
            {
				Log::Print(
					"Skipping linking of output '" + outputPath.string() + "' because there are no new source or link files.",
					"LANGUAGE_ZIG",
					LogType::LOG_INFO);
            }
        };

    compile();

    //
	// POST BUILD ACTIONS
	//

	if (!globalData.targetProfile.postBuildActions.empty())
	{
		Log::Print("\n===========================================================================\n");

		Log::Print(
			"Starting to run post build actions.",
			"LANGUAGE_ZIG",
			LogType::LOG_INFO);

		for (const auto& a : globalData.targetProfile.postBuildActions)
		{
			Log::Print("\naction: " + a);

			if (system(a.c_str()) != 0)
			{
				KalaMakeCore::CloseOnError(
					"LANGUAGE_ZIG",
					"Failed to run post build action '" + a + "'!");
			}
		}

		Log::Print(" ");

		Log::Print(
			"Finished all post build actions!",
			"LANGUAGE_ZIG",
			LogType::LOG_SUCCESS);
	}
}