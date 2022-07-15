#pragma once

#include <vector>
#include <string>
#include "../EngineDefinition.h"

/// <summary>
/// �������� ���ϰ� ���õ� ������ ���ؼ� ������� Ŭ����
/// ��θ� ã�ƿö� ������ �Լ���� Ȯ���� ������� ���ǵǾ� �ִ�.
/// </summary>
namespace PlayGround
{
	class FileSystem
	{
    public:
        // �ؽ�Ʈ ���� ����
        static void CreateTextFile(const std::string& file_path, const std::string& text);

        // ��������
        static bool IsEmptyOrWhitespace(const std::string& var);
        // ���ĺ� �Ǵ� �����ε�
        static bool IsAlphanumeric(const std::string& var);
        // #./! ���� ������ �ʴ� ���ڸ� �����Ѵ�.
        static std::string RemoveIllegalCharacters(const std::string& text);
        // Ư�� ���ڿ� �ٷ��������� ���ڿ��� �����´�.
        static std::string GetStringBeforeExpression(const std::string& str, const std::string& exp);
        // Ư�� ���ڿ� ������ ���ڿ��� �����´�.
        static std::string GetStringAfterExpression(const std::string& str, const std::string& exp);
        // Ư�� ���ڿ� ������ ���ڿ��� �����´�.
        static std::string GetStringBetweenExpressions(const std::string& str, const std::string& exp_a, const std::string& exp_b);
        // ���� �빮�ڷ� �ٲ۴�.
        static std::string ConvertToUppercase(const std::string& lower);
        // Ư�� ���ڿ��� from���� to�� ��ȯ
        static std::string ReplaceExpression(const std::string& str, const std::string& from, const std::string& to);
        // ��Ƽ����Ʈ�� WCHAR�� ��ȯ
        static std::wstring StringToWstring(const std::string& str);

        // ���� ���̾�α� ����
        static void OpenDirectoryWindow(const std::string& path);
        // ���丮 ����
        static bool CreateDirectory_(const std::string& path);
        // ���� ����
        static bool Delete(const std::string& path);
        // ���� üũ
        static bool Exists(const std::string& path);
        // ��� üũ
        static bool IsDirectory(const std::string& path);
        // ���� üũ
        static bool IsFile(const std::string& path);
        // ���� ����
        static bool CopyFileFromTo(const std::string& source, const std::string& destination);
        // ��ο��� ���� �̸��� �����´�.
        static std::string GetFileNameFromFilePath(const std::string& path);
        // ��ο��� ���� �̸��� �����´�. (Ȯ���� ����)
        static std::string GetFileNameWithoutExtensionFromFilePath(const std::string& path);
        // ���ϰ�ο��� ��θ� �����´�.
        static std::string GetDirectoryFromFilePath(const std::string& path);
        // Ȯ���ڸ� ������ ���ϰ�θ� �����´�.
        static std::string GetFilePathWithoutExtension(const std::string& path);
        // Ȯ���� ��ȯ
        static std::string ReplaceExtension(const std::string& path, const std::string& extension);
        // Ȯ���ڸ� �����´�.
        static std::string GetExtensionFromFilePath(const std::string& path);
        // ������ ���� ��ü�������� ��ȯ�Ѵ�.
        static std::string NativizeFilePath(const std::string& path);
        // ��� ��θ� �����´�.
        static std::string GetRelativePath(const std::string& path);
        // ���� ������Ʈ�� ����ǰ��ִ� ��θ� �����´�.
        static std::string GetWorkingDirectory();
        // ��Ʈ ���丮�� �����´�.
        static std::string GetRootDirectory(const std::string& path);
        // �θ� ���丮�� �����´�.
        static std::string GetParentDirectory(const std::string& path);
        // �� ���丮�� ��� ���丮���� ��ȯ�Ѵ�.
        static std::vector<std::string> GetDirectoriesInDirectory(const std::string& path);
        // �� ���丮�� ��� ���ϵ��� ��ȯ�Ѵ�.
        static std::vector<std::string> GetFilesInDirectory(const std::string& path);

        // ���� ��ü �������� Ȯ���ϴ�  �Լ���

