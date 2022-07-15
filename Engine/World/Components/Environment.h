#pragma once

#include "IComponent.h"
#include "../../RHI/RHI_Definition.h"

namespace PlayGround
{
	enum class EnvironmentType
	{
		Cubemap,
		Sphere
	};

	// 스카이 박스 컴포넌트
	class Environment : public IComponent
	{
	public:
		Environment(Context* context, Entity* entity, uint64_t id = 0);
		~Environment() = default;

		// IComponent 가상 메서드 오버라이드
		void Update(double delta_time) override;
		void Serialize(FileStream* stream) override;
		void Deserialize(FileStream* stream) override;

		const std::shared_ptr<RHI_Texture> GetTexture() const;
		void SetTexture(const std::shared_ptr<RHI_Texture>& texture);

	private:
		void SetFromTextureArray(const std::vector<std::string>& file_paths);
		void SetFromTextureSphere(const std::string& file_path);

		std::vector<std::string> m_vecFilePaths;
		EnvironmentType m_EnvironmentType = EnvironmentType::Sphere;
		bool m_IsDirty;
	};
}