#pragma once

#include <memory>
#include <vector>
#include "Material.h"
#include "../RHI/RHI_Definition.h"
#include "../Resource/IResource.h"
#include "../Math/BoundingBox.h"


namespace PlayGround
{
	class ResourceCache;
	class Entity;
	class Mesh;

    // 모델 클래스
	class Model : public IResource, public std::enable_shared_from_this<Model>
	{
    public:
        Model(Context* context);
        ~Model();

        void Clear();
        
        // IResource 가상 함수
        bool LoadFromFile(const std::string& file_path) override;
        bool SaveToFile(const std::string& file_path) override;

        void AppendGeometry(
            const std::vector<uint32_t>& indices,
            const std::vector<RHI_Vertex_PosTexNorTan>& vertices,
            uint32_t* index_offset = nullptr,
            uint32_t* vertex_offset = nullptr
        ) const;
        void GetGeometry(
            uint32_t index_offset,
            uint32_t index_count,
            uint32_t vertex_offset,
            uint32_t vertex_count,
            std::vector<uint32_t>* indices,
            std::vector<RHI_Vertex_PosTexNorTan>* vertices
        ) const;
        void UpdateGeometry();
        inline const auto& GetAABB() const { return m_AABB; }
        inline const auto& GetMesh() const { return m_Mesh; }

        inline void SetRootEntity(const std::shared_ptr<Entity>& entity) { m_RootEntity = entity; }
        void AddMaterial(std::shared_ptr<Material>& material, const std::shared_ptr<Entity>& entity) const;
        void AddTexture(std::shared_ptr<Material>& material, Material_Property texture_type, const std::string& file_path);

        inline bool IsAnimated()                         const { return m_IsAnimated; }
        inline void SetAnimated(const bool is_animated) { m_IsAnimated = is_animated; }
        inline const RHI_IndexBuffer* GetIndexBuffer()   const { return m_IndexBuffer.get(); }
        inline const RHI_VertexBuffer* GetVertexBuffer() const { return m_VertexBuffer.get(); }
        inline auto GetSharedPtr() { return shared_from_this(); }

    private:
        bool GeometryCreateBuffers();
        float GeometryComputeNormalizedScale() const;

        std::weak_ptr<Entity> m_RootEntity;
        std::shared_ptr<RHI_VertexBuffer> m_VertexBuffer;
        std::shared_ptr<RHI_IndexBuffer> m_IndexBuffer;
        std::shared_ptr<Mesh> m_Mesh;
        Math::BoundingBox m_AABB;
        float m_NormalizedScale = 1.0f;
        bool m_IsAnimated = false;

        ResourceCache* m_ResourceManager;
        std::shared_ptr<RHI_Device> m_RhiDevice;
	};
}

