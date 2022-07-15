#include "Common.h"
#include "Collider.h"
#include "Transform.h"
#include "RigidBody.h"
#include "Renderable.h"
#include "../Entity.h"
#include "../../IO/FileStream.h"
#include "../../Physics/BulletPhysicsHelper.h"
#include "../../RHI/RHI_Vertex.h"

#include "BulletCollision/CollisionShapes/btBoxShape.h"
#include "BulletCollision/CollisionShapes/btSphereShape.h"
#include "BulletCollision/CollisionShapes/btStaticPlaneShape.h"
#include "BulletCollision/CollisionShapes/btCylinderShape.h"
#include "BulletCollision/CollisionShapes/btCapsuleShape.h"
#include "BulletCollision/CollisionShapes/btConeShape.h"
#include "BulletCollision/CollisionShapes/btConvexHullShape.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
	Collider::Collider(Context* context, Entity* entity, uint64_t id /*= 0*/) : IComponent(context, entity, id)
	{
		// �浹ü �ʱ�ȭ
		m_ShapeType = ColliderShape_Box;
		m_Center = Vector3::Zero;
		m_Size = Vector3::One;
		m_Shape = nullptr;

		// �浹ü getter, setter
		REGISTER_ATTRIBUTE_VALUE_VALUE(m_Size, Vector3);
		REGISTER_ATTRIBUTE_VALUE_VALUE(m_Center, Vector3);
		REGISTER_ATTRIBUTE_VALUE_VALUE(m_VertexLimit, uint32_t);
		REGISTER_ATTRIBUTE_VALUE_VALUE(m_Optimize, bool);
		REGISTER_ATTRIBUTE_VALUE_SET(m_ShapeType, SetShapeType, ColliderShape);
	}

	void Collider::OnInit()
	{
		// �ʱ�ȭ�ÿ�
		auto renderable = GetEntity()->GetRenderable();
		// �߽����� �ٿ�� �ڽ��� ũ�⸦ �����Ѵ�.
		if (renderable)
		{
			m_Center = Vector3::Zero;
			m_Size = renderable->GetAabb().GetSize();
		}

		// ��� ����
		ShapeUpdate();
	}

	void Collider::OnRemove()
	{
		ShapeRelease();
	}

	void Collider::Serialize(FileStream* stream)
	{
		// ����

		stream->Write(uint32_t(m_ShapeType));
		stream->Write(m_Size);
		stream->Write(m_Center);
	}

	void Collider::Deserialize(FileStream* stream)
	{
		// �ҷ�����

		m_ShapeType = ColliderShape(stream->ReadAs<uint32_t>());
		stream->Read(&m_Size);
		stream->Read(&m_Center);

		ShapeUpdate();
	}

	void Collider::SetBoundingBox(const Vector3& boundingBox)
	{
		if (m_Size == boundingBox)
			return;

		// �ٿ�� �ڽ� ����

		m_Size = boundingBox;
		m_Size.x = Util::Clamp(Util::EPSILON, INFINITY, m_Size.x);
		m_Size.y = Util::Clamp(Util::EPSILON, INFINITY, m_Size.y);
		m_Size.z = Util::Clamp(Util::EPSILON, INFINITY, m_Size.z);

		// ��� ������Ʈ
		ShapeUpdate();
	}

	void Collider::SetCenter(const Vector3& center)
	{
		if (m_Center == center)
			return;

		m_Center = center;
		RigidBodySetCenterOfMass(m_Center);
	}

	void Collider::SetShapeType(ColliderShape type)
	{
		if (m_ShapeType == type)
			return;

		m_ShapeType = type;
		ShapeUpdate();
	}

	void Collider::SetOptimize(bool optimize)
	{
		if (m_Optimize == optimize)
			return;

		m_Optimize = optimize;
		ShapeUpdate();
	}

	void Collider::ShapeUpdate()
	{
		// ������ ����� �����Ѵ�.
		ShapeRelease();

		// ������
		const Vector3 worldScale = GetTransform()->GetScale();

		// ��� Ÿ�Կ� ���� �浹ü ��� ����
		switch (m_ShapeType)
		{
		// �ڽ��� ���
		case ColliderShape_Box:
			m_Shape = new btBoxShape(ToBtVector3(m_Size * 0.5f));
			m_Shape->setLocalScaling(ToBtVector3(worldScale));
			break;
		// ���� ���
		case ColliderShape_Sphere:
			m_Shape = new btSphereShape(m_Size.x * 0.5f);
			m_Shape->setLocalScaling(ToBtVector3(worldScale));
			break;
		// ����� ���
		case ColliderShape_StaticPlane:
			m_Shape = new btStaticPlaneShape(btVector3(0.0f, 1.0f, 0.0f), 0.0f);
			break;
		// ������� ���
		case ColliderShape_Cylinder:
			m_Shape = new btCylinderShape(btVector3(m_Size.x * 0.5f, m_Size.y * 0.5f, m_Size.x * 0.5f));
			m_Shape->setLocalScaling(ToBtVector3(worldScale));
			break;
		// ĸ���� ���
		case ColliderShape_Capsule:
			m_Shape = new btCapsuleShape(m_Size.x * 0.5f, Util::Max(m_Size.y - m_Size.x, 0.0f));
			m_Shape->setLocalScaling(ToBtVector3(worldScale));
			break;
		// ������ ���
		case ColliderShape_Cone:
			m_Shape = new btConeShape(m_Size.x * 0.5f, m_Size.y);
			m_Shape->setLocalScaling(ToBtVector3(worldScale));
			break;
		// ������ Ÿ��
		case ColliderShape_Mesh:
			// �������� ������Ʈ�� �����´�.
			Renderable* renderable = GetEntity()->GetComponent<Renderable>();
			if (!renderable)
			{
				LOG_WARNING("Can't construct mesh shape, there is no Renderable component attached.");
				return;
			}

			// ���ؽ��Ǽ��� �Ѱ�ġ�� ���� �ʴ��� üũ
			if (renderable->GeometryVertexCount() >= m_VertexLimit)
			{
				LOG_WARNING("No user defined collider with more than %d vertices is allowed.", m_VertexLimit);
				return;
			}

			// �ε����� ���ؽ� ���۸� �����´�.
			vector<uint32_t> indices;
			vector<RHI_Vertex_PosTexNorTan> vertices;
			renderable->GeometryGet(&indices, &vertices);

			if (vertices.empty())
			{
				LOG_WARNING("No vertices.");
				return;
			}

			// ������ �� ����
			m_Shape = new btConvexHullShape(
				(btScalar*)&vertices[0],                                 // ����
				renderable->GeometryVertexCount(),                       // ���� ����
				static_cast<uint32_t>(sizeof(RHI_Vertex_PosTexNorTan))); // ��Ʈ���̵�

			// �����ϸ�
			m_Shape->setLocalScaling(ToBtVector3(worldScale));

			// ����ȭ
			if (m_Optimize)
			{
				auto hull = static_cast<btConvexHullShape*>(m_Shape);
				hull->optimizeConvexHull();
				hull->initializePolyhedralFeatures();
			}
			break;
		}

		m_Shape->setUserPointer(this);

		RigidBodySetShape(m_Shape);
		RigidBodySetCenterOfMass(m_Center);
	}

	void Collider::ShapeRelease()
	{
		// ����
		RigidBodySetShape(nullptr);
		SAFE_DELETE(m_Shape);
	}

	void Collider::RigidBodySetShape(btCollisionShape* shape) const
	{
		auto rigidBody = m_Entity->GetComponent<RigidBody>();

		if(rigidBody)
			rigidBody->SetShape(shape);
	}

	void Collider::RigidBodySetCenterOfMass(const Vector3& center) const
	{
		auto rigidBody = m_Entity->GetComponent<RigidBody>();

		if (rigidBody)
			rigidBody->SetCenterOfMass(center);
	}
}
