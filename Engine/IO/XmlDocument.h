#pragma once

#include <memory>
#include <string>
#include <vector>
#include "../Core/EngineObject.h"

namespace pugi
{
	class xml_document;
	class xml_node;
	class xml_attribute;
}

namespace PlayGround
{
	namespace Math
	{
		class Vector2;
		class Vector3;
		class Vector4;
	}

	// XML을 이용하여 마테리얼 정보를 저장, 불러온다.

	class XmlDocument
	{
	public:
		XmlDocument();
		~XmlDocument();

		// 노드 추가
		void AddNode(const std::string& nodeName);
		bool AddChildNode(const std::string& parentNodeName, const std::string& childNodeName);

		// 속성 추가
		bool AddAttribute(const std::string& nodeName, const std::string& attributeName, const std::string& value);
		bool AddAttribute(const std::string& nodeName, const std::string& attributeName, bool value);
		bool AddAttribute(const std::string& nodeName, const std::string& attributeName, int value);
		bool AddAttribute(const std::string& nodeName, const std::string& attributeName, uint32_t value);
		bool AddAttribute(const std::string& nodeName, const std::string& attributeName, float value);
		bool AddAttribute(const std::string& nodeName, const std::string& attributeName, double value);
		bool AddAttribute(const std::string& nodeName, const std::string& attributeName, Math::Vector2& value);
		bool AddAttribute(const std::string& nodeName, const std::string& attributeName, Math::Vector3& value);
		bool AddAttribute(const std::string& nodeName, const std::string& attributeName, Math::Vector4& value);

		// 속성 가져오기
		bool GetAttribute(const std::string& nodeName, const std::string& attributeName, std::string* value);
		bool GetAttribute(const std::string& nodeName, const std::string& attributeName, int* value);
		bool GetAttribute(const std::string& nodeName, const std::string& attributeName, uint32_t* value);
		bool GetAttribute(const std::string& nodeName, const std::string& attributeName, bool* value);
		bool GetAttribute(const std::string& nodeName, const std::string& attributeName, float* value);
		bool GetAttribute(const std::string& nodeName, const std::string& attributeName, double* value);
		bool GetAttribute(const std::string& nodeName, const std::string& attributeName, Math::Vector2* value);
		bool GetAttribute(const std::string& nodeName, const std::string& attributeName, Math::Vector3* value);
		bool GetAttribute(const std::string& nodeName, const std::string& attributeName, Math::Vector4* value);

		// T 타입의 속성을 가져온다.
		template <typename T>
		T GetAttributeAs(const std::string& nodeName, const std::string& attributeName)
		{
			T value;
			GetAttribute(nodeName, attributeName, &value);
			return value;
		}

		bool Load(const std::string& filePath);
		bool Save(const std::string& filePath) const;
	private:
		pugi::xml_attribute GetAttribute(const std::string& nodeName, const std::string& attributeName);

		std::shared_ptr<pugi::xml_node> GetNodeByName(const std::string& name);

		void GetAllNodes();

		void GetNodes(pugi::xml_node node);

		std::unique_ptr<pugi::xml_document> m_Document;
		std::vector<std::shared_ptr<pugi::xml_node>> m_vecNodes;
	};
}

