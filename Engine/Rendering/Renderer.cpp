#include "Common.h"
#include "Renderer.h"
#include "Model.h"                              
#include "Grid.h"                               
#include "Font/Font.h"                          
#include "../Utils/Sampling.h"              
#include "../Profiling/Profiler.h"              
#include "../Resource/ResourceCache.h"          
#include "../World/Entity.h"                    
#include "../World/Components/Transform.h"      
#include "../World/Components/Renderable.h"     
#include "../World/Components/Camera.h"         
#include "../World/Components/Light.h"          
#include "../World/Components/ReflectionProbe.h"
#include "../RHI/RHI_Device.h"                  
#include "../RHI/RHI_ConstantBuffer.h"          
#include "../RHI/RHI_StructuredBuffer.h"        
#include "../RHI/RHI_CommandList.h"             
#include "../RHI/RHI_Texture2D.h"               
#include "../RHI/RHI_SwapChain.h"               
#include "../RHI/RHI_VertexBuffer.h"            
#include "../RHI/RHI_Implementation.h"          
#include "../RHI/RHI_Semaphore.h"
#include "../RHI/RHI_CommandPool.h"
#include "../Core/Window.h"                     
#include "../Input/Input.h"                     
#include "../World/Components/Environment.h"    
#include "../Core/Context.h"
#include "../Core/Timer.h"
#include "../Display/Display.h"

using namespace std;
using namespace PlayGround::Math;

// ∑ª¥ı ≈∏∞Ÿ ∏≈≈©∑Œ
#define RENDER_TARGET(rt_enum) m_render_targets[static_cast<uint8_t>(rt_enum)]

namespace PlayGround
{
    template<typename T>
    void update_dynamic_buffer(RHI_CommandList* cmd_list, RHI_ConstantBuffer* buffer_constant, T& buffer_cpu, T& buffer_cpu_mapped)
    {
        if (buffer_cpu == buffer_cpu_mapped)
            return;

        if (buffer_constant->GetOffset() + buffer_constant->GetStride() >= buffer_constant->GetObjectSizeGPU())
        {
            buffer_constant->Create<T>(buffer_constant->GetStrideCount() * 2);
        }

        {
            {
                uint64_t stride = buffer_constant->GetStride();
                uint64_t offset = buffer_constant->GetResetOffset() ? 0 : (buffer_constant->GetOffset() + stride);

                T* buffer_gpu = static_cast<T*>(buffer_constant->Map());

                memcpy(reinterpret_cast<std::byte*>(buffer_gpu) + offset, reinterpret_cast<std::byte*>(&buffer_cpu), stride);

                if (buffer_constant->IsPersistentBuffer())
                {
                    buffer_constant->Flush(stride, offset);
                }
                else
                {
                    buffer_constant->Unmap();
                }

            }

            buffer_cpu_mapped = buffer_cpu;
        }
    }

