#include "Common.h"
#include "Rectangle.h"
#include "../Rendering/Renderer.h"
#include "../RHI/RHI_VertexBuffer.h"
#include "../RHI/RHI_IndexBuffer.h"
#include "../RHI/RHI_Vertex.h"

using namespace std;

namespace PlayGround::Math
{
    const Rectangle Rectangle::Zero(0.0f, 0.0f, 0.0f, 0.0f);

    bool Rectangle::CreateBuffers(Renderer* renderer)
    {
        ASSERT(renderer != nullptr);

        // ��ũ�� ��ǥ ���
        const RHI_Viewport& viewport = renderer->GetViewport();
        const float sc_left = -(viewport.width * 0.5f) + left;
        const float sc_right = sc_left + Width();
        const float sc_top = (viewport.height * 0.5f) - top;
        const float sc_bottom = sc_top - Height();

        // ����
        const RHI_Vertex_PosTex vertices[6] =
        {
            // ù��° �ﰢ��
            RHI_Vertex_PosTex(Vector3(sc_left,  sc_top,    0.0f), Vector2(0.0f, 0.0f)), // ���� ��
            RHI_Vertex_PosTex(Vector3(sc_right, sc_bottom, 0.0f), Vector2(1.0f, 1.0f)), // ������ �Ʒ�
            RHI_Vertex_PosTex(Vector3(sc_left,  sc_bottom, 0.0f), Vector2(0.0f, 1.0f)), // ���� �Ʒ�
            // �ι�° �ﰢ��
            RHI_Vertex_PosTex(Vector3(sc_left,  sc_top,    0.0f), Vector2(0.0f, 0.0f)), // ���� ��
            RHI_Vertex_PosTex(Vector3(sc_right, sc_top,    0.0f), Vector2(1.0f, 0.0f)), // ������ ��
            RHI_Vertex_PosTex(Vector3(sc_right, sc_bottom, 0.0f), Vector2(1.0f, 1.0f))  // ������ �Ʒ�
        };

        m_vertex_buffer = make_shared<RHI_VertexBuffer>(renderer->GetRhiDevice(), false, "rectangle");
        m_vertex_buffer->SetObjectName("vertex_buffer_rectangle");
        if (!m_vertex_buffer->Create(vertices, 6))
        {
            LOG_ERROR("Failed to create vertex buffer.");
            return false;
        }

        // �ε���
        const uint32_t indices[6] = { 0, 1, 2, 3, 4, 5 };

        m_index_buffer = make_shared<RHI_IndexBuffer>(renderer->GetRhiDevice(), false, "rectangle");
        m_index_buffer->SetObjectName("index_buffer_rectangle");
        if (!m_index_buffer->Create(indices, 6))
        {
            LOG_ERROR("Failed to create index buffer.");
            return false;
        }

        return true;
    }
}