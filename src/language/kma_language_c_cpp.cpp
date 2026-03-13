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

using KalaHeaders::KalaCore::EnumToString;
using KalaHeaders::KalaCore::RemoveDuplicates;
using KalaHeaders::KalaCore::ContainsValue;

using KalaHeaders::KalaLog::Log;
using KalaHeaders::KalaLog::LogType;

using KalaHeaders::KalaFile::DeletePath;
using KalaHeaders::KalaFile::CreateNewDirectory;
using KalaHeaders::KalaFile::CreateNewFile;
using KalaHeaders::KalaFile::FileType;

using KalaHeaders::KalaThread::lockwait_m;
using KalaHeaders::KalaThread::unlock_m;

using KalaHeaders::KalaString::RemoveFromString;
using KalaHeaders::KalaString::ReplaceFromString;

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

using std::string;
using std::string_view;
using std::vector;
using std::filesystem::path;
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
static void Generate_Final(const GlobalData& globalData);

constexpr string_view objFolderName = "obj";

constexpr string_view gcc_compiler_windows_to_linux = "x86_64-linux-gnu-gcc";
constexpr string_view gcc_compiler_linux_to_windows = "x86_64-w64-mingw32-gcc";
constexpr string_view gpp_compiler_windows_to_linux = "x86_64-linux-gnu-g++";
constexpr string_view gpp_compiler_linux_to_windows = "x86_64-w64-mingw32-g++";

constexpr string_view clang_zig_target_windows_to_linux = "x86_64-linux-gnu";
//used by default for clang linux-to-windows
constexpr string_view clang_zig_target_linux_to_windows_gnu = "x86_64-windows-gnu";
//optional for clang linux-to-windows, must add custom flag use-clang-zig-msvc to enable
constexpr string_view clang_zig_target_linux_to_windows_msvc = "x86_64-windows-msvc";

struct CompileCommand
{
	path dir{};
	string command{};
	path file{};
	string output{};
};

static vector<CompileCommand> commands{};

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

		PreCheck(globalData);
		Compile_Final(globalData);
	}

	void LanguageCore::Generate(GlobalData& globalData)
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

		PreCheck(globalData);
		Generate_Final(globalData);
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
		|| standard == StandardType::C_23
		|| standard == StandardType::C_LATEST;

	bool isCPPLanguage =
		standard == StandardType::CPP_03
		|| standard == StandardType::CPP_11
		|| standard == StandardType::CPP_14
		|| standard == StandardType::CPP_17
		|| standard == StandardType::CPP_20
		|| standard == StandardType::CPP_23
		|| standard == StandardType::CPP_LATEST;

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
					&& target.extension() != ".cpp")
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

			string_view compiler{};
			EnumToString(globalData.targetProfile.compiler, KalaMakeCore::GetCompilerTypes(), compiler);
			
#if _WIN32
			if (globalData.targetProfile.targetType == TargetType::T_LINUX)
			{
				if (compiler == "gcc")      compiler = gcc_compiler_windows_to_linux;
				else if (compiler == "g++") compiler = gpp_compiler_windows_to_linux;
			}
#else
			if (globalData.targetProfile.targetType == TargetType::T_WINDOWS)
			{
				if (compiler == "gcc")      compiler = gcc_compiler_linux_to_windows;
				else if (compiler == "g++") compiler = gpp_compiler_linux_to_windows;
			}
