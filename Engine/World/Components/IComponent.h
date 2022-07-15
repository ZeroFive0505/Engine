#pragma once

#include <memory>
#include <string>
#include <any>
#include <vector>
#include <functional>
#include "../../Core/EngineObject.h"

namespace PlayGround
{
	class Entity;
	class Transform;
	class Context;
	class FileStream;


    // 컴포넌트 타입
	enum class EComponentType : uint32_t
	{
        AudioListener,
        AudioSource,
        Camera,
        Collider,
        Constraint,
        Light,
        Renderable,
        RigidBody,
        SoftBody,
        Environment,
        Transform,
        Terrain,
        ReflectionProbe,
        Unknown
	};

    // 컴포넌트 속성 Getter, Setter
    struct sAttribute
    {
        std::function<std::any()> getter;
        std::function<void(std::any)> setter;
    };

    // IComponent는 모든 component의 기본 인터페이스 클래스
    class IComponent : public EngineObject, public std::enable_shared_from_this<IComponent>
    {
    public:
        IComponent(Context* context, Entity* entity, uint64_t id = 0, Transform* transform = nullptr);
        virtual ~IComponent() = default;

        virtual void OnInit() {}

        virtual void OnStart() {}

        virtual void OnStop() {}

        virtual void OnRemove() {}

        virtual void Update(double delta_time) {}

        virtual void Serialize(FileStream* stream) {}

        virtual void Deserialize(FileStream* stream) {}

        template <typename T>
        static constexpr EComponentType TypeToEnum();

        inline Transform* GetTransform() const { return m_Transform; }

        inline Context* GetContext() const { return m_Context; }

        inline EComponentType GetComponentType() const { return m_Type; }

        inline void SetComponentType(EComponentType type) { m_Type = type; }

        template <typename T>
        std::shared_ptr<T> GetSharedPtr() { return std::dynamic_pointer_cast<T>(shared_from_this()); }

        inline const std::vector<sAttribute>& GetAttributes() const { return m_vecAttributes; }

        inline void SetAttributes(const std::vector<sAttribute>& attributes)
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(m_vecAttributes.size()); i++)
            {
                m_vecAttributes[i].setter(attributes[i].getter());
            }
        }

        inline Entity* GetEntity() const { return m_Entity; }

        std::string GetEntityName() const;

    protected:
        // Getter, Setter 매크로

        // type 변수의 getter, setter를 만들어낸다. getter의 경우 따로 콜백함수를 설정하여 호출하게 한다.
        #define REGISTER_ATTRIBUTE_GET_SET(getter, setter, type) RegisterAttribute(  \
        [this]()                        { return getter(); },                           \
        [this](const std::any& valueIn) { setter(std::any_cast<type>(valueIn)); });     \

        // type 변수의 setter 함수를 만들어낸다.
        #define REGISTER_ATTRIBUTE_VALUE_SET(value, setter, type) RegisterAttribute( \
        [this]()                        { return value; },                              \
        [this](const std::any& valueIn) { setter(std::any_cast<type>(valueIn)); });     \
        
        // type 변수의 setter 생성
        #define REGISTER_ATTRIBUTE_VALUE_VALUE(value, type) RegisterAttribute(       \
        [this]()                        { return value; },                              \
        [this](const std::any& valueIn) { value = std::any_cast<type>(valueIn); });     \


        // 위 매크로에서 이 함수를 이용하여 Getter, Setter를 생성
        void RegisterAttribute(std::function<std::any()>&& getter, std::function<void(std::any)>&& setter)
        {
            sAttribute attributes;
            // getter, setter 콜백 함수를 설정한다.
            attributes.getter = std::move(getter);
            attributes.setter = std::move(setter);
            // 그리고 저장
            m_vecAttributes.emplace_back(attributes);
        }

        EComponentType m_Type = EComponentType::Unknown;
        bool m_Enabled = false;
        Entity* m_Entity = nullptr;
        Transform* m_Transform = nullptr;
    private:
        std::vector<sAttribute> m_vecAttributes;
    };
}

