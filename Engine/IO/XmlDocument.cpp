#include "Common.h"
#include "XmlDocument.h"
#include "pugixml.hpp"
#include "../Core/FileSystem.h"
#include "../Log/Logger.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Math/Vector4.h"

using namespace std;
using namespace pugi;
using namespace PlayGround::Math;

namespace PlayGround
{
    XmlDocument::XmlDocument()
    {
        // XML 도큐먼트 생성
        m_Document = make_unique<xml_document>();

        // XML 선언
        auto declarationNode = m_Document->append_child(node_declaration);
        declarationNode.append_attribute("version") = "1.0";
        declarationNode.append_attribute("encoding") = "ISO-8859-1";
        declarationNode.append_attribute("standalone") = "yes";
    }

    XmlDocument::~XmlDocument()
    {
        m_vecNodes.clear();
    }

    void XmlDocument::AddNode(const string& nodeName)
    {
        if (!m_Document)
            return;

        // 노드를 추가한다.
        const auto node = make_shared<xml_node>(m_Document->append_child(nodeName.c_str()));
        m_vecNodes.push_back(node);
    }

    bool XmlDocument::AddChildNode(const string& parentNodeName, const string& childNodeName)
    {
        // 만약 부모 노드가 없다면
        auto parentNode = GetNodeByName(parentNodeName);
        if (!parentNode)
        {
            // 노드 추가 불가능 로그를 출력
            LOG_WARNING("Can't add child node \"%s\", parent node \"%s\" doesn't exist.", childNodeName.c_str(), parentNodeName.c_str());
            return false;
        }

        // 자식 노드로 새롭게 추가한다.
        const auto node = make_shared<xml_node>(parentNode->append_child(childNodeName.c_str()));
        m_vecNodes.push_back(node);

        return true;
    }

    bool XmlDocument::AddAttribute(const string& nodeName, const string& attributeName, const string& value)
    {
        // 노드가 있는지부터 탐색후에 없다면 추가 불가능
        auto node = GetNodeByName(nodeName);
        if (!node)
        {
            LOG_WARNING("Can't add attribute \"%s\", node \"%s\" doesn't exist.", attributeName.c_str(), nodeName.c_str());
            return false;
        }

        node->append_attribute(attributeName.c_str()) = value.c_str();

        return true;
    }

    bool XmlDocument::AddAttribute(const string& nodeName, const string& attributeName, bool value)
    {
        const string valueStr = value ? "true" : "false";
        return AddAttribute(nodeName, attributeName, valueStr);
    }

    bool XmlDocument::AddAttribute(const string& nodeName, const string& attributeName, int value)
    {
        return AddAttribute(nodeName, attributeName, to_string(value));
    }

    bool XmlDocument::AddAttribute(const string& nodeName, const string& attributeName, uint32_t value)
    {
        return AddAttribute(nodeName, attributeName, to_string(value));
    }

    bool XmlDocument::AddAttribute(const string& nodeName, const string& attributeName, float value)
    {
        return AddAttribute(nodeName, attributeName, to_string(value));
    }

    bool XmlDocument::AddAttribute(const string& nodeName, const string& attributeName, double value)
    {
        return AddAttribute(nodeName, attributeName, to_string(value));
    }

    bool XmlDocument::AddAttribute(const string& nodeName, const string& attributeName, Vector2& value)
    {
        return AddAttribute(nodeName, attributeName, value.ToString());
    }

    bool XmlDocument::AddAttribute(const string& nodeName, const string& attributeName, Vector3& value)
    {
        return AddAttribute(nodeName, attributeName, value.ToString());
    }

    bool XmlDocument::AddAttribute(const string& nodeName, const string& attributeName, Vector4& value)
    {
        return AddAttribute(nodeName, attributeName, value.ToString());
    }

    // 속성을 찾아온다.
    bool XmlDocument::GetAttribute(const string& nodeName, const string& attributeName, string* value)
    {
        const xml_attribute attribute = GetAttribute(nodeName, attributeName);

        if (!attribute)
            return false;

        // 값을 가져온다.
        (*value) = attribute.value();

        return true;
    }

    bool XmlDocument::GetAttribute(const string& nodeName, const string& attributeName, int* value)
    {
        const xml_attribute attribute = GetAttribute(nodeName, attributeName);

        if (!attribute)
            return false;

        (*value) = attribute.as_int();

        return true;
    }

    bool XmlDocument::GetAttribute(const string& nodeName, const string& attributeName, uint32_t* value)
    {
        const xml_attribute attribute = GetAttribute(nodeName, attributeName);

        if (!attribute)
            return false;

        (*value) = attribute.as_uint();

        return true;
    }

    bool XmlDocument::GetAttribute(const string& nodeName, const string& attributeName, bool* value)
    {
        const xml_attribute attribute = GetAttribute(nodeName, attributeName);

        if (!attribute)
            return false;

        (*value) = attribute.as_bool();

        return true;
    }

