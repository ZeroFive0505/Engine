#pragma once

#include <vector>
#include "../RHI/RHI_Definition.h"
#include "../RHI/RHI_Vertex.h"

// 엔진에서 사용되는 기본 지오메트리 제공 함수
namespace PlayGround::Utility::Geometry
{
    // 큐브 생성
	static void CreateCube(std::vector<RHI_Vertex_PosTexNorTan>* vertices, std::vector<uint32_t>* indices)
	{
        using namespace Math;

        // 앞면
        vertices->emplace_back(Vector3(-0.5f, -0.5f, -0.5f), Vector2(0, 1), Vector3(0, 0, -1), Vector3(0, 1, 0));
        vertices->emplace_back(Vector3(-0.5f, 0.5f, -0.5f), Vector2(0, 0), Vector3(0, 0, -1), Vector3(0, 1, 0));
        vertices->emplace_back(Vector3(0.5f, -0.5f, -0.5f), Vector2(1, 1), Vector3(0, 0, -1), Vector3(0, 1, 0));
        vertices->emplace_back(Vector3(0.5f, 0.5f, -0.5f), Vector2(1, 0), Vector3(0, 0, -1), Vector3(0, 1, 0));

        // 아랫면
        vertices->emplace_back(Vector3(-0.5f, -0.5f, 0.5f), Vector2(0, 1), Vector3(0, -1, 0), Vector3(1, 0, 0));
        vertices->emplace_back(Vector3(-0.5f, -0.5f, -0.5f), Vector2(0, 0), Vector3(0, -1, 0), Vector3(1, 0, 0));
        vertices->emplace_back(Vector3(0.5f, -0.5f, 0.5f), Vector2(1, 1), Vector3(0, -1, 0), Vector3(1, 0, 0));
        vertices->emplace_back(Vector3(0.5f, -0.5f, -0.5f), Vector2(1, 0), Vector3(0, -1, 0), Vector3(1, 0, 0));

        // 뒷면
        vertices->emplace_back(Vector3(-0.5f, -0.5f, 0.5f), Vector2(1, 1), Vector3(0, 0, 1), Vector3(0, 1, 0));
        vertices->emplace_back(Vector3(-0.5f, 0.5f, 0.5f), Vector2(1, 0), Vector3(0, 0, 1), Vector3(0, 1, 0));
        vertices->emplace_back(Vector3(0.5f, -0.5f, 0.5f), Vector2(0, 1), Vector3(0, 0, 1), Vector3(0, 1, 0));
        vertices->emplace_back(Vector3(0.5f, 0.5f, 0.5f), Vector2(0, 0), Vector3(0, 0, 1), Vector3(0, 1, 0));

        // 윗면
        vertices->emplace_back(Vector3(-0.5f, 0.5f, 0.5f), Vector2(0, 0), Vector3(0, 1, 0), Vector3(1, 0, 0));
        vertices->emplace_back(Vector3(-0.5f, 0.5f, -0.5f), Vector2(0, 1), Vector3(0, 1, 0), Vector3(1, 0, 0));
        vertices->emplace_back(Vector3(0.5f, 0.5f, 0.5f), Vector2(1, 0), Vector3(0, 1, 0), Vector3(1, 0, 0));
        vertices->emplace_back(Vector3(0.5f, 0.5f, -0.5f), Vector2(1, 1), Vector3(0, 1, 0), Vector3(1, 0, 0));

        // 왼쪽
        vertices->emplace_back(Vector3(-0.5f, -0.5f, 0.5f), Vector2(0, 1), Vector3(-1, 0, 0), Vector3(0, 1, 0));
        vertices->emplace_back(Vector3(-0.5f, 0.5f, 0.5f), Vector2(0, 0), Vector3(-1, 0, 0), Vector3(0, 1, 0));
        vertices->emplace_back(Vector3(-0.5f, -0.5f, -0.5f), Vector2(1, 1), Vector3(-1, 0, 0), Vector3(0, 1, 0));
        vertices->emplace_back(Vector3(-0.5f, 0.5f, -0.5f), Vector2(1, 0), Vector3(-1, 0, 0), Vector3(0, 1, 0));

        // 오른쪽
        vertices->emplace_back(Vector3(0.5f, -0.5f, 0.5f), Vector2(1, 1), Vector3(1, 0, 0), Vector3(0, 1, 0));
        vertices->emplace_back(Vector3(0.5f, 0.5f, 0.5f), Vector2(1, 0), Vector3(1, 0, 0), Vector3(0, 1, 0));
        vertices->emplace_back(Vector3(0.5f, -0.5f, -0.5f), Vector2(0, 1), Vector3(1, 0, 0), Vector3(0, 1, 0));
        vertices->emplace_back(Vector3(0.5f, 0.5f, -0.5f), Vector2(0, 0), Vector3(1, 0, 0), Vector3(0, 1, 0));

        // 앞면 인덱스 버퍼
        indices->emplace_back(0); indices->emplace_back(1); indices->emplace_back(2);
        indices->emplace_back(2); indices->emplace_back(1); indices->emplace_back(3);

        // 아랫면 인덱스 버퍼
        indices->emplace_back(4); indices->emplace_back(5); indices->emplace_back(6);
        indices->emplace_back(6); indices->emplace_back(5); indices->emplace_back(7);

        // 뒷면 인덱스 버퍼
        indices->emplace_back(10); indices->emplace_back(9); indices->emplace_back(8);
        indices->emplace_back(11); indices->emplace_back(9); indices->emplace_back(10);

        // 윗면 인덱스 버퍼
        indices->emplace_back(14); indices->emplace_back(13); indices->emplace_back(12);
        indices->emplace_back(15); indices->emplace_back(13); indices->emplace_back(14);

        // 왼쪽 인덱스 버퍼
        indices->emplace_back(16); indices->emplace_back(17); indices->emplace_back(18);
        indices->emplace_back(18); indices->emplace_back(17); indices->emplace_back(19);

        // 오른쪽 인덱스 버퍼
        indices->emplace_back(22); indices->emplace_back(21); indices->emplace_back(20);
        indices->emplace_back(23); indices->emplace_back(21); indices->emplace_back(22);
	}

