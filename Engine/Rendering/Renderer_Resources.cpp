#include "Common.h"
#include "Renderer.h"
#include "Font/Font.h"
#include "../Utils/Geometry.h"
#include "../Resource/ResourceCache.h"
#include "../RHI/RHI_Texture2D.h"
#include "../RHI/RHI_Texture2DArray.h"
#include "../RHI/RHI_Shader.h"
#include "../RHI/RHI_Sampler.h"
#include "../RHI/RHI_BlendState.h"
#include "../RHI/RHI_ConstantBuffer.h"
#include "../RHI/RHI_RasterizerState.h"
#include "../RHI/RHI_DepthStencilState.h"
#include "../RHI/RHI_SwapChain.h"
#include "../RHI/RHI_StructuredBuffer.h"
#include "../RHI/RHI_VertexBuffer.h"
#include "../RHI/RHI_IndexBuffer.h"
#include "../RHI/RHI_TextureCube.h"

using namespace std;
using namespace PlayGround::Math;

#define RENDER_TARGET(rt_enum) m_render_targets[static_cast<uint8_t>(rt_enum)]

namespace PlayGround
{
    void Renderer::CreateConstantBuffers()
    {
        const uint32_t offset_count = 1024;

        m_cb_frame_gpu = make_shared<RHI_ConstantBuffer>(m_rhi_device, "frame");
        m_cb_frame_gpu->Create<Cb_Frame>(offset_count);

        m_cb_uber_gpu = make_shared<RHI_ConstantBuffer>(m_rhi_device, "uber");
        m_cb_uber_gpu->Create<Cb_Uber>(offset_count);

        m_cb_light_gpu = make_shared<RHI_ConstantBuffer>(m_rhi_device, "light");
        m_cb_light_gpu->Create<Cb_Light>(offset_count);

        m_cb_material_gpu = make_shared<RHI_ConstantBuffer>(m_rhi_device, "material");
        m_cb_material_gpu->Create<Cb_Material>(offset_count);
    }

    void Renderer::CreateStructuredBuffers()
    {
        static uint32_t counter = 0;
        const uint32_t element_count = 1;
        m_sb_counter = make_shared<RHI_StructuredBuffer>(m_rhi_device, static_cast<uint32_t>(sizeof(uint32_t)), element_count, static_cast<void*>(&counter));
    }

    void Renderer::CreateDepthStencilStates()
    {
        RHI_Comparison_Function reverse_z_aware_comp_func = GetOption(Renderer::Option::ReverseZ) ? RHI_Comparison_Function::GreaterEqual : RHI_Comparison_Function::LessEqual;

        m_depth_stencil_off_off = make_shared<RHI_DepthStencilState>(m_rhi_device, false, false, RHI_Comparison_Function::Never, false, false, RHI_Comparison_Function::Never);  // no depth or stencil
        m_depth_stencil_rw_off = make_shared<RHI_DepthStencilState>(m_rhi_device, true, true, reverse_z_aware_comp_func, false, false, RHI_Comparison_Function::Never);  // depth
        m_depth_stencil_r_off = make_shared<RHI_DepthStencilState>(m_rhi_device, true, false, reverse_z_aware_comp_func, false, false, RHI_Comparison_Function::Never);  // depth
        m_depth_stencil_off_r = make_shared<RHI_DepthStencilState>(m_rhi_device, false, false, RHI_Comparison_Function::Never, true, false, RHI_Comparison_Function::Equal);  // depth + stencil
        m_depth_stencil_rw_w = make_shared<RHI_DepthStencilState>(m_rhi_device, true, true, reverse_z_aware_comp_func, false, true, RHI_Comparison_Function::Always); // depth + stencil
    }

    void Renderer::CreateRasterizerStates()
    {
        float depth_bias = GetOption(Renderer::Option::ReverseZ) ? -m_depth_bias : m_depth_bias;
        float depth_bias_slope_scaled = GetOption(Renderer::Option::ReverseZ) ? -m_depth_bias_slope_scaled : m_depth_bias_slope_scaled;

        m_rasterizer_cull_back_solid = make_shared<RHI_RasterizerState>(m_rhi_device, RHI_CullMode::Back, RHI_PolygonMode::Solid, true, false, false);
        m_rasterizer_cull_back_wireframe = make_shared<RHI_RasterizerState>(m_rhi_device, RHI_CullMode::Back, RHI_PolygonMode::Wireframe, true, false, true);
        m_rasterizer_cull_none_solid = make_shared<RHI_RasterizerState>(m_rhi_device, RHI_CullMode::Back, RHI_PolygonMode::Solid, true, false, false);
        m_rasterizer_light_point_spot = make_shared<RHI_RasterizerState>(m_rhi_device, RHI_CullMode::Back, RHI_PolygonMode::Solid, true, false, false, depth_bias, m_depth_bias_clamp, depth_bias_slope_scaled);
        m_rasterizer_light_directional = make_shared<RHI_RasterizerState>(m_rhi_device, RHI_CullMode::Back, RHI_PolygonMode::Solid, false, false, false, depth_bias * 0.1f, m_depth_bias_clamp, depth_bias_slope_scaled);
    }

