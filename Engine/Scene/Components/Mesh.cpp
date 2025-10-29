#include "Mesh.hpp"

Mesh::Mesh(const std::string& name) :
    Component(name)
{}

std::type_index Mesh::getType()
{
	return typeid(Mesh);
}

const std::vector<std::shared_ptr<SubMesh>> Mesh::getSubmeshes() const
{
	return submeshes;
}

void Mesh::addSubmesh(std::shared_ptr<SubMesh> submesh)
{
	submeshes.push_back(submesh);
}
