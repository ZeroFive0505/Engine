#include "Common.h"
#include "FileSystem.h"
#include <filesystem>
#include <regex>
#include <Windows.h>
#include <shellapi.h>
#include "../Log/Logger.h"
#include <fstream>

#pragma comment (lib, "Shell32")


using namespace std;

namespace PlayGround
{
	void FileSystem::CreateTextFile(const string& file_path, const string& text)
	{
		// ���Ӱ� ������ ����� ����.
		std::ofstream outfile(file_path);
		outfile << text;
		outfile.flush();
		outfile.close();
	}

	bool FileSystem::IsEmptyOrWhitespace(const std::string& var)
	{
		// ���� ��������� ��
		if (var.empty())
			return true;

		// ��� ���ڿ��� �ݺ��ϸ鼭 ������ ã�´�.
		for (char c : var)
		{
			if (!std::isspace(c))
				return false;
		}

		return true;
	}

	bool FileSystem::IsAlphanumeric(const std::string& var)
	{
		if (IsEmptyOrWhitespace(var))
			return false;

		// ��� ���ڿ��� �ݺ��ϸ鼭 ���ĺ��̳� ���������� Ȯ���Ѵ�.
		for (char c : var)
		{
			if (!std::isalnum(c))
				return false;
		}

		return true;
	}

	string FileSystem::RemoveIllegalCharacters(const string& text)
	{
		string text_legal = text;

		// �����̸����� ������ �ʴ� ���ڿ���
		string illegal = ":?\"<>|";

		// ��� ���ڿ��� �ݺ��ϸ鼭 ã�´�.
		for (auto it = text_legal.begin(); it < text_legal.end(); it++)
		{
			// ã���� �ÿ��� _�� ��ȯ
			if (illegal.find(*it) != string::npos)
				*it = '_';
		}

		// �׸��� ��� ���� ���θ� Ȯ���Ѵ�.
		if (IsDirectory(text_legal))
			return text_legal;

		// ���� ������ ã�� ���ߴٸ� �������� ���θ� ã�´�.
		illegal = "\\/";
		for (auto it = text_legal.begin(); it < text_legal.end(); it++)
		{
			if (illegal.find(*it) != string::npos)
				*it = '_';
		}

		return text_legal;
	}

	string FileSystem::GetStringBeforeExpression(const string& str, const string& exp)
	{
		// �ش� ���ڿ��� ��ġ�� ã����.
		const size_t position = str.find(exp);
		// ã�Ҵٸ� �� ���ڿ��� ��ġ������ ���� ��Ʈ���� ����� ��ȯ�Ѵ�.
		return position != string::npos ? str.substr(0, position) : "";
	}

	string FileSystem::GetStringAfterExpression(const string& str, const string& exp)
	{
		// �ش� ���ڿ��� ��ġ�� ã�´�.
		const size_t position = str.find(exp);
		// ã�Ҵٸ� �� ���� ���ڿ��� ����� ��ȯ
		return position != string::npos ? str.substr(position + exp.length()) : "";
	}

	string FileSystem::GetStringBetweenExpressions(const string& str, const string& exp_a, const string& exp_b)
	{
		// Ư�� ���ڿ� ������ ���� ǥ������ �����.
		const regex base_regex(exp_a + "(.*)" + exp_b);

		smatch base_match;
		// �ΰ��� ���ڿ� ������ ���ڿ��� ã�´�.
		if (regex_search(str, base_match, base_regex))
		{
			if (base_match.size() == 2)
				return base_match[1].str();
		}

		return str;
	}

	string FileSystem::ConvertToUppercase(const string& lower)
	{
		const locale loc;
		string upper;
		// ���� �빮�ڷ� ��ȯ�Ѵ�.
		for (const auto& c : lower)
		{
			upper += std::toupper(c, loc);
		}

		return upper;
	}

	string FileSystem::ReplaceExpression(const string& str, const string& from, const string& to)
	{
		// ��ȯ�Ѵ�.
		return regex_replace(str, regex(from), to);
	}

	wstring FileSystem::StringToWstring(const string& str)
	{
		// ���� ���ڿ��� ���̸� ���Ѵ�. null����
		int length = static_cast<int>(str.length()) + 1;
		// ũ�⸦ ���Ѵ�.
		int size = MultiByteToWideChar(CP_ACP, 0, str.c_str(), length, nullptr, 0);
		// ũ�⸸ŭ wchar�� �Ҵ��Ѵ�.
		wchar_t* buffer = new wchar_t[size];
		// �����Ѵ�.
		MultiByteToWideChar(CP_ACP, 0, str.c_str(), length, buffer, size);
		std::wstring result(buffer);
		// ����
		SAFE_DELETE_ARRAY(buffer);

		return result;
	}

