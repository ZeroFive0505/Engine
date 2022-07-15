#include "Common.h"
#include "Material.h"
#include "Renderer.h"
#include "../Resource/ResourceCache.h"
#include "../IO/XmlDocument.h"
#include "../RHI/RHI_Texture2D.h"
#include "../RHI/RHI_TextureCube.h"
#include "../World/World.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
	Material::Material(Context* context) : IResource(context, EResourceType::Material)
	{
		m_RhiDevice = context->GetSubModule<Renderer>()->GetRhiDevice();

        SetProperty(Material_Roughness, 0.9f);
        SetProperty(Material_Metallic, 0.0f);
        SetProperty(Material_Normal, 0.0f);
        SetProperty(Material_Height, 0.0f);
        SetProperty(Material_Clearcoat, 0.0f);
        SetProperty(Material_Clearcoat_Roughness, 0.0f);
        SetProperty(Material_Anisotropic, 0.0f);
        SetProperty(Material_Anisotropic_Rotation, 0.0f);
        SetProperty(Material_Sheen, 0.0f);
        SetProperty(Material_Sheen_Tint, 0.0f);
	}

    bool Material::LoadFromFile(const string& file_path)
    {
        auto xml = make_unique<XmlDocument>();

        if (!xml->Load(file_path))
            return false;

        SetResourceFilePath(file_path);

        xml->GetAttribute("Material", "Color", &m_ColorAlbedo);
        xml->GetAttribute("Material", "Roughness_Multiplier", &GetProperty(Material_Roughness));
        xml->GetAttribute("Material", "Metallic_Multiplier", &GetProperty(Material_Metallic));
        xml->GetAttribute("Material", "Normal_Multiplier", &GetProperty(Material_Normal));
        xml->GetAttribute("Material", "Height_Multiplier", &GetProperty(Material_Height));
        xml->GetAttribute("Material", "Clearcoat_Multiplier", &GetProperty(Material_Clearcoat));
        xml->GetAttribute("Material", "Clearcoat_Roughness_Multiplier", &GetProperty(Material_Clearcoat_Roughness));
        xml->GetAttribute("Material", "Anisotropi_Multiplier", &GetProperty(Material_Anisotropic));
        xml->GetAttribute("Material", "Anisotropic_Rotatio_Multiplier", &GetProperty(Material_Anisotropic_Rotation));
        xml->GetAttribute("Material", "Sheen_Multiplier", &GetProperty(Material_Sheen));
        xml->GetAttribute("Material", "Sheen_Tint_Multiplier", &GetProperty(Material_Sheen_Tint));
        xml->GetAttribute("Material", "IsEditable", &m_IsEditable);
        xml->GetAttribute("Material", "UV_Tiling", &m_UVTiling);
        xml->GetAttribute("Material", "UV_Offset", &m_UVOffset);

        const int texture_count = xml->GetAttributeAs<int>("Textures", "Count");

        for (int i = 0; i < texture_count; i++)
        {
            string node_name = "Texture_" + to_string(i);
            const Material_Property tex_type = static_cast<Material_Property>(xml->GetAttributeAs<uint32_t>(node_name, "Texture_Type"));
            string tex_name = xml->GetAttributeAs<string>(node_name, "Texture_Name");
            string tex_path = xml->GetAttributeAs<string>(node_name, "Texture_Path");

            auto texture = m_Context->GetSubModule<ResourceCache>()->GetByName<RHI_Texture2D>(tex_name);

            if (!texture)
                texture = m_Context->GetSubModule<ResourceCache>()->Load<RHI_Texture2D>(tex_path);

            SetTextureSlot(tex_type, texture, GetProperty(tex_type));
        }

        m_ObjectSizeCPU = sizeof(*this);

        return true;
    }

    bool Material::SaveToFile(const string& file_path)
    {
        SetResourceFilePath(file_path);

        auto xml = make_unique<XmlDocument>();
        xml->AddNode("Material");
        xml->AddAttribute("Material", "Color", m_ColorAlbedo);
        xml->AddAttribute("Material", "Roughness_Multiplier", GetProperty(Material_Roughness));
        xml->AddAttribute("Material", "Metallic_Multiplier", GetProperty(Material_Metallic));
        xml->AddAttribute("Material", "Normal_Multiplier", GetProperty(Material_Normal));
        xml->AddAttribute("Material", "Height_Multiplier", GetProperty(Material_Height));
        xml->AddAttribute("Material", "Clearcoat_Multiplier", GetProperty(Material_Clearcoat));
        xml->AddAttribute("Material", "Clearcoat_Roughness_Multiplier", GetProperty(Material_Clearcoat_Roughness));
        xml->AddAttribute("Material", "Anisotropi_Multiplier", GetProperty(Material_Anisotropic));
        xml->AddAttribute("Material", "Anisotropic_Rotatio_Multiplier", GetProperty(Material_Anisotropic_Rotation));
        xml->AddAttribute("Material", "Sheen_Multiplier", GetProperty(Material_Sheen));
        xml->AddAttribute("Material", "Sheen_Tint_Multiplier", GetProperty(Material_Sheen_Tint));
        xml->AddAttribute("Material", "UV_Tiling", m_UVTiling);
        xml->AddAttribute("Material", "UV_Offset", m_UVOffset);
        xml->AddAttribute("Material", "IsEditable", m_IsEditable);

        xml->AddChildNode("Material", "Textures");
        xml->AddAttribute("Textures", "Count", static_cast<uint32_t>(m_mapTextures.size()));

        int i = 0; 

        for (const auto& texture : m_mapTextures)
        {
            string tex_node = "Texture_" + to_string(i);
            xml->AddChildNode("Textures", tex_node);
            xml->AddAttribute(tex_node, "Texture_Type", static_cast<uint32_t>(texture.first));
            xml->AddAttribute(tex_node, "Texture_Name", texture.second ? texture.second->GetResourceName() : "");
            xml->AddAttribute(tex_node, "Texture_Path", texture.second ? texture.second->GetResourceFilePathNative() : "");
            i++;
        }

        return xml->Save(GetResourceFilePathNative());
    }

    void Material::SetTextureSlot(const Material_Property type, const shared_ptr<RHI_Texture>& texture, float multiplier /*= 1.0f*/)
    {
        if (texture)
        {
            const shared_ptr<RHI_Texture> texture_cached = m_Context->GetSubModule<ResourceCache>()->Cache(texture);
            m_mapTextures[type] = texture_cached != nullptr ? texture_cached : texture;
            m_Flags |= type;

            SetProperty(type, multiplier);
        }
        else
        {
            m_mapTextures.erase(type);
            m_Flags &= ~type;
        }
    }

    void Material::SetTextureSlot(const Material_Property type, const std::shared_ptr<RHI_Texture2D>& texture)
    {
        SetTextureSlot(type, static_pointer_cast<RHI_Texture>(texture));
    }

    void Material::SetTextureSlot(const Material_Property type, const std::shared_ptr<RHI_TextureCube>& texture)
    {
        SetTextureSlot(type, static_pointer_cast<RHI_Texture>(texture));
    }

    bool Material::HasTexture(const string& path) const
    {
        for (const auto& texture : m_mapTextures)
        {
            if (!texture.second)
                continue;

            if (texture.second->GetResourceFilePathNative() == path)
                return true;
        }

        return false;
    }

    string Material::GetTexturePathByType(const Material_Property type)
    {
        if (!HasTexture(type))
            return "";

        return m_mapTextures.at(type)->GetResourceFilePathNative();
    }

    vector<string> Material::GetTexturePaths()
    {
        vector<string> paths;

        for (const auto& texture : m_mapTextures)
        {
            if (!texture.second)
                continue;

            paths.emplace_back(texture.second->GetResourceFilePathNative());
        }

        return paths;
    }

    shared_ptr<RHI_Texture>& Material::GetTextureSharedPtr(const Material_Property type)
    {
        static shared_ptr<RHI_Texture> texture_empty;
        return HasTexture(type) ? m_mapTextures.at(type) : texture_empty;
    }

    void Material::SetColorAlbedo(const Math::Vector4& color)
    {
        // 투명, 불투명여부가 바뀌면 월드 업데이트
        if ((m_ColorAlbedo.w != 1.0f && color.w == 1.0f) ||
            (m_ColorAlbedo.w == 1.0f && color.w != 1.0f))
            m_Context->GetSubModule<World>()->Resolve();

        m_ColorAlbedo = color;
    }
}