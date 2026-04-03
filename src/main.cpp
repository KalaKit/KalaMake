//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include "log_utils.hpp"

#include "kc_core.hpp"
#include "kc_command.hpp"

#include "core/kma_core.hpp"

using KalaHeaders::KalaLog::Log;
using KalaHeaders::KalaLog::LogType;

using KalaCLI::Core;
using KalaCLI::CommandManager;

using KalaMake::Core::KalaMakeCore;

using std::vector;
using std::string;

static void AddExternalCommands()
{
	auto compile = [](const vector<string>& params)
		{
			if (params.size() == 1)
			{
				Log::Print(
					"Command 'compile' got no arguments! You must pass .kmake path and target profile!",
					"PARSE",
					LogType::LOG_ERROR,
					2);

				return;
			}
			if (params.size() == 2)
			{
				Log::Print(
					"Command 'compile' requires two arguments! You must pass .kmake path and target profile!",
					"PARSE",
					LogType::LOG_ERROR,
					2);

				return;
			}
			if (params.size() > 3)
			{
				Log::Print(
					"Command 'compile' only allows two arguments! You must pass .kmake path and target profile!",
					"PARSE",
					LogType::LOG_ERROR,
					2);

				return;
			}

			KalaMakeCore::OpenFile(params);
		};
	auto version = [](const vector<string>& params)
		{
			if (params.size() > 1)
			{
				Log::Print(
					"Command 'version' does not allow any arguments!",
					"PARSE",
					LogType::LOG_ERROR,
					2);

				return;
			}

			Log::Print("KalaMake 1.2");
		};

	CommandManager::AddCommand(
		{
			.primaryParam = "compile",
			.description =
				"Compile a project from a kalamake file, "
				"second parameter must be valid path to a .kmake file, "
				"third parameter must be a valid profile in the .kmake file.",
			.targetFunction = compile
		});

	CommandManager::AddCommand(
		{
			.primaryParam = "version",
			.description = "Prints current KalaMake version",
			.targetFunction = version
		});
}

int main(int argc, char* argv[])
{
	Core::Run(argc, argv, AddExternalCommands);

	return 0;
}