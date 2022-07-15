#include "Common.h"
#include "Terrain.h"
#include "Renderable.h"
#include "..\Entity.h"
#include "..\..\RHI\RHI_Texture2D.h"
#include "..\..\RHI\RHI_Vertex.h"
#include "..\..\Rendering\Model.h"
#include "..\..\IO\FileStream.h"
#include "..\..\Resource\ResourceCache.h"
#include "..\..\Rendering\Mesh.h"
#include "..\..\Threading\Threading.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
    Terrain::Terrain(Context* context, Entity* entity, uint64_t id /*= 0*/) : IComponent(context, entity, id)
    {

    }

    void Terrain::OnInit()
    {

    }

    void Terrain::Serialize(FileStream* stream)
    {
        const string no_path;

        // ����
        stream->Write(m_HeightMap ? m_HeightMap->GetResourceFilePathNative() : no_path);
        stream->Write(m_Model ? m_Model->GetResourceName() : no_path);
        stream->Write(m_MinY);
        stream->Write(m_MaxY);
    }

    void Terrain::Deserialize(FileStream* stream)
    {
        // �ҷ�����
        ResourceCache* resource_cache = m_Context->GetSubModule<ResourceCache>();
        m_HeightMap = resource_cache->GetByPath<RHI_Texture2D>(stream->ReadAs<string>());
        m_Model = resource_cache->GetByName<Model>(stream->ReadAs<string>());
        stream->Read(&m_MinY);
        stream->Read(&m_MaxY);

        UpdateFromModel(m_Model);
    }

    void Terrain::SetHeightMap(const shared_ptr<RHI_Texture2D>& height_map)
    {
        // ���� �� ����
        m_HeightMap = m_Context->GetSubModule<ResourceCache>()->Cache<RHI_Texture2D>(height_map);
    }

    void Terrain::GenerateAsync()
    {
        // �񵿱� ���� ����

        // �̹� �����ǰ��ִ� �����̶�� ��ȯ
        if (m_IsGenerating)
        {
            LOG_WARNING("Terrain is already being generated, please wait...");
            return;
        }

        // ���� ���� �������� ���� ���
        if (!m_HeightMap)
        {
            LOG_WARNING("You need to assign a height map before trying to generate a terrain.");

            // ���� �����Ѵ�.
            m_Context->GetSubModule<ResourceCache>()->Remove(m_Model);
            m_Model.reset();

            Renderable* renderable = m_Entity->AddComponent<Renderable>();

            if (renderable)
                renderable->GeometryClear();

            return;
        }

        // ������ �߰�
        m_Context->GetSubModule<Threading>()->AddTask([this]()
        {
            // ���� ����
            m_IsGenerating = true;

            // ���� ������ �����´�.
            vector<std::byte> height_data;
            {
                height_data = m_HeightMap->GetMip(0, 0).bytes;

                // �����Ͱ� �������� ���� ��� 
                if (height_data.empty())
                {
                    // ���Ͽ��� �ε��Ѵ�.
                    if (m_HeightMap->LoadFromFile(m_HeightMap->GetResourceFilePathNative()))
                    {
                        height_data = m_HeightMap->GetMip(0, 0).bytes;

                        if (height_data.empty())
                        {
                            LOG_ERROR("Failed to load height map");
                            return;
                        }
                    }
                }
            }

            // ���� ������ �����´�.
            m_Height = m_HeightMap->GetHeight();
            m_Width = m_HeightMap->GetWidth();
            m_VertexCount = m_Height * m_Width;
            m_FaceCount = (m_Height - 1) * (m_Width - 1) * 2;
            m_Progress_jobs_done = 0;
            m_Progress_job_count = m_VertexCount * 2 + m_FaceCount + m_VertexCount * m_FaceCount;

            // �̸� �޸𸮸� �Ҵ��Ѵ�.
            vector<Vector3> positions = vector<Vector3>(m_Height * m_Width);
            vector<RHI_Vertex_PosTexNorTan> vertices = vector<RHI_Vertex_PosTexNorTan>(m_VertexCount);
            vector<uint32_t> indices = vector<uint32_t>(m_FaceCount * 3);

            // �ҷ��� ���̷� ���ο� ������ ������.
            m_ProgressDesc = "Generating positions...";
            if (GeneratePositions(positions, height_data))
            {
                // ���ؽ��� �ε����� �����Ѵ�.
                m_ProgressDesc = "Generating terrain vertices and indices...";
                if (GenerateVerticesIndices(positions, indices, vertices))
                {
                    m_ProgressDesc = "Generating normals and tangents...";
                    positions.clear();
                    positions.shrink_to_fit();

                    // �븻�� ����ϰ� ���ġ�� ���� �ε巴���Ѵ�.
                    if (GenerateNormalTangents(indices, vertices))
                    {
                        // ������� ���ؽ��� �ε����� ���� ������.
                        UpdateFromVertices(indices, vertices);
                    }
                }
            }

            // �۾� �Ϸ�
            m_Progress_jobs_done = 0;
            m_Progress_job_count = 1;
            m_ProgressDesc.clear();

            m_IsGenerating = false;
        });
    }

    bool Terrain::GeneratePositions(vector<Vector3>& positions, const vector<std::byte>& height_map)
    {
        // �����Ͱ� ���ٸ� ��ȯ
        if (height_map.empty())
        {
            LOG_ERROR("Height map is empty");
            return false;
        }

        uint32_t k = 0;

        for (uint32_t y = 0; y < m_Height; y++)
        {
            for (uint32_t x = 0; x < m_Width; x++)
            {
                // ���̼�ġ�� �����ϰ� �����Ѵ�.
                const float height = (static_cast<float>(height_map[k]) / 255.0f);

                // �ε��� ���
                const uint32_t index = y * m_Width + x;
                // ����� �߽����� �����Ѵ�.
                positions[index].x = static_cast<float>(x) - m_Width * 0.5f;
                positions[index].z = static_cast<float>(y) - m_Height * 0.5f;
                // �ּ�ġ���� �ִ�ġ���� ���� �����Ѵ�.
                positions[index].y = Util::Lerp(m_MinY, m_MaxY, height);

                // 4���� ����
                k += 4;

                // ī��Ʈ�� �ϳ� �÷��ش�.
                m_Progress_jobs_done++;
            }
        }

        return true;
    }

    bool Terrain::GenerateVerticesIndices(const vector<Vector3>& positions, vector<uint32_t>& indices, vector<RHI_Vertex_PosTexNorTan>& vertices)
    {
        if (positions.empty())
        {
            LOG_ERROR("Positions are empty");
            return false;
        }

        uint32_t index = 0;
        uint32_t k = 0;
        uint32_t u_index = 0;
        uint32_t v_index = 0;

        for (uint32_t y = 0; y < m_Height - 1; y++)
        {
            for (uint32_t x = 0; x < m_Width - 1; x++)
            {
                // 4���� ���� ��ġ�� ���Ѵ�.
                // �Ʒ��� ����
                const uint32_t index_bottom_left = y * m_Width + x;
                const uint32_t index_bottom_right = y * m_Width + x + 1;
                const uint32_t index_top_left = (y + 1) * m_Width + x;
                const uint32_t index_top_right = (y + 1) * m_Width + x + 1;

                // �Ʒ� ������
                index = index_bottom_right;
                indices[k] = index;
                // UV�� ��� �Ʒ� �������̹Ƿ� 1, 1
                vertices[index] = RHI_Vertex_PosTexNorTan(positions[index], Vector2(u_index + 1.0f, v_index + 1.0f));

                // �Ʒ� ����
                index = index_bottom_left;
                indices[k + 1] = index;
                // UV�� ��� �Ʒ� �����̹Ƿ� 0, 1
                vertices[index] = RHI_Vertex_PosTexNorTan(positions[index], Vector2(u_index + 0.0f, v_index + 1.0f));

                // �� ����
                index = index_top_left;
                indices[k + 2] = index;
                // UV�� ��� �� �����̹Ƿ� 0, 0
                vertices[index] = RHI_Vertex_PosTexNorTan(positions[index], Vector2(u_index + 0.0f, v_index + 0.0f));

                // �Ʒ� ������
                index = index_bottom_right;
                indices[k + 3] = index;
                vertices[index] = RHI_Vertex_PosTexNorTan(positions[index], Vector2(u_index + 1.0f, v_index + 1.0f));

                // �� ����
                index = index_top_left;
                indices[k + 4] = index;
                vertices[index] = RHI_Vertex_PosTexNorTan(positions[index], Vector2(u_index + 0.0f, v_index + 0.0f));

                // �� ������
                index = index_top_right;
                indices[k + 5] = index;
                vertices[index] = RHI_Vertex_PosTexNorTan(positions[index], Vector2(u_index + 1.0f, v_index + 0.0f));
                
                // ���� �簢��
                k += 6;

                // �簢�� �ϳ��� �������� u�� �ϳ� �÷��ش�.
                u_index++;

                // track progress
                m_Progress_jobs_done++;
            }

            // ������ �������Ƿ� u�� �ʱ�ȭ v�� �ϳ� �÷��ش�.
            u_index = 0;
            v_index++;
        }

        return true;
    }

    bool Terrain::GenerateNormalTangents(const vector<uint32_t>& indices, vector<RHI_Vertex_PosTexNorTan>& vertices)
    {
        if (indices.empty())
        {
            LOG_ERROR("Indices are empty");
            return false;
        }

        if (vertices.empty())
        {
            LOG_ERROR("Vertices are empty");
            return false;
        }

        // ��� ���ؽ� ������ ����.
        uint32_t face_count = static_cast<uint32_t>(indices.size()) / 3;
        uint32_t vertex_count = static_cast<uint32_t>(vertices.size());

        // �� ������ �븻�� ź��Ʈ�� �Ҵ�
        vector<Vector3> face_normals(face_count);
        vector<Vector3> face_tangents(face_count);
        {
            // ���� ����ŭ �ݺ��Ѵ�.
            for (uint32_t i = 0; i < face_count; i++)
            {
                Vector3 edge_a;
                Vector3 edge_b;

                {
                    // ���� 1 -> 0 ���� 
                    edge_a = Vector3(
                        vertices[indices[(i * 3)]].pos[0] - vertices[indices[(i * 3) + 1]].pos[0],
                        vertices[indices[(i * 3)]].pos[1] - vertices[indices[(i * 3) + 1]].pos[1],
                        vertices[indices[(i * 3)]].pos[2] - vertices[indices[(i * 3) + 1]].pos[2]
                    );

                    // ���� 2 -> 1 ����
                    edge_b = Vector3(
                        vertices[indices[(i * 3) + 1]].pos[0] - vertices[indices[(i * 3) + 2]].pos[0],
                        vertices[indices[(i * 3) + 1]].pos[1] - vertices[indices[(i * 3) + 2]].pos[1],
                        vertices[indices[(i * 3) + 1]].pos[2] - vertices[indices[(i * 3) + 2]].pos[2]
                    );

                    // ���� �� ���͸� �����ϤŤ� �븻�� ���Ѵ�.
                    face_normals[i] = Vector3::Cross(edge_a, edge_b);
                }

                {
                    // �ؽ��Ŀ��� ù��° uv ��ǥ�� ���Ѵ�.
                    const float tcU1 = vertices[indices[(i * 3)]].tex[0] - vertices[indices[(i * 3) + 1]].tex[0];
                    const float tcV1 = vertices[indices[(i * 3)]].tex[1] - vertices[indices[(i * 3) + 1]].tex[1];

                    // �ؽ��Ŀ��� �ι�° uv ��ǥ�� ���Ѵ�.
                    const float tcU2 = vertices[indices[(i * 3) + 1]].tex[0] - vertices[indices[(i * 3) + 2]].tex[0];
                    const float tcV2 = vertices[indices[(i * 3) + 1]].tex[1] - vertices[indices[(i * 3) + 2]].tex[1];

                    // ������� ����� �̿��Ͽ� ź��Ʈ�� ���Ѵ�.
                    face_tangents[i].x = (tcV1 * edge_a.x - tcV2 * edge_b.x * (1.0f / (tcU1 * tcV2 - tcU2 * tcV1)));
                    face_tangents[i].y = (tcV1 * edge_a.y - tcV2 * edge_b.y * (1.0f / (tcU1 * tcV2 - tcU2 * tcV1)));
                    face_tangents[i].z = (tcV1 * edge_a.z - tcV2 * edge_b.z * (1.0f / (tcU1 * tcV2 - tcU2 * tcV1)));
                }
            }
        }

        // �븻�� ���ġ�� ����Ѵ�.
        const auto compute_vertex_normals_tangents = [this, &face_normals, &face_tangents, &vertices, &indices, &vertex_count, &face_count](uint32_t i_start, uint32_t i_end)
        {
            Vector3 normal_sum = Vector3::Zero;
            Vector3 tangent_sum = Vector3::Zero;
            float faces_using = 0;

            for (uint32_t i = i_start; i < i_end; ++i)
            {
                for (uint32_t j = 0; j < face_count; ++j)
                {
                    // ���� �ε����� �����������
                    if (indices[j * 3] == i || indices[(j * 3) + 1] == i || indices[(j * 3) + 2] == i)
                    {
                        // �� ���ؽ����� �ϳ��� ���� �����ϴ� ��� ���� ������Ų��.
                        normal_sum += face_normals[j];
                        tangent_sum += face_tangents[j];
                        faces_using++;
                    }

                    if (j == face_count < 1)
                    {
                        m_Progress_jobs_done += face_count;
                    }
                }

                // ���ġ�� ���ϰ� ����ȭ
                normal_sum /= faces_using;
                normal_sum.Normalize();

                // ���ġ�� ���ϰ� ����ȭ
                tangent_sum /= faces_using;
                tangent_sum.Normalize();

                // �븻 �� ����
                vertices[i].nor[0] = normal_sum.x;
                vertices[i].nor[1] = normal_sum.y;
                vertices[i].nor[2] = normal_sum.z;

                // ź��Ʈ �� ����
                vertices[i].tan[0] = tangent_sum.x;
                vertices[i].tan[1] = tangent_sum.y;
                vertices[i].tan[2] = tangent_sum.z;

                // �ʱ�ȭ
                normal_sum = Vector3::Zero;
                tangent_sum = Vector3::Zero;
                faces_using = 0;
            }
        };

        // ������ �߰�
        m_Context->GetSubModule<Threading>()->AddTaskLoop(compute_vertex_normals_tangents, vertex_count);

        return true;
    }

    void Terrain::UpdateFromModel(const shared_ptr<Model>& model) const
    {
        ASSERT(model != nullptr);

        // ������ �𵨿��� �ҷ��´�.

        if (Renderable* renderable = m_Entity->AddComponent<Renderable>())
        {
            renderable->GeometrySet(
                "Terrain",
                0,
                model->GetMesh()->IndicesCount(),
                0,
                model->GetMesh()->VerticesCount(),
                model->GetAABB(),
                model.get()
            );

            renderable->UseDefaultMaterial();
        }
    }

    void Terrain::UpdateFromVertices(const vector<uint32_t>& indices, vector<RHI_Vertex_PosTexNorTan>& vertices)
    {
        // ������ ���ؽ��κ��� �����Ѵ�.

        // ���� ���� ���
        if (!m_Model)
        {
            // ���� ������.
            m_Model = make_shared<Model>(m_Context);

            // ������ �߰�
            m_Model->AppendGeometry(indices, vertices);
            m_Model->UpdateGeometry();

            ResourceCache* resource_cache = m_Context->GetSubModule<ResourceCache>();
            m_Model->SetResourceFilePath(resource_cache->GetProjectDirectory() + m_Entity->GetObjectName() + "_terrain_" + to_string(m_ObjectID) + string(EXTENSION_MODEL));
            m_Model = resource_cache->Cache(m_Model);
        }
        else
        {
            // �� Ŭ���� �� ������ �߰�
            m_Model->Clear();
            m_Model->AppendGeometry(indices, vertices);
            m_Model->UpdateGeometry();
        }

        UpdateFromModel(m_Model);
    }
}