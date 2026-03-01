//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#pragma once

#include "core/kma_core.hpp"

namespace KalaMake::Language
{
	using KalaMake::Core::GlobalData;

	class Language_C_CPP
	{
	public:
		static void Compile(GlobalData& globalData);
	};
}