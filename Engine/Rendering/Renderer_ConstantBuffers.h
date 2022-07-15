#pragma once

#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Math/Matrix.h"

// 렌더링시 이용되는 상수 버퍼들 모음

namespace PlayGround
{
	// 매 프레임마다 업데이트 되는 상수 버퍼 주로 카메라 관련 데이터가 여기에 포함
	struct Cb_Frame
	{
		// 뷰
		Math::Matrix view;
		// 프로젝션
		Math::Matrix projection;
		// 프로젝션 역
		Math::Matrix projection_inverted;
		// 직교 투영
		Math::Matrix projection_ortho;
		// 뷰 + 프로젝션
		Math::Matrix view_projection;
		// 뷰 + 프로젝션 역
		Math::Matrix view_projection_inv;
		// 뷰 + 프로젝션 직교
		Math::Matrix view_projection_ortho;
		// 뷰 + 프로젝션 (카메라 보정)
		Math::Matrix view_projection_unjittered;
		// 뷰 + 프로젝션 (이전 프레임)
		Math::Matrix view_projection_previous;

		// 델타 타임
		float delta_time;
		// 시간
		float time;
		// 프레임
		uint32_t frame;
		// 카메라 조리개
		float camera_aperture;

		// 카메라 속성
		float camera_shutter_speed;
		float camera_iso;
		float camera_near;
		float camera_far;

		// 카메라 위치
		Math::Vector3 camera_position;
		// 블룸 세기
		float bloom_intensity;

		// 샤프 필터 강도
		float sharpen_strength;
		// 카메라 방향
		Math::Vector3 camera_direction;

		// 톤 매핑 변수
		float gamma;
		float tonemapping;
		float directional_light_intensity;
		float shadow_resolution;

		// 출력용 버퍼 크기
		Math::Vector2 resolution_render;
		Math::Vector2 resolution_output;

		// TAA
		Math::Vector2 taa_jitter_current;
		Math::Vector2 taa_jitter_previous;

		// 안개
		float fog;
		// 버퍼 옵션
		uint32_t options;
		// Mip 갯수
		uint32_t frame_mip_count;
		// SSR Mip 갯수
		uint32_t ssr_mip_count;

		// 스카이박스 해상도
		Math::Vector2 resolution_environment;
		// 패딩
		Math::Vector2 padding;

		// 해당 상수 버퍼의 옵션 비트 온 오프 함수
		void set_bit(const bool set, const uint32_t bit)
		{
			options = set ? (options |= bit) : (options & ~bit);
		}

		bool operator==(const Cb_Frame& rhs) const
		{
			return
				view == rhs.view &&
				projection == rhs.projection &&
				projection_inverted == rhs.projection_inverted &&
				projection_ortho == rhs.projection_ortho &&
				view_projection == rhs.view_projection &&
				view_projection_inv == rhs.view_projection_inv &&
				view_projection_ortho == rhs.view_projection_ortho &&
				view_projection_unjittered == rhs.view_projection_unjittered &&
				view_projection_previous == rhs.view_projection_previous &&
				delta_time == rhs.delta_time &&
				time == rhs.time &&
				frame == rhs.frame &&
				camera_aperture == rhs.camera_aperture &&
				camera_shutter_speed == rhs.camera_shutter_speed &&
				camera_iso == rhs.camera_iso &&
				camera_near == rhs.camera_near &&
				camera_far == rhs.camera_far &&
				camera_position == rhs.camera_position &&
				sharpen_strength == rhs.sharpen_strength &&
				camera_direction == rhs.camera_direction &&
				gamma == rhs.gamma &&
				tonemapping == rhs.tonemapping &&
				directional_light_intensity == rhs.directional_light_intensity &&
				shadow_resolution == rhs.shadow_resolution &&
				fog == rhs.fog &&
				resolution_output == rhs.resolution_output &&
				resolution_render == rhs.resolution_render &&
				taa_jitter_current == rhs.taa_jitter_current &&
				taa_jitter_previous == rhs.taa_jitter_previous &&
				options == rhs.options &&
				frame_mip_count == rhs.frame_mip_count &&
				ssr_mip_count == rhs.ssr_mip_count &&
				resolution_environment == rhs.resolution_environment;
		}
		bool operator!=(const Cb_Frame& rhs) const { return !(*this == rhs); }
	};


	// 이동과 관련된 데이터가 주로 포함되어 있는 상수 버퍼 업데이트 빈도가 아주 높다.
	struct Cb_Uber
	{
		// 트랜스폼
		Math::Matrix transform = Math::Matrix::Identity;
		Math::Matrix transform_previous = Math::Matrix::Identity;

		// 리플렉션 프로브 위치
		Math::Vector3 float3 = Math::Vector3::Zero;
		// 블러 변수
		float blur_sigma = 0.0f;

