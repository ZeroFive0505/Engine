#pragma once

#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Math/Vector4.h"
#include "../Math/Quaternion.h"
#include "../Math/Matrix.h"
#include <variant>
#include "../EngineDefinition.h"

/// <summary>
/// �������� ���Ǵ� �������� Variant�� �̿��Ͽ� �����Ѵ�.
/// Variant�� �̿��Ͽ� ����, �ҷ����⸦ �����ϰ� ����� �̺�Ʈ�� ��쿡�� Variant�� �̿��Ͽ� �پ��� ���ڸ� �޴� �ݹ� �Լ��� ���� �� �ִ�.
/// </summary>

namespace PlayGround
{
	class Entity;
}

typedef union SDL_Event SDL_Event;

// Variant Ÿ�Ե� �������� ���� ���Ǵ� ������
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

		// ���� TŸ���� VaraintŸ�԰� �ٸ��ٸ� ������ �����ε��� �Ѵ�.
		template <typename T, typename = std::enable_if<!std::is_same<T, Variant>::value>>
		Variant(T value)
		{
			// m_Variant�� int, char ���� ���� �� �ִ� ����
			m_Variant = value;
		}

		Variant& operator=(const Variant& rhs);

		// ���� TŸ���� VaraintŸ�԰� �ٸ��ٸ� ������ �����ε��� �Ѵ�.
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