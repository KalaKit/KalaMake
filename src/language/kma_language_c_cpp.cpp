//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include <memory>

#include "language/kma_language_c_cpp.hpp"
#include "core/kma_core.hpp"

using KalaMake::Language::GlobalData;
using KalaMake::Core::BinaryType;

using std::string_view;
using std::unique_ptr;
using std::make_unique;

static bool CompileLinkOnly(const GlobalData& data);

namespace KalaMake::Language
{
	constexpr string_view cl_ide_bat_2026 = "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat";
	constexpr string_view cl_build_bat_2026 = "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat";
	constexpr string_view cl_ide_bat_2022 = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat";
	constexpr string_view cl_build_bat_2022 = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat";

	static path foundCLPath{};

	unique_ptr<Language_C_CPP> languageContext{};

	Language_C_CPP* Language_C_CPP::Initialize(GlobalData data)
	{
		if (languageContext) languageContext = nullptr;

		unique_ptr<Language_C_CPP> newLanguageContext = make_unique<Language_C_CPP>();

		return nullptr;
	}

	bool Language_C_CPP::Compile()
	{
		//continue to static lib compilation function
		//since its very different from exe and shared lib
		if (compileData.profile.binaryType == BinaryType::B_LINK_ONLY)
		{	
				return CompileLinkOnly(compileData);
		}

		vector<string> finalFlags = std::move(compileData.profile.flags);

		return true;
	}

	bool Language_C_CPP::IsValidCompiler(CompilerType compiler)
	{
		return false;
	}
	bool Language_C_CPP::IsValidStandard(StandardType standard)
	{
		return false;
	}
	bool Language_C_CPP::AreValidSources(const vector<string>& sources)
	{
		return false;
	}
	bool Language_C_CPP::AreValidHeaders(const vector<string>& headers)
	{
		return false;
	}
	bool Language_C_CPP::AreValidLinks(const vector<string>& links)
	{
		return false;
	}
}

bool CompileLinkOnly(const GlobalData& data)
{
	return false;
}