    Renderer::Renderer(Context* context) : SubModule(context)
    {
        m_options |= Renderer::Option::ReverseZ;
        m_options |= Renderer::Option::Transform_Handle;
        m_options |= Renderer::Option::Debug_Grid;
        m_options |= Renderer::Option::Debug_ReflectionProbes;
        m_options |= Renderer::Option::Debug_Lights;
        m_options |= Renderer::Option::Debug_Physics;
        m_options |= Renderer::Option::Bloom;
        m_options |= Renderer::Option::VolumetricFog;
        m_options |= Renderer::Option::MotionBlur;
        m_options |= Renderer::Option::Ssao;
        m_options |= Renderer::Option::Ssao_Gi;
        m_options |= Renderer::Option::ScreenSpaceShadows;
        m_options |= Renderer::Option::ScreenSpaceReflections;
        m_options |= Renderer::Option::AntiAliasing_Taa;
        m_options |= Renderer::Option::Sharpening_AMD_FidelityFX_ContrastAdaptiveSharpening;
        m_options |= Renderer::Option::Debanding;

        m_option_values[Renderer::OptionValue::Anisotropy] = 16.0f;
        m_option_values[Renderer::OptionValue::ShadowResolution] = 2048.0f;
        m_option_values[Renderer::OptionValue::Tonemapping] = static_cast<float>(Tonemapping::Renderer_ToneMapping_Off);
        m_option_values[Renderer::OptionValue::Gamma] = 1.5f;
        m_option_values[Renderer::OptionValue::Sharpen_Strength] = 1.0f;
        m_option_values[Renderer::OptionValue::Bloom_Intensity] = 0.2f;
        m_option_values[Renderer::OptionValue::Fog] = 0.08f;

        SUBSCRIBE_TO_EVENT(EventType::WorldResolved, EVENT_HANDLER_VARIANT(OnRenderablesAcquire));
        SUBSCRIBE_TO_EVENT(EventType::WorldPreClear, EVENT_HANDLER(OnClear));
        SUBSCRIBE_TO_EVENT(EventType::WorldLoadEnd, EVENT_HANDLER(OnWorldLoaded));
        SUBSCRIBE_TO_EVENT(EventType::WindowOnFullScreenToggled, EVENT_HANDLER(OnFullScreenToggled));

        m_render_thread_id = this_thread::get_id();

        m_material_instances.fill(nullptr);
    }

    Renderer::~Renderer()
    {
        UNSUBSCRIBE_FROM_EVENT(EventType::WorldResolved, EVENT_HANDLER_VARIANT(OnRenderablesAcquire));
        UNSUBSCRIBE_FROM_EVENT(EventType::WorldPreClear, EVENT_HANDLER(OnClear));
        UNSUBSCRIBE_FROM_EVENT(EventType::WorldLoadEnd, EVENT_HANDLER(OnWorldLoaded));
        UNSUBSCRIBE_FROM_EVENT(EventType::WindowOnFullScreenToggled, EVENT_HANDLER(OnFullScreenToggled));

        Logger::m_Log_to_file = true;
    }

    void Renderer::OnInit()
    {
        Window* window = m_Context->GetSubModule<Window>();
        ASSERT(window && "The Renderer submodule requires a Window subsystem.");

        m_resource_cache = m_Context->GetSubModule<ResourceCache>();
        ASSERT(window && "The Renderer submodule requires a ResourceCache subsystem.");

        m_profiler = m_Context->GetSubModule<Profiler>();

        m_rhi_device = make_shared<RHI_Device>(m_Context);

        m_vertex_buffer_lines = make_shared<RHI_VertexBuffer>(m_rhi_device, true, "renderer_lines");

        m_gizmo_grid = make_unique<Grid>(m_rhi_device);

        uint32_t window_width = window->GetWidth();
        uint32_t window_height = window->GetHeight();

        m_swap_chain = make_shared<RHI_SwapChain>
            (
                window->GetHandle(),
                m_rhi_device,
                window_width,
                window_height,
                RHI_Format_R8G8B8A8_Unorm,
                m_swap_chain_buffer_count,
                RHI_Present_Immediate | RHI_Swap_Flip_Discard,
                "renderer"
                );

        m_cmd_pool = m_rhi_device->AllocateCommandPool("renderer", m_swap_chain->GetObjectID());

        m_cmd_pool->AllocateCommandLists(m_swap_chain_buffer_count);

        SetResolutionRender(window_width, window_height, false);
        SetResolutionOutput(window_width, window_height, false);
        SetViewport(static_cast<float>(window_width), static_cast<float>(window_height));

        CreateConstantBuffers();
        CreateShaders();
        CreateDepthStencilStates();
        CreateRasterizerStates();
        CreateBlendStates();
        CreateRenderTextures(true, true, true, true);
        CreateFonts();
        CreateMeshes();
        CreateSamplers(false);
        CreateStructuredBuffers();
        CreateTextures();
    }

