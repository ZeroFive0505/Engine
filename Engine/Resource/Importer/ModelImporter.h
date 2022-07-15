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
        // 삼각형 한계치
        uint32_t triangle_limit;
        // 정점 한계치
        uint32_t vertex_limit;
        // 법선 스무딩 각도 최대치
        float max_normal_smoothing_angle;
        // 탄젠트 스무딩 각도 최대치
        float max_tangent_smoothing_angle;
        // 파일 경로
        std::string file_path;
        // 파일 이름
        std::string name;
        // 애니메이션 여부
        bool has_animation;
        // 모델
        Model* model = nullptr;
        // 씬
        const aiScene* scene = nullptr;
    };

    // Assimp 모델 임포터
    class ModelImporter
    {
    public:
        ModelImporter(Context* context);
        ~ModelImporter() = default;

        bool Load(Model* model, const std::string& file_path);

    private:
        // 노드 파싱
        void ParseNode(const aiNode* assimp_node, const sModelParams& params, Entity* parent_node = nullptr, Entity* new_entity = nullptr);
        // 메시 파싱
        void ParseNodeMeshes(const aiNode* assimp_node, Entity* new_entitiy, const sModelParams& params);
        // 애니메이션 파싱
        void ParseAnimations(const sModelParams& params);
        
        // 메시 불러오기
        void LoadMesh(aiMesh* assimp_mesh, Entity* entity_parent, const sModelParams& params);
        // 본 불러오기
        void LoadBones(const aiMesh* assimp_mesh, const sModelParams& params);

        Context* m_Context;
        World* m_World;
    };
}
