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
        // Ʈ������ ������Ʈ �ʱ�ȭ
        m_LocalPosistion = Vector3::Zero;
        m_LocalRoation = Quaternion(0, 0, 0, 1);
        m_LocalScale = Vector3::One;
        m_Matrix = Matrix::Identity;
        m_LocalMatrix = Matrix::Identity;
        m_PrevMatrix = Matrix::Identity;
        m_Parent = nullptr;
        // ��Ƽ �÷����� ��� �ϴ� ������ �ʱ�ȭ
        m_IsDirty = true;

        // �ش� ������ gette, setter ����
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_LocalPosistion, Vector3);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_LocalRoation, Quaternion);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_LocalScale, Vector3);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_Matrix, Matrix);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_LocalMatrix, Matrix);
        REGISTER_ATTRIBUTE_VALUE_VALUE(m_LookAt, Vector3);
	}

    void Transform::OnInit()
    {
        // �ʱ�ȭ�ÿ� ��Ƽ �÷��� ������ �Ѵ�.
        m_IsDirty = true;
    }

    void Transform::Update(double delta_time)
    {
        // ���� ������Ʈ�ÿ� ��Ƽ �÷��װ� �����̶�� �ٷ� ���ư���.
        if (!m_IsDirty)
            return;

        // Ʈ������ ������Ʈ
        UpdateTransform();

        // Ʈ������ ������Ʈ ���� ��Ƽ �÷��� �ʱ�ȭ
        m_IsDirty = false;
        m_PositionChanged_this_frame = false;
        m_RotationChanged_this_frame = true;
        m_ScaleChanged_this_frame = true;
    }

    void Transform::OnStop()
    {
        // �÷��̸�忡�� ������ �ʱ� ���·� ���ư���.
        SetLocalPosition(m_InitPosition);
        SetLocalRotaion(m_InitRotation);
        SetLocalScale(m_InitScale);
    }

    void Transform::Serialize(FileStream* stream)
    {
        // ����
        stream->Write(m_LocalPosistion);
        stream->Write(m_LocalRoation);
        stream->Write(m_LocalScale);
        stream->Write(m_LookAt);

        stream->Write(m_Parent ? m_Parent->GetEntity()->GetObjectID() : 0);
    }

    void Transform::Deserialize(FileStream* stream)
    {
        // �ҷ�����
        stream->Read(&m_LocalPosistion);
        stream->Read(&m_LocalRoation);
        stream->Read(&m_LocalScale);
        stream->Read(&m_LookAt);

        uint64_t parent_entity_id = 0;
        stream->Read(&parent_entity_id);

        // ���� �θ��� ���̵� �����Ѵٸ� �θ� �����ߴٴ� ��
        if (parent_entity_id != 0)
        {
            // ���� ������ ���̵�� �θ� ã�ƿ´�.
            if (const shared_ptr<Entity>& parent = m_Context->GetSubModule<World>()->EntityGetByID(parent_entity_id))
            {
                parent->GetTransform()->AddChild(this);
            }
        }

        // Ʈ������ ������Ʈ
        UpdateTransform();
    }

    void Transform::UpdateTransform()
    {
        // ��� ����
        m_LocalMatrix = Matrix(m_LocalPosistion, m_LocalRoation, m_LocalScale);

        // ���� �θ� ������ ���
        if (m_Parent)
            m_Matrix = m_LocalMatrix * m_Parent->GetMatrix();
        // �ƴ� ���� �ٷ� ����
        else
            m_Matrix = m_LocalMatrix;

        // �ڽ� Ʈ�������� �����Ѵ�.
        for (Transform* child : m_vecChildren)
            child->UpdateTransform();
    }

    void Transform::SetPosition(const Vector3& position)
    {
        if (GetPosition() == position)
            return;
        // �������� �����Ѵ�.
        // ���� �θ� ������ ��� �θ��� �� ����� �����ش�.
        SetLocalPosition(!HasParent() ? position : position * GetParent()->GetMatrix().Inverted());
    }
    
    void Transform::SetLocalPosition(const Vector3& position)
    {
        if (m_LocalPosistion == position)
            return;

        // ���� ��尡 �ƴҽÿ��� ���ο� �ʱ� ��ġ
        if(!m_Context->m_Engine->IsEngineModeSet(EEngine_Mode::GameMode))
            m_InitPosition = position;

        m_LocalPosistion = position;
        UpdateTransform();

        // �̹� �����ӿ� �������� ��ȭ�� ����
        m_PositionChanged_this_frame = true;
    }

    void Transform::SetRotation(const Quaternion& rotation)
    {
        if (GetRotation() == rotation)
            return;

        // �θ� ������� �θ��� ȸ���� ���� �����ش�.
        SetLocalRotaion(!HasParent() ? rotation : rotation * GetParent()->GetRotation().Inverse());
    }

    void Transform::SetLocalRotaion(const Quaternion& rotation)
    {
        if (m_LocalRoation == rotation)
            return;

        // ���������� ���Ӹ�尡 �ƴҽÿ��� ���ο� �ʱ� ȸ����
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

        // �θ� �����Ұ�� �����ش�.
        SetLocalScale(!HasParent() ? scale : scale / GetParent()->GetScale());
    }

    void Transform::SetLocalScale(const Vector3& scale)
    {
        if (m_LocalScale == scale)
            return;

        // ���Ӹ�尡 �ƴ� ��� �ʱ� ������ ������ ����
        if (!m_Context->m_Engine->IsEngineModeSet(EEngine_Mode::GameMode))
            m_InitScale = scale;

        m_LocalScale = scale;

        // ���� ���� �������� 0�̶�� ���� ���� ������ �ʱ�ȭ�Ѵ�.
        m_LocalScale.x = (m_LocalScale.x == 0.0f) ? Util::EPSILON : m_LocalScale.x;
        m_LocalScale.y = (m_LocalScale.y == 0.0f) ? Util::EPSILON : m_LocalScale.y;
        m_LocalScale.z = (m_LocalScale.z == 0.0f) ? Util::EPSILON : m_LocalScale.z;

        UpdateTransform();

        m_ScaleChanged_this_frame = true;
    }

    void Transform::Translate(const Vector3& delta)
    {
        // �θ� ���� ���� �����ϰ� �����ش�.
        if (!HasParent())
            SetLocalPosition(m_LocalPosistion + delta);
        // �θ� ���� ��쿡�� �θ��� ����Ŀ� �̵����� ���� ��ŭ�� �����ش�.
        else
            SetLocalPosition(m_LocalPosistion + GetParent()->GetMatrix().Inverted() * delta);
    }

    void Transform::Rotate(const Quaternion& delta)
    {
        // �θ� ���� ���� ���ؼ� ����ȭ
        if (!HasParent())
            SetLocalRotaion((m_LocalRoation * delta).Normalized());
        // �θ� ���� ��쿡�� ����İ� ���� ȸ��ġ �׸��� ȸ������ ���Ѵ�.
        else
            SetLocalRotaion(m_LocalRoation * GetRotation().Inverse() * delta * GetRotation());
    }

    Vector3 Transform::GetUp() const
    {
        // ���� Up���͸� ���Ѵ�.
        return GetLocalRotation() * Vector3::Up;
    }

    Vector3 Transform::GetDown() const
    {
        // ���� Down���͸� ���Ѵ�.
        return GetLocalRotation() * Vector3::Down;
    }

    Vector3 Transform::GetForward() const
    {
        // ���� Forward���͸� ���Ѵ�.
        return GetLocalRotation() * Vector3::Forward;
    }

    Vector3 Transform::GetBackward() const
    {
        // ���� Backward���͸� ���Ѵ�.
        return GetLocalRotation() * Vector3::Backward;
    }

    Vector3 Transform::GetRight() const
    {
        // ���� Right���͸� ���Ѵ�.
        return GetLocalRotation() * Vector3::Right;
    }

    Vector3 Transform::GetLeft() const
    {
        // ���� Left���͸� ���Ѵ�.
        return GetLocalRotation() * Vector3::Left;
    }

    Transform* Transform::GetChildByIndex(const uint32_t index)
    {
        // ���� �ڽ��� ���� ��� ���
        if (!HasChildren())
        {
            LOG_WARNING("%s has no children.", GetEntityName().c_str());
            return nullptr;
        }

        // �ε����� �Ѿ�� ��쵵 ��������
        if (index >= GetChildrenCount())
        {
            LOG_WARNING("There is no child with an index of \"%d\".", index);
            return nullptr;
        }

        // �ڽ� ��ȯ
        return m_vecChildren[index];
    }

    Transform* Transform::GetChildByName(const string& name)
    {
        // �̸����� �ڽ��� ã�ƿ´�.
        for (const auto& child : m_vecChildren)
        {
            if (child->GetEntityName() == name)
                return child;
        }

        return nullptr;
    }

    void Transform::SetParent(Transform* new_parent)
    {
        // �ڱ��ڽ��� �θ�� ���Ѵٸ� ��ȯ
        if (new_parent)
        {
            if (GetObjectID() == new_parent->GetObjectID())
                return;
        }

        // ���� �θ� ���ο� �θ�� ������ ��ȯ
        if (m_Parent && new_parent)
        {
            if (m_Parent->GetObjectID() == new_parent->GetObjectID())
                return;
        }

        // ���� ���ο� �θ� ���� �� ������Ʈ�� �ڽ��̾��ٸ�
        if (new_parent && new_parent->IsDescendantOf(this))
        {
            // ��� �ڽ��� ��ȸ�Ѵ�.
            for (Transform* child : m_vecChildren)
            {
                // �� �ڽ��� �θ� �� ������Ʈ�� �θ�� �����Ѵ�.
                child->SetParentInternal(m_Parent);
            }

            // �ڽ� ������Ʈ Ŭ����
            m_vecChildren.clear();
        }
        
        // ���� �θ� �����ߴٸ� �� ������Ʈ�� �����Ѵ�.
        if (m_Parent)
            m_Parent->RemoveChildInternal(this);

        // ���ο� �θ� �����ҽÿ�
        if (new_parent)
        {
            // ���ο� �θ��� �ڽĿ� �� Ʈ�������� �߰��Ѵ�.
            new_parent->AddChildInternal(this);
            // ��Ƽ �÷���
            new_parent->MakeDirty();
        }

        m_Parent = new_parent;
        m_IsDirty = true;
    }


    void Transform::AddChild(Transform* child)
    {
        ASSERT(child != nullptr);

        // �ڱ� �ڽ��� �߰��Ѵٸ� ��ȯ
        if (child->GetObjectID() == GetObjectID())
            return;

        // �ڽ��� �θ� ���Ӱ� �����Ѵ�.
        child->SetParent(this);
    }

    void Transform::RemoveChild(Transform* child)
    {
        ASSERT(child != nullptr);

        // �ڱ� �ڽ��� ������ �Ұ���
        if (child->GetObjectID() == GetObjectID())
            return;

        // ���̵�� ��Ī�Ǵ� �ڽ��� �����Ѵ�.
        m_vecChildren.erase(remove_if(m_vecChildren.begin(), m_vecChildren.end(),
            [child](Transform* vec_transform) {
            return vec_transform->GetObjectID() == child->GetObjectID(); 
        }), m_vecChildren.end());

        // �׸��� �ڽ��� �θ� ���ش�.
        child->SetParent(nullptr);
    }

    void Transform::SetParentInternal(Transform* new_parent)
    {
        // ���� ���ο� �θ� �ڱ� �ڽ��̶�� �׳� ��ȯ
        if (new_parent)
        {
            if (GetObjectID() == new_parent->GetObjectID())
                return;
        }

        // �� �����߿� �ϳ��� ���̸� ��Ƽ �÷���
        if ((m_Parent && !new_parent) || (!m_Parent && new_parent))
            m_IsDirty = true;

        m_Parent = new_parent;
    }

    void Transform::AddChildInternal(Transform* child)
    {
        ASSERT(child != nullptr);

        if (child->GetObjectID() == GetObjectID())
            return;

        // ���� �ڽĿ� �̹� �ִ� �ڽ������� Ȯ���Ѵ�.
        if (!(find(m_vecChildren.begin(), m_vecChildren.end(), child) != m_vecChildren.end()))
        {
            // �����ٸ� �߰�
            m_vecChildren.emplace_back(child);
        }
    }

    void Transform::RemoveChildInternal(Transform* child)
    {
        ASSERT(child != nullptr);

        if (child->GetObjectID() == GetObjectID())
            return;

        // ����
        m_vecChildren.erase(remove_if(m_vecChildren.begin(), m_vecChildren.end(),
            [child](Transform* vec_transform) { 
            return vec_transform->GetObjectID() == child->GetObjectID(); 
        }), m_vecChildren.end());
    }

    void Transform::AcquireChildren()
    {
        // �ϴ� ����.
        m_vecChildren.clear();
        m_vecChildren.shrink_to_fit();

        // ���� ���� ��ġ�Ǿ��ִ� ��� ��ƼƼ���� �����´�.
        auto entities = GetContext()->GetSubModule<World>()->EntityGetAll();

        for (const auto& entity : entities)
        {
            if (!entity)
                continue;

            // ��ƼƼ�� Ʈ�������� �����´�.
            auto possible_child = entity->GetTransform();

            // �� Ʈ�������� �θ� ���������� �ʴٸ� �׳� �����Ѵ�.
            if (!possible_child->HasParent())
                continue;

            // �� Ʈ�������� �θ� �� Ʈ�������� ����Ų�ٸ�
            if (possible_child->GetParent()->GetObjectID() == GetObjectID())
            {
                // �߰��Ѵ�.
                m_vecChildren.emplace_back(possible_child);

                // ���Ӱ� �߰��� �ڽĵ� Ȯ���Ѵ�.
                possible_child->AcquireChildren();
            }
        }
    }

    bool Transform::IsDescendantOf(Transform* transform) const
    {
        ASSERT(transform != nullptr);

        // ���� �θ� ���ٸ� ��ȯ
        if (!m_Parent)
            return false;

        // ���� �θ��� ���̵�� �Ű������� �Ѿ�� ���̵��� ���� ���ٸ�
        if (m_Parent->GetObjectID() == transform->GetObjectID())
            return true;

        // ��� �ڽĵ��� ��ȸ�Ѵ�.
        for (Transform* child : transform->GetChildren())
        {
            if (IsDescendantOf(child))
                return true;
        }

        return false;
    }

    void Transform::GetDescendants(vector<Transform*>* descendants)
    {
        // ��� �ڽĵ��� �����Ѵ�.
        for (Transform* child : m_vecChildren)
        {
            descendants->emplace_back(child);

            if (child->HasChildren())
                child->GetDescendants(descendants);
        }
    }

    Matrix Transform::GetParentTransformMatrix() const
    {
        // �θ� ������ ��� �θ��� ����� ��ȯ�Ѵ�.
        return HasParent() ? GetParent()->GetMatrix() : Matrix::Identity;
    }
}