    void Renderer::Update(double delta_time)
    {
        if (m_frame_num == 1 && Logger::m_Log_to_file)
        {
            Logger::m_Log_to_file = false;
        }

        if (m_flush_requested)
        {
            Flush();
        }

        {
            Window* window = m_Context->GetSubModule<Window>();
            uint32_t width = static_cast<uint32_t>(window->IsMinimized() ? 0 : window->GetWidth());
            uint32_t height = static_cast<uint32_t>(window->IsMinimized() ? 0 : window->GetHeight());

            if ((m_swap_chain->GetWidth() != width || m_swap_chain->GetHeight() != height) || !m_swap_chain->PresentEnabled())
            {
                if (m_swap_chain->Resize(width, height))
                {
                    LOG_INFO("Swapchain resolution has been set to %dx%d", width, height);
                }
            }
        }

        if (!m_swap_chain->PresentEnabled() || !m_is_rendering_allowed)
            return;

        m_frame_num++;
        m_is_odd_frame = (m_frame_num % 2) == 1;

        bool command_pool_reset = m_cmd_pool->Update();
        m_cmd_current = m_cmd_pool->GetCurrentCommandList();
        m_cmd_current->Begin();

        if (command_pool_reset)
        {
            m_cb_uber_gpu->ResetOffset();
            m_cb_frame_gpu->ResetOffset();
            m_cb_light_gpu->ResetOffset();
            m_cb_material_gpu->ResetOffset();

            m_reading_requests = true;
            {
                if (m_environment_texture_temp)
                {
                    m_environment_texture = m_environment_texture_temp;
                    m_environment_texture_temp = nullptr;
                }

                {
                    if (!m_textures_mip_generation.empty())
                    {
                        for (shared_ptr<RHI_Texture> texture : m_textures_mip_generation)
                        {
                            uint32_t flags = texture->GetFlags();
                            flags &= ~RHI_Texture_PerMipViews;
                            flags &= ~RHI_Texture_Uav;
                            texture->SetFlags(flags);

                            {
                                const bool destroy_main = false;
                                const bool destroy_per_view = true;
                                texture->RHI_DestroyResource(destroy_main, destroy_per_view);
                            }
                        }

                        m_textures_mip_generation.clear();
                    }

                    if (!m_textures_mip_generation_pending.empty())
                    {
                        m_textures_mip_generation.insert(m_textures_mip_generation.end(), m_textures_mip_generation_pending.begin(), m_textures_mip_generation_pending.end());
                        m_textures_mip_generation_pending.clear();
                    }
                }

                Pass_Generate_Mips();
            }
            m_reading_requests = false;
        }

        {
            if (m_camera)
            {
                if (m_dirty_orthographic_projection || m_near_plane != m_camera->GetNearPlane() || m_far_plane != m_camera->GetFarPlane())
                {
                    m_near_plane = m_camera->GetNearPlane();
                    m_far_plane = m_camera->GetFarPlane();

                    m_cb_frame_cpu.projection_ortho = Matrix::CreateOrthographicLH(m_viewport.width, m_viewport.height, 0.0f, m_far_plane);
                    m_cb_frame_cpu.view_projection_ortho = Matrix::CreateLookAtLH(Vector3(0, 0, -m_near_plane), Vector3::Forward, Vector3::Up) * m_cb_frame_cpu.projection_ortho;

                    m_dirty_orthographic_projection = false;
                }

                m_cb_frame_cpu.view = m_camera->GetViewMatrix();
                m_cb_frame_cpu.projection = m_camera->GetProjectionMatrix();
                m_cb_frame_cpu.projection_inverted = Matrix::Invert(m_cb_frame_cpu.projection);
            }

            if (GetOption(Renderer::Option::AntiAliasing_Taa))
            {
                const uint8_t samples = 16;
                const uint8_t index = m_frame_num % samples;
                m_taa_jitter = Utility::Sampling::Halton2D(index, 2, 3) * 2.0f - 1.0f;
                m_taa_jitter.x = (m_taa_jitter.x / m_resolution_render.x);
                m_taa_jitter.y = (m_taa_jitter.y / m_resolution_render.y);
                m_cb_frame_cpu.projection *= Matrix::CreateTranslation(Vector3(m_taa_jitter.x, m_taa_jitter.y, 0.0f));
            }
            else
            {
                m_taa_jitter = Vector2::Zero;
            }

            m_cb_frame_cpu.view_projection_previous = m_cb_frame_cpu.view_projection;
            m_cb_frame_cpu.view_projection = m_cb_frame_cpu.view * m_cb_frame_cpu.projection;
            m_cb_frame_cpu.view_projection_inv = Matrix::Invert(m_cb_frame_cpu.view_projection);
            if (m_camera)
            {
                m_cb_frame_cpu.view_projection_unjittered = m_cb_frame_cpu.view * m_camera->GetProjectionMatrix();
                m_cb_frame_cpu.camera_aperture = m_camera->GetAperture();
                m_cb_frame_cpu.camera_shutter_speed = m_camera->GetShutterSpeed();
                m_cb_frame_cpu.camera_iso = m_camera->GetIso();
                m_cb_frame_cpu.camera_near = m_camera->GetNearPlane();
                m_cb_frame_cpu.camera_far = m_camera->GetFarPlane();
                m_cb_frame_cpu.camera_position = m_camera->GetTransform()->GetPosition();
                m_cb_frame_cpu.camera_direction = m_camera->GetTransform()->GetForward();
            }
            m_cb_frame_cpu.resolution_output = m_resolution_output;
            m_cb_frame_cpu.resolution_render = m_resolution_render;
            m_cb_frame_cpu.taa_jitter_previous = m_cb_frame_cpu.taa_jitter_current;
            m_cb_frame_cpu.taa_jitter_current = m_taa_jitter;
            m_cb_frame_cpu.delta_time = static_cast<float>(m_Context->GetSubModule<Timer>()->GetDeltaTimeSmoothedSec());
            m_cb_frame_cpu.time = static_cast<float>(m_Context->GetSubModule<Timer>()->GetTimeSec());
            m_cb_frame_cpu.bloom_intensity = GetOptionValue<float>(Renderer::OptionValue::Bloom_Intensity);
            m_cb_frame_cpu.sharpen_strength = GetOptionValue<float>(Renderer::OptionValue::Sharpen_Strength);
            m_cb_frame_cpu.fog = GetOptionValue<float>(Renderer::OptionValue::Fog);
            m_cb_frame_cpu.tonemapping = GetOptionValue<float>(Renderer::OptionValue::Tonemapping);
            m_cb_frame_cpu.gamma = GetOptionValue<float>(Renderer::OptionValue::Gamma);
            m_cb_frame_cpu.shadow_resolution = GetOptionValue<float>(Renderer::OptionValue::ShadowResolution);
            m_cb_frame_cpu.frame = static_cast<uint32_t>(m_frame_num);
            m_cb_frame_cpu.frame_mip_count = RENDER_TARGET(RenderTarget::Frame_Render)->GetMipCount();
            m_cb_frame_cpu.ssr_mip_count = RENDER_TARGET(RenderTarget::Ssr)->GetMipCount();
            m_cb_frame_cpu.resolution_environment = Vector2(GetEnvironmentTexture()->GetWidth(), GetEnvironmentTexture()->GetHeight());

            m_cb_frame_cpu.set_bit(GetOption(Renderer::Option::ScreenSpaceReflections), 1 << 0);
            m_cb_frame_cpu.set_bit(GetOption(Renderer::Option::Upsample_TAA), 1 << 1);
            m_cb_frame_cpu.set_bit(GetOption(Renderer::Option::Ssao), 1 << 2);
            m_cb_frame_cpu.set_bit(GetOption(Renderer::Option::VolumetricFog), 1 << 3);
            m_cb_frame_cpu.set_bit(GetOption(Renderer::Option::ScreenSpaceShadows), 1 << 4);
            m_cb_frame_cpu.set_bit(GetOption(Renderer::Option::Ssao_Gi), 1 << 5);
        }

        Lines_PreMain();
        Pass_Main(m_cmd_current);
        Lines_PostMain(delta_time);

        m_cmd_current->End();
        m_cmd_current->Submit();
    }

