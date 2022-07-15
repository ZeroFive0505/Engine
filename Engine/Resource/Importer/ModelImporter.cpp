#include "Common.h"
#include "ModelImporter.h"
#include "../ProgressTracker.h"
#include "../../RHI/RHI_Vertex.h"
#include "../../RHI/RHI_Texture.h"
#include "../../Rendering/Model.h"
#include "../../Rendering/Animation.h"
#include "../../World/World.h"
#include "../../World/Entity.h"
#include "../../World/Components/Renderable.h"
#include "../../World/Components/Transform.h"
#include "../../Core/Settings.h"

#include "assimp/color4.h"
#include "assimp/matrix4x4.h"
#include "assimp/vector2.h"
#include "assimp/quaternion.h"
#include "assimp/scene.h"
#include "assimp/ProgressHandler.hpp"
#include "assimp/version.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

using namespace std;
using namespace PlayGround::Math;
using namespace Assimp;

namespace PlayGround
{
    // Assimp matrix -> Matrix
	static Matrix convert_matrix(const aiMatrix4x4& transform)
	{
        return Matrix
        (
            transform.a1, transform.b1, transform.c1, transform.d1,
            transform.a2, transform.b2, transform.c2, transform.d2,
            transform.a3, transform.b3, transform.c3, transform.d3,
            transform.a4, transform.b4, transform.c4, transform.d4
        );
	}

    // Assimp aiColor4 -> Vector4
    static Vector4 convert_vector4(const aiColor4D& ai_color)
    {
        return Vector4(ai_color.r, ai_color.g, ai_color.b, ai_color.a);
    }

    // Assimp aiVector3 -> Vector3
    static Vector3 convert_vector3(const aiVector3D& ai_vector)
    {
        return Vector3(ai_vector.x, ai_vector.y, ai_vector.z);
    }

    // Assimp aiVector2 -> Vector2
    static Vector2 convert_vector2(const aiVector2D& ai_vector)
    {
        return Vector2(ai_vector.x, ai_vector.y);
    }

    // Assimp aiQuaternion -> Quaternion
    static Quaternion convert_quaternion(const aiQuaternion& ai_quaternion)
    {
        return Quaternion(ai_quaternion.x, ai_quaternion.y, ai_quaternion.z, ai_quaternion.w);
    }

    // 엔티티 트랜스폼 설정
    constexpr void set_entity_transform(const aiNode* node, Entity* entity)
    {
        if (!entity)
            return;

        // 노드에서 트랜스폼 정보를 가져온다.
        const Matrix matrix_engine = convert_matrix(node->mTransformation);

        // 그 트랜스폼 정보를 기반으로 위치, 회전, 스케일을 정한다.
        entity->GetTransform()->SetLocalPosition(matrix_engine.GetTranslation());
        entity->GetTransform()->SetLocalRotaion(matrix_engine.GetRotation());
        entity->GetTransform()->SetLocalScale(matrix_engine.GetScale());
    }

    // 노드 숫자 세기
    constexpr void compute_node_count(const aiNode* node, int* count)
    {
        // 만약 노드가 존재 하지 않는다면 반환
        if (!node)
            return;

        // 카운트를 늘린다.
        (*count)++;

        // 자식노드를 반복하면서 모든 노드 순회
        for (uint32_t i = 0; i < node->mNumChildren; i++)
        {
            compute_node_count(node->mChildren[i], count);
        }
    }

    // 모델 로딩 진행도
    class AssimpProgress : public ProgressHandler
    {
    public:
        AssimpProgress(const string& file_path)
        {
            // 파일의 경로
            m_FilePath = file_path;
            // 파일 이름
            m_FileName = FileSystem::GetFileNameFromFilePath(file_path);

            // 프로그래스 트래커 설정
            auto& progress = ProgressTracker::Get();
            progress.Reset(EProgressType::ModelImporter);
            progress.SetIsLoading(EProgressType::ModelImporter, true);
        }

        ~AssimpProgress()
        {
            // 로딩 완료
            ProgressTracker::Get().SetIsLoading(EProgressType::ModelImporter, false);
        }

        bool Update(float percentage) override
        {
            return true;
        }

