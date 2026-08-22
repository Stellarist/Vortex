module Runtime.World;

namespace Vortex {

std::type_index Transform::getType()
{
	return typeid(Transform);
}

void Transform::translate(const Vec3& delta)
{
	translation += delta;
	invalidateWorldMatrix();
}

void Transform::rotate(const Vec3& axis, float angle)
{
	rotation = Math::normalize(Math::angleAxis(angle, axis) * rotation);
	invalidateWorldMatrix();
}

void Transform::scale(const Vec3& factor)
{
	scaling *= factor;
	invalidateWorldMatrix();
}

const Vec3& Transform::getTranslation() const
{
	return translation;
}

void Transform::setTranslation(const Vec3& translation)
{
	this->translation = translation;
	invalidateWorldMatrix();
}

const Quat& Transform::getRotation() const
{
	return rotation;
}

void Transform::setRotation(const Quat& rotation)
{
	this->rotation = rotation;
	invalidateWorldMatrix();
}

const Vec3& Transform::getScaling() const
{
	return scaling;
}

void Transform::setScaling(const Vec3& scale)
{
	this->scaling = scale;
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

Mat4 Transform::getMatrix() const
{
	return Math::translate(Mat4(1.0f), translation) * Math::toMat4(rotation) * Math::scale(Mat4(1.0f), scaling);
}

void Transform::setMatrix(const Mat4& matrix)
{
	this->world_matrix = matrix;
	invalidateWorldMatrix();
}

Mat4 Transform::getWorldMatrix()
{
	updateWorldTransform();
	return world_matrix;
}

void Transform::invalidateWorldMatrix()
{
	world_matrix_dirty = true;
}

void Transform::updateWorldTransform()
{
	if (!world_matrix_dirty)
		return;

	world_matrix = getMatrix();
	if (auto* parent = node->getParent(); parent) {
		auto& transform = parent->getTransform();
		world_matrix = transform.getWorldMatrix() * world_matrix;
	}

	world_matrix_dirty = false;
}

bool Transform::dirty() const
{
	return world_matrix_dirty;
}

}        // namespace Vortex
