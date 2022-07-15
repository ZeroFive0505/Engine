#pragma once

#include "IComponent.h"
#include "../../RHI/RHI_Definition.h"

namespace PlayGround
{
	class Renderable;

	// 리플렉션 프로브
	class ReflectionProbe : public IComponent
	{
	public:
		ReflectionProbe(Context* context, Entity* entity, uint64_t id = 0);
		~ReflectionProbe() = default;

		// IComponent 가상 메서드 오버라이드
		void Update(double delta_time) override;
		void Serialize(FileStream* stream) override;
		void Deserialize(FileStream* stream) override;

		// 컬링 여부
		bool IsInViewFrustum(Renderable* renderable, uint32_t index) const;

		inline RHI_Texture* GetColorTexture() { return m_TextureColor.get(); }

		inline RHI_Texture* GetDepthTexture() { return m_TextureDepth.get(); }

		inline Math::Matrix& GetViewMatrix(const uint32_t index) { return m_MatrixViews[index]; }

		inline Math::Matrix& GetProjectionMatrix() { return m_MatrixProjection; }

		inline uint32_t GetResolution() const { return m_Resolution; }

		void SetResolution(const uint32_t resolution);

		inline const Math::Vector3& GetExtents() const { return m_Extents; }

		void SetExtents(const Math::Vector3& extentds);

		inline uint32_t GetUpdateIntervalFrames() const { return m_Update_interval_frames; }

		void SetUpdateIntervalFrames(const uint32_t update_interval_frame);

		inline uint32_t GetUpdateFaceCount() const { return m_Update_face_count; }

		void SetUpdateFaceCount(const uint32_t update_face_count);

		inline float GetNearPlane() const { return m_NearPlane; }

		void SetNearPlane(const float near_plane);

		inline float GetFarPlane() const { return m_FarPlane; }

		void SetFarPlane(const float far_plane);

		inline bool GetNeedsToUpdate() const { return m_Needs_to_update; }

		inline uint32_t GetUpdateFaceStartIndex() const { return m_Update_face_start_index; }

		inline const Math::BoundingBox& GetAABB() const { return m_AABB; }

	private:
		void CreateTextures();
		void ComputeProjectionMatrix();
		void ComputeFrustum();

		uint32_t m_Resolution = 512;

		Math::Vector3 m_Extents = Math::Vector3(4.0f, 2.0f, 4.0f);
		Math::BoundingBox m_AABB = Math::BoundingBox::Zero;

		uint32_t m_Update_interval_frames = 0;

		uint32_t m_Update_face_count = 6;

		float m_NearPlane = 0.3f;
		float m_FarPlane = 1000.0f;

		std::array<Math::Matrix, 6> m_MatrixViews;
		Math::Matrix m_MatrixProjection;
		std::array<Math::Frustum, 6> m_Frustums;

		uint32_t m_Frames_since_last_update = 0;
		uint32_t m_Update_face_start_index = 0;
		bool m_Needs_to_update = false;
		bool m_First_update = true;

		std::shared_ptr<RHI_Texture> m_TextureColor;
		std::shared_ptr<RHI_Texture> m_TextureDepth;

		bool m_Prev_reverse_z = false;
	};
}
