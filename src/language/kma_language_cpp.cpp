//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include <unordered_map>
#include <filesystem>
#include <sstream>

#include "KalaHeaders/log_utils.hpp"
#include "KalaHeaders/core_utils.hpp"
#include "KalaHeaders/string_utils.hpp"
#include "KalaHeaders/file_utils.hpp"

#include "language/kma_language_cpp.hpp"
#include "core/kma_core.hpp"

using KalaHeaders::KalaLog::Log;
using KalaHeaders::KalaLog::LogType;

using KalaHeaders::KalaCore::ContainsValue;
using KalaHeaders::KalaCore::ContainsDuplicates;
using KalaHeaders::KalaCore::RemoveDuplicates;

using KalaHeaders::KalaString::EndsWith;
using KalaHeaders::KalaString::EnumToString;

using KalaHeaders::KalaFile::CreateNewDirectory;

using KalaMake::Core::KalaMakeCore;
using KalaMake::Core::CompileData;

using std::unordered_map;
using std::ostringstream;
using std::filesystem::path;

#ifndef scast
	#define scast static_cast
#endif

static bool CompileLinkOnly(const CompileData& data);

namespace KalaMake::Language
{
	struct Standard_CPP_Hash
	{
		size_t operator()(CPP_Standard_Type s) const noexcept { return scast<size_t>(s); }
	};

	static const unordered_map<CPP_Standard_Type, string_view, Standard_CPP_Hash> cpp_standard_types =
	{
		{ CPP_Standard_Type::S_CPP11,     "c++11" },
		{ CPP_Standard_Type::S_CPP14,     "c++14" },
		{ CPP_Standard_Type::S_CPP17,     "c++17" },
		{ CPP_Standard_Type::S_CPP20,     "c++20" },
		{ CPP_Standard_Type::S_CPP23,     "c++23" },
		{ CPP_Standard_Type::S_CPP26,     "c++26" },
		{ CPP_Standard_Type::S_CPPLATEST, "c++latest" }
	};

	bool Language_CPP::IsCPPStandard(string_view value)
	{
		return ContainsValue(cpp_standard_types, value);
	}

	unique_ptr<Language_CPP> Language_CPP::Initialize(CompileData data)
	{
		return nullptr;
	}

