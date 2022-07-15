#include "Common.h"
#include "Mesh.h"
#include "../RHI/RHI_Vertex.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
    void Mesh::Clear()
    {
        m_vecVertices.clear();
        m_vecVertices.shrink_to_fit();
        m_vecIndices.clear();
        m_vecIndices.shrink_to_fit();
    }

    uint32_t Mesh::GetMemoryUsage() const
    {
        // 크기 계산
        uint32_t size = 0;
        size += uint32_t(m_vecVertices.size() * sizeof(RHI_Vertex_PosTexNorTan));
        size += uint32_t(m_vecIndices.size() * sizeof(uint32_t));

        return size;
    }

    void Mesh::GetGeometry(uint32_t indexOffset, uint32_t indexCount, uint32_t vertexOffset, unsigned vertexCount, vector<uint32_t>* indices, vector<RHI_Vertex_PosTexNorTan>* vertices)
    {
        if ((indexOffset == 0 && indexCount == 0) || (vertexOffset == 0 && vertexCount == 0) || !vertices || !indices)
        {
            LOG_ERROR("Mesh::Geometry_Get: Invalid parameters");
            return;
        }

        // 인덱스
        const auto indexFirst = m_vecIndices.begin() + indexOffset;
        const auto indexLast = m_vecIndices.begin() + indexOffset + indexCount;
        *indices = vector<uint32_t>(indexFirst, indexLast);

        // 버텍스
        const auto vertexFirst = m_vecVertices.begin() + vertexOffset;
        const auto vertexLast = m_vecVertices.begin() + vertexOffset + vertexCount;
        *vertices = vector<RHI_Vertex_PosTexNorTan>(vertexFirst, vertexLast);
    }

    // 버텍스 추가
    void Mesh::VerticesAppend(const vector<RHI_Vertex_PosTexNorTan>& vertices, uint32_t* vertexOffset)
    {
        if (vertexOffset)
        {
            *vertexOffset = static_cast<uint32_t>(m_vecVertices.size());
        }

        m_vecVertices.insert(m_vecVertices.end(), vertices.begin(), vertices.end());
    }

    uint32_t Mesh::VerticesCount() const
    {
        return static_cast<uint32_t>(m_vecVertices.size());
    }

    void Mesh::VertexAdd(const RHI_Vertex_PosTexNorTan& vertex)
    {
        m_vecVertices.emplace_back(vertex);
    }

    void Mesh::IndicesAppend(const vector<uint32_t>& indices, uint32_t* indexOffset)
    {
        if (indexOffset)
        {
            *indexOffset = static_cast<uint32_t>(m_vecIndices.size());
        }

        m_vecIndices.insert(m_vecIndices.end(), indices.begin(), indices.end());
    }
}