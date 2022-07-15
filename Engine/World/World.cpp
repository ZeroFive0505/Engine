#include "Common.h"
#include "World.h"
#include "Entity.h"
#include "Components/Transform.h"
#include "Components/Camera.h"
#include "Components/Light.h"
#include "Components/Environment.h"
#include "Components/AudioListener.h"
#include "TransformHandle/TransformHandle.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/ProgressTracker.h"
#include "../IO/FileStream.h"
#include "../Profiling/Profiler.h"
#include "../Input/Input.h"
#include "../RHI/RHI_Device.h"
#include "../Rendering/Renderer.h"
#include "../Core/Context.h"
#include "../Core/Engine.h"

using namespace std;
using namespace PlayGround::Math;

namespace PlayGround
{
	World::World(Context* context) : SubModule(context)
	{
		// ���� ���ֺ�� �ݹ� �Լ��� �����Ѵ�.
		SUBSCRIBE_TO_EVENT(EventType::WorldResolve, [this](Variant) { m_Resolve = true; });
	}

	World::~World()
	{
		m_Input = nullptr;
		m_Profiler = nullptr;
	}

	void World::OnInit()
	{
		// �ʱ�ȭ�ÿ� �Է°� �������ϸ� ����� �����´�.
		m_Input = m_Context->GetSubModule<Input>();
		m_Profiler = m_Context->GetSubModule<Profiler>();

		// �⺻ ��ƼƼ���� �����Ѵ�.
		CreateDefaultWorldEntities();
	}

	void World::PrevUpdate()
	{
		// Ʈ������ �ڵ��� �������� ���� ���
		if (!m_TransformHandle)
			m_TransformHandle = make_shared<TransformHandle>(m_Context);

		// �������� �����ϸ�
		if (Renderer* renderer = m_Context->GetSubModule<Renderer>())
		{
			// Ʈ������ �ڵ鷯�� �� ���� �Ǿ��ִ��� Ȯ���ϰ� ������Ʈ�Ѵ�.
			if (renderer->GetOption(Renderer::Option::Transform_Handle))
				m_TransformHandle->Update(renderer->GetCamera().get(), m_gizmo_transform_size);
		}

		for (auto& entity : m_vecEntities)
		{
			entity->PrevUpdate();
		}
	}

	void World::Update(double delta_time)
	{
	
		// ���� �ε��� ������ �ʾ��� ���
		if (IsLoading())
			return;

		// �������ϸ� ����
		SCOPED_TIME_BLOCK(m_Profiler);

		{
			// ���� �������� ����
			// ���� ���Ӹ��� �� �����ӿ� ������ ��忴�ٸ� ����
			const bool started = m_Context->m_Engine->IsEngineModeSet(GameMode) && m_Was_in_editor_mode;
			// ���� ���Ӹ�尡 �ƴϰ� �� �������� ������ ��尡 �ƴϿ��ٸ� ����
			const bool stopped = !m_Context->m_Engine->IsEngineModeSet(GameMode) && !m_Was_in_editor_mode;
			// ���� ���� ���Ӹ�尡 �ƴ϶�� ������ ���
			m_Was_in_editor_mode = !m_Context->m_Engine->IsEngineModeSet(GameMode);

			// ����
			if (started)
			{
				// ��� ��ƼƼ���� �����Լ��� ȣ�����ش�.
				for (auto& entity : m_vecEntities)
				{
					entity->OnStart();
				}
			}

			// ���� ����������
			// ���⼭ ������ �ѹ� ������Ʈ�� ���´����� ���� Ȯ���Ѵ�.
			if (stopped && m_FirstRun)
			{
				// ��� ��ƼƼ�� ���� �Լ��� ȣ�����ش�.
				for (auto& entity : m_vecEntities)
				{
					entity->OnStop();
				}
			}


			// �Ͻ������鼭 �� ������ ������Ʈ �Ѵٸ�
			if (m_Context->m_Engine->IsEngineModeSet(PauseMode) && m_UpdateOnce)
			{
				// ī�޶� ������Ʈ�ϰ� ��� ��ƼƼ�� ������Ʈ ���� �ʴ´�.
				for (auto& entity : m_vecEntities)
				{
					if (m_Context->m_Engine->IsEngineModeSet(PauseMode) && !entity->HasComponent<Camera>())
						continue;
					entity->Update(delta_time);
				}

				// �� ������ ������Ʈ �Ϸ�
				m_UpdateOnce = false;
			}
			// �Ͻ������� �ƴҰ�� �׳� ����ϰ� ������Ʈ�Ѵ�.
			else
			{
				for (auto& entity : m_vecEntities)
				{
					entity->Update(delta_time);
				}
			}
		}

		if (m_Resolve)
		{
			{
				// ��ƼƼ ���� ����
				auto entities_copy = m_vecEntities;

				for (auto& entity : entities_copy)
				{
					if (entity->IsPendingDestruction())
						_EntityRemove(entity);
				}
			}

			// �̺�Ʈ �߻�
			FIRE_EVENT_DATA(EventType::WorldResolved, m_vecEntities);

			m_Resolve = true;
		}

		// ù ������Ʈ ���� ��
		m_FirstRun = true;
	}

