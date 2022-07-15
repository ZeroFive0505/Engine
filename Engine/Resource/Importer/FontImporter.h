#pragma once

#include <string>
#include "../../EngineDefinition.h"

struct FT_LibraryRec_;
struct FT_StrokerRec_;

namespace PlayGround
{
	class Context;
	class Font;

	class FontImporter
	{
	public:
		FontImporter(Context* context);
		~FontImporter();

		bool LoadFromFile(Font* font, const std::string& file_path);

	private:
		Context* m_Context = nullptr;
		FT_LibraryRec_* m_Library = nullptr;
		FT_StrokerRec_* m_Stroker = nullptr;
	};
}
