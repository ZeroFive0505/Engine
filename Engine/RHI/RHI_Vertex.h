#pragma once

#include "../Math/Vector2.h"
#include "../Math//Vector3.h"
#include "../Math/Vector4.h"

namespace PlayGround
{
	struct RHI_Vertex_Undefined
	{

	};

	struct RHI_Vertex_Pos
	{
		RHI_Vertex_Pos(const Math::Vector3& position)
		{
			pos[0] = position.x;
			pos[1] = position.y;
			pos[2] = position.z;
		}

		RHI_Vertex_Pos(const float x, const float y, const float z)
		{
			pos[0] = x;
			pos[1] = y;
			pos[2] = z;
		}

		float pos[3] = { 0.0f };
	};

	struct RHI_Vertex_PosTex
	{
		RHI_Vertex_PosTex(const float x, const float y, const float z, const float u, const float v)
		{
			pos[0] = x;
			pos[1] = y;
			pos[2] = z;

			tex[0] = u;
			tex[1] = v;
		}

		RHI_Vertex_PosTex(const Math::Vector3& _pos, const Math::Vector2& _tex)
		{
			pos[0] = _pos.x;
			pos[1] = _pos.y;
			pos[2] = _pos.z;

			tex[0] = _tex.x;
			tex[1] = _tex.y;
		}

		float pos[3] = { 0.0f };
		float tex[2] = { 0.0f };
	};

	struct RHI_Vertex_PosCol
	{
		RHI_Vertex_PosCol() = default;

		RHI_Vertex_PosCol(const Math::Vector3& _pos, const Math::Vector4& _col)
		{
			pos[0] = _pos.x;
			pos[1] = _pos.y;
			pos[2] = _pos.z;

			col[0] = _col.x;
			col[1] = _col.y;
			col[2] = _col.z;
			col[3] = _col.w;
		}

		float pos[3] = { 0.0f };
		float col[4] = { 0.0f };
	};

	struct RHI_Vertex_Pos2DTexCol8
	{
		RHI_Vertex_Pos2DTexCol8() = default;

		RHI_Vertex_Pos2DTexCol8(const Math::Vector2& _pos, const Math::Vector2& _tex, const uint32_t _col)
		{
			pos[0] = _pos.x;
			pos[1] = _pos.y;

			tex[0] = _tex.x;
			tex[1] = _tex.y;

			col = _col;
		}

		RHI_Vertex_Pos2DTexCol8(const float x, const float y, const float u, const float v, const uint32_t _col)
		{
			pos[0] = x;
			pos[1] = y;

			tex[0] = u;
			tex[1] = v;

			col = _col;
		}

		float pos[2] = { 0.0f };
		float tex[2] = { 0.0f };
		uint32_t col = 0;
	};

	struct RHI_Vertex_PosTexNorTan
	{
		RHI_Vertex_PosTexNorTan() = default;

		RHI_Vertex_PosTexNorTan(const Math::Vector3& _pos, const Math::Vector2& _tex, const Math::Vector3& _nor = Math::Vector3::Zero,
			const Math::Vector3& _tan = Math::Vector3::Zero)
		{
			pos[0] = _pos.x;
			pos[1] = _pos.y;
			pos[2] = _pos.z;

			tex[0] = _tex.x;
			tex[1] = _tex.y;

			nor[0] = _nor.x;
			nor[1] = _nor.y;
			nor[2] = _nor.z;

			tan[0] = _tan.x;
			tan[1] = _tan.y;
			tan[2] = _tan.z;
		}

		float pos[3] = { 0.0f };
		float tex[2] = { 0.0f };
		float nor[3] = { 0.0f };
		float tan[3] = { 0.0f };
	};

	static_assert(std::is_trivially_copyable<RHI_Vertex_Pos>::value, "RHI_Vertex_Pos is not trivially copyable");
	static_assert(std::is_trivially_copyable<RHI_Vertex_PosTex>::value, "RHI_Vertex_PosTex is not trivially copyable");
	static_assert(std::is_trivially_copyable<RHI_Vertex_PosCol>::value, "RHI_Vertex_PosCol is not trivially copyable");
	static_assert(std::is_trivially_copyable<RHI_Vertex_Pos2DTexCol8>::value, "RHI_Vertex_Pos2dTexCol8 is not trivially copyable");
	static_assert(std::is_trivially_copyable<RHI_Vertex_PosTexNorTan>::value, "RHI_Vertex_PosTexNorTan is not trivially copyable");

	enum class RHI_Vertex_Type
	{
		Undefined,
		Pos,
		PosCol,
		PosTex,
		PosTexNorTan,
		Pos2DTexCol8
	};
}