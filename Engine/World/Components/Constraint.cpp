#include "Common.h"
#include "Constraint.h"
#include "RigidBody.h"
#include "Transform.h"
#include "../Entity.h"
#include "../World.h"
#include "../../IO/FileStream.h"
#include "../../Physics/Physics.h"
#include "../../Physics/BulletPhysicsHelper.h"
#include "../../Core/Context.h"
#include "../World.h"


#include "BulletDynamics/ConstraintSolver/btConeTwistConstraint.h"
#include "BulletDynamics/ConstraintSolver/btSliderConstraint.h"
#include "BulletDynamics/ConstraintSolver/btHingeConstraint.h"
#include "BulletDynamics/ConstraintSolver/btPoint2PointConstraint.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
	Constraint::Constraint(Context* context, Entity* entity, uint64_t id /*= 0*/) : IComponent(context, entity, id)
	{
		// 컨스트레인트 초기화
		m_Constraint = nullptr;
		m_EnabledEffective = true;
		m_CollisionWithLinkedBody = false;
		m_ErrorReduction = 0.0f;
		m_ConstraintForceMixing = 0.0f;
		m_ConstraintType = ConstraintType_Point;
		m_Physics = GetContext()->GetSubModule<Physics>();

		// 컨스트레인트 값 getter, setter 설정
		REGISTER_ATTRIBUTE_VALUE_VALUE(m_ErrorReduction, float);
		REGISTER_ATTRIBUTE_VALUE_VALUE(m_ConstraintForceMixing, float);
		REGISTER_ATTRIBUTE_VALUE_VALUE(m_EnabledEffective, bool);
		REGISTER_ATTRIBUTE_VALUE_VALUE(m_CollisionWithLinkedBody, bool);
		REGISTER_ATTRIBUTE_VALUE_VALUE(m_Position, Vector3);
		REGISTER_ATTRIBUTE_VALUE_VALUE(m_Rotation, Quaternion);
		REGISTER_ATTRIBUTE_VALUE_VALUE(m_HighLimit, Vector2);
		REGISTER_ATTRIBUTE_VALUE_VALUE(m_LowLimit, Vector2);
		REGISTER_ATTRIBUTE_VALUE_SET(m_ConstraintType, SetConstraintType, ConstraintType);
	}

	Constraint::~Constraint()
	{
		ReleaseConstraint();
	}

	void Constraint::OnInit()
	{

	}

	void Constraint::OnStart()
	{

	}

	void Constraint::OnStop()
	{

	}

	void Constraint::OnRemove()
	{
		ReleaseConstraint();
	}

	void Constraint::Update(double delta_time)
	{
		if (m_DeferredConstruction)
		{
			Construct();
		}
	}

	void Constraint::Serialize(FileStream* stream)
	{
		// 저장

		stream->Write(static_cast<uint32_t>(m_ConstraintType));
		stream->Write(m_Position);
		stream->Write(m_Rotation);
		stream->Write(m_HighLimit);
		stream->Write(m_LowLimit);
		stream->Write(!m_BodyOther.expired() ? m_BodyOther.lock()->GetObjectID() : static_cast<uint32_t>(0));
	}

	void Constraint::Deserialize(FileStream* stream)
	{
		// 불러오기
		
		uint32_t constraint_type = 0;
		stream->Read(&constraint_type);
		m_ConstraintType = static_cast<ConstraintType>(constraint_type);
		stream->Read(&m_Position);
		stream->Read(&m_Rotation);
		stream->Read(&m_HighLimit);
		stream->Read(&m_LowLimit);

		const auto body_other_id = stream->ReadAs<uint32_t>();
		m_BodyOther = GetContext()->GetSubModule<World>()->EntityGetByID(body_other_id);

		Construct();
	}

	void Constraint::SetConstraintType(const ConstraintType type)
	{
		if (m_ConstraintType != type || !m_Constraint)
		{
			m_ConstraintType = type;
			Construct();
		}
	}

	void Constraint::SetPosition(const Vector3& position)
	{
		if (m_Position != position)
		{
			m_Position = position;
			ApplyFrames();
		}
	}

	void Constraint::SetRotation(const Quaternion& rotation)
	{
		if (m_Rotation != rotation)
		{
			m_Rotation = rotation;
			ApplyFrames();
		}
	}

	void Constraint::SetPositionOther(const Vector3& position)
	{
		if (m_PositionOther != position)
		{
			m_PositionOther = position;
			ApplyFrames();
		}
	}

	void Constraint::SetRotationOther(const Quaternion& rotation)
	{
		if (rotation != m_RotationOther)
		{
			m_RotationOther = rotation;
			ApplyFrames();
		}
	}

	void Constraint::SetBodyOther(const std::weak_ptr<Entity>& body_other)
	{
		// 컨스트레인트 타겟으로 설정될 엔티티가 이미 존재하지 않는다면
		// 반환
		if (body_other.expired())
			return;

		// 자기자신을 연결은 불가능
		if (!body_other.expired() && body_other.lock()->GetObjectID() == m_Entity->GetObjectID())
		{
			LOG_WARNING("You can't connect a body to itself.");
			return;
		}

		m_BodyOther = body_other;
		Construct();
	}

	void Constraint::SetHighLimit(const Vector2& limit)
	{
		if (m_HighLimit != limit)
		{
			m_HighLimit = limit;
			ApplyLimits();
		}
	}

	void Constraint::SetLowLimit(const Vector2& limit)
	{
		if (m_LowLimit != limit)
		{
			m_LowLimit = limit;
			ApplyLimits();
		}
	}

	void Constraint::ReleaseConstraint()
	{
		// 컨스트레인트 해제
		if (m_Constraint)
		{
			// 자기자신과 연결된 엔티티를 가져온다.
			RigidBody* rigid_body_own = m_Entity->GetComponent<RigidBody>();
			RigidBody* rigid_body_other = !m_BodyOther.expired() ? m_BodyOther.lock()->GetComponent<RigidBody>() : nullptr;

			// 서로에게서 삭제를한다.
			if (rigid_body_own)    
				rigid_body_own->RemoveConstraint(this);
			if (rigid_body_other) 
				rigid_body_other->RemoveConstraint(this);

			// 컨스트레인트 제거
			m_Physics->RemoveConstraint(m_Constraint);
		}
	}

	void Constraint::ApplyFrames() const
	{
		if (!m_Constraint || m_BodyOther.expired())
			return;
		
		// 자기 자신과 연결된 엔티티를 가져온다.
		RigidBody* rigid_body_own = m_Entity->GetComponent<RigidBody>();
		RigidBody* rigid_body_other = !m_BodyOther.expired() ? m_BodyOther.lock()->GetComponent<RigidBody>() : nullptr;
		btRigidBody* bt_own_body = rigid_body_own ? rigid_body_own->GetBtRigidBody() : nullptr;
		btRigidBody* bt_other_body = rigid_body_other ? rigid_body_other->GetBtRigidBody() : nullptr;

		// 스케일 적용
		Vector3 own_body_scaled_position = m_Position * m_Transform->GetScale() - rigid_body_own->GetCenterOfMass();
		Vector3 other_body_scaled_position = !m_BodyOther.expired() ? m_PositionOther * rigid_body_other->GetTransform()->GetScale() - rigid_body_other->GetCenterOfMass() : m_PositionOther;

		// 컨스트레인트 타입에 맞는 효과를 적용한다.
		switch (m_Constraint->getConstraintType())
		{
		case POINT2POINT_CONSTRAINT_TYPE:
		{
			auto* point_constraint = dynamic_cast<btPoint2PointConstraint*>(m_Constraint);
			point_constraint->setPivotA(ToBtVector3(own_body_scaled_position));
			point_constraint->setPivotB(ToBtVector3(other_body_scaled_position));
		}
		break;

		case HINGE_CONSTRAINT_TYPE:
		{
			auto* hinge_constraint = dynamic_cast<btHingeConstraint*>(m_Constraint);
			btTransform own_frame(ToBtQuaternion(m_Rotation), ToBtVector3(own_body_scaled_position));
			btTransform other_frame(ToBtQuaternion(m_RotationOther), ToBtVector3(other_body_scaled_position));
			hinge_constraint->setFrames(own_frame, other_frame);
		}
		break;

		case SLIDER_CONSTRAINT_TYPE:
		{
			auto* slider_constraint = dynamic_cast<btSliderConstraint*>(m_Constraint);
			btTransform own_frame(ToBtQuaternion(m_Rotation), ToBtVector3(own_body_scaled_position));
			btTransform other_frame(ToBtQuaternion(m_RotationOther), ToBtVector3(other_body_scaled_position));
			slider_constraint->setFrames(own_frame, other_frame);
		}
		break;

		case CONETWIST_CONSTRAINT_TYPE:
		{
			auto* cone_twist_constraint = dynamic_cast<btConeTwistConstraint*>(m_Constraint);
			btTransform own_frame(ToBtQuaternion(m_Rotation), ToBtVector3(own_body_scaled_position));
			btTransform other_frame(ToBtQuaternion(m_RotationOther), ToBtVector3(other_body_scaled_position));
			cone_twist_constraint->setFrames(own_frame, other_frame);
		}
		break;

		default:
			break;
		}
	}

	void Constraint::Construct()
	{
		// 기존에 있던 컨스트레인트를 제거
		ReleaseConstraint();

		// 자기자신과 연결될 엔티티가 있는지 확인
		RigidBody* rigid_body_own = m_Entity->GetComponent<RigidBody>();
		RigidBody* rigid_body_other = !m_BodyOther.expired() ? m_BodyOther.lock()->GetComponent<RigidBody>() : nullptr;

		if (!rigid_body_own || !rigid_body_other)
		{
			LOG_INFO("A RigidBody component is still initializing, deferring construction...");
			m_DeferredConstruction = true;
			return;
		}
		else if (m_DeferredConstruction)
		{
			LOG_INFO("Deferred construction has succeeded");
			m_DeferredConstruction = false;
		}

		btRigidBody* bt_own_body = rigid_body_own ? rigid_body_own->GetBtRigidBody() : nullptr;
		btRigidBody* bt_other_body = rigid_body_other ? rigid_body_other->GetBtRigidBody() : nullptr;

		if (!bt_own_body)
			return;

		if (!bt_other_body)
		{
			bt_other_body = &btTypedConstraint::getFixedBody();
		}

		Vector3 own_body_scaled_position = m_Position * m_Transform->GetScale() - rigid_body_own->GetCenterOfMass();
		Vector3 other_body_scaled_position = rigid_body_other ? m_PositionOther * rigid_body_other->GetTransform()->GetScale() - rigid_body_other->GetCenterOfMass() : m_PositionOther;

		switch (m_ConstraintType)
		{
		case ConstraintType_Point:
		{
			m_Constraint = new btPoint2PointConstraint(*bt_own_body, *bt_other_body, ToBtVector3(own_body_scaled_position), ToBtVector3(other_body_scaled_position));
		}
		break;

		case ConstraintType_Hinge:
		{
			btTransform own_frame(ToBtQuaternion(m_Rotation), ToBtVector3(own_body_scaled_position));
			btTransform other_frame(ToBtQuaternion(m_RotationOther), ToBtVector3(other_body_scaled_position));
			m_Constraint = new btHingeConstraint(*bt_own_body, *bt_other_body, own_frame, other_frame);
		}
		break;

		case ConstraintType_Slider:
		{
			btTransform own_frame(ToBtQuaternion(m_Rotation), ToBtVector3(own_body_scaled_position));
			btTransform other_frame(ToBtQuaternion(m_RotationOther), ToBtVector3(other_body_scaled_position));
			m_Constraint = new btSliderConstraint(*bt_own_body, *bt_other_body, own_frame, other_frame, false);
		}
		break;

		case ConstraintType_ConeTwist:
		{
			btTransform own_frame(ToBtQuaternion(m_Rotation), ToBtVector3(own_body_scaled_position));
			btTransform other_frame(ToBtQuaternion(m_RotationOther), ToBtVector3(other_body_scaled_position));
			m_Constraint = new btConeTwistConstraint(*bt_own_body, *bt_other_body, own_frame, other_frame);
		}
		break;

		default:
			break;
		}

		if (m_Constraint)
		{
			m_Constraint->setUserConstraintPtr(this);
			m_Constraint->setEnabled(m_EnabledEffective);

			// 서로 추가한다.
			rigid_body_own->AddConstraint(this);
			if (rigid_body_other)
			{
				rigid_body_other->AddConstraint(this);
			}

			ApplyLimits();
			m_Physics->AddConstraint(m_Constraint, m_CollisionWithLinkedBody);
		}
	}

	void Constraint::ApplyLimits() const
	{
		if (!m_Constraint)
			return;

		switch (m_Constraint->getConstraintType())
		{
		case HINGE_CONSTRAINT_TYPE:
		{
			auto* hinge_constraint = dynamic_cast<btHingeConstraint*>(m_Constraint);
			hinge_constraint->setLimit(m_LowLimit.x * Util::DEG_TO_RAD, m_HighLimit.x * Util::DEG_TO_RAD);
		}
		break;

		case SLIDER_CONSTRAINT_TYPE:
		{
			auto* slider_constraint = dynamic_cast<btSliderConstraint*>(m_Constraint);
			slider_constraint->setUpperLinLimit(m_HighLimit.x);
			slider_constraint->setUpperAngLimit(m_HighLimit.y * Util::DEG_TO_RAD);
			slider_constraint->setLowerLinLimit(m_LowLimit.x);
			slider_constraint->setLowerAngLimit(m_LowLimit.y * Util::DEG_TO_RAD);
		}
		break;

		case CONETWIST_CONSTRAINT_TYPE:
		{
			auto* cone_twist_constraint = dynamic_cast<btConeTwistConstraint*>(m_Constraint);
			cone_twist_constraint->setLimit(m_HighLimit.y * Util::DEG_TO_RAD, m_HighLimit.y * Util::DEG_TO_RAD, m_HighLimit.x * Util::DEG_TO_RAD);
		}
		break;

		default:
			break;
		}

		if (m_ErrorReduction != 0.0f)
		{
			m_Constraint->setParam(BT_CONSTRAINT_STOP_ERP, m_ErrorReduction);
		}

		if (m_ConstraintForceMixing != 0.0f)
		{
			m_Constraint->setParam(BT_CONSTRAINT_STOP_CFM, m_ConstraintForceMixing);
		}
	}
}