    bool XmlDocument::GetAttribute(const string& nodeName, const string& attributeName, float* value)
    {
        const xml_attribute attribute = GetAttribute(nodeName, attributeName);

        if (!attribute)
            return false;

        (*value) = attribute.as_float();

        return true;
    }

    bool XmlDocument::GetAttribute(const string& nodeName, const string& attributeName, double* value)
    {
        const xml_attribute attribute = GetAttribute(nodeName, attributeName);

        if (!attribute)
            return false;

        (*value) = attribute.as_double();

        return true;
    }

    bool XmlDocument::GetAttribute(const std::string& nodeName, const std::string& attributeName, Vector2* value)
    {
        string valueStr;
        if (!GetAttribute(nodeName, attributeName, &valueStr))
            return false;

        value->x = static_cast<float>(atof(FileSystem::GetStringBetweenExpressions(valueStr, "X:", ",").c_str()));
        value->y = static_cast<float>(atof(FileSystem::GetStringAfterExpression(valueStr, "Y:").c_str()));

        return true;
    }

    bool XmlDocument::GetAttribute(const std::string& nodeName, const std::string& attributeName, Vector3* value)
    {
        string valueStr;
        if (!GetAttribute(nodeName, attributeName, &valueStr))
            return false;

        value->x = static_cast<float>(atof(FileSystem::GetStringBetweenExpressions(valueStr, "X:", ",").c_str()));
        value->y = static_cast<float>(atof(FileSystem::GetStringBetweenExpressions(valueStr, "Y:", ",").c_str()));
        value->z = static_cast<float>(atof(FileSystem::GetStringAfterExpression(valueStr, "Z:").c_str()));

        return true;
    }

    bool XmlDocument::GetAttribute(const std::string& nodeName, const std::string& attributeName, Vector4* value)
    {
        string valueStr;
        if (!GetAttribute(nodeName, attributeName, &valueStr))
            return false;

        value->x = static_cast<float>(atof(FileSystem::GetStringBetweenExpressions(valueStr, "X:", ",").c_str()));
        value->y = static_cast<float>(atof(FileSystem::GetStringBetweenExpressions(valueStr, "Y:", ",").c_str()));
        value->z = static_cast<float>(atof(FileSystem::GetStringBetweenExpressions(valueStr, "Z:", ",").c_str()));
        value->w = static_cast<float>(atof(FileSystem::GetStringAfterExpression(valueStr, "W:").c_str()));

        return true;
    }

    // 불러오기
    bool XmlDocument::Load(const string& filePath)
    {
        m_Document = make_unique<xml_document>();
        const xml_parse_result result = m_Document->load_file(filePath.c_str());

        if (result.status != status_ok)
        {
            if (result.status == status_file_not_found)
            {
                LOG_ERROR("File \"%s\" was not found.", filePath.c_str());
            }
            else
            {
                LOG_ERROR("%s", result.description());
            }

            m_Document.release();
            return false;
        }

        GetAllNodes();

        return true;
    }

    // 저장
    bool XmlDocument::Save(const string& path) const
    {
        if (!m_Document)
            return false;

        if (FileSystem::Exists(path))
        {
            FileSystem::Delete(path);
        }

        return m_Document->save_file(path.c_str());
    }

    xml_attribute XmlDocument::GetAttribute(const string& nodeName, const string& attributeName)
    {
        xml_attribute attribute;

        // 노드 존재 여부 확인
        const auto node = GetNodeByName(nodeName);
        if (!node)
        {
            LOG_WARNING("Can't get attribute \"%s\", node \"%s\" doesn't exist.", attributeName.c_str(), nodeName.c_str());
            return attribute;
        }

        // 속성 존재 여부 확인
        attribute = node->attribute(attributeName.c_str());
        if (!attribute)
        {
            LOG_WARNING("Can't get attribute, attribute \"%s\" doesn't exist.", attributeName.c_str());
        }

        return attribute;
    }

    shared_ptr<xml_node> XmlDocument::GetNodeByName(const string& name)
    {
        for (const auto& node : m_vecNodes)
        {
            if (node->name() == name)
            {
                return node;
            }
        }

        return shared_ptr<xml_node>();
    }

    void XmlDocument::GetAllNodes()
    {
        if (!m_Document)
            return;

        m_vecNodes.clear();
        m_vecNodes.shrink_to_fit();

        for (xml_node child = m_Document->first_child(); child; child = child.next_sibling())
        {
            m_vecNodes.push_back(make_shared<xml_node>(child));
            if (child.last_child())
            {
                GetNodes(child);
            }
        }
    }

    void XmlDocument::GetNodes(xml_node node)
    {
        if (!node)
            return;

        for (xml_node child = node.first_child(); child; child = child.next_sibling())
        {
            m_vecNodes.push_back(make_shared<xml_node>(child));
            if (child.last_child())
            {
                GetNodes(child);
            }
        }
    }
}