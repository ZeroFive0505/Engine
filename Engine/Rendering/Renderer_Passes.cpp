#include "Common.h"
#include "Renderer.h"
#include "Model.h"
#include "Grid.h"
#include "Font/Font.h"
#include "../Profiling/Profiler.h"
#include "../RHI/RHI_CommandList.h"
#include "../RHI/RHI_Implementation.h"
#include "../RHI/RHI_VertexBuffer.h"
#include "../RHI/RHI_IndexBuffer.h"
#include "../RHI/RHI_PipelineState.h"
#include "../RHI/RHI_Texture.h"
#include "../RHI/RHI_SwapChain.h"
#include "../RHI/RHI_Shader.h"
#include "../World/Entity.h"
#include "../World/Components/Camera.h"
#include "../World/Components/Light.h"
#include "../World/Components/Transform.h"
#include "../World/Components/Renderable.h"
#include "../World/Components/ReflectionProbe.h"
#include "../World/World.h"
#include "../World/TransformHandle/TransformHandle.h"

using namespace std;
using namespace PlayGround::Math;

#define RENDER_TARGET(rt_enum) m_render_targets[static_cast<uint8_t>(rt_enum)]

namespace PlayGround
{
    void Renderer::SetGlobalShaderResources(RHI_CommandList* cmd_list) const
    {
        cmd_list->SetConstantBuffer(Renderer::Bindings_Cb::frame, RHI_Shader_Vertex | RHI_Shader_Pixel | RHI_Shader_Compute, m_cb_frame_gpu);
        cmd_list->SetConstantBuffer(Renderer::Bindings_Cb::uber, RHI_Shader_Vertex | RHI_Shader_Pixel | RHI_Shader_Compute, m_cb_uber_gpu);
        cmd_list->SetConstantBuffer(Renderer::Bindings_Cb::light, RHI_Shader_Compute, m_cb_light_gpu);
        cmd_list->SetConstantBuffer(Renderer::Bindings_Cb::material, RHI_Shader_Pixel | RHI_Shader_Compute, m_cb_material_gpu);

        cmd_list->SetSampler(0, m_sampler_compare_depth);
        cmd_list->SetSampler(1, m_sampler_point_clamp);
        cmd_list->SetSampler(2, m_sampler_point_wrap);
        cmd_list->SetSampler(3, m_sampler_bilinear_clamp);
        cmd_list->SetSampler(4, m_sampler_bilinear_wrap);
        cmd_list->SetSampler(5, m_sampler_trilinear_clamp);
        cmd_list->SetSampler(6, m_sampler_anisotropic_wrap);

        cmd_list->SetTexture(Renderer::Bindings_Srv::noise_normal, m_tex_default_noise_normal);
        cmd_list->SetTexture(Renderer::Bindings_Srv::noise_blue, m_tex_default_noise_blue);
    }

    void Renderer::Pass_Main(RHI_CommandList* cmd_list)
    {
        ASSERT(cmd_list != nullptr);
        ASSERT(cmd_list->GetState() == RHI_CommandListState::Recording);

        SCOPED_TIME_BLOCK(m_profiler);

        RHI_Texture* rt1 = RENDER_TARGET(RenderTarget::Frame_Render).get();
        RHI_Texture* rt2 = RENDER_TARGET(RenderTarget::Frame_Render_2).get();
        RHI_Texture* rt_output = RENDER_TARGET(RenderTarget::Frame_Output).get();

        if (!m_camera)
        {
            m_cmd_current->ClearRenderTarget(rt_output, 0, 0, false, Vector4(0.0f, 0.0f, 0.0f, 1.0f));
        }
        else if (m_entities[ObjectType::GeometryOpaque].empty() && m_entities[ObjectType::GeometryTransparent].empty() && m_entities[ObjectType::Light].empty())
        {
            m_cmd_current->ClearRenderTarget(rt_output, 0, 0, false, m_camera->GetClearColor());
        }
        else
        {
            Pass_UpdateFrameBuffer(cmd_list);

            Pass_BrdfSpecularLut(cmd_list);

            const bool do_transparent_pass = !m_entities[ObjectType::GeometryTransparent].empty();

            {
                Pass_ShadowMaps(cmd_list, false);
                if (do_transparent_pass)
                {
                    Pass_ShadowMaps(cmd_list, true);
                }
            }

            Pass_ReflectionProbes(cmd_list);

            {
                bool is_transparent_pass = false;

                Pass_Depth_Prepass(cmd_list);
                Pass_GBuffer(cmd_list, is_transparent_pass);
                Pass_Ssao(cmd_list);
                Pass_Ssr(cmd_list, rt1);
                Pass_Light(cmd_list, is_transparent_pass); 
                Pass_Light_Composition(cmd_list, rt1, is_transparent_pass); 
                Pass_Light_ImageBased(cmd_list, rt1, is_transparent_pass);
            }

            if (do_transparent_pass)
            {
                cmd_list->Blit(rt1, rt2);

        
                const bool luminance_antiflicker = true;
                Pass_AMD_FidelityFX_SinglePassDownsampler(cmd_list, rt2, luminance_antiflicker);

               
                for (uint32_t i = 1; i < rt2->GetMipCount(); i++)
                {
                    const bool depth_aware = false;
                    const float sigma = 2.0f;
                    const float pixel_stride = 1.0;
                    Pass_Blur_Gaussian(cmd_list, rt2, depth_aware, sigma, pixel_stride, i);
                }

                bool is_transparent_pass = true;

                Pass_GBuffer(cmd_list, is_transparent_pass);
                Pass_Light(cmd_list, is_transparent_pass);
                Pass_Light_Composition(cmd_list, rt1, is_transparent_pass);
                Pass_Light_ImageBased(cmd_list, rt1, is_transparent_pass);
            }

            Pass_PostProcess(cmd_list);
        }

       
        Pass_DebugMeshes(cmd_list, rt_output);
        Pass_Outline(cmd_list, rt_output);
        Pass_Lines(cmd_list, rt_output);
        Pass_TransformHandle(cmd_list, rt_output);
        Pass_Icons(cmd_list, rt_output);
        Pass_PeformanceMetrics(cmd_list, rt_output);

       
        rt_output->SetLayout(RHI_Image_Layout::Shader_Read_Only_Optimal, cmd_list);
    }

    void Renderer::Pass_UpdateFrameBuffer(RHI_CommandList* cmd_list)
    {
        static RHI_PipelineState pso;
        pso.profile = false;
        pso.gpu_marker = false;
        pso.pass_name = "Pass_UpdateFrameBuffer";

        if (cmd_list->BeginRenderPass(pso))
        {
            Update_Cb_Frame(cmd_list);
            cmd_list->EndRenderPass();
        }

    }

