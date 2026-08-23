export module Runtime.World:PrimitiveComponent;

import :SceneComponent;

export namespace Vortex {

class PrimitiveComponent : public SceneComponent {
private:
	bool visible{true};

public:
	PrimitiveComponent(std::string name = "PrimitiveComponent");
	~PrimitiveComponent() override = default;

	bool isVisible() const noexcept;
	auto setVisible(bool visible) noexcept -> PrimitiveComponent&;
};

template <typename T>
concept IsPrimitiveComponent = std::is_base_of_v<PrimitiveComponent, T>;

}        // namespace Vortex
