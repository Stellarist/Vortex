module Runtime.World;

namespace Vortex {

World::World(AssetManager& assets) noexcept :
    asset_manager(&assets)
{}

World::World(std::unique_ptr<Scene> scene)
{
	setActiveScene(std::move(scene));
}

World::World(AssetManager& assets, std::unique_ptr<Scene> scene) :
    asset_manager(&assets)
{
	setActiveScene(std::move(scene));
}

World::~World()
{
	if (active_scene) {
		active_scene->endPlay();
		active_scene->world = nullptr;
	}
}

void World::tick(float dt)
{
	if (active_scene)
		active_scene->tick(dt);
}

Scene* World::getActiveScene() const noexcept
{
	return active_scene.get();
}

World& World::setActiveScene(std::unique_ptr<Scene> scene)
{
	if (active_scene && (active_scene->updating || active_scene->dispatching_lifecycle))
		throw std::logic_error("The active scene cannot be replaced during tick or lifecycle callbacks");
	if (scene && scene->world && scene->world != this)
		throw std::logic_error("A scene cannot be active in multiple worlds");

	if (active_scene) {
		active_scene->endPlay();
		active_scene->world = nullptr;
	}

	active_camera = nullptr;
	active_scene = std::move(scene);
	if (!active_scene)
		return *this;

	active_scene->world = this;
	active_scene->beginPlay();

	auto cameras = active_scene->getComponents<CameraComponent>();
	if (!cameras.empty())
		active_camera = cameras.front();

	return *this;
}

CameraComponent* World::getActiveCamera() const noexcept
{
	return active_camera;
}

World& World::setActiveCamera(CameraComponent* camera)
{
	if (camera && (!active_scene || camera->getScene() != active_scene.get()))
		throw std::invalid_argument("The active camera must belong to the active scene");

	active_camera = camera;
	return *this;
}

AssetManager* World::getAssetManager() const noexcept
{
	return asset_manager;
}

World& World::setAssetManager(AssetManager* assets) noexcept
{
	asset_manager = assets;
	return *this;
}

void World::onComponentRegistered(Component& component)
{
	if (!active_camera)
		active_camera = dynamic_cast<CameraComponent*>(&component);
}

void World::onComponentUnregistering(Component& component)
{
	if (active_camera != &component)
		return;

	active_camera = nullptr;
	if (!active_scene)
		return;

	for (auto* camera : active_scene->getComponents<CameraComponent>())
		if (camera != &component) {
			active_camera = camera;
			break;
		}
}

}        // namespace Vortex
