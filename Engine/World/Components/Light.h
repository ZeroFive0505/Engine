#pragma once

#include <array>
#include <memory>
#include "IComponent.h"
#include "../../Math/Vector4.h"
#include "../../Math/Vector3.h"
#include "../../Math/Matrix.h"
#include "../../RHI/RHI_Definition.h"
#include "../../Math/Frustum.h"

namespace PlayGround
{
	class Camera;
	class Renderable;
	class Renderer;

	enum class LightType
	{
		Directional,
		Point,
		Spot
	};

	struct ShadowSlice
	{
		Math::Vector3 min = Math::Vector3::Zero;
		Math::Vector3 max = Math::Vector3::Zero;
		Math::Vector3 center = Math::Vector3::Zero;
		Math::Frustum frustum;
	};

	struct ShadowMap
	{
		std::shared_ptr<RHI_Texture> texture_color;
		std::shared_ptr<RHI_Texture> texture_depth;
		std::vector<ShadowSlice> slices;
	};

    // 광원 컴포넌트
	class Light : public IComponent
	{
    public:
        Light(Context* context, Entity* entity, uint64_t id = 0);
        ~Light() = default;

        // IComponent 가상 메서드 오버라이드
        void OnInit() override;
        void OnStart() override;
        void Update(double delta_time) override;
        void Serialize(FileStream* stream) override;
        void Deserialize(FileStream* stream) override;

        inline const LightType GetLightType() const { return m_LightType; }
        void SetLightType(LightType type);

        inline void SetColor(const Math::Vector4& rgb) { m_ColorRGB = rgb; }
        inline const PlayGround::Math::Vector4& GetColor() const { return m_ColorRGB; }

        inline void SetIntensity(float value) { m_Intensity = value; }
        inline float GetIntensity()    const { return m_Intensity; }

        inline bool GetShadowsEnabled() const { return m_ShadowsEnabled; }
        void SetShadowsEnabled(bool cast_shadows);

        inline bool GetShadowsScreenSpaceEnabled() const { return m_Shadows_screen_space_enabled; }
        inline void SetShadowsScreenSpaceEnabled(bool cast_contact_shadows) { m_Shadows_screen_space_enabled = cast_contact_shadows; }

        inline bool GetShadowsTransparentEnabled() const { return m_Shadows_transparent_enabled; }
        void SetShadowsTransparentEnabled(bool cast_transparent_shadows);

        inline bool GetVolumetricEnabled() const { return m_VolumetricEnabled; }
        inline void SetVolumetricEnabled(bool is_volumetric) { m_VolumetricEnabled = is_volumetric; }

        void SetRange(float range);
        inline float GetRange() const { return m_Range; }

        void SetAngle(float angle);
        inline float GetAngle() const { return m_AngleRadian; }

        inline void SetBias(float value) { m_Bias = value; }
        inline float GetBias() const { return m_Bias; }

        inline void SetNormalBias(float value) { m_NormalBias = value; }
        inline float GetNormalBias() const { return m_NormalBias; }

        const Math::Matrix& GetViewMatrix(uint32_t index = 0) const;
        const Math::Matrix& GetProjectionMatrix(uint32_t index = 0) const;

        inline RHI_Texture* GetDepthTexture() const { return m_ShadowMap.texture_depth.get(); }
        inline RHI_Texture* GetColorTexture() const { return m_ShadowMap.texture_color.get(); }
        uint32_t GetShadowArraySize() const;
        void CreateShadowMap();

        bool IsInViewFrustum(Renderable* renderable, uint32_t index) const;

    private:
        void ComputeViewMatrix();
        bool ComputeProjectionMatrix(uint32_t index = 0);
        void ComputeCascadeSplits();

        // 그림자
        bool m_ShadowsEnabled = true;
        bool m_Shadows_screen_space_enabled = true;
        bool m_Shadows_transparent_enabled = true;
        uint32_t m_CascadeCount = 4;
        ShadowMap m_ShadowMap;

        // 그림자 바이어스
        float m_Bias = 0.0f;
        float m_NormalBias = 3.0f;

        LightType m_LightType = LightType::Directional;
        Math::Vector4 m_ColorRGB = Math::Vector4(1.0f, 0.76f, 0.57f, 1.0f);
        bool m_VolumetricEnabled = true;
        float m_Range = 10.0f;
        float m_Intensity = 128000.0f;
        float m_AngleRadian = 0.5f;
        bool m_Initialized = false;
        std::array<Math::Matrix, 6> m_MatrixViews;
        std::array<Math::Matrix, 6> m_MatrixProjections;

        // 더티 체크
        bool m_IsDirty = true;
        Math::Matrix m_PrevCameraView = Math::Matrix::Identity;
        bool m_PrevReverseZ = false;

        Renderer* m_Renderer;
	};
}