export module Runtime.Render:Scene.Proxy;

import Core;
import Runtime.World;
import Runtime.RHI;
import :Scene.Resource;

export namespace Vortex {

class LightProxy {
public:
	enum class Type : uint8 {
		Directional,
		Point,
		Spot,
	};

private:
	uint64 id{};
	Type type{Type::Directional};

	Vec3 position{0.0f};
	Vec3 direction{0.0f, -1.0f, 0.0f};
	Vec3 color{1.0f};

	float intensity{1.0f};
	float range{};
	float inner_cone_angle{};
	float outer_cone_angle{};

	void writeConstants(void* destination) const;

	friend class RenderScene;

public:
	LightProxy(const LightComponent& component);
	void update(const LightComponent& component);

	uint64 getId() const noexcept { return id; }
	Type getType() const noexcept { return type; }
	const Vec3& getDirection() const noexcept { return direction; }

	static constexpr uint32 constantSize() noexcept { return 64; }
};


class MeshProxy {
public:
	struct Section {
		uint32 index{};
		MaterialResource* material{};
	};

private:
	uint64 id{};
	MeshResource* mesh{};
	std::vector<Section> sections;

	RHIRef<RHIBuffer> object_constant_buffer;
	RHIRef<RHIBufferView> object_constant_buffer_view;
	RHIRef<RHIBindingSet> object_binding_set;

	Mat4 model{1.0f};
	Bounds world_bounds;
	bool visible{true};
	bool casts_shadow{true};

	void updateConstants(RHIDevice& device);

	friend class RenderScene;

public:
	MeshProxy(RHIDevice& device, uint64 id, MeshResource& mesh,
	    RHIBindingLayout& object_layout);

	void update(RHIDevice& device, const MeshComponent& component, MeshResource& mesh,
	    const RenderResourceCache& resources);

	uint64 getId() const noexcept { return id; }
	MeshResource* getMesh() const noexcept { return mesh; }

	std::span<const Section> getSections() const noexcept { return sections; }
	RHIBindingSet* getBindingSet() const noexcept { return object_binding_set.get(); }

	const Bounds& getWorldBounds() const noexcept { return world_bounds; }
	const Mat4& getModelMatrix() const noexcept { return model; }

	bool isVisible() const noexcept { return visible; }
	bool castsShadow() const noexcept { return casts_shadow; }
};

}        // namespace Vortex
