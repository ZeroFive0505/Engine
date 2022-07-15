#include "Common.h"
#include "ImageImporter.h"
#define FREEIMAGE_LIB
#include <FreeImage.h>
#include <Utilities.h>
#include "../../Threading/Threading.h"
#include "../../RHI/RHI_Texture2D.h"

using namespace std;

namespace PlayGround
{
	static bool get_is_srgb(FIBITMAP* bitmap)
	{
		if (FIICCPROFILE* icc_profile = FreeImage_GetICCProfile(bitmap))
		{
			int i, tag_count, tag_ofs, tag_size;
			unsigned char* icc, * tag, * icc_end;
			char tag_data[256];

			if (!icc_profile->data)
				return false;

			icc = static_cast<unsigned char*>(icc_profile->data);
			if (icc[36] != 'a' || icc[37] != 'c' || icc[38] != 's' || icc[39] != 'p')
				return false; // ICC파일이 아님

			icc_end = icc + icc_profile->size;
			tag_count = icc[128 + 0] * 0x1000000 + icc[128 + 1] * 0x10000 + icc[128 + 2] * 0x100 + icc[128 + 3];

			// desc 태그를 찾는다.
			for (i = 0; i < tag_count; i++)
			{
				tag = icc + 128 + 4 + i * 12;
				if (tag > icc_end)
					return false; // ICC 파일이 아님

				// desc 태그 확인
				if (memcmp(tag, "desc", 4) == 0)
				{
					tag_ofs = tag[4] * 0x1000000 + tag[5] * 0x10000 + tag[6] * 0x100 + tag[7];
					tag_size = tag[8] * 0x1000000 + tag[9] * 0x10000 + tag[10] * 0x100 + tag[11];

					if (static_cast<uint32_t>(tag_ofs + tag_size) > icc_profile->size)
						return false; // ICC 파일이 아님

					strncpy_s(tag_data, (char*)(icc + tag_ofs + 12), min(255, tag_size - 12));
					if (strcmp(tag_data, "sRGB IEC61966-2.1") == 0 || strcmp(tag_data, "sRGB IEC61966-2-1") == 0 || strcmp(tag_data, "sRGB IEC61966") == 0 || strcmp(tag_data, "* wsRGB") == 0)
						return true;

					return false;
				}
			}

			return false;
		}
		
		return false;
	}

	// 채널의 비트수를 가져온다.
	static uint32_t get_bits_per_channel(FIBITMAP* bitmap)
	{
		ASSERT(bitmap != nullptr);

		const FREE_IMAGE_TYPE type = FreeImage_GetImageType(bitmap);
		uint32_t size = 0;

		if (type == FIT_BITMAP)
		{
			size = sizeof(BYTE);
		}
		else if (type == FIT_UINT16 || type == FIT_RGB16 || type == FIT_RGBA16)
		{
			size = sizeof(WORD);
		}
		else if (type == FIT_FLOAT || type == FIT_RGBF || type == FIT_RGBAF)
		{
			size = sizeof(float);
		}

		ASSERT(size != 0);

		return size * 8;
	}

	// 채널 카운트를 가져온다.
	static uint32_t get_channel_count(FIBITMAP* bitmap)
	{
		ASSERT(bitmap != nullptr);

		// 픽셀당 비트수를 가져온다.
		const uint32_t bits_per_pixel = FreeImage_GetBPP(bitmap);
		// 채널당 비트수를 가져온다.
		const uint32_t bits_per_channel = get_bits_per_channel(bitmap);
		// 나누면 채널의 갯수
		const uint32_t channel_count = bits_per_pixel / bits_per_channel;

		ASSERT(channel_count != 0);

		return channel_count;
	}

	// RHI 포맷으로 변환
	static RHI_Format get_rhi_format(const uint32_t bits_per_channel, const uint32_t channel_count)
	{
		ASSERT(bits_per_channel != 0);
		ASSERT(channel_count != 0);

		RHI_Format format = RHI_Format_Undefined;

		if (channel_count == 1)
		{
			if (bits_per_channel == 8)
			{
				format = RHI_Format_R8_Unorm;
			}
			else if (bits_per_channel == 16)
			{
				format = RHI_Format_R16_Unorm;
			}
		}
		else if (channel_count == 2)
		{
			if (bits_per_channel == 8)
			{
				format = RHI_Format_R8G8_Unorm;
			}
		}
		else if (channel_count == 3)
		{
			if (bits_per_channel == 32)
			{
				format = RHI_Format_R32G32B32_Float;
			}
		}
		else if (channel_count == 4)
		{
			if (bits_per_channel == 8)
			{
				format = RHI_Format_R8G8B8A8_Unorm;
			}
			else if (bits_per_channel == 16)
			{
				format = RHI_Format_R16G16B16A16_Unorm;
			}
			else if (bits_per_channel == 32)
			{
				format = RHI_Format_R32G32B32A32_Float;
			}
		}


		ASSERT(format != RHI_Format_Undefined);

		return format;
	}

