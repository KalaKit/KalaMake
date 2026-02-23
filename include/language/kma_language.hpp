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
	using std::string_view;
	using std::filesystem::path;

	using KalaMake::Core::GlobalData;
	using KalaMake::Core::CompilerType;
	using KalaMake::Core::StandardType;

	//default build directory path relative to kmaPath if buildpath is not added or filled
	static const path defaultBuildPath = "build";
	//default object directory path relative to kmaPath if objpath is not added or filled
	static const path defaultObjPath = "build/obj";

	class LanguageCore
	{
	public:
		static bool IsValidMSVCCompiler(string_view value);
		static bool IsValidGNUCompiler(string_view value);

		virtual bool Compile(string_view profileName) = 0;

		virtual bool IsValidCompiler(CompilerType compiler) = 0;
		virtual bool IsValidStandard(StandardType standard) = 0;
		virtual bool AreValidSources(const vector<string>& sources) = 0;
		virtual bool AreValidHeaders(const vector<string>& headers) = 0;
		virtual bool AreValidLinks(const vector<string>& links) = 0;

		virtual ~LanguageCore() = default;
	protected:
		GlobalData compileData{};
	};
}