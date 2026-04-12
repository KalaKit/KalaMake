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
using std::filesystem::directory_iterator;
using std::ifstream;

//rust + linux-gnu
constexpr string_view target_type_rust_linux_gnu = "x86_64-unknown-linux-gnu";
//rust + linux-musl
constexpr string_view target_type_rust_linux_musl = "x86_64-unknown-linux-musl";
//rust + windows-gnu
constexpr string_view target_type_rust_windows_gnu = "x86_64-pc-windows-gnu";

//win msvc std folder
static const path win_msvc_std_dir = path("lib") / "rustlib" / "x86_64-pc-windows-msvc" / "lib";
//win gnu std folder
static const path win_gnu_std_dir = path("lib") / "rustlib" / "x86_64-pc-windows-gnu" / "lib";
//linux gnu std folder
static const path linux_gnu_std_dir = path("lib") / "rustlib" / "x86_64-unknown-linux-gnu" / "lib";
//linux musl std folder
static const path linux_musl_std_dir = path("lib") / "rustlib" / "x86_64-unknown-linux-musl" / "lib";

static path mainRust{};

static void PreCheck(GlobalData& globalData);

static void Compile_Final(const GlobalData& globalData);

static void GenerateSteps(const GlobalData& globalData)
{
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
	void LanguageCore::Compile_Rust(GlobalData& globalData)
	{
		PreCheck(globalData);
		Compile_Final(globalData);
	}
}

void PreCheck(GlobalData& globalData)
{
#ifdef _WIN32
    if (globalData.targetProfile.targetType == TargetType::T_LINUX_GNU)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_RUST",
			"Target type 'linux-gnu' is not supported for Rust on Windows! Use 'linux-musl' instead.");
    }
#else
    if (globalData.targetProfile.targetType == TargetType::T_WINDOWS_GNU)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_RUST",
			"Target type 'windows-gnu' is not supported for Rust on Linux!");
    }
