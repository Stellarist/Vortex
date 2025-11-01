#pragma once

#include <typeindex>
#include <atomic>

class Entity {
private:
	uint32_t uid;

	static std::atomic<uint32_t> id_counter;

public:
	Entity();
	virtual ~Entity() = default;

	uint32_t getUid() const;

	virtual std::type_index getType() = 0;
};
