module Runtime.World;

namespace Vortex {

Asset::Asset(std::string asset_name, std::string asset_path) :
    Object(std::move(asset_name)),
    virtual_path(std::move(asset_path))
{}

void Asset::touch() noexcept
{
	++revision;
}

const std::string& Asset::getVirtualPath() const noexcept
{
	return virtual_path;
}

uint64 Asset::getRevision() const noexcept
{
	return revision;
}

}        // namespace Vortex
