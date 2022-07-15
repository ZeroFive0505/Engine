#pragma once

#include <atlbase.h>
#include <dxcapi.h>

namespace PlayGround
{
	inline bool error_check(IDxcResult* dxc_result)
	{
		IDxcBlobEncoding* error_buffer = nullptr;
		HRESULT result = dxc_result->GetErrorBuffer(&error_buffer);

		if (SUCCEEDED(result))
		{
			// 로그 출력
			std::stringstream ss(std::string(static_cast<char*>(error_buffer->GetBufferPointer()), error_buffer->GetBufferSize()));
			std::string line;

			while (getline(ss, line, '\n'))
			{
				if (line.find("error") != std::string::npos)
				{
					LOG_ERROR(line);
				}
				else if (line.find("warning") != std::string::npos)
				{
					LOG_WARNING(line);
				}
				else if (!FileSystem::IsEmptyOrWhitespace(line))
				{
					LOG_INFO(line);
				}
			}
		}
		else
		{
			LOG_ERROR("Failed to get error buffer");
		}

		if (error_buffer)
		{
			error_buffer->Release();
			error_buffer = nullptr;
		}

		dxc_result->GetStatus(&result);
		return result == S_OK;
	}

	class DirectXShaderCompiler
	{
	public:
		DirectXShaderCompiler()
		{
			DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_Utils));
			DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_Compiler));
		}

		IDxcResult* Compile(const std::string& source, std::vector<std::string>& arguments)
		{
			// 쉐이더 소스를 가져온다.
			DxcBuffer dxc_buffer;
			CComPtr<IDxcBlobEncoding> blob_encoding = nullptr;
			{
				if (FAILED(m_Utils->CreateBlobFromPinned(source.c_str(), static_cast<uint32_t>(source.size()), CP_UTF8, &blob_encoding)))
				{
					LOG_ERROR("Failed to load shader source.");
					return nullptr;
				}

				dxc_buffer.Ptr = blob_encoding->GetBufferPointer();
				dxc_buffer.Size = blob_encoding->GetBufferSize();
				dxc_buffer.Encoding = DXC_CP_ACP;
			}

			std::vector<std::wstring> arguments_wstring;
			arguments_wstring.reserve(arguments.size());

			for (const std::string& str : arguments)
			{
				arguments_wstring.emplace_back(FileSystem::StringToWstring(str));
			}

			std::vector<LPCWSTR> arguments_lpcwstr;
			arguments_lpcwstr.reserve(arguments.size());

			for (const std::wstring& wstr : arguments_wstring)
			{
				arguments_lpcwstr.emplace_back(wstr.c_str());
			}

			// 컴파일
			IDxcResult* dxc_result = nullptr;
			m_Compiler->Compile(&dxc_buffer, arguments_lpcwstr.data(), static_cast<uint32_t>(arguments_lpcwstr.size()), nullptr, IID_PPV_ARGS(&dxc_result));

			if (!error_check(dxc_result))
			{
				LOG_ERROR("Failed to compile");

				if (dxc_result)
				{
					dxc_result->Release();
					dxc_result = nullptr;
				}
			}

			return dxc_result;
		}

		static DirectXShaderCompiler& Get()
		{
			static DirectXShaderCompiler instance;
			return instance;
		}

	private:
		CComPtr<IDxcUtils> m_Utils = nullptr;
		CComPtr<IDxcCompiler3> m_Compiler = nullptr;
	};
}