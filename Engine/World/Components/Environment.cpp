#include "Common.h"
#include "Environment.h"
#include "../../IO/FileStream.h"
#include "../../Threading/Threading.h"
#include "../../RHI/RHI_Texture2D.h"
#include "../../RHI/RHI_TextureCube.h"
#include "../../Resource/ResourceCache.h"
#include "../../Resource/Importer/ImageImporter.h"
#include "../../Rendering/Renderer.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
	Environment::Environment(Context* context, Entity* entity, uint64_t id /*= 0*/) : IComponent(context, entity, id)
	{
		// ��ġ�� ���Ѵ�.
		const string dir_cubemaps = GetContext()->GetSubModule<ResourceCache>()->GetResourceDirectory(EResourceDirectory::CubeMaps) + "/";
		
		// ���� ť����� ��� 6���� �ؽ��ĸ� ���Ѵ�.
		if (m_EnvironmentType == EnvironmentType::Cubemap)
		{
			m_vecFilePaths =
			{
				dir_cubemaps + "array/X+.tga", // right
				dir_cubemaps + "array/X-.tga", // left
				dir_cubemaps + "array/Y+.tga", // up
				dir_cubemaps + "array/Y-.tga", // down
				dir_cubemaps + "array/Z-.tga", // back
				dir_cubemaps + "array/Z+.tga"  // front
			};
		}
		// �ƴҰ�� �ϳ��� �ؽ���
		else if (m_EnvironmentType == EnvironmentType::Sphere)
			m_vecFilePaths = { dir_cubemaps + "texturify_pano-1-5.jpg" };
	}

	void Environment::Update(double delta_time)
	{
		// ���� ���ο� ��ī�̹ڽ��� ��ü�ȴٸ�
		if (m_IsDirty)
		{
			// �����忡 �۾��� �߰��Ѵ�.
			m_Context->GetSubModule<Threading>()->AddTask([this]()
			{
				SetFromTextureSphere(m_vecFilePaths.front());
			});

			// �۾� �Ϸ���
			m_IsDirty = false;
		}
	}

	void Environment::Serialize(FileStream* stream)
	{
		// ����

		stream->Write(static_cast<uint8_t>(m_EnvironmentType));
		stream->Write(m_vecFilePaths);
	}

	void Environment::Deserialize(FileStream* stream)
	{
		// �ҷ�����

		m_EnvironmentType = static_cast<EnvironmentType>(stream->ReadAs<uint8_t>());
		stream->Read(&m_vecFilePaths);

		// �����忡 �۾��� �߰��Ѵ�.
		m_Context->GetSubModule<Threading>()->AddTask([this]
		{
			if (m_EnvironmentType == EnvironmentType::Cubemap)
			{
				SetFromTextureArray(m_vecFilePaths);

			}
			else if (m_EnvironmentType == EnvironmentType::Sphere)
			{

				SetFromTextureSphere(m_vecFilePaths.front());
			}
		});
	}

	const shared_ptr<RHI_Texture> Environment::GetTexture() const
	{
		return m_Context->GetSubModule<Renderer>()->GetEnvironmentTexture();
	}

	void Environment::SetTexture(const shared_ptr<RHI_Texture>& texture)
	{
		m_Context->GetSubModule<Renderer>()->SetEnvironmentTexture(texture);
	}

	void Environment::SetFromTextureArray(const vector<string>& file_paths)
	{
		if (file_paths.empty())
			return;

		LOG_INFO("Loading sky box...");

		ResourceCache* resource_cache = m_Context->GetSubModule<ResourceCache>();

		shared_ptr<RHI_Texture> texture = make_shared<RHI_TextureCube>(GetContext());

		for (uint32_t slice_index = 0; static_cast<uint32_t>(file_paths.size()); slice_index++)
		{
			resource_cache->GetImageImporter()->Load(file_paths[slice_index], slice_index, static_cast<RHI_Texture*>(texture.get()));
		}


		texture->SetResourceFilePath(resource_cache->GetProjectDirectory() + "environment" + EXTENSION_TEXTURE);

		m_vecFilePaths = { texture->GetResourceFilePath() };

		SetTexture(texture);

		LOG_INFO("Sky box has been created successfully");
	}

	void Environment::SetFromTextureSphere(const string& file_path)
	{
		LOG_INFO("Loading sky sphere...");

		shared_ptr<RHI_Texture> texture = make_shared<RHI_Texture2D>(GetContext(), RHI_Texture_Srv | RHI_Texture_Mips);

		if (!texture->LoadFromFile(file_path))
		{
			LOG_ERROR("Sky sphere creation failed");
		}

		m_vecFilePaths = { texture->GetResourceFilePath() };

		SetTexture(texture);

		LOG_INFO("Sky sphere has been created successfully")
	}
}