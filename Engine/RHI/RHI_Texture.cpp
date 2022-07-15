#include "Common.h"
#include "RHI_Texture.h"
#include "RHI_Device.h"
#include "RHI_Implementation.h"
#include "../IO/FileStream.h"
#include "../Rendering/Renderer.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/Importer/ImageImporter.h"
#include "../Profiling/Profiler.h"
#include "compressonator.h"

using namespace std;


namespace PlayGround
{
	// 이미지 압축
	static CMP_FORMAT rhi_format_amd_format(const RHI_Format format)
	{
		CMP_FORMAT format_amd = CMP_FORMAT::CMP_FORMAT_Unknown;

		switch (format)
		{
		case RHI_Format::RHI_Format_R8G8B8A8_Unorm:
			format_amd = CMP_FORMAT::CMP_FORMAT_RGBA_8888;
			break;
		case RHI_Format::RHI_Format_BC7:
			format_amd = CMP_FORMAT::CMP_FORMAT_BC7;
			break;
		case RHI_Format::RHI_Format_ASTC:
			format_amd = CMP_FORMAT::CMP_FORMAT_ASTC;
			break;
		}

		ASSERT(format_amd != CMP_FORMAT::CMP_FORMAT_Unknown);

		return format_amd;
	}

	RHI_Texture::RHI_Texture(Context* context) : IResource(context, EResourceType::Texture)
	{
		ASSERT(context != nullptr);

		Renderer* renderer = context->GetSubModule<Renderer>();
		ASSERT(renderer != nullptr);

		m_rhi_device = renderer->GetRhiDevice();
		ASSERT(m_rhi_device != nullptr);
		ASSERT(m_rhi_device->GetContextRhi()->device != nullptr);

		m_layout.fill(RHI_Image_Layout::Undefined);
	}

	RHI_Texture::~RHI_Texture()
	{
		m_data.clear();
		m_data.shrink_to_fit();

		bool destroy_main = true;
		bool destroy_per_view = true;
		RHI_DestroyResource(destroy_main, destroy_per_view);
	}

	bool RHI_Texture::SaveToFile(const string& file_path)
	{
		m_ObjectSizeCPU = 0;
		{
			if (FileSystem::Exists(file_path))
			{
				auto file = make_unique<FileStream>(file_path, FileStream_Read);
				if (file->IsOpen())
				{
					file->Read(&m_ObjectSizeCPU);
				}
			}
		}

		bool append = true;
		auto file = make_unique<FileStream>(file_path, FileStream_Write | FileStream_Append);
		if (!file->IsOpen())
			return false;

		bool dont_overwrite_data = m_ObjectSizeCPU != 0 && !HasData();
		if (dont_overwrite_data)
		{
			file->Skip
			(
				sizeof(m_ObjectSizeCPU) +
				sizeof(m_array_length) +
				sizeof(m_mip_count) +
				m_ObjectSizeCPU
			);
		}
		else
		{
			ComputeMemoryUsage();

			file->Write(m_ObjectSizeCPU);
			file->Write(m_array_length);
			file->Write(m_mip_count);

			for (RHI_Texture_Slice& slice : m_data)
			{
				for (RHI_Texture_Mip& mip : slice.mips)
				{
					file->Write(mip.bytes);
				}
			}

			m_data.clear();
			m_data.shrink_to_fit();
		}

		file->Write(m_width);
		file->Write(m_height);
		file->Write(m_channel_count);
		file->Write(m_bits_per_channel);
		file->Write(static_cast<uint32_t>(m_format));
		file->Write(m_flags);
		file->Write(GetObjectID());
		file->Write(GetResourceFilePath());

		return true;
	}