	// 8비트로 변환한다. 만약 비트맵이 16비트 이상의 비트맵일 경우 또는
	// 그레이 스케일의 비트맵일 경우 결과는 그레이 스케일 비트맵이 될 것이다.
	static FIBITMAP* convert_to_8bits(FIBITMAP* bitmap)
	{
		ASSERT(bitmap != nullptr);

		FIBITMAP* prev_bitmap = bitmap;
		bitmap = FreeImage_ConvertTo8Bits(prev_bitmap);
		FreeImage_Unload(prev_bitmap);

		ASSERT(bitmap != nullptr);

		return bitmap;
	}

	static FIBITMAP* convert_to_32bits(FIBITMAP* bitmap)
	{
		ASSERT(bitmap != nullptr);

		FIBITMAP* prev_bitmap = bitmap;
		bitmap = FreeImage_ConvertTo32Bits(prev_bitmap);
		FreeImage_Unload(prev_bitmap);

		ASSERT(bitmap != nullptr);

		return bitmap;
	}

	static FIBITMAP* rescale(FIBITMAP* bitmap, const uint32_t width, const uint32_t height)
	{
		ASSERT(bitmap != nullptr);
		ASSERT(width != 0);
		ASSERT(height != 0);

		FIBITMAP* prev_bitmap = bitmap;
		bitmap = FreeImage_Rescale(prev_bitmap, width, height, FREE_IMAGE_FILTER::FILTER_LANCZOS3);

		if (!bitmap)
		{
			LOG_ERROR("Failed to rescale the iamge");
			return prev_bitmap;
		}

		FreeImage_Unload(prev_bitmap);
		return bitmap;
	}

	static FIBITMAP* apply_bitmap_corrections(FIBITMAP* bitmap)
	{
		ASSERT(bitmap != nullptr);

		// 기본 비트맵ㅇ브로 전환한다.
		const FREE_IMAGE_TYPE type = FreeImage_GetImageType(bitmap);

		if (type != FIT_BITMAP)
		{
			// FreeImage는 FIT_RGBF 포맷을 변환할 수 없다.
			if (type != FIT_RGBF)
			{
				const auto prev_bitmap = bitmap;
				bitmap = FreeImage_ConvertToType(bitmap, FIT_BITMAP);
				FreeImage_Unload(prev_bitmap);
			}
		}

		// 8비트보다 적은 색을 가지고 있는 텍스쳐의 경우 R8G8B8A8 타입으로 변환한다.
		if (FreeImage_GetColorsUsed(bitmap) <= 256 && FreeImage_GetColorType(bitmap) != FIC_RGB)
			bitmap = convert_to_32bits(bitmap);

		// 3가지 채널 그리고 8비트 이상의 경우 R8G8B8A8 포맷으로 변환한다.
		if (get_channel_count(bitmap) == 3 && get_bits_per_channel(bitmap) == 8)
			bitmap = convert_to_32bits(bitmap);


		// 대다수의 GPU의 경우 32 비트 텍스쳐를 이용할 수 없다.
		// 그걸 방지하기 위해서 32 비트를 유지하고 RGBA로 변환한다.
		if (get_channel_count(bitmap) == 3 && get_bits_per_channel(bitmap) == 32)
		{
			FIBITMAP* prev_bitmap = bitmap;
			bitmap = FreeImage_ConvertToRGBAF(bitmap);
			FreeImage_Unload(prev_bitmap);
		}

		// BGR를 RGB로 변환
		if (FreeImage_GetBPP(bitmap) == 32)
		{
			if (FreeImage_GetRedMask(bitmap) == 0xff0000 && get_channel_count(bitmap) >= 2)
			{
				if (!SwapRedBlue32(bitmap))
				{
					LOG_ERROR("Failed to swap red with blue channel");
				}
			}
		}

		// 상하를 뒤집는다.
		FreeImage_FlipVertical(bitmap);

		return bitmap;
	}

	static void get_bits_from_bitmap(RHI_Texture_Mip* mip, FIBITMAP* bitmap, const uint32_t width, const uint32_t height,
		const uint32_t channel_count, const uint32_t bits_per_channel)
	{
		// 확인
		ASSERT(mip != nullptr);
		ASSERT(bitmap != nullptr);
		ASSERT(width != 0);
		ASSERT(height != 0);
		ASSERT(channel_count != 0);

		// 예상 사이즈를 측정한다.
		const size_t size_bytes = static_cast<size_t>(width) *
			static_cast<size_t>(height) * static_cast<size_t>(channel_count) * static_cast<size_t>(bits_per_channel / 8);

		if (size_bytes != mip->bytes.size())
		{
			mip->bytes.clear();
			mip->bytes.reserve(size_bytes);
			mip->bytes.resize(size_bytes);
		}

		// 데이터 복사
		BYTE* bytes = FreeImage_GetBits(bitmap);
		memcpy(&mip->bytes[0], bytes, size_bytes);
	}

