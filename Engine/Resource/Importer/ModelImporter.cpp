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

    // ��ƼƼ Ʈ������ ����
    constexpr void set_entity_transform(const aiNode* node, Entity* entity)
    {
        if (!entity)
            return;

        // ��忡�� Ʈ������ ������ �����´�.
        const Matrix matrix_engine = convert_matrix(node->mTransformation);

        // �� Ʈ������ ������ ������� ��ġ, ȸ��, �������� ���Ѵ�.
        entity->GetTransform()->SetLocalPosition(matrix_engine.GetTranslation());
        entity->GetTransform()->SetLocalRotaion(matrix_engine.GetRotation());
        entity->GetTransform()->SetLocalScale(matrix_engine.GetScale());
    }

    // ��� ���� ����
    constexpr void compute_node_count(const aiNode* node, int* count)
    {
        // ���� ��尡 ���� ���� �ʴ´ٸ� ��ȯ
        if (!node)
            return;

        // ī��Ʈ�� �ø���.
        (*count)++;

        // �ڽĳ�带 �ݺ��ϸ鼭 ��� ��� ��ȸ
        for (uint32_t i = 0; i < node->mNumChildren; i++)
        {
            compute_node_count(node->mChildren[i], count);
        }
    }

    // �� �ε� ���൵
    class AssimpProgress : public ProgressHandler
    {
    public:
        AssimpProgress(const string& file_path)
        {
            // ������ ���
            m_FilePath = file_path;
            // ���� �̸�
            m_FileName = FileSystem::GetFileNameFromFilePath(file_path);

            // ���α׷��� Ʈ��Ŀ ����
            auto& progress = ProgressTracker::Get();
            progress.Reset(EProgressType::ModelImporter);
            progress.SetIsLoading(EProgressType::ModelImporter, true);
        }

        ~AssimpProgress()
        {
            // �ε� �Ϸ�
            ProgressTracker::Get().SetIsLoading(EProgressType::ModelImporter, false);
        }

        bool Update(float percentage) override
        {
            return true;
        }

        // ProgressHandler ���� �޼��� �������̵�
        void UpdateFileRead(int current_step, int number_of_steps) override
        {
            auto& progress = ProgressTracker::Get();
            progress.SetStatus(EProgressType::ModelImporter, "Loading \"" + m_FileName + "\" from disk...");
            progress.SetJobsDone(EProgressType::ModelImporter, current_step);
            progress.SetJobCount(EProgressType::ModelImporter, number_of_steps);
        }

        // ProgressHandler ���� �޼��� �������̵�
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

    // ������ ��� Ȯ���ڷ� �ؽ��� �ҷ����⸦ �õ��Ѵ�.
    static string texture_try_multiple_extensions(const string& file_path)
    {
        // ���� Ȯ���� ���� ��θ� �����´�.
        const string file_path_no_ext = FileSystem::GetFilePathWithoutExtension(file_path);

        // ��� ������ ������ �õ��غ���.
        for (const auto& supported_format : supported_formats_image)
        {
            // ���ο� ���� ��δ� Ȯ���ڸ� ������ ���ϰ�ο� ���ο� ������ ������
            string new_file_path = file_path_no_ext + supported_format;
            string new_file_path_upper = file_path_no_ext + FileSystem::ConvertToUppercase(supported_format);
            
            // �ٲ� Ȯ���ڷ� ������ �����ϴ��� Ȯ���Ѵ�.
            if (FileSystem::Exists(new_file_path))
                return new_file_path;

            if (FileSystem::Exists(new_file_path_upper))
                return new_file_path_upper;
        }

        return file_path;
    }

    // ��ΰ� �������� �˻��Ѵ�.
    static string texture_validate_path(string original_texture_path, const string& model_path)
    {
        replace(original_texture_path.begin(), original_texture_path.end(), '\\', '/');
            
        // ��θ� �����´�.
        const string model_dir = FileSystem::GetDirectoryFromFilePath(model_path);
        // ���� ���
        string full_texture_path = model_dir + original_texture_path;

        // �����Ѵٸ� ��ȯ
        if (FileSystem::Exists(full_texture_path))
            return full_texture_path;

        // ���� ������ ��� Ȯ���ڷ� �õ�
        full_texture_path = texture_try_multiple_extensions(full_texture_path);

        // ���� �����Ѵٸ� ��ȯ
        if (FileSystem::Exists(full_texture_path))
            return full_texture_path;

        // ���� �� ��ο� �ؽ��� ������ �ִ��� Ȯ��
        full_texture_path = model_dir + FileSystem::GetFileNameFromFilePath(full_texture_path);

        if (FileSystem::Exists(full_texture_path))
            return full_texture_path;

        full_texture_path = texture_try_multiple_extensions(full_texture_path);
        if (FileSystem::Exists(full_texture_path))
            return full_texture_path;

        return "";
    }

    // ���׸��� �ؽ��ĸ� �ҷ��´�.
    static bool load_material_texture(const sModelParams& params, shared_ptr<Material> material, const aiMaterial* material_assimp, const Material_Property texture_type, const aiTextureType texture_type_assimp_pbr, const aiTextureType texture_type_assimp_legacy)
    {
        aiTextureType type_assimp = aiTextureType_NONE;
        // �ؽ��İ� �ϳ��� �����ϴ���
        type_assimp = material_assimp->GetTextureCount(texture_type_assimp_pbr) > 0 ? texture_type_assimp_pbr : type_assimp;
        // ���� �ؽ��� Ÿ���� NONE�̶�� ���Žø� Ȯ���Ѵ�.
        type_assimp = (type_assimp == aiTextureType_NONE) ? (material_assimp->GetTextureCount(texture_type_assimp_legacy) > 0 ? texture_type_assimp_legacy : type_assimp) : type_assimp;

        // �ؽ�ó ������ ������ 0����� �׳� ��ȯ
        if (material_assimp->GetTextureCount(type_assimp) == 0)
            return true;

        // �ؽ��ĸ� �ҷ��´�.
        aiString texture_path;
        if (material_assimp->GetTexture(type_assimp, 0, &texture_path) != AI_SUCCESS)
            return false;

        // ���� �������� ���������� ������ �ƴ϶�� ��ȯ
        const string deduced_path = texture_validate_path(texture_path.data, params.file_path);
        if (!FileSystem::IsSupportedImageFile(deduced_path))
            return false;

        // �𵨿� �ؽ��� �߰�
        params.model->AddTexture(material, texture_type, texture_validate_path(texture_path.data, params.file_path));

        // �ؽ��� Ÿ���� BASE_COLOR �Ǵ� DIFFUSE��� ��
        if(type_assimp == aiTextureType_BASE_COLOR || type_assimp == aiTextureType_DIFFUSE)
            material->SetColorAlbedo(Vector4::One);

        // ���� �븻�̳� ������ ���
        if (texture_type == Material_Normal || texture_type == Material_Height)
        {
            if (shared_ptr<RHI_Texture> texture = material->GetTextureSharedPtr(texture_type))
            {
                // ���� ���� �ؽ����� Ÿ�԰� �ؽ��İ� �׷��� �������� �ؽ������� Ȯ���Ѵ�.
                // �̴� ���� ���̿� �븻���� �ڹٳ��� ��찡 �ֱ� ����
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

    // ���׸����� �ҷ��´�.
    static shared_ptr<Material> load_material(Context* context, const aiMaterial* material_assimp, const sModelParams& params)
    {
        ASSERT(material_assimp != nullptr);

        // ���׸��� ����
        shared_ptr<Material> material = make_shared<Material>(context);

        // ���׸��� �̸��� �����´�.
        aiString name;
        aiGetMaterialString(material_assimp, AI_MATKEY_NAME, &name);

        // ��� ����
        material->SetResourceFilePath(FileSystem::RemoveIllegalCharacters(FileSystem::GetDirectoryFromFilePath(params.file_path) + string(name.C_Str()) + EXTENSION_MATERIAL));

        // ������ �����´�.
        aiColor4D color_diffuse(1.0f, 1.0f, 1.0f, 1.0f);
        aiGetMaterialColor(material_assimp, AI_MATKEY_COLOR_DIFFUSE, &color_diffuse);

        // ������ �����´�.
        aiColor4D opacity(1.0f, 1.0f, 1.0f, 1.0f);
        aiGetMaterialColor(material_assimp, AI_MATKEY_OPACITY, &opacity);

        // �� ����
        material->SetColorAlbedo(Vector4(color_diffuse.r, color_diffuse.g, color_diffuse.b, opacity.r));


        // �ٸ� �ؽ��ĵ��� �ҷ��´�.
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

        // ������ ���� ���� Ȯ��
        if (!FileSystem::IsFile(file_path))
        {
            LOG_ERROR("Provided file path doesn't point to an existing file");
            return false;
        }

        sModelParams params;
        params.triangle_limit = 1000000;
        params.vertex_limit = 1000000;
        // �� �ִ�ġ ������ �Ѿ�� �븻�� �ε巴�� ���� �ʴ´�.
        params.max_normal_smoothing_angle = 80.0f;
        // �� �ִ�ġ ������ �Ѿ�� ź��Ʈ�� �ε巴�� ���� �ʴ´�.
        params.max_tangent_smoothing_angle = 80.0f;
        params.file_path = file_path;
        params.name = FileSystem::GetFileNameWithoutExtensionFromFilePath(file_path);
        params.model = model;


        // Assimp �� ������ ����
        Importer importer;
        // �븻 ������ ���� ����
        importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, params.max_normal_smoothing_angle);
        // ź��Ʈ ������ ���� ����
        importer.SetPropertyFloat(AI_CONFIG_PP_CT_MAX_SMOOTHING_ANGLE, params.max_tangent_smoothing_angle);
        // �ﰢ�� �ִ�ġ ����
        importer.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, params.triangle_limit);
        // ���� �ִ�ġ ����
        importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, params.vertex_limit);
        // ����Ʈ�� ���� ����
        importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
        // ���� �ִ� ī�޶�� ���� ����
        importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_CAMERAS | aiComponent_LIGHTS);
        // ���൵ ���� ����
        importer.SetPropertyBool(AI_CONFIG_GLOB_MEASURE_TIME, true);
        importer.SetProgressHandler(new AssimpProgress(file_path));

        const auto importer_flags =
            aiProcess_MakeLeftHanded |           // ���̷�Ʈ X�� �޼���ǥ�� ���� ��ǥ ��ȯ���ش�.
            aiProcess_FlipUVs |                  // ���̷�Ʈ X�� UV�� ������
            aiProcess_FlipWindingOrder |         // ���̷�Ʈ X�� ���� ����
            aiProcess_CalcTangentSpace |
            aiProcess_GenSmoothNormals |
            aiProcess_GenUVCoords |
            aiProcess_JoinIdenticalVertices |
            aiProcess_ImproveCacheLocality |     // �ﰢ���� ���ġ�ؼ� ĳ�� ��Ʈ���� ���δ�.
            aiProcess_LimitBoneWeights |
            aiProcess_Triangulate |
            aiProcess_SortByPType |              // �޽��� �ﰢ������ �ɰ���.
            aiProcess_FindDegenerates |          // ����
            aiProcess_FindInvalidData |
            aiProcess_FindInstances |
            aiProcess_ValidateDataStructure;

        // ���� �ҷ��´�.
        const aiScene* scene = importer.ReadFile(file_path, importer_flags);

        // ���� �����Ѵٸ�
        if (scene)
        {
            // �� ��� ���� �ľ�
            int job_count = 0;
            compute_node_count(scene->mRootNode, &job_count);
            // Ʈ��Ŀ ����
            ProgressTracker::Get().SetJobCount(EProgressType::ModelImporter, job_count);

            params.scene = scene;
            params.has_animation = scene->mNumAnimations != 0;

            const bool is_active = false;
            shared_ptr<Entity> new_entity = m_World->EntityCreate(is_active);
            new_entity->SetName(params.name);
            params.model->SetRootEntity(new_entity);
            
            // ��� �Ľ�
            ParseNode(scene->mRootNode, params, nullptr, new_entity.get());

            // �ִϸ��̼� �Ľ�
            ParseAnimations(params);

            // ������Ʈ�� ������Ʈ
            model->UpdateGeometry();
        }
        else
        {
            LOG_ERROR("%s", importer.GetErrorString());
        }

        // �� ����
        importer.FreeScene();

        return params.scene != nullptr;
    }

    void ModelImporter::ParseNode(const aiNode* assimp_node, const sModelParams& params, Entity* parent_node, Entity* new_entity)
    {
        // ���� �θ� ��尡 �����Ѵٸ� �̸� ����
        if (parent_node)
            new_entity->SetName(assimp_node->mName.C_Str());

        // Ʈ��Ŀ ����
        ProgressTracker::Get().SetStatus(EProgressType::ModelImporter, "Creating entity for " + new_entity->GetObjectName());

        // Ʈ�������� �����´�.
        Transform* parent_trans = parent_node ? parent_node->GetTransform() : nullptr;
        // �θ��ڽ� ����
        new_entity->GetTransform()->SetParent(parent_trans);

        // ��ƼƼ�� Ʈ�������� �����Ѵ�.
        set_entity_transform(assimp_node, new_entity);

        // �޽� �Ľ�
        ParseNodeMeshes(assimp_node, new_entity, params);

        // ��� �ڽ� ��带 ��ȸ�Ѵ�.
        for (uint32_t i = 0; i < assimp_node->mNumChildren; i++)
        {
            // �� ��ƼƼ�� �����
            auto child = m_World->EntityCreate();
            // ��带 �Ľ��Ѵ�.
            ParseNode(assimp_node->mChildren[i], params, new_entity, child.get());
        }

        // Ʈ��Ŀ ī��Ʈ ����
        ProgressTracker::Get().IncrementJobsDone(EProgressType::ModelImporter);
    }

    void ModelImporter::ParseNodeMeshes(const aiNode* assimp_node, Entity* new_entity, const sModelParams& params)
    {
        // ��� �޽ø� ��ȸ�Ѵ�.
        for (uint32_t i = 0; i < assimp_node->mNumMeshes; i++)
        {
            // �� ��ƼƼ ����
            auto entity = new_entity;
            // �޽ø� �����´�.
            const auto assimp_mesh = params.scene->mMeshes[assimp_node->mMeshes[i]];
            // �̸��� �����´�.
            string _name = assimp_node->mName.C_Str();

            // ���� �޽��� �����Ѵٸ�
            if (assimp_node->mNumMeshes > 1)
            {
                const bool is_active = false;
                // �� ��ƼƼ ����
                entity = m_World->EntityCreate(is_active).get();
                // �θ��ڽ� ����
                entity->GetTransform()->SetParent(new_entity->GetTransform());
                // �̸� ����
                _name += "_" + to_string(i + 1);
            }

            // �޽��� �̸��� �����ϰ� �޽��� ���������� �ε��Ѵ�.
            entity->SetName(_name);

            LoadMesh(assimp_mesh, entity, params);
            // ������ ����
            entity->SetActive(true);
        }
    }

    void ModelImporter::ParseAnimations(const sModelParams& params)
    {
        // ��� �ִϸ��̼��� ��ȸ�Ѵ�.
        for (uint32_t i = 0; i < params.scene->mNumAnimations; i++)
        {
            // �ִϸ��̼��� ��ȸ�ϰ� �����Ѵ�.
            const auto assimp_animation = params.scene->mAnimations[i];
            auto animation = make_shared<Animation>(m_Context);

            // �ִϸ��̼��� �̸�, ����ð�, ƽ �ð��� �����´�.
            animation->SetName(assimp_animation->mName.C_Str());
            animation->SetDuration(assimp_animation->mDuration);
            animation->SetTicksPerSec(assimp_animation->mTicksPerSecond != 0.0f ? assimp_animation->mTicksPerSecond : 25.0f);

            // �ִϸ��̼� ä��
            for (uint32_t j = 0; j < static_cast<uint32_t>(assimp_animation->mNumChannels); j++)
            {
                const auto assimp_node_anim = assimp_animation->mChannels[j];
                AnimationNode animation_node;

                // �ִϸ��̼��� �̸�
                animation_node.name = assimp_node_anim->mNodeName.C_Str();

                // ��ġ Ű���� �ҷ��´�.
                for (uint32_t k = 0; k < static_cast<uint32_t>(assimp_node_anim->mNumPositionKeys); k++)
                {
                    // �ð�
                    const auto time = assimp_node_anim->mPositionKeys[k].mTime;
                    // ��ġ
                    const auto value = convert_vector3(assimp_node_anim->mPositionKeys[k].mValue);

                    // �߰��Ѵ�.
                    animation_node.positionFrames.emplace_back(KeyVector{ time, value });
                }

                // ȸ�� Ű���� �ҷ��´�.
                for (uint32_t k = 0; k < static_cast<uint32_t>(assimp_node_anim->mNumRotationKeys); k++)
                {
                    const auto time = assimp_node_anim->mPositionKeys[k].mTime;
                    const auto value = convert_quaternion(assimp_node_anim->mRotationKeys[k].mValue);

                    animation_node.rotationFrames.emplace_back(KeyQuaternion{ time, value });
                }

                // �����ϸ� Ű���� �ҷ��´�.
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

        // ���ؽ���, �ε��� ī����
        const uint32_t vertex_count = assimp_mesh->mNumVertices;
        const uint32_t index_count = assimp_mesh->mNumFaces * 3;

        // ���ؽ� ����ŭ ���� ����
        vector<RHI_Vertex_PosTexNorTan> vertices = vector<RHI_Vertex_PosTexNorTan>(vertex_count);
        {
            // ���ؽ� ����ŭ �ݺ��Ѵ�.
            for (uint32_t i = 0; i < vertex_count; i++)
            {
                RHI_Vertex_PosTexNorTan& vertex = vertices[i];

                // ��ġ
                const aiVector3D& pos = assimp_mesh->mVertices[i];
                vertex.pos[0] = pos.x;
                vertex.pos[1] = pos.y;
                vertex.pos[2] = pos.z;


                // �븻
                if (assimp_mesh->mNormals)
                {
                    const aiVector3D& normal = assimp_mesh->mNormals[i];
                    vertex.nor[0] = normal.x;
                    vertex.nor[1] = normal.y;
                    vertex.nor[2] = normal.z;
                }

                // ź��Ʈ
                if (assimp_mesh->mTangents)
                {
                    const aiVector3D& tangent = assimp_mesh->mTangents[i];
                    vertex.tan[0] = tangent.x;
                    vertex.tan[1] = tangent.y;
                    vertex.tan[2] = tangent.z;
                }

                // UV ��ǥ
                const uint32_t uv_channel = 0;
                if (assimp_mesh->HasTextureCoords(uv_channel))
                {
                    const auto& tex_coords = assimp_mesh->mTextureCoords[uv_channel][i];
                    vertex.tex[0] = tex_coords.x;
                    vertex.tex[1] = tex_coords.y;
                }
            }
        }

        // �ε��� �߰�
        vector<uint32_t> indices = vector<uint32_t>(index_count);
        {
            // ���� ����ŭ �ݺ��Ѵ�.
            for (uint32_t face_index = 0; face_index < assimp_mesh->mNumFaces; face_index++)
            {
                const aiFace& face = assimp_mesh->mFaces[face_index];
                // �ּ� ������ �ﰢ���̴� ���� 3�� �����ش�.
                const uint32_t indices_index = (face_index * 3);
                indices[indices_index + 0] = face.mIndices[0];
                indices[indices_index + 1] = face.mIndices[1];
                indices[indices_index + 2] = face.mIndices[2];
            }
        }

        // �ٿ�� �ڽ� ����
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

        // ���׸��� ����
        if (params.scene->HasMaterials())
        {
            const aiMaterial* assimp_material = params.scene->mMaterials[assimp_mesh->mMaterialIndex];
            // ���׸����� �ҷ����� �߰��Ѵ�.
            shared_ptr<Material> material = load_material(m_Context, assimp_material, params);
            params.model->AddMaterial(material, entity_parent->GetSharedPtr());
        }

        // ��
        LoadBones(assimp_mesh, params);
    }

    void ModelImporter::LoadBones(const aiMesh* assimp_mesh, const sModelParams& params)
    {
    }
}