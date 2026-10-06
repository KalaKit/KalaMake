//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#pragma once

#include <string>
#include <vector>
#include <functional>

#include "core_utils.hpp"

namespace KalaCLI
{
	using std::string;
	using std::string_view;
	using std::vector;
	using std::function;

	//The prefix that must be in front of the primary parameter,
	//for example '--help', leave empty if you dont want a required prefix
	static constexpr string_view CLI_COMMAND_PREFIX = "--";

	struct LIB_API Command
	{
		//What parameter to use to call this command
		string primaryParam{};

		//The description of this command that is listed when the built-in 'info' command is called
		string description{};

		//Reference to the target function you want this command to call,
		//must contain vector<string> as its only parameter to be able to receive user-passed parameters
		function<void(const vector<string>&)> targetFunction{};
	};

	class LIB_API CLI
	{
	public:
		//Owning loop, call once, should not be ran together with TUI::Run
		static void Run(
			int argc,
			char* argv[],
			function<void()> AddExternalCommands);

		KNODISCARD
		static vector<Command>& GetCommands();

		//Parse given strings from end user
		KNODISCARD
		static bool ParseCommand(const vector<string>& params);

		//Add new command to commands list
		KNODISCARD
		static bool AddCommand(Command newValue);

		//Built-in command for listing all commands
		static void Command_Help(const vector<string>& params);
		//Built-in command for listing info about chosen command
		static void Command_Info(const vector<string>& params);

		//Built-in command for listing current path
		static void Command_Where(const vector<string>& params);
		//Built-in command for listing all files and folders in current dir
		static void Command_List(const vector<string>& params);
		//Built-in command for going to target path
		static void Command_Go(const vector<string>& params);

		//Built-in command for creating a directory at the target path
		static void Command_CreateDir(const vector<string>& params);
		//Built-in command for renaming the file or directory at the target path
		static void Command_Rename(const vector<string>& params);
		//Built-in command for deleting the file or directory at the target path
		static void Command_Delete(const vector<string>& params);
		//Built-in command for moving the file or directory from the origin to the target path,
		//the file or directory at the target path is overridden if it already exists
		static void Command_Move(const vector<string>& params);
		//Built-in command for copying the file or directory from the origin to the target path,
		//the copy action bails if a file or directory already exists at the target path
		static void Command_Copy(const vector<string>& params);
		//Built-in command for copying the file or directory from the origin to the target path,
		//the file or directory at the target path is overridden if it already exists
		static void Command_ForceCopy(const vector<string>& params);

		//Built-in command for cleaning console commands
		static void Command_Clear(const vector<string>& params);
		//Built-in command for closing the cli
		static void Command_Exit(const vector<string>& params);
	};
}