	bool Language_CPP::Compile(vector<CompileFlag> compileFlags)
	{
		if (compileData.binaryType == BinaryType::B_LINK_ONLY)
		{
			//continue to static lib compilation function
			//since its very different from exe and shared lib
			return CompileLinkOnly(compileData);
		}

		vector<string> finalFlags = std::move(compileData.flags);
		vector<string> finalDebugFlags = std::move(compileData.debugflags);

		//
		// SOURCES
		//

		for (const auto& s : compileData.sources)
		{
			finalFlags.push_back("\"" + s + "\"");
			finalDebugFlags.push_back("\"" + s + "\"");
		}

		//
		// HEADERS
		//

		if (!compileData.headers.empty())
		{
			if (IsMSVCCompiler(compileData.compiler))
			{
				for (const auto& s : compileData.headers)
				{
					finalFlags.push_back("/I\"" + s + "\"");
					finalDebugFlags.push_back("/I\"" + s + "\"");
				}
			}
			else if (IsGNUCompiler(compileData.compiler))
			{
				for (const auto& s : compileData.headers)
				{
					finalFlags.push_back("-I\"" + s + "\"");
					finalDebugFlags.push_back("-I\"" + s + "\"");
				}
			}
		}

		//
		// LINKS
		//

		if (!compileData.links.empty())
		{
			if (IsMSVCCompiler(compileData.compiler))
			{
				for (const auto& l : compileData.links)
				{
					if (EndsWith(l, ".lib")) finalFlags.push_back("\"" + l + "\"");
					else                     finalFlags.push_back(l);
				}
			}
			else if (IsGNUCompiler(compileData.compiler))
			{
				for (const auto& l : compileData.links)
				{
					if (EndsWith(l, ".lib")
						|| EndsWith(l, ".a"))
					{
						finalFlags.push_back("\"" + l + "\"");
					}
					else finalFlags.push_back("-l" + l);
				}
			}
		}
		if (!compileData.debuglinks.empty())
		{
			if (IsMSVCCompiler(compileData.compiler))
			{
				for (const auto& l : compileData.debuglinks)
				{
					if (EndsWith(l, ".lib")) finalDebugFlags.push_back("\"" + l + "\"");
					else                     finalDebugFlags.push_back(l);
				}
			}
			else if (IsGNUCompiler(compileData.compiler))
			{
				for (const auto& l : compileData.debuglinks)
				{
					if (EndsWith(l, ".lib")
						|| EndsWith(l, ".a"))
					{
						finalDebugFlags.push_back("\"" + l + "\"");
					}
					else finalDebugFlags.push_back("-l" + l);
				}
			}
		}

		//
		// STANDARD
		//

		if (IsMSVCCompiler(compileData.compiler))
		{
			finalFlags.push_back("/std:" + compileData.standard);
			finalDebugFlags.push_back("/std:" + compileData.standard);
		}
		else if (IsGNUCompiler(compileData.compiler))
		{
			finalFlags.push_back("-std=" + compileData.standard);
			finalDebugFlags.push_back("-std=" + compileData.standard);
		}

		//
		// EXTENSIONS
		//

		//...

		//
		// WARNING LEVEL
		//

		if (IsMSVCCompiler(compileData.compiler))
		{
			if (compileData.warningLevel == WarningLevel::W_NONE)
			{
				finalFlags.push_back("/W0");
				finalDebugFlags.push_back("/W0");
			}
			else if (compileData.warningLevel == WarningLevel::W_BASIC)
			{
				finalFlags.push_back("/W1");
				finalDebugFlags.push_back("/W1");
			}
			else if (compileData.warningLevel == WarningLevel::W_NORMAL)
			{
				finalFlags.push_back("/W2");
				finalDebugFlags.push_back("/W2");
			}
			else if (compileData.warningLevel == WarningLevel::W_STRONG)
			{
				finalFlags.push_back("/W3");
				finalDebugFlags.push_back("/W3");
			}
			else if (compileData.warningLevel == WarningLevel::W_STRICT)
			{
				finalFlags.push_back("/W4");
				finalDebugFlags.push_back("/W4");
			}
			else if (compileData.warningLevel == WarningLevel::W_ALL)
			{
				finalFlags.push_back("/Wall");
				finalDebugFlags.push_back("/Wall");
			}
		}
		else if (IsGNUCompiler(compileData.compiler))
		{
			if (compileData.warningLevel == WarningLevel::W_NONE)
			{
				finalFlags.push_back("-w");
				finalDebugFlags.push_back("-w");
			}
			else if (compileData.warningLevel == WarningLevel::W_BASIC)
			{
				finalFlags.push_back("-Wall");
				finalDebugFlags.push_back("-Wall");
			}
			else if (compileData.warningLevel == WarningLevel::W_NORMAL)
			{
				finalFlags.push_back("-Wall");
				finalFlags.push_back("-Wextra");

				finalDebugFlags.push_back("-Wall");
				finalDebugFlags.push_back("-Wextra");
			}
			else if (compileData.warningLevel == WarningLevel::W_STRONG)
			{
				finalFlags.push_back("-Wall");
				finalFlags.push_back("-Wextra");
				finalFlags.push_back("-Wpedantic");

				finalDebugFlags.push_back("-Wall");
				finalDebugFlags.push_back("-Wextra");
				finalDebugFlags.push_back("-Wpedantic");
			}
			else if (compileData.warningLevel == WarningLevel::W_STRICT)
			{
				finalFlags.push_back("-Wall");
				finalFlags.push_back("-Wextra");
				finalFlags.push_back("-Wpedantic");
				finalFlags.push_back("-Wshadow");
				finalFlags.push_back("-Wconversion");
				finalFlags.push_back("-Wsign-conversion");

				finalDebugFlags.push_back("-Wall");
				finalDebugFlags.push_back("-Wextra");
				finalDebugFlags.push_back("-Wpedantic");
				finalDebugFlags.push_back("-Wshadow");
				finalDebugFlags.push_back("-Wconversion");
				finalDebugFlags.push_back("-Wsign-conversion");
			}
			else if (compileData.warningLevel == WarningLevel::W_ALL)
			{
				if (compileData.compiler == "clang"
					|| compileData.compiler == "clang++")
				{
					finalFlags.push_back("-Wall");
					finalFlags.push_back("-Wextra");
					finalFlags.push_back("-Wpedantic");
					finalFlags.push_back("-Weverything");

					finalDebugFlags.push_back("-Wall");
					finalDebugFlags.push_back("-Wextra");
					finalDebugFlags.push_back("-Wpedantic");
					finalDebugFlags.push_back("-Weverything");
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

					finalDebugFlags.push_back("-Wall");
					finalDebugFlags.push_back("-Wextra");
					finalDebugFlags.push_back("-Wpedantic");
					finalDebugFlags.push_back("-Wshadow");
					finalDebugFlags.push_back("-Wconversion");
					finalDebugFlags.push_back("-Wsign-conversion");
					finalDebugFlags.push_back("-Wcast-align");
					finalDebugFlags.push_back("-Wnull-dereference");
					finalDebugFlags.push_back("-Wdouble-promotion");
					finalDebugFlags.push_back("-Wformat=2");
				}
			}
		}

		//
		// DEFINES
		//

		if (!compileData.defines.empty())
		{
			for (const auto& d : compileData.defines)
			{
				finalFlags.push_back("-D" + d);
				finalDebugFlags.push_back("-D" + d);
			}
		}

		//
		// CUSTOM FLAGS
		//

		for (const auto& f : compileData.customFlags)
		{
			if (f == CustomFlag::F_STANDARD_REQUIRED
				&& IsMSVCCompiler(compileData.compiler))
			{
				finalFlags.push_back("/permissive-");
				finalDebugFlags.push_back("/permissive-");

				continue;
			}

			if (f == CustomFlag::F_WARNINGS_AS_ERRORS)
			{
				if (IsMSVCCompiler(compileData.compiler))
				{
					finalFlags.push_back("/WX");
					finalDebugFlags.push_back("/WX");
				}
				else if (IsGNUCompiler(compileData.compiler))
				{
					finalFlags.push_back("-Werror");
					finalDebugFlags.push_back("-Werror");
				}

				continue;
			}
		}

		//
		// BINARY TYPES
		//

		vector<string> buildFlags{};

		for (const auto& f : compileData.customFlags)
		{
			if (f == CustomFlag::F_BUILD_DEBUG)      buildFlags.push_back(CustomFlag::F_BUILD_DEBUG);
			if (f == CustomFlag::F_BUILD_RELEASE)    buildFlags.push_back(CustomFlag::F_BUILD_RELEASE);
			if (f == CustomFlag::F_BUILD_RELDEBUG)   buildFlags.push_back(CustomFlag::F_BUILD_RELDEBUG);
			if (f == CustomFlag::F_BUILD_MINSIZEREL) buildFlags.push_back(CustomFlag::F_BUILD_MINSIZEREL);
		}

		//enable standard C++ extensions for MSVC
		if (Language_CPP::IsCPPStandard(compileData.standard)
			&& IsMSVCCompiler(compileData.compiler))
		{
			finalFlags.push_back("/EHsc");
			finalDebugFlags.push_back("/EHsc");
		}

		//
		// BUILD PROJECT
		//

		auto get_build_path = [](
			string_view name,
			string_view compiler,
			BinaryType binaryType,
			const path& buildPath) -> string
			{
				if (binaryType == BinaryType::B_EXECUTABLE)
				{
					if (IsMSVCCompiler(compiler))
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
					else if (IsGNUCompiler(compiler))
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
				else if (binaryType == BinaryType::B_LINK_RUNTIME)
				{
					if (IsMSVCCompiler(compiler))
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
					else if (IsGNUCompiler(compiler))
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
				else if (binaryType == BinaryType::B_RUNTIME_ONLY)
				{
					if (IsMSVCCompiler(compiler))
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
					else if (IsGNUCompiler(compiler))
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
						&& ContainsValue(flags, "/Od")
						&& ContainsValue(flags, "/Zi"))
					{
						return "debug";
					}

					if (flagsType == "release")
					{
						if (ContainsValue(flags, "/O2")
							&& ContainsValue(flags, "/Zi"))
						{
							return "reldebug";
						}
						else if (ContainsValue(flags, "/O2"))
						{
							return "release";
						}
						else if (ContainsValue(flags, "/O1"))
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
				else if (compiler == "clang"
					|| compiler == "clang++"
					|| compiler == "gcc"
					|| compiler == "g++")
				{
					if (flagsType == "debug"
						&& ContainsValue(flags, "-O0")
						&& ContainsValue(flags, "-g"))
					{
						return "debug";
					}

					if (flagsType == "release")
					{
						if (ContainsValue(flags, "-O2")
							&& ContainsValue(flags, "-g"))
						{
							return "reldebug";
						}
						else if (ContainsValue(flags, "-O2"))
						{
							return "release";
						}
						else if (ContainsValue(flags, "-Os"))
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
					string clPath = KalaMakeCore::GetFoundCLPath().string();

					Log::Print(
						"Found valid vcvars64.bat from '" + clPath + "'.",
						"KALAMAKE",
						LogType::LOG_INFO);

					finalValue =
						"cmd /c \"\""
						+ clPath
						+ "\" && cl "
						+ oss.str() + "\"";
				}
				else if (compiler == "clang-cl") finalValue = "clang-cl " + oss.str();
				else if (compiler == "clang")    finalValue = "clang " + oss.str();
				else if (compiler == "clang++")  finalValue = "clang++ " + oss.str();
				else if (compiler == "gcc")      finalValue = "gcc " + oss.str();
				else if (compiler == "g++")      finalValue = "g++ " + oss.str();

				if (system(finalValue.c_str()) != 0)
				{
					KalaMakeCore::PrintError("Compilation failed!");

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
					compileData.binaryType,
					path(compileData.buildPath) / b);

				if (ContainsString(finalBuildPath, "Result: "))
				{
					KalaMakeCore::PrintError("Failed to create new directory for compiled file target path missing parent path! " + finalBuildPath);
					return;
				}

				if (b != "debug")
				{
					if (compileData.compiler == "clang-cl"
						|| compileData.compiler == "cl")
					{
						if (b == "release")
						{
							finalFlags.push_back("/O2");
						}
						else if (b == "reldebug")
						{
							finalFlags.push_back("/O2");
							finalFlags.push_back("/Zi");
						}
						else if (b == "minsizerel")
						{
							finalFlags.push_back("/O1");
						}

						finalFlags.push_back("/MD");
					}
					else if (compileData.compiler == "clang"
						|| compileData.compiler == "clang++"
						|| compileData.compiler == "gcc"
						|| compileData.compiler == "g++")
					{
						if (b == "release")
						{
							finalFlags.push_back("-O2");
						}
						else if (b == "reldebug")
						{
							finalFlags.push_back("-O2");
							finalFlags.push_back("-g");
						}
						else if (b == "minsizerel")
						{
							finalFlags.push_back("-Os");
						}
					}

					if (!build_project(
						compileData.compiler,
						finalBuildPath,
						finalFlags))
					{
						return;
					}
				}
				else
				{
					if (compileData.compiler == "clang-cl"
						|| compileData.compiler == "cl")
					{
						finalDebugFlags.push_back("/Od");
						finalDebugFlags.push_back("/Zi");
						finalDebugFlags.push_back("/MDd");
					}
					else if (compileData.compiler == "clang"
						|| compileData.compiler == "clang++"
						|| compileData.compiler == "gcc"
						|| compileData.compiler == "g++")
					{
						finalDebugFlags.push_back("-O0");
						finalDebugFlags.push_back("-g");
					}

					if (!build_project(
						compileData.compiler,
						finalBuildPath,
						finalDebugFlags))
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
				compileData.flags);

			//build in any release mode
			{
				//adds build flag as build folder between build path and binary name
				string finalBuildPath = get_build_path(
					compileData.name,
					compileData.compiler,
					compileData.binaryType,
					path(compileData.buildPath) / relbuildType);

				if (ContainsString(finalBuildPath, "Result: "))
				{
					KalaMakeCore::PrintError("Failed to create new directory for compiled file target path missing parent path! " + finalBuildPath);
					return;
				}

				if (!build_project(
					compileData.compiler,
					finalBuildPath,
					finalFlags))
				{
					return;
				}
			}

			if (!compileData.debugflags.empty())
			{
				string debbuildType = get_build_type(
					"debug",
					compileData.compiler,
					compileData.debugflags);

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
						KalaMakeCore::PrintError("Failed to create new directory for compiled file target path missing parent path! " + finalBuildPath);
						return;
					}

					if (!build_project(
						compileData.compiler,
						finalBuildPath,
						finalDebugFlags))
					{
						return;
					}
				}
			}
		}
	}
}

bool CompileLinkOnly(const CompileData& data)
{
	return false;
}