    void Renderer::SetViewport(float width, float height)
    {
        if (IsCallingFromOtherThread())
        {
            while (m_reading_requests)
            {
                LOG_INFO("External thread is waiting for the renderer thread...");
                this_thread::sleep_for(chrono::milliseconds(16));
            }
        }

        if (m_viewport.width != width || m_viewport.height != height)
        {
            m_viewport.width = width;
            m_viewport.height = height;

            m_dirty_orthographic_projection = true;
        }
    }

    void Renderer::SetResolutionRender(uint32_t width, uint32_t height, bool recreate_resources /*= true*/)
    {
        if (!m_rhi_device->IsValidResolution(width, height))
        {
            LOG_WARNING("%dx%d is an invalid resolution", width, height);
            return;
        }

        width -= (width % 2 != 0) ? 1 : 0;
        height -= (height % 2 != 0) ? 1 : 0;

        if (m_resolution_render.x == width && m_resolution_render.y == height)
            return;

        m_resolution_render.x = static_cast<float>(width);
        m_resolution_render.y = static_cast<float>(height);

        sDisplayMode display_mode = Display::GetActiveDisplayMode();
        display_mode.width = width;
        display_mode.height = height;
        Display::SetActiveDisplayMode(display_mode);

        bool update_fps_limit_to_highest_hz = false;
        Display::RegisterDisplayMode(display_mode, update_fps_limit_to_highest_hz, m_Context);

        if (recreate_resources)
        {
            CreateRenderTextures(true, false, false, true);

            CreateSamplers(true);
        }

        LOG_INFO("Render resolution has been set to %dx%d", width, height);
    }

