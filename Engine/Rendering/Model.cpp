#include "Common.h"
#include "Model.h"
#include "Mesh.h"
#include "Renderer.h"
#include "../IO/FileStream.h"
#include "../Core/Stopwatch.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/Importer/ModelImporter.h"
#include "../World/Entity.h"
#include "../World/Components/Transform.h"
#include "../World/Components/Renderable.h"
#include "../RHI/RHI_VertexBuffer.h"
#include "../RHI/RHI_IndexBuffer.h"
#include "../RHI/RHI_Texture2D.h"
#include "../RHI/RHI_Vertex.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
    // 모델 초기화
    Model::Model(Context* context) : IResource(context, EResourceType::Model)
    {
        m_ResourceManager = m_Context->GetSubModule<ResourceCache>();
        m_RhiDevice = m_Context->GetSubModule<Renderer>()->GetRhiDevice();
        m_Mesh = make_unique<Mesh>();
    }

    Model::~Model()
    {
        Clear();
    }

    void Model::Clear()
    {
        m_RootEntity.reset();
        m_VertexBuffer.reset();
        m_IndexBuffer.reset();
        m_Mesh->Clear();
        m_AABB.Undefine();
        m_NormalizedScale = 1.0f;
        m_IsAnimated = false;
    }

    bool Model::LoadFromFile(const string& file_path)
    {
        const StopWatch timer;

        if (file_path.empty() || FileSystem::IsDirectory(file_path))
        {
            LOG_WARNING("Invalid file path");
            return false;
        }

        // 만약 엔진 자체 포맷이면
        if (FileSystem::GetExtensionFromFilePath(file_path) == EXTENSION_MODEL)
        {
            // 파일 스트림으로 불러온다.
            auto file = make_unique<FileStream>(file_path, FileStream_Read);
            if (!file->IsOpen())
                return false;

            SetResourceFilePath(file->ReadAs<string>());
            file->Read(&m_NormalizedScale);
            file->Read(&m_Mesh->GetIndices());
            file->Read(&m_Mesh->GetVertices());

            UpdateGeometry();
        }
        // 외부 포맷일 경우
        else
        {
            SetResourceFilePath(file_path);

            // 모델 임포터로 불러온다.
            if (m_ResourceManager->GetModelImporter()->Load(this, file_path))
            {
                // 스케일은 1로 설정
                m_NormalizedScale = GeometryComputeNormalizedScale();
                m_RootEntity.lock()->GetComponent<Transform>()->SetScale(m_NormalizedScale);
            }
            else
            {
                return false;
            }
        }

        // 메모리 크기 계산
        {
            // CPU
            m_ObjectSizeCPU = !m_Mesh ? 0 : m_Mesh->GetMemoryUsage();
            
            // GPU에서 크기는 버텍스, 인덱스 버퍼의 크기
            if (m_VertexBuffer && m_IndexBuffer)
            {
                m_ObjectSizeGPU = m_VertexBuffer->GetObjectSizeGPU();
                m_ObjectSizeGPU += m_IndexBuffer->GetObjectSizeGPU();
            }
        }

        LOG_INFO("Loading \"%s\" took %d ms", FileSystem::GetFileNameFromFilePath(file_path).c_str(), static_cast<int>(timer.GetElapsedTimeMS()));

        return true;
    }

    // 파일 스트림으로 저장
    bool Model::SaveToFile(const string& file_path)
    {
        auto file = make_unique<FileStream>(file_path, FileStream_Write);
        if (!file->IsOpen())
            return false;

        file->Write(GetResourceFilePath());
        file->Write(m_NormalizedScale);
        file->Write(m_Mesh->GetIndices());
        file->Write(m_Mesh->GetVertices());

        file->Close();

        return true;
    }

    void Model::AppendGeometry(const vector<uint32_t>& indices, const vector<RHI_Vertex_PosTexNorTan>& vertices, uint32_t* index_offset, uint32_t* vertex_offset) const
    {
        ASSERT(!indices.empty());
        ASSERT(!vertices.empty());

        // 메쉬 추가
        m_Mesh->IndicesAppend(indices, index_offset);
        m_Mesh->VerticesAppend(vertices, vertex_offset);
    }

    void Model::GetGeometry(const uint32_t index_offset, const uint32_t index_count, const uint32_t vertex_offset, const uint32_t vertex_count, vector<uint32_t>* indices, vector<RHI_Vertex_PosTexNorTan>* vertices) const
    {
        m_Mesh->GetGeometry(index_offset, index_count, vertex_offset, vertex_count, indices, vertices);
    }

    void Model::UpdateGeometry()
    {
        ASSERT(m_Mesh->IndicesCount() != 0);
        ASSERT(m_Mesh->VerticesCount() != 0);

        GeometryCreateBuffers();
        m_NormalizedScale = GeometryComputeNormalizedScale();
        m_AABB = BoundingBox(m_Mesh->GetVertices().data(), static_cast<uint32_t>(m_Mesh->GetVertices().size()));
    }

    void Model::AddMaterial(shared_ptr<Material>& material, const shared_ptr<Entity>& entity) const
    {
        ASSERT(material != nullptr);
        ASSERT(entity != nullptr);

        // 파일의 경로를 찾아온다.
        const string asset_path = FileSystem::GetDirectoryFromFilePath(GetResourceFilePathNative()) + material->GetResourceName() + EXTENSION_MATERIAL;
        // 마테리얼의 경로 설정
        material->SetResourceFilePath(asset_path);

        // 마테리얼 설정
        entity->AddComponent<Renderable>()->SetMaterial(material);
    }

    void Model::AddTexture(shared_ptr<Material>& material, const Material_Property texture_type, const string& file_path)
    {
        ASSERT(material != nullptr);
        ASSERT(!file_path.empty());

        // 경로에서 파일 이름만 가져온다.
        const string tex_name = FileSystem::GetFileNameWithoutExtensionFromFilePath(file_path);
        shared_ptr<RHI_Texture> texture = m_Context->GetSubModule<ResourceCache>()->GetByName<RHI_Texture2D>(tex_name);

        // 만약 텍스쳐가 존재할시에 텍스쳐 설정
        if (texture)
        {
            material->SetTextureSlot(texture_type, texture);
        }
        else // 텍스쳐가 존재하지 않으면 불러오고 저장한다.
        {
            texture = make_shared<RHI_Texture2D>(m_Context, RHI_Texture_Srv | RHI_Texture_Mips | RHI_Texture_PerMipViews | RHI_Texture_Compressed);
            texture->LoadFromFile(file_path);

            material->SetTextureSlot(texture_type, texture);
        }
    }

    bool Model::GeometryCreateBuffers()
    {
        auto success = true;

        const auto indices = m_Mesh->GetIndices();
        const auto vertices = m_Mesh->GetVertices();

        // 인덱스 버퍼 생성
        if (!indices.empty())
        {
            m_IndexBuffer = make_shared<RHI_IndexBuffer>(m_RhiDevice, false, "model");
            if (!m_IndexBuffer->Create(indices))
            {
                LOG_ERROR("Failed to create index buffer for \"%s\".", GetResourceName().c_str());
                success = false;
            }
        }
        else
        {
            LOG_ERROR("Failed to create index buffer for \"%s\". Provided indices are empty", GetResourceName().c_str());
            success = false;
        }

        // 버텍스 버퍼 생성
        if (!vertices.empty())
        {
            m_VertexBuffer = make_shared<RHI_VertexBuffer>(m_RhiDevice, false, "model");
            if (!m_VertexBuffer->Create(vertices))
            {
                LOG_ERROR("Failed to create vertex buffer for \"%s\".", GetResourceName().c_str());
                success = false;
            }
        }
        else
        {
            LOG_ERROR("Failed to create vertex buffer for \"%s\". Provided vertices are empty", GetResourceName().c_str());
            success = false;
        }

        return success;
    }

    float Model::GeometryComputeNormalizedScale() const
    {
        // 바운딩 박스의 크기를 구한다.
        const auto scale_offset = m_AABB.GetExtents().Length();

        // 정규화
        return 1.0f / scale_offset;
    }
}