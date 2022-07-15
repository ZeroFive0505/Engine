#pragma once

#include "IComponent.h"
#include <atomic>
#include "../../RHI/RHI_Definition.h"

namespace PlayGround
{
	class Model;

	namespace Math
	{
		class Vector3;
	}

	// 지형 컴포넌트
	class Terrain : public IComponent
	{
	public:
		Terrain(Context* context, Entity* entity, uint64_t id = 0);
		~Terrain() = default;

		// IComponent 가상 메서드 오버라이드
		void OnInit() override;
		void Serialize(FileStream* stream) override;
		void Deserialize(FileStream* stream) override;

		const auto& GetHeightMap() const { return m_HeightMap; }
		void SetHeightMap(const std::shared_ptr<RHI_Texture2D>& height_map);

		inline float GetMinY() const { return m_MinY; }
		inline void SetMinY(float min_z) { m_MinY = min_z; }

		inline float GetMaxY() const { return m_MaxY; }
		inline void SetMaxY(float max_z) { m_MaxY = max_z; }

		inline float GetProgress() const { return static_cast<float>(static_cast<double>(m_Progress_jobs_done) / static_cast<double>(m_Progress_job_count)); }
		const std::string& GetProgressDescription() const { return m_ProgressDesc; }

		void GenerateAsync();

	private:
		bool GeneratePositions(std::vector<Math::Vector3>& positions, const std::vector<std::byte>& height_map);
		bool GenerateVerticesIndices(const std::vector<Math::Vector3>& positions, std::vector<uint32_t>& indices, std::vector<RHI_Vertex_PosTexNorTan>& vertices);
		bool GenerateNormalTangents(const std::vector<uint32_t>& indices, std::vector<RHI_Vertex_PosTexNorTan>& vertices);
		void UpdateFromModel(const std::shared_ptr<Model>& model) const;
		void UpdateFromVertices(const std::vector<uint32_t>& indices, std::vector<RHI_Vertex_PosTexNorTan>& vertices);

		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
		float m_MinY = 0.0f;
		float m_MaxY = 30.0f;
		float m_Vertex_density = 1.0f;
		std::atomic<bool> m_IsGenerating = false;
		uint64_t m_VertexCount = 0;
		uint64_t m_FaceCount = 0;
		std::atomic<uint64_t> m_Progress_jobs_done = 0;
		uint64_t m_Progress_job_count = 1;
		std::string m_ProgressDesc;
		std::shared_ptr<RHI_Texture2D> m_HeightMap;
		std::shared_ptr<Model> m_Model;
	};
}