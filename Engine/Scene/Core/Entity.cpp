#include "Entity.hpp"

std::atomic<uint32_t> Entity::id_counter = 0;

Entity::Entity() :
    uid(id_counter++)
{}

uint32_t Entity::getUid() const
{
	return uid;
}
