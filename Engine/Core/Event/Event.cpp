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
