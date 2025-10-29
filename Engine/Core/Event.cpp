#include "Event.hpp"

uint32_t EventSystem::callback_id = 0;

Event::Event(EventType type) :
    type(type)
{}

EventType Event::getType() const
{
	return type;
}

void EventSystem::emit(const Event& event)
{
	uint32_t event_type = static_cast<uint32_t>(event.getType());

	if (listeners.find(event_type) == listeners.end())
		return;

	for (const auto& listener : listeners[event_type])
		listener.callback(event);
}

uint32_t EventSystem::subscribe(uint32_t event_type, EventCallback callback)
{
	uint32_t      id = callback_id++;
	EventListener listener{id, std::move(callback)};

	listeners[static_cast<uint32_t>(event_type)].emplace_back(id, std::move(callback));

	return id;
}

void EventSystem::unsubscribe(uint32_t event_type, uint32_t callback_id)
{
	if (listeners.find(event_type) == listeners.end())
		return;

	auto& callbacks = listeners[event_type];
	callbacks.erase(std::remove_if(callbacks.begin(), callbacks.end(),
	                               [callback_id](const EventListener& rhs) { return rhs.id == callback_id; }),
	                callbacks.end());
}
