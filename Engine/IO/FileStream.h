#pragma once

#include <vector>
#include <fstream>
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Math/Vector4.h"
#include "../Math/Quaternion.h"
#include "../Math/BoundingBox.h"

namespace PlayGround
{
	class Entity;

	struct RHI_Vertex_PosTexNorTan;

	enum EFileStream_Mode : uint32_t
	{
		FileStream_Read = 1 << 0,
		FileStream_Write = 1 << 1,
		FileStream_Append = 1 << 2
	};
	
	// 파일 입출력을 위한 클래스
	class FileStream
	{
	public:
		FileStream(const std::string& path, uint32_t flags);
		~FileStream();

		inline bool IsOpen() const	{ return m_Open; }

		void Close();

		// 파일 출력시 엔진에서 제공되는 타입들만 쓸 수 있게한다.
		template <class T, class = typename std::enable_if <
			std::is_same<T, bool>::value ||
			std::is_same<T, unsigned char>::value ||
			std::is_same<T, int>::value ||
			std::is_same<T, long>::value ||
			std::is_same<T, long long>::value ||
			std::is_same<T, uint8_t>::value ||
			std::is_same<T, uint16_t>::value ||
			std::is_same<T, uint32_t>::value ||
			std::is_same<T, uint64_t>::value ||
			std::is_same<T, unsigned long>::value ||
			std::is_same<T, unsigned long long>::value ||
			std::is_same<T, float>::value ||
			std::is_same<T, double>::value ||
			std::is_same<T, long double>::value ||
			std::is_same<T, std::byte>::value ||
			std::is_same<T, Math::Vector2>::value ||
			std::is_same<T, Math::Vector3>::value ||
			std::is_same<T, Math::Vector4>::value ||
			std::is_same<T, Math::Quaternion>::value ||
			std::is_same<T, Math::BoundingBox>::value
		> ::type >
		void Write(T value)
		{
			m_OutputStream.write(reinterpret_cast<char*>(&value), sizeof(value));
		}

		void Write(const std::string& value);
		void Write(const std::vector<std::string>& value);
		void Write(const std::vector<RHI_Vertex_PosTexNorTan>& value);
		void Write(const std::vector<uint32_t>& value);
		void Write(const std::vector<unsigned char>& value);
		void Write(const std::vector<std::byte>& value);
		void Skip(const uint64_t n);

		// 파입 입력시도 마찬가지
		template <class T, class = typename std::enable_if
			<
			std::is_same<T, bool>::value ||
			std::is_same<T, unsigned char>::value ||
			std::is_same<T, int>::value ||
			std::is_same<T, long>::value ||
			std::is_same<T, long long>::value ||
			std::is_same<T, uint8_t>::value ||
			std::is_same<T, uint16_t>::value ||
			std::is_same<T, uint32_t>::value ||
			std::is_same<T, uint64_t>::value ||
			std::is_same<T, unsigned long>::value ||
			std::is_same<T, unsigned long long>::value ||
			std::is_same<T, float>::value ||
			std::is_same<T, double>::value ||
			std::is_same<T, long double>::value ||
			std::is_same<T, std::byte>::value ||
			std::is_same<T, Math::Vector2>::value ||
			std::is_same<T, Math::Vector3>::value ||
			std::is_same<T, Math::Vector4>::value ||
			std::is_same<T, Math::Quaternion>::value ||
			std::is_same<T, Math::BoundingBox>::value
		> ::type >
		void Read(T* value)
		{
			m_InputStream.read(reinterpret_cast<char*>(value), sizeof(T));
		}

		void Read(std::string* value);
		void Read(std::vector<std::string>* vec);
		void Read(std::vector<RHI_Vertex_PosTexNorTan>* vec);
		void Read(std::vector<uint32_t>* vec);
		void Read(std::vector<unsigned char>* vec);
		void Read(std::vector<std::byte>* vec);

		// T타입의 데이터가 만약 지원되는 데이터 타입이라면 불러온다.
		template <class T, class = typename std::enable_if
			<
			std::is_same<T, bool>::value ||
			std::is_same<T, unsigned char>::value ||
			std::is_same<T, int>::value ||
			std::is_same<T, long>::value ||
			std::is_same<T, long long>::value ||
			std::is_same<T, uint8_t>::value ||
			std::is_same<T, uint16_t>::value ||
			std::is_same<T, uint32_t>::value ||
			std::is_same<T, uint64_t>::value ||
			std::is_same<T, unsigned long>::value ||
			std::is_same<T, unsigned long long>::value ||
			std::is_same<T, float>::value ||
			std::is_same<T, double>::value ||
			std::is_same<T, long double>::value ||
			std::is_same<T, std::byte>::value ||
			std::is_same<T, std::string>::value
		>::type>
		T ReadAs()
		{
			T value;
			Read(&value);
			return value;
		}


	private:
		std::ofstream m_OutputStream;
		std::ifstream m_InputStream;
		uint32_t m_Flags;
		bool m_Open;
	};
}
