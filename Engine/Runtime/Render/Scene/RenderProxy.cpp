module Runtime.Render;

namespace Vortex {

struct LightConstants {
	Vec4 position{0.0f};
	Vec4 direction{0.0f};
	Vec4 color{0.0f};
	Vec4 params{0.0f};
};

struct ObjectConstants {
	Mat4 model{1.0f};
	Mat4 normal_matrix{1.0f};
};

static_assert(sizeof(LightConstants) == LightProxy::constantSize());
static_assert(sizeof(ObjectConstants) == 128);


LightProxy::LightProxy(const LightComponent& component) :
    id(component.getUid())
{
	update(component);
}

void LightProxy::update(const LightComponent& component)
{
	CHECK(component.getUid() == id, "Cannot update a light proxy from another light");

	position = component.getWorldPosition();
	direction = {0.0f, -1.0f, 0.0f};
	color = component.getColor();
	intensity = component.getIntensity();

	range = 0.0f;
	inner_cone_angle = 0.0f;
	outer_cone_angle = 0.0f;

	if (const auto* directional = dynamic_cast<const DirectionalLightComponent*>(&component)) {
		type = Type::Directional;
		direction = directional->getDirection();

	} else if (const auto* point = dynamic_cast<const PointLightComponent*>(&component)) {
		type = Type::Point;
		range = point->getRange();

	} else if (const auto* spot = dynamic_cast<const SpotLightComponent*>(&component)) {
		type = Type::Spot;
		direction = spot->getDirection();
		range = spot->getRange();
		inner_cone_angle = spot->getInnerConeAngle();
		outer_cone_angle = spot->getOuterConeAngle();
	}
}

void LightProxy::writeConstants(void* destination) const
{
	auto& constants = *static_cast<LightConstants*>(destination);
	constants = {};
	constants.color = Vec4(color, intensity);

	switch (type) {
	case Type::Directional:
		constants.position = Vec4(direction, 0.0f);
		constants.direction = Vec4(direction, 0.0f);
		break;

	case Type::Point:
		constants.position = Vec4(position, 1.0f);
		constants.params = Vec4(range, 0.0f, 0.0f, 1.0f);
		break;

	case Type::Spot:
		constants.position = Vec4(position, 2.0f);
		constants.direction = Vec4(direction, 0.0f);
		constants.params = Vec4(range, inner_cone_angle,
		    outer_cone_angle, 2.0f);
		break;
	}
}


MeshProxy::MeshProxy(RHIDevice& new_device, uint64 new_id,
    MeshResource& new_mesh, RHIBindingLayout& object_layout) :
    id(new_id), mesh(&new_mesh)
{
	RHIBufferDesc constant_buffer_desc{};
	constant_buffer_desc.setSize(sizeof(ObjectConstants))
	    .setUsage(RHIBufferUsage::ConstantBuffer | RHIBufferUsage::CopyDest)
	    .setAccess(RHIAccessMode::Write);

	object_constant_buffer = new_device.createBuffer(constant_buffer_desc);
	object_constant_buffer->setName(std::format("Primitive.{}.Constants", id));

	object_constant_buffer_view = new_device.createBufferView(
	    RHIBufferViewDesc{}
	        .setBuffer(object_constant_buffer.get())
	        .setType(RHIBufferViewType::Constant));
	object_constant_buffer_view->setName(std::format("Primitive.{}.Constants.CBV", id));

	object_binding_set = new_device.createBindingSet(
	    RHIBindingSetDesc{}
	        .addItem(RHIBindingSetItem::constantBuffer(0, object_constant_buffer_view.get())),
	    object_layout);
	object_binding_set->setName(std::format("Primitive.{}.BindingSet", id));
}

void MeshProxy::updateConstants(RHIDevice& device)
{
	ObjectConstants constants{};
	constants.model = model;
	const Mat4 normal = Math::transpose(Math::inverse(model));
	constants.normal_matrix = Math::isFinite(normal) ? normal : Mat4{1.0f};

	void* mapped = device.mapBuffer(object_constant_buffer.get(), RHIAccessMode::Write);
	CHECK(mapped, "Failed to map the primitive constant buffer");

	std::memcpy(mapped, &constants, sizeof(constants));
	device.unmapBuffer(object_constant_buffer.get());
}

void MeshProxy::update(RHIDevice& device, const MeshComponent& component,
    MeshResource& new_mesh, const RenderResourceCache& resources)
{
	CHECK(component.getUid() == id, "Cannot update a mesh proxy from another component");
	CHECK(component.getMesh(), "Cannot update a mesh proxy without a mesh");

	mesh = &new_mesh;
	model = component.getWorldMatrix();
	world_bounds = component.getMesh()->getLocalBounds();
	world_bounds.transform(model);
	visible = component.isVisible() && component.isEnabled() &&
	    component.getOwner() && component.getOwner()->isEnabled();
	casts_shadow = component.castsShadow();

	sections.clear();
	const auto& mesh_sections = component.getMesh()->getSections();
	sections.reserve(mesh_sections.size());

	for (uint32 index = 0; index < mesh_sections.size(); ++index) {
		const auto material = component.getMaterial(mesh_sections[index].material_slot);
		if (auto* resource = resources.findMaterial(material))
			sections.push_back({index, resource});
	}
	updateConstants(device);
}

}        // namespace Vortex
