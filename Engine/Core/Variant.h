#pragma once

#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Math/Vector4.h"
#include "../Math/Quaternion.h"
#include "../Math/Matrix.h"
#include <variant>
#include "../EngineDefinition.h"

/// <summary>
/// 엔진에서 사용되는 변수들을 Variant를 이용하여 정의한다.
/// Variant를 이용하여 저장, 불러오기를 간편하게 만들고 이벤트의 경우에도 Variant를 이용하여 다양한 인자를 받는 콜백 함수를 만들 수 있다.
/// </summary>

namespace PlayGround
{
	class Entity;
}

typedef union SDL_Event SDL_Event;

// Variant 타입들 엔진에서 자주 사용되는 변수들
#define _VARIANT_TYPES                             \
    char,                                          \
    unsigned char,                                 \
    int,                                           \
    uint32_t,                                      \
    bool,                                          \
    float,                                         \
    double,                                        \
    void*,                                         \
    PlayGround::Entity*,                              \
    std::shared_ptr<PlayGround::Entity>,              \
    std::weak_ptr<PlayGround::Entity>,                \
    std::vector<std::weak_ptr<PlayGround::Entity>>,   \
    std::vector<std::shared_ptr<PlayGround::Entity>>, \
    PlayGround::Math::Vector2,                        \
    PlayGround::Math::Vector3,                        \
    PlayGround::Math::Vector4,                        \
    PlayGround::Math::Matrix,                         \
    PlayGround::Math::Quaternion,                     \
    SDL_Event*

#define VARIANT_TYPES std::variant<_VARIANT_TYPES>
typedef std::variant<_VARIANT_TYPES, VARIANT_TYPES> StdVariant;

namespace PlayGround
{
	class Variant
	{
	public:
		Variant() = default;
		~Variant() = default;

		Variant(const Variant& var)
		{
			m_Variant = var.GetVariantRaw();
		}

		// 만약 T타입이 Varaint타입과 다르다면 생성자 오버로딩을 한다.
		template <typename T, typename = std::enable_if<!std::is_same<T, Variant>::value>>
		Variant(T value)
		{
			// m_Variant는 int, char 등을 담을 수 있는 변수
			m_Variant = value;
		}

		Variant& operator=(const Variant& rhs);

		// 만약 T타입이 Varaint타입과 다르다면 생성자 오버로딩을 한다.
		template <typename T, typename = std::enable_if<!std::is_same<T, Variant>::value>>
		Variant& operator=(T rhs)
		{
			return m_Variant = rhs;
		}

		const StdVariant& GetVariantRaw() const
		{
			return m_Variant;
		}

		template <typename T>
		inline const T& Get() const
		{
			return std::get<T>(m_Variant);
		}

	private:
		StdVariant m_Variant;
	};
}