	bool RHI_Texture::LoadFromFile(const string& file_path)
	{
		if (!FileSystem::IsFile(file_path))
		{
			LOG_ERROR("\"%s\" is not a valid file path.", file_path.c_str());
			return false;
		}

		m_IsLoading = true;

		m_data.clear();
		m_data.shrink_to_fit();

		bool loaded = false;
		bool is_native_format = FileSystem::IsEngineTextureFile(file_path);
		bool is_foreign_format = FileSystem::IsSupportedImageFile(file_path);
		{
			if (is_native_format)
			{
				auto file = make_unique<FileStream>(file_path, FileStream_Read);
				if (!file->IsOpen())
				{
					m_IsLoading = false;
					return false;
				}

				m_data.clear();
				m_data.shrink_to_fit();

				file->Read(&m_ObjectSizeCPU);
				file->Read(&m_array_length);
				file->Read(&m_mip_count);

				m_data.resize(m_array_length);
				for (RHI_Texture_Slice& slice : m_data)
				{
					slice.mips.resize(m_mip_count);
					for (RHI_Texture_Mip& mip : slice.mips)
					{
						file->Read(&mip.bytes);
					}
				}

				file->Read(&m_width);
				file->Read(&m_height);
				file->Read(&m_channel_count);
				file->Read(&m_bits_per_channel);
				file->Read(reinterpret_cast<uint32_t*>(&m_format));
				file->Read(&m_flags);
				SetObjectID(file->ReadAs<uint64_t>());
				SetResourceFilePath(file->ReadAs<string>());

				loaded = true;
			}
			else if (is_foreign_format)
			{
				vector<string> file_paths = { file_path };

				if (m_ResourceType == EResourceType::Texture2dArray)
				{
					string file_path_extension = FileSystem::GetExtensionFromFilePath(file_path);
					string file_path_no_extension = FileSystem::GetFilePathWithoutExtension(file_path);
					string file_path_no_digit = file_path_no_extension.substr(0, file_path_no_extension.size() - 1);

					uint32_t index = 1;
					string file_path_guess = file_path_no_digit + to_string(index) + file_path_extension;
					while (FileSystem::Exists(file_path_guess))
					{
						file_paths.emplace_back(file_path_guess);
						file_path_guess = file_path_no_digit + to_string(++index) + file_path_extension;
					}
				}

				ImageImporter* image_importer = m_Context->GetSubModule<ResourceCache>()->GetImageImporter();
				for (uint32_t slice_index = 0; slice_index < static_cast<uint32_t>(file_paths.size()); slice_index++)
				{
					if (!image_importer->Load(file_paths[slice_index], slice_index, this))
					{
						m_IsLoading = false;
						return false;
					}
				}

				SetResourceFilePath(file_path);

				loaded = true;
			}
		}

		if (m_ObjectName.empty())
		{
			m_ObjectName = GetResourceName();
		}

		if (!loaded)
		{
			LOG_ERROR("Failed to load \"%s\".", file_path.c_str());
			m_IsLoading = false;
			return false;
		}

		if (m_flags & RHI_Texture_Mips)
		{
			if (!is_native_format)
			{
				uint32_t width = m_width;
				uint32_t height = m_height;
				uint32_t smallest_dimension = 1;
				while (width > smallest_dimension && height > smallest_dimension)
				{
					width /= 2;
					height /= 2;
					CreateMip(0);
				}
			}

			m_flags |= RHI_Texture_PerMipViews;
			m_flags |= RHI_Texture_Uav;
		}

		if (!RHI_CreateResource())
		{
			string path = is_native_format ? GetResourceFilePathNative() : GetResourceFilePath();
			LOG_ERROR("Failed to create shader resource for \"%s\".", path.c_str());
			m_IsLoading = false;
			return false;
		}

		if (is_native_format)
		{
			m_data.clear();
			m_data.shrink_to_fit();
		}

		ComputeMemoryUsage();

		if (m_flags & RHI_Texture_Mips)
		{
			m_Context->GetSubModule<Renderer>()->RequestTextureMipGeneration(shared_from_this());
		}

		m_IsLoading = false;
		return true;
	}

	RHI_Texture_Mip& RHI_Texture::CreateMip(const uint32_t array_index)
	{
		while (array_index >= m_data.size())
		{
			m_data.emplace_back();
		}

		RHI_Texture_Mip& mip = m_data[array_index].mips.emplace_back();

		uint32_t mip_index = m_data[array_index].GetMipCount() - 1;
		uint32_t width = m_width >> mip_index;
		uint32_t height = m_height >> mip_index;
		const size_t size_bytes = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(m_channel_count) * static_cast<size_t>(m_bits_per_channel / 8);
		mip.bytes.resize(size_bytes);
		mip.bytes.reserve(mip.bytes.size());

		if (!m_data.empty())
		{
			m_array_length = static_cast<uint32_t>(m_data.size());
			m_mip_count = m_data[0].GetMipCount();
		}

		return mip;
	}

	RHI_Texture_Mip& RHI_Texture::GetMip(const uint32_t array_index, const uint32_t mip_index)
	{
		static RHI_Texture_Mip empty;

		if (array_index >= m_data.size())
			return empty;

		if (mip_index >= m_data[array_index].mips.size())
			return empty;

		return m_data[array_index].mips[mip_index];
	}

	RHI_Texture_Slice& RHI_Texture::GetSlice(const uint32_t array_index)
	{
		static RHI_Texture_Slice empty;

		if (array_index >= m_data.size())
			return empty;

		return m_data[array_index];
	}

	void RHI_Texture::SetFlag(const uint32_t flag, bool enabled /*= true*/)
	{
		if (enabled)
		{
			m_flags |= flag;
		}
		else
		{
			m_flags &= ~flag;
		}
	}

