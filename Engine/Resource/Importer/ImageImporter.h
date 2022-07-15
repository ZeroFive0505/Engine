#pragma once

#include <vector>
#include <string>
#include "../../RHI/RHI_Definition.h"
#include "../../EngineDefinition.h"

struct FIBITMAP;

namespace PlayGround
{
	class Context;
	
	class ImageImporter
	{
	public:
		ImageImporter(Context* context);
		~ImageImporter();

		bool Load(const std::string& file_path, const uint32_t slice_index, RHI_Texture* texture);

	private:
		Context* m_Context = nullptr;
	};
}

