export module Core.Event;

import std;
import Core.Types;
import Core.Input;

export namespace Vortex {

enum class EventType : uint32;

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

enum class EventType : uint32 {
	Input,
};


using EventCallback = std::function<void(const Event&)>;


struct EventListener {
	uint32        id;
	EventCallback callback;
};


class EventBus {
private:
	EventBus() = default;

	uint32 callback_id{};

	std::unordered_map<uint32, std::vector<EventListener>> listeners;

public:
	static EventBus& instance();

	void emit(const Event& event);

	template <EventType E>
	uint32 subscribe(const EventCallback& callback);

	template <EventType E>
	void unsubscribe(uint32 callback_id);
};

template <EventType E>
uint32 EventBus::subscribe(const EventCallback& callback)
{
	uint32 id = callback_id++;
	listeners[static_cast<uint32>(E)].emplace_back(id, callback);

	return id;
}

template <EventType E>
void EventBus::unsubscribe(uint32 callback_id)
{
	if (listeners.find(static_cast<uint32>(E)) == listeners.end())
		return;

	auto& callbacks = listeners[static_cast<uint32>(E)];
	callbacks.erase(std::remove_if(callbacks.begin(), callbacks.end(), [callback_id](const EventListener& rhs) {
		return rhs.id == callback_id;
	}),
	    callbacks.end());
}

}        // namespace Vortex
