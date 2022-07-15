#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "../Core/EngineObject.h"
#include "RHI_Vertex.h"
#include "RHI_Descriptor.h"

namespace PlayGround
{
	class Context;

    // RHI Ω¶¿Ã¥ı
	class RHI_Shader : public EngineObject
	{
    public:
        RHI_Shader() = default;
        RHI_Shader(Context* context, const RHI_Vertex_Type vertex_type = RHI_Vertex_Type::Undefined);
        ~RHI_Shader();

        void Compile(const RHI_Shader_Type type, const std::string& file_path, bool async);
        inline Shader_Compilation_State GetCompilationState() const { return m_compilation_state; }
        inline bool IsCompiled()                              const { return m_compilation_state == Shader_Compilation_State::Succeeded; }

        void* GetResource() const;
        inline bool HasResource()  const { return m_resource != nullptr; }

        void LoadSource(const std::string& file_path);
        inline const std::vector<std::string>& GetNames()     const { return m_names; }
        inline const std::vector<std::string>& GetFilePaths() const { return m_file_paths; }
        inline const std::vector<std::string>& GetSources()   const { return m_sources; }
        void SetSource(const uint32_t index, const std::string& source);

        inline void AddDefine(const std::string& define, const std::string& value = "1") { m_defines[define] = value; }
        inline auto& GetDefines() const { return m_defines; }

        inline const uint32_t GetVertexSize() const;
        inline const std::vector<RHI_Descriptor>& GetDescriptors()      const { return m_descriptors; }
        inline const std::shared_ptr<RHI_InputLayout>& GetInputLayout() const { return m_input_layout; } // only valid for vertex shader
        inline const auto& GetFilePath()                                const { return m_file_path; }
        RHI_Shader_Type GetShaderStage()                         const { return m_shader_type; }
        const char* GetEntryPoint()                              const;
        const char* GetTargetProfile()                           const;

    protected:
        std::shared_ptr<RHI_Device> m_rhi_device;

    private:
        void ParseSource(const std::string& file_path);
        void* Compile2();
        void Reflect(const RHI_Shader_Type shader_type, const uint32_t* ptr, uint32_t size);

        std::string m_file_path;
        std::string m_source;
        std::vector<std::string> m_names;               
        std::vector<std::string> m_file_paths;          
        std::vector<std::string> m_sources;             
        std::vector<std::string> m_file_paths_multiple; 
        std::unordered_map<std::string, std::string> m_defines;
        std::vector<RHI_Descriptor> m_descriptors;
        std::shared_ptr<RHI_InputLayout> m_input_layout;
        std::atomic<Shader_Compilation_State> m_compilation_state = Shader_Compilation_State::Idle;
        RHI_Shader_Type m_shader_type = RHI_Shader_Unknown;
        RHI_Vertex_Type m_vertex_type = RHI_Vertex_Type::Undefined;
        void* m_resource = nullptr;
	};
}

