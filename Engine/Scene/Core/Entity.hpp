#pragma once

#include <typeindex>
#include <atomic>

class Entity {
private:
	uint64_t uid;

	static std::atomic<uint64_t> id_counter;

public:
	Entity();
	virtual ~Entity() = default;

	uint64_t getUid() const;

	virtual std::type_index getType() = 0;
};
