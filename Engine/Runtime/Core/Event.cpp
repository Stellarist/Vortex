#include "Event.hpp"

Event::Event(EventType type) :
    type(type)
{}

EventType Event::getType() const
{
	return type;
}

InputEvent::InputEvent(std::unique_ptr<Input> input) :
    Event(EventType::Input),
    input(std::move(input))
{}

Input* InputEvent::getInput() const
{
	return input.get();
}

EventBus& EventBus::instance()
{
	static EventBus bus;
	return bus;
}

void EventBus::emit(const Event& event)
{
	uint32_t event_type = static_cast<uint32_t>(event.getType());

	if (listeners.count(event_type) == 0)
		return;

	for (const auto& listener : listeners[event_type])
		listener.callback(event);
}
