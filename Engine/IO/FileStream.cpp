#include "Common.h"
#include "FileStream.h"
#include "../RHI/RHI_Vertex.h"

using namespace std;

namespace PlayGround
{
	FileStream::FileStream(const string& path, uint32_t flags)
	{
		m_Open = false;
		m_Flags = flags;

		int ios_flags = ios::binary;
		ios_flags |= (flags & FileStream_Read) ? ios::in : 0;
		ios_flags |= (flags & FileStream_Write) ? ios::out : 0;
		ios_flags |= (flags & FileStream_Append) ? ios::app : 0;

		// 파일 입출력을 실시한다.

		if (m_Flags & FileStream_Write)
		{
			m_OutputStream.open(path, ios_flags);
			if (m_OutputStream.fail())
			{
				LOG_ERROR("Failed to open \"%s\" for writing", path.c_str());
				return;
			}
		}
		else if (m_Flags & FileStream_Read)
		{
			m_InputStream.open(path, ios_flags);
			if (m_InputStream.fail())
			{
				LOG_ERROR("Failed to open \"%s\" for reading", path.c_str());
				return;
			}
		}

		m_Open = true;
	}

	FileStream::~FileStream()
	{
		Close();
	}

	void FileStream::Close()
	{
		// 파일 스트림을 닫을 시 앎맞은 스트림의 버퍼를 초기화하고 닫는다.

		if (m_Flags & FileStream_Write)
		{
			m_OutputStream.flush();
			m_OutputStream.close();
		}
		else if (m_Flags & FileStream_Read)
		{
			m_InputStream.clear();
			m_InputStream.close();
		}
	}

	void FileStream::Write(const string& value)
	{
		const uint32_t length = static_cast<uint32_t>(value.length());
		Write(length);

		m_OutputStream.write(const_cast<char*>(value.c_str()), length);
	}

	void FileStream::Write(const vector<string>& value)
	{
		const uint32_t size = static_cast<uint32_t>(value.size());
		Write(size);

		for (uint32_t i = 0; i < size; i++)
		{
			Write(value[i]);
		}
	}

	void FileStream::Write(const vector<RHI_Vertex_PosTexNorTan>& value)
	{
		const uint32_t length = static_cast<uint32_t>(value.size());
		Write(length);
		m_OutputStream.write(reinterpret_cast<const char*>(&value[0]), sizeof(RHI_Vertex_PosTexNorTan) * length);
	}

	void FileStream::Write(const vector<uint32_t>& value)
	{
		const uint32_t size = static_cast<uint32_t>(value.size());
		Write(size);
		m_OutputStream.write(reinterpret_cast<const char*>(&value[0]), sizeof(unsigned char) * size);
	}

	void FileStream::Write(const vector<unsigned char>& value)
	{
		const uint32_t size = static_cast<uint32_t>(value.size());
		Write(size);
		m_OutputStream.write(reinterpret_cast<const char*>(&value[0]), sizeof(unsigned char) * size);
	}

	void FileStream::Write(const vector<std::byte>& value)
	{
		const uint32_t size = static_cast<uint32_t>(value.size());
		Write(size);
		m_OutputStream.write(reinterpret_cast<const char*>(&value[0]), sizeof(std::byte) * size);
	}

	void FileStream::Skip(const uint64_t n)
	{
		// 파일 탐색 위치를 설정한다.

		if (m_Flags & FileStream_Write)
			m_OutputStream.seekp(n, ios::cur);
		else if (m_Flags & FileStream_Read)
			m_InputStream.ignore(n, ios::cur);
	}

	void FileStream::Read(string* value)
	{
		uint32_t length = 0;
		Read(&length);

		value->resize(length);
		m_InputStream.read(const_cast<char*>(value->c_str()), length);
	}

	void FileStream::Read(vector<string>* vec)
	{
		if (!vec)
			return;

		vec->clear();
		vec->shrink_to_fit();

		uint32_t size = 0;
		Read(&size);

		string str;
		for (uint32_t i = 0; i < size; i++)
		{
			Read(&str);
			vec->emplace_back(str);
		}
	}

	void FileStream::Read(vector<RHI_Vertex_PosTexNorTan>* vec)
	{
		if (!vec)
			return;

		vec->clear();
		vec->shrink_to_fit();

		const uint32_t length = ReadAs<uint32_t>();

		vec->reserve(length);
		vec->resize(length);

		m_InputStream.read(reinterpret_cast<char*>(vec->data()), sizeof(RHI_Vertex_PosTexNorTan) * length);
	}

	void FileStream::Read(vector<uint32_t>* vec)
	{
		if (!vec)
			return;

		vec->clear();
		vec->shrink_to_fit();

		uint32_t length = ReadAs<uint32_t>();

		vec->reserve(length);
		vec->resize(length);

		m_InputStream.read(reinterpret_cast<char*>(vec->data()), sizeof(uint32_t) * length);
	}

	void FileStream::Read(vector<unsigned char>* vec)
	{
		if (!vec)
			return;

		vec->clear();
		vec->shrink_to_fit();

		const uint32_t length = ReadAs<uint32_t>();

		vec->reserve(length);
		vec->resize(length);

		m_InputStream.read(reinterpret_cast<char*>(vec->data()), sizeof(unsigned char) * length);
	}

	void FileStream::Read(vector<std::byte>* vec)
	{
		if (!vec)
			return;

		vec->clear();
		vec->shrink_to_fit();

		const uint32_t length = ReadAs<uint32_t>();

		vec->reserve(length);
		vec->resize(length);

		m_InputStream.read(reinterpret_cast<char*>(vec->data()), sizeof(std::byte) * length);
	}
}
