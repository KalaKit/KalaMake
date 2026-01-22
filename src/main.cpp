//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include "core.hpp"
#include "command.hpp"

#include "compile.hpp"

using KalaCLI::Core;
using KalaCLI::Command;
using KalaCLI::CommandManager;

using KalaMake::KalaMakeCore;

static void AddExternalCommands()
{
	Command cmd_compile
		{
			.primary = { "compile, cp" },
			.description = "Compile a project from a kma file, second parameter must be valid path.",
			.paramCount = 2,
			.targetFunction = KalaMakeCore::Compile
		};

	CommandManager::AddCommand(cmd_compile);
}

int main(int argc, char* argv[])
{
	Core::Run(argc, argv, AddExternalCommands);

	return 0;
}