    void Renderer::SetResolutionOutput(uint32_t width, uint32_t height, bool recreate_resources /*= true*/)
    {
        if (!m_rhi_device->IsValidResolution(width, height))
        {
            LOG_WARNING("%dx%d is an invalid resolution", width, height);
            return;
        }

        width -= (width % 2 != 0) ? 1 : 0;
        height -= (height % 2 != 0) ? 1 : 0;

        if (m_resolution_output.x == width && m_resolution_output.y == height)
            return;

        m_resolution_output.x = static_cast<float>(width);
        m_resolution_output.y = static_cast<float>(height);

        if (recreate_resources)
        {
            CreateRenderTextures(false, true, false, true);

            CreateSamplers(true);
        }

        LOG_INFO("Output resolution output has been set to %dx%d", width, height);
    }

    void Renderer::Update_Cb_Frame(RHI_CommandList* cmd_list)
    {
        for (const auto& entity : m_entities[ObjectType::Light])
        {
            if (Light* light = entity->GetComponent<Light>())
            {
                if (light->GetLightType() == LightType::Directional)
                {
                    m_cb_frame_cpu.directional_light_intensity = light->GetIntensity();
                }
            }
        }

        update_dynamic_buffer<Cb_Frame>(cmd_list, m_cb_frame_gpu.get(), m_cb_frame_cpu, m_cb_frame_cpu_mapped);

        cmd_list->SetConstantBuffer(Renderer::Bindings_Cb::frame, RHI_Shader_Vertex | RHI_Shader_Pixel | RHI_Shader_Compute, m_cb_frame_gpu);
    }

    void Renderer::Update_Cb_Uber(RHI_CommandList* cmd_list)
    {
        update_dynamic_buffer<Cb_Uber>(cmd_list, m_cb_uber_gpu.get(), m_cb_uber_cpu, m_cb_uber_cpu_mapped);

        cmd_list->SetConstantBuffer(Renderer::Bindings_Cb::uber, RHI_Shader_Vertex | RHI_Shader_Pixel | RHI_Shader_Compute, m_cb_uber_gpu);
    }