	void World::New()
	{
		Clear();
		CreateDefaultWorldEntities();
	}

	bool World::SaveToFile(const string& file_path)
	{
		// Ʈ��Ŀ ����
		ProgressTracker::Get().Reset(EProgressType::World);
		ProgressTracker::Get().SetIsLoading(EProgressType::World, true);
		ProgressTracker::Get().SetStatus(EProgressType::World, "Saving world...");
		
		// Ÿ�̸� ����
		const StopWatch timer;

		// ��θ� Ȯ���Ѵ�.
		string path = file_path;
		if (FileSystem::GetExtensionFromFilePath(file_path) != EXTENSION_WORLD)
		{
			path += EXTENSION_WORLD;
		}

		m_Name = FileSystem::GetFileNameWithoutExtensionFromFilePath(path);
		m_FilePath = path;

		FIRE_EVENT(EventType::WorldSaveStart);

		auto file = make_unique<FileStream>(path, FileStream_Write);

		if (!file->IsOpen())
		{
			LOG_ERROR("Failed to open file.");
			return false;
		}

		// ��Ʈ ��ƼƼ�� �����´�.
		vector<shared_ptr<Entity>> root_actors = EntityGetRoots();
		// ��Ʈ ��ƼƼ�� ������ �����´�.
		const uint32_t root_entity_count = static_cast<uint32_t>(root_actors.size());

		// ������ŭ Ʈ��Ŀ ����
		ProgressTracker::Get().SetJobCount(EProgressType::World, root_entity_count);

		// ���� ����
		file->Write(root_entity_count);

		// ���̵� ����
		for (auto& root : root_actors)
		{
			file->Write(root->GetObjectID());
		}

		// ���� ���� ����
		for (auto& root : root_actors)
		{
			root->Serialize(file.get());
			ProgressTracker::Get().IncrementJobsDone(EProgressType::World);
		}

		ProgressTracker::Get().SetIsLoading(EProgressType::World, false);
		LOG_INFO("World \"%s\" has been saved. Duration %.2f ms", m_FilePath.c_str(), timer.GetElapsedTimeMS());

		// ���� �� �̺�Ʈ �߻�
		FIRE_EVENT(EventType::WorldSavedEnd);

		return true;
	}

	bool World::LoadFromFile(const string& file_path)
	{
		// ������ ���� ���� Ȯ��
		if (!FileSystem::Exists(file_path))
		{
			LOG_ERROR("\"%s\" was not found.", file_path.c_str());
			return false;
		}

		// ���� ����
		unique_ptr<FileStream> file = make_unique<FileStream>(file_path, FileStream_Read);

		if (!file->IsOpen())
		{
			LOG_ERROR("Failed to open \"%s\"", file_path.c_str());
			return false;
		}

		ProgressTracker& progress_tracker = ProgressTracker::Get();

		progress_tracker.Reset(EProgressType::World);
		progress_tracker.SetIsLoading(EProgressType::World, true);
		progress_tracker.SetStatus(EProgressType::World, "Loading world...");
		const StopWatch timer;

		// ���� �ѹ� ����.
		Clear();

		m_Name = FileSystem::GetFileNameWithoutExtensionFromFilePath(file_path);
		m_FilePath = file_path;
		
		// �ε� ��ŸƮ �̺�Ʈ
		FIRE_EVENT(EventType::WorldLoadStart);

		// ��Ʈ ��ƼƼ�� ������ �����´�.
		const uint32_t root_entity_count = file->ReadAs<uint32_t>();

		progress_tracker.SetJobCount(EProgressType::World, root_entity_count);

		// ������ŭ �ݺ��ϸ鼭 ���̵� �����´�.
		for (uint32_t i = 0; i < root_entity_count; i++)
		{
			shared_ptr<Entity> entity = EntityCreate();
			entity->SetObjectID(file->ReadAs<uint64_t>());
		}

		// ��ƼƼ �ε�
		for (uint32_t i = 0; i < root_entity_count; i++)
		{
			m_vecEntities[i]->Deserialize(file.get(), nullptr);
			progress_tracker.IncrementJobsDone(EProgressType::World);
		}

		progress_tracker.SetIsLoading(EProgressType::World, false);
		LOG_INFO("World \"%s\" has been loaded. Duration %.2f ms", m_FilePath.c_str(), timer.GetElapsedTimeMS());

		FIRE_EVENT(EventType::WorldLoadEnd);

		return true;
	}