#endif
			
			command += string(compiler);

			if (compiler == "zig")
			{
				StandardType standardType = globalData.targetProfile.standard;

				if (standardType == StandardType::C_89
					|| standardType == StandardType::C_99
					|| standardType == StandardType::C_11
					|| standardType == StandardType::C_17
					|| standardType == StandardType::C_23
					|| standardType == StandardType::C_LATEST)
				{
					command += " c";
				}
				else if (standardType == StandardType::CPP_03
					|| standardType == StandardType::CPP_11
					|| standardType == StandardType::CPP_14
					|| standardType == StandardType::CPP_17
					|| standardType == StandardType::CPP_20
					|| standardType == StandardType::CPP_23
					|| standardType == StandardType::CPP_LATEST)
				{
					command += " c++";
				}
			}

			//set target type

			if (compiler == "clang"
				|| compiler == "clang++"
				|| compiler == "zig")
			{
#if _WIN32
				if (globalData.targetProfile.targetType == TargetType::T_LINUX)
				{
					command += " -target " + string(clang_zig_target_windows_to_linux);
				}
#else
				if (globalData.targetProfile.targetType == TargetType::T_WINDOWS)
				{
					if (ContainsValue(globalData.targetProfile.customFlags, CustomFlag::F_USE_CLANG_ZIG_MSVC))
					{
						command += " -target " + string(clang_zig_target_linux_to_windows_msvc);
					}
					else command += " -target " + string(clang_zig_target_linux_to_windows_gnu);
				}
#endif
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

			//explicitly enable exceptions
			if (isMSVC) finalFlags.push_back("EHsc");

			if (ContainsValue(
				globalData.targetProfile.customFlags, 
				CustomFlag::F_WARNINGS_AS_ERRORS))
			{
				if (isMSVC) finalFlags.push_back("WX");
				else        finalFlags.push_back("Werror");
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
				: "-o ";

			vector<path> compiledObj{};
			mutex m_compiledObj;

			Log::Print("\n===========================================================================\n");

			auto headers_up_to_date = [&globalData](const auto& t) -> bool
				{
					for (const auto& h : globalData.targetProfile.headers)
					{
						if (last_write_time(h) > t) return false;
					}

					return true;
				};

			auto needs_compile = [headers_up_to_date](
				const path& source,
				const path& object
				) -> bool
				{
					bool objExists = exists(object);

					const auto t = objExists
						? last_write_time(object)
						: file_time_type{};

					return 
						!objExists
						|| last_write_time(source) > t
						|| !headers_up_to_date(t);
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

					if (ContainsValue(globalData.targetProfile.customFlags,CustomFlag::F_EXPORT_COMPILE_COMMANDS))
					{
						commands.push_back(
						{
							.dir = globalData.targetProfile.buildPath,
							.command = RemoveFromString(perFileCommand, "\"", true),
							.file = s,
							.output = objPath.string()
						});
					}

					//only recompile this object file when source and object timestamp differences occur
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
							"Skipping compilation of existing object file '" + objPath.string() + "'.",
							"LANGUAGE_C_CPP",
							LogType::LOG_INFO);

						Log::Print("\n");
					}

					lockwait_m(m_compiledObj);
					compiledObj.push_back(objPath);
					unlock_m(m_compiledObj);
				};

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

			string_view compiler{};
			if (globalData.targetProfile.binaryType == BinaryType::B_EXECUTABLE
				|| globalData.targetProfile.binaryType == BinaryType::B_SHARED)
			{
				EnumToString(globalData.targetProfile.compiler, KalaMakeCore::GetCompilerTypes(), compiler);
			}
			else
			{
#ifdef _WIN32
				compiler = "lib";
#else
				compiler = "ar rcs";
#endif
			}

#if _WIN32
			if (globalData.targetProfile.targetType == TargetType::T_LINUX)
			{
				if (compiler == "gcc")      compiler = gcc_compiler_windows_to_linux;
				else if (compiler == "g++") compiler = gpp_compiler_windows_to_linux;
			}
#else
			if (globalData.targetProfile.targetType == TargetType::T_WINDOWS)
			{
				if (compiler == "gcc")      compiler = gcc_compiler_linux_to_windows;
				else if (compiler == "g++") compiler = gpp_compiler_linux_to_windows;
			}
