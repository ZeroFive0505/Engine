#pragma once

#include <memory>
#include <unordered_map>
#include "Glyph.h"
#include "../../RHI/RHI_Definition.h"
#include "../../Resource/IResource.h"
#include "../../Math/Vector2.h"
#include "../../Math/Vector4.h"
#include "../../EngineDefinition.h"


namespace PlayGround
{
	// 폰트 힌트 타입
	enum Font_Hinting_Type
	{
		Font_Hinting_None,
		Font_Hinting_Light,
		Font_Hinting_Normal
	};
	
	// 폰트 아웃라인 타입
	enum Font_Outline_Type
	{
		Font_Outline_None,
		Font_Outline_Edge,
		Font_Outline_Positive,
		Font_Outline_Negative
	};

	// 폰트는 리소스라 IResource를 상속받는다.
	class Font : public IResource
	{
	public:
		Font(Context* context, const std::string& file_path, int font_size, const Math::Vector4& color);
		~Font() = default;

		// IResource에서 제공하는 인터페이스
		bool SaveToFile(const std::string& file_path) override;
		bool LoadFromFile(const std::string& file_path) override;

		void SetText(const std::string& text, const Math::Vector2& position);
		void SetSize(uint32_t size);

		inline const Math::Vector4& GetColor() const { return m_Color; }

		inline void SetColor(const Math::Vector4& color) { m_Color = color; }

		inline const Math::Vector4 GetColorOutline() const { return m_ColorOutline; }

		inline void SetColorOutline(const Math::Vector4& outline) { m_ColorOutline = outline; }

		inline void SetOutline(const Font_Outline_Type outline) { m_Outline = outline; }

		inline const Font_Outline_Type GetOutline() const { return m_Outline; }

		inline void SetOutlineSize(const uint32_t outline_size) { m_OutlineSize = outline_size; }

		inline const uint32_t GetOutlineSize() const { return m_OutlineSize; }

		inline const auto& GetAtlas() const { return m_Atlas; }

		inline void SetAtlas(const std::shared_ptr<RHI_Texture>& atlas) { m_Atlas = atlas; }

		inline const auto& GetAtlasOutline() const { return m_AtlasOutline; }

		inline void SetAtlasOutline(const std::shared_ptr<RHI_Texture>& atlas) { m_AtlasOutline = atlas; }

		inline RHI_IndexBuffer* GetIndexBuffer() const { return m_IndexBuffer.get(); }

		inline RHI_VertexBuffer* GetVertexBuffer() const { return m_VertexBuffer.get(); }

		inline uint32_t GetIndexCount() const { return static_cast<uint32_t>(m_vecIndices.size()); }

		inline uint32_t GetSize() const { return m_FontSize; }

		inline void SetGlyph(const uint32_t char_code, const Glyph& glyph) { m_mapGlyphs[char_code] = glyph; }

		inline Font_Hinting_Type GetHinting() const { return m_Hinting; }

		inline bool GetForceAutoHint() const { return m_Force_autohint; }

	private:
		bool UpdateBuffers(std::vector<RHI_Vertex_PosTex>& vertices, std::vector<uint32_t>& indices) const;

		uint32_t m_FontSize = 14;
		uint32_t m_OutlineSize = 2;
		bool m_Force_autohint = false;
		Font_Hinting_Type m_Hinting = Font_Hinting_Normal;
		Font_Outline_Type m_Outline = Font_Outline_Positive;
		Math::Vector4 m_Color = Math::Vector4::One;
		Math::Vector4 m_ColorOutline = Math::Vector4(0.0f, 0.0f, 0.0f, 1.0f);
		std::string m_CurrentText;
		uint32_t m_CharMaxWidth;
		uint32_t m_CharMaxHeight;
		std::shared_ptr<RHI_Texture> m_Atlas;
		std::shared_ptr<RHI_Texture> m_AtlasOutline;
		std::unordered_map<uint32_t, Glyph> m_mapGlyphs;
		std::shared_ptr<RHI_VertexBuffer> m_VertexBuffer;
		std::shared_ptr<RHI_IndexBuffer> m_IndexBuffer;
		std::vector<RHI_Vertex_PosTex> m_vecVertices;
		std::vector<uint32_t> m_vecIndices;
		std::shared_ptr<RHI_Device> m_RhiDevice;
	};
}