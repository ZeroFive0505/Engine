#pragma once

#include <vector>
#include "../RHI/RHI_Definition.h"

namespace PlayGround
{
    // 3D ¸ðµ¨ ¸Þ½¬
	class Mesh
	{
    public:
        Mesh() = default;
        ~Mesh() { Clear(); }

        void Clear();
        void GetGeometry(
            uint32_t indexOffset,
            uint32_t indexCount,
            uint32_t vertexOffset,
            unsigned vertexCount,
            std::vector<uint32_t>* indices,
            std::vector<RHI_Vertex_PosTexNorTan>* vertices
        );
        uint32_t GetMemoryUsage() const;

        // ¹öÅØ½º ¹öÆÛ
        void VertexAdd(const RHI_Vertex_PosTexNorTan& vertex);
        void VerticesAppend(const std::vector<RHI_Vertex_PosTexNorTan>& vertices, uint32_t* vertexOffset);
        uint32_t VerticesCount() const;
        inline std::vector<RHI_Vertex_PosTexNorTan>& GetVertices() { return m_vecVertices; }
        inline void SetVertices(const std::vector<RHI_Vertex_PosTexNorTan>& vertices) { m_vecVertices = vertices; }

        // ÀÎµ¦½º ¹öÆÛ
        inline void IndexAdd(uint32_t index) { m_vecIndices.emplace_back(index); }
        inline std::vector<uint32_t>& GetIndices() { return m_vecIndices; }
        inline void SetIndices(const std::vector<uint32_t>& indices) { m_vecIndices = indices; }
        inline uint32_t IndicesCount() const { return static_cast<uint32_t>(m_vecIndices.size()); }
        void IndicesAppend(const std::vector<uint32_t>& indices, uint32_t* indexOffset);

        // »ï°¢Çü °¹¼ö
        inline uint32_t GetTriangleCount() const { return IndicesCount() / 3; }

    private:
        std::vector<RHI_Vertex_PosTexNorTan> m_vecVertices;
        std::vector<uint32_t> m_vecIndices;
	};
}