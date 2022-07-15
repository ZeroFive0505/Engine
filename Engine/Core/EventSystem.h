#pragma once

#include <unordered_map>
#include <vector>
#include <functional>
#include "../Core/Variant.h"

// 특정 이벤트와 그 이벤트를 구독하는 구독자를 설정하는 매크로

// 이벤트 핸들러는 람다를 이용해서 특정 이벤트가 발생했을시 콜백으로 호출한다.
#define EVENT_HANDLER(function)				[this](const PlayGround::Variant& var) { function(); }
#define EVENT_HANDLER_STATIC(function)		[](const PlayGround::Variant& var) { function(); }

#define EVENT_HANDLER_VARIANT(function)			[this](const PlayGround::Variant& var) { function(var); }
#define EVENT_HANDLER_VARIANT_STATIC(function)	[](const PlayGround::Variant& var) { function(var); }

// 해당 이벤트를 구독하고있는 모든 구독자들에게 신호를 보낸다.
#define FIRE_EVENT(eventID)                     PlayGround::EventSystem::Get().Fire(eventID)
#define FIRE_EVENT_DATA(eventID, data)          PlayGround::EventSystem::Get().Fire(eventID, data)

// 이벤트 구독, 구독 취소 매크로
#define SUBSCRIBE_TO_EVENT(eventID, function)       PlayGround::EventSystem::Get().Subscribe(eventID, function)
#define UNSUBSCRIBE_FROM_EVENT(eventID, function)   PlayGround::EventSystem::Get().Unsubscribe(eventID, function);

enum class EventType
{
    PostPresent,
    WorldSaveStart,            // 씬 저장
    WorldSavedEnd,             // 씬 저장 끝
    WorldLoadStart,            // 씬 로드
    WorldLoadEnd,              // 씬 로드 끝
    WorldPreClear,             // 씬 클리어 전
    WorldClear,                // 씬 클리어
    WorldResolve,              // 씬 재연산
    WorldResolved,             // 씬 재연산 끝
    EventSDL,                  // SDL 이벤트
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

        // 해당 이벤트 ID에 구독한다.
        void Subscribe(const EventType event_id, subscriber&& function)
        {
            m_mapSubScribers[event_id].push_back(std::forward<subscriber>(function));
        }

        // 구독 취소
        void Unsubscribe(const EventType event_id, subscriber&& function)
        {
            // 함수의 주소를 취득한다.
            const size_t function_address = *reinterpret_cast<long*>(reinterpret_cast<char*>(&function));
            auto& subscribers = m_mapSubScribers[event_id];

            // 반복하며 같은 주소를 같는 경우를 찾는다.
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

        // 구독자에게 신호 전송
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