#pragma once

#include "IComponent.h"
#include <vector>
#include "../../Math/Vector3.h"
#include "../../Math/Quaternion.h"
#include "../../Math/Matrix.h"

namespace PlayGround
{
	class RHI_Device;
	class RHI_ConstantBuffer;

	// 모든 엔티티에 기본적으로 추가되는 트랜스폼 컴포넌트
	class Transform : public IComponent
	{
	public:
		Transform(Context* context, Entity* entity, uint64_t id = 0);
		~Transform() = default;

		// IComponent 가상 메서드 오버라이드
		void OnInit() override;
		void Update(double delta_time) override;
		void OnStop() override;
		void Serialize(FileStream* stream) override;
		void Deserialize(FileStream* stream) override;

		inline Math::Vector3 GetPosition() const { return m_Matrix.GetTranslation(); }

		inline const Math::Vector3& GetLocalPosition() const { return m_LocalPosistion; }

		void SetPosition(const Math::Vector3& position);

		void SetLocalPosition(const Math::Vector3& position);

		inline Math::Quaternion GetRotation() const { return m_Matrix.GetRotation(); }

		inline const Math::Quaternion& GetLocalRotation() const { return m_LocalRoation; }

		void SetRotation(const Math::Quaternion& rotation);

		void SetLocalRotaion(const Math::Quaternion& rotation);

		inline Math::Vector3 GetScale() const { return m_Matrix.GetScale(); }

		inline const Math::Vector3& GetLocalScale() const { return m_LocalScale; }

		inline Math::Vector3 GetIninPosition() const { return m_InitPosition; }
		inline Math::Quaternion GetinitRotation() const { return m_InitRotation; }
		inline Math::Vector3 GetInitScale() const { return m_InitScale; }

		void SetScale(const Math::Vector3& scale);

		void SetLocalScale(const Math::Vector3& scale);

		void Translate(const Math::Vector3& delta);
		void Rotate(const Math::Quaternion& delta);

		Math::Vector3 GetUp() const;
		Math::Vector3 GetDown() const;
		Math::Vector3 GetForward() const;
		Math::Vector3 GetBackward() const;
		Math::Vector3 GetRight() const;
		Math::Vector3 GetLeft() const;

		inline bool HasPositionChangedThisFrame() const { return m_PositionChanged_this_frame; }

		inline bool HasRotationChangedThisFrame() const { return m_RotationChanged_this_frame; }

		inline bool HasScaleChangedThisFrame() const { return m_ScaleChanged_this_frame; }

		void SetParent(Transform* new_parent);
		Transform* GetChildByIndex(uint32_t index);
		Transform* GetChildByName(const std::string& name);
		void AcquireChildren();
		void RemoveChild(Transform* child);
		void AddChild(Transform* child);
		bool IsDescendantOf(Transform* transform) const;
		void GetDescendants(std::vector<Transform*>* descendants);

		inline bool IsRoot() const { return m_Parent == nullptr; }

		inline bool HasParent() const { return m_Parent != nullptr; }

		inline bool HasChildren() const { return GetChildrenCount() > 0 ? true : false; }

		inline uint32_t GetChildrenCount() const { return static_cast<uint32_t>(m_vecChildren.size()); }

		inline Transform* GetRoot() { return HasParent() ? GetParent()->GetRoot() : this; }

		inline Transform* GetParent() const { return m_Parent; }

		inline std::vector<Transform*>& GetChildren() { return m_vecChildren; }

		inline void MakeDirty() { m_IsDirty = true; }

		inline void LookAt(const Math::Vector3& v) { m_LookAt = v; }

		inline const Math::Matrix& GetMatrix() const { return m_Matrix; }

		inline const Math::Matrix& GetLocalMatrix() const { return m_LocalMatrix; }

		inline const Math::Matrix& GetPrevMatrix() const { return m_PrevMatrix; }

		inline void SetPrevMatrix(const Math::Matrix& matrix) { m_PrevMatrix = matrix; }

	private:
		void SetParentInternal(Transform* parent);
		void AddChildInternal(Transform* child);
		void RemoveChildInternal(Transform* child);

		void UpdateTransform();
		Math::Matrix GetParentTransformMatrix() const;
		bool m_IsDirty = false;

		Math::Vector3 m_InitPosition;
		Math::Quaternion m_InitRotation;
		Math::Vector3 m_InitScale = Math::Vector3::One;

		Math::Vector3 m_LocalPosistion;
		Math::Quaternion m_LocalRoation;
		Math::Vector3 m_LocalScale;

		Math::Matrix m_Matrix;
		Math::Matrix m_LocalMatrix;
		Math::Vector3 m_LookAt;

		Transform* m_Parent;
		std::vector<Transform*> m_vecChildren;

		Math::Matrix m_PrevMatrix;

		bool m_PositionChanged_this_frame = false;
		bool m_RotationChanged_this_frame = false;
		bool m_ScaleChanged_this_frame = false;
	};
}