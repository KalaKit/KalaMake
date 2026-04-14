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
using KalaMake::Core::StartType;

using std::vector;
using std::string;

static void AddExternalCommands()
{
	auto command_compile = [](const vector<string>& params)
		{
			if (params.size() == 1)
			{
				Log::Print(
					"Command 'compile' got no arguments! You must pass a .kmake path and target profile!",
					"PARSE",
					LogType::LOG_ERROR,
					2);

				return;
			}
			if (params.size() == 2)
			{
				Log::Print(
					"Command 'compile' requires two arguments! You must pass a .kmake path and target profile!",
					"PARSE",
					LogType::LOG_ERROR,
					2);

				return;
			}
			if (params.size() > 3)
			{
				Log::Print(
					"Command 'compile' only allows two arguments! You must pass a .kmake path and target profile!",
					"PARSE",
					LogType::LOG_ERROR,
					2);

				return;
			}

			KalaMakeCore::OpenFile(StartType::S_COMPILE, params);
		};
	auto command_clean = [](const vector<string>& params)
		{
			if (params.size() == 1)
			{
				Log::Print(
					"Command 'clean' got no arguments! You must pass a .kmake path!",
					"PARSE",
					LogType::LOG_ERROR,
					2);

				return;
			}
			if (params.size() > 2)
			{
				Log::Print(
					"Command 'clean' only allows one argument! You must pass a .kmake path!",
					"PARSE",
					LogType::LOG_ERROR,
					2);

				return;
			}

			KalaMakeCore::OpenFile(StartType::S_CLEAN, params);
		};
	auto command_version = [](const vector<string>& params)
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

			Log::Print("KalaMake 1.4");
		};
	auto command_list_profiles = [](const vector<string>& params)
		{
			if (params.size() == 1)
			{
				Log::Print(
					"Command 'list-profiles' got no arguments! You must pass a .kmake path!",
					"PARSE",
					LogType::LOG_ERROR,
					2);

				return;
			}
			if (params.size() > 2)
			{
				Log::Print(
					"Command 'list-profiles' only allows one argument! You must pass a .kmake path!",
					"PARSE",
					LogType::LOG_ERROR,
					2);

				return;
			}

			KalaMakeCore::OpenFile(StartType::S_LIST_PROFILES, params);
		};

	CommandManager::AddCommand(
		{
			.primaryParam = "compile",
			.description =
				"Compile a project from a kalamake file, "
				"second parameter must be valid path to a .kmake file, "
				"third parameter must be a valid profile in the .kmake file.",
			.targetFunction = command_compile
		});

	CommandManager::AddCommand(
		{
			.primaryParam = "clean",
			.description = "Deletes all build directories from your target kalamake file path.",
			.targetFunction = command_clean
		});

	CommandManager::AddCommand(
		{
			.primaryParam = "version",
			.description = "Prints current KalaMake version.",
			.targetFunction = command_version
		});

	CommandManager::AddCommand(
		{
			.primaryParam = "list-profiles",
			.description =
				"Lists all profiles in a kalamake file, "
				"second parameter must be valid path to a .kmake file.",
			.targetFunction = command_list_profiles
		});
}

int main(int argc, char* argv[])
{
	Core::Run(argc, argv, AddExternalCommands);

	return 0;
}
