#include "Transform.hpp"

#include "Scene/Core/Node.hpp"

std::type_index Transform::getType()
{
	return typeid(Transform);
}

const glm::vec3& Transform::getTranslation() const
{
	return translation;
}

void Transform::setTranslation(const glm::vec3& translation)
{
	this->translation = translation;
	invalidateWorldMatrix();
}

const glm::quat& Transform::getRotation() const
{
	return rotation;
}

void Transform::setRotation(const glm::quat& rotation)
{
	this->rotation = rotation;
	invalidateWorldMatrix();
}

const glm::vec3& Transform::getScale() const
{
	return scale;
}

void Transform::setScale(const glm::vec3& scale)
{
	this->scale = scale;
	invalidateWorldMatrix();
}

Node& Transform::getNode() const
{
	return *node;
}

void Transform::setNode(Node& node)
{
	this->node = &node;
}

glm::mat4 Transform::getMatrix() const
{
	return glm::translate(glm::mat4(1.0f), translation) *
	       glm::mat4_cast(rotation) *
	       glm::scale(glm::mat4(1.0f), scale);
}

glm::mat4 Transform::getWorldMatrix()
{
	updateWorldTransform();
	return world_matrix;
}

void Transform::setMatrix(const glm::mat4& matrix)
{
	this->world_matrix = matrix;
	invalidateWorldMatrix();
}

void Transform::invalidateWorldMatrix()
{
	update_world_matrix = true;
}

void Transform::updateWorldTransform()
{
	if (!update_world_matrix)
		return;

	world_matrix = getMatrix();
	if (auto* parent = node->getParent(); parent) {
		auto& transform = parent->getTransform();
		world_matrix = transform.getWorldMatrix() * world_matrix;
	}

	update_world_matrix = false;
}
