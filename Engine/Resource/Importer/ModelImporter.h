#pragma once

#include <memory>
#include <string>
#include "../../EngineDefinition.h"

struct aiNode;
struct aiScene;
struct aiMaterial;
struct aiMesh;


namespace PlayGround
{
    class Context;
    class Material;
    class Entity;
    class Model;
    class World;

    struct sModelParams
    {
        // �ﰢ�� �Ѱ�ġ
        uint32_t triangle_limit;
        // ���� �Ѱ�ġ
        uint32_t vertex_limit;
        // ���� ������ ���� �ִ�ġ
        float max_normal_smoothing_angle;
        // ź��Ʈ ������ ���� �ִ�ġ
        float max_tangent_smoothing_angle;
        // ���� ���
        std::string file_path;
        // ���� �̸�
        std::string name;
        // �ִϸ��̼� ����
        bool has_animation;
        // ��
        Model* model = nullptr;
        // ��
        const aiScene* scene = nullptr;
    };

    // Assimp �� ������
    class ModelImporter
    {
    public:
        ModelImporter(Context* context);
        ~ModelImporter() = default;

        bool Load(Model* model, const std::string& file_path);

    private:
        // ��� �Ľ�
        void ParseNode(const aiNode* assimp_node, const sModelParams& params, Entity* parent_node = nullptr, Entity* new_entity = nullptr);
        // �޽� �Ľ�
        void ParseNodeMeshes(const aiNode* assimp_node, Entity* new_entitiy, const sModelParams& params);
        // �ִϸ��̼� �Ľ�
        void ParseAnimations(const sModelParams& params);
        
        // �޽� �ҷ�����
        void LoadMesh(aiMesh* assimp_mesh, Entity* entity_parent, const sModelParams& params);
        // �� �ҷ�����
        void LoadBones(const aiMesh* assimp_mesh, const sModelParams& params);

        Context* m_Context;
        World* m_World;
    };
}
