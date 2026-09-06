module Runtime.Render;

namespace Vortex {

static Vec4 matrixRow(const Mat4& matrix, uint32 row) noexcept
{
	return {matrix[0][row], matrix[1][row], matrix[2][row], matrix[3][row]};
}

static bool intersectsFrustum(const Bounds& bounds, const Mat4& view_projection) noexcept
{
	if (!bounds.valid())
		return true;

	const auto row0 = matrixRow(view_projection, 0);
	const auto row1 = matrixRow(view_projection, 1);
	const auto row2 = matrixRow(view_projection, 2);
	const auto row3 = matrixRow(view_projection, 3);
	const std::array planes{
	    row3 + row0,
	    row3 - row0,
	    row3 + row1,
	    row3 - row1,
	    row2,
	    row3 - row2,
	};

	for (const auto& plane : planes) {
		const Vec3 positive{
		    plane.x >= 0.0f ? bounds.max().x : bounds.min().x,
		    plane.y >= 0.0f ? bounds.max().y : bounds.min().y,
		    plane.z >= 0.0f ? bounds.max().z : bounds.min().z,
		};
		if (plane.x * positive.x + plane.y * positive.y +
		        plane.z * positive.z + plane.w <
		    0.0f)
			return false;
	}
	return true;
}

struct VisibleDraw {
	const MeshProxy* proxy{};
	const MeshProxy::Section* section{};
};

static DrawPacket makeDrawPacket(const VisibleDraw& draw)
{
	CHECK(draw.proxy && draw.proxy->getMesh() &&
	        draw.proxy->getMesh()->getSource() && draw.section &&
	        draw.section->material,
	    "Cannot create a draw packet from an incomplete mesh proxy");
	const auto& mesh = *draw.proxy->getMesh();
	const auto& sections = mesh.getSource()->getSections();
	CHECK(Range, draw.section->index < sections.size(),
	    "Mesh proxy references an invalid mesh section");
	const auto& section = sections[draw.section->index];

	DrawPacket packet{};
	packet.vertex_buffer.setBuffer(mesh.getVertexBuffer());
	packet.index_buffer.setBuffer(mesh.getIndexBuffer());
	packet.material_binding = draw.section->material->getBindingSet();
	packet.object_binding = draw.proxy->getBindingSet();
	packet.arguments.setVertexCount(section.index_count)
	    .setInstanceCount(1)
	    .setStartIndex(section.first_index)
	    .setStartVertex(section.first_vertex)
	    .setStartInstance(0);
	return packet;
}

static DrawList makeDrawList(DrawListType type,
    std::span<const VisibleDraw> primary,
    std::span<const VisibleDraw> secondary = {})
{
	std::vector<DrawPacket> packets;
	packets.reserve(primary.size() + secondary.size());
	auto append = [&packets](std::span<const VisibleDraw> source) {
		for (const auto& draw : source)
			packets.push_back(makeDrawPacket(draw));
	};
	append(primary);
	append(secondary);
	return DrawList(DrawListDesc{.type = type, .draws = std::move(packets)});
}

RenderFrame RenderViewBuilder::build(RenderScene& scene,
    const RenderViewDesc& view_desc, bool enable_frustum_culling) const
{
	scene.updateView(view_desc);

	std::vector<VisibleDraw> opaque_draws;
	std::vector<VisibleDraw> masked_draws;
	std::vector<VisibleDraw> transparent_draws;
	std::vector<VisibleDraw> shadow_draws;
	size_t culled_draw_count = 0;

	const bool cull = enable_frustum_culling && view_desc.has_camera;
	const Mat4 view_projection = cull ?
	    view_desc.projection * view_desc.view :
	    Mat4(1.0f);

	for (const auto& [id, owned_proxy] : scene.mesh_proxies) {
		const auto* proxy = owned_proxy.get();
		if (!proxy || !proxy->getMesh() || !proxy->isVisible())
			continue;
		const bool culled = cull &&
		    !intersectsFrustum(proxy->getWorldBounds(), view_projection);
		for (const auto& section : proxy->getSections()) {
			if (!section.material)
				continue;
			const VisibleDraw draw{proxy, &section};
			if (proxy->castsShadow())
				shadow_draws.push_back(draw);
			if (culled) {
				++culled_draw_count;
				continue;
			}

			switch (section.material->getSource()->getAlphaMode()) {
			case MaterialAsset::AlphaMode::Opaque:
				opaque_draws.push_back(draw);
				break;
			case MaterialAsset::AlphaMode::Mask:
				masked_draws.push_back(draw);
				break;
			case MaterialAsset::AlphaMode::Blend:
				transparent_draws.push_back(draw);
				break;
			case MaterialAsset::AlphaMode::Count:
				break;
			}
		}
	}

	if (view_desc.has_camera) {
		const Vec3 camera_position = view_desc.position;
		std::stable_sort(transparent_draws.begin(), transparent_draws.end(),
		    [&camera_position](const VisibleDraw& lhs, const VisibleDraw& rhs) {
			    const Vec3 lhs_position = Vec3(lhs.proxy->getModelMatrix()[3]);
			    const Vec3 rhs_position = Vec3(rhs.proxy->getModelMatrix()[3]);
			    const Vec3 lhs_delta = lhs_position - camera_position;
			    const Vec3 rhs_delta = rhs_position - camera_position;
			    return Math::dot(lhs_delta, lhs_delta) >
			        Math::dot(rhs_delta, rhs_delta);
		    });
	}

	std::optional<Vec3> light_direction;
	if (const auto* light = scene.getMainDirectionalLight())
		light_direction = light->getDirection();

	const RenderFrameStats stats{
	    .opaque_draw_count = opaque_draws.size(),
	    .masked_draw_count = masked_draws.size(),
	    .transparent_draw_count = transparent_draws.size(),
	    .shadow_draw_count = shadow_draws.size(),
	    .culled_draw_count = culled_draw_count,
	};
	return RenderFrame(
	    RenderView(view_desc),
	    makeDrawList(DrawListType::Opaque, opaque_draws, masked_draws),
	    makeDrawList(DrawListType::Transparent, transparent_draws),
	    makeDrawList(DrawListType::Shadow, shadow_draws),
	    scene.scene_layout.get(), scene.resources->getMaterialLayout(),
	    scene.resources->getObjectLayout(), scene.scene_binding_set.get(),
	    scene.scene_constant_buffer_view.get(), scene.light_buffer_view.get(),
	    light_direction, stats);
}

}        // namespace Vortex
