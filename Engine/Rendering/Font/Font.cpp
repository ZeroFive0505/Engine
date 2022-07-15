#include "Common.h"
#include "Font.h"
#include "../Renderer.h"
#include "../../RHI/RHI_Vertex.h"
#include "../../RHI/RHI_VertexBuffer.h"
#include "../../RHI/RHI_IndexBuffer.h"
#include "../../Resource/ResourceCache.h"
#include "../../Resource/Importer/FontImporter.h"
#include "../../Core/Stopwatch.h"

using namespace std;
using namespace PlayGround::Math;

#define ASCII_TAB 9
#define ASCII_NEW_LINE 10
#define ASCII_SPACE 32

namespace PlayGround
{
	// �ʱ�ȭ
	Font::Font(Context* context, const string& file_path, const int font_size, const Vector4& color) :
		IResource(context, EResourceType::Font)
	{
		// ����̽� ���
		m_RhiDevice = m_Context->GetSubModule<Renderer>()->GetRhiDevice();
		// ���� ����
		m_VertexBuffer = make_shared<RHI_VertexBuffer>(m_RhiDevice, true, "font");
		m_IndexBuffer = make_shared<RHI_IndexBuffer>(m_RhiDevice, true, "font");
		m_CharMaxWidth = 0;
		m_CharMaxHeight = 0;
		m_Color = color;

		SetSize(font_size);
		LoadFromFile(file_path);
	}

	// ��Ʈ�� ���� �����ϰ� ������ ���� ����. ���� �׳� �� ��ȯ
	bool Font::SaveToFile(const string& file_path)
	{
		return true;
	}

	// �ε�
	bool Font::LoadFromFile(const string& file_path)
	{
		if (!m_Context)
			return false;

		// �ε� �ð� ����
		const StopWatch timer;

		if (!m_Context->GetSubModule<ResourceCache>()->GetFontImporter()->LoadFromFile(this, file_path))
		{
			LOG_ERROR("Failed to load font \"%s\"", file_path.c_str());
			return false;
		}

		for (const auto& char_info : m_mapGlyphs)
		{
			m_CharMaxWidth = Util::Max<int>(char_info.second.width, m_CharMaxWidth);
			m_CharMaxHeight = Util::Max<int>(char_info.second.height, m_CharMaxHeight);
		}

		LOG_INFO("Loading \"%s\" took %d ms", FileSystem::GetFileNameFromFilePath(file_path).c_str(), static_cast<int>(timer.GetElapsedTimeMS()));
		return true;
	}

	void Font::SetText(const string& text, const Vector2& position)
	{
		const bool same_text = text == m_CurrentText;
		const bool has_buffers = (m_VertexBuffer && m_IndexBuffer);

		// ���� �Ȱ��� �ؽ�Ʈ�� ���۰� ���ٸ� ��ȯ
		if (same_text || !has_buffers)
			return;

		Vector2 pen = position;
		m_CurrentText = text;
		m_vecVertices.clear();

		// ���� �ϳ��ϳ� �������Ѵ�.
		for (char c : m_CurrentText)
		{
			Glyph& glyph = m_mapGlyphs[c];

			if (c == ASCII_TAB)
			{
				const uint32_t space_offset = m_mapGlyphs[ASCII_SPACE].horizontal_advance;
				const uint32_t space_count = 8;
				const uint32_t tab_spacing = space_offset * space_count;
				const uint32_t offset_from_start = static_cast<uint32_t>(Math::Util::Abs(pen.x - position.x));
				const uint32_t next_column_index = (offset_from_start / tab_spacing) + 1;
				const uint32_t offset_to_column = (next_column_index * tab_spacing) - offset_from_start;
				pen.x += offset_to_column;
			}
			else if (c == ASCII_NEW_LINE)
			{
				pen.y -= m_CharMaxHeight;
				pen.x = position.x;
			}
			else if (c == ASCII_SPACE)
				pen.x += glyph.horizontal_advance;
			else
			{
				// �簢���� ù��° �ﰢ��   
				m_vecVertices.emplace_back(pen.x + glyph.offset_x, pen.y + glyph.offset_y, 0.0f, glyph.uv_x_left, glyph.uv_y_top);       // top left
				m_vecVertices.emplace_back(pen.x + glyph.offset_x + glyph.width, pen.y + glyph.offset_y - glyph.height, 0.0f, glyph.uv_x_right, glyph.uv_y_bottom);    // bottom right
				m_vecVertices.emplace_back(pen.x + glyph.offset_x, pen.y + glyph.offset_y - glyph.height, 0.0f, glyph.uv_x_left, glyph.uv_y_bottom);    // bottom left
				// �ι�° �ﰢ��
				m_vecVertices.emplace_back(pen.x + glyph.offset_x, pen.y + glyph.offset_y, 0.0f, glyph.uv_x_left, glyph.uv_y_top);       // top left
				m_vecVertices.emplace_back(pen.x + glyph.offset_x + glyph.width, pen.y + glyph.offset_y, 0.0f, glyph.uv_x_right, glyph.uv_y_top);       // top right
				m_vecVertices.emplace_back(pen.x + glyph.offset_x + glyph.width, pen.y + glyph.offset_y - glyph.height, 0.0f, glyph.uv_x_right, glyph.uv_y_bottom);    // bottom right

				// ����
				pen.x += glyph.horizontal_advance;
			}
		}

		m_vecVertices.shrink_to_fit();

		m_vecIndices.clear();

		uint32_t size = static_cast<uint32_t>(m_vecVertices.size());

		for (uint32_t i = 0; i < size; i++)
		{
			m_vecIndices.emplace_back(i);
		}

		UpdateBuffers(m_vecVertices, m_vecIndices);
	}

	void Font::SetSize(const uint32_t size)
	{
		m_FontSize = Math::Util::Clamp<uint32_t>(8, 50, size);
	}

	bool Font::UpdateBuffers(vector<RHI_Vertex_PosTex>& vertices, vector<uint32_t>& indices) const
	{
		ASSERT(m_Context != nullptr);
		ASSERT(m_VertexBuffer != nullptr);
		ASSERT(m_IndexBuffer != nullptr);

		// ���� ������ ũ�Ⱑ ���ڸ��ٸ� �ø���.
		if (vertices.size() > m_VertexBuffer->GetVertexCount())
		{
			if (!m_VertexBuffer->CreateDynamic<RHI_Vertex_PosTex>(static_cast<uint32_t>(vertices.size())))
			{
				LOG_ERROR("Failed to update vertex buffer.");
				return false;
			}

			if (!m_IndexBuffer->CreateDynamic<uint32_t>(static_cast<uint32_t>(indices.size())))
			{
				LOG_ERROR("Failed to update index buffer.");
				return false;
			}
		}

		// ���� ����

		const auto vertex_buffer = static_cast<RHI_Vertex_PosTex*>(m_VertexBuffer->Map());
		
		if (vertex_buffer)
		{
			copy(vertices.begin(), vertices.end(), vertex_buffer);
			m_VertexBuffer->Unmap();
		}

		const auto index_buffer = static_cast<uint32_t*>(m_IndexBuffer->Map());

		if (index_buffer)
		{
			copy(indices.begin(), indices.end(), index_buffer);
			m_IndexBuffer->Unmap();
		}

		return true;
	}
}