        static bool IsSupportedAudioFile(const std::string& path);
        static bool IsSupportedImageFile(const std::string& path);
        static bool IsSupportedModelFile(const std::string& path);
        static bool IsSupportedShaderFile(const std::string& path);
        static bool IsSupportedFontFile(const std::string& path);
        static bool IsEngineScriptFile(const std::string& path);
        static bool IsEnginePrefabFile(const std::string& path);
        static bool IsEngineMaterialFile(const std::string& path);
        static bool IsEngineMeshFile(const std::string& path);
        static bool IsEngineModelFile(const std::string& path);
        static bool IsEngineSceneFile(const std::string& path);
        static bool IsEngineTextureFile(const std::string& path);
        static bool IsEngineAudioFile(const std::string& path);
        static bool IsEngineShaderFile(const std::string& path);
        static bool IsEngineFile(const std::string& path);

        static std::vector<std::string> GetSupportedFilesInDirectory(const std::string& path);
        static std::vector<std::string> GetSupportedImageFilesFromPaths(const std::vector<std::string>& paths);
        static std::vector<std::string> GetSupportedAudioFilesFromPaths(const std::vector<std::string>& paths);
        static std::vector<std::string> GetSupportedScriptFilesFromPaths(const std::vector<std::string>& paths);
        static std::vector<std::string> GetSupportedModelFilesFromPaths(const std::vector<std::string>& paths);
        static std::vector<std::string> GetSupportedModelFilesInDirectory(const std::string& path);
        static std::vector<std::string> GetSupportedSceneFilesInDirectory(const std::string& path);
    };

    // ���� ���� Ȯ����
    static const char* EXTENSION_WORLD = ".world";
    static const char* EXTENSION_MATERIAL = ".material";
    static const char* EXTENSION_MODEL = ".model";
    static const char* EXTENSION_PREFAB = ".prefab";
    static const char* EXTENSION_SHADER = ".shader";
    static const char* EXTENSION_FONT = ".font";
    static const char* EXTENSION_TEXTURE = ".texture";
    static const char* EXTENSION_MESH = ".mesh";
    static const char* EXTENSION_AUDIO = ".audio";
    static const char* EXTENSION_SCRIPT = ".cs";

    static const std::vector<std::string> supported_formats_image
    {
            ".jpg",
            ".png",
            ".bmp",
            ".tga",
            ".dds",
            ".exr",
            ".raw",
            ".gif",
            ".hdr",
            ".ico",
            ".iff",
            ".jng",
            ".jpeg",
            ".koala",
            ".kodak",
            ".mng",
            ".pcx",
            ".pbm",
            ".pgm",
            ".ppm",
            ".pfm",
            ".pict",
            ".psd",
            ".raw",
            ".sgi",
            ".targa",
            ".tiff",
            ".tif", // tiff, tif
            ".wbmp",
            ".webp",
            ".xbm",
            ".xpm"
    };

    static const std::vector<std::string> supported_formats_audio
    {
        ".aiff",
        ".asf",
        ".asx",
        ".dls",
        ".flac",
        ".fsb",
        ".it",
        ".m3u",
        ".midi",
        ".mod",
        ".mp2",
        ".mp3",
        ".ogg",
        ".pls",
        ".s3m",
        ".vag", // PS2/PSP
        ".wav",
        ".wax",
        ".wma",
        ".xm",
        ".xma" // XBOX 360
    };

    static const std::vector<std::string> supported_formats_model
    {
            ".3ds",
            ".obj",
            ".fbx",
            ".blend",
            ".dae",
            ".gltf",
            ".lwo",
            ".c4d",
            ".ase",
            ".dxf",
            ".hmp",
            ".md2",
            ".md3",
            ".md5",
            ".mdc",
            ".mdl",
            ".nff",
            ".ply",
            ".stl",
            ".x",
            ".smd",
            ".lxo",
            ".lws",
            ".ter",
            ".ac3d",
            ".ms3d",
            ".cob",
            ".q3bsp",
            ".xgl",
            ".csm",
            ".bvh",
            ".b3d",
            ".ndo"
    };

    static const std::vector<std::string> supported_formats_shader
    {
        ".hlsl"
    };

    static const std::vector<std::string> supported_formats_script
    {
        ".cs"
    };

    static const std::vector<std::string> supported_formats_font
    {
        ".ttf",
        ".ttc",
        ".cff",
        ".woff",
        ".otf",
        ".otc",
        ".pfa",
        ".pfb",
        ".fnt",
        ".bdf",
        ".pfr"
    };
}