	bool RHI_Texture::Compress(const RHI_Format format)
	{
		bool success = true;

		for (uint32_t index_array = 0; index_array < m_array_length; index_array++)
		{
			for (uint32_t index_mip = 0; index_mip < m_mip_count; index_mip++)
			{
				uint32_t width = m_width >> index_mip;
				uint32_t height = m_height >> index_mip;
				uint32_t src_pitch = width * m_channel_count * (m_bits_per_channel / 8);
				RHI_Texture_Mip& src_data = GetMip(index_array, index_mip);

				CMP_Texture src_texture = {};
				src_texture.dwSize = sizeof(src_texture);
				src_texture.format = rhi_format_amd_format(m_format);
				src_texture.dwWidth = width;
				src_texture.dwHeight = height;
				src_texture.dwPitch = src_pitch;
				src_texture.dwDataSize = CMP_CalculateBufferSize(&src_texture);
				src_texture.pData = reinterpret_cast<CMP_BYTE*>(&src_data.bytes[0]);

				vector<std::byte> dst_data;
				dst_data.reserve(src_texture.dwDataSize);
				dst_data.resize(src_texture.dwDataSize);

				CMP_Texture dst_texture = {};
				dst_texture.dwSize = sizeof(dst_texture);
				dst_texture.dwWidth = src_texture.dwWidth;
				dst_texture.dwHeight = src_texture.dwHeight;
				dst_texture.dwPitch = src_texture.dwPitch;
				dst_texture.format = rhi_format_amd_format(format);
				dst_texture.dwDataSize = CMP_CalculateBufferSize(&dst_texture);
				dst_texture.pData = reinterpret_cast<CMP_BYTE*>(&dst_data[0]);

				CMP_BYTE alpha_threshold = IsTransparent() ? 128 : 0;

				float compression_quality = 0.05f;

				CMP_Speed compression_speed = CMP_Speed::CMP_Speed_Normal;
				if (dst_texture.format == CMP_FORMAT_BC1 && alpha_threshold != 0)
				{
					compression_speed = CMP_Speed::CMP_Speed_Normal;
				}

				CMP_CompressOptions options = {};
				options.dwSize = sizeof(options);
				options.nAlphaThreshold = alpha_threshold;
				options.nCompressionSpeed = compression_speed;
				options.fquality = compression_quality;
				options.dwnumThreads = 0;
				options.nEncodeWith = CMP_HPC;

				if (CMP_ConvertTexture(&src_texture, &dst_texture, &options, nullptr) == CMP_OK)
				{
					m_data[index_array].mips[index_mip].bytes = dst_data;

					m_format = format;
				}
				else
				{
					LOG_ERROR("Failed to compress slice %d, mip %d.", index_array, index_mip);
					success = false;
					continue;

				}
			}
		}

		return success;
	}

	void RHI_Texture::ComputeMemoryUsage()
	{
		m_ObjectSizeCPU = 0;
		m_ObjectSizeGPU = 0;

		for (uint32_t array_index = 0; array_index < m_array_length; array_index++)
		{
			for (uint32_t mip_index = 0; mip_index < m_mip_count; mip_index++)
			{
				const uint32_t mip_width = m_width >> mip_index;
				const uint32_t mip_height = m_height >> mip_index;

				if (array_index < m_data.size())
				{
					if (mip_index < m_data[array_index].mips.size())
					{
						m_ObjectSizeCPU += m_data[array_index].mips[mip_index].bytes.size();
					}
				}
				m_ObjectSizeGPU += mip_width * mip_height * m_channel_count * (m_bits_per_channel / 8);
			}
		}
	}

	void RHI_Texture::SetLayout(const RHI_Image_Layout new_layout, RHI_CommandList* cmd_list, const int mip /*= -1*/, const bool ranged /*= true*/)
	{
		const bool mip_specified = mip != -1;
		uint32_t mip_start = mip_specified ? mip : 0;
		uint32_t mip_remaining = m_mip_count - mip_start;
		uint32_t mip_range = ranged ? (mip_specified ? mip_remaining : m_mip_count) : 1;

		{
			if (mip_specified)
			{
				ASSERT(HasPerMipViews());
			}

			ASSERT(mip_remaining <= m_mip_count);
		}

		{
			if (mip_specified && !ranged)
			{
				if (m_layout[mip_start] == new_layout)
					return;
			}
			else
			{
				bool all_set = true;

				for (uint32_t mip_index = mip_start; mip_index < mip_range; mip_index++)
				{
					if (m_layout[mip_index] != new_layout)
					{
						mip_start = mip_index;
						mip_remaining = m_mip_count - mip_start;
						mip_range = ranged ? (mip_specified ? mip_remaining : m_mip_count) : mip_remaining;
						all_set = false;
						break;
					}
				}

				if (all_set)
					return;
			}
		}

		if (cmd_list != nullptr)
		{
			while (IsLoading())
			{
				LOG_INFO("Waiting for texture \"%s\" to finish loading...", m_ObjectName.c_str());
				this_thread::sleep_for(chrono::milliseconds(16));
			}

			RHI_SetLayout(new_layout, cmd_list, mip_start, mip_range);
			m_Context->GetSubModule<Profiler>()->m_Rhi_pipeline_barriers++;
		}

		for (uint32_t i = mip_start; i < mip_start + mip_range; i++)
		{
			m_layout[i] = new_layout;
		}
	}

	bool RHI_Texture::DoAllMipsHaveTheSameLayout() const
	{
		for (uint32_t mip_index = 1; mip_index < m_mip_count; mip_index++)
		{
			if (m_layout[0] != m_layout[mip_index])
			{
				return false;
			}
		}

		return true;
	}
}