	bool World::IsLoading()
	{
		auto& progress_report = ProgressTracker::Get();

		const bool is_loading_model = progress_report.GetIsLoading(EProgressType::ModelImporter);
		const bool is_loading_scene = progress_report.GetIsLoading(EProgressType::World);

		return is_loading_model || is_loading_scene;
	}

	shared_ptr<Entity> World::EntityCreate(bool is_active /*= true*/)
	{
		// �� ��ƼƼ ����
		shared_ptr<Entity> entity = m_vecEntities.emplace_back(make_shared<Entity>(m_Context));
		entity->SetActive(is_active);
		return entity;
	}

	bool World::EntityExists(const shared_ptr<Entity>& entity)
	{
		if (!entity)
			return false;

		return EntityGetByID(entity->GetObjectID()) != nullptr;
	}

	void World::EntityRemove(const shared_ptr<Entity>& entity)
	{
		if (!entity)
			return;

		entity->MarkForDestruction();
		m_Resolve = true;
	}

	vector<shared_ptr<Entity>> World::EntityGetRoots()
	{
		vector<shared_ptr<Entity>> root_entities;
		for (const shared_ptr<Entity> entity : m_vecEntities)
		{
			if (entity->GetTransform()->IsRoot())
				root_entities.emplace_back(entity);
		}

		return root_entities;
	}

	const shared_ptr<Entity>& World::EntityGetByName(const string& name)
	{
		for (const auto& entity : m_vecEntities)
		{
			if (entity->GetObjectName() == name)
				return entity;
		}

		static shared_ptr<Entity> empty;
		return empty;
	}

	const std::shared_ptr<Entity>& World::EntityGetByID(const uint64_t id)
	{
		for (const auto& entity : m_vecEntities)
		{
			if (entity->GetObjectID() == id)
				return entity;
		}

		static shared_ptr<Entity> empty;
		return empty;
	}

	void World::Clear()
	{
		FIRE_EVENT(EventType::WorldPreClear);

		FIRE_EVENT(EventType::WorldClear);

		m_vecEntities.clear();

		m_Name.clear();

		m_FilePath.clear();

		m_Resolve = true;
	}

	void World::_EntityRemove(const std::shared_ptr<Entity>& entity)
	{
		// ��� �ڽ� ��ƼƼ�� �����´�.
		auto children = entity->GetTransform()->GetChildren();

		// ����
		for (const auto& child : children)
		{
			EntityRemove(child->GetEntity()->GetSharedPtr());
		}

		// �׸��� �θ� �����Ѵ�.
		auto parent = entity->GetTransform()->GetParent();

		for (auto it = m_vecEntities.begin(); it != m_vecEntities.end(); it++)
		{
			if ((*it)->GetObjectID() == entity->GetObjectID())
			{
				it = m_vecEntities.erase(it);
				break;
			}
		}


		if (parent)
			parent->AcquireChildren();
	}

	void World::CreateDefaultWorldEntities()
	{
		CreateCamera();
		CreateEnvironment();
		CreateDirectionalLight();
	}

	// ��ī�� �ڽ� ����
	shared_ptr<Entity> World::CreateEnvironment()
	{
		shared_ptr<Entity> environment = EntityCreate();
		environment->SetName("Environment");
		environment->AddComponent<Environment>();

		return environment;
	}
	
	// ī�޶� ����
	shared_ptr<Entity> World::CreateCamera()
	{
		shared_ptr<Entity> entity = EntityCreate();
		entity->SetName("Camera");
		entity->AddComponent<Camera>();
		entity->GetTransform()->SetLocalPosition(Vector3(0.0f, 1.0f, -5.0f));

		return entity;
	}

	// ���� ����
	shared_ptr<Entity> World::CreateDirectionalLight()
	{
		shared_ptr<Entity> light = EntityCreate();
		light->SetName("DirectionalLight");
		light->GetTransform()->SetLocalRotaion(Quaternion::FromEulerAngles(30.0f, 30.0f, 0.0f));
		light->GetTransform()->SetPosition(Vector3(0.0f, 10.0f, 0.0f));

		auto light_comp = light->AddComponent<Light>();
		light_comp->SetLightType(LightType::Directional);

		return light;
	}
}