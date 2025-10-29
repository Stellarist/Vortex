#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Scene/Core/Component.hpp"

class Node;

class Transform : public Component {
private:
	glm::vec3 translation{0.0f, 0.0f, 0.0f};
	glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
	glm::vec3 scale{1.0f, 1.0f, 1.0f};
	glm::mat4 world_matrix{1.0f};
	bool      update_world_matrix{true};

	Node* node{};

	void updateWorldTransform();

public:
	Transform() = default;
	~Transform() = default;

	std::type_index getType() override;

	auto getTranslation() const -> const glm::vec3&;
	void setTranslation(const glm::vec3& translation);

	auto getRotation() const -> const glm::quat&;
	void setRotation(const glm::quat& rotation);

	auto getScale() const -> const glm::vec3&;
	void setScale(const glm::vec3& scale);

	auto getNode() const -> Node&;
	void setNode(Node& node);

	auto getMatrix() const -> glm::mat4;
	auto getWorldMatrix() -> glm::mat4;
	void setMatrix(const glm::mat4& matrix);
	void invalidateWorldMatrix();
};