	void FileSystem::OpenDirectoryWindow(const string& directory)
	{
		// ���� Ž���� ����
		ShellExecute(nullptr, nullptr, StringToWstring(directory).c_str(), nullptr, nullptr, SW_SHOW);
	}

	bool FileSystem::CreateDirectory_(const string& path)
	{
		// ��� ������ �Ѵ�.
		try
		{
			// ��� ���� �����ÿ��� �׳ɹ�ȯ
			if (filesystem::create_directories(path))
				return true;
		}
		// ���нÿ��� �α׸� ����.
		catch (filesystem::filesystem_error& e)
		{
			LOG_WARNING("%s, %s", e.what(), path.c_str());
		}

		return false;
	}

	bool FileSystem::Delete(const string& path)
	{
		try
		{
			// ���� ���� �����ÿ��� ��ȯ
			if (filesystem::exists(path) && filesystem::remove_all(path))
				return true;
		}
		// ���нÿ��� �α�
		catch (filesystem::filesystem_error& e)
		{
			LOG_WARNING("%s, %s", e.what(), path.c_str());
		}

		return false;
	}

	bool FileSystem::Exists(const string& path)
	{
		try
		{
			if (filesystem::exists(path))
				return true;
		}
		catch (filesystem::filesystem_error& e)
		{
			LOG_WARNING("%s, %s", e.what(), path.c_str());
		}

		return false;
	}

	bool FileSystem::IsDirectory(const string& path)
	{
		try
		{
			if (filesystem::exists(path) && filesystem::is_directory(path))
				return true;
		}
		catch (filesystem::filesystem_error& e)
		{
			LOG_WARNING("%s, %s", e.what(), path.c_str());
		}

		return false;
	}

	bool FileSystem::IsFile(const string& path)
	{
		if (path.empty())
			return false;

		try
		{
			if (filesystem::exists(path) && filesystem::is_regular_file(path))
				return true;
		}
		catch (filesystem::filesystem_error& e)
		{
			LOG_WARNING("%s, %s", e.what(), path.c_str());
		}

		return false;
	}

	bool FileSystem::CopyFileFromTo(const string& source, const string& destination)
	{
		// ���۰�ο� ���� ��� ��ΰ� ������ �׳� ��ȯ
		if (source == destination)
			return true;

		// ���� �������� �ʴ� ��ζ�� ����
		if (!Exists(GetDirectoryFromFilePath(destination)))
			CreateDirectory_(GetDirectoryFromFilePath(destination));

		// �׸��� ���縦 �õ��Ѵ�.
		try
		{
			return filesystem::copy_file(source, destination, filesystem::copy_options::overwrite_existing);
		}
		catch (filesystem::filesystem_error& e)
		{
			LOG_WARNING("%s", e.what());
			return false;
		}
	}

	string FileSystem::GetFileNameFromFilePath(const string& path)
	{
		return filesystem::path(path).filename().generic_string();
	}

	string FileSystem::GetFileNameWithoutExtensionFromFilePath(const string& path)
	{
		const string file_name = GetFileNameFromFilePath(path);
		const size_t last_index = file_name.find_last_of('.');

		if (last_index != string::npos)
			return file_name.substr(0, last_index);

		return "";
	}

	string FileSystem::GetDirectoryFromFilePath(const string& path)
	{
		const size_t last_index = path.find_last_of("\\/");

		if (last_index != string::npos)
			return path.substr(0, last_index + 1);

		return "";
	}

	string FileSystem::GetFilePathWithoutExtension(const string& path)
	{
		return GetDirectoryFromFilePath(path) + GetFileNameWithoutExtensionFromFilePath(path);
	}

	string FileSystem::ReplaceExtension(const string& path, const string& extension)
	{
		return GetDirectoryFromFilePath(path) + GetFileNameWithoutExtensionFromFilePath(path) + extension;
	}

	string FileSystem::GetExtensionFromFilePath(const string& path)
	{
		string extension;

		try
		{
			extension = filesystem::path(path).extension().generic_string();
		}
		catch (system_error& e)
		{
			LOG_WARNING("Failed. %s", e.what());
		}

		return extension;
	}

	string FileSystem::NativizeFilePath(const string& path)
	{
		const string file_path_no_ext = GetFilePathWithoutExtension(path);

		if (IsSupportedAudioFile(path))
			return file_path_no_ext + EXTENSION_AUDIO;

		if (IsSupportedImageFile(path))
			return file_path_no_ext + EXTENSION_TEXTURE;

		if (IsSupportedFontFile(path))
			return file_path_no_ext + EXTENSION_FONT;

		if (IsSupportedModelFile(path))
			return file_path_no_ext + EXTENSION_MODEL;

		if (IsSupportedShaderFile(path))
			return file_path_no_ext + EXTENSION_SHADER;

		LOG_WARNING("Failed to nativize file path");

		return path;
	}

