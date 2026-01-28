//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#pragma once

#include "language/kma_language.hpp"

namespace KalaMake::Language
{
	class Language_C_CPP : public LanguageCore
	{
	public:
		static Language_C_CPP* Initialize(GlobalData data);

		static bool IsValidC_CPPCompiler(string_view value);

		bool Compile() override;

		bool IsValidCompiler(CompilerType compiler) override;
		bool IsValidStandard(StandardType standard) override;
		bool AreValidSources(const vector<string>& sources) override;
		bool AreValidHeaders(const vector<string>& headers) override;
		bool AreValidLinks(const vector<string>& links) override;
	};
}