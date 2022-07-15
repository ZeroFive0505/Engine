#include "Common.h"
#include "Transform.h"
#include "../World.h"
#include "../Entity.h"
#include "../../IO/FileStream.h"
#include "../../Core/Context.h"
#include "../../Core/Engine.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
	Transform::Transform(Context* context, Entity* entity, uint64_t id /*= 0*/) : IComponent(context, entity, id, this)
	{
        // 트랜스폼 컴포넌트 초기화
        m_LocalPosistion = Vector3::Zero;
        m_LocalRoation = Quaternion(0, 0, 0, 1);
        m_LocalScale = Vector3::One;
        m_Matrix = Matrix::Identity;
        m_LocalMatrix = Matrix::Identity;
        m_PrevMatrix = Matrix::Identity;
        m_Parent = nullptr;
        // 더티 플래그의 경우 일단 참으로 초기화
        m_IsDirty = true;

        // 해당 값들의 gette, setter 설정
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_LocalPosistion, Vector3);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_LocalRoation, Quaternion);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_LocalScale, Vector3);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_Matrix, Matrix);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_LocalMatrix, Matrix);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_LookAt, Vector3);
	}

    void Transform::OnInit()
    {
        // 초기화시에 더티 플래그 참으로 한다.
        m_IsDirty = true;
    }

    void Transform::Update(double delta_time)
    {
        // 만약 업데이트시에 더티 플래그가 거짓이라면 바로 돌아간다.
        if (!m_IsDirty)
            return;

        // 트랜스폼 업데이트
        UpdateTransform();

        // 트랜스폼 업데이트 이후 더티 플래그 초기화
        m_IsDirty = false;
        m_PositionChanged_this_frame = false;
        m_RotationChanged_this_frame = true;
        m_ScaleChanged_this_frame = true;
    }

    void Transform::OnStop()
    {
        // 플레이모드에서 정지시 초기 상태로 돌아간다.
        SetLocalPosition(m_InitPosition);
        SetLocalRotaion(m_InitRotation);
        SetLocalScale(m_InitScale);
    }

    void Transform::Serialize(FileStream* stream)
    {
        // 저장
        stream->Write(m_LocalPosistion);
        stream->Write(m_LocalRoation);
        stream->Write(m_LocalScale);
        stream->Write(m_LookAt);

        stream->Write(m_Parent ? m_Parent->GetEntity()->GetObjectID() : 0);
    }

    void Transform::Deserialize(FileStream* stream)
    {
        // 불러오기
        stream->Read(&m_LocalPosistion);
        stream->Read(&m_LocalRoation);
        stream->Read(&m_LocalScale);
        stream->Read(&m_LookAt);

        uint64_t parent_entity_id = 0;
        stream->Read(&parent_entity_id);

        // 만약 부모의 아이디가 존재한다면 부모가 존재했다는 뜻
        if (parent_entity_id != 0)
        {
            // 현재 씬에서 아이디로 부모를 찾아온다.
            if (const shared_ptr<Entity>& parent = m_Context->GetSubModule<World>()->EntityGetByID(parent_entity_id))
            {
                parent->GetTransform()->AddChild(this);
            }
        }

        // 트랜스폼 업데이트
        UpdateTransform();
    }

    void Transform::UpdateTransform()
    {
        // 행렬 생성
        m_LocalMatrix = Matrix(m_LocalPosistion, m_LocalRoation, m_LocalScale);

        // 만약 부모가 존재할 경우
        if (m_Parent)
            m_Matrix = m_LocalMatrix * m_Parent->GetMatrix();
        // 아닐 경우는 바로 적용
        else
            m_Matrix = m_LocalMatrix;

        // 자식 트랜스폼을 갱신한다.
        for (Transform* child : m_vecChildren)
            child->UpdateTransform();
    }

    void Transform::SetPosition(const Vector3& position)
    {
        if (GetPosition() == position)
            return;
        // 포지션을 적용한다.
        // 만약 부모가 존재할 경우 부모의 역 행렬을 곱해준다.
        SetLocalPosition(!HasParent() ? position : position * GetParent()->GetMatrix().Inverted());
    }
    
    void Transform::SetLocalPosition(const Vector3& position)
    {
        if (m_LocalPosistion == position)
            return;

        // 게임 모드가 아닐시에는 새로운 초기 위치
        if(!m_Context->m_Engine->IsEngineModeSet(EEngine_Mode::GameMode))
            m_InitPosition = position;

        m_LocalPosistion = position;
        UpdateTransform();

        // 이번 프레임에 포지션의 변화가 생김
        m_PositionChanged_this_frame = true;
    }

    void Transform::SetRotation(const Quaternion& rotation)
    {
        if (GetRotation() == rotation)
            return;

        // 부모가 있을경우 부모의 회전의 역을 곱해준다.
        SetLocalRotaion(!HasParent() ? rotation : rotation * GetParent()->GetRotation().Inverse());
    }

    void Transform::SetLocalRotaion(const Quaternion& rotation)
    {
        if (m_LocalRoation == rotation)
            return;

        // 마찬가지로 게임모드가 아닐시에는 새로운 초기 회전값
        if (!m_Context->m_Engine->IsEngineModeSet(EEngine_Mode::GameMode))
            m_InitRotation = rotation;

        m_LocalRoation = rotation;
        UpdateTransform();

        m_RotationChanged_this_frame = true;
    }

    void Transform::SetScale(const Vector3& scale)
    {
        if (GetScale() == scale)
            return;

        // 부모가 존재할경우 나눠준다.
        SetLocalScale(!HasParent() ? scale : scale / GetParent()->GetScale());
    }

    void Transform::SetLocalScale(const Vector3& scale)
    {
        if (m_LocalScale == scale)
            return;

        // 게임모드가 아닐 경우 초기 스케일 값으로 설정
        if (!m_Context->m_Engine->IsEngineModeSet(EEngine_Mode::GameMode))
            m_InitScale = scale;

        m_LocalScale = scale;

        // 만약 들어온 스케일이 0이라면 아주 작은 값으로 초기화한다.
        m_LocalScale.x = (m_LocalScale.x == 0.0f) ? Util::EPSILON : m_LocalScale.x;
        m_LocalScale.y = (m_LocalScale.y == 0.0f) ? Util::EPSILON : m_LocalScale.y;
        m_LocalScale.z = (m_LocalScale.z == 0.0f) ? Util::EPSILON : m_LocalScale.z;

        UpdateTransform();

        m_ScaleChanged_this_frame = true;
    }

    void Transform::Translate(const Vector3& delta)
    {
        // 부모가 없을 경우는 간단하게 더해준다.
        if (!HasParent())
            SetLocalPosition(m_LocalPosistion + delta);
        // 부모가 있을 경우에는 부모의 역행렬에 이동량을 곱한 만큼을 더해준다.
        else
            SetLocalPosition(m_LocalPosistion + GetParent()->GetMatrix().Inverted() * delta);
    }

    void Transform::Rotate(const Quaternion& delta)
    {
        // 부모가 없을 경우는 곱해서 정규화
        if (!HasParent())
            SetLocalRotaion((m_LocalRoation * delta).Normalized());
        // 부모가 있을 경우에는 역행렬과 현재 회전치 그리고 회전량을 곱한다.
        else
            SetLocalRotaion(m_LocalRoation * GetRotation().Inverse() * delta * GetRotation());
    }

    Vector3 Transform::GetUp() const
    {
        // 로컬 Up벡터를 구한다.
        return GetLocalRotation() * Vector3::Up;
    }

    Vector3 Transform::GetDown() const
    {
        // 로컬 Down벡터를 구한다.
        return GetLocalRotation() * Vector3::Down;
    }

    Vector3 Transform::GetForward() const
    {
        // 로컬 Forward벡터를 구한다.
        return GetLocalRotation() * Vector3::Forward;
    }

    Vector3 Transform::GetBackward() const
    {
        // 로컬 Backward벡터를 구한다.
        return GetLocalRotation() * Vector3::Backward;
    }

    Vector3 Transform::GetRight() const
    {
        // 로컬 Right벡터를 구한다.
        return GetLocalRotation() * Vector3::Right;
    }

    Vector3 Transform::GetLeft() const
    {
        // 로컬 Left벡터를 구한다.
        return GetLocalRotation() * Vector3::Left;
    }

    Transform* Transform::GetChildByIndex(const uint32_t index)
    {
        // 만약 자식이 없을 경우 경고
        if (!HasChildren())
        {
            LOG_WARNING("%s has no children.", GetEntityName().c_str());
            return nullptr;
        }

        // 인덱스가 넘어갔을 경우도 마찬가지
        if (index >= GetChildrenCount())
        {
            LOG_WARNING("There is no child with an index of \"%d\".", index);
            return nullptr;
        }

        // 자식 반환
        return m_vecChildren[index];
    }

    Transform* Transform::GetChildByName(const string& name)
    {
        // 이름으로 자식을 찾아온다.
        for (const auto& child : m_vecChildren)
        {
            if (child->GetEntityName() == name)
                return child;
        }

        return nullptr;
    }

    void Transform::SetParent(Transform* new_parent)
    {
        // 자기자신을 부모로 정한다면 반환
        if (new_parent)
        {
            if (GetObjectID() == new_parent->GetObjectID())
                return;
        }

        // 기존 부모가 새로운 부모랑 같으면 반환
        if (m_Parent && new_parent)
        {
            if (m_Parent->GetObjectID() == new_parent->GetObjectID())
                return;
        }

        // 만약 새로운 부모가 원래 이 컴포넌트의 자식이었다면
        if (new_parent && new_parent->IsDescendantOf(this))
        {
            // 모든 자식을 순회한다.
            for (Transform* child : m_vecChildren)
            {
                // 그 자식의 부모를 이 컴포넌트의 부모로 설정한다.
                child->SetParentInternal(m_Parent);
            }

            // 자식 컴포넌트 클리어
            m_vecChildren.clear();
        }
        
        // 원래 부모가 존재했다면 이 컴포넌트를 삭제한다.
        if (m_Parent)
            m_Parent->RemoveChildInternal(this);

        // 새로운 부모가 존재할시에
        if (new_parent)
        {
            // 새로운 부모의 자식에 이 트랜스폼을 추가한다.
            new_parent->AddChildInternal(this);
            // 더티 플래그
            new_parent->MakeDirty();
        }

        m_Parent = new_parent;
        m_IsDirty = true;
    }


    void Transform::AddChild(Transform* child)
    {
        ASSERT(child != nullptr);

        // 자기 자신을 추가한다면 반환
        if (child->GetObjectID() == GetObjectID())
            return;

        // 자식의 부모를 새롭게 설정한다.
        child->SetParent(this);
    }

    void Transform::RemoveChild(Transform* child)
    {
        ASSERT(child != nullptr);

        // 자기 자신의 삭제는 불가능
        if (child->GetObjectID() == GetObjectID())
            return;

        // 아이디와 매칭되는 자식을 삭제한다.
        m_vecChildren.erase(remove_if(m_vecChildren.begin(), m_vecChildren.end(),
            [child](Transform* vec_transform) {
            return vec_transform->GetObjectID() == child->GetObjectID(); 
        }), m_vecChildren.end());

        // 그리고 자식의 부모를 없앤다.
        child->SetParent(nullptr);
    }

    void Transform::SetParentInternal(Transform* new_parent)
    {
        // 만약 새로운 부모가 자기 자신이라면 그냥 반환
        if (new_parent)
        {
            if (GetObjectID() == new_parent->GetObjectID())
                return;
        }

        // 두 조건중에 하나라도 참이면 더티 플래그
        if ((m_Parent && !new_parent) || (!m_Parent && new_parent))
            m_IsDirty = true;

        m_Parent = new_parent;
    }

    void Transform::AddChildInternal(Transform* child)
    {
        ASSERT(child != nullptr);

        if (child->GetObjectID() == GetObjectID())
            return;

        // 기존 자식에 이미 있는 자식인지를 확인한다.
        if (!(find(m_vecChildren.begin(), m_vecChildren.end(), child) != m_vecChildren.end()))
        {
            // 없었다면 추가
            m_vecChildren.emplace_back(child);
        }
    }

    void Transform::RemoveChildInternal(Transform* child)
    {
        ASSERT(child != nullptr);

        if (child->GetObjectID() == GetObjectID())
            return;

        // 삭제
        m_vecChildren.erase(remove_if(m_vecChildren.begin(), m_vecChildren.end(),
            [child](Transform* vec_transform) { 
            return vec_transform->GetObjectID() == child->GetObjectID(); 
        }), m_vecChildren.end());
    }

    void Transform::AcquireChildren()
    {
        // 일단 비운다.
        m_vecChildren.clear();
        m_vecChildren.shrink_to_fit();

        // 현재 씬에 배치되어있는 모든 엔티티들을 가져온다.
        auto entities = GetContext()->GetSubModule<World>()->EntityGetAll();

        for (const auto& entity : entities)
        {
            if (!entity)
                continue;

            // 엔티티의 트랜스폼을 가져온다.
            auto possible_child = entity->GetTransform();

            // 이 트랜스폼이 부모를 가지고있지 않다면 그냥 무시한다.
            if (!possible_child->HasParent())
                continue;

            // 이 트랜스폼의 부모가 이 트랜스폼을 가리킨다면
            if (possible_child->GetParent()->GetObjectID() == GetObjectID())
            {
                // 추가한다.
                m_vecChildren.emplace_back(possible_child);

                // 새롭게 추가된 자식도 확인한다.
                possible_child->AcquireChildren();
            }
        }
    }

    bool Transform::IsDescendantOf(Transform* transform) const
    {
        ASSERT(transform != nullptr);

        // 만약 부모가 없다면 반환
        if (!m_Parent)
            return false;

        // 만약 부모의 아이디와 매개변수로 넘어온 아이디의 값이 같다면
        if (m_Parent->GetObjectID() == transform->GetObjectID())
            return true;

        // 모든 자식들을 순회한다.
        for (Transform* child : transform->GetChildren())
        {
            if (IsDescendantOf(child))
                return true;
        }

        return false;
    }

    void Transform::GetDescendants(vector<Transform*>* descendants)
    {
        // 모든 자식들을 반한한다.
        for (Transform* child : m_vecChildren)
        {
            descendants->emplace_back(child);

            if (child->HasChildren())
                child->GetDescendants(descendants);
        }
    }

    Matrix Transform::GetParentTransformMatrix() const
    {
        // 부모가 존재할 경우 부모의 행렬을 반환한다.
        return HasParent() ? GetParent()->GetMatrix() : Matrix::Identity;
    }
}