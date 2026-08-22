export module Runtime.World:Transform;

import Core;
import :Component;

export namespace Vortex {

class Node;

class Transform : public Component {
private:
	Vec3 translation{0.0f, 0.0f, 0.0f};
	Quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
	Vec3 scaling{1.0f, 1.0f, 1.0f};
	Mat4 world_matrix{1.0f};

	bool world_matrix_dirty{true};

	void updateWorldTransform();

public:
	Transform() = default;
	~Transform() = default;

	std::type_index getType() override;

	void translate(const Vec3& delta);
	void rotate(const Vec3& axis, float angle);
	void scale(const Vec3& factor);

	auto getTranslation() const -> const Vec3&;
	void setTranslation(const Vec3& translation);

	auto getRotation() const -> const Quat&;
	void setRotation(const Quat& rotation);

	auto getScaling() const -> const Vec3&;
	void setScaling(const Vec3& scale);

	auto getNode() const -> Node&;
	void setNode(Node& node);

	auto getMatrix() const -> Mat4;
	void setMatrix(const Mat4& matrix);

	auto getWorldMatrix() -> Mat4;
	void invalidateWorldMatrix();

	bool dirty() const;
};

}        // namespace Vortex
