#include "Entity.hpp"

std::atomic<uint64_t> Entity::id_counter = 0;

Entity::Entity() :
    uid(id_counter++)
{}

uint64_t Entity::getUid() const
{
	return uid;
}