        // ProgressHandler 가상 메서드 오버라이드
        void UpdateFileRead(int current_step, int number_of_steps) override
        {
            auto& progress = ProgressTracker::Get();
            progress.SetStatus(EProgressType::ModelImporter, "Loading \"" + m_FileName + "\" from disk...");
            progress.SetJobsDone(EProgressType::ModelImporter, current_step);
            progress.SetJobCount(EProgressType::ModelImporter, number_of_steps);
        }

        // ProgressHandler 가상 메서드 오버라이드
        void UpdatePostProcess(int current_step, int number_of_steps) override
        {
            auto& progress = ProgressTracker::Get();
            progress.SetStatus(EProgressType::ModelImporter, "Post-Processing \"" + m_FileName + "\"");
            progress.SetJobsDone(EProgressType::ModelImporter, current_step);
            progress.SetJobCount(EProgressType::ModelImporter, number_of_steps);
        }

    private:
        string m_FilePath;
        string m_FileName;
    };

    // 가능한 모든 확장자로 텍스쳐 불러오기를 시도한다.
    static string texture_try_multiple_extensions(const string& file_path)
    {
        // 먼저 확장자 없는 경로만 가져온다.
        const string file_path_no_ext = FileSystem::GetFilePathWithoutExtension(file_path);

        // 모든 가능한 포맷을 시도해본다.
        for (const auto& supported_format : supported_formats_image)
        {
            // 새로운 파일 경로는 확장자를 제외한 파일경로에 새로운 포맷을 덧붙임
            string new_file_path = file_path_no_ext + supported_format;
            string new_file_path_upper = file_path_no_ext + FileSystem::ConvertToUppercase(supported_format);
            
            // 바꾼 확장자로 파일이 존재하는지 확인한다.
            if (FileSystem::Exists(new_file_path))
                return new_file_path;

            if (FileSystem::Exists(new_file_path_upper))
                return new_file_path_upper;
        }

        return file_path;
    }

    // 경로가 유요한지 검사한다.
    static string texture_validate_path(string original_texture_path, const string& model_path)
    {
        replace(original_texture_path.begin(), original_texture_path.end(), '\\', '/');
            
        // 경로를 가져온다.
        const string model_dir = FileSystem::GetDirectoryFromFilePath(model_path);
        // 완전 경로
        string full_texture_path = model_dir + original_texture_path;

        // 존재한다면 반환
        if (FileSystem::Exists(full_texture_path))
            return full_texture_path;

        // 지원 가능한 모든 확장자로 시도
        full_texture_path = texture_try_multiple_extensions(full_texture_path);

        // 만약 존재한다면 반환
        if (FileSystem::Exists(full_texture_path))
            return full_texture_path;

        // 현재 모델 경로에 텍스쳐 파일이 있는지 확인
        full_texture_path = model_dir + FileSystem::GetFileNameFromFilePath(full_texture_path);

        if (FileSystem::Exists(full_texture_path))
            return full_texture_path;

        full_texture_path = texture_try_multiple_extensions(full_texture_path);
        if (FileSystem::Exists(full_texture_path))
            return full_texture_path;

        return "";
    }

