#pragma once

#include <string>
#include <functional>

enum class EventType : uint32_t;

class Event {
private:
	EventType type;

	int   key{};
	float mouse_x{};
	float mouse_y{};

	std::string name{};

public:
	Event(EventType type);
	~Event() = default;

	EventType getType() const;
};

using EventCallback = std::function<void(const Event&)>;

struct EventListener {
	uint32_t      id;
	EventCallback callback;
};

class EventSystem {
private:
	static uint32_t callback_id;

	std::unordered_map<uint32_t, std::vector<EventListener>> listeners;

public:
	void emit(const Event& event);

	uint32_t subscribe(uint32_t event_type, EventCallback callback);
	void     unsubscribe(uint32_t event_type, uint32_t callback_id);
};

enum class EventType : uint32_t {
	KEY_DOWN,
	KEY_UP,
	MOUSE_MOVE,
	MOUSE_BUTTON,

	FRAME_START,
	FRAME_END,
};
