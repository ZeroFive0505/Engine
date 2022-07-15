#pragma once

#include <vector>
#include <string>
#include "../EngineDefinition.h"

/// <summary>
/// 엔진에서 파일과 관련된 연산을 위해서 만들어진 클래스
/// 경로를 찾아올때 유용한 함수들과 확장자 상수들이 정의되어 있다.
/// </summary>
namespace PlayGround
{
	class FileSystem
	{
    public:
        // 텍스트 파일 생성
        static void CreateTextFile(const std::string& file_path, const std::string& text);

        // 공백인지
        static bool IsEmptyOrWhitespace(const std::string& var);
        // 알파벳 또는 숫자인디
        static bool IsAlphanumeric(const std::string& var);
        // #./! 등의 허용되지 않는 문자를 제거한다.
        static std::string RemoveIllegalCharacters(const std::string& text);
        // 특정 문자열 바로전까지의 문자열을 가져온다.
        static std::string GetStringBeforeExpression(const std::string& str, const std::string& exp);
        // 특정 문자열 이후의 문자열을 가져온다.
        static std::string GetStringAfterExpression(const std::string& str, const std::string& exp);
        // 특정 문자열 사이의 문자열을 가져온다.
        static std::string GetStringBetweenExpressions(const std::string& str, const std::string& exp_a, const std::string& exp_b);
        // 전부 대문자로 바꾼다.
        static std::string ConvertToUppercase(const std::string& lower);
        // 특정 문자열을 from에서 to로 변환
        static std::string ReplaceExpression(const std::string& str, const std::string& from, const std::string& to);
        // 멀티바이트를 WCHAR로 변환
        static std::wstring StringToWstring(const std::string& str);

        // 파일 다이얼로그 오픈
        static void OpenDirectoryWindow(const std::string& path);
        // 디렉토리 생성
        static bool CreateDirectory_(const std::string& path);
        // 파일 제거
        static bool Delete(const std::string& path);
        // 파일 체크
        static bool Exists(const std::string& path);
        // 경로 체크
        static bool IsDirectory(const std::string& path);
        // 파일 체크
        static bool IsFile(const std::string& path);
        // 파일 복사
        static bool CopyFileFromTo(const std::string& source, const std::string& destination);
        // 경로에서 파일 이름만 가져온다.
        static std::string GetFileNameFromFilePath(const std::string& path);
        // 경로에서 파일 이름만 가져온다. (확장자 제외)
        static std::string GetFileNameWithoutExtensionFromFilePath(const std::string& path);
        // 파일경로에서 경로를 가져온다.
        static std::string GetDirectoryFromFilePath(const std::string& path);
        // 확장자를 제외한 파일경로를 가져온다.
        static std::string GetFilePathWithoutExtension(const std::string& path);
        // 확장자 변환
        static std::string ReplaceExtension(const std::string& path, const std::string& extension);
        // 확장자를 가져온다.
        static std::string GetExtensionFromFilePath(const std::string& path);
        // 파일을 엔진 자체포맷으로 변환한다.
        static std::string NativizeFilePath(const std::string& path);
        // 상대 경로를 가져온다.
        static std::string GetRelativePath(const std::string& path);
        // 현재 프로젝트가 실행되고있는 경로를 가져온다.
        static std::string GetWorkingDirectory();
        // 루트 디렉토리를 가져온다.
        static std::string GetRootDirectory(const std::string& path);
        // 부모 디렉토리를 가져온다.
        static std::string GetParentDirectory(const std::string& path);
        // 현 디렉토리에 모든 디렉토리들을 반환한다.
        static std::vector<std::string> GetDirectoriesInDirectory(const std::string& path);
        // 현 디렉토리에 모든 파일들을 반환한다.
        static std::vector<std::string> GetFilesInDirectory(const std::string& path);

        // 엔진 자체 포멧인지 확인하는  함수들

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

    // 엔진 파일 확장자
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

