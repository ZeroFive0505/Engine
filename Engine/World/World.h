#pragma once

#include <vector>
#include <memory>
#include <string>
#include "../Core/SubModule.h"
#include "../EngineDefinition.h"


namespace PlayGround
{
	class Entity;
	class Light;
	class Input;
	class Profiler;
	class TransformHandle;

	// 씬 클래스 서브 모듈 클래스를 상속받는다.
	class World : public SubModule
	{
	public:
		World(Context* context);
		~World();

		// 서브 모듈 가상 메서드
		void OnInit() override;
		void PrevUpdate() override;
		void Update(double delta_time) override;

		// 월드 생성
		void New();
		// 월드 저장
		bool SaveToFile(const std::string& file_path);
		// 월드 로딩
		bool LoadFromFile(const std::string& file_path);
		inline void Resolve() { m_Resolve = true; }
		// 일시정지에서 한 프레임만 업데이트 시킬 시
		inline void SetUpdateOnce() { m_UpdateOnce = true; }
		bool IsLoading();
		inline const std::string GetName() const { return m_Name; }
		inline const std::string& GetFilePath() const{ return m_FilePath; }
		inline bool GetUpdateOnce() const { return m_UpdateOnce; }

		std::shared_ptr<Entity> EntityCreate(bool is_active = true);
		bool EntityExists(const std::shared_ptr<Entity>& entity);
		void EntityRemove(const std::shared_ptr<Entity>& entity);
		std::vector<std::shared_ptr<Entity>> EntityGetRoots();
		const std::shared_ptr<Entity>& EntityGetByName(const std::string& name);
		const std::shared_ptr<Entity>& EntityGetByID(const uint64_t id);
		inline const auto& EntityGetAll() const { return m_vecEntities; }


		std::shared_ptr<TransformHandle> GetTransformHandle() { return m_TransformHandle; }
		float m_gizmo_transform_size = 0.015f;
	private:
		void Clear();
		void _EntityRemove(const std::shared_ptr<Entity>& entity);
		void CreateDefaultWorldEntities();

		// 씬에 기본으로 배치되는 엔티티들
		std::shared_ptr<Entity> CreateEnvironment();
		std::shared_ptr<Entity> CreateCamera();
		std::shared_ptr<Entity> CreateDirectionalLight();

		// 월드의 이름
		std::string m_Name;
		// 경로
		std::string m_FilePath;
		// 전 프레임에 에디터 모드였는지
		bool m_Was_in_editor_mode = false;
		// 리솔브
		bool m_Resolve = true;
		// 첫 업데이트 확인
		bool m_FirstRun = false;
		bool m_UpdateOnce = false;
		// 입력
		Input* m_Input;
		Profiler* m_Profiler = nullptr;

		// 트랜스폼 핸들
		std::shared_ptr<TransformHandle> m_TransformHandle;
		// 씬에 배치되는 모든 엔티티들
		std::vector<std::shared_ptr<Entity>> m_vecEntities;
	};
}