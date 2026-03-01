//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include <string>
#include <vector>
#include <filesystem>

#include "KalaHeaders/log_utils.hpp"

#include "language/kma_language_c_cpp.hpp"
#include "core/kma_core.hpp"

using KalaHeaders::KalaLog::Log;
using KalaHeaders::KalaLog::LogType;

using KalaMake::Core::KalaMakeCore;
using KalaMake::Language::GlobalData;
using KalaMake::Core::BinaryType;
using KalaMake::Core::CompilerType;
using KalaMake::Core::StandardType;
using KalaMake::Core::BuildType;
using KalaMake::Core::WarningLevel;
using KalaMake::Core::CustomFlag;

using std::string;
using std::string_view;
using std::vector;
using std::filesystem::path;
using std::filesystem::exists;
using std::filesystem::is_regular_file;

namespace KalaMake::Language
{
	constexpr string_view cl_ide_bat_2026 = "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat";
	constexpr string_view cl_build_bat_2026 = "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat";
	constexpr string_view cl_ide_bat_2022 = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat";
	constexpr string_view cl_build_bat_2022 = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat";

	static path foundCLPath{};

	void Language_C_CPP::Compile(GlobalData& globalData)
	{
		CompilerType compilerType = globalData.targetProfile.compiler;
		BinaryType binaryType = globalData.targetProfile.binaryType;

		//continue to static lib compilation function
		//since its very different from exe and shared lib
		if (binaryType == BinaryType::B_LINK_ONLY)
		{	
			//handle link here
		}

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
				if (!exists(target)
					|| is_directory(target)
					|| !is_regular_file(target)
					|| !target.has_extension())
				{
					return true;
				}

				if (!isCLanguage
					&& !isCPPLanguage)
				{
					return true;
				}

				string extension = target.extension();

				if (isCLanguage)
				{
					if (isSource
						&& target.extension() != ".c")
					{
						return true;
					}
					if (!isSource
						&& target.extension() != ".h")
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
					if (!isSource
						&& target.extension() != ".h"
						&& target.extension() != ".hpp")
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
					LogType::LOG_WARNING);

				sources.erase(it);
			}
			else ++it;
		}

		vector<path>& headers = globalData.targetProfile.headers;
		for (auto it = headers.begin(); it != headers.end();)
		{
			path target = *it;
			if (should_remove(target, false))
			{
				Log::Print(
					"Removed invalid header script path '" + target.string() + "'",
					"LANGUAGE_C_CPP",
					LogType::LOG_WARNING);

				headers.erase(it);
			}
			else ++it;
		}

		if (sources.empty())
		{
			KalaMakeCore::CloseOnError(
				"LANGUAGE_C_CPP",
				"No sources were remaining after cleaning source scripts list!");
		}

		Log::Print("\n@@@@@ completed compile parse");
	}
}