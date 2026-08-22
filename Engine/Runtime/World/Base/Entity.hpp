export module Runtime.World:Entity;

import Core;

export namespace Vortex {

class Entity {
private:
	uint64 uid;

	static std::atomic<uint64> id_counter;

public:
	Entity();
	virtual ~Entity() = default;

	uint64 getUid() const;

	virtual std::type_index getType() = 0;
};

}        // namespace Vortex
