//Copyright(C) 2026 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include <sstream>
#include <filesystem>

#include "KalaHeaders/log_utils.hpp"
#include "KalaHeaders/file_utils.hpp"

#include "core.hpp"

#include "core/kma_core.hpp"

using KalaHeaders::KalaLog::Log;
using KalaHeaders::KalaLog::LogType;

using KalaHeaders::KalaFile::ReadLinesFromFile;

using KalaCLI::Core;

using std::ostringstream;
using std::filesystem::exists;
using std::filesystem::path;
using std::filesystem::current_path;
using std::filesystem::weakly_canonical;
using std::filesystem::is_directory;
using std::filesystem::filesystem_error;

namespace KalaMake::Core
{
	constexpr string_view EXE_VERSION_NUMBER = "1.0";
	constexpr string_view KMA_VERSION_NUMBER = "1.0";

	//kma path is the root directory where the kma file is stored at
	static path kmaPath{};

	void KalaMakeCore::Initialize(const vector<string>& params)
	{
		ostringstream details{};

		details
			<< "     | exe version: " << EXE_VERSION_NUMBER.data() << "\n"
			<< "     | kma version: " << KMA_VERSION_NUMBER.data() << "\n";

		Log::Print(details.str());

		path projectFile = params[1];

		string& currentDir = KalaCLI::Core::GetCurrentDir();
		if (currentDir.empty()) currentDir = current_path().string();

		auto readprojectfile = [](path filePath) -> void
			{
				if (is_directory(filePath))
				{
					KalaMakeCore::PrintError("Failed to compile project because its path '" + filePath.string() + "' leads to a directory!");

					return;
				}

				if (!filePath.has_extension()
					|| filePath.extension() != ".kma")
				{
					KalaMakeCore::PrintError("Failed to compile project because its path '" + filePath.string() + "' has an incorrect extension!");

					return;
				}

				Log::Print(
					"Starting to parse kma file '" + filePath.string() + "'"
					"\n\n==========================================================================================\n",
					"KALAMAKE",
					LogType::LOG_INFO);

				vector<string> content{};

				string result = ReadLinesFromFile(
					filePath,
					content);

				if (!result.empty())
				{
					KalaMakeCore::PrintError("Failed to read project file '" + filePath.string() + "'! Reason: " + result);

					return;
				}

				if (content.empty())
				{
					KalaMakeCore::PrintError("Failed to compile project at '" + filePath.string() + "' because it was empty!");

					return;
				}

				kmaPath = filePath.parent_path();

				//TODO: start parsing here...
			};

		//partial path was found

		path correctTarget{};

		try
		{
			correctTarget = weakly_canonical(path(KalaCLI::Core::GetCurrentDir()) / projectFile);
		}
		catch (const filesystem_error&)
		{
			KalaMakeCore::PrintError("Failed to compile project because partial path via '" + projectFile.string() + "' could not be resolved!");

			return;
		}

		if (exists(correctTarget))
		{
			readprojectfile(correctTarget);

			return;
		}

		//full path was found

		try
		{
			correctTarget = weakly_canonical(projectFile);
		}
		catch (const filesystem_error&)
		{
			KalaMakeCore::PrintError("Failed to compile project because full path '" + projectFile.string() + "' could not be resolved!");

			return;
		}

		if (exists(correctTarget))
		{
			readprojectfile(correctTarget);

			return;
		}

		KalaMakeCore::PrintError("Failed to compile project because its path '" + projectFile.string() + "' does not exist!");
	}

    void KalaMakeCore::PrintError(string_view message)
	{
		Log::Print(
			message,
			"KALAMAKE",
			LogType::LOG_ERROR,
			2);
	}
}