    // 사각형 생성
    static void CreateQuad(std::vector<RHI_Vertex_PosTexNorTan>* vertices, std::vector<uint32_t>* indices)
    {
        using namespace Math;

        vertices->emplace_back(Vector3(-0.5f, 0.0f, 0.5f), Vector2(0, 0), Vector3(0, 1, 0), Vector3(1, 0, 0)); // 0 왼쪽위 정점
        vertices->emplace_back(Vector3(0.5f, 0.0f, 0.5f), Vector2(1, 0), Vector3(0, 1, 0), Vector3(1, 0, 0)); // 1 오른쪽 위
        vertices->emplace_back(Vector3(-0.5f, 0.0f, -0.5f), Vector2(0, 1), Vector3(0, 1, 0), Vector3(1, 0, 0)); // 2 왼쪽 아래
        vertices->emplace_back(Vector3(0.5f, 0.0f, -0.5f), Vector2(1, 1), Vector3(0, 1, 0), Vector3(1, 0, 0)); // 3 오른쪽 아래

        indices->emplace_back(3);
        indices->emplace_back(2);
        indices->emplace_back(0);
        indices->emplace_back(3);
        indices->emplace_back(0);
        indices->emplace_back(1);
    }

    // 구 생성
    static void CreateSphere(std::vector<RHI_Vertex_PosTexNorTan>* vertices, std::vector<uint32_t>* indices, float radius = 1.0f, int slices = 20, int stacks = 20)
    {
        using namespace Math;

        // 노말, 탄젠트
        Vector3 normal = Vector3(0.0f, 1.0f, 0.0f);
        Vector3 tangent = Vector3(1.0f, 0.0f, 0.0f);
        // 구의 가장 윗 정점 
        vertices->emplace_back(Vector3(0.0f, radius, 0.0f), Vector2::Zero, normal, tangent);

        // 위도
        const float phiStep = Util::PI / stacks;
        // 경도
        const float thetaStep = Util::PI_2 / slices;
        
        // 위에서 하나의 정점은 이미 넣었으니 1부터 시작한다.
        for (int i = 1; i <= stacks - 1; i++)
        {
            // 얼마나 움직여야하는지
            const float phi = i * phiStep;

            // 경도 반복
            for (int j = 0; j <= slices; j++)
            {
                // 값
                const float theta = j * thetaStep;

                // 구 좌표계로 변환
                Vector3 p = Vector3(
                    (radius * sin(phi) * cos(theta)),
                    (radius * cos(phi)),
                    (radius * sin(phi) * sin(theta))
                );

                // 탄젠트
                Vector3 t = Vector3(
                    -radius * sin(phi) * sin(theta),
                    0.0f,
                    radius * sin(phi) * cos(theta)).Normalized();
                // 노말의 경우 이미 점 p를 알고있으니 간단하게 정규화만 한다.
                Vector3 n = p.Normalized();
                // UV좌표 u의 경우 경도, v의 경우 위도 따라서 0 ~ 1사이의 값이 나오게 최대치 만큼 나눠준다.
                Vector2 uv = Vector2(theta / Util::PI_2, phi / Util::PI);
                vertices->emplace_back(p, uv, n, t);
            }
        }

        normal = Vector3(0.0f, -1.0f, 0.0f);
        tangent = Vector3(1.0f, 0.0f, 0.0f);
        // 아래 정점 삽입
        vertices->emplace_back(Vector3(0.0f, -radius, 0.0f), Vector2(0.0f, 1.0f), normal, tangent);

        for (int i = 1; i <= slices; i++)
        {
            indices->emplace_back(0);
            indices->emplace_back(i + 1);
            indices->emplace_back(i);
        }

        int baseIndex = 1;
        const int ringVertexCount = slices + 1;

        for (int i = 0; i < stacks - 2; i++)
        {
            for (int j = 0; j < slices; j++)
            {
                indices->emplace_back(baseIndex + i * ringVertexCount + j);
                indices->emplace_back(baseIndex + i * ringVertexCount + j + 1);
                indices->emplace_back(baseIndex + (i + 1) * ringVertexCount + j);

                indices->emplace_back(baseIndex + (i + 1) * ringVertexCount + j);
                indices->emplace_back(baseIndex + i * ringVertexCount + j + 1);
                indices->emplace_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
            }
        }

        int southPoleIndex = (int)vertices->size() - 1;
        baseIndex = southPoleIndex - ringVertexCount;

        for (int i = 0; i < slices; i++)
        {
            indices->emplace_back(southPoleIndex);
            indices->emplace_back(baseIndex + i);
            indices->emplace_back(baseIndex + i + 1);
        }
    }