    void Renderer::Update_Cb_Light(RHI_CommandList* cmd_list, const Light* light, const RHI_Shader_Type scope)
    {
        for (uint32_t i = 0; i < light->GetShadowArraySize(); i++)
        {
            m_cb_light_cpu.view_projection[i] = light->GetViewMatrix(i) * light->GetProjectionMatrix(i);
        }

        float luminous_intensity = light->GetIntensity() * m_camera->GetExposure();
        if (light->GetLightType() == LightType::Point)
        {
            luminous_intensity /= Math::Util::PI_4;
            luminous_intensity *= 255.0f;
        }
        else if (light->GetLightType() == LightType::Spot)
        {
            luminous_intensity /= Math::Util::PI;
            luminous_intensity *= 255.0f;
        }

        m_cb_light_cpu.intensity_range_angle_bias = Vector4(luminous_intensity, light->GetRange(), light->GetAngle(), GetOption(Renderer::Option::ReverseZ) ? light->GetBias() : -light->GetBias());
        m_cb_light_cpu.color = light->GetColor();
        m_cb_light_cpu.normal_bias = light->GetNormalBias();
        m_cb_light_cpu.position = light->GetTransform()->GetPosition();
        m_cb_light_cpu.direction = light->GetTransform()->GetForward();
        m_cb_light_cpu.options = 0;
        m_cb_light_cpu.options |= light->GetLightType() == LightType::Directional ? (1 << 0) : 0;
        m_cb_light_cpu.options |= light->GetLightType() == LightType::Point ? (1 << 1) : 0;
        m_cb_light_cpu.options |= light->GetLightType() == LightType::Spot ? (1 << 2) : 0;
        m_cb_light_cpu.options |= light->GetShadowsEnabled() ? (1 << 3) : 0;
        m_cb_light_cpu.options |= light->GetShadowsTransparentEnabled() ? (1 << 4) : 0;
        m_cb_light_cpu.options |= light->GetShadowsScreenSpaceEnabled() ? (1 << 5) : 0;
        m_cb_light_cpu.options |= light->GetVolumetricEnabled() ? (1 << 6) : 0;

        update_dynamic_buffer<Cb_Light>(cmd_list, m_cb_light_gpu.get(), m_cb_light_cpu, m_cb_light_cpu_mapped);

        cmd_list->SetConstantBuffer(Renderer::Bindings_Cb::light, scope, m_cb_light_gpu);
    }

    void Renderer::Update_Cb_Material(RHI_CommandList* cmd_list)
    {
        for (uint32_t i = 0; i < m_max_material_instances; i++)
        {
            Material* material = m_material_instances[i];
            if (!material)
                continue;

            m_cb_material_cpu.mat_clearcoat_clearcoatRough_anis_anisRot[i].x = material->GetProperty(Material_Clearcoat);
            m_cb_material_cpu.mat_clearcoat_clearcoatRough_anis_anisRot[i].y = material->GetProperty(Material_Clearcoat_Roughness);
            m_cb_material_cpu.mat_clearcoat_clearcoatRough_anis_anisRot[i].z = material->GetProperty(Material_Anisotropic);
            m_cb_material_cpu.mat_clearcoat_clearcoatRough_anis_anisRot[i].w = material->GetProperty(Material_Anisotropic_Rotation);
            m_cb_material_cpu.mat_sheen_sheenTint_pad[i].x = material->GetProperty(Material_Sheen);
            m_cb_material_cpu.mat_sheen_sheenTint_pad[i].y = material->GetProperty(Material_Sheen_Tint);
        }

        update_dynamic_buffer<Cb_Material>(cmd_list, m_cb_material_gpu.get(), m_cb_material_cpu, m_cb_material_cpu_mapped);

        cmd_list->SetConstantBuffer(Renderer::Bindings_Cb::material, RHI_Shader_Pixel, m_cb_material_gpu);
    }

