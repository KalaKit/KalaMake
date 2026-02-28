//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include "core.hpp"
#include "command.hpp"

#include "core/kma_core.hpp"

using KalaCLI::Core;
using KalaCLI::CommandManager;

using KalaMake::Core::KalaMakeCore;
using KalaMake::Core::TargetState;

using std::vector;
using std::string;

static void AddExternalCommands()
{
	auto compile = [](const vector<string>& params)
		{
			KalaMakeCore::OpenFile(params, TargetState::S_COMPILE);
		};
	auto generate = [](const vector<string>& params)
		{
			KalaMakeCore::OpenFile(params, TargetState::S_GENERATE);
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
			.primaryParam = "generate",
			.description =
				"Generate a solution file from a kalamake file, "
				"second parameter must be valid path to a .kmake file, "
				"third parameter must be a valid profile in the .kmake file, "
				"fourth parameter must be a solution type.",
			.paramCount = 4,
			.targetFunction = generate
		});
}

int main(int argc, char* argv[])
{
	Core::Run(argc, argv, AddExternalCommands);

	return 0;
}