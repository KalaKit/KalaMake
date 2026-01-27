//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#pragma once

#include <vector>
#include <string>
#include <filesystem>

#include "core/kma_core.hpp"

namespace KalaMake::Language
{
	using std::vector;
	using std::string;
	using std::filesystem::path;

	using KalaMake::Core::GlobalData;

	//default build directory path relative to kmaPath if buildpath is not added or filled
	static const path defaultBuildPath = "build";
	//default object directory path relative to kmaPath if objpath is not added or filled
	static const path defaultObjPath = "build/obj";

	class LanguageCore
	{
	public:
		virtual bool Parse(const vector<string>& lines) = 0;
		virtual bool Compile() = 0;

		virtual ~LanguageCore() = default;
	protected:
		GlobalData compileData{};
	};
}