    void Renderer::OnRenderablesAcquire(const Variant& entities_variant)
    {
        SCOPED_TIME_BLOCK(m_profiler);

        m_entities.clear();
        m_camera = nullptr;

        vector<shared_ptr<Entity>> entities = entities_variant.Get<vector<shared_ptr<Entity>>>();
        for (const auto& entity : entities)
        {
            if (!entity || !entity->IsActive())
                continue;

            if (Renderable* renderable = entity->GetComponent<Renderable>())
            {
                bool is_transparent = false;
                bool is_visible = true;

                if (const Material* material = renderable->GetMaterial())
                {
                    is_transparent = material->GetColorAlbedo().w < 1.0f;
                    is_visible = material->GetColorAlbedo().w != 0.0f;
                }

                if (is_visible)
                {
                    m_entities[is_transparent ? ObjectType::GeometryTransparent : ObjectType::GeometryOpaque].emplace_back(entity.get());
                }
            }

            if (Light* light = entity->GetComponent<Light>())
            {
                m_entities[ObjectType::Light].emplace_back(entity.get());
            }

            if (Camera* camera = entity->GetComponent<Camera>())
            {
                m_entities[ObjectType::Camera].emplace_back(entity.get());
                m_camera = camera->GetSharedPtr<Camera>();
            }

            if (ReflectionProbe* reflection_probe = entity->GetComponent<ReflectionProbe>())
            {
                m_entities[ObjectType::ReflectionProbe].emplace_back(entity.get());
            }
        }

        SortRenderables(&m_entities[ObjectType::GeometryOpaque]);
        SortRenderables(&m_entities[ObjectType::GeometryTransparent]);
    }

    void Renderer::OnClear()
    {
        Flush();
        m_entities.clear();
    }

    void Renderer::OnWorldLoaded()
    {
        m_is_rendering_allowed = true;
    }

    void Renderer::OnFullScreenToggled()
    {
        Window* window = m_Context->GetSubModule<Window>();
        Input* input = m_Context->GetSubModule<Input>();
        const bool is_full_screen = window->IsFullScreen();

        if (is_full_screen)
        {
            m_viewport_previous = m_viewport;
            m_resolution_output_previous = m_resolution_output;

            SetViewport(static_cast<float>(window->GetWidth()), static_cast<float>(window->GetHeight()));
            SetResolutionOutput(window->GetWidth(), window->GetHeight());
        }
        else
        {
            SetViewport(m_viewport_previous.x, m_viewport_previous.y);
            SetResolutionOutput(static_cast<uint32_t>(m_resolution_output_previous.x), static_cast<uint32_t>(m_resolution_output_previous.y));
        }

        input->SetMouseCursorVisible(!is_full_screen);
    }

    void Renderer::SortRenderables(vector<Entity*>* renderables)
    {
        if (!m_camera || renderables->size() <= 2)
            return;

        auto comparison_op = [this](Entity* entity)
        {
            auto renderable = entity->GetRenderable();
            if (!renderable)
                return 0.0f;

            return (renderable->GetAabb().GetCenter() - m_camera->GetTransform()->GetPosition()).LengthSquared();
        };

        sort(renderables->begin(), renderables->end(), [&comparison_op](Entity* a, Entity* b)
        {
            return comparison_op(a) < comparison_op(b);
        });
    }

    bool Renderer::IsCallingFromOtherThread()
    {
        return m_render_thread_id != this_thread::get_id();
    }

    const shared_ptr<RHI_Texture> Renderer::GetEnvironmentTexture()
    {
        return m_environment_texture ? m_environment_texture : m_tex_default_black;
    }

