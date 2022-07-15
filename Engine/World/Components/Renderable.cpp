#include "Common.h"
#include "Renderable.h"
#include "Transform.h"
#include "../../IO/FileStream.h"
#include "../../Resource/ResourceCache.h"
#include "../../Utils/Geometry.h"
#include "../../RHI/RHI_Texture2D.h"
#include "../../Rendering/Model.h"
#include "../../RHI/RHI_Vertex.h"
#include "../../Rendering/Mesh.h"
#include "../../RHI/RHI_VertexBuffer.h"
#include "../../RHI/RHI_IndexBuffer.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
	inline void build(const Geometry_Type type, Renderable* renderable)
	{
		Model* model = new Model(renderable->GetContext());
		vector<RHI_Vertex_PosTexNorTan> vertices;
		vector<uint32_t> indices;

		const string project_directory = renderable->GetContext()->GetSubModule<ResourceCache>()->GetProjectDirectory();
        const string model_directory = renderable->GetContext()->GetSubModule<ResourceCache>()->GetResourceDirectory();

        // 지오메트리 생성
        if (type == Geometry_Default_Cube)
        {
            Utility::Geometry::CreateCube(&vertices, &indices);
            model->SetResourceFilePath(project_directory + "default_cube" + EXTENSION_MODEL);
        }
        else if (type == Geometry_Default_Quad)
        {
            Utility::Geometry::CreateQuad(&vertices, &indices);
            model->SetResourceFilePath(project_directory + "default_quad" + EXTENSION_MODEL);
        }
        else if (type == Geometry_Default_Sphere)
        {
            Utility::Geometry::CreateSphere(&vertices, &indices);
            model->SetResourceFilePath(project_directory + "default_sphere" + EXTENSION_MODEL);
        }
        else if (type == Geometry_Default_Cylinder)
        {
            Utility::Geometry::CreateCylinder(&vertices, &indices);
            model->SetResourceFilePath(project_directory + "default_cylinder" + EXTENSION_MODEL);
        }
        else if (type == Geometry_Default_Cone)
        {
            Utility::Geometry::CreateCone(&vertices, &indices);
            model->SetResourceFilePath(project_directory + "default_cone" + EXTENSION_MODEL);
        }

        // 만약 버텍스와 인덱스가 비어있을 경우
        if (vertices.empty() || indices.empty())
            return;

        // 추가
        model->AppendGeometry(indices, vertices, nullptr, nullptr);
        model->UpdateGeometry();

        renderable->GeometrySet(
            "Default_Geometry",
            0,
            static_cast<uint32_t>(indices.size()),
            0,
            static_cast<uint32_t>(vertices.size()),
            BoundingBox(vertices.data(), static_cast<uint32_t>(vertices.size())),
            model
        );
	}

    Renderable::Renderable(Context* context, Entity* entity, uint64_t id /*= 0*/) : IComponent(context, entity, id)
    {
        m_geometry_type = Geometry_Custom;
        m_geometryIndexOffset = 0;
        m_geometryIndexCount = 0;
        m_geometryVertexOffset = 0;
        m_geometryVertexCount = 0;
        m_material_default = false;
        m_cast_shadows = true;

        REGISTER_ATTRIBUTE_VALUE_VALUE(m_material_default, bool);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_material, Material*);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_cast_shadows, bool);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_geometryIndexOffset, uint32_t);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_geometryIndexCount, uint32_t);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_geometryVertexOffset, uint32_t);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_geometryVertexCount, uint32_t);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_geometryName, string);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_model, Model*);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_bounding_box, BoundingBox);
        REGISTER_ATTRIBUTE_GET_SET(Geometry_Type, GeometrySet, Geometry_Type);
    }

    void Renderable::Serialize(FileStream* stream)
    {
        stream->Write(static_cast<uint32_t>(m_geometry_type));
        stream->Write(m_geometryIndexOffset);
        stream->Write(m_geometryIndexCount);
        stream->Write(m_geometryVertexOffset);
        stream->Write(m_geometryVertexCount);
        stream->Write(m_bounding_box);
        stream->Write(m_model ? m_model->GetResourceName() : "");

        stream->Write(m_cast_shadows);
        stream->Write(m_material_default);
        if (!m_material_default)
        {
            stream->Write(m_material ? m_material->GetResourceName() : "");
        }
    }

    void Renderable::Deserialize(FileStream* stream)
    {
        m_geometry_type = static_cast<Geometry_Type>(stream->ReadAs<uint32_t>());
        m_geometryIndexOffset = stream->ReadAs<uint32_t>();
        m_geometryIndexCount = stream->ReadAs<uint32_t>();
        m_geometryVertexOffset = stream->ReadAs<uint32_t>();
        m_geometryVertexCount = stream->ReadAs<uint32_t>();
        stream->Read(&m_bounding_box);
        string model_name;
        stream->Read(&model_name);
        m_model = m_Context->GetSubModule<ResourceCache>()->GetByName<Model>(model_name).get();

        if (m_geometry_type != Geometry_Custom)
        {
            GeometrySet(m_geometry_type);
        }

        stream->Read(&m_cast_shadows);
        stream->Read(&m_material_default);
        if (m_material_default)
        {
            UseDefaultMaterial();
        }
        else
        {
            string material_name;
            stream->Read(&material_name);
            m_material = m_Context->GetSubModule<ResourceCache>()->GetByName<Material>(material_name).get();
        }
    }

    void Renderable::GeometrySet(const string& name, const uint32_t index_offset, const uint32_t index_count, const uint32_t vertex_offset, const uint32_t vertex_count, const BoundingBox& bounding_box, Model* model)
    {
        if (m_geometryName == "Default_Geometry")
        {
            SAFE_DELETE(m_model);
        }

        m_geometryName = name;
        m_geometryIndexOffset = index_offset;
        m_geometryIndexCount = index_count;
        m_geometryVertexOffset = vertex_offset;
        m_geometryVertexCount = vertex_count;
        m_bounding_box = bounding_box;
        m_model = model;
    }

    void Renderable::GeometrySet(const Geometry_Type type)
    {
        m_geometry_type = type;

        if (type != Geometry_Custom)
        {
            build(type, this);
        }
    }

    void Renderable::GeometryClear()
    {
        GeometrySet("Cleared", 0, 0, 0, 0, BoundingBox(), nullptr);
    }

    void Renderable::GeometryGet(vector<uint32_t>* indices, vector<RHI_Vertex_PosTexNorTan>* vertices) const
    {
        if (!m_model)
        {
            LOG_ERROR("Invalid model");
            return;
        }

        m_model->GetGeometry(m_geometryIndexOffset, m_geometryIndexCount, m_geometryVertexOffset, m_geometryVertexCount, indices, vertices);
    }

    const BoundingBox& Renderable::GetAabb()
    {
        if (m_last_transform != GetTransform()->GetMatrix() || !m_aabb.Defined())
        {
            m_aabb = m_bounding_box.Transform(GetTransform()->GetMatrix());
            m_last_transform = GetTransform()->GetMatrix();
        }

        return m_aabb;
    }

    shared_ptr<Material> Renderable::SetMaterial(const shared_ptr<Material>& material)
    {
        ASSERT(material != nullptr);

        shared_ptr<Material> _material = m_Context->GetSubModule<ResourceCache>()->Cache(material);

        m_material = _material.get();

        m_material_default = false;

        return _material;
    }

    shared_ptr<Material> Renderable::SetMaterial(const string& file_path)
    {
        auto material = make_shared<Material>(GetContext());
        if (!material->LoadFromFile(file_path))
        {
            LOG_WARNING("Failed to load material from \"%s\"", file_path.c_str());
            return nullptr;
        }

        return SetMaterial(material);
    }

    void Renderable::UseDefaultMaterial()
    {
        m_material_default = true;
        ResourceCache* resource_cache = GetContext()->GetSubModule<ResourceCache>();
        const auto data_dir = resource_cache->GetResourceDirectory() + "/";
        FileSystem::CreateDirectory_(data_dir);

        auto material = make_shared<Material>(GetContext());
        material->SetResourceFilePath(resource_cache->GetProjectDirectory() + "standard" + EXTENSION_MATERIAL); // Set resource file path so it can be used by the resource cache
        material->SetEditable(false);

        const shared_ptr<RHI_Texture2D> texture = resource_cache->Load<RHI_Texture2D>(resource_cache->GetResourceDirectory(EResourceDirectory::Textures) + "/no_texture.png");
        material->SetTextureSlot(Material_Color, texture);

        SetMaterial(material);
        m_material_default = true;
    }

    string Renderable::GetMaterialName() const
    {
        return m_material ? m_material->GetResourceName() : "";
    }
}