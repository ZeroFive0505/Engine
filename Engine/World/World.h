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

	// �� Ŭ���� ���� ��� Ŭ������ ��ӹ޴´�.
	class World : public SubModule
	{
	public:
		World(Context* context);
		~World();

		// ���� ��� ���� �޼���
		void OnInit() override;
		void PrevUpdate() override;
		void Update(double delta_time) override;

		// ���� ����
		void New();
		// ���� ����
		bool SaveToFile(const std::string& file_path);
		// ���� �ε�
		bool LoadFromFile(const std::string& file_path);
		inline void Resolve() { m_Resolve = true; }
		// �Ͻ��������� �� �����Ӹ� ������Ʈ ��ų ��
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

		// ���� �⺻���� ��ġ�Ǵ� ��ƼƼ��
		std::shared_ptr<Entity> CreateEnvironment();
		std::shared_ptr<Entity> CreateCamera();
		std::shared_ptr<Entity> CreateDirectionalLight();

		// ������ �̸�
		std::string m_Name;
		// ���
		std::string m_FilePath;
		// �� �����ӿ� ������ ��忴����
		bool m_Was_in_editor_mode = false;
		// ���ֺ�
		bool m_Resolve = true;
		// ù ������Ʈ Ȯ��
		bool m_FirstRun = false;
		bool m_UpdateOnce = false;
		// �Է�
		Input* m_Input;
		Profiler* m_Profiler = nullptr;

		// Ʈ������ �ڵ�
		std::shared_ptr<TransformHandle> m_TransformHandle;
		// ���� ��ġ�Ǵ� ��� ��ƼƼ��
		std::vector<std::shared_ptr<Entity>> m_vecEntities;
	};
}