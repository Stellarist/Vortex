module Runtime.World;

namespace Vortex {

SubMesh::SubMesh(const std::string& name) :
    Resource(name)
{}

std::type_index SubMesh::getType()
{
	return typeid(SubMesh);
}

auto SubMesh::getVertices() const -> const std::vector<Vertex>&
{
	return vertex_data;
}

void SubMesh::setVertices(std::vector<Vertex> vertex_data)
{
	this->vertex_data = std::move(vertex_data);
	invalidateAABB();
}

auto SubMesh::getIndices() const -> const std::vector<uint32>&
{
	return index_data;
}

void SubMesh::setIndices(std::vector<uint32> index_data)
{
	this->index_data = std::move(index_data);
}

uint32 SubMesh::getVerticesCount() const
{
	return vertex_data.size();
}

uint32 SubMesh::getIndicesCount() const
{
	return index_data.size();
}

std::shared_ptr<Material> SubMesh::getMaterial() const
{
	return material;
}

void SubMesh::setMaterial(std::shared_ptr<Material> new_material)
{
	material = new_material;
}

const std::string& SubMesh::getShaderName() const
{
	return shader_name;
}

void SubMesh::setShaderName(const std::string& shader_name)
{
	this->shader_name = shader_name;
}

bool SubMesh::isVisible() const
{
	return visible;
}

void SubMesh::setVisible(bool visible)
{
	this->visible = visible;
}

const AABB& SubMesh::getAABB() const
{
	updateAABB();
	return aabb;
}

void SubMesh::invalidateAABB() const
{
	aabb_dirty = true;
}

void SubMesh::updateAABB() const
{
	if (!aabb_dirty)
		return;

	aabb.reset();
	for (const auto& vertex : vertex_data)
		aabb.expand(vertex.pos);

	aabb_dirty = false;
}

}        // namespace Vortex
