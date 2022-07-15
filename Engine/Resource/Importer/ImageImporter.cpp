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
				return false; // ICC������ �ƴ�

			icc_end = icc + icc_profile->size;
			tag_count = icc[128 + 0] * 0x1000000 + icc[128 + 1] * 0x10000 + icc[128 + 2] * 0x100 + icc[128 + 3];

			// desc �±׸� ã�´�.
			for (i = 0; i < tag_count; i++)
			{
				tag = icc + 128 + 4 + i * 12;
				if (tag > icc_end)
					return false; // ICC ������ �ƴ�

				// desc �±� Ȯ��
				if (memcmp(tag, "desc", 4) == 0)
				{
					tag_ofs = tag[4] * 0x1000000 + tag[5] * 0x10000 + tag[6] * 0x100 + tag[7];
					tag_size = tag[8] * 0x1000000 + tag[9] * 0x10000 + tag[10] * 0x100 + tag[11];

					if (static_cast<uint32_t>(tag_ofs + tag_size) > icc_profile->size)
						return false; // ICC ������ �ƴ�

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

	// ä���� ��Ʈ���� �����´�.
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

	// ä�� ī��Ʈ�� �����´�.
	static uint32_t get_channel_count(FIBITMAP* bitmap)
	{
		ASSERT(bitmap != nullptr);

		// �ȼ��� ��Ʈ���� �����´�.
		const uint32_t bits_per_pixel = FreeImage_GetBPP(bitmap);
		// ä�δ� ��Ʈ���� �����´�.
		const uint32_t bits_per_channel = get_bits_per_channel(bitmap);
		// ������ ä���� ����
		const uint32_t channel_count = bits_per_pixel / bits_per_channel;

		ASSERT(channel_count != 0);

		return channel_count;
	}

	// RHI �������� ��ȯ
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

	// 8��Ʈ�� ��ȯ�Ѵ�. ���� ��Ʈ���� 16��Ʈ �̻��� ��Ʈ���� ��� �Ǵ�
	// �׷��� �������� ��Ʈ���� ��� ����� �׷��� ������ ��Ʈ���� �� ���̴�.
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

		// �⺻ ��Ʈ�ʤ���� ��ȯ�Ѵ�.
		const FREE_IMAGE_TYPE type = FreeImage_GetImageType(bitmap);

		if (type != FIT_BITMAP)
		{
			// FreeImage�� FIT_RGBF ������ ��ȯ�� �� ����.
			if (type != FIT_RGBF)
			{
				const auto prev_bitmap = bitmap;
				bitmap = FreeImage_ConvertToType(bitmap, FIT_BITMAP);
				FreeImage_Unload(prev_bitmap);
			}
		}

		// 8��Ʈ���� ���� ���� ������ �ִ� �ؽ����� ��� R8G8B8A8 Ÿ������ ��ȯ�Ѵ�.
		if (FreeImage_GetColorsUsed(bitmap) <= 256 && FreeImage_GetColorType(bitmap) != FIC_RGB)
			bitmap = convert_to_32bits(bitmap);

		// 3���� ä�� �׸��� 8��Ʈ �̻��� ��� R8G8B8A8 �������� ��ȯ�Ѵ�.
		if (get_channel_count(bitmap) == 3 && get_bits_per_channel(bitmap) == 8)
			bitmap = convert_to_32bits(bitmap);


		// ��ټ��� GPU�� ��� 32 ��Ʈ �ؽ��ĸ� �̿��� �� ����.
		// �װ� �����ϱ� ���ؼ� 32 ��Ʈ�� �����ϰ� RGBA�� ��ȯ�Ѵ�.
		if (get_channel_count(bitmap) == 3 && get_bits_per_channel(bitmap) == 32)
		{
			FIBITMAP* prev_bitmap = bitmap;
			bitmap = FreeImage_ConvertToRGBAF(bitmap);
			FreeImage_Unload(prev_bitmap);
		}

		// BGR�� RGB�� ��ȯ
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

		// ���ϸ� �����´�.
		FreeImage_FlipVertical(bitmap);

		return bitmap;
	}

	static void get_bits_from_bitmap(RHI_Texture_Mip* mip, FIBITMAP* bitmap, const uint32_t width, const uint32_t height,
		const uint32_t channel_count, const uint32_t bits_per_channel)
	{
		// Ȯ��
		ASSERT(mip != nullptr);
		ASSERT(bitmap != nullptr);
		ASSERT(width != 0);
		ASSERT(height != 0);
		ASSERT(channel_count != 0);

		// ���� ����� �����Ѵ�.
		const size_t size_bytes = static_cast<size_t>(width) *
			static_cast<size_t>(height) * static_cast<size_t>(channel_count) * static_cast<size_t>(bits_per_channel / 8);

		if (size_bytes != mip->bytes.size())
		{
			mip->bytes.clear();
			mip->bytes.reserve(size_bytes);
			mip->bytes.resize(size_bytes);
		}

		// ������ ����
		BYTE* bytes = FreeImage_GetBits(bitmap);
		memcpy(&mip->bytes[0], bytes, size_bytes);
	}

	ImageImporter::ImageImporter(Context* context)
	{
		// �ʱ�ȭ
		m_Context = context;
		FreeImage_Initialise();

		// ���� �ڵ鸵 ���� �Լ�
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

		// �̹��� ������ �����´�.
		FREE_IMAGE_FORMAT format = FIF_UNKNOWN;
		{
			format = FreeImage_GetFileType(file_path.c_str(), 0);

			// ���� �ľ��� ������ �ȵǸ� �н����� �����´�.
			if (format == FIF_UNKNOWN)
				format = FreeImage_GetFIFFromFilename(file_path.c_str());

			// �׷��� ������ �𸥴ٸ� ��ȯ.
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

		// �̹����� Ÿ�Ե��� �ε����.
		const bool is_transparent = FreeImage_IsTransparent(bitmap);
		const bool is_grayscale = FreeImage_GetColorType(bitmap) == FREE_IMAGE_COLOR_TYPE::FIC_MINISBLACK;
		const bool is_srgb = get_is_srgb(bitmap);

		bitmap = apply_bitmap_corrections(bitmap);

		if (!bitmap)
		{
			LOG_ERROR("Failed to apply bitmap corrections");
			return false;
		}

		// �̹����� �� ������ �����´�.
		const uint32_t bits_per_channel = get_bits_per_channel(bitmap);
		const uint32_t channel_count = get_channel_count(bitmap);
		const RHI_Format image_format = get_rhi_format(bits_per_channel, channel_count);
		const bool user_define_dimensions = (texture->GetWidth() != 0 && texture->GetWidth() != 0);
		const bool dimension_mismatch = (FreeImage_GetWidth(bitmap) != texture->GetWidth() && FreeImage_GetHeight(bitmap) != texture->GetHeight());
		const bool scale = user_define_dimensions && dimension_mismatch;
		bitmap = scale ? rescale(bitmap, texture->GetWidth(), texture->GetHeight()) : bitmap;
		const unsigned int width = FreeImage_GetWidth(bitmap);
		const unsigned int height = FreeImage_GetHeight(bitmap);

		// �����͸� �����Ѵ�.
		RHI_Texture_Mip& mip = texture->CreateMip(slice_index);
		get_bits_from_bitmap(&mip, bitmap, width, height, channel_count, bits_per_channel);

		// �̹��� ����
		FreeImage_Unload(bitmap);

		// RHI �ؽ��� ������ �����ش�.
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