#endif

	//
	// ENSURE REQUIRED FIELDS ARE NOT MISSING
	//

    if (globalData.targetProfile.standard == StandardType::S_INVALID)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_RUST",
			"Field 'standard' must be assigned in Rust!");
    }
	if (globalData.targetProfile.buildType == BuildType::B_INVALID)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_RUST",
			"Field 'buildtype' must be assigned in Rust!");
    }

    //
    // ENSURE UNSUPPORTED FIELDS ARE NOT USED
    //

    if (globalData.targetProfile.compilerLauncher != CompilerLauncherType::C_INVALID)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_RUST",
			"Field 'compilerlauncher' is not supported in Rust!");
    }
    if (!globalData.targetProfile.headers.empty())
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_RUST",
			"Field 'headers' is not supported in Rust!");
    }
    if (globalData.targetProfile.warningLevel != WarningLevel::W_INVALID)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_RUST",
			"Field 'warninglevel' is not supported in Rust!");
    }
    if (!globalData.targetProfile.linkFlags.empty())
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_RUST",
			"Field 'linkflags' is not supported in Rust!");
    }

    if (globalData.targetProfile.jobs != 0)
    {
        KalaMakeCore::CloseOnError(
			"LANGUAGE_RUST",
			"Field 'jobs' is not supported in Rust!");
    }

	if (ContainsValue(
			globalData.targetProfile.customFlags, 
			CustomFlag::F_EXPORT_COMPILE_COMMANDS))
	{
        KalaMakeCore::CloseOnError(
			"LANGUAGE_RUST",
			"Custom flag 'export-compile-commands' is not supported in Rust!");
	}
    if (ContainsValue(
		globalData.targetProfile.customFlags, 
		CustomFlag::F_MSVC_STATIC_RUNTIME))
	{
        KalaMakeCore::CloseOnError(
			"LANGUAGE_RUST",
			"Custom flag 'msvc-static-runtime' is not supported in Rust!");
	}
	if (ContainsValue(
		globalData.targetProfile.customFlags, 
		CustomFlag::F_PACKAGE_JAR))
	{
        KalaMakeCore::CloseOnError(
			"LANGUAGE_RUST",
			"Custom flag 'package-jar' is not supported in Rust!");
	}
	if (ContainsValue(
		globalData.targetProfile.customFlags, 
		CustomFlag::F_JAVA_WIN_CONSOLE))
	{
        KalaMakeCore::CloseOnError(
			"LANGUAGE_RUST",
			"Custom flag 'java-win-console' is not supported in Rust!");
	}
	if (ContainsValue(
		globalData.targetProfile.customFlags, 
		CustomFlag::F_EXPORT_JAVA_SLN))
	{
        KalaMakeCore::CloseOnError(
			"LANGUAGE_RUST",
			"Custom flag 'export-java-sln' is not supported in Rust!");
	}
    if (ContainsValue(
		globalData.targetProfile.customFlags, 
		CustomFlag::F_PYTHON_ONE_FILE))
	{
        KalaMakeCore::CloseOnError(
			"LANGUAGE_RUST",
			"Custom flag 'python-one-file' is not supported in Rust!");
	}

    //
	// FILTER OUT BAD SOURCE FILES 
	//

	for (const auto& p : globalData.targetProfile.sources)
	{
		if ((p.stem() == "main"
			|| p.stem() == "lib"))
		{
			if (!mainRust.empty())
			{
                KalaMakeCore::CloseOnError(
				    "LANGUAGE_RUST",
				    "Cannot have more than one main Rust script! Please ensure you only have one main.rs or lib.rs script, and not both.");
			}

            if (is_empty(p))
            {
                KalaMakeCore::CloseOnError(
				    "LANGUAGE_RUST",
				    "Main Rust script was empty!");
            }

            mainRust = p;
		}
	}

    if (mainRust.empty())
    {
        KalaMakeCore::CloseOnError(
            "LANGUAGE_RUST",
            "Did not find main Rust script! Please ensure main.rs or lib.rs is added to sources.");
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

			return target.extension() != ".rs";
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
						"LANGUAGE_RUST",
						"Cannot ignore target '" + target.string() + "' if it hasn't already been added to sources list!");
				}
			}

			exclusions.push_back(withoutExclamation);

			Log::Print(
				"Ignoring excluded source script path '" + (*itMatch).string() + "'.",
				"LANGUAGE_RUST",
				LogType::LOG_INFO);

			foundInvalid = true;
			continue;
		}
		if (should_remove(target))
		{
			Log::Print(
				"Ignoring invalid source script path '" + target.string() + "'",
				"LANGUAGE_RUST",
				LogType::LOG_INFO);

			foundInvalid = true;
			continue;
		}

		path canonicalPath = weakly_canonical(target);
		if (!exists(canonicalPath))
		{
			Log::Print(
				"Ignoring non-existing source script path '" + target.string() + "'",
				"LANGUAGE_RUST",
				LogType::LOG_INFO);

			foundInvalid = true;
			continue;
		}
		if (!is_regular_file(canonicalPath))
		{
			Log::Print(
				"Ignoring non-file source script path '" + target.string() + "'",
				"LANGUAGE_RUST",
				LogType::LOG_INFO);

			foundInvalid = true;
			continue;
		}

		finalSources.push_back(target);
	}

	if (finalSources.empty())
	{
		KalaMakeCore::CloseOnError(
			"LANGUAGE_RUST",
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
			"LANGUAGE_RUST",
			LogType::LOG_INFO);

		for (const auto& a : globalData.targetProfile.preBuildActions)
		{
			Log::Print("\naction: " + a);

			if (system(a.c_str()) != 0)
			{
				KalaMakeCore::CloseOnError(
					"LANGUAGE_RUST",
					"Failed to run pre build action '" + a + "'!");
			}
		}

		Log::Print(" ");

		Log::Print(
			"Finished all pre build actions!",
			"LANGUAGE_RUST",
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

            command = "rustc";

            //set binary type

            command += " --crate-type ";
            switch(globalData.targetProfile.binaryType)
            {
            case BinaryType::B_EXECUTABLE:
            {
                command += "bin";
                break;
            }
            case BinaryType::B_SHARED:
            {
                command += "dylib";
                break;
            }
            case BinaryType::B_STATIC:
            {
                command += "rlib";
                break;
            }
            default: break;
            }

            //set standard

            command += " --edition ";

            switch(globalData.targetProfile.standard)
            {
            case StandardType::RUST_15:
            {
                command += "2015";
                break;
            }
            case StandardType::RUST_18:
            {
                command += "2018";
                break;
            }
            case StandardType::RUST_21:
            {
                command += "2021";
                break;
            }
            case StandardType::RUST_24:
            {
                command += "2024";
                break;
            }
            default:
            {
                string_view standard;
            
                EnumToString(
                    globalData.targetProfile.standard, 
                    KalaMakeCore::GetStandardTypes(),
                    standard);

                if (!standard.starts_with("rust"))
                {
                    KalaMakeCore::CloseOnError(
                        "LANGUAGE_RUST",
                        "Unsupported standard type '" + string(standard) + "' was passed to Rust compiler!");
                }
            }
            }

            //set compile flags and build type

            vector<string> finalFlags = globalData.targetProfile.compileFlags;

            if (ContainsValue(
                globalData.targetProfile.customFlags,
                CustomFlag::F_WARNINGS_AS_ERRORS))
            {
                finalFlags.push_back("-D warnings");
            }

            switch (globalData.targetProfile.buildType)
            {
            case BuildType::B_RELEASE:
            {
                finalFlags.push_back("-C opt-level=2");
                finalFlags.push_back("-C strip=symbols");
                break;
            }
            case BuildType::B_DEBUG:
            {
                finalFlags.push_back("-C opt-level=0");
                finalFlags.push_back("-C debuginfo=2");
                break;
            } 
            case BuildType::B_RELDEBUG:
            {
                finalFlags.push_back("-C opt-level=1");
                finalFlags.push_back("-C debuginfo=2");
                break;
            } 
            case BuildType::B_MINSIZEREL:
            {
                finalFlags.push_back("-C opt-level=z");
                finalFlags.push_back("-C strip=symbols");
                break;
            } 
            default: break;
            }

            RemoveDuplicates(finalFlags);

            for (const auto& f : finalFlags)
            {
                command += " " + f;
            }

            //set defines

            for (const auto& d : globalData.targetProfile.defines)
            {
                command += " --cfg " + d;
            }

            //set target type

            switch(globalData.targetProfile.targetType)
            {
            case TargetType::T_LINUX_GNU:
            {
                command += " --target " + string(target_type_rust_linux_gnu);
                command += " -C linker=lld";
                break;
            }
            case TargetType::T_LINUX_MUSL:
            {
                command += " --target " + string(target_type_rust_linux_musl);
                command += " -C linker=lld";
                break;
            }
            case TargetType::T_WINDOWS_GNU:
            {
                command += " --target " + string(target_type_rust_windows_gnu);
                break;
            }
            default: break;
            }

            //set links

            for (const auto& l : globalData.targetProfile.links)
            {
                if (!path(l).has_extension())
                {
                    command += " -l " + l.string();
                }
                else
                {
                    path lpath = l;
                    path lpathDir = lpath.parent_path();
                    string lpathName = lpath.stem().string();

                    //trim off remaining suffix because rust likes to do .dll.lib on Windows
                    if (lpathName.ends_with(".dll")) lpathName = lpathName.substr(0, lpathName.size() - 4);

                    if (lpathName.starts_with("lib")) lpathName = lpathName.substr(3);

                    command += " -L \"" + lpathDir.string() + "\" --extern " + lpathName + "=\"" + lpath.string() + "\"";
                }
            }

            //set crate name

            command += " --crate-name " + globalData.targetProfile.binaryName;

            //set output

            auto has_shared_lib = [&globalData]() -> bool
                {
                    for (const auto& l : globalData.targetProfile.links)
                    {
                        if (l.string().ends_with(".so")
                            || l.string().ends_with(".dll"))
                        {
                            return true;
                        }
                    }

                    return false;
                };

            auto copy_std_dll = [&globalData](const path& targetDir) -> void
                {
                    string origin{};

#ifdef _WIN32
                    FILE* pipe = _popen("rustc --print sysroot", "r");
#else
                    FILE* pipe = popen("rustc --print sysroot", "r");
#endif
                    if (pipe)
                    {
                        char buffer[256]{};
                        while (fgets(buffer, sizeof(buffer), pipe)) origin += buffer;
#ifdef _WIN32
                        _pclose(pipe);
#else
                        pclose(pipe);
#endif
                        if (!origin.empty()
                            && origin.back() == '\n')
                        {
                            origin.pop_back();
                        }
                        else if (origin.empty())
                        {
                            KalaMakeCore::CloseOnError(
                                "LANGUAGE_RUST",
                                "Failed to get Rust sysroot path from pipe!");
                        }
                    }
                    else
                    {
                        KalaMakeCore::CloseOnError(
						    "LANGUAGE_RUST",
						    "Failed to open pipe to get Rust sysroot path!");
                    }

                    path originDir = path(origin);

                    if (!exists(originDir))
                    {
                        KalaMakeCore::CloseOnError(
						    "LANGUAGE_RUST",
						    "Failed to find Rust std library sysroot folder at path '" + originDir.string() + "'!");
                    }

                    switch (globalData.targetProfile.targetType)
                    {
                    case TargetType::T_INVALID:
                    {
#ifdef _WIN32
                        originDir = originDir / win_msvc_std_dir;
#else
                        originDir = originDir / linux_gnu_std_dir;
#endif
                        break;
                    }
                    case TargetType::T_LINUX_GNU:
                    {
                        originDir = originDir / linux_gnu_std_dir;
                        break;
                    }
                    case TargetType::T_LINUX_MUSL:
                    {
                        originDir = originDir / linux_musl_std_dir;
                        break;
                    }
                    case TargetType::T_WINDOWS_GNU:
                    {
                        originDir = originDir / win_gnu_std_dir;
                        break;
                    }
                    default: break;
                    }

                    if (!exists(originDir))
                    {
                        KalaMakeCore::CloseOnError(
						    "LANGUAGE_RUST",
						    "Failed to find Rust std library folder at path '" + originDir.string() + "'!");
                    }

                    bool isWindows = 
#ifdef _WIN32
                        true;
#else
                        false;
#endif

                    bool isWindowsTarget = 
                        globalData.targetProfile.targetType == 
                            (TargetType::T_INVALID)
                            && isWindows;

                    string start = "libstd-";
                    if (isWindowsTarget) start = start.substr(3);
                    string end = isWindowsTarget
                        ? ".dll"
                        : ".so";

                    bool foundTargetFile{};
                    path originPath = originDir;
                    for (const auto& f : directory_iterator(originDir))
                    {
                        string fpath = path(f).filename().string();
                        if (fpath.starts_with(start)
                            && fpath.ends_with(end))
                        {
                            originPath = originPath / fpath;

                            foundTargetFile = true;
                            break;
                        }
                    }                

                    if (!foundTargetFile)
                    {
                        KalaMakeCore::CloseOnError(
						    "LANGUAGE_RUST",
						    "Failed to find Rust std library at path '" + originDir.string() + "'!");
                    }

                    string front = originPath.filename().string();
                    path targetPath = targetDir / front;

                    if (!exists(targetPath))
                    {
                        string err = CopyPath(originPath, targetDir / front);

                        if (!err.empty())
                        {
                            KalaMakeCore::CloseOnError(
                                "LANGUAGE_RUST",
                                "Failed to copy Rust std library to user build dir! Reason: " + err);
                        }

                        Log::Print(
                            "Copied std library from '" + originPath.string() + "' to target '" + (targetDir / front).string() + "'.",
                            "LANGUAGE_RUST",
                            LogType::LOG_INFO);
                    }
                };

            path buildPath = globalData.targetProfile.buildPath;
            string binaryName = globalData.targetProfile.binaryName;

			bool isOnLinux{};
#ifdef __linux__
			isOnLinux = true;
#endif

            string extension{};
            switch (globalData.targetProfile.binaryType)
            {
            case BinaryType::B_EXECUTABLE:
            {
                if (has_shared_lib())
                {
                    if (isOnLinux) command += " -C link-arg=-Wl,-rpath,\\$ORIGIN";
                    command += " -C prefer-dynamic";

                    copy_std_dll(buildPath);
                }

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

                break;
            }
            case BinaryType::B_SHARED:
            {
                command += " -C prefer-dynamic";

				if ((isOnLinux
					&& globalData.targetProfile.targetType == TargetType::T_INVALID)
					|| globalData.targetProfile.targetType == TargetType::T_LINUX_GNU
					|| globalData.targetProfile.targetType == TargetType::T_LINUX_MUSL)
				{
                    if (!binaryName.starts_with("lib")) binaryName = "lib" + binaryName;
					if (!binaryName.ends_with(".so")) extension = ".so";
				}
				else
				{
					if (!binaryName.ends_with(".dll")) extension = ".dll";
				}

                break;
            }
            case BinaryType::B_STATIC:
            {
                if (!binaryName.starts_with("lib")) binaryName = "lib" + binaryName;
                if (!binaryName.ends_with(".rlib")) extension = ".rlib";
                break;
            }
            default: break;
            }

            path outputPath = path(buildPath / string(binaryName + extension));

            command += " -o \"" + outputPath.string() + "\"";

            //add main rust script

            command += " \"" + mainRust.string() + "\"";

            //compile

            bool isAnyNewer{};

            file_time_type kmakeTime = last_write_time(globalData.projectFile);

            if (exists(outputPath))
            {
                file_time_type outputWriteTime = last_write_time(outputPath);

                if (kmakeTime > outputWriteTime) isAnyNewer = true;

                if (!isAnyNewer)
                {
                    for (const auto& s : globalData.targetProfile.sources)
                    {
                        if (outputWriteTime < last_write_time(s))
                        {
                            isAnyNewer = true;
                            break;
                        }
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
					"LANGUAGE_RUST",
					LogType::LOG_INFO);

                Log::Print(" ");

                if (system(command.c_str()) != 0)
                {
					KalaMakeCore::CloseOnError(
						"LANGUAGE_RUST",
						"Failed to compile '" + outputPath.string() + "'!");
                }

				Log::Print(
					"Finished compiling to output '" + outputPath.string() + "'!",
					"LANGUAGE_RUST",
					LogType::LOG_SUCCESS);
            }
            else
            {
				Log::Print(
					"Skipping compiling to output '" + outputPath.string() + "' because there are no new source files.",
					"LANGUAGE_RUST",
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
			"LANGUAGE_RUST",
			LogType::LOG_INFO);

		for (const auto& a : globalData.targetProfile.postBuildActions)
		{
			Log::Print("\naction: " + a);

			if (system(a.c_str()) != 0)
			{
				KalaMakeCore::CloseOnError(
					"LANGUAGE_RUST",
					"Failed to run post build action '" + a + "'!");
			}
		}

		Log::Print(" ");

		Log::Print(
			"Finished all post build actions!",
			"LANGUAGE_RUST",
			LogType::LOG_SUCCESS);
	}
}