//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include <unordered_map>

#include "language/kma_language.hpp"

using std::unordered_map;

namespace KalaMake::Language
{
	struct CompilerTypeHash
	{
		size_t operator()(CompilerType c) const noexcept { return scast<size_t>(c); }
	};

	static const unordered_map<CompilerType, string_view, CompilerTypeHash> compilerTypes =
	{
		{ CompilerType::C_CLANG_CL, "clang-cl" },
		{ CompilerType::C_CL,       "cl" },

		{ CompilerType::C_CLANG,    "clang" },
		{ CompilerType::C_CLANGPP,  "clang++" },
		{ CompilerType::C_GCC,      "gcc" },
		{ CompilerType::C_GPP,      "g++" }
	};
}