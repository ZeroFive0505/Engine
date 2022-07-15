#pragma once

#include <memory>
#include <unordered_map>
#include "../RHI/RHI_Definition.h"
#include "../Resource/IResource.h"
#include "../Math/Vector2.h"
#include "../Math/Vector4.h"

namespace PlayGround
{
    // ���׸��� ����
    enum Material_Property : uint32_t
    {
        Material_Unknown = 0U << 0,
        Material_Clearcoat = 1U << 0,   // Ŭ���� ��Ʈ
        Material_Clearcoat_Roughness = 1U << 1,   // ��ĥ�Ⱑ �����ϴ� Ŭ���� ��Ʈ
        Material_Anisotropic = 1U << 2,   // �ݻ�� ���漺�� ��
        Material_Anisotropic_Rotation = 1U << 3,   // ���漺 ȸ�� 1.0 ���� ȸ��
        Material_Sheen = 1U << 4,   // ���ڶ� �ݻ�
        Material_Sheen_Tint = 1U << 5,   // ���ڶ� �ݻ�ƾƮ
        Material_Color = 1U << 6,   // ���׸��� �÷�
        Material_Roughness = 1U << 7,   // ���� ��ĥ��
        Material_Metallic = 1U << 8,   // �ݼ� ����
        Material_Normal = 1U << 9,   // ����
        Material_Height = 1U << 10,  // ����
        Material_Occlusion = 1U << 11,  // SSAO
        Material_Emission = 1U << 12,  // �߱�
        Material_AlphaMask = 1U << 13   // ���� ����ŷ
    };

    // ���׸��� IResource �������̽� Ŭ������ ��ӹ޴´�.
	class Material : public IResource
	{
    public:
        Material(Context* context);
        ~Material() = default;

        // IResource Ŭ���� �����Լ� �������̵�
        bool LoadFromFile(const std::string& file_path) override;
        bool SaveToFile(const std::string& file_path) override;

        // �ؽ��� ����
        void SetTextureSlot(const Material_Property type, const std::shared_ptr<RHI_Texture>& texture, float multiplier = 1.0f);
        void SetTextureSlot(const Material_Property type, const std::shared_ptr<RHI_Texture2D>& texture);
        void SetTextureSlot(const Material_Property type, const std::shared_ptr<RHI_TextureCube>& texture);
        bool HasTexture(const std::string& path) const;
        inline bool HasTexture(const Material_Property type) const { return m_Flags & type; }

        std::string GetTexturePathByType(Material_Property type);
        std::vector<std::string> GetTexturePaths();
        inline RHI_Texture* GetTexturePtr(const Material_Property type) { return HasTexture(type) ? m_mapTextures[type].get() : nullptr; }

        std::shared_ptr<RHI_Texture>& GetTextureSharedPtr(const Material_Property type);

        inline const Math::Vector4& GetColorAlbedo() const { return m_ColorAlbedo; }

        void SetColorAlbedo(const Math::Vector4& color);

        inline const Math::Vector2& GetTiling() const { return m_UVTiling; }

        inline void SetTiling(const Math::Vector2& tiling) { m_UVTiling = tiling; }

        inline const Math::Vector2& GetOffset() const { return m_UVOffset; }

        inline void SetOffset(const Math::Vector2& offset) { m_UVOffset = offset; }

        inline bool IsEditable() const { return m_IsEditable; }

        inline void SetEditable(const bool editable) { m_IsEditable = editable; }

        inline auto& GetProperty(const Material_Property type) { return m_mapProperties[type]; }
        
        inline void SetProperty(const Material_Property type, const float value) { m_mapProperties[type] = value; }

        inline uint32_t GetFlags() const { return m_Flags; }

    private:
        Math::Vector4 m_ColorAlbedo = Math::Vector4::One;
        Math::Vector2 m_UVTiling = Math::Vector2::One;
        Math::Vector2 m_UVOffset = Math::Vector2::Zero;
        bool m_IsEditable = true;
        uint32_t m_Flags = 0;
        std::unordered_map<Material_Property, std::shared_ptr<RHI_Texture>> m_mapTextures;
        std::unordered_map<Material_Property, float> m_mapProperties;
        std::shared_ptr<RHI_Device> m_RhiDevice;
	};
}

