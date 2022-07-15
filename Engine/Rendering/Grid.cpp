#include "Common.h"
#include "Grid.h"
#include "../RHI/RHI_Vertex.h"
#include "../RHI/RHI_VertexBuffer.h"
#include "../World/Components/Transform.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
	Grid::Grid(shared_ptr<RHI_Device> rhi_device)
	{
		// ���ؽ��� �����Ѵ�.
		vector<RHI_Vertex_PosCol> vertices;
		BuildGrid(&vertices);
		m_VertexCount = static_cast<uint32_t>(vertices.size());

		// ���ؽ� ����
		m_VertexBuffer = make_shared<RHI_VertexBuffer>(rhi_device, false, "grid");
		if (!m_VertexBuffer->Create(vertices))
		{
			LOG_ERROR("Failed to create vertex buffer.");
		}
	}

	const Matrix& Grid::ComputeWorldMatrix(Transform* camera)
	{
		// �׸��� ��ġ

		// �׸���� 1ĭ�� �������� ��ġ
		const float grid_spacing = 1.0f;
		// ī�޶���  ��ġ�� ����ؼ� �̵��Ѵ�.
		const Vector3 translation = Vector3(
			static_cast<int>(camera->GetPosition().x / grid_spacing) * grid_spacing,
			0.0f,
			static_cast<int>(camera->GetPosition().z / grid_spacing) * grid_spacing);

		// ������� ����
		m_World = Matrix::CreateScale(grid_spacing) * Matrix::CreateTranslation(translation);

		return m_World;
	}

	void Grid::BuildGrid(vector<RHI_Vertex_PosCol>* vertices)
	{
		// �� ����, ���̸� ���Ѵ�.
		const int halfsizeW = int(m_Terrain_width * 0.5f);
		const int halfsizeH = int(m_Terrain_height * 0.5f);

		for (int j = -halfsizeH; j < halfsizeH; j++)
		{
			for (int i = -halfsizeW; i < halfsizeW; i++)
			{
				// �ָ� ���������� ����
				const float alphaWidth = 1.0f - static_cast<float>(Util::Abs(i)) / static_cast<float>(halfsizeH);
				const float alphaHeight = 1.0f - static_cast<float>(Util::Abs(j)) / static_cast<float>(halfsizeW);
				float alpha = (alphaWidth + alphaHeight) * 0.5f;
				alpha = Util::Pow(alpha, 10.0f);

				// ù��° ����
				// ���� ��
				float positionX = static_cast<float>(i);
				float positionZ = static_cast<float>(j + 1);
				vertices->emplace_back(Vector3(positionX, 0.0f, positionZ), Vector4(1.0f, 1.0f, 1.0f, alpha));

				// ������ ��
				positionX = static_cast<float>(i + 1);
				positionZ = static_cast<float>(j + 1);
				vertices->emplace_back(Vector3(positionX, 0.0f, positionZ), Vector4(1.0f, 1.0f, 1.0f, alpha));

				// �ι�° ����
				// ������ ��
				positionX = static_cast<float>(i + 1);
				positionZ = static_cast<float>(j + 1);
				vertices->emplace_back(Vector3(positionX, 0.0f, positionZ), Vector4(1.0f, 1.0f, 1.0f, alpha));

				// ������ �Ʒ�
				positionX = static_cast<float>(i + 1);
				positionZ = static_cast<float>(j);
				vertices->emplace_back(Vector3(positionX, 0.0f, positionZ), Vector4(1.0f, 1.0f, 1.0f, alpha));

				// ����° ����
				// ������ �Ʒ�
				positionX = static_cast<float>(i + 1);
				positionZ = static_cast<float>(j);
				vertices->emplace_back(Vector3(positionX, 0.0f, positionZ), Vector4(1.0f, 1.0f, 1.0f, alpha));

				// ���� �Ʒ�
				positionX = static_cast<float>(i);
				positionZ = static_cast<float>(j);
				vertices->emplace_back(Vector3(positionX, 0.0f, positionZ), Vector4(1.0f, 1.0f, 1.0f, alpha));

				// �׹�° ����
				// ���� �Ʒ�
				positionX = static_cast<float>(i);
				positionZ = static_cast<float>(j);
				vertices->emplace_back(Vector3(positionX, 0.0f, positionZ), Vector4(1.0f, 1.0f, 1.0f, alpha));

				// ���� ��
				positionX = static_cast<float>(i);
				positionZ = static_cast<float>(j + 1);
				vertices->emplace_back(Vector3(positionX, 0.0f, positionZ), Vector4(1.0f, 1.0f, 1.0f, alpha));
			}
		}
	}
}