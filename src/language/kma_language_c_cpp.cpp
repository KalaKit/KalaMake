//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include "language/kma_language_c_cpp.hpp"
#include "core/kma_core.hpp"

using KalaMake::Language::GlobalData;

static bool CompileLinkOnly(const GlobalData& data);

namespace KalaMake::Language
{
	unique_ptr<Language_C_CPP> Language_C_CPP::Initialize(GlobalData data)
	{
		return nullptr;
	}

	bool Language_C_CPP::Parse(const vector<string>& lines)
	{
		return false;
	}

	bool Language_C_CPP::Compile()
	{
		if (compileData.binaryType == BinaryType::B_LINK_ONLY)
		{
			//continue to static lib compilation function
			//since its very different from exe and shared lib
			return CompileLinkOnly(compileData);
		}

		vector<string> finalFlags = std::move(compileData.flags);

		return false;
	}
}

bool CompileLinkOnly(const GlobalData& data)
{
	return false;
}