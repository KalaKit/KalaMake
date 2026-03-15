//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace KalaMake::Core
{
    using std::filesystem::path;
    using std::string;
    using std::vector;

    struct CompileCommand
    {
        path dir{};
        string command{};
        path file{};
        string output{};
    };

    //Values for launch.json per profile.
    //Values added that arent listed here:
    //request: launch
    //args: []
    //cwd: "${workspaceFolder}"
    //stopAtEntry: false
    //console: integratedTerminal
    struct VSCode_Launch
    {
        //launch profile name, will be assigned as profile name
        string name{};
        string type{};
        //workspace + build dir + binary name
        string program{};
    };

    //Values for tasks.json per profile.
    //Values added that arent listed here:
    //type: shell
    //command: kalamake --compile projectFile label (reuses project file path and label value)
    //group: "build"
    struct VSCode_Task
    {
        //task label name, will be assigned as profile name
        string label{};
        //which kalamake project file was used, passed to command
        string projectFile{};
    };

    class Generate
    {
    public:
        //Exports compile_commands.json
        static void GenerateCompileCommands(
            bool isMSVC,
            const path& buildPath,
            const vector<CompileCommand>& commands);

        //Updates existing launch.json and tasks.json or makes new ones
        static void GenerateVSCodeSolution(
            bool isMSVC,
            bool isExe,
            const VSCode_Launch& launch,
            const VSCode_Task& task);
    };
}