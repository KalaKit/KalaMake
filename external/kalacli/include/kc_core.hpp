//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <filesystem>

#include "core_utils.hpp"

namespace KalaCLI
{
	using std::string;
	using std::string_view;
	using std::vector;
	using std::pair;
	using std::function;
	using std::filesystem::path;
	
	//Prefix required in front of all commands
    static constexpr string_view COMMAND_PREFIX = "--";

    //How many newest lines to keep if console or external application keeps adding lines
    static constexpr u32 MAX_PAGE_LINES = 10000;

	//How many characters can be written into the text box
	static constexpr u32 MAX_INPUT_LENGTH = 1000;
    //How far back to store typed text history
	//after enter was pressed and text was cleared from input box
    static constexpr u32 MAX_TYPED_TEXT_HISTORY = 100;

    enum class PageDirection : u8
    {
        D_UP = 0,  //nth row from bottom
        D_DOWN = 1 //nth row from top
    };

    enum class TextRange : u8
    {
        R_ALL             = 0, //all content in page box
        R_VISIBLE         = 1, //content currently visible in page box
		R_HIGHLIGHTED_ROW = 2  //content within hovered row in page view
    };

	struct LIB_API Command
	{
		//Can this command also be ran in headless mode without the full CLI
		bool allowInHeadlessMode{};

		//What parameter to use to call this command
		string primaryParam{};

		//The description of this command that is listed when the built-in 'info' command is called
		string description{};

		//The function that contains the action this command will do,
		//must contain vector<string> as its only parameter to be able to receive user-passed parameters
		function<void(const vector<string>&)> targetFunction{};
	};

	class LIB_API KalaCLICore
	{
	public:
		static bool IsVerboseLoggingEnabled();
		static void SetVerboseLoggingState(bool newValue);

		static path GetExePath();

		//Returns max character count in page row
		static u32 GetRowSize();
		//Returns row count in page visible area
		static u32 GetRowCount();
		//Returns the global or visible range row depending on text range state,
		//cannot be used with R_HIGHLIGHTED_ROW,
		//returns 0 if not in page view mode
		static u32 GetHighlightedRow(TextRange textRange);

		static bool CanConsoleWriteToPage();
		//Should new log messages be appended at the bottom of existing page content,
		//if disabled then console only allows to draw what the current page contains
		static void SetConsoleWritesToPageState(bool state);

		static bool GetWrapState();
		//Toggles page view text wrapping on and off, defaults to true
		static void SetWrapState(bool state);

		//Choose which rows from top and bottom count as scrollable area,
		//rows in between scroll range count as final scrollable area
		//so if outside rows are not overwritten once per frame then they will appear blank.
		//Set to { 0, 0 } to reset to full range.
		//Does not need to be called every frame, range scales dynamically with resize,
		//gets ignored if its top or bottom exceeds total page box height
		static void SetPageScrollRange(pair<u32, u32> range);

		//Choose what to display at the top bar above the table
		static void SetPageTitle(string_view title);

		//Returns all page content if text range is set to R_ALL,
		//returns all page content within visible page box area
		//if text range is set to R_VISIBLE,
		//returns content on highlighted row if in page view mode
		//and if text range is set to R_HIGHLIGHTED_ROW
		static vector<string> GetPageContent(TextRange textRange = TextRange::R_ALL);
		//Decide what to display in the active page,
		//clears all old data and stores new data
		static void SetPageContent(const vector<string>& content);

		//Creates a new row after the last row and writes to it,
		//or overwrites last row if row count is maxed out,
		//sends to STDOUT if UpdateDisplayedContent hasn't been called at least once,
		//set SetConsoleWritesToPageState to true to see those log messages
		static void AppendToPage(string_view content);

		//Overwrites specific row within visible page area.
		//If page direction is down then counts down from top row,
		//if page direction is up then counts up from bottom row,
		//target row is the row which will be overwritten.
		//Does not need to be called every frame because
		//the overwritten row is stored in a separate internal container,
		//store as empty to same target row to remove from saved overwritten rows,
		//does not get drawn and draws other regular in its place
		//if its range exceeds total page box height
		static void OverwriteRow(
			const string& content,
			PageDirection pageDirection,
			u32 targetRow);

		//Choose what to do when enter is pressed over a row while in page read mode,
		//Returns highlighted R_VISIBLE row and text on that row,
		//changing to empty restores original operation where it copies highlighted page view row to input field
		static void SetHighlightedRowEnterAction(function<void(u32, string&)> action);

		//Choose what to do when a prefixless command is sent to the CLI,
		//great for things like chat CLI or server terminal,
		//changing to empty restores original operation where it simply clears input box
		static void SetPrefixlessInputAction(function<void(string&)> action);

		//Alternative to manually typing a command,
		//commands that start with COMMAND_PREFIX are sent to CLI internal command parser,
		//writing a message without a command prefix writes it to the CLI like a normal message.
		//Built in CLI commands:
		//- --h: lists all available commands and what they do,
		//- --c: clears all CLI page messages,
		//- --cmd command: sends selected message as command to console,
		//- --ec: enables console-based updates,
		//- --dc: disables console-based updates,
		//- --spt title: updates page title,
		//- --cp: copies page content to clipboard
		//- --e: exit the CLI
		static void SendCommand(string_view command);

		//Add a new command
		static void AddCommand(Command&& command);

		//Call once per frame,
		//draws current content,
		//updates values for GetRowCount, GetRowSize
		static void UpdateDisplayedContent();

		//Use this when you absolutely need a hard crash at this very moment.
		//Aborts and doesn't clean up data
		KNORETURN
		static void ForceClose(
			const string& title,
			const string& reason);
	};
}
