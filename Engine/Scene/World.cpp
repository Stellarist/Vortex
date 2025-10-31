#include "World.hpp"

World::World()
{}

World::World(std::unique_ptr<Scene>&& new_scene) :
    World()
{
	setActiveScene(std::move(new_scene));
}

void World::tick(float dt)
{
	if (!active_scene)
		return;

	active_scene->update(dt);
}

Scene* World::getActiveScene() const
{
	return active_scene.get();
}

void World::setActiveScene(std::unique_ptr<Scene>&& new_scene)
{
	active_scene = std::move(new_scene);
	if (!active_scene)
		return;

	active_scene->setWorld(*this);
	active_scene->start();

	auto cameras = active_scene->getComponents<Camera>();
	if (!cameras.empty())
		active_camera = cameras.front();
}

Camera* World::getActiveCamera() const
{
	return active_camera;
}

void World::setActiveCamera(Camera* camera)
{
	active_camera = camera;
}
