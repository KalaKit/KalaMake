//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include "log_utils.hpp"

#include "kc_core.hpp"
#include "kc_command.hpp"

#include "core/kma_core.hpp"

using KalaHeaders::KalaLog::Log;

using KalaCLI::Core;
using KalaCLI::CommandManager;

using KalaMake::Core::KalaMakeCore;

using std::vector;
using std::string;

static void AddExternalCommands()
{
	auto compile = [](const vector<string>& params)
		{
			KalaMakeCore::OpenFile(params);
		};
	auto version = [](const vector<string>& params)
		{
			Log::Print("KalaMake 1.0");
		};

	CommandManager::AddCommand(
		{
			.primaryParam = "compile",
			.description =
				"Compile a project from a kalamake file, "
				"second parameter must be valid path to a .kmake file, "
				"third parameter must be a valid profile in the .kmake file.",
			.paramCount = 3,
			.targetFunction = compile
		});

	CommandManager::AddCommand(
		{
			.primaryParam = "version",
			.description = "Prints current KalaMake version",
			.paramCount = 1,
			.targetFunction = version
		});
}

int main(int argc, char* argv[])
{
	Core::Run(argc, argv, AddExternalCommands);

	return 0;
}