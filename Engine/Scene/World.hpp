#pragma once

#include <memory>

#include "Core/Scene.hpp"
#include "Components/Camera.hpp"

class World {
private:
	std::unique_ptr<Scene> active_scene;

	Camera* active_camera{};

public:
	World() = default;
	World(std::unique_ptr<Scene>&& scene);

	World(const World&) = delete;
	World& operator=(const World&) = delete;

	World(World&&) noexcept = default;
	World& operator=(World&&) noexcept = default;

	~World() = default;

	void tick(float dt);

	auto getActiveScene() const -> Scene*;
	void setActiveScene(std::unique_ptr<Scene>&& scene);

	auto getActiveCamera() const -> Camera*;
	void setActiveCamera(Camera* camera);
};
