#pragma once

#include <unordered_map>
#include <vector>
#include <functional>
#include "../Core/Variant.h"

// Ư�� �̺�Ʈ�� �� �̺�Ʈ�� �����ϴ� �����ڸ� �����ϴ� ��ũ��

// �̺�Ʈ �ڵ鷯�� ���ٸ� �̿��ؼ� Ư�� �̺�Ʈ�� �߻������� �ݹ����� ȣ���Ѵ�.
#define EVENT_HANDLER(function)				[this](const PlayGround::Variant& var) { function(); }
#define EVENT_HANDLER_STATIC(function)		[](const PlayGround::Variant& var) { function(); }

#define EVENT_HANDLER_VARIANT(function)			[this](const PlayGround::Variant& var) { function(var); }
#define EVENT_HANDLER_VARIANT_STATIC(function)	[](const PlayGround::Variant& var) { function(var); }

// �ش� �̺�Ʈ�� �����ϰ��ִ� ��� �����ڵ鿡�� ��ȣ�� ������.
#define FIRE_EVENT(eventID)                     PlayGround::EventSystem::Get().Fire(eventID)
#define FIRE_EVENT_DATA(eventID, data)          PlayGround::EventSystem::Get().Fire(eventID, data)

// �̺�Ʈ ����, ���� ��� ��ũ��
#define SUBSCRIBE_TO_EVENT(eventID, function)       PlayGround::EventSystem::Get().Subscribe(eventID, function)
#define UNSUBSCRIBE_FROM_EVENT(eventID, function)   PlayGround::EventSystem::Get().Unsubscribe(eventID, function);

enum class EventType
{
    PostPresent,
    WorldSaveStart,            // �� ����
    WorldSavedEnd,             // �� ���� ��
    WorldLoadStart,            // �� �ε�
    WorldLoadEnd,              // �� �ε� ��
    WorldPreClear,             // �� Ŭ���� ��
    WorldClear,                // �� Ŭ����
    WorldResolve,              // �� �翬��
    WorldResolved,             // �� �翬�� ��
    EventSDL,                  // SDL �̺�Ʈ
    WindowOnFullScreenToggled
};

namespace PlayGround
{
    using subscriber = std::function<void(const Variant&)>;

    class EventSystem
    {
    public:
        static EventSystem& Get()
        {
            static EventSystem instance;
            return instance;
        }

        // �ش� �̺�Ʈ ID�� �����Ѵ�.
        void Subscribe(const EventType event_id, subscriber&& function)
        {
            m_mapSubScribers[event_id].push_back(std::forward<subscriber>(function));
        }

        // ���� ���
        void Unsubscribe(const EventType event_id, subscriber&& function)
        {
            // �Լ��� �ּҸ� ����Ѵ�.
            const size_t function_address = *reinterpret_cast<long*>(reinterpret_cast<char*>(&function));
            auto& subscribers = m_mapSubScribers[event_id];

            // �ݺ��ϸ� ���� �ּҸ� ���� ��츦 ã�´�.
            for (auto it = subscribers.begin(); it != subscribers.end();)
            {
                const size_t subscriber_address = *reinterpret_cast<long*>(reinterpret_cast<char*>(&(*it)));
                if (subscriber_address == function_address)
                {
                    it = subscribers.erase(it);
                    return;
                }
            }
        }

        // �����ڿ��� ��ȣ ����
        void Fire(const EventType event_id, const Variant& data = 0)
        {
            if (m_mapSubScribers.find(event_id) == m_mapSubScribers.end())
                return;

            for (const auto& sub : m_mapSubScribers[event_id])
            {
                sub(data);
            }
        }

        inline void Clear()
        {
            m_mapSubScribers.clear();
        }

    private:
        std::unordered_map<EventType, std::vector<subscriber>> m_mapSubScribers;
    };
}