module Runtime.Render;

namespace Vortex {

RenderScene collectRenderScene(const World& world)
{
	RenderScene scene;

	for (const auto* component : world.getComponents<LightComponent>()) {
		if (!component || !component->isEnabled() || !component->getOwner() ||
		    !component->getOwner()->isEnabled())
			continue;
		RenderLight light{};
		light.id = component->getUid();
		light.position = component->getWorldPosition();
		light.color = component->getColor();
		light.intensity = component->getIntensity();
		if (const auto* directional = dynamic_cast<const DirectionalLightComponent*>(component)) {
			light.type = 0;
			light.direction = directional->getDirection();
		} else if (const auto* point = dynamic_cast<const PointLightComponent*>(component)) {
			light.type = 1;
			light.range = point->getRange();
		} else if (const auto* spot = dynamic_cast<const SpotLightComponent*>(component)) {
			light.type = 2;
			light.direction = spot->getDirection();
			light.range = spot->getRange();
			light.inner_cone = spot->getInnerConeAngle();
			light.outer_cone = spot->getOuterConeAngle();
		} else {
			continue;
		}
		scene.lights.push_back(light);
	}
	std::ranges::sort(scene.lights, {}, &RenderLight::id);

	for (const auto* component : world.getComponents<MeshComponent>()) {
		if (!component || !component->isEnabled() || !component->getOwner() ||
		    !component->getOwner()->isEnabled() || !component->isVisible())
			continue;
		const auto mesh = component->getMesh();
		if (!mesh || !mesh->valid())
			continue;
		RenderPrimitive primitive{};
		primitive.id = component->getUid();
		primitive.mesh_id = mesh->getUid();
		primitive.model = component->getWorldMatrix();
		primitive.bounds = mesh->getLocalBounds();
		primitive.bounds.transform(primitive.model);
		primitive.casts_shadow = component->castsShadow();
		for (const auto& section : mesh->getSections()) {
			auto material = component->getMaterial(section.material_slot);
			if (!material || !section.index_count)
				continue;
			const auto id = material->getUid();
			if (!scene.materials.contains(id))
				scene.materials.emplace(
				    id,
				    RenderMaterialData{
				        .id = id,
				        .revision = material->getRevision(),
				        .name = material->getName(),
				        .base_color = material->getAlbedo(),
				        .metallic = material->getMetallic(),
				        .roughness = material->getRoughness(),
				        .alpha_cutoff = material->getAlphaCutoff(),
				        .alpha_mode = material->getAlphaMode(),
				        .emissive = material->getEmissive(),
				        .textures = material->getTextures(),
				    });
			primitive.surfaces.push_back({section, id});
		}

		if (!primitive.surfaces.empty()) {
			scene.meshes.insert_or_assign(primitive.mesh_id, mesh);
			scene.primitives.push_back(std::move(primitive));
		}
	}

	std::ranges::sort(scene.primitives, {}, &RenderPrimitive::id);
	return scene;
}

static constexpr uint32 MaxLights = 1024;

static Vec4 row(const Mat4& matrix, uint32 index)
{
	return {matrix[0][index], matrix[1][index], matrix[2][index], matrix[3][index]};
}

static bool visible(const Bounds& bounds, const Mat4& vp)
{
	if (!bounds.valid())
		return true;
	const auto x = row(vp, 0), y = row(vp, 1);
	const auto z = row(vp, 2), w = row(vp, 3);
	const std::array planes{w + x, w - x, w + y, w - y, z, w - z};
	for (const auto& plane : planes) {
		const Vec3 extreme{
		    plane.x >= 0.0f ? bounds.max().x : bounds.min().x,
		    plane.y >= 0.0f ? bounds.max().y : bounds.min().y,
		    plane.z >= 0.0f ? bounds.max().z : bounds.min().z,
		};
		if (plane.x * extreme.x + plane.y * extreme.y +
		        plane.z * extreme.z + plane.w <
		    0.0f)
			return false;
	}
	return true;
}

RenderView collectRenderView(const World& world, RHIExtent extent)
{
	RenderView view{};
	if (const auto* camera = world.getActiveCamera(); camera && camera->isEnabled() &&
	    camera->getOwner() && camera->getOwner()->isEnabled()) {
		view = {
		    .id = camera->getUid(),
		    .view = camera->getView(),
		    .projection = camera->getProjection(),
		    .position = camera->getWorldPosition(),
		    .valid = true,
		};
	}

	view.extent = extent;
	return view;
}

RenderPreparedView prepareRenderView(const RenderScene& scene, const RenderManager& assets,
    const RenderView& camera, const RenderSettings& settings)
{
	RenderPreparedView frame{};
	frame.view = camera;
	const auto extent = camera.extent;
	RenderViewConstants view{};
	view.viewport_width = extent.width;
	view.viewport_height = extent.height;
	if (camera.valid) {
		view.camera.view = camera.view;
		view.camera.projection = camera.projection;
		view.camera.inverse_view_projection = Math::inverse(
		    camera.projection * camera.view);
		view.camera.position = Vec4(camera.position, 1.0f);
	}
	const auto light_count = std::min(scene.lights.size(), static_cast<size_t>(MaxLights));
	view.light_count = static_cast<uint32>(light_count);
	frame.constants = view;
	frame.lights.reserve(light_count);
	for (size_t index = 0; index < light_count; ++index) {
		const auto& light = scene.lights[index];
		RenderLightConstants data{};
		data.position = Vec4(light.position, static_cast<float>(light.type));
		data.direction = Vec4(light.direction, 0.0f);
		data.color = Vec4(light.color, light.intensity);
		data.params = Vec4(light.range, light.inner_cone, light.outer_cone,
		    static_cast<float>(light.type));
		frame.lights.push_back(data);
		if (light.type == 0 && !frame.shadow_direction)
			frame.shadow_direction = light.direction;
	}

	if (!camera.valid)
		return frame;

	const Mat4 vp = camera.projection * camera.view;
	struct RenderSortedDraw {
		float distance{};
		RenderDraw draw{};
	};
	std::vector<RenderSortedDraw> transparent;
	for (const auto& primitive : scene.primitives) {
		const auto* mesh = assets.findMesh(primitive.mesh_id);
		if (!mesh || !mesh->vertices || !mesh->indices)
			continue;
		const bool culled = settings.frustum_culling && !visible(primitive.bounds, vp);
		if (culled && !primitive.casts_shadow) {
			frame.stats.culled_draw_count += static_cast<uint32>(primitive.surfaces.size());
			continue;
		}
		const RenderDraw::PrimitiveConstants transform{
		    .model = primitive.model,
		    .normal_matrix = [&] {
			    const Mat4 candidate = Math::transpose(Math::inverse(primitive.model));
			    return Math::isFinite(candidate) ? candidate : Mat4{1.0f};
		    }(),
		};

		for (const auto& surface : primitive.surfaces) {
			const auto* material = assets.findMaterial(surface.material_id);
			if (!material || !material->binding)
				continue;
			RenderDraw draw{};
			draw.vertices.setBuffer(mesh->vertices.get());
			draw.indices.setBuffer(mesh->indices.get());
			draw.material = material->binding;
			draw.transform = transform;
			draw.arguments.setVertexCount(surface.section.index_count)
			    .setInstanceCount(1)
			    .setStartIndex(surface.section.first_index)
			    .setStartVertex(surface.section.first_vertex);
			const auto mode = scene.materials.at(surface.material_id).alpha_mode;
			if (primitive.casts_shadow && mode != MaterialAsset::AlphaMode::Blend) {
				frame.shadow.push_back(draw);
				++frame.stats.shadow_draw_count;
			}
			if (culled) {
				++frame.stats.culled_draw_count;
				continue;
			}
			if (mode == MaterialAsset::AlphaMode::Blend) {
				const Vec3 offset = Vec3(primitive.model[3]) - camera.position;
				transparent.push_back({Math::dot(offset, offset), std::move(draw)});
				++frame.stats.transparent_draw_count;
			} else if (mode == MaterialAsset::AlphaMode::Mask || mode == MaterialAsset::AlphaMode::Opaque) {
				frame.opaque.push_back(std::move(draw));
				if (mode == MaterialAsset::AlphaMode::Mask)
					++frame.stats.masked_draw_count;
				else
					++frame.stats.opaque_draw_count;
			}
		}
	}
	std::stable_sort(transparent.begin(), transparent.end(),
	    [](const RenderSortedDraw& a, const RenderSortedDraw& b) { return a.distance > b.distance; });
	for (auto& entry : transparent)
		frame.transparent.push_back(std::move(entry.draw));
	frame.stats.draw_count = frame.stats.opaque_draw_count +
	    frame.stats.masked_draw_count + frame.stats.transparent_draw_count;
	return frame;
}

}        // namespace Vortex
