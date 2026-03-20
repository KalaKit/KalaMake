//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include <string>
#include <vector>
#include <filesystem>
#include <atomic>
#include <thread>
#include <sstream>

#include "core_utils.hpp"
#include "log_utils.hpp"
#include "file_utils.hpp"
#include "thread_utils.hpp"
#include "string_utils.hpp"

#include "language/kma_language.hpp"
#include "core/kma_core.hpp"
#include "core/kma_generate.hpp"

using KalaHeaders::KalaCore::EnumToString;
using KalaHeaders::KalaCore::RemoveDuplicates;
using KalaHeaders::KalaCore::ContainsValue;

using KalaHeaders::KalaLog::Log;
using KalaHeaders::KalaLog::LogType;

using KalaHeaders::KalaFile::CreateNewDirectory;

using KalaHeaders::KalaThread::lockwait_m;
using KalaHeaders::KalaThread::unlock_m;

using KalaHeaders::KalaString::RemoveFromString;

using KalaMake::Core::KalaMakeCore;
using KalaMake::Language::GlobalData;
using KalaMake::Core::BinaryType;
using KalaMake::Core::CompilerLauncherType;
using KalaMake::Core::CompilerType;
using KalaMake::Core::StandardType;
using KalaMake::Core::TargetType;
using KalaMake::Core::BuildType;
using KalaMake::Core::WarningLevel;
using KalaMake::Core::CustomFlag;
using KalaMake::Core::Generate;
using KalaMake::Core::CompileCommand;
using KalaMake::Core::VSCode_Launch;
using KalaMake::Core::VSCode_Task;

using std::string;
using std::string_view;
using std::vector;
using std::filesystem::path;
using std::filesystem::current_path;
using std::filesystem::relative;
using std::filesystem::exists;
using std::filesystem::is_regular_file;
using std::filesystem::is_directory;
using std::filesystem::last_write_time;
using std::filesystem::file_time_type;
using std::min;
using std::atomic;
using std::thread;
using std::mutex;
using std::ostringstream;

using u16 = uint16_t;

static void PreCheck(GlobalData& globalData);

static void Compile_Final(const GlobalData& globalData);

constexpr string_view objFolderName = "obj";

//gcc + linux-gnu
constexpr string_view target_type_linux_gnu_gcc = "x86_64-linux-gnu-gcc";
//g++ + linux-gnu
constexpr string_view target_type_linux_gnu_gpp = "x86_64-linux-gnu-g++";
//clang/clang++/zig + linux-gnu
constexpr string_view target_type_linux_gnu = "x86_64-linux-gnu";
//gcc + linux-musl
constexpr string_view target_type_linux_musl_gcc = "x86_64-linux-musl-gcc";
//g++ + linux-musl
constexpr string_view target_type_linux_musl_gpp = "x86_64-linux-musl-g++";
//clang/clang++/zig + linux_musl
constexpr string_view target_type_linux_musl = "x86_64-linux-musl";

//gcc + windows-gnu
constexpr string_view target_type_win_gcc = "x86_64-w64-mingw32-gcc";
//g++ + windows-gnu
constexpr string_view target_type_win_gpp = "x86_64-w64-mingw32-g++";
//clang/clang++/zig + windows-gnu
constexpr string_view target_type_win_gnu = "x86_64-w64-windows-gnu";

static vector<CompileCommand> commands{};

