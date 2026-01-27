//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#pragma once

#include <memory>

#include "language/kma_language.hpp"

namespace KalaMake::Language
{
	using std::unique_ptr;

	using u8 = uint8_t;

	class Language_C_CPP : public LanguageCore
	{
	public:
		static unique_ptr<Language_C_CPP> Initialize(GlobalData data);

		bool Parse(const vector<string>& lines) override;
		bool Compile() override;
	};
}