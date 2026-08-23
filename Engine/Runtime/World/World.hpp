export module Runtime.World:World;

import Core;
import Runtime.Asset;
import :Scene;
import :CameraComponent;

export namespace Vortex {

class World {
private:
	std::unique_ptr<Scene> active_scene;
	CameraComponent*       active_camera{};
	AssetManager*          asset_manager{};

	void onComponentRegistered(Component& component);
	void onComponentUnregistering(Component& component);

	friend class Scene;

public:
	World() = default;
	World(AssetManager& asset_manager) noexcept;
	World(std::unique_ptr<Scene> scene);
	World(AssetManager& asset_manager, std::unique_ptr<Scene> scene);
	~World();

	World(const World&) = delete;
	World& operator=(const World&) = delete;
	World(World&&) noexcept = delete;
	World& operator=(World&&) noexcept = delete;

	void tick(float dt);

	Scene* getActiveScene() const noexcept;
	auto   setActiveScene(std::unique_ptr<Scene> scene) -> World&;

	CameraComponent* getActiveCamera() const noexcept;
	auto             setActiveCamera(CameraComponent* camera) -> World&;

	AssetManager* getAssetManager() const noexcept;
	auto          setAssetManager(AssetManager* asset_manager) noexcept -> World&;
};

}        // namespace Vortex