    void Renderer::CreateBlendStates()
    {
        m_blend_disabled = make_shared<RHI_BlendState>(m_rhi_device, false);
        m_blend_alpha = make_shared<RHI_BlendState>(m_rhi_device, true, RHI_Blend::Src_Alpha, RHI_Blend::Inv_Src_Alpha, RHI_Blend_Operation::Add, RHI_Blend::One, RHI_Blend::One, RHI_Blend_Operation::Add, 0.0f);
        m_blend_additive = make_shared<RHI_BlendState>(m_rhi_device, true, RHI_Blend::One, RHI_Blend::One, RHI_Blend_Operation::Add, RHI_Blend::One, RHI_Blend::One, RHI_Blend_Operation::Add, 1.0f);
    }

    void Renderer::CreateSamplers(const bool create_only_anisotropic /*= false*/)
    {
        float anisotropy = GetOptionValue<float>(Renderer::OptionValue::Anisotropy);
        RHI_Comparison_Function depth_comparison = GetOption(Renderer::Option::ReverseZ) ? RHI_Comparison_Function::Greater : RHI_Comparison_Function::Less;
        float mip_lod_bias = -log2(m_resolution_output.x / m_resolution_render.x);

        if (!create_only_anisotropic)
        {
            m_sampler_compare_depth = make_shared<RHI_Sampler>(m_rhi_device, RHI_Filter::Linear, RHI_Filter::Linear, RHI_Sampler_Mipmap_Mode::Nearest, RHI_Sampler_Address_Mode::Clamp, depth_comparison, 0.0f, true);
            m_sampler_point_clamp = make_shared<RHI_Sampler>(m_rhi_device, RHI_Filter::Nearest, RHI_Filter::Nearest, RHI_Sampler_Mipmap_Mode::Nearest, RHI_Sampler_Address_Mode::Clamp, RHI_Comparison_Function::Always);
            m_sampler_point_wrap = make_shared<RHI_Sampler>(m_rhi_device, RHI_Filter::Nearest, RHI_Filter::Nearest, RHI_Sampler_Mipmap_Mode::Nearest, RHI_Sampler_Address_Mode::Wrap, RHI_Comparison_Function::Always);
            m_sampler_bilinear_clamp = make_shared<RHI_Sampler>(m_rhi_device, RHI_Filter::Linear, RHI_Filter::Linear, RHI_Sampler_Mipmap_Mode::Nearest, RHI_Sampler_Address_Mode::Clamp, RHI_Comparison_Function::Always);
            m_sampler_bilinear_wrap = make_shared<RHI_Sampler>(m_rhi_device, RHI_Filter::Linear, RHI_Filter::Linear, RHI_Sampler_Mipmap_Mode::Nearest, RHI_Sampler_Address_Mode::Wrap, RHI_Comparison_Function::Always);
            m_sampler_trilinear_clamp = make_shared<RHI_Sampler>(m_rhi_device, RHI_Filter::Linear, RHI_Filter::Linear, RHI_Sampler_Mipmap_Mode::Linear, RHI_Sampler_Address_Mode::Clamp, RHI_Comparison_Function::Always);
        }

        m_sampler_anisotropic_wrap = make_shared<RHI_Sampler>(m_rhi_device, RHI_Filter::Linear, RHI_Filter::Linear, RHI_Sampler_Mipmap_Mode::Linear, RHI_Sampler_Address_Mode::Wrap, RHI_Comparison_Function::Always, anisotropy, false, mip_lod_bias);

        LOG_INFO("Mip load bias set to %f", mip_lod_bias);
    }

