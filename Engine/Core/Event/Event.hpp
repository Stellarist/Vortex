#pragma once

#include <memory>

#include "Core/Input/Input.hpp"

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
