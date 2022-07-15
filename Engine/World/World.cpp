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
		// 월드 리솔브시 콜백 함수를 설정한다.
		SUBSCRIBE_TO_EVENT(EventType::WorldResolve, [this](Variant) { m_Resolve = true; });
	}

	World::~World()
	{
		m_Input = nullptr;
		m_Profiler = nullptr;
	}

	void World::OnInit()
	{
		// 초기화시에 입력과 프로파일링 모듈을 가져온다.
		m_Input = m_Context->GetSubModule<Input>();
		m_Profiler = m_Context->GetSubModule<Profiler>();

		// 기본 엔티티들을 생성한다.
		CreateDefaultWorldEntities();
	}

	void World::PrevUpdate()
	{
		// 트랜스폼 핸들이 존재하지 않을 경우
		if (!m_TransformHandle)
			m_TransformHandle = make_shared<TransformHandle>(m_Context);

		// 렌더러가 존재하면
		if (Renderer* renderer = m_Context->GetSubModule<Renderer>())
		{
			// 트랜스폼 핸들러가 온 오프 되어있는지 확인하고 업데이트한다.
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
	
		// 아직 로딩이 끝나지 않았을 경우
		if (IsLoading())
			return;

		// 프로파일링 시작
		SCOPED_TIME_BLOCK(m_Profiler);

		{
			// 현재 에디터의 상태
			// 만약 게임모드고 전 프레임에 에디터 모드였다면 시작
			const bool started = m_Context->m_Engine->IsEngineModeSet(GameMode) && m_Was_in_editor_mode;
			// 만약 게임모드가 아니고 전 프레임이 에디터 모드가 아니였다면 멈춤
			const bool stopped = !m_Context->m_Engine->IsEngineModeSet(GameMode) && !m_Was_in_editor_mode;
			// 만약 현재 게임모드가 아니라면 에디터 모드
			m_Was_in_editor_mode = !m_Context->m_Engine->IsEngineModeSet(GameMode);

			// 시작
			if (started)
			{
				// 모든 엔티티들의 시작함수를 호출해준다.
				for (auto& entity : m_vecEntities)
				{
					entity->OnStart();
				}
			}

			// 만약 정지했을시
			// 여기서 엔진이 한번 업데이트를 끝냈는지도 같이 확인한다.
			if (stopped && m_FirstRun)
			{
				// 모든 엔티티의 정지 함수를 호출해준다.
				for (auto& entity : m_vecEntities)
				{
					entity->OnStop();
				}
			}


			// 일시정지면서 한 프레임 업데이트 한다면
			if (m_Context->m_Engine->IsEngineModeSet(PauseMode) && m_UpdateOnce)
			{
				// 카메라만 업데이트하고 모든 엔티티는 업데이트 하지 않는다.
				for (auto& entity : m_vecEntities)
				{
					if (m_Context->m_Engine->IsEngineModeSet(PauseMode) && !entity->HasComponent<Camera>())
						continue;
					entity->Update(delta_time);
				}

				// 한 프레임 업데이트 완료
				m_UpdateOnce = false;
			}
			// 일시정지가 아닐경우 그냥 평범하게 업데이트한다.
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
				// 엔티티 삭제 시작
				auto entities_copy = m_vecEntities;

				for (auto& entity : entities_copy)
				{
					if (entity->IsPendingDestruction())
						_EntityRemove(entity);
				}
			}

			// 이벤트 발생
			FIRE_EVENT_DATA(EventType::WorldResolved, m_vecEntities);

			m_Resolve = true;
		}

		// 첫 업데이트 루프 끝
		m_FirstRun = true;
	}

	void World::New()
	{
		Clear();
		CreateDefaultWorldEntities();
	}

	bool World::SaveToFile(const string& file_path)
	{
		// 트래커 설정
		ProgressTracker::Get().Reset(EProgressType::World);
		ProgressTracker::Get().SetIsLoading(EProgressType::World, true);
		ProgressTracker::Get().SetStatus(EProgressType::World, "Saving world...");
		
		// 타이머 설정
		const StopWatch timer;

		// 경로를 확인한다.
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

		// 루트 엔티티를 가져온다.
		vector<shared_ptr<Entity>> root_actors = EntityGetRoots();
		// 루트 엔티티의 갯수를 가져온다.
		const uint32_t root_entity_count = static_cast<uint32_t>(root_actors.size());

		// 갯수만큼 트래커 설정
		ProgressTracker::Get().SetJobCount(EProgressType::World, root_entity_count);

		// 갯수 저장
		file->Write(root_entity_count);

		// 아이디 저장
		for (auto& root : root_actors)
		{
			file->Write(root->GetObjectID());
		}

		// 파일 저장 시작
		for (auto& root : root_actors)
		{
			root->Serialize(file.get());
			ProgressTracker::Get().IncrementJobsDone(EProgressType::World);
		}

		ProgressTracker::Get().SetIsLoading(EProgressType::World, false);
		LOG_INFO("World \"%s\" has been saved. Duration %.2f ms", m_FilePath.c_str(), timer.GetElapsedTimeMS());

		// 저장 끝 이벤트 발생
		FIRE_EVENT(EventType::WorldSavedEnd);

		return true;
	}

	bool World::LoadFromFile(const string& file_path)
	{
		// 파일의 존재 여부 확인
		if (!FileSystem::Exists(file_path))
		{
			LOG_ERROR("\"%s\" was not found.", file_path.c_str());
			return false;
		}

		// 파일 생성
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

		// 먼저 한번 비운다.
		Clear();

		m_Name = FileSystem::GetFileNameWithoutExtensionFromFilePath(file_path);
		m_FilePath = file_path;
		
		// 로딩 스타트 이벤트
		FIRE_EVENT(EventType::WorldLoadStart);

		// 루트 엔티티의 갯수를 가져온다.
		const uint32_t root_entity_count = file->ReadAs<uint32_t>();

		progress_tracker.SetJobCount(EProgressType::World, root_entity_count);

		// 갯수만큼 반복하면서 아이디를 가져온다.
		for (uint32_t i = 0; i < root_entity_count; i++)
		{
			shared_ptr<Entity> entity = EntityCreate();
			entity->SetObjectID(file->ReadAs<uint64_t>());
		}

		// 엔티티 로딩
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
		// 빈 엔티티 생성
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
		// 모든 자식 엔티티를 가져온다.
		auto children = entity->GetTransform()->GetChildren();

		// 삭제
		for (const auto& child : children)
		{
			EntityRemove(child->GetEntity()->GetSharedPtr());
		}

		// 그리고 부모를 삭제한다.
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

	// 스카이 박스 생성
	shared_ptr<Entity> World::CreateEnvironment()
	{
		shared_ptr<Entity> environment = EntityCreate();
		environment->SetName("Environment");
		environment->AddComponent<Environment>();

		return environment;
	}
	
	// 카메라 생성
	shared_ptr<Entity> World::CreateCamera()
	{
		shared_ptr<Entity> entity = EntityCreate();
		entity->SetName("Camera");
		entity->AddComponent<Camera>();
		entity->GetTransform()->SetLocalPosition(Vector3(0.0f, 1.0f, -5.0f));

		return entity;
	}

	// 조명 생성
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