    void Renderer::CreateRenderTextures(const bool create_render, const bool create_output, const bool create_fixed, const bool create_dynamic)
    {
        uint32_t width_render = static_cast<uint32_t>(m_resolution_render.x);
        uint32_t height_render = static_cast<uint32_t>(m_resolution_render.y);

        uint32_t width_output = static_cast<uint32_t>(m_resolution_output.x);
        uint32_t height_output = static_cast<uint32_t>(m_resolution_output.y);

        Flush();

        uint32_t mip_count = 1;
        uint32_t width = width_render;
        uint32_t height = height_render;
        uint32_t smallest_dimension = 1;
        while (width > smallest_dimension && height > smallest_dimension)
        {
            width /= 2;
            height /= 2;
            mip_count++;
        }

        if (create_render)
        {
            RENDER_TARGET(RenderTarget::Frame_Render) = make_unique<RHI_Texture2D>(m_Context, width_render, height_render, mip_count, RHI_Format_R11G11B10_Float, RHI_Texture_Rt_Color | RHI_Texture_Uav | RHI_Texture_Srv | RHI_Texture_PerMipViews, "rt_frame_render");
            RENDER_TARGET(RenderTarget::Frame_Render_2) = make_unique<RHI_Texture2D>(m_Context, width_render, height_render, mip_count, RHI_Format_R11G11B10_Float, RHI_Texture_Rt_Color | RHI_Texture_Uav | RHI_Texture_Srv | RHI_Texture_PerMipViews, "rt_frame_render_2");

            RENDER_TARGET(RenderTarget::Gbuffer_Albedo) = make_shared<RHI_Texture2D>(m_Context, width_render, height_render, 1, RHI_Format_R8G8B8A8_Unorm, RHI_Texture_Rt_Color | RHI_Texture_Srv, "rt_gbuffer_albedo");
            RENDER_TARGET(RenderTarget::Gbuffer_Normal) = make_shared<RHI_Texture2D>(m_Context, width_render, height_render, 1, RHI_Format_R16G16B16A16_Float, RHI_Texture_Rt_Color | RHI_Texture_Srv, "rt_gbuffer_normal");
            RENDER_TARGET(RenderTarget::Gbuffer_Material) = make_shared<RHI_Texture2D>(m_Context, width_render, height_render, 1, RHI_Format_R8G8B8A8_Unorm, RHI_Texture_Rt_Color | RHI_Texture_Srv, "rt_gbuffer_material");
            RENDER_TARGET(RenderTarget::Gbuffer_Velocity) = make_shared<RHI_Texture2D>(m_Context, width_render, height_render, 1, RHI_Format_R16G16_Float, RHI_Texture_Rt_Color | RHI_Texture_Srv, "rt_gbuffer_velocity");
            RENDER_TARGET(RenderTarget::Gbuffer_Velocity_2) = make_shared<RHI_Texture2D>(m_Context, width_render, height_render, 1, RHI_Format_R16G16_Float, RHI_Texture_Rt_Color | RHI_Texture_Srv, "rt_gbuffer_velocity_2");
            RENDER_TARGET(RenderTarget::Gbuffer_Depth) = make_shared<RHI_Texture2D>(m_Context, width_render, height_render, 1, RHI_Format_D32_Float, RHI_Texture_Rt_DepthStencil | RHI_Texture_Rt_DepthStencilReadOnly | RHI_Texture_Srv, "rt_gbuffer_depth");

            RENDER_TARGET(RenderTarget::Light_Diffuse) = make_unique<RHI_Texture2D>(m_Context, width_render, height_render, 1, RHI_Format_R11G11B10_Float, RHI_Texture_Uav | RHI_Texture_Srv | RHI_Texture_CanBeCleared, "rt_light_diffuse");
            RENDER_TARGET(RenderTarget::Light_Diffuse_Transparent) = make_unique<RHI_Texture2D>(m_Context, width_render, height_render, 1, RHI_Format_R11G11B10_Float, RHI_Texture_Uav | RHI_Texture_Srv | RHI_Texture_CanBeCleared, "rt_light_diffuse_transparent");
            RENDER_TARGET(RenderTarget::Light_Specular) = make_unique<RHI_Texture2D>(m_Context, width_render, height_render, 1, RHI_Format_R11G11B10_Float, RHI_Texture_Uav | RHI_Texture_Srv | RHI_Texture_CanBeCleared, "rt_light_specular");
            RENDER_TARGET(RenderTarget::Light_Specular_Transparent) = make_unique<RHI_Texture2D>(m_Context, width_render, height_render, 1, RHI_Format_R11G11B10_Float, RHI_Texture_Uav | RHI_Texture_Srv | RHI_Texture_CanBeCleared, "rt_light_specular_transparent");
            RENDER_TARGET(RenderTarget::Light_Volumetric) = make_unique<RHI_Texture2D>(m_Context, width_render, height_render, 1, RHI_Format_R11G11B10_Float, RHI_Texture_Uav | RHI_Texture_Srv | RHI_Texture_CanBeCleared, "rt_light_volumetric");

            RENDER_TARGET(RenderTarget::Ssr) = make_shared<RHI_Texture2D>(m_Context, width_render, height_render, mip_count, RHI_Format_R16G16B16A16_Float, RHI_Texture_Uav | RHI_Texture_Srv | RHI_Texture_PerMipViews, "rt_ssr");

            RENDER_TARGET(RenderTarget::Ssao) = make_unique<RHI_Texture2D>(m_Context, width_render, height_render, 1, RHI_Format_R16G16B16A16_Snorm, RHI_Texture_Uav | RHI_Texture_Srv, "rt_ssao");
            RENDER_TARGET(RenderTarget::Ssao_Gi) = make_unique<RHI_Texture2D>(m_Context, width_render, height_render, 1, RHI_Format_R16G16B16A16_Snorm, RHI_Texture_Uav | RHI_Texture_Srv, "rt_ssao_gi");

            RENDER_TARGET(RenderTarget::Dof_Half) = make_unique<RHI_Texture2D>(m_Context, width_render / 2, height_render / 2, 1, RHI_Format_R16G16B16A16_Float, RHI_Texture_Uav | RHI_Texture_Srv, "rt_dof_half");
            RENDER_TARGET(RenderTarget::Dof_Half_2) = make_unique<RHI_Texture2D>(m_Context, width_render / 2, height_render / 2, 1, RHI_Format_R16G16B16A16_Float, RHI_Texture_Uav | RHI_Texture_Srv, "rt_dof_half_2");
        }

        if (create_output)
        {
            RENDER_TARGET(RenderTarget::Frame_Output) = make_unique<RHI_Texture2D>(m_Context, width_output, height_output, 1, RHI_Format_R11G11B10_Float, RHI_Texture_Rt_Color | RHI_Texture_Uav | RHI_Texture_Srv, "rt_frame_output");
            RENDER_TARGET(RenderTarget::Frame_Output_2) = make_unique<RHI_Texture2D>(m_Context, width_output, height_output, 1, RHI_Format_R11G11B10_Float, RHI_Texture_Rt_Color | RHI_Texture_Uav | RHI_Texture_Srv, "rt_frame_output_2");

            RENDER_TARGET(RenderTarget::Bloom) = make_shared<RHI_Texture2D>(m_Context, width_output, height_output, mip_count, RHI_Format_R11G11B10_Float, RHI_Texture_Uav | RHI_Texture_Srv | RHI_Texture_PerMipViews, "rt_bloom");
        }

        if (create_fixed)
        {
            RENDER_TARGET(RenderTarget::Brdf_Specular_Lut) = make_unique<RHI_Texture2D>(m_Context, 400, 400, 1, RHI_Format_R8G8_Unorm, RHI_Texture_Uav | RHI_Texture_Srv, "rt_brdf_specular_lut");
            m_brdf_specular_lut_rendered = false;
        }

        if (create_dynamic)
        {
            bool is_output_larger = width_output > width_render && height_output > height_render;
            uint32_t width = is_output_larger ? width_output : width_render;
            uint32_t height = is_output_larger ? height_output : height_render;
            RENDER_TARGET(RenderTarget::Blur) = make_unique<RHI_Texture2D>(m_Context, width, height, 1, RHI_Format_R16G16B16A16_Float, RHI_Texture_Uav | RHI_Texture_Srv, "rt_blur");

            {
                RHI_Texture* rt_taa_history = RENDER_TARGET(RenderTarget::Taa_History).get();
                bool upsampling_enabled = GetOption(Renderer::Option::Upsample_TAA);
                width = upsampling_enabled ? width_output : width_render;
                height = upsampling_enabled ? height_output : height_render;

                if (!rt_taa_history || (rt_taa_history->GetWidth() != width || rt_taa_history->GetHeight() != height))
                {
                    RENDER_TARGET(RenderTarget::Taa_History) = make_unique<RHI_Texture2D>(m_Context, width, height, 1, RHI_Format_R11G11B10_Float, RHI_Texture_Uav | RHI_Texture_Srv, "rt_taa_history");
                    LOG_INFO("Taa history resolution has been set to %dx%d", width, height);
                }
            }
        }
    }