    void Renderer::SetEnvironmentTexture(shared_ptr<RHI_Texture> texture)
    {
        if (IsCallingFromOtherThread())
        {
            while (m_reading_requests)
            {
                LOG_INFO("External thread is waiting for the renderer thread...");
                this_thread::sleep_for(chrono::milliseconds(16));
            }
        }

        lock_guard<mutex> guard(m_environment_texture_mutex);
        m_environment_texture_temp = texture;
    }

    void Renderer::SetOption(Renderer::Option option, bool enable)
    {
        bool toggled = false;

        if (enable && !GetOption(option))
        {
            m_options |= option;
            toggled = true;
        }
        else if (!enable && GetOption(option))
        {
            m_options &= ~option;
            toggled = true;
        }

        if (!toggled)
            return;

        if (option == Renderer::Option::Upsample_TAA || option == Renderer::Option::Upsample_AMD_FidelityFX_SuperResolution)
        {
            CreateRenderTextures(false, false, false, true);
        }

        if (option == Renderer::Option::ReverseZ)
        {
            CreateDepthStencilStates();

            if (m_camera)
            {
                m_camera->MakeDirty();
            }
        }
    }

    void Renderer::SetOptionValue(Renderer::OptionValue option, float value)
    {
        if (!m_rhi_device || !m_rhi_device->GetContextRhi())
            return;

        if (option == Renderer::OptionValue::Anisotropy)
        {
            value = Util::Clamp(0.0f, 16.0f, value);
        }
        else if (option == Renderer::OptionValue::ShadowResolution)
        {
            value = Util::Clamp(static_cast<float>(m_resolution_shadow_min), static_cast<float>(m_rhi_device->GetMaxTexture2dDimension()), value);
        }

        if (m_option_values[option] == value)
            return;

        m_option_values[option] = value;

        if (option == Renderer::OptionValue::ShadowResolution)
        {
            const auto& light_entities = m_entities[ObjectType::Light];
            for (const auto& light_entity : light_entities)
            {
                auto light = light_entity->GetComponent<Light>();
                if (light->GetShadowsEnabled())
                {
                    light->CreateShadowMap();
                }
            }
        }
    }

    void Renderer::Present()
    {
        if (!m_swap_chain->PresentEnabled())
            return;

        m_swap_chain->Present();

        FIRE_EVENT(EventType::PostPresent);
    }

    void Renderer::Flush()
    {
        if (IsCallingFromOtherThread())
        {
            m_is_rendering_allowed = false;
            m_flush_requested = true;

            while (m_flush_requested)
            {
                LOG_INFO("External thread is waiting for the renderer thread to flush...");
                this_thread::sleep_for(chrono::milliseconds(16));
            }

            return;
        }

        {
            if (!m_is_rendering_allowed)
            {
                LOG_INFO("Renderer thread is flushing...");

                if (!m_rhi_device->QueueWaitAll())
                {
                    LOG_ERROR("Failed to flush GPU");
                }
            }

            if (m_cmd_current)
            {
                m_cmd_current->Discard();
            }
        }

        m_flush_requested = false;
    }

    RHI_Api_Type Renderer::GetApiType() const
    {
        return m_rhi_device->GetContextRhi()->api_type;
    }

    void Renderer::RequestTextureMipGeneration(shared_ptr<RHI_Texture> texture)
    {
        if (IsCallingFromOtherThread())
        {
            while (m_reading_requests)
            {
                LOG_INFO("External thread is waiting for the renderer thread...");
                this_thread::sleep_for(chrono::milliseconds(16));
            }
        }

        ASSERT(texture != nullptr);
        ASSERT(texture->GetResource_View_Srv() != nullptr);
        ASSERT(texture->HasMips());       
        ASSERT(texture->HasPerMipViews());

        lock_guard<mutex> guard(m_texture_mip_generation_mutex);
        m_textures_mip_generation_pending.push_back(texture);
    }

    uint32_t Renderer::GetCmdIndex() const
    {
        return m_cmd_pool->GetCommandListIndex();
    }
}