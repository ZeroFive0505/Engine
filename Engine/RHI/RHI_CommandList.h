#pragma once

#include <array>
#include <atomic>
#include "RHI_Definition.h"
#include "RHI_PipelineState.h"
#include "RHI_Descriptor.h"
#include "../Core/EngineObject.h"
#include "../Rendering/Renderer.h"

namespace PlayGround
{
    class Profiler;
    class Context;
    namespace Math
    {
        class Rectangle;
    }

    class RHI_CommandList : public EngineObject
    {
    public:
        RHI_CommandList(Context* context, void* cmd_pool_resource, const char* name);
        ~RHI_CommandList();

        void Begin();
        bool End();
        bool Submit();
        bool Reset();
        void Wait();
        void Discard();

        bool BeginRenderPass(RHI_PipelineState& pipeline_state);
        void EndRenderPass();

        void ClearPipelineStateRenderTargets(RHI_PipelineState& pipeline_state);
        void ClearRenderTarget(
            RHI_Texture* texture,
            const uint32_t color_index = 0,
            const uint32_t depth_stencil_index = 0,
            const bool storage = false,
            const Math::Vector4& clear_color = rhi_color_load,
            const float clear_depth = rhi_depth_stencil_load,
            const float clear_stencil = rhi_depth_stencil_load
        );

        void Draw(uint32_t vertex_count, uint32_t vertex_start_index = 0);
        void DrawIndexed(uint32_t index_count, uint32_t index_offset = 0, uint32_t vertex_offset = 0);

        void Dispatch(uint32_t x, uint32_t y, uint32_t z, bool async = false);

        void Blit(RHI_Texture* source, RHI_Texture* destination);
        void Blit(const std::shared_ptr<RHI_Texture>& source, const std::shared_ptr<RHI_Texture>& destination) { Blit(source.get(), destination.get()); }

        void SetViewport(const RHI_Viewport& viewport) const;

        void SetScissorRectangle(const Math::Rectangle& scissor_rectangle) const;

        void SetBufferVertex(const RHI_VertexBuffer* buffer, const uint64_t offset = 0);

        void SetBufferIndex(const RHI_IndexBuffer* buffer, const uint64_t offset = 0);

        void SetConstantBuffer(const uint32_t slot, const uint8_t scope, RHI_ConstantBuffer* constant_buffer) const;
        inline void SetConstantBuffer(const Renderer::Bindings_Cb slot, const uint8_t scope, const std::shared_ptr<RHI_ConstantBuffer>& constant_buffer) const { SetConstantBuffer(static_cast<uint32_t>(slot), scope, constant_buffer.get()); }

        void SetSampler(const uint32_t slot, RHI_Sampler* sampler) const;
        inline void SetSampler(const uint32_t slot, const std::shared_ptr<RHI_Sampler>& sampler) const { SetSampler(slot, sampler.get()); }

        void SetTexture(const uint32_t slot, RHI_Texture* texture, const int mip = -1, bool ranged = false, const bool uav = false);
        inline void SetTexture(const Renderer::Bindings_Uav slot, RHI_Texture* texture, const int mip = -1, const bool ranged = false) { SetTexture(static_cast<uint32_t>(slot), texture, mip, ranged, true); }
        inline void SetTexture(const Renderer::Bindings_Uav slot, const std::shared_ptr<RHI_Texture>& texture, const int mip = -1, const bool ranged = false) { SetTexture(static_cast<uint32_t>(slot), texture.get(), mip, ranged, true); }
        inline void SetTexture(const Renderer::Bindings_Srv slot, RHI_Texture* texture, const int mip = -1, const bool ranged = false) { SetTexture(static_cast<uint32_t>(slot), texture, mip, ranged, false); }
        inline void SetTexture(const Renderer::Bindings_Srv slot, const std::shared_ptr<RHI_Texture>& texture, const int mip = -1, const bool ranged = false) { SetTexture(static_cast<uint32_t>(slot), texture.get(), mip, ranged, false); }

        void SetStructuredBuffer(const uint32_t slot, RHI_StructuredBuffer* structured_buffer) const;
        inline void SetStructuredBuffer(const Renderer::Bindings_Sb slot, const std::shared_ptr<RHI_StructuredBuffer>& structured_buffer) const { SetStructuredBuffer(static_cast<uint32_t>(slot), structured_buffer.get()); }

        void StartMarker(const char* name);
        void EndMarker();

        void Timestamp_Start(void* query);
        void Timestamp_End(void* query);
        float Timestamp_GetDuration(void* query_start, void* query_end, const uint32_t pass_index);

        static uint32_t Gpu_GetMemory(RHI_Device* rhi_device);
        static uint32_t Gpu_GetMemoryUsed(RHI_Device* rhi_device);

        inline const RHI_CommandListState GetState() const { return m_state; }
        bool IsExecuting();

        inline RHI_Semaphore* GetSemaphoreProccessed() { return m_proccessed_semaphore.get(); }

        inline void* GetResource() const { return m_resource; }

    private:
        void Timeblock_Start(const char* name, const bool profile, const bool gpu_markers);
        void Timeblock_End();

        void OnDraw();
        void UnbindOutputTextures();

        void Descriptors_GetLayoutFromPipelineState(RHI_PipelineState& pipeline_state);
        void Descriptors_GetDescriptorsFromPipelineState(RHI_PipelineState& pipeline_state, std::vector<RHI_Descriptor>& descriptors);

        RHI_Pipeline* m_pipeline = nullptr;
        Renderer* m_renderer = nullptr;
        RHI_Device* m_rhi_device = nullptr;
        Profiler* m_profiler = nullptr;
        void* m_resource = nullptr;
        std::atomic<bool> m_discard = false;
        bool m_is_render_pass_active = false;
        bool m_pipeline_dirty = false;
        std::atomic<RHI_CommandListState> m_state = RHI_CommandListState::Idle;
        static const uint8_t m_resource_array_length_max = 16;
        static bool m_memory_query_support;
        std::mutex m_mutex_reset;

        std::shared_ptr<RHI_Fence> m_proccessed_fence;
        std::shared_ptr<RHI_Semaphore> m_proccessed_semaphore;

        std::unordered_map<std::size_t, std::shared_ptr<RHI_DescriptorSetLayout>> m_descriptor_set_layouts;
        RHI_DescriptorSetLayout* m_descriptor_layout_current = nullptr;

        RHI_PipelineState m_pipeline_state;
        static std::unordered_map<uint32_t, std::shared_ptr<RHI_Pipeline>> m_pipelines;

        struct OutputTexture
        {
            RHI_Texture* texture = nullptr;
            uint32_t slot;
            int mip;
            bool ranged;
        };
        std::array<OutputTexture, m_resource_array_length_max> m_output_textures;
        uint32_t m_output_textures_index = 0;

        void* m_query_pool = nullptr;
        uint32_t m_timestamp_index = 0;
        static const uint32_t m_max_timestamps = 512;
        std::array<uint64_t, m_max_timestamps> m_timestamps;

        uint64_t m_vertex_buffer_id = 0;
        uint64_t m_vertex_buffer_offset = 0;
        uint64_t m_index_buffer_id = 0;
        uint64_t m_index_buffer_offset = 0;
	};
}