    void Renderer::CreateShaders()
    {
        bool async = true;

        const auto dir_shaders = m_resource_cache->GetResourceDirectory(EResourceDirectory::Shaders) + "\\";

        m_shaders[Renderer::Shader::Gbuffer_V] = make_shared<RHI_Shader>(m_Context, RHI_Vertex_Type::PosTexNorTan);
        m_shaders[Renderer::Shader::Gbuffer_V]->Compile(RHI_Shader_Vertex, dir_shaders + "gbuffer.hlsl", async);
        m_shaders[Renderer::Shader::Gbuffer_P] = make_shared<RHI_Shader>(m_Context);
        m_shaders[Renderer::Shader::Gbuffer_P]->Compile(RHI_Shader_Pixel, dir_shaders + "gbuffer.hlsl", async);

        m_shaders[Renderer::Shader::Light_C] = make_shared<RHI_Shader>(m_Context);
        m_shaders[Renderer::Shader::Light_C]->Compile(RHI_Shader_Compute, dir_shaders + "light.hlsl", async);

        m_shaders[Renderer::Shader::FullscreenTriangle_V] = make_shared<RHI_Shader>(m_Context, RHI_Vertex_Type::Undefined);
        m_shaders[Renderer::Shader::FullscreenTriangle_V]->Compile(RHI_Shader_Vertex, dir_shaders + "fullscreen_triangle.hlsl", async);

        m_shaders[Renderer::Shader::Quad_V] = make_shared<RHI_Shader>(m_Context, RHI_Vertex_Type::PosTex);
        m_shaders[Renderer::Shader::Quad_V]->Compile(RHI_Shader_Vertex, dir_shaders + "quad.hlsl", async);

        {
            m_shaders[Renderer::Shader::Depth_Prepass_V] = make_shared<RHI_Shader>(m_Context, RHI_Vertex_Type::PosTex);
            m_shaders[Renderer::Shader::Depth_Prepass_V]->Compile(RHI_Shader_Vertex, dir_shaders + "depth_prepass.hlsl", async);

            m_shaders[Renderer::Shader::Depth_Prepass_P] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::Depth_Prepass_P]->Compile(RHI_Shader_Pixel, dir_shaders + "depth_prepass.hlsl", async);
        }

