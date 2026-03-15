//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include <algorithm>
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

        auto fix_slashes = [&isMSVC](string_view input) -> string
			{
                string l = ReplaceFromString(string(input), "\\", "/", true);
                if (isMSVC) l = ReplaceFromString(l, "/", "\\\\", true);

                return l;
			};

		auto fix_json_quotes = [](string_view input) -> string
			{
				return ReplaceFromString(string(input), "\"", "\\\"", true);
			};

		for (size_t i = 0; i < commands.size(); ++i)
		{
			const CompileCommand& s = commands[i];

			string cleanDir = fix_json_quotes(fix_slashes(s.dir.string()));
			string cleanComm = fix_json_quotes(fix_slashes(s.command));
			string cleanFile = fix_json_quotes(fix_slashes(s.file.string()));
			string cleanOut = fix_json_quotes(fix_slashes(s.output));

			out << "        {\n";
			out << "            \"directory\": \"" << cleanDir  << "\",\n";
			out << "            \"command\": \""   << cleanComm << "\",\n";
			out << "            \"file\": \""      << cleanFile << "\",\n";
			out << "            \"output\": \""    << cleanOut  << "\"\n";
			out << "        }";

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
			"Finished generating compile_commands.json!",
			"GENERATE_COMP_COMM",
			LogType::LOG_SUCCESS);
    }

    void Generate::GenerateVSCodeSolution(
        bool isMSVC,
        bool isExe,
        const VSCode_Launch& launch,
        const VSCode_Task& task)
    {
        path vscodeDir = current_path() / ".vscode";

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

        auto fix_slashes = [&isMSVC](string_view input) -> string
			{
                string l = ReplaceFromString(string(input), "\\", "/", true);
                if (isMSVC) l = ReplaceFromString(l, "/", "\\\\", true);

                return l;
			};

        auto remove_useless_lines = [](vector<string>& fields) -> void
            {
                for (auto& s : fields)
                {
                    size_t pos = s.find("//");
                    if (pos != string::npos) s.erase(pos);
                }

                fields.erase(
                    remove_if(fields.begin(), fields.end(),
                        [](const string& s)
                        {
                            return s.find_first_not_of(" \t\r\n") == string::npos;
                        }),
                    fields.end()
                );
            };

        auto remove_existing_profile = [](
            size_t start, 
            size_t end,
            vector<string>& fields) -> void
            {
                fields.erase(
                    fields.begin() + start,
                    fields.begin() + end + 1);
            };

        //
        // LAUNCH
        //

        if (isExe)
        {
            Log::Print(
                "Starting to generate launch.json.",
                "GENERATE_VSCODE_SLN",
                LogType::LOG_INFO);

            path launchJson = vscodeDir / "launch.json";

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

            remove_useless_lines(launchLines);

            bool isValidLaunch = launchLines.empty();

            if (!launchLines.empty()
                && launchLines.size() >= 5)
            {
                isValidLaunch = 
                    launchLines[0] == "{"
                    && launchLines[1] == "    \"version\": \"0.2.0\","
                    && launchLines[2] == "    \"configurations\": ["

                    //any content inside
                    
                    && launchLines[launchLines.size() - 2] == "    ]"
                    && launchLines[launchLines.size() - 1] == "}";
            }

            if (isValidLaunch
                && !launchLines.empty())
            {
                struct LaunchProfile
                {
                    string name{};
                    size_t start{};
                    size_t end{};
                };

                vector<LaunchProfile> profiles{};
                LaunchProfile p{};

                //get profiles inside valid structure
                for (size_t i = 3; i < launchLines.size() - 2; i++)
                {
                    string_view line = launchLines[i];

                    if (p.start == 0
                        && line == "        {")
                    {
                        p.start = i;
                    }

                    if (p.start != 0
                        && p.end == 0
                        && line.starts_with("            \"name\": "))
                    {
                        size_t colon = line.find(':');
                        size_t first = line.find('"', colon) + 1;
                        size_t last = line.find('"', first);

                        p.name = string(line.substr(first, last - first));
                    }

                    if (p.start != 0
                        && p.end == 0
                        && (line == "        }"
                        || line == "        },"))
                    {
                        p.end = i;

                        if (!p.name.empty()
                            && p.start != 0)
                        {
                            profiles.push_back(p);
                            p = LaunchProfile{};
                        }
                    }
                }

                LaunchProfile existingProfile{};

                for (const auto& p : profiles)
                {
                    if (p.name == launch.name)
                    {
                        existingProfile = p;
                        break;
                    }
                }

                if (!existingProfile.name.empty())
                {
                    remove_existing_profile(
                        existingProfile.start,
                        existingProfile.end,
                        launchLines);
                }
            }

            if (!isValidLaunch)
            {
                KalaMakeCore::CloseOnError(
                    "GENERATE_VSCODE_SLN", 
                    "Failed to update existing launch.json at '" + launchJson.string() + "' because it was malformed!");
            }

            string program = fix_slashes(launch.program + (isMSVC ? ".exe" : ""));

            vector<string> newProfileLines{
                "        {",
                "            \"name\": \"" + launch.name + "\",",
                "            \"type\": \"" + launch.type + "\",",
                "            \"request\": \"launch\",",
                "            \"program\": \"" + program + "\",",
                "            \"args\": [],",
                "            \"cwd\": \"${workspaceFolder}\",",
                "            \"stopAtEntry\": false,",
                "            \"console\": \"integratedTerminal\"",
                "        },"
            };

            if (launchLines.empty())
            {
                launchLines = 
                {
                    "{",
                    "    \"version\": \"0.2.0\",",
                    "    \"configurations\": [",
                    "    ]",
                    "}"
                };
            }

            launchLines.insert(
                launchLines.begin() + 3,
                newProfileLines.begin(),
                newProfileLines.end());

            if (exists(launchJson))
            {
                string launchErr = DeletePath(launchJson);

                if (!launchErr.empty())
                {
                    KalaMakeCore::CloseOnError(
                        "GENERATE_VSCODE_SLN", 
                        "Failed to delete old launch.json at '" + launchJson.string() + "' before generating new one! Reason: " + launchErr);
                }
            }

            string launchErr = CreateNewFile(
                launchJson,
                FileType::FILE_TEXT,
                { .inLines = std::move(launchLines) });

            if (!launchErr.empty())
            {
                KalaMakeCore::CloseOnError(
                    "GENERATE_VSCODE_SLN", 
                    "Failed to generate new launch.json at '" + launchJson.string() + "'! Reason: " + launchErr);
            }

            /*
            for (const auto& l : launchLines)
            {
                Log::Print(l);
            }
            */
        }
        else
        {
            Log::Print(
                "Skipping launch.json generation because target is not an executable.",
                "GENERATE_VSCODE_SLN",
                LogType::LOG_INFO);
        }

        //
        // TASKS
        //

        Log::Print(" ");

        Log::Print(
			"Starting to generate tasks.json.",
			"GENERATE_VSCODE_SLN",
			LogType::LOG_INFO);

        path tasksJson = vscodeDir / "tasks.json";

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

        remove_useless_lines(tasksLines);

        bool isValidTasks = tasksLines.empty();

        if (!tasksLines.empty()
            && tasksLines.size() >= 5)
        {
            isValidTasks = 
                tasksLines[0] == "{"
                && tasksLines[1] == "    \"version\": \"2.0.0\","
                && tasksLines[2] == "    \"tasks\": ["

                //any content inside
                
                && tasksLines[tasksLines.size() - 2] == "    ]"
                && tasksLines[tasksLines.size() - 1] == "}";
        }

        if (isValidTasks
            && !tasksLines.empty())
        {
            struct TasksProfile
            {
                string label{};
                size_t start{};
                size_t end{};
            };

            vector<TasksProfile> profiles{};
            TasksProfile p{};

            //get profiles inside valid structure
            for (size_t i = 3; i < tasksLines.size() - 2; i++)
            {
                string_view line = tasksLines[i];

                if (p.start == 0
                    && line == "        {")
                {
                    p.start = i;
                }

                if (p.start != 0
                    && p.end == 0
                    && line.starts_with("            \"label\": "))
                {
                    size_t colon = line.find(':');
                    size_t first = line.find('"', colon) + 1;
                    size_t last = line.find('"', first);

                    p.label = string(line.substr(first, last - first));
                }

                if (p.start != 0
                    && p.end == 0
                    && (line == "        }"
                    || line == "        },"))
                {
                    p.end = i;

                    if (!p.label.empty()
                        && p.start != 0)
                    {
                        profiles.push_back(p);
                        p = TasksProfile{};
                    }
                }
            }

            TasksProfile existingProfile{};

            for (const auto& p : profiles)
            {
                if (p.label == task.label)
                {
                    existingProfile = p;
                    break;
                }
            }

            if (!existingProfile.label.empty())
            {
                remove_existing_profile(
                    existingProfile.start,
                    existingProfile.end,
                    tasksLines);
            }
        }

        if (!isValidTasks)
        {
            KalaMakeCore::CloseOnError(
                "GENERATE_VSCODE_SLN", 
                "Failed to update existing tasks.json at '" + tasksJson.string() + "' because it was malformed!");
        }

        vector<string> newProfileLines{
            "        {",
            "            \"label\": \"" + task.label + "\",",
            "            \"type\": \"shell\",",
            "            \"command\": \"kalamake --compile " + task.projectFile + " " + task.label + "\",",
            "            \"group\": \"build\"",
            "        },"
        };

        if (tasksLines.empty())
        {
            tasksLines = 
            {
                "{",
                "    \"version\": \"2.0.0\",",
                "    \"tasks\": [",
                "    ]",
                "}"
            };
        }

        tasksLines.insert(
            tasksLines.begin() + 3,
            newProfileLines.begin(),
            newProfileLines.end());

        if (exists(tasksJson))
        {
            string tasksErr = DeletePath(tasksJson);

            if (!tasksErr.empty())
            {
                KalaMakeCore::CloseOnError(
                    "GENERATE_VSCODE_SLN", 
                    "Failed to delete old tasks.json at '" + tasksJson.string() + "' before generating new one! Reason: " + tasksErr);
            }
        }

        string tasksErr = CreateNewFile(
            tasksJson,
            FileType::FILE_TEXT,
            { .inLines = std::move(tasksLines) });

        if (!tasksErr.empty())
        {
            KalaMakeCore::CloseOnError(
                "GENERATE_VSCODE_SLN", 
                "Failed to generate new tasks.json at '" + tasksJson.string() + "'! Reason: " + tasksErr);
        }

        /*
        for (const auto& l : tasksLines)
        {
            Log::Print(l);
        }
        */

        Log::Print(
			"Finished generating vscode solution files!",
			"GENERATE_VSCODE_SLN",
			LogType::LOG_SUCCESS);

        Log::Print(" ");
    }
}