    void Renderer::Pass_ShadowMaps(RHI_CommandList* cmd_list, const bool is_transparent_pass)
    {

        RHI_Shader* shader_v = m_shaders[Renderer::Shader::Depth_Light_V].get();
        RHI_Shader* shader_p = m_shaders[Renderer::Shader::Depth_Light_P].get();
        if (!shader_v->IsCompiled() || !shader_p->IsCompiled())
            return;

        const vector<Entity*>& entities = m_entities[is_transparent_pass ? ObjectType::GeometryTransparent : ObjectType::GeometryOpaque];
        if (entities.empty())
            return;

        const auto& entities_light = m_entities[ObjectType::Light];
        for (uint32_t light_index = 0; light_index < entities_light.size(); light_index++)
        {
            const Light* light = entities_light[light_index]->GetComponent<Light>();

            if (!light)
                continue;

            if (!light->GetShadowsEnabled() || light->GetIntensity() == 0.0f)
                continue;

            if (is_transparent_pass && !light->GetShadowsTransparentEnabled())
                continue;

            RHI_Texture* tex_depth = light->GetDepthTexture();
            RHI_Texture* tex_color = light->GetColorTexture();
            if (!tex_depth)
                continue;

            static RHI_PipelineState pso;
            pso.shader_vertex = shader_v;
            pso.shader_pixel = is_transparent_pass ? shader_p : nullptr;
            pso.blend_state = is_transparent_pass ? m_blend_alpha.get() : m_blend_disabled.get();
            pso.depth_stencil_state = is_transparent_pass ? m_depth_stencil_r_off.get() : m_depth_stencil_rw_off.get();
            pso.render_target_color_textures[0] = tex_color;
            pso.render_target_depth_texture = tex_depth;
            pso.clear_stencil = rhi_depth_stencil_dont_care;
            pso.viewport = tex_depth->GetViewport();
            pso.primitive_topology = RHI_PrimitiveTopology_Mode::TriangleList;
            pso.pass_name = is_transparent_pass ? "Pass_ShadowMaps_Color" : "Pass_ShadowMaps_Depth";

            for (uint32_t array_index = 0; array_index < tex_depth->GetArrayLength(); array_index++)
            {
                pso.render_target_color_texture_array_index = array_index;
                pso.render_target_depth_stencil_texture_array_index = array_index;

                pso.clear_color[0] = Vector4::One;
                pso.clear_depth = is_transparent_pass ? rhi_depth_stencil_load : GetClearDepth();

                const Matrix& view_projection = light->GetViewMatrix(array_index) * light->GetProjectionMatrix(array_index);

                if (light->GetLightType() == LightType::Directional)
                {
                    // "Pancaking" - https://www.gamedev.net/forums/topic/639036-shadow-mapping-and-high-up-objects/
                    pso.rasterizer_state = m_rasterizer_light_directional.get();
                }
                else
                {
                    pso.rasterizer_state = m_rasterizer_light_point_spot.get();
                }

                bool render_pass_active = false;
                uint64_t m_set_material_id = 0;

                for (uint32_t entity_index = 0; entity_index < static_cast<uint32_t>(entities.size()); entity_index++)
                {
                    Entity* entity = entities[entity_index];

                    Renderable* renderable = entity->GetRenderable();
                    if (!renderable)
                        continue;

                    if (!renderable->GetCastShadows())
                        continue;

                    Model* model = renderable->GeometryModel();
                    if (!model || !model->GetVertexBuffer() || !model->GetIndexBuffer())
                        continue;

                    Material* material = renderable->GetMaterial();
                    if (!material)
                        continue;

                    if (!light->IsInViewFrustum(renderable, array_index))
                        continue;

                    if (!render_pass_active)
                    {
                        render_pass_active = cmd_list->BeginRenderPass(pso);
                    }

                    if (is_transparent_pass && m_set_material_id != material->GetObjectID())
                    {
                        RHI_Texture* tex_albedo = material->GetTexturePtr(Material_Color);
                        cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex_albedo ? tex_albedo : m_tex_default_white.get());

                        m_cb_uber_cpu.mat_color = material->GetColorAlbedo();
                        m_cb_uber_cpu.mat_tiling_uv = material->GetTiling();
                        m_cb_uber_cpu.mat_offset_uv = material->GetOffset();

                        m_set_material_id = material->GetObjectID();
                    }

                    cmd_list->SetBufferIndex(model->GetIndexBuffer());
                    cmd_list->SetBufferVertex(model->GetVertexBuffer());

                    m_cb_uber_cpu.transform = entity->GetTransform()->GetMatrix() * view_projection;
                    Update_Cb_Uber(cmd_list);

                    cmd_list->DrawIndexed(renderable->GeometryIndexCount(), renderable->GeometryIndexOffset(), renderable->GeometryVertexOffset());
                }

                if (render_pass_active)
                {
                    cmd_list->EndRenderPass();
                }
            }
        }
    }

    void Renderer::Pass_ReflectionProbes(RHI_CommandList* cmd_list)
    {
        RHI_Shader* shader_v = m_shaders[Renderer::Shader::Reflection_Probe_V].get();
        RHI_Shader* shader_p = m_shaders[Renderer::Shader::Reflection_Probe_P].get();
        if (!shader_v->IsCompiled() || !shader_p->IsCompiled())
            return;

        const vector<Entity*>& probes = m_entities[ObjectType::ReflectionProbe];
        if (probes.empty())
            return;

        const vector<Entity*>& renderables = m_entities[ObjectType::GeometryOpaque];
        if (renderables.empty())
            return;

        const vector<Entity*>& lights = m_entities[ObjectType::Light];
        if (lights.empty())
            return;

        for (uint32_t probe_index = 0; probe_index < static_cast<uint32_t>(probes.size()); probe_index++)
        {
            ReflectionProbe* probe = probes[probe_index]->GetComponent<ReflectionProbe>();
            if (!probe || !probe->GetNeedsToUpdate())
                continue;

            static RHI_PipelineState pso;
            pso.shader_vertex = shader_v;
            pso.shader_pixel = shader_p;
            pso.rasterizer_state = m_rasterizer_cull_back_solid.get();
            pso.blend_state = m_blend_additive.get();
            pso.depth_stencil_state = m_depth_stencil_rw_off.get();
            pso.render_target_color_textures[0] = probe->GetColorTexture();
            pso.render_target_depth_texture = probe->GetDepthTexture();
            pso.clear_color[0] = Vector4::Zero;
            pso.clear_depth = GetClearDepth();
            pso.clear_stencil = rhi_depth_stencil_dont_care;
            pso.viewport = probe->GetColorTexture()->GetViewport();
            pso.pass_name = "Pass_ReflectionProbes";
            pso.primitive_topology = RHI_PrimitiveTopology_Mode::TriangleList;

            uint32_t index_start = probe->GetUpdateFaceStartIndex();
            uint32_t index_end = (index_start + probe->GetUpdateFaceCount()) % 7;
            for (uint32_t face_index = index_start; face_index < index_end; face_index++)
            {
                pso.render_target_color_texture_array_index = face_index;

                cmd_list->BeginRenderPass(pso);

                Matrix view_projection = probe->GetViewMatrix(face_index) * probe->GetProjectionMatrix();

                for (uint32_t index_renderable = 0; index_renderable < static_cast<uint32_t>(renderables.size()); index_renderable++)
                {
                    Entity* entity = renderables[index_renderable];

                    for (uint32_t index_light = 0; index_light < static_cast<uint32_t>(lights.size()); index_light++)
                    {
                        if (Light* light = lights[index_light]->GetComponent<Light>())
                        {
                            if (light->GetIntensity() != 0)
                            {
                                Renderable* renderable = entity->GetRenderable();
                                if (!renderable)
                                    continue;

                                Material* material = renderable->GetMaterial();
                                if (!material)
                                    continue;

                                Model* model = renderable->GeometryModel();
                                if (!model || !model->GetVertexBuffer() || !model->GetIndexBuffer())
                                    continue;

                                if (!probe->IsInViewFrustum(renderable, face_index))
                                    continue;

                                cmd_list->SetBufferIndex(model->GetIndexBuffer());
                                cmd_list->SetBufferVertex(model->GetVertexBuffer());

                                cmd_list->SetTexture(Renderer::Bindings_Srv::material_albedo, material->GetTexturePtr(Material_Color));
                                cmd_list->SetTexture(Renderer::Bindings_Srv::material_roughness, material->GetTexturePtr(Material_Metallic));
                                cmd_list->SetTexture(Renderer::Bindings_Srv::material_metallic, material->GetTexturePtr(Material_Metallic));

                                m_cb_uber_cpu.mat_color = material->GetColorAlbedo();
                                m_cb_uber_cpu.mat_textures = 0;
                                m_cb_uber_cpu.mat_textures |= material->HasTexture(Material_Color) ? (1U << 2) : 0;
                                m_cb_uber_cpu.mat_textures |= material->HasTexture(Material_Roughness) ? (1U << 3) : 0;
                                m_cb_uber_cpu.mat_textures |= material->HasTexture(Material_Metallic) ? (1U << 4) : 0;

                                m_cb_uber_cpu.transform = entity->GetTransform()->GetMatrix() * view_projection;
                                Update_Cb_Uber(cmd_list);

                                Update_Cb_Light(cmd_list, light, RHI_Shader_Pixel);

                                cmd_list->DrawIndexed(renderable->GeometryIndexCount(), renderable->GeometryIndexOffset(), renderable->GeometryVertexOffset());
                            }
                        }
                    }
                }
                cmd_list->EndRenderPass();
            }
        }
    }

    void Renderer::Pass_Depth_Prepass(RHI_CommandList* cmd_list)
    {
        if ((m_options & Renderer::Option::DepthPrepass) == 0)
            return;

        RHI_Shader* shader_v = m_shaders[Renderer::Shader::Depth_Prepass_V].get();
        RHI_Shader* shader_p = m_shaders[Renderer::Shader::Depth_Prepass_P].get();
        if (!shader_v->IsCompiled() || !shader_p->IsCompiled())
            return;

        RHI_Texture* tex_depth = RENDER_TARGET(RenderTarget::Gbuffer_Depth).get();
        const auto& entities = m_entities[ObjectType::GeometryOpaque];

        static RHI_PipelineState pso;
        pso.shader_vertex = shader_v;
        pso.shader_pixel = shader_p;
        pso.rasterizer_state = m_rasterizer_cull_back_solid.get();
        pso.blend_state = m_blend_disabled.get();
        pso.depth_stencil_state = m_depth_stencil_rw_off.get();
        pso.render_target_depth_texture = tex_depth;
        pso.clear_depth = GetClearDepth();
        pso.viewport = tex_depth->GetViewport();
        pso.primitive_topology = RHI_PrimitiveTopology_Mode::TriangleList;
        pso.pass_name = "Pass_Depth_Prepass";

        if (cmd_list->BeginRenderPass(pso))
        {
            uint64_t currently_bound_geometry = 0;

            for (const auto& entity : entities)
            {
                Renderable* renderable = entity->GetRenderable();
                if (!renderable)
                    continue;

                Material* material = renderable->GetMaterial();
                if (!material)
                    continue;

                Model* model = renderable->GeometryModel();
                if (!model || !model->GetVertexBuffer() || !model->GetIndexBuffer())
                    continue;

                Transform* transform = entity->GetTransform();
                if (!transform)
                    continue;

                if (!m_camera->IsInViewFrustum(renderable))
                    continue;

                if (currently_bound_geometry != model->GetObjectID())
                {
                    cmd_list->SetBufferIndex(model->GetIndexBuffer());
                    cmd_list->SetBufferVertex(model->GetVertexBuffer());
                    currently_bound_geometry = model->GetObjectID();
                }

                cmd_list->SetTexture(Renderer::Bindings_Srv::material_albedo, material->GetTexturePtr(Material_Color));
                cmd_list->SetTexture(Renderer::Bindings_Srv::material_mask, material->GetTexturePtr(Material_AlphaMask));

                m_cb_uber_cpu.transform = transform->GetMatrix();
                m_cb_uber_cpu.mat_color.w = material->HasTexture(Material_Color) ? 1.0f : 0.0f;
                m_cb_uber_cpu.is_transparent_pass = material->HasTexture(Material_AlphaMask);
                Update_Cb_Uber(cmd_list);

                cmd_list->DrawIndexed(renderable->GeometryIndexCount(), renderable->GeometryIndexOffset(), renderable->GeometryVertexOffset());
            }

            cmd_list->EndRenderPass();
        }
    }

    void Renderer::Pass_GBuffer(RHI_CommandList* cmd_list, const bool is_transparent_pass)
    {
        RHI_Shader* shader_v = m_shaders[Renderer::Shader::Gbuffer_V].get();
        RHI_Shader* shader_p = m_shaders[Renderer::Shader::Gbuffer_P].get();
        if (!shader_v->IsCompiled() || !shader_p->IsCompiled())
            return;

        RHI_Texture* tex_albedo = RENDER_TARGET(RenderTarget::Gbuffer_Albedo).get();
        RHI_Texture* tex_normal = RENDER_TARGET(RenderTarget::Gbuffer_Normal).get();
        RHI_Texture* tex_material = RENDER_TARGET(RenderTarget::Gbuffer_Material).get();
        RHI_Texture* tex_velocity = RENDER_TARGET(!m_is_odd_frame ? RenderTarget::Gbuffer_Velocity : RenderTarget::Gbuffer_Velocity_2).get();
        RHI_Texture* tex_depth = RENDER_TARGET(RenderTarget::Gbuffer_Depth).get();

        bool depth_prepass = GetOption(Renderer::Option::DepthPrepass);
        bool wireframe = GetOption(Renderer::Option::Debug_Wireframe);

        static Vector4 clear_color_sky = Vector4(0.0f, 0.0f, 0.0f, 1.0f);

        RHI_PipelineState pso;
        pso.shader_vertex = shader_v;
        pso.shader_pixel = shader_p;
        pso.blend_state = m_blend_disabled.get();
        pso.rasterizer_state = wireframe ? m_rasterizer_cull_back_wireframe.get() : m_rasterizer_cull_back_solid.get();
        pso.depth_stencil_state = is_transparent_pass ? m_depth_stencil_rw_w.get() : (depth_prepass ? m_depth_stencil_r_off.get() : m_depth_stencil_rw_off.get());
        pso.render_target_color_textures[0] = tex_albedo;
        pso.clear_color[0] = !is_transparent_pass ? clear_color_sky : rhi_color_load;
        pso.render_target_color_textures[1] = tex_normal;
        pso.clear_color[1] = !is_transparent_pass ? clear_color_sky : rhi_color_load;
        pso.render_target_color_textures[2] = tex_material;
        pso.clear_color[2] = !is_transparent_pass ? clear_color_sky : rhi_color_load;
        pso.render_target_color_textures[3] = tex_velocity;
        pso.clear_color[3] = !is_transparent_pass ? clear_color_sky : rhi_color_load;
        pso.render_target_depth_texture = tex_depth;
        pso.clear_depth = (is_transparent_pass || depth_prepass) ? rhi_depth_stencil_load : GetClearDepth();
        pso.clear_stencil = rhi_depth_stencil_dont_care;
        pso.viewport = tex_albedo->GetViewport();
        pso.primitive_topology = RHI_PrimitiveTopology_Mode::TriangleList;
        pso.pass_name = is_transparent_pass ? "GBuffer_Transparent" : "GBuffer_Opaque";

        uint32_t material_index = 0;
        uint64_t material_bound_id = 0;
        m_material_instances.fill(nullptr);
        auto& entities = m_entities[is_transparent_pass ? ObjectType::GeometryTransparent : ObjectType::GeometryOpaque];

        if (cmd_list->BeginRenderPass(pso))
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(entities.size()); i++)
            {
                Entity* entity = entities[i];

                Renderable* renderable = entity->GetRenderable();
                if (!renderable)
                    continue;

                Material* material = renderable->GetMaterial();
                if (!material)
                    continue;

                Model* model = renderable->GeometryModel();
                if (!model || !model->GetVertexBuffer() || !model->GetIndexBuffer())
                    continue;

                if (!m_camera->IsInViewFrustum(renderable))
                    continue;

                cmd_list->SetBufferIndex(model->GetIndexBuffer());
                cmd_list->SetBufferVertex(model->GetVertexBuffer());

                const bool firs_run = material_index == 0;
                const bool new_material = material_bound_id != material->GetObjectID();
                if (firs_run || new_material)
                {
                    material_bound_id = material->GetObjectID();

                    if (material_index + 1 < m_material_instances.size())
                    {
                        material_index++;

                        m_material_instances[material_index] = material;
                    }
                    else
                    {
                        LOG_ERROR("Material instance array has reached it's maximum capacity of %d elements. Consider increasing the size.", m_max_material_instances);
                    }

                    cmd_list->SetTexture(Renderer::Bindings_Srv::material_albedo, material->GetTexturePtr(Material_Color));
                    cmd_list->SetTexture(Renderer::Bindings_Srv::material_roughness, material->GetTexturePtr(Material_Roughness));
                    cmd_list->SetTexture(Renderer::Bindings_Srv::material_metallic, material->GetTexturePtr(Material_Metallic));
                    cmd_list->SetTexture(Renderer::Bindings_Srv::material_normal, material->GetTexturePtr(Material_Normal));
                    cmd_list->SetTexture(Renderer::Bindings_Srv::material_height, material->GetTexturePtr(Material_Height));
                    cmd_list->SetTexture(Renderer::Bindings_Srv::material_occlusion, material->GetTexturePtr(Material_Occlusion));
                    cmd_list->SetTexture(Renderer::Bindings_Srv::material_emission, material->GetTexturePtr(Material_Emission));
                    cmd_list->SetTexture(Renderer::Bindings_Srv::material_mask, material->GetTexturePtr(Material_AlphaMask));

                    m_cb_uber_cpu.mat_id = material_index;
                    m_cb_uber_cpu.mat_color = material->GetColorAlbedo();
                    m_cb_uber_cpu.mat_tiling_uv = material->GetTiling();
                    m_cb_uber_cpu.mat_offset_uv = material->GetOffset();
                    m_cb_uber_cpu.mat_roughness_mul = material->GetProperty(Material_Roughness);
                    m_cb_uber_cpu.mat_metallic_mul = material->GetProperty(Material_Metallic);
                    m_cb_uber_cpu.mat_normal_mul = material->GetProperty(Material_Normal);
                    m_cb_uber_cpu.mat_height_mul = material->GetProperty(Material_Height);
                    m_cb_uber_cpu.mat_textures = 0;
                    m_cb_uber_cpu.mat_textures |= material->HasTexture(Material_Height) ? (1U << 0) : 0;
                    m_cb_uber_cpu.mat_textures |= material->HasTexture(Material_Normal) ? (1U << 1) : 0;
                    m_cb_uber_cpu.mat_textures |= material->HasTexture(Material_Color) ? (1U << 2) : 0;
                    m_cb_uber_cpu.mat_textures |= material->HasTexture(Material_Roughness) ? (1U << 3) : 0;
                    m_cb_uber_cpu.mat_textures |= material->HasTexture(Material_Metallic) ? (1U << 4) : 0;
                    m_cb_uber_cpu.mat_textures |= material->HasTexture(Material_AlphaMask) ? (1U << 5) : 0;
                    m_cb_uber_cpu.mat_textures |= material->HasTexture(Material_Emission) ? (1U << 6) : 0;
                    m_cb_uber_cpu.mat_textures |= material->HasTexture(Material_Occlusion) ? (1U << 7) : 0;
                }

                if (Transform* transform = entity->GetTransform())
                {
                    m_cb_uber_cpu.transform = transform->GetMatrix();
                    m_cb_uber_cpu.transform_previous = transform->GetPrevMatrix();

                    transform->SetPrevMatrix(m_cb_uber_cpu.transform);

                    Update_Cb_Uber(cmd_list);
                }

                cmd_list->DrawIndexed(renderable->GeometryIndexCount(), renderable->GeometryIndexOffset(), renderable->GeometryVertexOffset());

                if (m_profiler)
                {
                    m_profiler->m_Renderer_meshes_rendered++;
                }
            }

            cmd_list->EndRenderPass();
        }
    }

    void Renderer::Pass_Ssao(RHI_CommandList* cmd_list)
    {
        if ((m_options & Renderer::Option::Ssao) == 0)
            return;

        RHI_Shader* shader_c = m_shaders[Renderer::Shader::Ssao_C].get();
        if (!shader_c->IsCompiled())
            return;

        RHI_Texture* tex_ssao = RENDER_TARGET(RenderTarget::Ssao).get();
        RHI_Texture* tex_ssao_gi = RENDER_TARGET(RenderTarget::Ssao_Gi).get();

        cmd_list->StartMarker("Pass_Ssao");

        static RHI_PipelineState pso;
        pso.shader_compute = shader_c;
        pso.pass_name = "Pass_Ssao";

        if (cmd_list->BeginRenderPass(pso))
        {
            m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_ssao->GetWidth()), static_cast<float>(tex_ssao->GetHeight()));
            Update_Cb_Uber(cmd_list);

            const uint32_t thread_group_count_x = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_ssao->GetWidth()) / m_thread_group_count));
            const uint32_t thread_group_count_y = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_ssao->GetHeight()) / m_thread_group_count));
            const uint32_t thread_group_count_z = 1;
            const bool async = false;

            cmd_list->SetTexture(Renderer::Bindings_Uav::rgba, tex_ssao);
            cmd_list->SetTexture(Renderer::Bindings_Uav::rgba2, tex_ssao_gi);
            cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_albedo, RENDER_TARGET(RenderTarget::Gbuffer_Albedo));
            cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_normal, RENDER_TARGET(RenderTarget::Gbuffer_Normal));
            cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_depth, RENDER_TARGET(RenderTarget::Gbuffer_Depth));
            cmd_list->SetTexture(Renderer::Bindings_Srv::light_diffuse, RENDER_TARGET(RenderTarget::Light_Diffuse));

            cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
            cmd_list->EndRenderPass();
        }

        const bool depth_aware = true;
        const float sigma = 4.0f;
        const float pixel_stride = 2.0f;
        Pass_Blur_Gaussian(cmd_list, tex_ssao, depth_aware, sigma, pixel_stride);
        Pass_Blur_Gaussian(cmd_list, tex_ssao_gi, depth_aware, sigma, pixel_stride);

        cmd_list->EndMarker();
    }

    void Renderer::Pass_Ssr(RHI_CommandList* cmd_list, RHI_Texture* tex_in)
    {
        if ((m_options & Renderer::Option::ScreenSpaceReflections) == 0)
            return;

        RHI_Shader* shader_c = m_shaders[Renderer::Shader::Ssr_C].get();
        if (!shader_c->IsCompiled())
            return;

        RHI_Texture* tex_ssr = RENDER_TARGET(RenderTarget::Ssr).get();

        cmd_list->StartMarker("Pass_Ssr");

        static RHI_PipelineState pso;
        pso.shader_compute = shader_c;
        pso.pass_name = "Pass_Ssr";

        if (cmd_list->BeginRenderPass(pso))
        {
            m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_ssr->GetWidth()), static_cast<float>(tex_ssr->GetHeight()));
            Update_Cb_Uber(cmd_list);

            const uint32_t thread_group_count_x = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_ssr->GetWidth()) / m_thread_group_count));
            const uint32_t thread_group_count_y = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_ssr->GetHeight()) / m_thread_group_count));
            const uint32_t thread_group_count_z = 1;
            const bool async = false;

            cmd_list->SetTexture(Renderer::Bindings_Uav::rgba, tex_ssr);
            cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex_in);  
            cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_albedo, RENDER_TARGET(RenderTarget::Gbuffer_Albedo));
            cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_normal, RENDER_TARGET(RenderTarget::Gbuffer_Normal));
            cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_depth, RENDER_TARGET(RenderTarget::Gbuffer_Depth));
            cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_material, RENDER_TARGET(RenderTarget::Gbuffer_Material));
            cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_velocity, RENDER_TARGET(RenderTarget::Gbuffer_Velocity));

            cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
            cmd_list->EndRenderPass();
        }

        const bool luminance_antiflicker = false;
        Pass_AMD_FidelityFX_SinglePassDownsampler(cmd_list, tex_ssr, luminance_antiflicker);

        for (uint32_t i = 1; i < tex_ssr->GetMipCount(); i++)
        {
            const bool depth_aware = true;
            const float sigma = 2.0f;
            const float pixel_stride = 1.0;
            Pass_Blur_Gaussian(cmd_list, tex_ssr, depth_aware, sigma, pixel_stride, i);
        }

        cmd_list->EndMarker();
    }

    void Renderer::Pass_Light(RHI_CommandList* cmd_list, const bool is_transparent_pass)
    {
        RHI_Shader* shader_c = m_shaders[Renderer::Shader::Light_C].get();
        if (!shader_c->IsCompiled())
            return;

        const vector<Entity*>& entities = m_entities[ObjectType::Light];
        if (entities.empty())
            return;

        RHI_Texture* tex_diffuse = is_transparent_pass ? RENDER_TARGET(RenderTarget::Light_Diffuse_Transparent).get() : RENDER_TARGET(RenderTarget::Light_Diffuse).get();
        RHI_Texture* tex_specular = is_transparent_pass ? RENDER_TARGET(RenderTarget::Light_Specular_Transparent).get() : RENDER_TARGET(RenderTarget::Light_Specular).get();
        RHI_Texture* tex_volumetric = RENDER_TARGET(RenderTarget::Light_Volumetric).get();

        cmd_list->ClearRenderTarget(tex_diffuse, 0, 0, true, Vector4::Zero);
        cmd_list->ClearRenderTarget(tex_specular, 0, 0, true, Vector4::Zero);
        cmd_list->ClearRenderTarget(tex_volumetric, 0, 0, true, Vector4::Zero);

        static RHI_PipelineState pso;
        pso.shader_compute = shader_c;
        pso.pass_name = is_transparent_pass ? "Pass_Light_Transparent" : "Pass_Light_Opaque";

        if (cmd_list->BeginRenderPass(pso))
        {
            for (const auto& entity : entities)
            {
                if (Light* light = entity->GetComponent<Light>())
                {
                    if (light->GetIntensity() != 0)
                    {
                        cmd_list->SetTexture(Renderer::Bindings_Uav::rgb, tex_diffuse);
                        cmd_list->SetTexture(Renderer::Bindings_Uav::rgb2, tex_specular);
                        cmd_list->SetTexture(Renderer::Bindings_Uav::rgb3, tex_volumetric);
                        cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_albedo, RENDER_TARGET(RenderTarget::Gbuffer_Albedo));
                        cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_normal, RENDER_TARGET(RenderTarget::Gbuffer_Normal));
                        cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_material, RENDER_TARGET(RenderTarget::Gbuffer_Material));
                        cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_depth, RENDER_TARGET(RenderTarget::Gbuffer_Depth));
                        cmd_list->SetTexture(Renderer::Bindings_Srv::ssao, RENDER_TARGET(RenderTarget::Ssao));
                        cmd_list->SetTexture(Renderer::Bindings_Srv::ssao_gi, RENDER_TARGET(RenderTarget::Ssao_Gi));

                        {
                            RHI_Texture* tex_depth = light->GetDepthTexture();
                            RHI_Texture* tex_color = light->GetShadowsTransparentEnabled() ? light->GetColorTexture() : m_tex_default_white.get();

                            if (light->GetLightType() == LightType::Directional)
                            {
                                cmd_list->SetTexture(Renderer::Bindings_Srv::light_directional_depth, tex_depth);
                                cmd_list->SetTexture(Renderer::Bindings_Srv::light_directional_color, tex_color);
                            }
                            else if (light->GetLightType() == LightType::Point)
                            {
                                cmd_list->SetTexture(Renderer::Bindings_Srv::light_point_depth, tex_depth);
                                cmd_list->SetTexture(Renderer::Bindings_Srv::light_point_color, tex_color);
                            }
                            else if (light->GetLightType() == LightType::Spot)
                            {
                                cmd_list->SetTexture(Renderer::Bindings_Srv::light_spot_depth, tex_depth);
                                cmd_list->SetTexture(Renderer::Bindings_Srv::light_spot_color, tex_color);
                            }
                        }

                        Update_Cb_Material(cmd_list);

                        Update_Cb_Light(cmd_list, light, RHI_Shader_Compute);

                        m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_diffuse->GetWidth()), static_cast<float>(tex_diffuse->GetHeight()));
                        m_cb_uber_cpu.is_transparent_pass = is_transparent_pass;
                        Update_Cb_Uber(cmd_list);

                        const uint32_t thread_group_count_x = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_diffuse->GetWidth()) / m_thread_group_count));
                        const uint32_t thread_group_count_y = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_diffuse->GetHeight()) / m_thread_group_count));
                        const uint32_t thread_group_count_z = 1;
                        const bool async = false;

                        cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
                    }
                }
            }
            cmd_list->EndRenderPass();
        }
    }

    void Renderer::Pass_Light_Composition(RHI_CommandList* cmd_list, RHI_Texture* tex_out, const bool is_transparent_pass)
    {
        RHI_Shader* shader_c = m_shaders[Renderer::Shader::Light_Composition_C].get();
        if (!shader_c->IsCompiled())
            return;

        static RHI_PipelineState pso;
        pso.shader_compute = shader_c;
        pso.pass_name = is_transparent_pass ? "Pass_Light_Composition_Transparent" : "Pass_Light_Composition_Opaque";

        if (cmd_list->BeginRenderPass(pso))
        {
            m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_out->GetWidth()), static_cast<float>(tex_out->GetHeight()));
            m_cb_uber_cpu.is_transparent_pass = is_transparent_pass;
            Update_Cb_Uber(cmd_list);

            const uint32_t thread_group_count_x = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetWidth()) / m_thread_group_count));
            const uint32_t thread_group_count_y = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetHeight()) / m_thread_group_count));
            const uint32_t thread_group_count_z = 1;
            const bool async = false;

            cmd_list->SetTexture(Renderer::Bindings_Uav::rgba, tex_out);
            cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_albedo, RENDER_TARGET(RenderTarget::Gbuffer_Albedo));
            cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_material, RENDER_TARGET(RenderTarget::Gbuffer_Material));
            cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_normal, RENDER_TARGET(RenderTarget::Gbuffer_Normal));
            cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_depth, RENDER_TARGET(RenderTarget::Gbuffer_Depth));
            cmd_list->SetTexture(Renderer::Bindings_Srv::light_diffuse, is_transparent_pass ? RENDER_TARGET(RenderTarget::Light_Diffuse_Transparent).get() : RENDER_TARGET(RenderTarget::Light_Diffuse).get());
            cmd_list->SetTexture(Renderer::Bindings_Srv::light_specular, is_transparent_pass ? RENDER_TARGET(RenderTarget::Light_Specular_Transparent).get() : RENDER_TARGET(RenderTarget::Light_Specular).get());
            cmd_list->SetTexture(Renderer::Bindings_Srv::light_volumetric, RENDER_TARGET(RenderTarget::Light_Volumetric));
            cmd_list->SetTexture(Renderer::Bindings_Srv::frame, RENDER_TARGET(RenderTarget::Frame_Render_2)); // refraction
            cmd_list->SetTexture(Renderer::Bindings_Srv::ssao, RENDER_TARGET(RenderTarget::Ssao));
            cmd_list->SetTexture(Renderer::Bindings_Srv::environment, GetEnvironmentTexture());

            cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
            cmd_list->EndRenderPass();
        }
    }

    void Renderer::Pass_Light_ImageBased(RHI_CommandList* cmd_list, RHI_Texture* tex_out, const bool is_transparent_pass)
    {
        RHI_Shader* shader_v = m_shaders[Renderer::Shader::FullscreenTriangle_V].get();
        RHI_Shader* shader_p = m_shaders[Renderer::Shader::Light_ImageBased_P].get();
        if (!shader_v->IsCompiled() || !shader_p->IsCompiled())
            return;

        const vector<Entity*>& probes = m_entities[ObjectType::ReflectionProbe];

        static RHI_PipelineState pso;
        pso.shader_vertex = shader_v;
        pso.shader_pixel = shader_p;
        pso.rasterizer_state = m_rasterizer_cull_back_solid.get();
        pso.depth_stencil_state = m_depth_stencil_off_off.get();
        pso.blend_state = m_blend_additive.get();
        pso.render_target_color_textures[0] = tex_out;
        pso.clear_color[0] = rhi_color_load;
        pso.viewport = tex_out->GetViewport();
        pso.primitive_topology = RHI_PrimitiveTopology_Mode::TriangleList;
        pso.pass_name = is_transparent_pass ? "Pass_Light_ImageBased_Transparent" : "Pass_Light_ImageBased_Opaque";
        pso.can_use_vertex_index_buffers = false;

        if (cmd_list->BeginRenderPass(pso))
        {
            cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_albedo, RENDER_TARGET(RenderTarget::Gbuffer_Albedo));
            cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_normal, RENDER_TARGET(RenderTarget::Gbuffer_Normal));
            cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_material, RENDER_TARGET(RenderTarget::Gbuffer_Material));
            cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_depth, RENDER_TARGET(RenderTarget::Gbuffer_Depth));
            cmd_list->SetTexture(Renderer::Bindings_Srv::ssao, RENDER_TARGET(RenderTarget::Ssao));
            cmd_list->SetTexture(Renderer::Bindings_Srv::ssr, RENDER_TARGET(RenderTarget::Ssr));
            cmd_list->SetTexture(Renderer::Bindings_Srv::lutIbl, RENDER_TARGET(RenderTarget::Brdf_Specular_Lut));
            cmd_list->SetTexture(Renderer::Bindings_Srv::environment, GetEnvironmentTexture());

            if (!probes.empty())
            {
                ReflectionProbe* probe = probes[0]->GetComponent<ReflectionProbe>();

                cmd_list->SetTexture(Renderer::Bindings_Srv::reflection_probe, probe->GetColorTexture());
                m_cb_uber_cpu.extents = probe->GetExtents();
                m_cb_uber_cpu.float3 = probe->GetTransform()->GetPosition();
            }

            m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_out->GetWidth()), static_cast<float>(tex_out->GetHeight()));
            m_cb_uber_cpu.is_transparent_pass = is_transparent_pass;
            m_cb_uber_cpu.reflection_proble_available = !probes.empty();
            Update_Cb_Uber(cmd_list);

            cmd_list->Draw(3, 0);
            cmd_list->EndRenderPass();
        }
    }

    void Renderer::Pass_Blur_Gaussian(RHI_CommandList* cmd_list, RHI_Texture* tex_in, const bool depth_aware, const float sigma, const float pixel_stride, const int mip /*= -1*/)
    {
        RHI_Shader* shader_c = m_shaders[depth_aware ? Renderer::Shader::BlurGaussianBilateral_C : Renderer::Shader::BlurGaussian_C].get();
        if (!shader_c->IsCompiled())
            return;

        const bool mip_requested = mip != -1;

        if (mip_requested)
        {
            ASSERT(tex_in->HasPerMipViews());
        }

        const uint32_t width = mip_requested ? (tex_in->GetWidth() >> mip) : tex_in->GetWidth();
        const uint32_t height = mip_requested ? (tex_in->GetHeight() >> mip) : tex_in->GetHeight();

        RHI_Texture* tex_depth = RENDER_TARGET(RenderTarget::Gbuffer_Depth).get();
        RHI_Texture* tex_normal = RENDER_TARGET(RenderTarget::Gbuffer_Normal).get();
        RHI_Texture* tex_blur = RENDER_TARGET(RenderTarget::Blur).get();

        ASSERT(tex_blur->GetWidth() >= width && tex_blur->GetHeight() >= height);

        const uint32_t thread_group_count_x = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(width) / m_thread_group_count));
        const uint32_t thread_group_count_y = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(height) / m_thread_group_count));
        const uint32_t thread_group_count_z = 1;
        const bool async = false;

        {
            static RHI_PipelineState pso;
            pso.shader_compute = shader_c;
            pso.pass_name = "Pass_Blur_Gaussian_Horizontal";

            if (cmd_list->BeginRenderPass(pso))
            {
                m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(width), static_cast<float>(height));
                m_cb_uber_cpu.resolution_in = Vector2(static_cast<float>(width), static_cast<float>(height));
                m_cb_uber_cpu.blur_direction = Vector2(pixel_stride, 0.0f);
                m_cb_uber_cpu.blur_sigma = sigma;
                Update_Cb_Uber(cmd_list);

                cmd_list->SetTexture(Renderer::Bindings_Uav::rgba, tex_blur);
                cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex_in, mip);
                if (depth_aware)
                {
                    cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_depth, tex_depth);
                    cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_normal, tex_normal);
                }

                cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
                cmd_list->EndRenderPass();
            }
        }

        {
            static RHI_PipelineState pso;
            pso.shader_compute = shader_c;
            pso.pass_name = "Pass_Blur_Gaussian_Vertical";

            if (cmd_list->BeginRenderPass(pso))
            {
                m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_blur->GetWidth()), static_cast<float>(tex_blur->GetHeight()));
                m_cb_uber_cpu.blur_direction = Vector2(0.0f, pixel_stride);
                Update_Cb_Uber(cmd_list);

                cmd_list->SetTexture(Renderer::Bindings_Uav::rgba, tex_in, mip);
                cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex_blur);
                if (depth_aware)
                {
                    cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_depth, tex_depth);
                    cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_normal, tex_normal);
                }

                cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
                cmd_list->EndRenderPass();
            }
        }
    }

    void Renderer::Pass_PostProcess(RHI_CommandList* cmd_list)
    {
        shared_ptr<RHI_Texture>& rt_frame_render_1 = RENDER_TARGET(RenderTarget::Frame_Render);   
        shared_ptr<RHI_Texture>& rt_frame_render_2 = RENDER_TARGET(RenderTarget::Frame_Render_2); 
        shared_ptr<RHI_Texture>& rt_frame_output_1 = RENDER_TARGET(RenderTarget::Frame_Output);   
        shared_ptr<RHI_Texture>& rt_frame_output_2 = RENDER_TARGET(RenderTarget::Frame_Output_2); 


        uint64_t frame_output_in_id = rt_frame_output_1->GetObjectID();

        cmd_list->StartMarker("Pass_PostProcess");

        bool upsampled = false;
        {
            if (GetOption(Renderer::Option::DepthOfField))
            {
                Pass_PostProcess_DepthOfField(cmd_list, rt_frame_render_1, rt_frame_render_2);
                rt_frame_render_1.swap(rt_frame_render_2);
            }

            bool resolution_output_larger = m_resolution_output.x > m_resolution_render.x || m_resolution_output.y > m_resolution_render.y;

            if (GetOption(Renderer::Option::AntiAliasing_Taa))
            {
                if (GetOption(Renderer::Option::Upsample_TAA) && resolution_output_larger)
                {
                    Pass_PostProcess_TAA(cmd_list, rt_frame_render_1, rt_frame_output_1);
                    upsampled = true;
                }
                else
                {
                    Pass_PostProcess_TAA(cmd_list, rt_frame_render_1, rt_frame_render_2);
                    rt_frame_render_1.swap(rt_frame_render_2);
                }
            }

            if (GetOption(Renderer::Option::Upsample_AMD_FidelityFX_SuperResolution) && resolution_output_larger)
            {
                Pass_AMD_FidelityFX_SuperResolution(cmd_list, rt_frame_render_1.get(), rt_frame_output_1.get(), rt_frame_output_2.get());
                upsampled = true;
            }
        }

        if (!upsampled)
        {
            bool resolution_output_different = m_resolution_output != m_resolution_render;
            bool bilinear = resolution_output_different;

            
            Pass_Copy(cmd_list, rt_frame_render_1.get(), rt_frame_output_1.get(), bilinear);
        }

        {
            if (GetOption(Renderer::Option::MotionBlur))
            {
                Pass_PostProcess_MotionBlur(cmd_list, rt_frame_output_1, rt_frame_output_2);
                rt_frame_output_1.swap(rt_frame_output_2);
            }

            if (GetOption(Renderer::Option::Bloom))
            {
                Pass_PostProcess_Bloom(cmd_list, rt_frame_output_1, rt_frame_output_2);
                rt_frame_output_1.swap(rt_frame_output_2);
            }

            if (GetOption(Renderer::Option::Sharpening_AMD_FidelityFX_ContrastAdaptiveSharpening))
            {
                Pass_AMD_FidelityFX_ContrastAdaptiveSharpening(cmd_list, rt_frame_output_1, rt_frame_output_2);
                rt_frame_output_1.swap(rt_frame_output_2);
            }

            Pass_PostProcess_ToneMapping(cmd_list, rt_frame_output_1, rt_frame_output_2);
            rt_frame_output_1.swap(rt_frame_output_2);

            if (GetOption(Renderer::Option::Debanding))
            {
                Pass_PostProcess_Debanding(cmd_list, rt_frame_output_1, rt_frame_output_2);
                rt_frame_output_1.swap(rt_frame_output_2);
            }

            if (GetOption(Renderer::Option::AntiAliasing_Fxaa))
            {
                Pass_PostProcess_Fxaa(cmd_list, rt_frame_output_1, rt_frame_output_2);
                rt_frame_output_1.swap(rt_frame_output_2);
            }

            if (GetOption(Renderer::Option::ChromaticAberration))
            {
                Pass_PostProcess_ChromaticAberration(cmd_list, rt_frame_output_1, rt_frame_output_2);
                rt_frame_output_1.swap(rt_frame_output_2);
            }

            if (GetOption(Renderer::Option::FilmGrain))
            {
                Pass_PostProcess_FilmGrain(cmd_list, rt_frame_output_1, rt_frame_output_2);
                rt_frame_output_1.swap(rt_frame_output_2);
            }

            if (frame_output_in_id != rt_frame_output_1->GetObjectID())
            {
                rt_frame_output_1.swap(rt_frame_output_2);
            }
        }

        cmd_list->EndMarker();
    }

    void Renderer::Pass_PostProcess_TAA(RHI_CommandList* cmd_list, shared_ptr<RHI_Texture>& tex_in, shared_ptr<RHI_Texture>& tex_out)
    {
        RHI_Shader* shader_c = m_shaders[Renderer::Shader::Taa_C].get();
        if (!shader_c->IsCompiled())
            return;

        RHI_Texture* tex_history = RENDER_TARGET(RenderTarget::Taa_History).get();

        static RHI_PipelineState pso;
        pso.shader_compute = shader_c;
        pso.pass_name = "Pass_PostProcess_TAA";

        if (cmd_list->BeginRenderPass(pso))
        {
            m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_out->GetWidth()), static_cast<float>(tex_out->GetHeight()));
            Update_Cb_Uber(cmd_list);

            const uint32_t thread_group_count_x = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetWidth()) / m_thread_group_count));
            const uint32_t thread_group_count_y = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetHeight()) / m_thread_group_count));
            const uint32_t thread_group_count_z = 1;
            const bool async = false;

            cmd_list->SetTexture(Renderer::Bindings_Uav::rgb, tex_out);
            cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex_history);
            cmd_list->SetTexture(Renderer::Bindings_Srv::tex2, tex_in);
            cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_velocity, RENDER_TARGET(!m_is_odd_frame ? RenderTarget::Gbuffer_Velocity : RenderTarget::Gbuffer_Velocity_2));
            cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_velocity_previous, RENDER_TARGET(m_is_odd_frame ? RenderTarget::Gbuffer_Velocity : RenderTarget::Gbuffer_Velocity_2));
            cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_depth, RENDER_TARGET(RenderTarget::Gbuffer_Depth));
            cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
            cmd_list->EndRenderPass();
        }

        bool bilinear = false;
        Pass_Copy(cmd_list, tex_out.get(), tex_history, bilinear);
    }

    void Renderer::Pass_PostProcess_Bloom(RHI_CommandList* cmd_list, shared_ptr<RHI_Texture>& tex_in, shared_ptr<RHI_Texture>& tex_out)
    {
        RHI_Shader* shader_luminance = m_shaders[Renderer::Shader::BloomLuminance_C].get();
        RHI_Shader* shader_upsampleBlendMip = m_shaders[Renderer::Shader::BloomUpsampleBlendMip_C].get();
        RHI_Shader* shader_blendFrame = m_shaders[Renderer::Shader::BloomBlendFrame_C].get();

        if (!shader_luminance->IsCompiled() || !shader_upsampleBlendMip->IsCompiled() || !shader_blendFrame->IsCompiled())
            return;

        RHI_Texture* tex_bloom = RENDER_TARGET(RenderTarget::Bloom).get();

        cmd_list->StartMarker("Pass_Bloom");

        {
            static RHI_PipelineState pso;
            pso.shader_compute = shader_luminance;
            pso.pass_name = "Pass_PostProcess_BloomLuminance";

            if (cmd_list->BeginRenderPass(pso))
            {
                m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_bloom->GetWidth()), static_cast<float>(tex_bloom->GetHeight()));
                Update_Cb_Uber(cmd_list);

                const uint32_t thread_group_count_x = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_bloom->GetWidth()) / m_thread_group_count));
                const uint32_t thread_group_count_y = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_bloom->GetHeight()) / m_thread_group_count));
                const uint32_t thread_group_count_z = 1;
                const bool async = false;

                cmd_list->SetTexture(Renderer::Bindings_Uav::rgb, tex_bloom);
                cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex_in);
                cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
                cmd_list->EndRenderPass();
            }
        }

        const bool luminance_antiflicker = true;
        Pass_AMD_FidelityFX_SinglePassDownsampler(cmd_list, tex_bloom, luminance_antiflicker);

        {
            static RHI_PipelineState pso;
            pso.shader_compute = shader_upsampleBlendMip;
            pso.pass_name = "Pass_PostProcess_BloomUpsampleBlendMip";

            if (cmd_list->BeginRenderPass(pso))
            {
                for (int i = static_cast<int>(tex_bloom->GetMipCount() - 1); i > 0; i--)
                {
                    int mip_index_small = i;
                    int mip_index_big = i - 1;
                    int mip_width_large = tex_bloom->GetWidth() >> mip_index_big;
                    int mip_height_height = tex_bloom->GetHeight() >> mip_index_big;

                    m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(mip_width_large), static_cast<float>(mip_height_height));
                    Update_Cb_Uber(cmd_list);

                    const uint32_t thread_group_count_x = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(mip_width_large) / m_thread_group_count));
                    const uint32_t thread_group_count_y = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(mip_height_height) / m_thread_group_count));
                    const uint32_t thread_group_count_z = 1;
                    const bool async = false;

                    cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex_bloom, mip_index_small);
                    cmd_list->SetTexture(Renderer::Bindings_Uav::rgb, tex_bloom, mip_index_big);
                    cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
                }

                cmd_list->EndRenderPass();
            }
        }

        {
            static RHI_PipelineState pso;
            pso.shader_compute = shader_blendFrame;
            pso.pass_name = "Pass_PostProcess_BloomBlendFrame";

            if (cmd_list->BeginRenderPass(pso))
            {
                m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_out->GetWidth()), static_cast<float>(tex_out->GetHeight()));
                Update_Cb_Uber(cmd_list);

                const uint32_t thread_group_count_x = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetWidth()) / m_thread_group_count));
                const uint32_t thread_group_count_y = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetHeight()) / m_thread_group_count));
                const uint32_t thread_group_count_z = 1;
                const bool async = false;

                cmd_list->SetTexture(Renderer::Bindings_Uav::rgb, tex_out.get());
                cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex_in);
                cmd_list->SetTexture(Renderer::Bindings_Srv::tex2, tex_bloom, 0);
                cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
                cmd_list->EndRenderPass();
            }
        }

        cmd_list->EndMarker();
    }

    void Renderer::Pass_PostProcess_ToneMapping(RHI_CommandList* cmd_list, shared_ptr<RHI_Texture>& tex_in, shared_ptr<RHI_Texture>& tex_out)
    {
        RHI_Shader* shader_c = m_shaders[Renderer::Shader::ToneMapping_C].get();
        if (!shader_c->IsCompiled())
            return;

        static RHI_PipelineState pso;
        pso.shader_compute = shader_c;
        pso.pass_name = "Pass_PostProcess_ToneMapping";

        if (cmd_list->BeginRenderPass(pso))
        {
            m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_out->GetWidth()), static_cast<float>(tex_out->GetHeight()));
            Update_Cb_Uber(cmd_list);

            const uint32_t thread_group_count_x = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetWidth()) / m_thread_group_count));
            const uint32_t thread_group_count_y = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetHeight()) / m_thread_group_count));
            const uint32_t thread_group_count_z = 1;
            const bool async = false;

            cmd_list->SetTexture(Renderer::Bindings_Uav::rgb, tex_out);
            cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex_in);
            cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
            cmd_list->EndRenderPass();
        }
    }

    void Renderer::Pass_PostProcess_Fxaa(RHI_CommandList* cmd_list, shared_ptr<RHI_Texture>& tex_in, shared_ptr<RHI_Texture>& tex_out)
    {
        RHI_Shader* shader_c = m_shaders[Renderer::Shader::Fxaa_C].get();
        if (!shader_c->IsCompiled())
            return;

        static RHI_PipelineState pso;
        pso.shader_compute = shader_c;
        pso.pass_name = "Pass_PostProcess_FXAA";

        if (cmd_list->BeginRenderPass(pso))
        {
            const uint32_t thread_group_count_x = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetWidth()) / m_thread_group_count));
            const uint32_t thread_group_count_y = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetHeight()) / m_thread_group_count));
            const uint32_t thread_group_count_z = 1;
            const bool async = false;

            m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_out->GetWidth()), static_cast<float>(tex_out->GetHeight()));
            Update_Cb_Uber(cmd_list);

            cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex_in);
            cmd_list->SetTexture(Renderer::Bindings_Uav::rgb, tex_out);
            cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
            cmd_list->EndRenderPass();
        }
    }

    void Renderer::Pass_PostProcess_ChromaticAberration(RHI_CommandList* cmd_list, shared_ptr<RHI_Texture>& tex_in, shared_ptr<RHI_Texture>& tex_out)
    {
        RHI_Shader* shader_c = m_shaders[Renderer::Shader::ChromaticAberration_C].get();
        if (!shader_c->IsCompiled())
            return;

        static RHI_PipelineState pso;
        pso.shader_compute = shader_c;
        pso.pass_name = "Pass_PostProcess_ChromaticAberration";

        if (cmd_list->BeginRenderPass(pso))
        {
            m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_out->GetWidth()), static_cast<float>(tex_out->GetHeight()));
            Update_Cb_Uber(cmd_list);

            const uint32_t thread_group_count_x = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetWidth()) / m_thread_group_count));
            const uint32_t thread_group_count_y = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetHeight()) / m_thread_group_count));
            const uint32_t thread_group_count_z = 1;
            const bool async = false;

            cmd_list->SetTexture(Renderer::Bindings_Uav::rgb, tex_out);
            cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex_in);
            cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
            cmd_list->EndRenderPass();
        }
    }

    void Renderer::Pass_PostProcess_MotionBlur(RHI_CommandList* cmd_list, shared_ptr<RHI_Texture>& tex_in, shared_ptr<RHI_Texture>& tex_out)
    {
        RHI_Shader* shader_c = m_shaders[Renderer::Shader::MotionBlur_C].get();
        if (!shader_c->IsCompiled())
            return;

        static RHI_PipelineState pso;
        pso.shader_compute = shader_c;
        pso.pass_name = "Pass_PostProcess_MotionBlur";

        if (cmd_list->BeginRenderPass(pso))
        {
            m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_out->GetWidth()), static_cast<float>(tex_out->GetHeight()));
            Update_Cb_Uber(cmd_list);

            const uint32_t thread_group_count_x = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetWidth()) / m_thread_group_count));
            const uint32_t thread_group_count_y = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetHeight()) / m_thread_group_count));
            const uint32_t thread_group_count_z = 1;
            const bool async = false;

            cmd_list->SetTexture(Renderer::Bindings_Uav::rgb, tex_out);
            cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex_in);
            cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_velocity, RENDER_TARGET(RenderTarget::Gbuffer_Velocity));
            cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_depth, RENDER_TARGET(RenderTarget::Gbuffer_Depth));
            cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
            cmd_list->EndRenderPass();
        }
    }

    void Renderer::Pass_PostProcess_DepthOfField(RHI_CommandList* cmd_list, shared_ptr<RHI_Texture>& tex_in, shared_ptr<RHI_Texture>& tex_out)
    {
        RHI_Shader* shader_downsampleCoc = m_shaders[Renderer::Shader::Dof_DownsampleCoc_C].get();
        RHI_Shader* shader_bokeh = m_shaders[Renderer::Shader::Dof_Bokeh_C].get();
        RHI_Shader* shader_tent = m_shaders[Renderer::Shader::Dof_Tent_C].get();
        RHI_Shader* shader_upsampleBlend = m_shaders[Renderer::Shader::Dof_UpscaleBlend_C].get();
        if (!shader_downsampleCoc->IsCompiled() || !shader_bokeh->IsCompiled() || !shader_tent->IsCompiled() || !shader_upsampleBlend->IsCompiled())
            return;

        RHI_Texture* tex_bokeh_half = RENDER_TARGET(RenderTarget::Dof_Half).get();
        RHI_Texture* tex_bokeh_half_2 = RENDER_TARGET(RenderTarget::Dof_Half_2).get();
        RHI_Texture* tex_depth = RENDER_TARGET(RenderTarget::Gbuffer_Depth).get();

        {
            static RHI_PipelineState pso;
            pso.shader_compute = shader_downsampleCoc;
            pso.pass_name = "Pass_PostProcess_Dof_DownsampleCoc";

            if (cmd_list->BeginRenderPass(pso))
            {
                m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_bokeh_half->GetWidth()), static_cast<float>(tex_bokeh_half->GetHeight()));
                Update_Cb_Uber(cmd_list);

                const uint32_t thread_group_count_x = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_bokeh_half->GetWidth()) / m_thread_group_count));
                const uint32_t thread_group_count_y = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_bokeh_half->GetHeight()) / m_thread_group_count));
                const uint32_t thread_group_count_z = 1;
                const bool async = false;

                cmd_list->SetTexture(Renderer::Bindings_Uav::rgba, tex_bokeh_half);
                cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_depth, tex_depth);
                cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex_in);
                cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
                cmd_list->EndRenderPass();
            }
        }

        {
            static RHI_PipelineState pso;
            pso.shader_compute = shader_bokeh;
            pso.pass_name = "Pass_PostProcess_Dof_Bokeh";

            if (cmd_list->BeginRenderPass(pso))
            {
                m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_bokeh_half_2->GetWidth()), static_cast<float>(tex_bokeh_half_2->GetHeight()));
                Update_Cb_Uber(cmd_list);

                const uint32_t thread_group_count_x = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_bokeh_half_2->GetWidth()) / m_thread_group_count));
                const uint32_t thread_group_count_y = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_bokeh_half_2->GetHeight()) / m_thread_group_count));
                const uint32_t thread_group_count_z = 1;
                const bool async = false;

                cmd_list->SetTexture(Renderer::Bindings_Uav::rgba, tex_bokeh_half_2);
                cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex_bokeh_half);
                cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
                cmd_list->EndRenderPass();
            }
        }

        {
            static RHI_PipelineState pso;
            pso.shader_compute = shader_tent;
            pso.pass_name = "Pass_PostProcess_Dof_Tent";

            if (cmd_list->BeginRenderPass(pso))
            {
                m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_bokeh_half->GetWidth()), static_cast<float>(tex_bokeh_half->GetHeight()));
                Update_Cb_Uber(cmd_list);

                const uint32_t thread_group_count_x = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_bokeh_half->GetWidth()) / m_thread_group_count));
                const uint32_t thread_group_count_y = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_bokeh_half->GetHeight()) / m_thread_group_count));
                const uint32_t thread_group_count_z = 1;
                const bool async = false;

                cmd_list->SetTexture(Renderer::Bindings_Uav::rgba, tex_bokeh_half);
                cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex_bokeh_half_2);
                cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
                cmd_list->EndRenderPass();
            }
        }

        {
            static RHI_PipelineState pso;
            pso.shader_compute = shader_upsampleBlend;
            pso.pass_name = "Pass_PostProcess_Dof_UpscaleBlend";

            if (cmd_list->BeginRenderPass(pso))
            {
                m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_out->GetWidth()), static_cast<float>(tex_out->GetHeight()));
                Update_Cb_Uber(cmd_list);

                const uint32_t thread_group_count_x = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetWidth()) / m_thread_group_count));
                const uint32_t thread_group_count_y = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetHeight()) / m_thread_group_count));
                const uint32_t thread_group_count_z = 1;
                const bool async = false;

                cmd_list->SetTexture(Renderer::Bindings_Uav::rgba, tex_out);
                cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_depth, tex_depth);
                cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex_in);
                cmd_list->SetTexture(Renderer::Bindings_Srv::tex2, tex_bokeh_half);
                cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
                cmd_list->EndRenderPass();
            }
        }
    }

    void Renderer::Pass_PostProcess_Debanding(RHI_CommandList* cmd_list, shared_ptr<RHI_Texture>& tex_in, shared_ptr<RHI_Texture>& tex_out)
    {
        RHI_Shader* shader = m_shaders[Renderer::Shader::Debanding_C].get();
        if (!shader->IsCompiled())
            return;

        static RHI_PipelineState pso;
        pso.shader_compute = shader;
        pso.pass_name = "Pass_PostProcess_Debanding";

        if (cmd_list->BeginRenderPass(pso))
        {
            m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_out->GetWidth()), static_cast<float>(tex_out->GetHeight()));
            Update_Cb_Uber(cmd_list);

            const uint32_t thread_group_count_x = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetWidth()) / m_thread_group_count));
            const uint32_t thread_group_count_y = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetHeight()) / m_thread_group_count));
            const uint32_t thread_group_count_z = 1;
            const bool async = false;

            cmd_list->SetTexture(Renderer::Bindings_Uav::rgb, tex_out);
            cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex_in);
            cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
            cmd_list->EndRenderPass();
        }
    }

    void Renderer::Pass_PostProcess_FilmGrain(RHI_CommandList* cmd_list, shared_ptr<RHI_Texture>& tex_in, shared_ptr<RHI_Texture>& tex_out)
    {
        RHI_Shader* shader_c = m_shaders[Renderer::Shader::FilmGrain_C].get();
        if (!shader_c->IsCompiled())
            return;

        static RHI_PipelineState pso;
        pso.shader_compute = shader_c;
        pso.pass_name = "Pass_PostProcess_FilmGrain";

        if (cmd_list->BeginRenderPass(pso))
        {
            m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_out->GetWidth()), static_cast<float>(tex_out->GetHeight()));
            Update_Cb_Uber(cmd_list);

            const uint32_t thread_group_count_x = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetWidth()) / m_thread_group_count));
            const uint32_t thread_group_count_y = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetHeight()) / m_thread_group_count));
            const uint32_t thread_group_count_z = 1;
            const bool async = false;

            cmd_list->SetTexture(Renderer::Bindings_Uav::rgb, tex_out);
            cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex_in);
            cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
            cmd_list->EndRenderPass();
        }
    }

    void Renderer::Pass_AMD_FidelityFX_ContrastAdaptiveSharpening(RHI_CommandList* cmd_list, shared_ptr<RHI_Texture>& tex_in, shared_ptr<RHI_Texture>& tex_out)
    {
        RHI_Shader* shader_c = m_shaders[Renderer::Shader::AMD_FidelityFX_CAS_C].get();
        if (!shader_c->IsCompiled())
            return;

        static RHI_PipelineState pso;
        pso.shader_compute = shader_c;
        pso.pass_name = "Pass_AMD_FidelityFX_ContrastAdaptiveSharpening";

        if (cmd_list->BeginRenderPass(pso))
        {
            m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_out->GetWidth()), static_cast<float>(tex_out->GetHeight()));
            Update_Cb_Uber(cmd_list);

            const uint32_t thread_group_count_x = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetWidth()) / m_thread_group_count));
            const uint32_t thread_group_count_y = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetHeight()) / m_thread_group_count));
            const uint32_t thread_group_count_z = 1;
            const bool async = false;

            cmd_list->SetTexture(Renderer::Bindings_Uav::rgb, tex_out);
            cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex_in);
            cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
            cmd_list->EndRenderPass();
        }
    }

    void Renderer::Pass_AMD_FidelityFX_SinglePassDownsampler(RHI_CommandList* cmd_list, RHI_Texture* tex, const bool luminance_antiflicker)
    {
        // AMD FidelityFX Single Pass Downsampler.
        // Provides an RDNA¢â-optimized solution for generating up to 12 MIP levels of a texture.
        // GitHub:        https://github.com/GPUOpen-Effects/FidelityFX-SPD
        // Documentation: https://github.com/GPUOpen-Effects/FidelityFX-SPD/blob/master/docs/FidelityFX_SPD.pdf

        uint32_t output_mip_count = tex->GetMipCount() - 1;
        uint32_t smallest_width = tex->GetWidth() >> output_mip_count;
        uint32_t smallest_height = tex->GetWidth() >> output_mip_count;

        // Ensure that the input texture meets the requirements.
        ASSERT(tex->HasPerMipViews());
        ASSERT(output_mip_count <= 12); // As per documentation (page 22)

        // Acquire shader
        RHI_Shader* shader = m_shaders[luminance_antiflicker ? Renderer::Shader::AMD_FidelityFX_SPD_LuminanceAntiflicker_C : Renderer::Shader::AMD_FidelityFX_SPD_C].get();

        if (!shader->IsCompiled())
            return;

        // Set render state
        static RHI_PipelineState pso;
        pso.shader_compute = shader;
        pso.pass_name = "Pass_AMD_FidelityFX_SinglePassDowsnampler";

        // Draw
        if (cmd_list->BeginRenderPass(pso))
        {
            // As per documentation (page 22)
            const uint32_t thread_group_count_x = (tex->GetWidth() + 63) >> 6;
            const uint32_t thread_group_count_y = (tex->GetHeight() + 63) >> 6;
            const uint32_t thread_group_count_z = 1;
            const bool async = false;

            // Update uber buffer
            m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex->GetWidth()), static_cast<float>(tex->GetHeight()));
            m_cb_uber_cpu.mip_count = output_mip_count;
            m_cb_uber_cpu.work_group_count = thread_group_count_x * thread_group_count_y * thread_group_count_z;
            Update_Cb_Uber(cmd_list);

            cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex, 0); // top mip
            cmd_list->SetTexture(Renderer::Bindings_Uav::rgba_mips, tex, 1, true); // rest of the mips
            cmd_list->SetStructuredBuffer(Renderer::Bindings_Sb::counter, m_sb_counter);
            cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
            cmd_list->EndRenderPass();
        }
    }

    void Renderer::Pass_AMD_FidelityFX_SuperResolution(RHI_CommandList* cmd_list, RHI_Texture* tex_in, RHI_Texture* tex_out, RHI_Texture* tex_out_scratch)
    {
        RHI_Shader* shader_upsample_c = m_shaders[Renderer::Shader::AMD_FidelityFX_FSR_Upsample_C].get();
        RHI_Shader* shader_sharpen_c = m_shaders[Renderer::Shader::AMD_FidelityFX_FSR_Sharpen_C].get();
        if (!shader_upsample_c->IsCompiled() || !shader_sharpen_c->IsCompiled())
            return;

        {
            static RHI_PipelineState pso;
            pso.shader_compute = shader_upsample_c;
            pso.pass_name = "Pass_AMD_FidelityFX_SuperResolution_Upsample";

            if (cmd_list->BeginRenderPass(pso))
            {
                static const int thread_group_work_region_dim = 16;
                const uint32_t thread_group_count_x = (tex_out->GetWidth() + (thread_group_work_region_dim - 1)) / thread_group_work_region_dim;
                const uint32_t thread_group_count_y = (tex_out->GetHeight() + (thread_group_work_region_dim - 1)) / thread_group_work_region_dim;
                const uint32_t thread_group_count_z = 1;
                const bool async = false;

                cmd_list->SetTexture(Renderer::Bindings_Uav::rgb, tex_out_scratch);
                cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex_in);
                cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
                cmd_list->EndRenderPass();
            }
        }

        {
            static RHI_PipelineState pso;
            pso.shader_compute = shader_sharpen_c;
            pso.pass_name = "Pass_AMD_FidelityFX_SuperResolution_Sharpen";

            if (cmd_list->BeginRenderPass(pso))
            {
                static const int thread_group_work_region_dim = 16;
                const uint32_t thread_group_count_x = (tex_out->GetWidth() + (thread_group_work_region_dim - 1)) / thread_group_work_region_dim;
                const uint32_t thread_group_count_y = (tex_out->GetHeight() + (thread_group_work_region_dim - 1)) / thread_group_work_region_dim;
                const uint32_t thread_group_count_z = 1;
                const bool async = false;

                cmd_list->SetTexture(Renderer::Bindings_Uav::rgb, tex_out);
                cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex_out_scratch);
                cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
                cmd_list->EndRenderPass();
            }
        }
    }

    void Renderer::Pass_Lines(RHI_CommandList* cmd_list, RHI_Texture* tex_out)
    {
        const bool draw_grid = m_options & Renderer::Option::Debug_Grid;
        const bool draw_lines_depth_off = m_lines_index_depth_off != numeric_limits<uint32_t>::max();
        const bool draw_lines_depth_on = m_lines_index_depth_on > ((m_line_vertices.size() / 2) - 1);
        if (!draw_grid && !draw_lines_depth_off && !draw_lines_depth_on)
            return;

        RHI_Shader* shader_color_v = m_shaders[Renderer::Shader::Color_V].get();
        RHI_Shader* shader_color_p = m_shaders[Renderer::Shader::Color_P].get();
        if (!shader_color_v->IsCompiled() || !shader_color_p->IsCompiled())
            return;

        if (draw_grid)
        {
            static RHI_PipelineState pso;
            pso.shader_vertex = shader_color_v;
            pso.shader_pixel = shader_color_p;
            pso.rasterizer_state = m_rasterizer_cull_back_wireframe.get();
            pso.blend_state = m_blend_alpha.get();
            pso.depth_stencil_state = m_depth_stencil_r_off.get();
            pso.render_target_color_textures[0] = tex_out;
            pso.render_target_depth_texture = RENDER_TARGET(RenderTarget::Gbuffer_Depth).get();
            pso.viewport = tex_out->GetViewport();
            pso.primitive_topology = RHI_PrimitiveTopology_Mode::LineList;
            pso.pass_name = "Pass_Lines_Grid";

            if (cmd_list->BeginRenderPass(pso))
            {
                m_cb_uber_cpu.resolution_rt = m_resolution_render;
                if (m_camera)
                {
                    m_cb_uber_cpu.transform = m_gizmo_grid->ComputeWorldMatrix(m_camera->GetTransform()) * m_cb_frame_cpu.view_projection_unjittered;
                }
                Update_Cb_Uber(cmd_list);

                cmd_list->SetBufferVertex(m_gizmo_grid->GetVertexBuffer().get());
                cmd_list->Draw(m_gizmo_grid->GetVertexCount());
                cmd_list->EndRenderPass();
            }
        }

        if (draw_lines_depth_off || draw_lines_depth_on)
        {
            uint32_t vertex_count = static_cast<uint32_t>(m_line_vertices.size());
            if (vertex_count > m_vertex_buffer_lines->GetVertexCount())
            {
                m_vertex_buffer_lines->CreateDynamic<RHI_Vertex_PosCol>(vertex_count);
            }

            if (vertex_count != 0)
            {
                RHI_Vertex_PosCol* buffer = static_cast<RHI_Vertex_PosCol*>(m_vertex_buffer_lines->Map());
                std::copy(m_line_vertices.begin(), m_line_vertices.end(), buffer);
                m_vertex_buffer_lines->Unmap();

                static RHI_PipelineState pso;
                pso.shader_vertex = shader_color_v;
                pso.shader_pixel = shader_color_p;
                pso.rasterizer_state = m_rasterizer_cull_back_wireframe.get();
                pso.render_target_color_textures[0] = tex_out;
                pso.viewport = tex_out->GetViewport();
                pso.primitive_topology = RHI_PrimitiveTopology_Mode::LineList;

                if (draw_lines_depth_off)
                {
                    pso.blend_state = m_blend_disabled.get();
                    pso.depth_stencil_state = m_depth_stencil_off_off.get();
                    pso.pass_name = "Pass_Lines_Depth_Off";

                    if (cmd_list->BeginRenderPass(pso))
                    {
                        cmd_list->SetBufferVertex(m_vertex_buffer_lines.get());
                        cmd_list->Draw(m_lines_index_depth_off + 1);
                        cmd_list->EndRenderPass();
                    }
                }

                if (m_lines_index_depth_on > (vertex_count / 2) - 1)
                {
                    pso.blend_state = m_blend_alpha.get();
                    pso.depth_stencil_state = m_depth_stencil_r_off.get();
                    pso.render_target_depth_texture = RENDER_TARGET(RenderTarget::Gbuffer_Depth).get();
                    pso.pass_name = "Pass_Lines_Depth_On";

                    if (cmd_list->BeginRenderPass(pso))
                    {
                        cmd_list->SetBufferVertex(m_vertex_buffer_lines.get());
                        cmd_list->Draw((m_lines_index_depth_on - (vertex_count / 2)) + 1, vertex_count / 2);
                        cmd_list->EndRenderPass();
                    }
                }
            }
        }
    }

    void Renderer::Pass_Icons(RHI_CommandList* cmd_list, RHI_Texture* tex_out)
    {
        if (!(m_options & Renderer::Option::Debug_Lights))
            return;

        auto& lights = m_entities[ObjectType::Light];
        RHI_Shader* shader_v = m_shaders[Renderer::Shader::Quad_V].get();
        RHI_Shader* shader_p = m_shaders[Renderer::Shader::Copy_Bilinear_P].get();
        if (lights.empty() || !shader_v->IsCompiled() || !shader_p->IsCompiled())
            return;

        static RHI_PipelineState pso;
        pso.shader_vertex = shader_v;
        pso.shader_pixel = shader_p;
        pso.rasterizer_state = m_rasterizer_cull_back_solid.get();
        pso.blend_state = m_blend_alpha.get();
        pso.depth_stencil_state = m_depth_stencil_off_off.get();
        pso.render_target_color_textures[0] = tex_out;
        pso.primitive_topology = RHI_PrimitiveTopology_Mode::TriangleList;
        pso.viewport = tex_out->GetViewport();
        pso.pass_name = "Pass_Icons";

        if (!m_camera)
        {
            return;
        }

        for (const auto& entity : lights)
        {
            if (cmd_list->BeginRenderPass(pso))
            {
                if (Light* light = entity->GetComponent<Light>())
                {
                    Vector3 position_light_world = entity->GetTransform()->GetPosition();
                    Vector3 position_camera_world = m_camera->GetTransform()->GetPosition();
                    Vector3 direction_camera_to_light = (position_light_world - position_camera_world).Normalized();
                    const float v_dot_l = Vector3::Dot(m_camera->GetTransform()->GetForward(), direction_camera_to_light);

                    if (v_dot_l > 0.5f)
                    {
                        const Vector2 position_light_screen = m_camera->WorldToScreenCoordinates(position_light_world);
                        const float distance = (position_camera_world - position_light_world).Length() + Util::EPSILON;
                        float scale = m_gizmo_size_max / distance;
                        scale = Util::Clamp(m_gizmo_size_min, m_gizmo_size_max, scale);

                        shared_ptr<RHI_Texture> light_tex = nullptr;
                        const LightType type = light->GetLightType();
                        if (type == LightType::Directional) light_tex = m_tex_gizmo_light_directional;
                        else if (type == LightType::Point)  light_tex = m_tex_gizmo_light_point;
                        else if (type == LightType::Spot)   light_tex = m_tex_gizmo_light_spot;

                        const float tex_width = light_tex->GetWidth() * scale;
                        const float tex_height = light_tex->GetHeight() * scale;
                        Math::Rectangle rectangle = Math::Rectangle
                        (
                            position_light_screen.x - tex_width * 0.5f,
                            position_light_screen.y - tex_height * 0.5f,
                            position_light_screen.x + tex_width,
                            position_light_screen.y + tex_height
                        );

                        if (rectangle != m_gizmo_light_rect)
                        {
                            m_gizmo_light_rect = rectangle;
                            m_gizmo_light_rect.CreateBuffers(this);
                        }

                        m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_width), static_cast<float>(tex_width));
                        m_cb_uber_cpu.transform = m_cb_frame_cpu.view_projection_ortho;
                        Update_Cb_Uber(cmd_list);

                        cmd_list->SetTexture(Renderer::Bindings_Srv::tex, light_tex);
                        cmd_list->SetBufferIndex(m_gizmo_light_rect.GetIndexBuffer());
                        cmd_list->SetBufferVertex(m_gizmo_light_rect.GetVertexBuffer());
                        cmd_list->DrawIndexed(Rectangle::GetIndexCount());
                    }
                }
                cmd_list->EndRenderPass();
            }
        }
    }

    void Renderer::Pass_TransformHandle(RHI_CommandList* cmd_list, RHI_Texture* tex_out)
    {
        if (!GetOption(Transform_Handle))
            return;

        RHI_Shader* shader_v = m_shaders[Renderer::Shader::Entity_V].get();
        RHI_Shader* shader_p = m_shaders[Renderer::Shader::Entity_Transform_P].get();
        if (!shader_v->IsCompiled() || !shader_p->IsCompiled())
            return;

        shared_ptr<TransformHandle> transform_handle = m_Context->GetSubModule<World>()->GetTransformHandle();
        if (!transform_handle)
            return;

        if (transform_handle->GetSelectedEntity() != nullptr)
        {
            bool needs_to_render = transform_handle->GetVertexBuffer() != nullptr;
            if (!needs_to_render)
                return;

            static RHI_PipelineState pso;
            pso.shader_vertex = shader_v;
            pso.shader_pixel = shader_p;
            pso.rasterizer_state = m_rasterizer_cull_back_solid.get();
            pso.blend_state = m_blend_alpha.get();
            pso.depth_stencil_state = m_depth_stencil_off_off.get();
            pso.render_target_color_textures[0] = tex_out;
            pso.primitive_topology = RHI_PrimitiveTopology_Mode::TriangleList;
            pso.viewport = tex_out->GetViewport();

            pso.pass_name = "Pass_Handle_Axis_X";
            if (cmd_list->BeginRenderPass(pso))
            {
                m_cb_uber_cpu.transform = transform_handle->GetHandle()->GetTransform(Vector3::Right);
                m_cb_uber_cpu.float3 = transform_handle->GetHandle()->GetColor(Vector3::Right);
                Update_Cb_Uber(cmd_list);

                cmd_list->SetBufferIndex(transform_handle->GetIndexBuffer());
                cmd_list->SetBufferVertex(transform_handle->GetVertexBuffer());
                cmd_list->DrawIndexed(transform_handle->GetIndexCount());
                cmd_list->EndRenderPass();
            }

            pso.pass_name = "Pass_Handle_Axis_Y";
            if (cmd_list->BeginRenderPass(pso))
            {
                m_cb_uber_cpu.transform = transform_handle->GetHandle()->GetTransform(Vector3::Up);
                m_cb_uber_cpu.float3 = transform_handle->GetHandle()->GetColor(Vector3::Up);
                Update_Cb_Uber(cmd_list);

                cmd_list->SetBufferIndex(transform_handle->GetIndexBuffer());
                cmd_list->SetBufferVertex(transform_handle->GetVertexBuffer());
                cmd_list->DrawIndexed(transform_handle->GetIndexCount());
                cmd_list->EndRenderPass();
            }

            pso.pass_name = "Pass_Handle_Axis_Z";
            if (cmd_list->BeginRenderPass(pso))
            {
                m_cb_uber_cpu.transform = transform_handle->GetHandle()->GetTransform(Vector3::Forward);
                m_cb_uber_cpu.float3 = transform_handle->GetHandle()->GetColor(Vector3::Forward);
                Update_Cb_Uber(cmd_list);

                cmd_list->SetBufferIndex(transform_handle->GetIndexBuffer());
                cmd_list->SetBufferVertex(transform_handle->GetVertexBuffer());
                cmd_list->DrawIndexed(transform_handle->GetIndexCount());
                cmd_list->EndRenderPass();
            }

            if (transform_handle->DrawXYZ())
            {
                pso.pass_name = "Pass_Gizmos_Axis_XYZ";
                if (cmd_list->BeginRenderPass(pso))
                {
                    m_cb_uber_cpu.transform = transform_handle->GetHandle()->GetTransform(Vector3::One);
                    m_cb_uber_cpu.float3 = transform_handle->GetHandle()->GetColor(Vector3::One);
                    Update_Cb_Uber(cmd_list);

                    cmd_list->SetBufferIndex(transform_handle->GetIndexBuffer());
                    cmd_list->SetBufferVertex(transform_handle->GetVertexBuffer());
                    cmd_list->DrawIndexed(transform_handle->GetIndexCount());
                    cmd_list->EndRenderPass();
                }
            }
        }
    }

    void Renderer::Pass_DebugMeshes(RHI_CommandList* cmd_list, RHI_Texture* tex_out)
    {
        if (!GetOption(Renderer::Option::Debug_ReflectionProbes))
            return;

        RHI_Shader* shader_v = m_shaders[Renderer::Shader::Debug_ReflectionProbe_V].get();
        RHI_Shader* shader_p = m_shaders[Renderer::Shader::Debug_ReflectionProbe_P].get();
        if (!shader_v->IsCompiled() || !shader_p->IsCompiled())
            return;

        const vector<Entity*>& probes = m_entities[ObjectType::ReflectionProbe];
        if (probes.empty())
            return;

        static RHI_PipelineState pso;
        pso.shader_vertex = shader_v;
        pso.shader_pixel = shader_p;
        pso.rasterizer_state = m_rasterizer_cull_back_solid.get();
        pso.blend_state = m_blend_disabled.get();
        pso.depth_stencil_state = m_depth_stencil_r_off.get();
        pso.render_target_color_textures[0] = tex_out;
        pso.render_target_depth_texture = RENDER_TARGET(RenderTarget::Gbuffer_Depth).get();
        pso.viewport = tex_out->GetViewport();
        pso.primitive_topology = RHI_PrimitiveTopology_Mode::TriangleList;
        pso.pass_name = "Pass_Debug_ReflectionProbes";

        if (cmd_list->BeginRenderPass(pso))
        {
            cmd_list->SetBufferVertex(m_sphere_vertex_buffer.get());
            cmd_list->SetBufferIndex(m_sphere_index_buffer.get());

            for (uint32_t probe_index = 0; probe_index < static_cast<uint32_t>(probes.size()); probe_index++)
            {
                if (ReflectionProbe* probe = probes[probe_index]->GetComponent<ReflectionProbe>())
                {
                    m_cb_uber_cpu.transform = probe->GetTransform()->GetMatrix();
                    Update_Cb_Uber(cmd_list);

                    cmd_list->SetTexture(Renderer::Bindings_Srv::reflection_probe, probe->GetColorTexture());
                    cmd_list->DrawIndexed(m_sphere_index_buffer->GetIndexCount());

                    BoundingBox extents = BoundingBox(probe->GetTransform()->GetPosition() - probe->GetExtents(), probe->GetTransform()->GetPosition() + probe->GetExtents());
                    DrawBox(extents);
                }
            }

            cmd_list->EndRenderPass();
        }
    }

    void Renderer::Pass_Outline(RHI_CommandList* cmd_list, RHI_Texture* tex_out)
    {
        if (!GetOption(Renderer::Option::Debug_SelectionOutline))
            return;

        if (const Entity* entity = m_Context->GetSubModule<World>()->GetTransformHandle()->GetSelectedEntity())
        {
            const Renderable* renderable = entity->GetRenderable();
            if (!renderable)
                return;

            const Material* material = renderable->GetMaterial();
            if (!material)
                return;

            const Model* model = renderable->GeometryModel();
            if (!model || !model->GetVertexBuffer() || !model->GetIndexBuffer())
                return;

            const auto& shader_v = m_shaders[Renderer::Shader::Entity_V];
            const auto& shader_p = m_shaders[Renderer::Shader::Entity_Outline_P];
            if (!shader_v->IsCompiled() || !shader_p->IsCompiled())
                return;

            RHI_Texture* tex_depth = RENDER_TARGET(RenderTarget::Gbuffer_Depth).get();
            RHI_Texture* tex_normal = RENDER_TARGET(RenderTarget::Gbuffer_Normal).get();

            static RHI_PipelineState pso;
            pso.shader_vertex = shader_v.get();
            pso.shader_pixel = shader_p.get();
            pso.rasterizer_state = m_rasterizer_cull_back_solid.get();
            pso.blend_state = m_blend_alpha.get();
            pso.depth_stencil_state = m_depth_stencil_r_off.get();
            pso.render_target_color_textures[0] = tex_out;
            pso.render_target_depth_texture = tex_depth;
            pso.render_target_depth_texture_read_only = true;
            pso.primitive_topology = RHI_PrimitiveTopology_Mode::TriangleList;
            pso.viewport = tex_out->GetViewport();
            pso.pass_name = "Pass_Outline";

            if (cmd_list->BeginRenderPass(pso))
            {
                if (Transform* transform = entity->GetTransform())
                {
                    m_cb_uber_cpu.transform = transform->GetMatrix();
                    m_cb_uber_cpu.resolution_rt = Vector2(tex_out->GetWidth(), tex_out->GetHeight());
                    Update_Cb_Uber(cmd_list);
                }

                cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_depth, tex_depth);
                cmd_list->SetTexture(Renderer::Bindings_Srv::gbuffer_normal, tex_normal);
                cmd_list->SetBufferVertex(model->GetVertexBuffer());
                cmd_list->SetBufferIndex(model->GetIndexBuffer());
                cmd_list->DrawIndexed(renderable->GeometryIndexCount(), renderable->GeometryIndexOffset(), renderable->GeometryVertexOffset());
                cmd_list->EndRenderPass();
            }
        }
    }

    void Renderer::Pass_PeformanceMetrics(RHI_CommandList* cmd_list, RHI_Texture* tex_out)
    {
        if (!m_profiler)
            return;

        const bool draw = m_options & Renderer::Option::Debug_PerformanceMetrics;
        const bool empty = m_profiler->GetMetrics().empty();
        const auto& shader_v = m_shaders[Renderer::Shader::Font_V];
        const auto& shader_p = m_shaders[Renderer::Shader::Font_P];
        if (!draw || empty || !shader_v->IsCompiled() || !shader_p->IsCompiled())
            return;

        if (!m_profiler->GetEnabled())
        {
            m_profiler->SetEnabled(true);
        }

        cmd_list->StartMarker("Pass_PeformanceMetrics");

        static RHI_PipelineState pso;
        pso.shader_vertex = shader_v.get();
        pso.shader_pixel = shader_p.get();
        pso.rasterizer_state = m_rasterizer_cull_back_solid.get();
        pso.blend_state = m_blend_alpha.get();
        pso.depth_stencil_state = m_depth_stencil_off_off.get();
        pso.render_target_color_textures[0] = tex_out;
        pso.primitive_topology = RHI_PrimitiveTopology_Mode::TriangleList;
        pso.viewport = tex_out->GetViewport();
        pso.gpu_marker = false;
        pso.pass_name = "Pass_PeformanceMetrics_Outline";

        const Vector2 text_pos = Vector2(-m_viewport.width * 0.5f + 5.0f, m_viewport.height * 0.5f - m_font->GetSize() - 2.0f);
        m_font->SetText(m_profiler->GetMetrics(), text_pos);

        if (m_font->GetOutline() != Font_Outline_None && m_font->GetOutlineSize() != 0)
        {
            if (cmd_list->BeginRenderPass(pso))
            {
                m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_out->GetWidth()), static_cast<float>(tex_out->GetHeight()));
                m_cb_uber_cpu.mat_color = m_font->GetColorOutline();
                Update_Cb_Uber(cmd_list);

                cmd_list->SetBufferIndex(m_font->GetIndexBuffer());
                cmd_list->SetBufferVertex(m_font->GetVertexBuffer());
                cmd_list->SetTexture(Renderer::Bindings_Srv::font_atlas, m_font->GetAtlasOutline());
                cmd_list->DrawIndexed(m_font->GetIndexCount());
                cmd_list->EndRenderPass();
            }
        }

        pso.pass_name = "Pass_PeformanceMetrics_Inline";
        if (cmd_list->BeginRenderPass(pso))
        {
            m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_out->GetWidth()), static_cast<float>(tex_out->GetHeight()));
            m_cb_uber_cpu.mat_color = m_font->GetColor();
            Update_Cb_Uber(cmd_list);

            cmd_list->SetBufferIndex(m_font->GetIndexBuffer());
            cmd_list->SetBufferVertex(m_font->GetVertexBuffer());
            cmd_list->SetTexture(Renderer::Bindings_Srv::font_atlas, m_font->GetAtlas());
            cmd_list->DrawIndexed(m_font->GetIndexCount());
            cmd_list->EndRenderPass();
        }

        cmd_list->EndMarker();
    }

    void Renderer::Pass_BrdfSpecularLut(RHI_CommandList* cmd_list)
    {
        if (m_brdf_specular_lut_rendered)
            return;

        RHI_Shader* shader = m_shaders[Renderer::Shader::BrdfSpecularLut_C].get();
        if (!shader->IsCompiled())
            return;

        RHI_Texture* tex_brdf_specular_lut = RENDER_TARGET(RenderTarget::Brdf_Specular_Lut).get();

        static RHI_PipelineState pso;
        pso.shader_compute = shader;
        pso.pass_name = "Pass_BrdfSpecularLut";

        if (cmd_list->BeginRenderPass(pso))
        {
            m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_brdf_specular_lut->GetWidth()), static_cast<float>(tex_brdf_specular_lut->GetHeight()));
            Update_Cb_Uber(cmd_list);

            const uint32_t thread_group_count_x = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_brdf_specular_lut->GetWidth()) / m_thread_group_count));
            const uint32_t thread_group_count_y = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_brdf_specular_lut->GetHeight()) / m_thread_group_count));
            const uint32_t thread_group_count_z = 1;
            const bool async = false;

            cmd_list->SetTexture(Renderer::Bindings_Uav::rg, tex_brdf_specular_lut);
            cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
            cmd_list->EndRenderPass();

            m_brdf_specular_lut_rendered = true;
        }
    }

    void Renderer::Pass_Copy(RHI_CommandList* cmd_list, RHI_Texture* tex_in, RHI_Texture* tex_out, const bool bilinear)
    {
        RHI_Shader* shader_c = m_shaders[bilinear ? Renderer::Shader::Copy_Bilinear_C : Renderer::Shader::Copy_Point_C].get();
        if (!shader_c->IsCompiled())
            return;

        static RHI_PipelineState pso;
        pso.shader_compute = shader_c;
        pso.pass_name = bilinear ? "Pass_Copy_Bilinear" : "Pass_Copy_Point";

        if (cmd_list->BeginRenderPass(pso))
        {
            m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(tex_out->GetWidth()), static_cast<float>(tex_out->GetHeight()));
            Update_Cb_Uber(cmd_list);

            const uint32_t thread_group_count_x = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetWidth()) / m_thread_group_count));
            const uint32_t thread_group_count_y = static_cast<uint32_t>(Math::Util::Ceil(static_cast<float>(tex_out->GetHeight()) / m_thread_group_count));
            const uint32_t thread_group_count_z = 1;
            const bool async = false;

            cmd_list->SetTexture(Renderer::Bindings_Uav::rgb, tex_out);
            cmd_list->SetTexture(Renderer::Bindings_Srv::tex, tex_in);
            cmd_list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z, async);
            cmd_list->EndRenderPass();
        }
    }

    void Renderer::Pass_CopyToBackbuffer()
    {
        RHI_Shader* shader_v = m_shaders[Renderer::Shader::FullscreenTriangle_V].get();
        RHI_Shader* shader_p = m_shaders[Renderer::Shader::Copy_Point_P].get();
        if (!shader_v->IsCompiled() || !shader_p->IsCompiled())
            return;

        m_swap_chain->SetLayout(RHI_Image_Layout::Color_Attachment_Optimal, m_cmd_current);

        static RHI_PipelineState pso = {};
        pso.shader_vertex = shader_v;
        pso.shader_pixel = shader_p;
        pso.rasterizer_state = m_rasterizer_cull_back_solid.get();
        pso.blend_state = m_blend_disabled.get();
        pso.depth_stencil_state = m_depth_stencil_off_off.get();
        pso.render_target_swapchain = m_swap_chain.get();
        pso.clear_color[0] = rhi_color_dont_care;
        pso.primitive_topology = RHI_PrimitiveTopology_Mode::TriangleList;
        pso.viewport = m_viewport;
        pso.pass_name = "Pass_CopyToBackbuffer";

        if (m_cmd_current->BeginRenderPass(pso))
        {
            m_cb_uber_cpu.resolution_rt = Vector2(static_cast<float>(m_swap_chain->GetWidth()), static_cast<float>(m_swap_chain->GetHeight()));
            Update_Cb_Uber(m_cmd_current);

            m_cmd_current->SetTexture(Renderer::Bindings_Srv::tex, RENDER_TARGET(RenderTarget::Frame_Output).get());
            m_cmd_current->DrawIndexed(3, 0);
            m_cmd_current->EndRenderPass();
        }
        m_swap_chain->SetLayout(RHI_Image_Layout::Present_Src, m_cmd_current);
    }

    void Renderer::Pass_Generate_Mips()
    {
        for (shared_ptr<RHI_Texture> texture : m_textures_mip_generation)
        {
            ASSERT(texture != nullptr);

            ASSERT(texture->HasMips());

            ASSERT(texture->HasPerMipViews());

            const bool luminance_antiflicker = false;
            Pass_AMD_FidelityFX_SinglePassDownsampler(m_cmd_current, texture.get(), luminance_antiflicker);
        }
    }
}
