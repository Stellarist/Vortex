#include "CameraController.hpp"

#include "Scene/Core/Node.hpp"

CameraController::CameraController(std::string name) :
    Behaviour(std::move(name))
{}

std::type_index CameraController::getType()
{
	return typeid(CameraController);
}

void CameraController::start()
{
	if (node && node->hasComponent<Camera>())
		camera = &node->getComponent<Camera>();
}

void CameraController::update(float dt)
{
}
