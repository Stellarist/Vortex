export module Runtime.World:Components.SceneComponent;

import Core;
import :Components.Component;

export namespace Vortex {

class SceneComponent : public Component {
private:
	Transform transform;

	SceneComponent*              attach_parent{};
	std::vector<SceneComponent*> attach_children;

public:
	SceneComponent(std::string name = "SceneComponent");
	~SceneComponent() noexcept override;

	Transform& getTransform() noexcept;
	const Transform& getTransform() const noexcept;

	Mat4 getWorldMatrix() const noexcept;
	Vec3 getWorldPosition() const noexcept;

	SceneComponent* getAttachParent() const noexcept;
	auto getAttachChildren() const noexcept -> const std::vector<SceneComponent*>&;

	auto attachTo(SceneComponent& parent) -> SceneComponent&;
	auto detach() -> SceneComponent&;
};

template <typename T>
concept IsSceneComponent = std::is_base_of_v<SceneComponent, T>;

}        // namespace Vortex
