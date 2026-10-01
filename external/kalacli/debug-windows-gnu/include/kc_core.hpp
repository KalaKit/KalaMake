//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#pragma once

#include <string>
#include <functional>

#include "core_utils.hpp"

namespace KalaCLI
{
	using std::string;
	using std::function;

	class LIB_API KalaCLICore
	{
	public:
		KNODISCARD
		static string& GetCurrentDir();

		static void Run(
			int argc,
			char* argv[],
			function<void()> AddExternalCommands);

		//Use this when you absolutely need a hard crash at this very moment.
		//Aborts and doesn't clean up data.
		KNORETURN
		static void ForceClose(
			const string& title,
			const string& reason);
	};
}
