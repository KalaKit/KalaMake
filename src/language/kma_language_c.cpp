//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include <unordered_map>

#include "language/kma_language_c.hpp"

using std::unordered_map;

#ifndef scast
	#define scast static_cast
#endif

namespace KalaMake::Language
{
	struct Standard_C_Hash
	{
		size_t operator()(C_Standard_Type s) const noexcept { return scast<size_t>(s); }
	};

	static const unordered_map<C_Standard_Type, string_view, Standard_C_Hash> c_standard_Types =
	{
		{ C_Standard_Type::S_C89,     "c89" },
		{ C_Standard_Type::S_C99,     "c99" },
		{ C_Standard_Type::S_C11,     "c11" },
		{ C_Standard_Type::S_C17,     "c17" },
		{ C_Standard_Type::S_C23,     "c23" },
		{ C_Standard_Type::S_CLATEST, "clatest" }
	};

	bool Language_C::IsCStandard(string_view value)
	{

	}
}