	vector<string> FileSystem::GetDirectoriesInDirectory(const string& path)
	{
		vector<string> directories;
		const filesystem::directory_iterator it_end;
		for (filesystem::directory_iterator it(path); it != it_end; it++)
		{
			if (!filesystem::is_directory(it->status()))
				continue;

			string path;

			try
			{
				path = it->path().string();
			}
			catch (system_error& e)
			{
				LOG_WARNING("Failed to read a directory path. %s", e.what());
			}

			if (!path.empty())
				directories.emplace_back(path);
		}

		return directories;
	}

	vector<string> FileSystem::GetFilesInDirectory(const string& path)
	{
		vector<string> file_paths;
		const filesystem::directory_iterator it_end;
		for (filesystem::directory_iterator it(path); it != it_end; it++)
		{
			if (!filesystem::is_regular_file(it->status()))
				continue;

			try
			{
				file_paths.emplace_back(it->path().string());
			}
			catch (system_error& e)
			{
				LOG_WARNING("Failed to read a file path. %s", e.what());
			}
		}

		return file_paths;
	}

	bool FileSystem::IsSupportedAudioFile(const string& path)
	{
		const string extension = GetExtensionFromFilePath(path);

		for (const auto& format : supported_formats_audio)
		{
			if (extension == format || extension == ConvertToUppercase(format))
				return true;
		}

		return false;
	}

	bool FileSystem::IsSupportedImageFile(const string& path)
	{
		const string extension = GetExtensionFromFilePath(path);

		for (const auto& format : supported_formats_image)
		{
			if (extension == format || extension == ConvertToUppercase(format))
				return true;
		}

		if (GetExtensionFromFilePath(path) == EXTENSION_TEXTURE)
			return true;

		return false;
	}

	bool FileSystem::IsSupportedModelFile(const string& path)
	{
		const string extension = GetExtensionFromFilePath(path);

		for (const auto& format : supported_formats_model)
		{
			if (extension == format || extension == ConvertToUppercase(format))
				return true;
		}

		return false;
	}

	bool FileSystem::IsSupportedShaderFile(const string& path)
	{
		const string extension = GetExtensionFromFilePath(path);

		for (const auto& format : supported_formats_shader)
		{
			if (extension == format || extension == ConvertToUppercase(format))
				return true;
		}

		return false;
	}

	bool FileSystem::IsSupportedFontFile(const string& path)
	{
		const string extension = GetExtensionFromFilePath(path);

		for (const auto& format : supported_formats_font)
		{
			if (extension == format || extension == ConvertToUppercase(format))
				return true;
		}

		return false;
	}

	bool FileSystem::IsEngineScriptFile(const string& path)
	{
		const string extension = GetExtensionFromFilePath(path);

		for (const auto& format : supported_formats_script)
		{
			if (extension == format || extension == ConvertToUppercase(format))
				return true;
		}

		return false;
	}

	bool FileSystem::IsEnginePrefabFile(const string& path)
	{
		return GetExtensionFromFilePath(path) == EXTENSION_PREFAB;
	}

	bool FileSystem::IsEngineModelFile(const string& path)
	{
		return GetExtensionFromFilePath(path) == EXTENSION_MODEL;
	}

	bool FileSystem::IsEngineMaterialFile(const string& path)
	{
		return GetExtensionFromFilePath(path) == EXTENSION_MATERIAL;
	}

	bool FileSystem::IsEngineMeshFile(const string& path)
	{
		return GetExtensionFromFilePath(path) == EXTENSION_MESH;
	}

	bool FileSystem::IsEngineSceneFile(const string& path)
	{
		return GetExtensionFromFilePath(path) == EXTENSION_WORLD;
	}

	bool FileSystem::IsEngineTextureFile(const string& path)
	{
		return GetExtensionFromFilePath(path) == EXTENSION_TEXTURE;
	}

	bool FileSystem::IsEngineAudioFile(const std::string& path)
	{
		return GetExtensionFromFilePath(path) == EXTENSION_AUDIO;
	}

	bool FileSystem::IsEngineShaderFile(const string& path)
	{
		return GetExtensionFromFilePath(path) == EXTENSION_SHADER;
	}

