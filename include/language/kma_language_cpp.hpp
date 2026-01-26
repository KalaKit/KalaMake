//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#pragma once

#include <string>
#include <memory>

#include "language/kma_language.hpp"

namespace KalaMake::Language
{
	using std::string_view;
	using std::unique_ptr;

	using u8 = uint8_t;

	enum class CPP_Standard_Type : u8
	{
		S_INVALID = 0,

		S_CPP11 = 1,
		S_CPP14 = 2,
		S_CPP17 = 3,
		S_CPP20 = 4,
		S_CPP23 = 5,
		S_CPP26 = 6,
		S_CPPLATEST = 7
	};

	class Language_CPP : public LanguageCore
	{
	public:
		static bool IsCPPStandard(string_view value);

		static unique_ptr<Language_CPP> Initialize(CompileData data);

		bool Compile(vector<CompileFlag> compileFlags = {}) override;
	};
}