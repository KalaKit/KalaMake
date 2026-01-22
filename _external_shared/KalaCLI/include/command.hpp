//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#pragma once

#include <string>
#include <vector>
#include <functional>

#include "KalaHeaders/core_utils.hpp"
#include "KalaHeaders/math_utils.hpp"

namespace KalaCLI
{
	using std::string;
	using std::string_view;
	using std::vector;
	using std::function;

	//The prefix that must be in front of the primary parameter,
	//for example '--help', leave empty if you dont want a required prefix
	constexpr string_view COMMAND_PREFIX = "--";

	struct LIB_API Command
	{
		//Variants for the primary keyword that determines the desired action,
		//for example 'help' and 'h'. You should always add atleast one
		vector<string> primary{};

		//The description of this command that is listed when the built-in 'info' command is called
		string description{};

		//The count of how many params total this command must contain.
		//Range should be 1-255
		u8 paramCount{};

		//Reference to the target function you want this command to call,
		//must contain vector<string> as its only parameter to be able to receive user-passed parameters
		function<void(const vector<string>&)> targetFunction{};
	};

	class LIB_API CommandManager
	{
	public:
		static vector<Command>& GetCommands();

		//Parse given strings from end user
		static bool ParseCommand(const vector<string>& params);

		//Add new command to commands list
		static bool AddCommand(Command newValue);
	};
}