	ImageImporter::ImageImporter(Context* context)
	{
		// 초기화
		m_Context = context;
		FreeImage_Initialise();

		// 에러 핸들링 람다 함수
		const auto free_image_error_handler = [](const FREE_IMAGE_FORMAT fif, const char* message)
		{
			const auto text = (message != nullptr) ? message : "Unknown error";
			const auto format = (fif != FIF_UNKNOWN) ? FreeImage_GetFormatFromFIF(fif) : "Unknown";

			LOG_ERROR("%s, Format: %s", text, format);
		};

		FreeImage_SetOutputMessage(free_image_error_handler);

		m_Context->GetSubModule<Settings>()->RegisterThirdParty("FreeImage", FreeImage_GetVersion(), "http://freeimage.sourceforge.net/download.html");
	}

	ImageImporter::~ImageImporter()
	{
		FreeImage_DeInitialise();
	}

	bool ImageImporter::Load(const string& file_path, const uint32_t slice_index, RHI_Texture* texture)
	{
		ASSERT(texture != nullptr);

		if (!FileSystem::Exists(file_path))
		{
			LOG_ERROR("Path \"%s\" is invalid.", file_path.c_str());
			return false;
		}

		// 이미지 포맷을 가져온다.
		FREE_IMAGE_FORMAT format = FIF_UNKNOWN;
		{
			format = FreeImage_GetFileType(file_path.c_str(), 0);

			// 포맷 파악이 아직도 안되면 패스에서 가져온다.
			if (format == FIF_UNKNOWN)
				format = FreeImage_GetFIFFromFilename(file_path.c_str());

			// 그래도 포맷을 모른다면 반환.
			if (!FreeImage_FIFSupportsReading(format))
			{
				LOG_ERROR("Unsupported format");
				return false;
			}
		}

		FIBITMAP* bitmap = FreeImage_Load(format, file_path.c_str());
		if (!bitmap)
		{
			LOG_ERROR("Failed to load \"%s\"", file_path.c_str());
			return false;
		}

		// 이미지의 타입들을 로도흔다.
		const bool is_transparent = FreeImage_IsTransparent(bitmap);
		const bool is_grayscale = FreeImage_GetColorType(bitmap) == FREE_IMAGE_COLOR_TYPE::FIC_MINISBLACK;
		const bool is_srgb = get_is_srgb(bitmap);

		bitmap = apply_bitmap_corrections(bitmap);

		if (!bitmap)
		{
			LOG_ERROR("Failed to apply bitmap corrections");
			return false;
		}

		// 이미지의 상세 정보를 가져온다.
		const uint32_t bits_per_channel = get_bits_per_channel(bitmap);
		const uint32_t channel_count = get_channel_count(bitmap);
		const RHI_Format image_format = get_rhi_format(bits_per_channel, channel_count);
		const bool user_define_dimensions = (texture->GetWidth() != 0 && texture->GetWidth() != 0);
		const bool dimension_mismatch = (FreeImage_GetWidth(bitmap) != texture->GetWidth() && FreeImage_GetHeight(bitmap) != texture->GetHeight());
		const bool scale = user_define_dimensions && dimension_mismatch;
		bitmap = scale ? rescale(bitmap, texture->GetWidth(), texture->GetHeight()) : bitmap;
		const unsigned int width = FreeImage_GetWidth(bitmap);
		const unsigned int height = FreeImage_GetHeight(bitmap);

		// 데이터를 복사한다.
		RHI_Texture_Mip& mip = texture->CreateMip(slice_index);
		get_bits_from_bitmap(&mip, bitmap, width, height, channel_count, bits_per_channel);

		// 이미지 해제
		FreeImage_Unload(bitmap);

		// RHI 텍스쳐 포맷을 정해준다.
		{
			ASSERT(bits_per_channel != 0);
			ASSERT(channel_count != 0);
			ASSERT(width != 0);
			ASSERT(height != 0);

			texture->SetBitsPerChannel(bits_per_channel);
			texture->SetWidth(width);
			texture->SetHeight(height);
			texture->SetChannelCount(channel_count);
			texture->SetFormat(image_format);

			uint32_t flags = texture->GetFlags();

			flags |= is_transparent ? RHI_Texture_Transparent : 0;
			flags |= is_grayscale ? RHI_Texture_Grayscale : 0;
			flags |= is_srgb ? RHI_Texture_Srgb : 0;

			texture->SetFlags(flags);
		}

		return true;
	}
}
