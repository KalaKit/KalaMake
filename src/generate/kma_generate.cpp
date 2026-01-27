//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include "generate/kma_generate.hpp"

using KalaHeaders::KalaCore::StringToEnum;

namespace KalaMake::Generate
{
	static const unordered_map<GeneratorType, string_view, EnumHash<GeneratorType>> generatorTypes =
	{
		{ GeneratorType::G_VS,     "vs" },
		{ GeneratorType::G_VSCODE, "vscode" },
		{ GeneratorType::G_NINJA,  "ninja" }
	};

	bool GenerateCore::IsGeneratorType(string_view value)
	{
		GeneratorType type{};

		return StringToEnum(value, generatorTypes, type)
			&& (type == GeneratorType::G_VS
			|| type == GeneratorType::G_VSCODE
			|| type == GeneratorType::G_NINJA);
	}

	const unordered_map<GeneratorType, string_view, EnumHash<GeneratorType>>& GenerateCore::GetGeneratorTypes() { return generatorTypes; }
}