#include "Common.h"
#include "IResource.h"
#include "../Audio/AudioClip.h"
#include "../Rendering/Model.h"
#include "../Rendering/Font/Font.h"
#include "../Rendering/Animation.h"
#include "../RHI/RHI_Texture2D.h"
#include "../RHI/RHI_Texture2DArray.h"
#include "../RHI/RHI_TextureCube.h"

using namespace std;
using namespace PlayGround;

IResource::IResource(Context* context, const EResourceType type)
{
	m_Context = context;
	m_ResourceType = type;
}

template <typename T>
inline constexpr EResourceType IResource::TypeToEnum()
{
	return EResourceType::Unknown;
}

// 해당 T타입이 IResource를 상속받고 있는지 확인
template <typename T>
inline constexpr void Validate_ResourceType()
{
	static_assert(std::is_base_of<IResource, T>::value, "Provided type does not implement IResource");
}

// 탬플릿 특수화
#define INSTANTIATE_TO_RESOURCE_TYPE(T, enumT) template<> EResourceType IResource::TypeToEnum<T>() { Validate_ResourceType<T>(); return enumT; }

// 탬플릿 특스화 매크로
INSTANTIATE_TO_RESOURCE_TYPE(RHI_Texture, EResourceType::Texture)
INSTANTIATE_TO_RESOURCE_TYPE(RHI_Texture2D, EResourceType::Texture2d)
INSTANTIATE_TO_RESOURCE_TYPE(RHI_Texture2DArray, EResourceType::Texture2dArray)
INSTANTIATE_TO_RESOURCE_TYPE(RHI_TextureCube, EResourceType::TextureCube)
INSTANTIATE_TO_RESOURCE_TYPE(AudioClip, EResourceType::Audio)
INSTANTIATE_TO_RESOURCE_TYPE(Material, EResourceType::Material)
INSTANTIATE_TO_RESOURCE_TYPE(Model, EResourceType::Model)
INSTANTIATE_TO_RESOURCE_TYPE(Animation, EResourceType::Animation)
INSTANTIATE_TO_RESOURCE_TYPE(Font, EResourceType::Font)
