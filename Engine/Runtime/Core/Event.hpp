#pragma once

#include <memory>

#include "Input.hpp"

enum class EventType : uint32_t;

class Event {
protected:
	EventType type;

public:
	Event(EventType type);
	virtual ~Event() = default;

	EventType getType() const;
};

class InputEvent : public Event {
private:
	std::unique_ptr<Input> input;

public:
	InputEvent(std::unique_ptr<Input> input);

	Input* getInput() const;
};

enum class EventType : uint32_t {
	Input,
};

using EventCallback = std::function<void(const Event&)>;

struct EventListener {
	uint32_t      id;
	EventCallback callback;
};

class EventBus {
private:
	EventBus() = default;

	uint32_t callback_id{};

	std::unordered_map<uint32_t, std::vector<EventListener>> listeners;

public:
	static EventBus& instance();

	void emit(const Event& event);

	template <EventType E>
	uint32_t subscribe(const EventCallback& callback);

	template <EventType E>
	void unsubscribe(uint32_t callback_id);
};

template <EventType E>
uint32_t EventBus::subscribe(const EventCallback& callback)
{
	uint32_t id = callback_id++;
	listeners[static_cast<uint32_t>(E)].emplace_back(id, callback);

	return id;
}

template <EventType E>
void EventBus::unsubscribe(uint32_t callback_id)
{
	if (listeners.find(static_cast<uint32_t>(E)) == listeners.end())
		return;

	auto& callbacks = listeners[static_cast<uint32_t>(E)];
	callbacks.erase(std::remove_if(callbacks.begin(), callbacks.end(),
	                    [callback_id](const EventListener& rhs) {
		                    return rhs.id == callback_id;
	                    }),
	    callbacks.end());
}