    // 원기둥 생성
    static void CreateCylinder(std::vector<RHI_Vertex_PosTexNorTan>* vertices, std::vector<uint32_t>* indices, float radiusTop = 1.0f, float radiusBottom = 1.0f, float height = 1.0f, int slices = 15, int stacks = 15)
    {
        using namespace Math;

        // 쌓는 양
        const float stackHeight = height / stacks;
        // 반지름 변화량
        const float radiusStep = (radiusTop - radiusBottom) / stacks;
        const float ringCount = (float)(stacks + 1);

        for (int i = 0; i < ringCount; i++)
        {
            // 가운데 지점을 찾는다.
            const float y = -0.5f * height + i * stackHeight;
            // 반지름 계산
            const float r = radiusBottom + i * radiusStep;
            // 변화량 계산
            const float dTheta = Util::PI_2 / slices;
            for (int j = 0; j <= slices; j++)
            {
                // 좌표 계산
                const float c = cos(j * dTheta);
                const float s = sin(j * dTheta);

                // 정점
                Vector3 v = Vector3(r * c, y, r * s);
                // uv 계산 여기서 v의 경우 상하를 뒤집는다.
                Vector2 uv = Vector2((float)j / slices, 1.0f - (float)i / stacks);
                // 탄젠트
                Vector3 t = Vector3(-s, 0.0f, c);

                const float dr = radiusBottom - radiusTop;
                Vector3 bitangent = Vector3(dr * c, -height, dr * s);

                Vector3 n = Vector3::Cross(t, bitangent).Normalized();
                vertices->emplace_back(v, uv, n, t);
            }
        }

        const int ringVertexCount = slices + 1;
        for (int i = 0; i < stacks; i++)
        {
            for (int j = 0; j < slices; j++)
            {
                indices->push_back(i * ringVertexCount + j);
                indices->push_back((i + 1) * ringVertexCount + j);
                indices->push_back((i + 1) * ringVertexCount + j + 1);

                indices->push_back(i * ringVertexCount + j);
                indices->push_back((i + 1) * ringVertexCount + j + 1);
                indices->push_back(i * ringVertexCount + j + 1);
            }
        }

        // 윗부분
        int baseIndex = (int)vertices->size();
        float y = 0.5f * height;
        const float dTheta = Util::PI_2 / slices;

        Vector3 normal;
        Vector3 tangent;

        for (int i = 0; i <= slices; i++)
        {
            const float x = radiusTop * cos(i * dTheta);
            const float z = radiusTop * sin(i * dTheta);
            const float u = x / height + 0.5f;
            const float v = z / height + 0.5f;

            normal = Vector3(0, 1, 0);
            tangent = Vector3(1, 0, 0);
            vertices->emplace_back(Vector3(x, y, z), Vector2(u, v), normal, tangent);
        }

        normal = Vector3(0, 1, 0);
        tangent = Vector3(1, 0, 0);
        vertices->emplace_back(Vector3(0, y, 0), Vector2(0.5f, 0.5f), normal, tangent);

        int centerIndex = (int)vertices->size() - 1;
        for (int i = 0; i < slices; i++)
        {
            indices->push_back(centerIndex);
            indices->push_back(baseIndex + i + 1);
            indices->push_back(baseIndex + i);
        }

        // 아랫부분
        baseIndex = (int)vertices->size();
        y = -0.5f * height;

        for (int i = 0; i <= slices; i++)
        {
            const float x = radiusBottom * cos(i * dTheta);
            const float z = radiusBottom * sin(i * dTheta);
            const float u = x / height + 0.5f;
            const float v = z / height + 0.5f;

            normal = Vector3(0, -1, 0);
            tangent = Vector3(1, 0, 0);
            vertices->emplace_back(Vector3(x, y, z), Vector2(u, v), normal, tangent);
        }

        normal = Vector3(0, -1, 0);
        tangent = Vector3(1, 0, 0);
        vertices->emplace_back(Vector3(0, y, 0), Vector2(0.5f, 0.5f), normal, tangent);

        centerIndex = (int)vertices->size() - 1;
        for (int i = 0; i < slices; i++)
        {
            indices->push_back(centerIndex);
            indices->push_back(baseIndex + i);
            indices->push_back(baseIndex + i + 1);
        }
    }


    static void CreateCone(std::vector<RHI_Vertex_PosTexNorTan>* vertices, std::vector<uint32_t>* indices, float radius = 1.0f, float height = 2.0f)
    {
        // 원뿔의 경우 위의 반지름을 0으로 원기둥을 만들어낸다.
        CreateCylinder(vertices, indices, 0.0f, radius, height);
    }
}