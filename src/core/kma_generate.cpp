//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include <sstream>
#include <filesystem>

#include "log_utils.hpp"
#include "string_utils.hpp"
#include "file_utils.hpp"

#include "core/kma_generate.hpp"
#include "core/kma_core.hpp"

using KalaHeaders::KalaLog::Log;
using KalaHeaders::KalaLog::LogType;

using KalaHeaders::KalaString::ReplaceFromString;

using KalaHeaders::KalaFile::DeletePath;
using KalaHeaders::KalaFile::CreateNewFile;
using KalaHeaders::KalaFile::CreateNewDirectory;
using KalaHeaders::KalaFile::ReadLinesFromFile;
using KalaHeaders::KalaFile::FileType;

using std::ostringstream;
using std::filesystem::current_path;
using std::filesystem::exists;

namespace KalaMake::Core
{
    void Generate::GenerateCompileCommands(
        bool isMSVC,
        const path& buildPath,
        const vector<CompileCommand>& commands)
    {
		Log::Print(
			"Starting to create compile_commands.json.",
			"GENERATE_COMP_COMM",
			LogType::LOG_INFO);

		path compComm = buildPath / "compile_commands.json";

		if (exists(compComm))
		{
			string errorMsg = DeletePath(compComm);

			if (!errorMsg.empty())
			{
				KalaMakeCore::CloseOnError(
					"GENERATE_COMP_COMM",
					"Failed to remove existing compile_commands.json! Reason: " + errorMsg);
			}
		}

		ostringstream out{};

		out << "[\n";

		auto fix_msvc_slash = [&isMSVC](string_view input) -> string
			{
				return isMSVC
					? ReplaceFromString(string(input), "\\", "\\\\", true)
					: string(input);
			};

		auto fix_json_quotes = [](string_view input) -> string
			{
				return ReplaceFromString(string(input), "\"", "\\\"", true);
			};

		for (size_t i = 0; i < commands.size(); ++i)
		{
			const CompileCommand& s = commands[i];

			string cleanDir = fix_json_quotes(fix_msvc_slash(s.dir.string()));
			string cleanComm = fix_json_quotes(fix_msvc_slash(s.command));
			string cleanFile = fix_json_quotes(fix_msvc_slash(s.file.string()));
			string cleanOut = fix_json_quotes(fix_msvc_slash(s.output));

			out << "{\n";
			out << "  \"directory\": \"" << cleanDir  << "\",\n";
			out << "  \"command\": \""   << cleanComm << "\",\n";
			out << "  \"file\": \""      << cleanFile << "\",\n";
			out << "  \"output\": \""    << cleanOut  << "\"\n";
			out << "}";

			if (i + 1 < commands.size()) out << ",";

			out << "\n";
		}

		out << "]";

		string errorMsg = CreateNewFile(
			compComm,
			FileType::FILE_TEXT,
			{ .inText = out.str() });

		if (!errorMsg.empty())
		{
			KalaMakeCore::CloseOnError(
				"GENERATE_COMP_COMM",
				"Failed to create new compile_commands.json! Reason: " + errorMsg);
		}

		Log::Print(
			"Finished creating compile_commands.json!",
			"GENERATE_COMP_COMM",
			LogType::LOG_SUCCESS);
    }

    void Generate::GenerateVSCodeSolution(
        bool isMSVC,
        const VSCode_Launch& launch,
        const VSCode_Task& task)
    {
        path vscodeDir = current_path() / ".vscode";
        path launchJson = vscodeDir / "launch.json";
        path tasksJson = vscodeDir / "tasks.json";

        if (!exists(vscodeDir))
        {
            string dirErr = CreateNewDirectory(vscodeDir);
            if (!dirErr.empty())
            {
                KalaMakeCore::CloseOnError(
                    "GENERATE_VSCODE_SLN", 
                    "Failed to create vs code dir at '" + vscodeDir.string() + "'! Reason: " + dirErr);
            }
        }

        vector<string> launchLines{};
        if (exists(launchJson))
        {
            string readLaunchErr = ReadLinesFromFile(
                launchJson, 
                launchLines);

            if (!readLaunchErr.empty())
            {
                KalaMakeCore::CloseOnError(
                    "GENERATE_VSCODE_SLN", 
                    "Failed to read launch.json at '" + launchJson.string() + "'! Reason: " + readLaunchErr);
            }
        }

        vector<string> tasksLines{};
        if (exists(tasksJson))
        {
            string readTasksErr = ReadLinesFromFile(
                tasksJson, 
                tasksLines);

            if (!readTasksErr.empty())
            {
                KalaMakeCore::CloseOnError(
                    "GENERATE_VSCODE_SLN", 
                    "Failed to read tasks.json at '" + tasksJson.string() + "'! Reason: " + readTasksErr);
            }
        }

		Log::Print(
			"Starting to generate .vscode/launch.json.",
			"GENERATE_VSCODE_SLN",
			LogType::LOG_INFO);

        ostringstream launchStream{};

        launchStream
            << "        {\n"
            << "            \"name\": \"" << launch.name << "\",\n" 
            << "            \"type\": \"" << (isMSVC ? "cppvsdbg" : "cppdbg") << "\",\n"
            << "            \"request\": \"launch\",\n"
            << "            \"program\": \"" << launch.program << (isMSVC ? ".exe" : "") << "\",\n"
            << "            \"args\": [],\n"
            << "            \"cwd\": \"${workspaceFolder}\",\n"
            << "            \"stopAtEntry\": false,\n"
            << "            \"console\": \"integratedTerminal\"\n"
            << "        },\n";

        Log::Print("\nlaunch data:\n" + launchStream.str());

        Log::Print(
			"Starting to generate .vscode/tasks.json.",
			"GENERATE_VSCODE_SLN",
			LogType::LOG_INFO);

        ostringstream tasksStream{};

        tasksStream
            << "        {\n"
            << "            \"label\": \"" << task.label << "\",\n" 
            << "            \"type\": \"shell\",\n" 
            << "            \"command\": \"kalamake --compile " << task.projectFile << " " << task.label << "\",\n"
            << "            \"group\": \"build\"\n"
            << "        },\n";

        Log::Print("\ntasks data:\n" + tasksStream.str());
    }
}