    // 마테리얼 텍스쳐를 불러온다.
    static bool load_material_texture(const sModelParams& params, shared_ptr<Material> material, const aiMaterial* material_assimp, const Material_Property texture_type, const aiTextureType texture_type_assimp_pbr, const aiTextureType texture_type_assimp_legacy)
    {
        aiTextureType type_assimp = aiTextureType_NONE;
        // 텍스쳐가 하나라도 존재하는지
        type_assimp = material_assimp->GetTextureCount(texture_type_assimp_pbr) > 0 ? texture_type_assimp_pbr : type_assimp;
        // 만약 텍스쳐 타입이 NONE이라면 레거시를 확인한다.
        type_assimp = (type_assimp == aiTextureType_NONE) ? (material_assimp->GetTextureCount(texture_type_assimp_legacy) > 0 ? texture_type_assimp_legacy : type_assimp) : type_assimp;

        // 텍스처 갯수가 아직도 0개라면 그녕 반환
        if (material_assimp->GetTextureCount(type_assimp) == 0)
            return true;

        // 텍스쳐를 불러온다.
        aiString texture_path;
        if (material_assimp->GetTexture(type_assimp, 0, &texture_path) != AI_SUCCESS)
            return false;

        // 만약 엔진에서 지원가능한 포맷이 아니라면 반환
        const string deduced_path = texture_validate_path(texture_path.data, params.file_path);
        if (!FileSystem::IsSupportedImageFile(deduced_path))
            return false;

        // 모델에 텍스쳐 추가
        params.model->AddTexture(material, texture_type, texture_validate_path(texture_path.data, params.file_path));

        // 텍스쳐 타입이 BASE_COLOR 또는 DIFFUSE라면 색
        if(type_assimp == aiTextureType_BASE_COLOR || type_assimp == aiTextureType_DIFFUSE)
            material->SetColorAlbedo(Vector4::One);

        // 만약 노말이나 높이일 경우
        if (texture_type == Material_Normal || texture_type == Material_Height)
        {
            if (shared_ptr<RHI_Texture> texture = material->GetTextureSharedPtr(texture_type))
            {
                // 먼저 현재 텍스쳐의 타입과 텍스쳐가 그레이 스케일의 텍스쳐인지 확인한다.
                // 이는 가끔 높이와 노말맵이 뒤바끼는 경우가 있기 때문
                Material_Property proper_type = texture_type;
                proper_type = (proper_type == Material_Normal && texture->IsGrayscale()) ? Material_Height : proper_type;
                proper_type = (proper_type == Material_Height && !texture->IsGrayscale()) ? Material_Normal : proper_type;

                if (proper_type != texture_type)
                {
                    material->SetTextureSlot(texture_type, shared_ptr<RHI_Texture>(nullptr));
                    material->SetTextureSlot(proper_type, texture);
                }
            }
        }

        return true;
    }

    // 마테리얼을 불러온다.
    static shared_ptr<Material> load_material(Context* context, const aiMaterial* material_assimp, const sModelParams& params)
    {
        ASSERT(material_assimp != nullptr);

        // 마테리얼 생성
        shared_ptr<Material> material = make_shared<Material>(context);

        // 마테리얼 이름을 가져온다.
        aiString name;
        aiGetMaterialString(material_assimp, AI_MATKEY_NAME, &name);

        // 경로 저장
        material->SetResourceFilePath(FileSystem::RemoveIllegalCharacters(FileSystem::GetDirectoryFromFilePath(params.file_path) + string(name.C_Str()) + EXTENSION_MATERIAL));

        // 색깔을 가져온다.
        aiColor4D color_diffuse(1.0f, 1.0f, 1.0f, 1.0f);
        aiGetMaterialColor(material_assimp, AI_MATKEY_COLOR_DIFFUSE, &color_diffuse);

        // 투명도를 가져온다.
        aiColor4D opacity(1.0f, 1.0f, 1.0f, 1.0f);
        aiGetMaterialColor(material_assimp, AI_MATKEY_OPACITY, &opacity);

        // 색 지정
        material->SetColorAlbedo(Vector4(color_diffuse.r, color_diffuse.g, color_diffuse.b, opacity.r));


        // 다른 텍스쳐들을 불러온다.
        load_material_texture(params, material, material_assimp, Material_Color, aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE);
        load_material_texture(params, material, material_assimp, Material_Roughness, aiTextureType_DIFFUSE_ROUGHNESS, aiTextureType_SHININESS); // Use specular as fallback
        load_material_texture(params, material, material_assimp, Material_Metallic, aiTextureType_METALNESS, aiTextureType_AMBIENT);   // Use ambient as fallback
        load_material_texture(params, material, material_assimp, Material_Normal, aiTextureType_NORMAL_CAMERA, aiTextureType_NORMALS);
        load_material_texture(params, material, material_assimp, Material_Occlusion, aiTextureType_AMBIENT_OCCLUSION, aiTextureType_LIGHTMAP);
        load_material_texture(params, material, material_assimp, Material_Emission, aiTextureType_EMISSION_COLOR, aiTextureType_EMISSIVE);
        load_material_texture(params, material, material_assimp, Material_Height, aiTextureType_HEIGHT, aiTextureType_NONE);
        load_material_texture(params, material, material_assimp, Material_AlphaMask, aiTextureType_OPACITY, aiTextureType_NONE);

        return material;
    }

    ModelImporter::ModelImporter(Context* context)
    {
        m_Context = context;
        m_World = context->GetSubModule<World>();

        const int major = aiGetVersionMajor();
        const int minor = aiGetVersionMinor();
        const int rev = aiGetVersionRevision();

        m_Context->GetSubModule<Settings>()->RegisterThirdParty("Assimp", to_string(major) + "." + to_string(minor) + "." + to_string(rev), "https://github.com/assimp/assimp");
    }