#endif

			command += string(compiler);

			if (compiler == "zig")
			{
				StandardType standardType = globalData.targetProfile.standard;

				if (standardType == StandardType::C_89
					|| standardType == StandardType::C_99
					|| standardType == StandardType::C_11
					|| standardType == StandardType::C_17
					|| standardType == StandardType::C_23
					|| standardType == StandardType::C_LATEST)
				{
					command += " c";
				}
				else if (standardType == StandardType::CPP_03
					|| standardType == StandardType::CPP_11
					|| standardType == StandardType::CPP_14
					|| standardType == StandardType::CPP_17
					|| standardType == StandardType::CPP_20
					|| standardType == StandardType::CPP_23
					|| standardType == StandardType::CPP_LATEST)
				{
					command += " c++";
				}
			}

			//set target type

			if (compiler == "clang"
				|| compiler == "clang++"
				|| compiler == "zig")
			{
#if _WIN32
				if (globalData.targetProfile.targetType == TargetType::T_LINUX)
				{
					command += " -target " + string(clang_zig_target_windows_to_linux);
				}
#else
				if (globalData.targetProfile.targetType == TargetType::T_WINDOWS)
				{
					if (ContainsValue(globalData.targetProfile.customFlags, CustomFlag::F_USE_CLANG_ZIG_MSVC))
					{
						command += " -target " + string(clang_zig_target_linux_to_windows_msvc);
					}
					else command += " -target " + string(clang_zig_target_linux_to_windows_gnu);
				}
#endif
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

			string extension{};
			if (globalData.targetProfile.binaryType == BinaryType::B_EXECUTABLE)
			{
#ifdef _WIN32
				if (globalData.targetProfile.targetType == TargetType::T_LINUX)
				{
					
					if (binaryName.ends_with(".exe")) binaryName.resize(binaryName.size() - 4);
				}
				else
				{
					if (!binaryName.ends_with(".exe")) extension = ".exe";
				}
#else
				if (globalData.targetProfile.targetType == TargetType::T_WINDOWS)
				{
					if (!binaryName.ends_with(".exe")) extension = ".exe";
				}
				else
				{
					if (binaryName.ends_with(".exe")) binaryName.resize(binaryName.size() - 4);
				}
#endif
			}
			else if (globalData.targetProfile.binaryType == BinaryType::B_SHARED)
			{
#ifdef _WIN32
				if (globalData.targetProfile.targetType == TargetType::T_LINUX)
				{
					if (!binaryName.ends_with(".so")) extension = ".so";
				}
				else
				{
					if (!binaryName.ends_with(".dll")) extension = ".dll";
				}
#else
				if (globalData.targetProfile.targetType == TargetType::T_WINDOWS)
				{
					if (!binaryName.ends_with(".dll")) extension = ".dll";
				}
				else
				{
					if (!binaryName.ends_with(".so")) extension = ".so";
				}
#endif
			}
			else if (globalData.targetProfile.binaryType == BinaryType::B_STATIC)
			{
#ifdef _WIN32
				if (globalData.targetProfile.targetType == TargetType::T_LINUX)
				{
					if (!binaryName.ends_with(".a")) extension = ".a";
				}
				else
				{
					if (!binaryName.ends_with(".lib")) extension = ".lib";
				}
#else
				if (globalData.targetProfile.targetType == TargetType::T_WINDOWS)
				{
					if (!binaryName.ends_with(".lib")) extension = ".lib";
				}
				else
				{
					if (!binaryName.ends_with(".a")) extension = ".a";
				}
#endif
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

				Log::Print("\n");

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
					"Skipping linking of up to date output '" + outputPath.string() + "'.",
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
	// EXPORT COMPILE COMMANDS
	//

	if (ContainsValue(globalData.targetProfile.customFlags,CustomFlag::F_EXPORT_COMPILE_COMMANDS))
	{
		Log::Print("\n===========================================================================\n");

		Log::Print(
			"Starting to create compile_commands.json.",
			"LANGUAGE_C_CPP",
			LogType::LOG_INFO);

		path compComm = globalData.targetProfile.buildPath / "compile_commands.json";

		if (exists(compComm))
		{
			string errorMsg = DeletePath(compComm);

			if (!errorMsg.empty())
			{
				KalaMakeCore::CloseOnError(
					"LANGUAGE_C_CPP",
					"Failed to remove existing compile_commands.json! Reason: " + errorMsg);
			}
		}

		ostringstream out{};

		out << "[\n";

		auto fix_msvc_slash = [&isMSVC](string_view input) -> string
			{
				return isMSVC
					? ReplaceFromString(string(input), "\\", "\\\\", true)
					: string(input);
			};

		auto fix_json_quotes = [](string_view input) -> string
			{
				return ReplaceFromString(string(input), "\"", "\\\"", true);
			};

		for (size_t i = 0; i < commands.size(); ++i)
		{
			const CompileCommand& s = commands[i];

			string cleanDir = fix_json_quotes(fix_msvc_slash(s.dir.string()));
			string cleanComm = fix_json_quotes(fix_msvc_slash(s.command));
			string cleanFile = fix_json_quotes(fix_msvc_slash(s.file.string()));
			string cleanOut = fix_json_quotes(fix_msvc_slash(s.output));

			out << "{\n";
			out << "  \"directory\": \"" << cleanDir  << "\",\n";
			out << "  \"command\": \""   << cleanComm << "\",\n";
			out << "  \"file\": \""      << cleanFile << "\",\n";
			out << "  \"output\": \""    << cleanOut  << "\"\n";
			out << "}";

			if (i + 1 < commands.size()) out << ",";

			out << "\n";
		}

		out << "]";

		string errorMsg = CreateNewFile(
			compComm,
			FileType::FILE_TEXT,
			{ .inText = out.str() });

		if (!errorMsg.empty())
		{
			KalaMakeCore::CloseOnError(
				"LANGUAGE_C_CPP",
				"Failed to create new compile_commands.json! Reason: " + errorMsg);
		}

		Log::Print("\n");

		Log::Print(
			"Finished creating compile_commands.json!",
			"LANGUAGE_C_CPP",
			LogType::LOG_SUCCESS);
	}

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
					"Failed to run action!");
			}
		}

		Log::Print("\n");

		Log::Print(
			"Finished all post build actions!",
			"LANGUAGE_C_CPP",
			LogType::LOG_SUCCESS);
	}
}

void Generate_Final(const GlobalData& globalData)
{
	
}