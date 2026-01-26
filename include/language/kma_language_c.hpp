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

	enum class C_Standard_Type : u8
	{
		S_INVALID = 0,

		S_C89 = 1,
		S_C99 = 2,
		S_C11 = 3,
		S_C17 = 4,
		S_C23 = 5,
		S_CLATEST = 6
	};

	class Language_C : public LanguageCore
	{
	public:
		static bool IsCStandard(string_view value);

		static unique_ptr<Language_C> Initialize(CompileData data);

		bool Compile(vector<CompileFlag> compileFlags = {}) override;
	};
}