#include "Common.h"
#include "TransformHandle.h"
#include "../../RHI/RHI_IndexBuffer.h"
#include "../../Input/Input.h"
#include "../../World/World.h"
#include "../../World/Entity.h"
#include "../../World/Components/Camera.h"
#include "../../World/Components/Transform.h"
#include "TransformPosition.h"
#include "TransformScale.h"
#include "TransformRotation.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
	TransformHandle::TransformHandle(Context* context)
	{
		m_Context = context;
		m_Input = context->GetSubModule<Input>();
		m_World = context->GetSubModule<World>();
		m_Type = TransformHandleType::Position;
		m_Space = TransformHandleSpace::World;
		m_Is_editing = false;

		m_mapTransform_operator[TransformHandleType::Position] = make_shared<TransformPosition>(context);
		m_mapTransform_operator[TransformHandleType::Rotation] = make_shared<TransformRotation>(context);
		m_mapTransform_operator[TransformHandleType::Scale] = make_shared<TransformScale>(context);
	}

	bool TransformHandle::Update(Camera* camera, const float handle_size)
	{
		shared_ptr<Entity> selected_entity = m_SelectedEntity.lock();

		if (!camera || !selected_entity)
		{
			m_Is_editing = false;
			return false;
		}

		if (selected_entity->GetObjectID() == camera->GetTransform()->GetEntity()->GetObjectID())
		{
			m_Is_editing = false;
			return false;
		}

		if (!camera->IsFpsControlled())
		{
			if (m_Input->GetKeyDown(EKeyCode::W))
				m_Type = TransformHandleType::Position;
			else if (m_Input->GetKeyDown(EKeyCode::E))
				m_Type = TransformHandleType::Rotation;
			else if (m_Input->GetKeyDown(EKeyCode::R))
				m_Type = TransformHandleType::Scale;
		}

		m_mapTransform_operator[m_Type]->Update(m_Space, selected_entity.get(), camera, handle_size);

		m_Is_editing = m_mapTransform_operator[m_Type]->IsEditing();

		return true;
	}

	weak_ptr<Entity> TransformHandle::SetSelectedEntity(const shared_ptr<Entity>& entity)
	{
		if (!m_Is_editing)
		{
			if (!m_mapTransform_operator[m_Type]->IsHovered())
				m_SelectedEntity = entity;
		}

		return m_SelectedEntity;
	}

	uint32_t TransformHandle::GetIndexCount()
	{
		return m_mapTransform_operator[m_Type]->GetIndexBuffer()->GetIndexCount();
	}

	const RHI_VertexBuffer* TransformHandle::GetVertexBuffer()
	{
		return m_mapTransform_operator[m_Type]->GetVertexBuffer();
	}

	const RHI_IndexBuffer* TransformHandle::GetIndexBuffer()
	{
		return m_mapTransform_operator[m_Type]->GetIndexBuffer();
	}

	const TransformOperator* TransformHandle::GetHandle()
	{
		return m_mapTransform_operator[m_Type].get();
	}
}