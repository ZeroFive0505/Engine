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

        // 저장
        stream->Write(m_HeightMap ? m_HeightMap->GetResourceFilePathNative() : no_path);
        stream->Write(m_Model ? m_Model->GetResourceName() : no_path);
        stream->Write(m_MinY);
        stream->Write(m_MaxY);
    }

    void Terrain::Deserialize(FileStream* stream)
    {
        // 불러오기
        ResourceCache* resource_cache = m_Context->GetSubModule<ResourceCache>();
        m_HeightMap = resource_cache->GetByPath<RHI_Texture2D>(stream->ReadAs<string>());
        m_Model = resource_cache->GetByName<Model>(stream->ReadAs<string>());
        stream->Read(&m_MinY);
        stream->Read(&m_MaxY);

        UpdateFromModel(m_Model);
    }

    void Terrain::SetHeightMap(const shared_ptr<RHI_Texture2D>& height_map)
    {
        // 높이 맵 적용
        m_HeightMap = m_Context->GetSubModule<ResourceCache>()->Cache<RHI_Texture2D>(height_map);
    }

    void Terrain::GenerateAsync()
    {
        // 비동기 지형 생성

        // 이미 생성되고있는 도중이라면 반환
        if (m_IsGenerating)
        {
            LOG_WARNING("Terrain is already being generated, please wait...");
            return;
        }

        // 높이 맵이 존재하지 않을 경우
        if (!m_HeightMap)
        {
            LOG_WARNING("You need to assign a height map before trying to generate a terrain.");

            // 모델을 삭제한다.
            m_Context->GetSubModule<ResourceCache>()->Remove(m_Model);
            m_Model.reset();

            Renderable* renderable = m_Entity->AddComponent<Renderable>();

            if (renderable)
                renderable->GeometryClear();

            return;
        }

        // 스레드 추가
        m_Context->GetSubModule<Threading>()->AddTask([this]()
        {
            // 생성 시작
            m_IsGenerating = true;

            // 높이 정보를 가져온다.
            vector<std::byte> height_data;
            {
                height_data = m_HeightMap->GetMip(0, 0).bytes;

                // 데이터가 존재하지 않을 경우 
                if (height_data.empty())
                {
                    // 파일에서 로드한다.
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

            // 파일 정보를 가져온다.
            m_Height = m_HeightMap->GetHeight();
            m_Width = m_HeightMap->GetWidth();
            m_VertexCount = m_Height * m_Width;
            m_FaceCount = (m_Height - 1) * (m_Width - 1) * 2;
            m_Progress_jobs_done = 0;
            m_Progress_job_count = m_VertexCount * 2 + m_FaceCount + m_VertexCount * m_FaceCount;

            // 미리 메모리를 할당한다.
            vector<Vector3> positions = vector<Vector3>(m_Height * m_Width);
            vector<RHI_Vertex_PosTexNorTan> vertices = vector<RHI_Vertex_PosTexNorTan>(m_VertexCount);
            vector<uint32_t> indices = vector<uint32_t>(m_FaceCount * 3);

            // 불러온 높이로 새로운 지형을 만들어낸다.
            m_ProgressDesc = "Generating positions...";
            if (GeneratePositions(positions, height_data))
            {
                // 버텍스랑 인덱스를 생성한다.
                m_ProgressDesc = "Generating terrain vertices and indices...";
                if (GenerateVerticesIndices(positions, indices, vertices))
                {
                    m_ProgressDesc = "Generating normals and tangents...";
                    positions.clear();
                    positions.shrink_to_fit();

                    // 노말을 계산하고 평균치를 내서 부드럽게한다.
                    if (GenerateNormalTangents(indices, vertices))
                    {
                        // 만들어진 버텍스와 인덱스로 모델을 만들어낸다.
                        UpdateFromVertices(indices, vertices);
                    }
                }
            }

            // 작업 완료
            m_Progress_jobs_done = 0;
            m_Progress_job_count = 1;
            m_ProgressDesc.clear();

            m_IsGenerating = false;
        });
    }

    bool Terrain::GeneratePositions(vector<Vector3>& positions, const vector<std::byte>& height_map)
    {
        // 데이터가 없다면 반환
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
                // 높이수치를 적당하게 조절한다.
                const float height = (static_cast<float>(height_map[k]) / 255.0f);

                // 인덱스 계산
                const uint32_t index = y * m_Width + x;
                // 가운데를 중심으로 시작한다.
                positions[index].x = static_cast<float>(x) - m_Width * 0.5f;
                positions[index].z = static_cast<float>(y) - m_Height * 0.5f;
                // 최소치에서 최대치까지 선형 보간한다.
                positions[index].y = Util::Lerp(m_MinY, m_MaxY, height);

                // 4개의 정점
                k += 4;

                // 카운트를 하나 늘려준다.
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
                // 4개의 정점 위치를 구한다.
                // 아래를 기준
                const uint32_t index_bottom_left = y * m_Width + x;
                const uint32_t index_bottom_right = y * m_Width + x + 1;
                const uint32_t index_top_left = (y + 1) * m_Width + x;
                const uint32_t index_top_right = (y + 1) * m_Width + x + 1;

                // 아래 오른쪽
                index = index_bottom_right;
                indices[k] = index;
                // UV의 경우 아래 오른쪽이므로 1, 1
                vertices[index] = RHI_Vertex_PosTexNorTan(positions[index], Vector2(u_index + 1.0f, v_index + 1.0f));

                // 아래 왼쪽
                index = index_bottom_left;
                indices[k + 1] = index;
                // UV의 경우 아래 왼쪽이므로 0, 1
                vertices[index] = RHI_Vertex_PosTexNorTan(positions[index], Vector2(u_index + 0.0f, v_index + 1.0f));

                // 위 왼쪽
                index = index_top_left;
                indices[k + 2] = index;
                // UV의 경우 위 왼쪽이므로 0, 0
                vertices[index] = RHI_Vertex_PosTexNorTan(positions[index], Vector2(u_index + 0.0f, v_index + 0.0f));

                // 아래 오른쪽
                index = index_bottom_right;
                indices[k + 3] = index;
                vertices[index] = RHI_Vertex_PosTexNorTan(positions[index], Vector2(u_index + 1.0f, v_index + 1.0f));

                // 위 왼쪽
                index = index_top_left;
                indices[k + 4] = index;
                vertices[index] = RHI_Vertex_PosTexNorTan(positions[index], Vector2(u_index + 0.0f, v_index + 0.0f));

                // 위 오른쪽
                index = index_top_right;
                indices[k + 5] = index;
                vertices[index] = RHI_Vertex_PosTexNorTan(positions[index], Vector2(u_index + 1.0f, v_index + 0.0f));
                
                // 다음 사각형
                k += 6;

                // 사각형 하나가 끝났으니 u를 하나 늘려준다.
                u_index++;

                // track progress
                m_Progress_jobs_done++;
            }

            // 한줄이 끝났으므로 u는 초기화 v는 하나 늘려준다.
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

        // 면과 버텍스 갯수를 센다.
        uint32_t face_count = static_cast<uint32_t>(indices.size()) / 3;
        uint32_t vertex_count = static_cast<uint32_t>(vertices.size());

        // 면 갯수로 노말과 탄젠트를 할당
        vector<Vector3> face_normals(face_count);
        vector<Vector3> face_tangents(face_count);
        {
            // 면의 수만큼 반복한다.
            for (uint32_t i = 0; i < face_count; i++)
            {
                Vector3 edge_a;
                Vector3 edge_b;

                {
                    // 간선 1 -> 0 벡터 
                    edge_a = Vector3(
                        vertices[indices[(i * 3)]].pos[0] - vertices[indices[(i * 3) + 1]].pos[0],
                        vertices[indices[(i * 3)]].pos[1] - vertices[indices[(i * 3) + 1]].pos[1],
                        vertices[indices[(i * 3)]].pos[2] - vertices[indices[(i * 3) + 1]].pos[2]
                    );

                    // 간선 2 -> 1 벡터
                    edge_b = Vector3(
                        vertices[indices[(i * 3) + 1]].pos[0] - vertices[indices[(i * 3) + 2]].pos[0],
                        vertices[indices[(i * 3) + 1]].pos[1] - vertices[indices[(i * 3) + 2]].pos[1],
                        vertices[indices[(i * 3) + 1]].pos[2] - vertices[indices[(i * 3) + 2]].pos[2]
                    );

                    // 위의 두 벡터를 외적하ㅕㅇ 노말을 구한다.
                    face_normals[i] = Vector3::Cross(edge_a, edge_b);
                }

                {
                    // 텍스쳐에서 첫번째 uv 좌표를 구한다.
                    const float tcU1 = vertices[indices[(i * 3)]].tex[0] - vertices[indices[(i * 3) + 1]].tex[0];
                    const float tcV1 = vertices[indices[(i * 3)]].tex[1] - vertices[indices[(i * 3) + 1]].tex[1];

                    // 텍스쳐에서 두번째 uv 좌표를 구한다.
                    const float tcU2 = vertices[indices[(i * 3) + 1]].tex[0] - vertices[indices[(i * 3) + 2]].tex[0];
                    const float tcV2 = vertices[indices[(i * 3) + 1]].tex[1] - vertices[indices[(i * 3) + 2]].tex[1];

                    // 역행렬을 계산을 이용하여 탄젠트를 구한다.
                    face_tangents[i].x = (tcV1 * edge_a.x - tcV2 * edge_b.x * (1.0f / (tcU1 * tcV2 - tcU2 * tcV1)));
                    face_tangents[i].y = (tcV1 * edge_a.y - tcV2 * edge_b.y * (1.0f / (tcU1 * tcV2 - tcU2 * tcV1)));
                    face_tangents[i].z = (tcV1 * edge_a.z - tcV2 * edge_b.z * (1.0f / (tcU1 * tcV2 - tcU2 * tcV1)));
                }
            }
        }

        // 노말의 평균치를 계산한다.
        const auto compute_vertex_normals_tangents = [this, &face_normals, &face_tangents, &vertices, &indices, &vertex_count, &face_count](uint32_t i_start, uint32_t i_end)
        {
            Vector3 normal_sum = Vector3::Zero;
            Vector3 tangent_sum = Vector3::Zero;
            float faces_using = 0;

            for (uint32_t i = i_start; i < i_end; ++i)
            {
                for (uint32_t j = 0; j < face_count; ++j)
                {
                    // 같은 인덱스를 쓰고있을경우
                    if (indices[j * 3] == i || indices[(j * 3) + 1] == i || indices[(j * 3) + 2] == i)
                    {
                        // 이 버텍스들은 하나의 면을 구성하는 요소 따라서 누적시킨다.
                        normal_sum += face_normals[j];
                        tangent_sum += face_tangents[j];
                        faces_using++;
                    }

                    if (j == face_count < 1)
                    {
                        m_Progress_jobs_done += face_count;
                    }
                }

                // 평균치를 구하고 정규화
                normal_sum /= faces_using;
                normal_sum.Normalize();

                // 평균치를 구하고 정규화
                tangent_sum /= faces_using;
                tangent_sum.Normalize();

                // 노말 값 적용
                vertices[i].nor[0] = normal_sum.x;
                vertices[i].nor[1] = normal_sum.y;
                vertices[i].nor[2] = normal_sum.z;

                // 탄젠트 값 적용
                vertices[i].tan[0] = tangent_sum.x;
                vertices[i].tan[1] = tangent_sum.y;
                vertices[i].tan[2] = tangent_sum.z;

                // 초기화
                normal_sum = Vector3::Zero;
                tangent_sum = Vector3::Zero;
                faces_using = 0;
            }
        };

        // 스레드 추가
        m_Context->GetSubModule<Threading>()->AddTaskLoop(compute_vertex_normals_tangents, vertex_count);

        return true;
    }

    void Terrain::UpdateFromModel(const shared_ptr<Model>& model) const
    {
        ASSERT(model != nullptr);

        // 지형을 모델에서 불러온다.

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
        // 지형을 버텍스로부터 생성한다.

        // 모델이 없을 경우
        if (!m_Model)
        {
            // 모델을 만들어낸다.
            m_Model = make_shared<Model>(m_Context);

            // 데이터 추가
            m_Model->AppendGeometry(indices, vertices);
            m_Model->UpdateGeometry();

            ResourceCache* resource_cache = m_Context->GetSubModule<ResourceCache>();
            m_Model->SetResourceFilePath(resource_cache->GetProjectDirectory() + m_Entity->GetObjectName() + "_terrain_" + to_string(m_ObjectID) + string(EXTENSION_MODEL));
            m_Model = resource_cache->Cache(m_Model);
        }
        else
        {
            // 모델 클리어 후 데이터 추가
            m_Model->Clear();
            m_Model->AppendGeometry(indices, vertices);
            m_Model->UpdateGeometry();
        }

        UpdateFromModel(m_Model);
    }
}