#pragma once

#include <string>
#include "../EngineDefinition.h"

/// <summary>
/// 엔진에서 사용되는 객체들은 이 클래스를 기본으로 상속받는다.
/// </summary>
namespace PlayGround
{
	class Context;

	// GUID
	extern uint64_t GID;

	class EngineObject
	{
	public:
		EngineObject(Context* context = nullptr);

		inline const std::string& GetObjectName() const { return m_ObjectName; }

		inline void SetObjectName(const std::string& name) { m_ObjectName = name; }

		inline const uint64_t GetObjectID() const { return m_ObjectID; }

		inline void SetObjectID(const uint64_t id) { m_ObjectID = id; }

		inline static uint64_t GenerateObjectID() { return ++GID; }

		// CPU에서 오브젝트의 크기
		inline const uint64_t GetObjectSizeCPU() const { return m_ObjectSizeCPU; }

		// GPU에서 오브젝트의 크기
		inline const uint64_t GetObjectSizeGPU() const { return m_ObjectSizeGPU; }

		inline Context* GetContext() const { return m_Context; }

	protected:
		// 이름
		std::string m_ObjectName;
		// 아이디
		uint64_t m_ObjectID = 0;
		uint64_t m_ObjectSizeCPU = 0;
		uint64_t m_ObjectSizeGPU = 0;

		Context* m_Context = nullptr;
	};
}