        {
            m_shaders[Renderer::Shader::Depth_Light_V] = make_shared<RHI_Shader>(m_Context, RHI_Vertex_Type::PosTex);
            m_shaders[Renderer::Shader::Depth_Light_V]->Compile(RHI_Shader_Vertex, dir_shaders + "depth_light.hlsl", async);

            m_shaders[Renderer::Shader::Depth_Light_P] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::Depth_Light_P]->Compile(RHI_Shader_Pixel, dir_shaders + "depth_light.hlsl", async);
        }

        m_shaders[Renderer::Shader::BrdfSpecularLut_C] = make_shared<RHI_Shader>(m_Context);
        m_shaders[Renderer::Shader::BrdfSpecularLut_C]->Compile(RHI_Shader_Compute, dir_shaders + "brdf_specular_lut.hlsl", async);

        {
            m_shaders[Renderer::Shader::Copy_Point_C] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::Copy_Point_C]->AddDefine("COMPUTE");
            m_shaders[Renderer::Shader::Copy_Point_C]->Compile(RHI_Shader_Compute, dir_shaders + "copy.hlsl", async);

            m_shaders[Renderer::Shader::Copy_Bilinear_C] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::Copy_Bilinear_C]->AddDefine("COMPUTE");
            m_shaders[Renderer::Shader::Copy_Bilinear_C]->AddDefine("BILINEAR");
            m_shaders[Renderer::Shader::Copy_Bilinear_C]->Compile(RHI_Shader_Compute, dir_shaders + "copy.hlsl", async);

            m_shaders[Renderer::Shader::Copy_Point_P] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::Copy_Point_P]->AddDefine("PIXEL");
            m_shaders[Renderer::Shader::Copy_Point_P]->AddDefine("BILINEAR");
            m_shaders[Renderer::Shader::Copy_Point_P]->Compile(RHI_Shader_Pixel, dir_shaders + "copy.hlsl", async);

            m_shaders[Renderer::Shader::Copy_Bilinear_P] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::Copy_Bilinear_P]->AddDefine("PIXEL");
            m_shaders[Renderer::Shader::Copy_Bilinear_P]->AddDefine("BILINEAR");
            m_shaders[Renderer::Shader::Copy_Bilinear_P]->Compile(RHI_Shader_Pixel, dir_shaders + "copy.hlsl", async);
        }

        {
            m_shaders[Renderer::Shader::BlurGaussian_C] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::BlurGaussian_C]->AddDefine("PASS_BLUR_GAUSSIAN");
            m_shaders[Renderer::Shader::BlurGaussian_C]->Compile(RHI_Shader_Compute, dir_shaders + "blur.hlsl", async);

            m_shaders[Renderer::Shader::BlurGaussianBilateral_C] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::BlurGaussianBilateral_C]->AddDefine("PASS_BLUR_BILATERAL_GAUSSIAN");
            m_shaders[Renderer::Shader::BlurGaussianBilateral_C]->Compile(RHI_Shader_Compute, dir_shaders + "blur.hlsl", async);
        }

        {
            m_shaders[Renderer::Shader::BloomLuminance_C] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::BloomLuminance_C]->AddDefine("LUMINANCE");
            m_shaders[Renderer::Shader::BloomLuminance_C]->Compile(RHI_Shader_Compute, dir_shaders + "bloom.hlsl", async);

            m_shaders[Renderer::Shader::BloomUpsampleBlendMip_C] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::BloomUpsampleBlendMip_C]->AddDefine("UPSAMPLE_BLEND_MIP");
            m_shaders[Renderer::Shader::BloomUpsampleBlendMip_C]->Compile(RHI_Shader_Compute, dir_shaders + "bloom.hlsl", async);

            m_shaders[Renderer::Shader::BloomBlendFrame_C] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::BloomBlendFrame_C]->AddDefine("BLEND_FRAME");
            m_shaders[Renderer::Shader::BloomBlendFrame_C]->Compile(RHI_Shader_Compute, dir_shaders + "bloom.hlsl", async);
        }

        m_shaders[Renderer::Shader::FilmGrain_C] = make_shared<RHI_Shader>(m_Context);
        m_shaders[Renderer::Shader::FilmGrain_C]->Compile(RHI_Shader_Compute, dir_shaders + "film_grain.hlsl", async);

        m_shaders[Renderer::Shader::ChromaticAberration_C] = make_shared<RHI_Shader>(m_Context);
        m_shaders[Renderer::Shader::ChromaticAberration_C]->AddDefine("PASS_CHROMATIC_ABERRATION");
        m_shaders[Renderer::Shader::ChromaticAberration_C]->Compile(RHI_Shader_Compute, dir_shaders + "chromatic_aberration.hlsl", async);

        m_shaders[Renderer::Shader::ToneMapping_C] = make_shared<RHI_Shader>(m_Context);
        m_shaders[Renderer::Shader::ToneMapping_C]->Compile(RHI_Shader_Compute, dir_shaders + "tone_mapping.hlsl", async);

        {
            m_shaders[Renderer::Shader::Taa_C] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::Taa_C]->Compile(RHI_Shader_Compute, dir_shaders + "temporal_antialiasing.hlsl", async);

            m_shaders[Renderer::Shader::Fxaa_C] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::Fxaa_C]->Compile(RHI_Shader_Compute, dir_shaders + "fxaa.hlsl", async);
        }

        {
            m_shaders[Renderer::Shader::Dof_DownsampleCoc_C] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::Dof_DownsampleCoc_C]->AddDefine("DOWNSAMPLE_CIRCLE_OF_CONFUSION");
            m_shaders[Renderer::Shader::Dof_DownsampleCoc_C]->Compile(RHI_Shader_Compute, dir_shaders + "depth_of_field.hlsl", async);

            m_shaders[Renderer::Shader::Dof_Bokeh_C] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::Dof_Bokeh_C]->AddDefine("BOKEH");
            m_shaders[Renderer::Shader::Dof_Bokeh_C]->Compile(RHI_Shader_Compute, dir_shaders + "depth_of_field.hlsl", async);

            m_shaders[Renderer::Shader::Dof_Tent_C] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::Dof_Tent_C]->AddDefine("TENT");
            m_shaders[Renderer::Shader::Dof_Tent_C]->Compile(RHI_Shader_Compute, dir_shaders + "depth_of_field.hlsl", async);

            m_shaders[Renderer::Shader::Dof_UpscaleBlend_C] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::Dof_UpscaleBlend_C]->AddDefine("UPSCALE_BLEND");
            m_shaders[Renderer::Shader::Dof_UpscaleBlend_C]->Compile(RHI_Shader_Compute, dir_shaders + "depth_of_field.hlsl", async);
        }

        m_shaders[Renderer::Shader::MotionBlur_C] = make_shared<RHI_Shader>(m_Context);
        m_shaders[Renderer::Shader::MotionBlur_C]->Compile(RHI_Shader_Compute, dir_shaders + "motion_blur.hlsl", async);

        m_shaders[Renderer::Shader::Debanding_C] = make_shared<RHI_Shader>(m_Context);
        m_shaders[Renderer::Shader::Debanding_C]->Compile(RHI_Shader_Compute, dir_shaders + "debanding.hlsl", async);

        m_shaders[Renderer::Shader::Ssao_C] = make_shared<RHI_Shader>(m_Context);
        m_shaders[Renderer::Shader::Ssao_C]->Compile(RHI_Shader_Compute, dir_shaders + "ssao.hlsl", async);

        {
            m_shaders[Renderer::Shader::Light_Composition_C] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::Light_Composition_C]->Compile(RHI_Shader_Compute, dir_shaders + "light_composition.hlsl", async);

            m_shaders[Renderer::Shader::Light_ImageBased_P] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::Light_ImageBased_P]->Compile(RHI_Shader_Pixel, dir_shaders + "light_image_based.hlsl", async);
        }

        m_shaders[Renderer::Shader::Ssr_C] = make_shared<RHI_Shader>(m_Context);
        m_shaders[Renderer::Shader::Ssr_C]->Compile(RHI_Shader_Compute, dir_shaders + "ssr.hlsl", async);

        m_shaders[Renderer::Shader::Entity_V] = make_shared<RHI_Shader>(m_Context, RHI_Vertex_Type::PosTexNorTan);
        m_shaders[Renderer::Shader::Entity_V]->Compile(RHI_Shader_Vertex, dir_shaders + "entity.hlsl", async);

        m_shaders[Renderer::Shader::Entity_Transform_P] = make_shared<RHI_Shader>(m_Context);
        m_shaders[Renderer::Shader::Entity_Transform_P]->AddDefine("TRANSFORM");
        m_shaders[Renderer::Shader::Entity_Transform_P]->Compile(RHI_Shader_Pixel, dir_shaders + "entity.hlsl", async);

        m_shaders[Renderer::Shader::Entity_Outline_P] = make_shared<RHI_Shader>(m_Context);
        m_shaders[Renderer::Shader::Entity_Outline_P]->AddDefine("OUTLINE");
        m_shaders[Renderer::Shader::Entity_Outline_P]->Compile(RHI_Shader_Pixel, dir_shaders + "entity.hlsl", async);

        m_shaders[Renderer::Shader::Font_V] = make_shared<RHI_Shader>(m_Context, RHI_Vertex_Type::PosTex);
        m_shaders[Renderer::Shader::Font_V]->Compile(RHI_Shader_Vertex, dir_shaders + "font.hlsl", async);
        m_shaders[Renderer::Shader::Font_P] = make_shared<RHI_Shader>(m_Context);
        m_shaders[Renderer::Shader::Font_P]->Compile(RHI_Shader_Pixel, dir_shaders + "font.hlsl", async);

        m_shaders[Renderer::Shader::Color_V] = make_shared<RHI_Shader>(m_Context, RHI_Vertex_Type::PosCol);
        m_shaders[Renderer::Shader::Color_V]->Compile(RHI_Shader_Vertex, dir_shaders + "color.hlsl", async);
        m_shaders[Renderer::Shader::Color_P] = make_shared<RHI_Shader>(m_Context);
        m_shaders[Renderer::Shader::Color_P]->Compile(RHI_Shader_Pixel, dir_shaders + "color.hlsl", async);

        m_shaders[Renderer::Shader::Reflection_Probe_V] = make_shared<RHI_Shader>(m_Context, RHI_Vertex_Type::PosTexNorTan);
        m_shaders[Renderer::Shader::Reflection_Probe_V]->Compile(RHI_Shader_Vertex, dir_shaders + "reflection_probe.hlsl", async);
        m_shaders[Renderer::Shader::Reflection_Probe_P] = make_shared<RHI_Shader>(m_Context);
        m_shaders[Renderer::Shader::Reflection_Probe_P]->Compile(RHI_Shader_Pixel, dir_shaders + "reflection_probe.hlsl", async);

        {
            m_shaders[Renderer::Shader::AMD_FidelityFX_CAS_C] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::AMD_FidelityFX_CAS_C]->Compile(RHI_Shader_Compute, dir_shaders + "amd_fidelityfx_cas.hlsl", async);

            m_shaders[Renderer::Shader::AMD_FidelityFX_SPD_C] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::AMD_FidelityFX_SPD_C]->Compile(RHI_Shader_Compute, dir_shaders + "amd_fidelityfx_spd.hlsl", async);
            m_shaders[Renderer::Shader::AMD_FidelityFX_SPD_LuminanceAntiflicker_C] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::AMD_FidelityFX_SPD_LuminanceAntiflicker_C]->AddDefine("LUMINANCE_ANTIFLICKER");
            m_shaders[Renderer::Shader::AMD_FidelityFX_SPD_LuminanceAntiflicker_C]->Compile(RHI_Shader_Compute, dir_shaders + "amd_fidelityfx_spd.hlsl", async);

            m_shaders[Renderer::Shader::AMD_FidelityFX_FSR_Upsample_C] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::AMD_FidelityFX_FSR_Upsample_C]->AddDefine("UPSAMPLE");
            m_shaders[Renderer::Shader::AMD_FidelityFX_FSR_Upsample_C]->Compile(RHI_Shader_Compute, dir_shaders + "amd_fidelityfx_fsr.hlsl", async);
            m_shaders[Renderer::Shader::AMD_FidelityFX_FSR_Sharpen_C] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::AMD_FidelityFX_FSR_Sharpen_C]->AddDefine("SHARPEN");
            m_shaders[Renderer::Shader::AMD_FidelityFX_FSR_Sharpen_C]->Compile(RHI_Shader_Compute, dir_shaders + "amd_fidelityfx_fsr.hlsl", async);
        }

        {
            m_shaders[Renderer::Shader::Debug_ReflectionProbe_V] = make_shared<RHI_Shader>(m_Context, RHI_Vertex_Type::PosTexNorTan);
            m_shaders[Renderer::Shader::Debug_ReflectionProbe_V]->Compile(RHI_Shader_Vertex, dir_shaders + "debug_reflection_probe.hlsl", async);
            m_shaders[Renderer::Shader::Debug_ReflectionProbe_P] = make_shared<RHI_Shader>(m_Context);
            m_shaders[Renderer::Shader::Debug_ReflectionProbe_P]->Compile(RHI_Shader_Pixel, dir_shaders + "debug_reflection_probe.hlsl", async);
        }
    }

    void Renderer::CreateFonts()
    {
        const string dir_font = m_resource_cache->GetResourceDirectory(EResourceDirectory::Fonts) + "/";

        m_font = make_unique<Font>(m_Context, dir_font + "Roboto-Bold.ttf", 16, Vector4(0.8f, 0.8f, 0.8f, 1.0f));
    }

    void Renderer::CreateMeshes()
    {
        vector<RHI_Vertex_PosTexNorTan> vertices;
        vector<uint32_t> indices;

        Utility::Geometry::CreateSphere(&vertices, &indices, 0.2f, 20, 20);

        m_sphere_vertex_buffer = make_shared<RHI_VertexBuffer>(m_rhi_device, false, "sphere");
        m_sphere_vertex_buffer->Create(vertices);

        m_sphere_index_buffer = make_shared<RHI_IndexBuffer>(m_rhi_device, false, "sphere");
        m_sphere_index_buffer->Create(indices);
    }

    void Renderer::CreateTextures()
    {
        const string dir_texture = m_resource_cache->GetResourceDirectory(EResourceDirectory::Textures) + "/";

        {
            m_tex_default_noise_normal = make_shared<RHI_Texture2D>(m_Context, RHI_Texture_Srv, "default_noise_normal");
            m_tex_default_noise_normal->LoadFromFile(dir_texture + "noise_normal.png");

            m_tex_default_noise_blue = static_pointer_cast<RHI_Texture>(make_shared<RHI_Texture2DArray>(m_Context, RHI_Texture_Srv, "default_noise_blue"));
            m_tex_default_noise_blue->LoadFromFile(dir_texture + "noise_blue_0.png");
        }

        {
            m_tex_default_white = make_shared<RHI_Texture2D>(m_Context, RHI_Texture_Srv, "default_white");
            m_tex_default_white->LoadFromFile(dir_texture + "white.png");

            m_tex_default_black = make_shared<RHI_Texture2D>(m_Context, RHI_Texture_Srv, "default_black");
            m_tex_default_black->LoadFromFile(dir_texture + "black.png");

            m_tex_default_transparent = make_shared<RHI_Texture2D>(m_Context, RHI_Texture_Srv, "default_transparent");
            m_tex_default_transparent->LoadFromFile(dir_texture + "transparent.png");
        }

        {
            m_tex_gizmo_light_directional = make_shared<RHI_Texture2D>(m_Context, RHI_Texture_Srv, "default_icon_light_directional");
            m_tex_gizmo_light_directional->LoadFromFile(dir_texture + "sun.png");

            m_tex_gizmo_light_point = make_shared<RHI_Texture2D>(m_Context, RHI_Texture_Srv, "default_icon_light_point");
            m_tex_gizmo_light_point->LoadFromFile(dir_texture + "light_bulb.png");

            m_tex_gizmo_light_spot = make_shared<RHI_Texture2D>(m_Context, RHI_Texture_Srv, "default_icon_light_spot");
            m_tex_gizmo_light_spot->LoadFromFile(dir_texture + "flashlight.png");
        }
    }
}