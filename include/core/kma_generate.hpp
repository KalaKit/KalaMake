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

    //Values for compile_commands.json,
    //C and C++ only
    struct CompileCommand
    {
        path dir{};
        string command{};
        path file{};
        string output{};
    };

    //Values for classpath file, Java only
    struct JavaClassPath
    {
        string binaryName{};
        path srcDir{};
        path outputDir{};
    };

    //Values for launch.json per profile
    struct VSCode_Launch
    {
        //launch profile name, will be assigned as profile name
        string name{};
        string type{};
        //workspace + build dir + binary name
        string program{};

        //java-only
        string mainClass{};
    };

    //Values for tasks.json per profile
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
            const vector<CompileCommand>& commands);

        static void GenerateJavaClassPath(const JavaClassPath& javaData);

        //Updates existing launch.json and tasks.json or makes new ones
        static void GenerateVSCodeSolution(
            bool isMSVC,
            bool isExe,
            const VSCode_Launch& launch,
            const VSCode_Task& task);
    };
}