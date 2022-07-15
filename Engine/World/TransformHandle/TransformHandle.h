#pragma once

#include <memory>

#include <memory>
#include "TransformOperator.h"
#include "../../EngineDefinition.h"
#include <unordered_map>

namespace PlayGround
{
	class World;
	class Input;
	class Camera;
	class Context;
	class Entity;
	class RHI_IndexBuffer;
	class RHI_VertexBuffer;

	class TransformHandle
	{
	public:
		TransformHandle(Context* context);
		~TransformHandle() = default;

		bool Update(Camera* camera, float handle_size);

		std::weak_ptr<Entity> SetSelectedEntity(const std::shared_ptr<Entity>& entity);
		uint32_t GetIndexCount();
		const RHI_VertexBuffer* GetVertexBuffer();
		const RHI_IndexBuffer* GetIndexBuffer();
		const TransformOperator* GetHandle();
		inline bool DrawXYZ() const { return m_Type == TransformHandleType::Scale; }
		inline bool IsEditing() const { return m_Is_editing; }
		inline Entity* GetSelectedEntity() const { return m_SelectedEntity.lock().get(); }

	private:
		bool m_Needs_to_render = false;
		bool m_Is_editing = false;

		std::weak_ptr<Entity> m_SelectedEntity;
		std::unordered_map<TransformHandleType, std::shared_ptr<TransformOperator>> m_mapTransform_operator;
		TransformHandleType m_Type = TransformHandleType::Unknown;
		TransformHandleSpace m_Space;
		Context* m_Context = nullptr;
		Input* m_Input = nullptr;
		World* m_World = nullptr;
	};
}