#pragma once

#include <typeindex>

class Entity {
private:
	uint64_t uid;

	static uint64_t id_counter;

public:
	Entity();
	virtual ~Entity() = default;

	uint64_t getUid() const;

	virtual std::type_index getType() = 0;
};
