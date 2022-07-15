#pragma once

#include <memory>
#include "../RHI/RHI_Definition.h"

namespace PlayGround
{
	class Renderer;

	namespace Math
	{
		class Rectangle
		{
        public:
            Rectangle()
            {
                left = 0.0f;
                top = 0.0f;
                right = 0.0f;
                bottom = 0.0f;
            }

            Rectangle(const float left, const float top, const float right, const float bottom)
            {
                this->left = left;
                this->top = top;
                this->right = right;
                this->bottom = bottom;
            }

            Rectangle(const Rectangle& rectangle)
            {
                left = rectangle.left;
                top = rectangle.top;
                right = rectangle.right;
                bottom = rectangle.bottom;
            }

            ~Rectangle() = default;

            bool operator==(const Rectangle& rhs) const
            {
                return
                    left == rhs.left &&
                    top == rhs.top &&
                    right == rhs.right &&
                    bottom == rhs.bottom;
            }

            bool operator!=(const Rectangle& rhs) const
            {
                return
                    left != rhs.left ||
                    top != rhs.top ||
                    right != rhs.right ||
                    bottom != rhs.bottom;
            }

            inline bool IsDefined() const
            {
                return  left != 0.0f ||
                    top != 0.0f ||
                    right != 0.0f ||
                    bottom != 0.0f;
            }

            inline float Width()  const { return right - left; }
            inline float Height() const { return bottom - top; }

            // 정점을 합쳐서 사각형을 만든다.
            void Merge(const Vector2& point)
            {
                left = Math::Util::Min(left, point.x);
                top = Math::Util::Min(top, point.y);
                right = Math::Util::Max(right, point.x);
                bottom = Math::Util::Max(bottom, point.y);
            }

            bool CreateBuffers(Renderer* renderer);

            static int GetIndexCount() { return 6; }
            inline RHI_IndexBuffer* GetIndexBuffer()   const { return m_index_buffer.get(); }
            inline RHI_VertexBuffer* GetVertexBuffer() const { return m_vertex_buffer.get(); }

            float left = 0.0f;
            float top = 0.0f;
            float right = 0.0f;
            float bottom = 0.0f;

            static const Rectangle Zero;

        private:
            std::shared_ptr<RHI_VertexBuffer> m_vertex_buffer;
            std::shared_ptr<RHI_IndexBuffer> m_index_buffer;
        };
	}
}