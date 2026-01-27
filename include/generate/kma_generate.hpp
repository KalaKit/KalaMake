//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "KalaHeaders/core_utils.hpp"

namespace KalaMake::Generate
{
	using std::string_view;
	using std::unordered_map;

	using KalaHeaders::KalaCore::EnumHash;

	using u8 = uint8_t;

	//Allowed generator types that can be added generated with the --generate command
	enum class GeneratorType : u8
	{
		G_INVALID = 0,

		//generates the visual studio sln and vcxproj files
		G_VS = 1,
		//generates the visual studio code solution files
		G_VSCODE = 2,
		//generatees the ninja solution file
		G_NINJA = 3
	};

	class GenerateCore
	{
	public:
		static bool IsGeneratorType(string_view value);

		static const unordered_map<GeneratorType, string_view, EnumHash<GeneratorType>>& GetGeneratorTypes();
	};
}