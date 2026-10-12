//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include <chrono>
#include <thread>

#include "kc_core.hpp"

#include "core/kma_core.hpp"

using KalaCLI::KalaCLICore;
using KalaCLI::COMMAND_PREFIX;

using KalaMake::Core::KalaMakeCore;
using KalaMake::Core::StartType;

using std::string;
using std::string_view;
using std::vector;
using std::chrono::milliseconds;
using std::this_thread::sleep_for;

static constexpr string_view KALAMAKE_VERSION = "2.0.0";

static void Command_Version(const vector<string>& _)
{
    KalaCLICore::AppendToPage("KalaMake " + string(KALAMAKE_VERSION));
}
static void Command_Validate(const vector<string>& params)
{
    if (params.size() < 2)
    {
        KalaCLICore::AppendToPage("ERROR: Not enough arguments! You must pass a .kmake path and target profile.");

        return;
    }
    else if (params.size() > 2)
    {
        KalaCLICore::AppendToPage("ERROR: Too many arguments! You can only pass a .kmake path and target profile.");

        return;
    }
    
    KalaMakeCore::OpenFile(StartType::S_VALIDATE, params);
}
static void Command_List_Profiles(const vector<string>& params)
{
    if (params.size() != 1)
    {
        KalaCLICore::AppendToPage("ERROR: Invalid amount of arguments! You can only pass a .kmake path.");

        return;
    }

    KalaMakeCore::OpenFile(StartType::S_LIST_PROFILES, params);
}
static void Command_Clean(const vector<string>& params)
{
    if (params.size() != 1)
    {
        KalaCLICore::AppendToPage("ERROR: Invalid amount of arguments! You can only pass a .kmake path.");

        return;
    }

    KalaMakeCore::OpenFile(StartType::S_CLEAN, params);
}
static void Command_Compile(const vector<string>& params)
{
    if (params.size() < 2)
    {
        KalaCLICore::AppendToPage("ERROR: Not enough arguments! You must pass a .kmake path and target profile.");

        return;
    }
    else if (params.size() > 2)
    {
        KalaCLICore::AppendToPage("ERROR: Too many arguments! You can only pass a .kmake path and target profile.");

        return;
    }
    
    KalaMakeCore::OpenFile(StartType::S_COMPILE, params);
}

int main(int argc, char* argv[])
{
    //all commands require -- prefix
    KalaCLICore::SetPrefixlessInputAction([](string& _)
        {
            KalaCLICore::AppendToPage("ERROR: You must add the '" + string(COMMAND_PREFIX) + "' prefix to commands!");
        });

    KalaCLICore::AddCommand(
        {
            .allowInHeadlessMode = true,
            .primaryParam = "version",
            .description =
                "Prints current KalaMake version.",
            .targetFunction = [](const vector<string>& params) { Command_Version(params); }
        });

    KalaCLICore::AddCommand(
        {
            .allowInHeadlessMode = true,
            .primaryParam = "validate",
            .description =
                "Validates a kalamake file, "
				"second parameter must be valid path to a .kmake file, "
				"third parameter must be a valid profile in the .kmake file.",
            .targetFunction = [](const vector<string>& params) { Command_Validate(params); }
        });

    KalaCLICore::AddCommand(
        {
            .allowInHeadlessMode = true,
            .primaryParam = "list-profiles",
            .description =
				"Lists all profiles in a kalamake file, "
				"second parameter must be valid path to a .kmake file.",
            .targetFunction = [](const vector<string>& params) { Command_List_Profiles(params); }
        });

    KalaCLICore::AddCommand(
        {
            .allowInHeadlessMode = true,
            .primaryParam = "clean",
            .description =
				"Deletes all build directories from your target kalamake file path.",
            .targetFunction = [](const vector<string>& params) { Command_Clean(params); }
        });

    KalaCLICore::AddCommand(
        {
            .allowInHeadlessMode = true,
            .primaryParam = "compile",
            .description =
                "Compile a project from a kalamake file, "
				"second parameter must be valid path to a .kmake file, "
				"third parameter must be a valid profile in the .kmake file.",
            .targetFunction = [](const vector<string>& params) { Command_Compile(params); }
        });

    if (argc > 1)
    {
        string command{};
        for (int i = 1; i < argc; ++i)
        {
            if (i > 1) command += " ";
            command += argv[i];
        }

        KalaMakeCore::SetHeadlessModeState(true);
        KalaCLICore::SendCommand(command);

        //always exits if a command was passed, otherwise goes into cli mode
        exit(0);
    }

    KalaCLICore::SetConsoleWritesToPageState(true);
    KalaCLICore::SetWrapState(true);
    KalaCLICore::SetPageTitle("KalaMake " + string(KALAMAKE_VERSION));

    KalaCLICore::AppendToPage(" ");
    KalaCLICore::AppendToPage("Type '" + string(COMMAND_PREFIX) + "help' to list all commands.");
    
    while(true)
    {
        KalaCLICore::UpdateDisplayedContent();

        sleep_for(milliseconds(33)); //30 frames per second
    }

    return 0;
}