		// 블러 방향
		Math::Vector2 blur_direction = Math::Vector2::Zero;
		// 렌더 타겟 해상도
		Math::Vector2 resolution_rt = Math::Vector2::Zero;

		// 렌더 타겟 입력
		Math::Vector2 resolution_in = Math::Vector2::Zero;
		// 텍스쳐 옵션
		uint32_t options_texture_visualisation = 0;
		float radius = 0.0f;

		// 마테리얼 색깔
		Math::Vector4 mat_color = Math::Vector4::Zero;

		// 텍스쳐 UV 변수들
		Math::Vector2 mat_tiling_uv = Math::Vector2::Zero;
		Math::Vector2 mat_offset_uv = Math::Vector2::Zero;

		// 마테리얼 성질
		float mat_roughness_mul = 0.0f;
		float mat_metallic_mul = 0.0f;
		float mat_normal_mul = 0.0f;
		float mat_height_mul = 0.0f;

		// 마테리얼 ID
		uint32_t mat_id = 0;
		// 텍스쳐 갯수
		uint32_t mat_textures = 0;
		// 불투명 여부
		uint32_t is_transparent_pass = 0;
		// Mip 갯수
		uint32_t mip_count = 0;

		// 크기
		Math::Vector3 extents = Math::Vector3::Zero;
		// 스레드
		uint32_t work_group_count = 0;

		// 리플렉션 프로브
		uint32_t reflection_proble_available = 0;
		Math::Vector3 padding = Math::Vector3::Zero;

		bool operator==(const Cb_Uber& rhs) const
		{
			return
				transform == rhs.transform &&
				transform_previous == rhs.transform_previous &&
				mat_id == rhs.mat_id &&
				mat_color == rhs.mat_color &&
				mat_tiling_uv == rhs.mat_tiling_uv &&
				mat_offset_uv == rhs.mat_offset_uv &&
				mat_roughness_mul == rhs.mat_roughness_mul &&
				mat_metallic_mul == rhs.mat_metallic_mul &&
				mat_normal_mul == rhs.mat_normal_mul &&
				mat_height_mul == rhs.mat_height_mul &&
				float3 == rhs.float3 &&
				blur_sigma == rhs.blur_sigma &&
				blur_direction == rhs.blur_direction &&
				is_transparent_pass == rhs.is_transparent_pass &&
				resolution_rt == rhs.resolution_rt &&
				resolution_in == rhs.resolution_in &&
				mip_count == rhs.mip_count &&
				work_group_count == rhs.work_group_count &&
				reflection_proble_available == rhs.reflection_proble_available &&
				radius == rhs.radius &&
				extents == rhs.extents &&
				mat_textures == rhs.mat_textures &&
				options_texture_visualisation == rhs.options_texture_visualisation;
		}

		bool operator!=(const Cb_Uber& rhs) const { return !(*this == rhs); }
	};

	// 광원 관련 상수 버퍼
	struct Cb_Light
	{
		Math::Matrix view_projection[6];
		Math::Vector4 intensity_range_angle_bias;
		Math::Vector3 color;
		float normal_bias;
		Math::Vector4 position;
		Math::Vector4 direction;
		uint32_t options;
		Math::Vector3 padding;

		bool operator==(const Cb_Light& rhs)
		{
			return
				view_projection[0] == rhs.view_projection[0] &&
				view_projection[1] == rhs.view_projection[1] &&
				view_projection[2] == rhs.view_projection[2] &&
				view_projection[3] == rhs.view_projection[3] &&
				view_projection[4] == rhs.view_projection[4] &&
				view_projection[5] == rhs.view_projection[5] &&
				intensity_range_angle_bias == rhs.intensity_range_angle_bias &&
				normal_bias == rhs.normal_bias &&
				color == rhs.color &&
				position == rhs.position &&
				direction == rhs.direction &&
				options == rhs.options;
		}
	};

	// 마테리얼 버퍼
	// 쉐이더 코드와 일치해야함
	static const uint32_t m_max_material_instances = 1024;
	struct Cb_Material
	{
		std::array<Math::Vector4, m_max_material_instances> mat_clearcoat_clearcoatRough_anis_anisRot;
		std::array<Math::Vector4, m_max_material_instances> mat_sheen_sheenTint_pad;


		bool operator==(const Cb_Material& rhs) const
		{
			return
				mat_clearcoat_clearcoatRough_anis_anisRot == rhs.mat_clearcoat_clearcoatRough_anis_anisRot &&
				mat_sheen_sheenTint_pad == rhs.mat_sheen_sheenTint_pad;
		}

		bool operator!=(const Cb_Material& rhs) const { return !(*this == rhs); }
	};
}