static void GenerateSteps(
	bool isMSVC,
	const GlobalData& globalData)
{
	bool canGenerateCompComm = ContainsValue(globalData.targetProfile.customFlags, CustomFlag::F_EXPORT_COMPILE_COMMANDS);
	bool canGenerateVSCodeSln = ContainsValue(globalData.targetProfile.customFlags, CustomFlag::F_EXPORT_VSCODE_SLN);

	if (canGenerateCompComm)
	{
		path projectFileDir = globalData.projectFile.parent_path();

		Generate::GenerateCompileCommands(
			isMSVC,
			projectFileDir,
			commands);

		if (canGenerateVSCodeSln) Log::Print(" ");
	}
	if (canGenerateVSCodeSln)
	{
		path relativeBuildPath = relative(globalData.targetProfile.buildPath, current_path());
		path programPath = relativeBuildPath / globalData.targetProfile.binaryName;

		VSCode_Launch launch
		{
			.name = globalData.targetProfile.profileName,
			.type = isMSVC ? "cppvsdbg" : "cppdbg",
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

	if (canGenerateCompComm
		|| canGenerateVSCodeSln)
	{
		Log::Print("===========================================================================\n");
	}
}

namespace KalaMake::Language
{
	void LanguageCore::Compile(GlobalData& globalData)
	{
		CompilerType c = globalData.targetProfile.compiler;
		if (c != CompilerType::C_ZIG
			&& c != CompilerType::C_CL
			&& c != CompilerType::C_CLANG_CL
			&& c != CompilerType::C_CLANG
			&& c != CompilerType::C_CLANGPP
			&& c != CompilerType::C_GCC
			&& c != CompilerType::C_GPP)
		{
			return;
		}

		commands.clear();

		PreCheck(globalData);
		Compile_Final(globalData);
	}
}

void PreCheck(GlobalData& globalData)
{		
	StandardType standard = globalData.targetProfile.standard;
	bool isCLanguage =
		standard == StandardType::C_89
		|| standard == StandardType::C_99
		|| standard == StandardType::C_11
		|| standard == StandardType::C_17
		|| standard == StandardType::C_23;

	bool isCPPLanguage =
		standard == StandardType::CPP_03
		|| standard == StandardType::CPP_11
		|| standard == StandardType::CPP_14
		|| standard == StandardType::CPP_17
		|| standard == StandardType::CPP_20
		|| standard == StandardType::CPP_23
		|| standard == StandardType::CPP_26;

	auto should_remove = [isCLanguage, isCPPLanguage](
		const path& target, 
		bool isSource) -> bool
		{
			if (!isSource
				&& (!exists(target)
				|| !is_directory(target)))
			{
				return true;
			}

			if (isSource
				&& (!exists(target)
				|| is_directory(target)
				|| !is_regular_file(target)
				|| !target.has_extension()))
			{
				return true;
			}

			if (!isCLanguage
				&& !isCPPLanguage)
			{
				return true;
			}

			string extension = target.extension().string();

			if (isCLanguage)
			{
				if (isSource
					&& target.extension() != ".c")
				{
					return true;
				}
			}
			if (isCPPLanguage)
			{
				if (isSource
					&& target.extension() != ".c"
					&& target.extension() != ".cpp"
					&& target.extension() != ".cc"
					&& target.extension() != ".cxx")
				{
					return true;
				}
			}

			return false;
		};

	vector<path>& sources = globalData.targetProfile.sources;
	for (auto it = sources.begin(); it != sources.end();)
	{
		path target = *it;
		if (should_remove(target, true))
		{
			Log::Print(
				"Removed invalid source script path '" + target.string() + "'",
				"LANGUAGE_C_CPP",
				LogType::LOG_INFO);

			sources.erase(it);
		}
		else ++it;
	}

	if (sources.empty())
	{
		KalaMakeCore::CloseOnError(
			"LANGUAGE_C_CPP",
			"No sources were remaining after cleaning source scripts list!");
	}
}

void Compile_Final(const GlobalData& globalData)
{
	bool isMSVC = 
		globalData.targetProfile.compiler == CompilerType::C_CL
		|| globalData.targetProfile.compiler == CompilerType::C_CLANG_CL;

	if ((globalData.targetProfile.compiler == CompilerType::C_CL
		|| globalData.targetProfile.compiler == CompilerType::C_CLANG_CL)
		&& globalData.targetProfile.targetType != TargetType::T_INVALID)
	{
		KalaMakeCore::CloseOnError(
			"LANGUAGE_C_CPP",
			"Cannot assign target type for cl or clang-cl compiler!");
	}

	string frontArg = isMSVC
		? "/"
		: "-";

	//
	// PRE BUILD ACTIONS
	//

	if (!globalData.targetProfile.preBuildActions.empty())
	{
		Log::Print(
			"Starting to run pre build actions.",
			"LANGUAGE_C_CPP",
			LogType::LOG_INFO);

		for (const auto& a : globalData.targetProfile.preBuildActions)
		{
			Log::Print("\naction: " + a);

			if (system(a.c_str()) != 0)
			{
				KalaMakeCore::CloseOnError(
					"LANGUAGE_C_CPP",
					"Failed to run pre build action!");
			}
		}

		Log::Print(" ");

		Log::Print(
			"Finished all pre build actions!",
			"LANGUAGE_C_CPP",
			LogType::LOG_SUCCESS);

		Log::Print("\n===========================================================================\n");
	}

	//
	// COMPILE
	//

	auto compile = [&isMSVC, &frontArg, &globalData]() -> vector<path>
		{
			string command{};

			//set compiler launcher

			if (globalData.targetProfile.compilerLauncher != CompilerLauncherType::C_INVALID)
			{
				string_view compilerLauncher{};
				EnumToString(globalData.targetProfile.compilerLauncher, KalaMakeCore::GetCompilerLauncherTypes(), compilerLauncher);

				command += string(compilerLauncher) + " ";
			}

			//set compiler

			//TODO: call cl x64 bat correctly

			string_view compiler{};
			EnumToString(globalData.targetProfile.compiler, KalaMakeCore::GetCompilerTypes(), compiler);

			if (globalData.targetProfile.targetType == TargetType::T_LINUX_GNU)
			{
				if (compiler == "gcc")      compiler = target_type_linux_gnu_gcc;
				else if (compiler == "g++") compiler = target_type_linux_gnu_gpp;
			}
			else if (globalData.targetProfile.targetType == TargetType::T_LINUX_MUSL)
			{
				if (compiler == "gcc")      compiler = target_type_linux_musl_gcc;
				else if (compiler == "g++") compiler = target_type_linux_musl_gpp;
			}
			else if (globalData.targetProfile.targetType == TargetType::T_WINDOWS_GNU)
			{
				if (compiler == "gcc")      compiler = target_type_win_gcc;
				else if (compiler == "g++") compiler = target_type_win_gpp;
			}

			command += string(compiler);

			if (compiler == "zig")
			{
				StandardType standardType = globalData.targetProfile.standard;

				if (standardType == StandardType::C_89
					|| standardType == StandardType::C_99
					|| standardType == StandardType::C_11
					|| standardType == StandardType::C_17
					|| standardType == StandardType::C_23)
				{
					command += " c";
				}
				else if (standardType == StandardType::CPP_03
					|| standardType == StandardType::CPP_11
					|| standardType == StandardType::CPP_14
					|| standardType == StandardType::CPP_17
					|| standardType == StandardType::CPP_20
					|| standardType == StandardType::CPP_23
					|| standardType == StandardType::CPP_26)
				{
					command += " c++";
				}
			}

			//set target type
			if (compiler == "clang"
				|| compiler == "clang++"
				|| compiler == "zig")
			{
				if (globalData.targetProfile.targetType == TargetType::T_LINUX_GNU)
				{
					command += " -target " + string(target_type_linux_gnu);
				}
				else if (globalData.targetProfile.targetType == TargetType::T_LINUX_MUSL)
				{
					command += " -target " + string(target_type_linux_musl);
				}
				else if (globalData.targetProfile.targetType == TargetType::T_WINDOWS_GNU)
				{
					command += " -target " + string(target_type_win_gnu);
				}
			}

			//set standard

			string_view standard{};
			EnumToString(globalData.targetProfile.standard, KalaMakeCore::GetStandardTypes(), standard);

			string standardArg = isMSVC
				? frontArg + "std:"
				: frontArg + "std=";

			command += " " + standardArg + string(standard);

			//set compile flags and warning level

			vector<string> finalFlags = globalData.targetProfile.compileFlags;

			if (isMSVC)
			{
				string runtimeFlag = ContainsValue(
					globalData.targetProfile.customFlags, 
					CustomFlag::F_MSVC_STATIC_RUNTIME) ? "MT" : "MD";
				
				finalFlags.push_back(globalData.targetProfile.buildType == KalaMake::Core::BuildType::B_DEBUG
					? runtimeFlag + "d"
					: runtimeFlag);
			}

			//always enable exceptions for msvc
			if (isMSVC) finalFlags.push_back("EHsc");

			if (ContainsValue(
				globalData.targetProfile.customFlags, 
				CustomFlag::F_WARNINGS_AS_ERRORS))
			{
				if (isMSVC) finalFlags.push_back("WX");
				else        finalFlags.push_back("Werror");
			}

			//always use NDEBUG for release, minsizerel and reldebug
			if (globalData.targetProfile.buildType != BuildType::B_DEBUG)
			{
				finalFlags.push_back("DNDEBUG");
			}

			switch (globalData.targetProfile.buildType)
			{
			case BuildType::B_DEBUG:
			{
				if (isMSVC)
				{
					finalFlags.push_back("Zi");
					finalFlags.push_back("Od");
				}
				else
				{
					finalFlags.push_back("g");
					finalFlags.push_back("O0");
				}
				break;
			}
			case BuildType::B_RELDEBUG:
			{
				if (isMSVC)
				{
					finalFlags.push_back("Zi");
					finalFlags.push_back("O2");
				}
				else
				{
					finalFlags.push_back("g");
					finalFlags.push_back("O2");
				}
				break;
			}
			case BuildType::B_RELEASE:
			{
				if (isMSVC)
				{
					finalFlags.push_back("O2");
				}
				else
				{
					finalFlags.push_back("g0");
					finalFlags.push_back("O2");
				}
				break;
			}
			case BuildType::B_MINSIZEREL:
			{
				if (isMSVC)
				{
					finalFlags.push_back("O1");
				}
				else
				{
					finalFlags.push_back("g0");
					finalFlags.push_back("Os");
				}
				break;
			}

			default: break;
			}

			switch (globalData.targetProfile.warningLevel)
			{
			case WarningLevel::W_BASIC:
			{
				if (isMSVC) finalFlags.push_back("W1");
				else        finalFlags.push_back("Wall");
				break;
			}
			case WarningLevel::W_NORMAL:
			{
				if (isMSVC) finalFlags.push_back("W3");
				else
				{
					finalFlags.push_back("Wall");
					finalFlags.push_back("Wextra");
				}
				break;
			}
			case WarningLevel::W_STRONG:
			{
				if (isMSVC) finalFlags.push_back("W4");
				else
				{
					finalFlags.push_back("Wall");
					finalFlags.push_back("Wextra");
					finalFlags.push_back("Wpedantic");
				}
				break;
			}
			case WarningLevel::W_STRICT:
			{
				if (isMSVC)
				{
					finalFlags.push_back("W4");
					finalFlags.push_back("permissive-");
				}
				else
				{
					finalFlags.push_back("Wall");
					finalFlags.push_back("Wextra");
					finalFlags.push_back("Wpedantic");
					finalFlags.push_back("Wconversion");
					finalFlags.push_back("Wsign-conversion");
				}
				break;
			}
			case WarningLevel::W_ALL:
			{
				if (isMSVC) finalFlags.push_back("Wall");
				else if (globalData.targetProfile.compiler == CompilerType::C_CLANG
						|| globalData.targetProfile.compiler == CompilerType::C_CLANGPP
						|| globalData.targetProfile.compiler == CompilerType::C_ZIG)
				{
					finalFlags.push_back("Weverything");
				}
				else
				{
					finalFlags.push_back("Wall");
					finalFlags.push_back("Wextra");
					finalFlags.push_back("Wpedantic");
					finalFlags.push_back("Wconversion");
					finalFlags.push_back("Wsign-conversion");
				}
				break;
			}

			default: break;
			}

			RemoveDuplicates(finalFlags);
			for (const auto& f : finalFlags)
			{
				command += " " + frontArg + f;
			}

			//set defines

			string defineArg = frontArg + "D";

			for (const auto& d : globalData.targetProfile.defines)
			{
				command += " " + defineArg + d;
			}

			//set headers

			if (!globalData.targetProfile.headers.empty())
			{
				for (const auto& h : globalData.targetProfile.headers)
				{
					command += " " + frontArg + "I\"" + h.string() + "\"";
				}
			}

			//compile

			path buildPath = globalData.targetProfile.buildPath / objFolderName;

			if (!exists(buildPath))
			{
				string errorMsg = CreateNewDirectory(buildPath);
				if (!errorMsg.empty())
				{
					KalaMakeCore::CloseOnError(
						"LANGUAGE_C_CPP",
						"Failed to create new obj dir for compilation! Reason: " + errorMsg);
				}
			}

			string binaryName = globalData.targetProfile.binaryName;

			string extension = isMSVC
				? ".obj"
				: ".o";

			//compile-time shared flag for object file
			if (!isMSVC
				&& globalData.targetProfile.binaryType == BinaryType::B_SHARED)
			{
				command += " -fPIC";
			}

			command += " " + frontArg + "c";
			string objFront = isMSVC
				? "/Fo:"
				: "-o";

			vector<path> compiledObj{};
			mutex m_compiledObj;

			vector<file_time_type> headerTimes{};
			headerTimes.reserve(globalData.targetProfile.headers.size());
			for (const auto& h : globalData.targetProfile.headers)
			{
				headerTimes.push_back(last_write_time(h));
			}

			auto headers_up_to_date = [headerTimes](const auto& t) -> bool
				{
					for (const auto& ht : headerTimes) if (ht > t) return false;

					return true;
				};

			auto needs_compile = [headers_up_to_date](
				const path& source,
				const path& object
				) -> bool
				{
					return !exists(object)
						|| last_write_time(object) < last_write_time(source)
						|| !headers_up_to_date(last_write_time(object));
				};

			auto generate = [
				&isMSVC,
				&globalData,
				&buildPath,
				&extension,
				&command,
				&objFront]() -> void
				{
					if (ContainsValue(globalData.targetProfile.customFlags,CustomFlag::F_EXPORT_COMPILE_COMMANDS))
					{
						path fullBuildPath = buildPath.is_absolute()
							? buildPath
							: current_path() / buildPath;

						for (size_t i = 0; i < globalData.targetProfile.sources.size(); i++)
						{
							const path& s = globalData.targetProfile.sources[i];

							path objPath = fullBuildPath / (s.stem().string() + extension);
							
							string perFileCommand = command;

							perFileCommand += " \"" + s.string() + "\"";
							perFileCommand += " " + objFront + " \"" + objPath.string() + "\"";

							commands.push_back(
							{
								.dir = globalData.projectFile.parent_path(),
								.command = RemoveFromString(perFileCommand, "\"", true),
								.file = s,
								.output = objPath.string()
							});
						}
					}

					GenerateSteps(isMSVC, globalData);
				};

			auto compile = [
				&globalData,
				&buildPath,
				&extension,
				&command,
				&objFront,
				needs_compile,
				&compiledObj,
				&m_compiledObj]
				(int targetIndex) -> void
				{
					const path& s = globalData.targetProfile.sources[targetIndex];

					path objPath = buildPath / (s.stem().string() + extension);
					
					string perFileCommand = command;

					perFileCommand += " \"" + s.string() + "\"";
					perFileCommand += " " + objFront + " \"" + objPath.string() + "\"";

					if (needs_compile(s, objPath))
					{
						Log::Print(
							"Starting to compile via '" + perFileCommand + "'.",
							"LANGUAGE_C_CPP",
							LogType::LOG_INFO);
							
						if (system(perFileCommand.c_str()) != 0)
						{
							KalaMakeCore::CloseOnError(
								"LANGUAGE_C_CPP",
								"Failed to compile object file '" + objPath.string() + "'!");
						}
					}
					else
					{
						Log::Print(
							"Skipping compilation of object file '" + objPath.string() + "' because it is newer than its source and header files.",
							"LANGUAGE_C_CPP",
							LogType::LOG_INFO);

						Log::Print("\n");
					}

					lockwait_m(m_compiledObj);
					compiledObj.push_back(objPath);
					unlock_m(m_compiledObj);
				};

			generate();

			if (globalData.targetProfile.sources.size() == 1) compile(0);
			else
			{
				u16 max_jobs = min(
					scast<size_t>(globalData.targetProfile.jobs), 
					globalData.targetProfile.sources.size());

				atomic<int> next{};

				vector<thread> workers{};

				for (u16 i = 0; i < max_jobs; ++i)
				{
					workers.emplace_back([
						&next, 
						&globalData,
						compile] 
						{
							while (true)
							{
								int idx = next++;

								if (scast<size_t>(idx) >= globalData.targetProfile.sources.size()) break;

								compile(idx);
							}
						});
				}

				for (auto& w : workers) w.join();
			}

			return compiledObj;
		};

	//
	// LINK
	//

	auto link = [&isMSVC, &globalData, &frontArg](const vector<path>& objFiles) -> void
		{
			string sharedArg = globalData.targetProfile.binaryType == BinaryType::B_SHARED
				? (isMSVC ? "/LD" : "-shared")
				: string{};

			string command{};

			//set compiler launcher

			if (globalData.targetProfile.compilerLauncher != CompilerLauncherType::C_INVALID
				&& globalData.targetProfile.binaryType != BinaryType::B_STATIC)
			{
				string_view compilerLauncher{};
				EnumToString(globalData.targetProfile.compilerLauncher, KalaMakeCore::GetCompilerLauncherTypes(), compilerLauncher);

				command += string(compilerLauncher) + " ";
			}

			//set compiler

			//TODO: call cl x64 bat correctly

			string compiler{};
			if (globalData.targetProfile.binaryType == BinaryType::B_EXECUTABLE
				|| globalData.targetProfile.binaryType == BinaryType::B_SHARED)
			{
				string_view nonStaticCompiler{};
				EnumToString(globalData.targetProfile.compiler, KalaMakeCore::GetCompilerTypes(), nonStaticCompiler);

				compiler = nonStaticCompiler;

				if (globalData.targetProfile.targetType == TargetType::T_LINUX_GNU)
				{
					if (compiler == "gcc")      compiler = target_type_linux_gnu_gcc;
					else if (compiler == "g++") compiler = target_type_linux_gnu_gpp;
				}
				else if (globalData.targetProfile.targetType == TargetType::T_LINUX_MUSL)
				{
					if (compiler == "gcc")      compiler = target_type_linux_musl_gcc;
					else if (compiler == "g++") compiler = target_type_linux_musl_gpp;
				}
				else if (globalData.targetProfile.targetType == TargetType::T_WINDOWS_GNU)
				{
					if (compiler == "gcc")      compiler = target_type_win_gcc;
					else if (compiler == "g++") compiler = target_type_win_gpp;
				}
			}
			else
			{
#ifdef _WIN32
				compiler = "lib";
#else
				compiler = "ar rcs";
#endif
			}

			command += compiler;

			if (compiler == "zig")
			{
				StandardType standardType = globalData.targetProfile.standard;

				if (standardType == StandardType::C_89
					|| standardType == StandardType::C_99
					|| standardType == StandardType::C_11
					|| standardType == StandardType::C_17
					|| standardType == StandardType::C_23)
				{
					command += " c";
				}
				else if (standardType == StandardType::CPP_03
					|| standardType == StandardType::CPP_11
					|| standardType == StandardType::CPP_14
					|| standardType == StandardType::CPP_17
					|| standardType == StandardType::CPP_20
					|| standardType == StandardType::CPP_23
					|| standardType == StandardType::CPP_26)
				{
					command += " c++";
				}
			}

			//set target type
			if (compiler == "clang"
				|| compiler == "clang++"
				|| compiler == "zig")
			{
				if (globalData.targetProfile.targetType == TargetType::T_LINUX_GNU)
				{
					command += " -target " + string(target_type_linux_gnu);
				}
				else if (globalData.targetProfile.targetType == TargetType::T_LINUX_MUSL)
				{
					command += " -target " + string(target_type_linux_musl);
				}
				else if (globalData.targetProfile.targetType == TargetType::T_WINDOWS_GNU)
				{
					command += " -target " + string(target_type_win_gnu);
				}
			}

			//set output

			string outputArgFront{};
			string outputArg{};

			if (globalData.targetProfile.binaryType == BinaryType::B_EXECUTABLE
				|| globalData.targetProfile.binaryType == BinaryType::B_SHARED)
			{
				//link-time shared flag for output
				outputArgFront = globalData.targetProfile.binaryType == BinaryType::B_SHARED
					? (isMSVC ? "/LD " : "-shared " )
					: string{};

				outputArg = isMSVC
					? "/Fe:"
					: "-o ";
			}
			else
			{
				if (isMSVC) outputArg = "/OUT:";
			}

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
				}
				else
				{
					if (!binaryName.ends_with(".lib")) extension = ".lib";
				}
			}

			path outputPath = path(buildPath / string(binaryName + extension));

			command += " " + outputArgFront + outputArg + "\"" + outputPath.string() + "\"";

			//add object files
			
			for (const auto& o : objFiles)
			{
				command += " \"" + o.string() + "\"";
			}

			//set links

			if (!globalData.targetProfile.links.empty())
			{
				for (const auto& l : globalData.targetProfile.links)
				{
					if (path(l).has_extension()) command += " \"" + l.string() + "\"";
					else
					{
						if (isMSVC) command += " " + l.string() + ".lib";
						else        command += " -l" + l.string();
					}
				}
			}

			//set link flags

			vector<string> finalFlags = globalData.targetProfile.linkFlags;

			if (globalData.targetProfile.binaryType != BinaryType::B_STATIC)
			{
				if (isMSVC)
				{
					if (globalData.targetProfile.buildType == KalaMake::Core::BuildType::B_DEBUG
						|| globalData.targetProfile.buildType == KalaMake::Core::BuildType::B_RELDEBUG)
					{
						finalFlags.push_back("DEBUG");
					}
					else finalFlags.push_back("RELEASE");
				}
			}

			if (isMSVC
				&& !finalFlags.empty())
			{
				finalFlags.insert(finalFlags.begin(), "link");
			}

			RemoveDuplicates(finalFlags);
			for (const auto& f : finalFlags)
			{
				command += " " + frontArg + f;
			}

			//link 

			auto needs_link = [&globalData](
				const path& output,
				const vector<path>& objects
				) -> bool
				{
					if (!exists(output)) return true;

					auto exeTime = last_write_time(output);

					for (const auto& o : objects)
					{
						if (!exists(o)
							|| last_write_time(o) > exeTime)
						{
							return true;
						}
					}

					for (const auto& l : globalData.targetProfile.links)
					{
						if (exists(l)
							&& last_write_time(l) > exeTime)
						{
							return true;
						}
					}

					return false;
				};

			Log::Print("===========================================================================\n");

			if (needs_link(outputPath, objFiles))
			{
				Log::Print(
					"Starting to link via '" + command + "'.",
					"LANGUAGE_C_CPP",
					LogType::LOG_INFO);

				Log::Print(" ");

				if (system(command.c_str()) != 0)
				{
					KalaMakeCore::CloseOnError(
						"LANGUAGE_C_CPP",
						"Failed to link '" + outputPath.string() + "'!");
				}

				Log::Print(
					"Finished linking to output '" + outputPath.string() + "'!",
					"LANGUAGE_C_CPP",
					LogType::LOG_SUCCESS);
			}
			else
			{
				Log::Print(
					"Skipping linking of output '" + outputPath.string() + "' because there are no new object files.",
					"LANGUAGE_C_CPP",
					LogType::LOG_INFO);
			}
			};

	//
	// COMPILE AND LINK
	//

	vector<path> objFiles = compile();
	if (!objFiles.empty()) link(objFiles);

	//
	// POST BUILD ACTIONS
	//

	if (!globalData.targetProfile.postBuildActions.empty())
	{
		Log::Print("\n===========================================================================\n");

		Log::Print(
			"Starting to run post build actions.",
			"LANGUAGE_C_CPP",
			LogType::LOG_INFO);

		for (const auto& a : globalData.targetProfile.postBuildActions)
		{
			Log::Print("\naction: " + a);

			if (system(a.c_str()) != 0)
			{
				KalaMakeCore::CloseOnError(
					"LANGUAGE_C_CPP",
					"Failed to run post build action!");
			}
		}

		Log::Print(" ");

		Log::Print(
			"Finished all post build actions!",
			"LANGUAGE_C_CPP",
			LogType::LOG_SUCCESS);
	}
}