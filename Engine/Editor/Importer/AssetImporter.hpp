export module Editor:AssetImporter;

import Runtime.World;

export namespace Vortex {

std::unique_ptr<World> loadGltfWorld(std::string_view scene_path);

}        // namespace Vortex
