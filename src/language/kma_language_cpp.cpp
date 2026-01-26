//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include <unordered_map>

#include "language/kma_language_cpp.hpp"

using std::unordered_map;

#ifndef scast
	#define scast static_cast
#endif

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
}