	bool FileSystem::IsEngineFile(const string& path)
	{
		return  IsEngineScriptFile(path) ||
			IsEnginePrefabFile(path) ||
			IsEngineModelFile(path) ||
			IsEngineMaterialFile(path) ||
			IsEngineMeshFile(path) ||
			IsEngineSceneFile(path) ||
			IsEngineTextureFile(path) ||
			IsEngineAudioFile(path) ||
			IsEngineShaderFile(path);
	}

	vector<string> FileSystem::GetSupportedFilesInDirectory(const string& path)
	{
		const vector<string> filesInDirectory = GetFilesInDirectory(path);
		vector<string> imagesInDirectory = GetSupportedImageFilesFromPaths(filesInDirectory);
		vector<string> scriptsInDirectory = GetSupportedScriptFilesFromPaths(filesInDirectory);
		vector<string> modelsInDirectory = GetSupportedModelFilesFromPaths(filesInDirectory);
		vector<string> supportedFiles;

		for (string& s : imagesInDirectory)
		{
			supportedFiles.emplace_back(s);
		}

		for (string& s : scriptsInDirectory)
		{
			supportedFiles.emplace_back(s);
		}

		for (string& s : modelsInDirectory)
		{
			supportedFiles.emplace_back(s);
		}

		return supportedFiles;
	}

	vector<string> FileSystem::GetSupportedImageFilesFromPaths(const vector<string>& paths)
	{
		vector<string> imageFiles;
		for (const auto& path : paths)
		{
			if (!IsSupportedImageFile(path))
				continue;

			imageFiles.emplace_back(path);
		}

		return imageFiles;
	}

	vector<string> FileSystem::GetSupportedAudioFilesFromPaths(const vector<string>& paths)
	{
		vector<string> audioFiles;
		for (const auto& path : paths)
		{
			if (!IsSupportedAudioFile(path))
				continue;

			audioFiles.emplace_back(path);
		}

		return audioFiles;
	}

	vector<string> FileSystem::GetSupportedScriptFilesFromPaths(const vector<string>& paths)
	{
		vector<string> scripts;
		for (const auto& path : paths)
		{
			if (!IsEngineScriptFile(path))
				continue;

			scripts.emplace_back(path);
		}

		return scripts;
	}

	vector<string> FileSystem::GetSupportedModelFilesFromPaths(const vector<string>& paths)
	{
		vector<string> images;
		for (const auto& path : paths)
		{
			if (!IsSupportedModelFile(path))
				continue;

			images.emplace_back(path);
		}

		return images;
	}

	vector<string> FileSystem::GetSupportedModelFilesInDirectory(const string& path)
	{
		return GetSupportedModelFilesFromPaths(GetFilesInDirectory(path));
	}

	vector<string> FileSystem::GetSupportedSceneFilesInDirectory(const string& path)
	{
		vector<string> sceneFiles;

		auto files = GetFilesInDirectory(path);
		for (const auto& file : files)
		{
			if (!IsEngineSceneFile(file))
				continue;

			sceneFiles.emplace_back(file);
		}

		return sceneFiles;
	}

	string FileSystem::GetRelativePath(const string& path)
	{
		if (filesystem::path(path).is_relative())
			return path;

		// ���� ��θ� �����´�.
		const filesystem::path p = filesystem::absolute(path);
		const filesystem::path r = filesystem::absolute(GetWorkingDirectory());

		// ���� ���� �ٸ��ٸ� �ٷ� ��ȯ�Ѵ�.
		if (p.root_path() != r.root_path())
			return p.generic_string();

		filesystem::path result;

		filesystem::path::const_iterator itr_path = p.begin();
		filesystem::path::const_iterator itr_relative_to = r.begin();
		// �� ���� ��ΰ� �޶����� �����ϴ� �������� �ݺ��Ѵ�.
		while (*itr_path == *itr_relative_to && itr_path != p.end() && itr_relative_to != r.end())
		{
			++itr_path;
			++itr_relative_to;
		}

		// '../' ��ū�� ����ΰ� ���������� �߰��Ѵ�.
		if (itr_relative_to != r.end())
		{
			itr_relative_to++;

			while (itr_relative_to != r.end())
			{
				result /= "..";
				itr_relative_to++;
			}
		}

		// ������ ��θ� �߰��Ѵ�.
		while (itr_path != p.end())
		{
			result /= *itr_path;
			itr_path++;
		}

		return result.generic_string();
	}

	string FileSystem::GetWorkingDirectory()
	{
		return filesystem::current_path().generic_string();
	}

	string FileSystem::GetParentDirectory(const string& path)
	{
		return filesystem::path(path).parent_path().generic_string();
	}

	string FileSystem::GetRootDirectory(const std::string& path)
	{
		return filesystem::path(path).root_directory().generic_string();
	}
}