    bool ModelImporter::Load(Model* model, const string& file_path)
    {
        ASSERT(model != nullptr);

        // 파일의 존재 여부 확인
        if (!FileSystem::IsFile(file_path))
        {
            LOG_ERROR("Provided file path doesn't point to an existing file");
            return false;
        }

        sModelParams params;
        params.triangle_limit = 1000000;
        params.vertex_limit = 1000000;
        // 이 최대치 각도를 넘어가면 노말을 부드럽게 하지 않는다.
        params.max_normal_smoothing_angle = 80.0f;
        // 이 최대치 각도를 넘어가면 탄젠트를 부드럽게 하지 않는다.
        params.max_tangent_smoothing_angle = 80.0f;
        params.file_path = file_path;
        params.name = FileSystem::GetFileNameWithoutExtensionFromFilePath(file_path);
        params.model = model;


        // Assimp 모델 임포터 설정
        Importer importer;
        // 노말 스무딩 각도 설정
        importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, params.max_normal_smoothing_angle);
        // 탄젠트 스무딩 각도 설정
        importer.SetPropertyFloat(AI_CONFIG_PP_CT_MAX_SMOOTHING_ANGLE, params.max_tangent_smoothing_angle);
        // 삼각형 최대치 설정
        importer.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, params.triangle_limit);
        // 정점 최대치 설정
        importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, params.vertex_limit);
        // 포인트와 라인 삭제
        importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
        // 씬에 있는 카메라와 광원 삭제
        importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_CAMERAS | aiComponent_LIGHTS);
        // 진행도 추적 설정
        importer.SetPropertyBool(AI_CONFIG_GLOB_MEASURE_TIME, true);
        importer.SetProgressHandler(new AssimpProgress(file_path));

        const auto importer_flags =
            aiProcess_MakeLeftHanded |           // 다이렉트 X는 왼손좌표계 따라서 좌표 변환해준다.
            aiProcess_FlipUVs |                  // 다이렉트 X는 UV가 뒤집힘
            aiProcess_FlipWindingOrder |         // 다이렉트 X에 맞춤 설정
            aiProcess_CalcTangentSpace |
            aiProcess_GenSmoothNormals |
            aiProcess_GenUVCoords |
            aiProcess_JoinIdenticalVertices |
            aiProcess_ImproveCacheLocality |     // 삼각형을 재배치해서 캐쉬 히트율을 높인다.
            aiProcess_LimitBoneWeights |
            aiProcess_Triangulate |
            aiProcess_SortByPType |              // 메쉬를 삼각형으로 쪼갠다.
            aiProcess_FindDegenerates |          // 정리
            aiProcess_FindInvalidData |
            aiProcess_FindInstances |
            aiProcess_ValidateDataStructure;

        // 씬을 불러온다.
        const aiScene* scene = importer.ReadFile(file_path, importer_flags);

        // 씬이 존재한다면
        if (scene)
        {
            // 총 노드 갯수 파악
            int job_count = 0;
            compute_node_count(scene->mRootNode, &job_count);
            // 트래커 설정
            ProgressTracker::Get().SetJobCount(EProgressType::ModelImporter, job_count);

            params.scene = scene;
            params.has_animation = scene->mNumAnimations != 0;

            const bool is_active = false;
            shared_ptr<Entity> new_entity = m_World->EntityCreate(is_active);
            new_entity->SetName(params.name);
            params.model->SetRootEntity(new_entity);
            
            // 노드 파싱
            ParseNode(scene->mRootNode, params, nullptr, new_entity.get());

            // 애니메이션 파싱
            ParseAnimations(params);

            // 지오메트리 업데이트
            model->UpdateGeometry();
        }
        else
        {
            LOG_ERROR("%s", importer.GetErrorString());
        }

        // 씬 해제
        importer.FreeScene();

        return params.scene != nullptr;
    }

    void ModelImporter::ParseNode(const aiNode* assimp_node, const sModelParams& params, Entity* parent_node, Entity* new_entity)
    {
        // 만약 부모 노드가 존재한다면 이름 설정
        if (parent_node)
            new_entity->SetName(assimp_node->mName.C_Str());

        // 트래커 설정
        ProgressTracker::Get().SetStatus(EProgressType::ModelImporter, "Creating entity for " + new_entity->GetObjectName());

        // 트랜스폼을 가져온다.
        Transform* parent_trans = parent_node ? parent_node->GetTransform() : nullptr;
        // 부모자식 설정
        new_entity->GetTransform()->SetParent(parent_trans);

        // 엔티티의 트랜스폼을 설정한다.
        set_entity_transform(assimp_node, new_entity);

        // 메시 파싱
        ParseNodeMeshes(assimp_node, new_entity, params);

        // 모든 자식 노드를 순회한다.
        for (uint32_t i = 0; i < assimp_node->mNumChildren; i++)
        {
            // 빈 엔티티를 만들고
            auto child = m_World->EntityCreate();
            // 노드를 파싱한다.
            ParseNode(assimp_node->mChildren[i], params, new_entity, child.get());
        }

        // 트래커 카운트 증가
        ProgressTracker::Get().IncrementJobsDone(EProgressType::ModelImporter);
    }

    void ModelImporter::ParseNodeMeshes(const aiNode* assimp_node, Entity* new_entity, const sModelParams& params)
    {
        // 모든 메시를 순회한다.
        for (uint32_t i = 0; i < assimp_node->mNumMeshes; i++)
        {
            // 빈 엔티티 생성
            auto entity = new_entity;
            // 메시를 가져온다.
            const auto assimp_mesh = params.scene->mMeshes[assimp_node->mMeshes[i]];
            // 이름을 가져온다.
            string _name = assimp_node->mName.C_Str();

            // 만약 메쉬가 존재한다면
            if (assimp_node->mNumMeshes > 1)
            {
                const bool is_active = false;
                // 빈 엔티티 생성
                entity = m_World->EntityCreate(is_active).get();
                // 부모자식 설정
                entity->GetTransform()->SetParent(new_entity->GetTransform());
                // 이름 설정
                _name += "_" + to_string(i + 1);
            }

            // 메쉬의 이름을 설정하고 메쉬를 본격적으로 로드한다.
            entity->SetName(_name);

            LoadMesh(assimp_mesh, entity, params);
            // 렌더링 여부
            entity->SetActive(true);
        }
    }

    void ModelImporter::ParseAnimations(const sModelParams& params)
    {
        // 모든 애니메이션을 순회한다.
        for (uint32_t i = 0; i < params.scene->mNumAnimations; i++)
        {
            // 애니메이션을 순회하고 생성한다.
            const auto assimp_animation = params.scene->mAnimations[i];
            auto animation = make_shared<Animation>(m_Context);

            // 애니메이션의 이름, 재생시간, 틱 시간을 가져온다.
            animation->SetName(assimp_animation->mName.C_Str());
            animation->SetDuration(assimp_animation->mDuration);
            animation->SetTicksPerSec(assimp_animation->mTicksPerSecond != 0.0f ? assimp_animation->mTicksPerSecond : 25.0f);

            // 애니메이션 채널
            for (uint32_t j = 0; j < static_cast<uint32_t>(assimp_animation->mNumChannels); j++)
            {
                const auto assimp_node_anim = assimp_animation->mChannels[j];
                AnimationNode animation_node;

                // 애니메이션의 이름
                animation_node.name = assimp_node_anim->mNodeName.C_Str();

                // 위치 키값을 불러온다.
                for (uint32_t k = 0; k < static_cast<uint32_t>(assimp_node_anim->mNumPositionKeys); k++)
                {
                    // 시간
                    const auto time = assimp_node_anim->mPositionKeys[k].mTime;
                    // 위치
                    const auto value = convert_vector3(assimp_node_anim->mPositionKeys[k].mValue);

                    // 추가한다.
                    animation_node.positionFrames.emplace_back(KeyVector{ time, value });
                }

                // 회전 키값을 불러온다.
                for (uint32_t k = 0; k < static_cast<uint32_t>(assimp_node_anim->mNumRotationKeys); k++)
                {
                    const auto time = assimp_node_anim->mPositionKeys[k].mTime;
                    const auto value = convert_quaternion(assimp_node_anim->mRotationKeys[k].mValue);

                    animation_node.rotationFrames.emplace_back(KeyQuaternion{ time, value });
                }

                // 스케일링 키값을 불러온다.
                for (uint32_t k = 0; k < static_cast<uint32_t>(assimp_node_anim->mNumScalingKeys); k++)
                {
                    const auto time = assimp_node_anim->mPositionKeys[k].mTime;
                    const auto value = convert_vector3(assimp_node_anim->mScalingKeys[k].mValue);

                    animation_node.scaleFrames.emplace_back(KeyVector{ time, value });
                }
            }
        }
    }

    void ModelImporter::LoadMesh(aiMesh* assimp_mesh, Entity* entity_parent, const sModelParams& params)
    {
        ASSERT(assimp_mesh != nullptr);
        ASSERT(entity_parent != nullptr);

        // 버텍스와, 인덱스 카운팅
        const uint32_t vertex_count = assimp_mesh->mNumVertices;
        const uint32_t index_count = assimp_mesh->mNumFaces * 3;

        // 버텍스 수만큼 벡터 생성
        vector<RHI_Vertex_PosTexNorTan> vertices = vector<RHI_Vertex_PosTexNorTan>(vertex_count);
        {
            // 버텍스 수만큼 반복한다.
            for (uint32_t i = 0; i < vertex_count; i++)
            {
                RHI_Vertex_PosTexNorTan& vertex = vertices[i];

                // 위치
                const aiVector3D& pos = assimp_mesh->mVertices[i];
                vertex.pos[0] = pos.x;
                vertex.pos[1] = pos.y;
                vertex.pos[2] = pos.z;


                // 노말
                if (assimp_mesh->mNormals)
                {
                    const aiVector3D& normal = assimp_mesh->mNormals[i];
                    vertex.nor[0] = normal.x;
                    vertex.nor[1] = normal.y;
                    vertex.nor[2] = normal.z;
                }

                // 탄젠트
                if (assimp_mesh->mTangents)
                {
                    const aiVector3D& tangent = assimp_mesh->mTangents[i];
                    vertex.tan[0] = tangent.x;
                    vertex.tan[1] = tangent.y;
                    vertex.tan[2] = tangent.z;
                }

                // UV 좌표
                const uint32_t uv_channel = 0;
                if (assimp_mesh->HasTextureCoords(uv_channel))
                {
                    const auto& tex_coords = assimp_mesh->mTextureCoords[uv_channel][i];
                    vertex.tex[0] = tex_coords.x;
                    vertex.tex[1] = tex_coords.y;
                }
            }
        }

        // 인덱스 추가
        vector<uint32_t> indices = vector<uint32_t>(index_count);
        {
            // 면의 수만큼 반복한다.
            for (uint32_t face_index = 0; face_index < assimp_mesh->mNumFaces; face_index++)
            {
                const aiFace& face = assimp_mesh->mFaces[face_index];
                // 최소 단위는 삼각형이다 따라서 3을 곱해준다.
                const uint32_t indices_index = (face_index * 3);
                indices[indices_index + 0] = face.mIndices[0];
                indices[indices_index + 1] = face.mIndices[1];
                indices[indices_index + 2] = face.mIndices[2];
            }
        }

        // 바운딩 박스 생성
        const BoundingBox aabb = BoundingBox(vertices.data(), static_cast<uint32_t>(vertices.size()));

        uint32_t index_offset;
        uint32_t vertex_offset;
        params.model->AppendGeometry(move(indices), move(vertices), &index_offset, &vertex_offset);

        Renderable* renderable = entity_parent->AddComponent<Renderable>();

        renderable->GeometrySet(
            entity_parent->GetObjectName(),
            index_offset,
            static_cast<uint32_t>(indices.size()),
            vertex_offset,
            static_cast<uint32_t>(vertices.size()),
            aabb,
            params.model
        );

        // 마테리얼 설정
        if (params.scene->HasMaterials())
        {
            const aiMaterial* assimp_material = params.scene->mMaterials[assimp_mesh->mMaterialIndex];
            // 마테리얼을 불러오고 추가한다.
            shared_ptr<Material> material = load_material(m_Context, assimp_material, params);
            params.model->AddMaterial(material, entity_parent->GetSharedPtr());
        }

        // 본
        LoadBones(assimp_mesh, params);
    }

    void ModelImporter::LoadBones(const aiMesh* assimp_mesh, const sModelParams& params)
    {
    }
}