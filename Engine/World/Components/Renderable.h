#pragma once

#include "IComponent.h"
#include <vector>
#include "../../Math/BoundingBox.h"
#include "../../Math/Matrix.h"

namespace PlayGround
{
	class Model;
	class Mesh;
	class Light;
	class Material;

	namespace Math
	{
		class Vector3;
	}

	enum Geometry_Type
	{
		Geometry_Custom,
		Geometry_Default_Cube,
		Geometry_Default_Quad,
		Geometry_Default_Sphere,
		Geometry_Default_Cylinder,
		Geometry_Default_Cone
	};

	// 런데링을 담당하는 렌더러블 컴포넌트
	class Renderable : public IComponent
	{
    public:
        Renderable(Context* context, Entity* entity, uint64_t id = 0);
        ~Renderable() = default;

        void Serialize(FileStream* stream) override;
        void Deserialize(FileStream* stream) override;

        void GeometrySet(
            const std::string& name,
            uint32_t index_offset,
            uint32_t index_count,
            uint32_t vertex_offset,
            uint32_t vertex_count,
            const Math::BoundingBox& aabb,
            Model* model
        );
        void GeometryClear();
        void GeometrySet(Geometry_Type type);
        void GeometryGet(std::vector<uint32_t>* indices, std::vector<RHI_Vertex_PosTexNorTan>* vertices) const;
        inline uint32_t GeometryIndexOffset()              const { return m_geometryIndexOffset; }
        inline uint32_t GeometryIndexCount()               const { return m_geometryIndexCount; }
        inline uint32_t GeometryVertexOffset()             const { return m_geometryVertexOffset; }
        inline uint32_t GeometryVertexCount()              const { return m_geometryVertexCount; }
        inline Geometry_Type GeometryType()                const { return m_geometry_type; }
        inline const std::string& GeometryName()           const { return m_geometryName; }
        inline Model* GeometryModel()                      const { return m_model; }
        inline const Math::BoundingBox& GetBoundingBox()   const { return m_bounding_box; }
        const Math::BoundingBox& GetAabb();

        std::shared_ptr<Material> SetMaterial(const std::shared_ptr<Material>& material);

        std::shared_ptr<Material> SetMaterial(const std::string& file_path);

        void UseDefaultMaterial();
        std::string GetMaterialName()   const;
        inline Material* GetMaterial()         const { return m_material; }
        inline auto HasMaterial()              const { return m_material != nullptr; }

        inline void SetCastShadows(const bool cast_shadows) { m_cast_shadows = cast_shadows; }
        inline bool GetCastShadows() const { return m_cast_shadows; }

    private:
        std::string m_geometryName;
        uint32_t m_geometryIndexOffset;
        uint32_t m_geometryIndexCount;
        uint32_t m_geometryVertexOffset;
        uint32_t m_geometryVertexCount;
        Geometry_Type m_geometry_type;
        Math::BoundingBox m_bounding_box;
        Math::BoundingBox m_aabb;
        Math::Matrix m_last_transform = Math::Matrix::Identity;
        bool m_cast_shadows = true;
        bool m_material_default;
        Model* m_model = nullptr;
        Material* m_material = nullptr;
	};
}