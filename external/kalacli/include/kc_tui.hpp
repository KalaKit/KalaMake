//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#pragma once

#include <string>
#include <vector>
#include <functional>

#include "core_utils.hpp"

#include "kc_cli.hpp"

namespace KalaCLI
{
    using KalaCLI::Command;

    using std::string;
    using std::string_view;
    using std::vector;
    using std::function;

    //how many newest lines to keep if console or external application keeps adding lines
    static constexpr u32 MAX_PAGE_LINES = 1000;
    //how far back to store typed text history
    static constexpr u32 MAX_TYPED_TEXT_HISTORY = 100;

    //Used for hardcoded TUI commands
    static constexpr string_view TUI_COMMAND_PREFIX = "/";

    class LIB_API TUI
    {
    public:
        //Owning loop, call once, should not be ran together with KalaCLICore::Run
        static void Run();

        static bool CanConsoleWriteToPage();
        //Should new log messages be appended at the bottom of existing page content,
        //if disabled then console only allows to draw what the current page contains
        static void SetConsoleWritesToPageState(bool state);

        //Choose what to display at the top bar above the table
        static void SetPageTitle(string_view title);

        //Decide what to display in the active page
        static void SetPageContent(const vector<string>& content);
        //Add a new line at the bottom of the page
        static void AppendToPage(string_view line);

        //Alternative to manually typing a command,
        //commands that start with CLI_COMMAND_PREFIX are sent to kc_command.hpp ParseCommand,
        //commands that start with TUI_COMMAND_PREFIX are sent to kc_tui.hpp internal command parser,
        //writing a message without a command prefix writes it to the tui like a normal message
        //TUI commands:
        //  /help, /h: lists all available commands and what they do
        //  /clear, /c: clears all tui page messages
        //  /getclicommands, /gcc: lists all available cli commands and what they do
        //  /command command, /cmd command: sends selected message as command to console
        //  /enableconsole, /ec: enables console-based updates
        //  /disableconsole, /dc: disables console-based updates
        //  /setpagetitle title, /spt title: updates page title
        static void SendCommand(string_view command);

        //Add a new command
        static void AddCommand(Command&& command);

        //Choose what to do when a prefixless command is sent to the TUI,
        //great for things like chat TUI or server terminal,
        //changing to empty restores original operation where it simply sends message to page box
        static void SetPrefixlessTargetAction(function<void(string&)> action);

    private:
        static void UpdateDisplayedContent();
    };
}