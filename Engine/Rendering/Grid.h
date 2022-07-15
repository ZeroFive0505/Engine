#pragma once

#include <vector>
#include <memory>
#include "../RHI/RHI_Definition.h"
#include "../Math/Matrix.h"
#include "../EngineDefinition.h"


namespace PlayGround
{
	class Context;
	class Transform;

	// ���� ������ ����Ʈ���� �����Ǵ� �׸���
	class Grid
	{
	public:
		Grid(std::shared_ptr<RHI_Device> rhi_device);
		~Grid() = default;

		const Math::Matrix& ComputeWorldMatrix(Transform* camera);
		
		inline const auto& GetVertexBuffer() const { return m_VertexBuffer; }

		inline const uint32_t GetVertexCount() const { return m_VertexCount; }

	private:
		void BuildGrid(std::vector<RHI_Vertex_PosCol>* vertices);

		uint32_t m_Terrain_height = 200;
		uint32_t m_Terrain_width = 200;
		uint32_t m_VertexCount = 0;

		std::shared_ptr<RHI_VertexBuffer> m_VertexBuffer;
		Math::Matrix m_World;
	};
}
