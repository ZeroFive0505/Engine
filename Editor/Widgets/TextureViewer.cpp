#include "TextureViewer.h"
#include "Rendering/Renderer.h"
#include "RHI/RHI_Texture.h"
#include "ImGuiHelper.h"

using namespace std;
using namespace PlayGround;
using namespace PlayGround::Math;

TextureViewer::TextureViewer(Editor* editor) : Widget(editor)
{
	// ÅØ½ºÃÄ ºä¾î À§Á¬ ÃÊ±âÈ­
	m_Title = "Texture viewer";
	m_Visible = false;
	m_Position = widget_position_screen_center;
	m_MinSize = Vector2(720.0f, 576.0f);
}

void TextureViewer::UpdateVisible()
{
	m_Renderer = m_Context->GetSubModule<Renderer>();

	static vector<string> render_target_options;
	if (render_target_options.empty())
	{
		render_target_options.emplace_back("None");

		// ¸ðµç ·»´õ Å¸°ÙÀ» °¡Á®¿Â´Ù.
		for (const shared_ptr<RHI_Texture>& render_target : m_Renderer->GetRenderTargets())
		{
			if (render_target)
				render_target_options.emplace_back(render_target->GetObjectName());
		}
	}

	ImGui::Text("Render target");
	ImGui::SameLine();
	ImGuiEX::ComboBox("##Render_target", render_target_options, &m_TextureIndex);

	// ÀÎµ¦½º·Î ·»´õ Å¸°ÙÀ» °¡Á®¿Â´Ù.
	if (shared_ptr<RHI_Texture> texture = m_Renderer->GetRenderTarget(static_cast<Renderer::RenderTarget>(m_TextureIndex)))
	{
		texture->SetFlag(RHI_Texture_Flags::RHI_Texture_Visualise);

		float bottom_padding = 200.0f;
		float texture_shrink_percentage_x = ImGui::GetWindowWidth() / static_cast<float>(texture->GetWidth());
		float texture_shrink_percentage_y = ImGui::GetWindowHeight() / static_cast<float>(texture->GetHeight() + bottom_padding);
		float texture_shrink_percentage = Math::Util::Min<float>(texture_shrink_percentage_x, texture_shrink_percentage_y);

		float width = static_cast<float>(texture->GetWidth()) * texture_shrink_percentage;
		float height = static_cast<float>(texture->GetHeight()) * texture_shrink_percentage;
		bool border = true;
		ImGuiEX::Image(texture.get(), Vector2(width, height), border);

		if (m_Magnifying_glass && ImGui::IsItemHovered())
		{
			const float region_sz = 32.0f;
			const float zoom = 16.0f;
			const ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			const ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);

			ImVec2 pos = ImGui::GetCursorScreenPos();
			ImGuiIO& io = ImGui::GetIO();
			float region_x = io.MousePos.x - pos.x - region_sz * 0.5f;
			float region_y = io.MousePos.y - pos.y - region_sz * 0.5f;

			ImGui::BeginTooltip();
			{
				region_x = Math::Util::Clamp<float>(0.0f, width - region_sz, region_x);
				region_y = Math::Util::Clamp<float>(0.0f, height - region_sz, region_y);

				ImVec2 uv0 = ImVec2(region_x / width, region_y / height);
				ImVec2 uv1 = ImVec2((region_x + region_sz) / width, (region_y + region_sz) / height);
				ImGui::Image(static_cast<ImTextureID>(texture.get()), ImVec2(region_sz * zoom, region_sz * zoom), uv0, uv1, tint_col, border_col);
			}
			ImGui::EndTooltip();
		}

		ImGui::BeginGroup();
		{
			ImGui::BeginGroup();
			ImGui::Text(("Name: " + texture->GetObjectName()).c_str());
			ImGui::Text(("Dimensions: " + to_string(texture->GetWidth()) + "x" + to_string(texture->GetHeight())).c_str());
			ImGui::Text(("Channels: " + to_string(texture->GetChannelCount())).c_str());
			ImGui::Text(("Format: " + RhiFormatToString(texture->GetFormat())).c_str());
			ImGui::EndGroup();

			ImGui::SameLine();
			ImGui::BeginGroup();
			ImGui::Text("Channels");
			ImGui::Checkbox("R", &m_ChannelR);
			ImGui::Checkbox("G", &m_ChannelG);
			ImGui::Checkbox("B", &m_ChannelB);
			ImGui::Checkbox("A", &m_ChannelA);
			ImGui::EndGroup();


			ImGui::SameLine();
			ImGui::BeginGroup();
			ImGui::Checkbox("Gamma correct", &m_GammaCorrect);
			ImGui::Checkbox("Pack", &m_Pack);
			ImGui::Checkbox("Boost", &m_Boost);
			ImGui::Checkbox("Abs", &m_Abs);
			ImGui::Checkbox("Point sampling", &m_PointSampling);
			ImGui::EndGroup();
		}
		ImGui::EndGroup();

		texture->SetFlag(RHI_Texture_Flags::RHI_Texture_Visualise_Channel_R, m_ChannelR);
		texture->SetFlag(RHI_Texture_Flags::RHI_Texture_Visualise_Channel_G, m_ChannelG);
		texture->SetFlag(RHI_Texture_Flags::RHI_Texture_Visualise_Channel_B, m_ChannelB);
		texture->SetFlag(RHI_Texture_Flags::RHI_Texture_Visualise_Channel_A, m_ChannelA);
		texture->SetFlag(RHI_Texture_Flags::RHI_Texture_Visualise_GammaCorrect, m_GammaCorrect);
		texture->SetFlag(RHI_Texture_Flags::RHI_Texture_Visualise_Pack, m_Pack);
		texture->SetFlag(RHI_Texture_Flags::RHI_Texture_Visualise_Boost, m_Boost);
		texture->SetFlag(RHI_Texture_Flags::RHI_Texture_Visualise_Abs, m_Abs);
		texture->SetFlag(RHI_Texture_Flags::RHI_Texture_Visualise_Sample_Point, m_PointSampling);
	}
}