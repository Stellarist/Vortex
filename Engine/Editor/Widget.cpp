module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <SDL3/SDL.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_dx12.h>

#ifdef ERROR
#	undef ERROR
#endif

module Editor;

import vulkan;
import d3d12;
import Runtime.Vulkan;
import Runtime.D3D12;

namespace Vortex {

Widget::Widget(Window& window, Renderer& renderer) :
    window(&window), renderer(&renderer)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui::GetIO().FontGlobalScale = 1.75f;

	const bool d3d12 = renderer.getContext().getDevice().getAPI() == RHIAPI::D3D12;
	if (!(d3d12 ? ImGui_ImplSDL3_InitForD3D(window.get()) : ImGui_ImplSDL3_InitForVulkan(window.get()))) {
		ImGui::DestroyContext();
		ERROR("Failed to initialize ImGui SDL3 backend");
	}
	if (d3d12) {
		auto& native = static_cast<D3D12Device&>(renderer.getContext().getDevice());
		d3d12_descriptors = std::make_unique<D3D12DescriptorHeap>(native, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 256, true);
		ImGui_ImplDX12_InitInfo info{};
		info.Device = native.getHandle();
		info.CommandQueue = native.getQueue().getHandle();
		info.NumFramesInFlight = RHIMaxFramesInFlight;
		info.RTVFormat = toDXGIFormat(renderer.getContext().getFormat());
		info.SrvDescriptorHeap = d3d12_descriptors->getHandle();
		info.UserData = d3d12_descriptors.get();
		info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* cpu, D3D12_GPU_DESCRIPTOR_HANDLE* gpu) {
			auto& heap = *static_cast<D3D12DescriptorHeap*>(info->UserData);
			const auto index = heap.allocate();
			*cpu = heap.cpu(index);
			*gpu = heap.gpu(index);
		};
		info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE) {
			auto& heap = *static_cast<D3D12DescriptorHeap*>(info->UserData);
			const auto stride = info->Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			heap.release(static_cast<uint32>((cpu.ptr - heap.cpu(0).ptr) / stride));
		};
		CHECK(ImGui_ImplDX12_Init(&info), "Failed to initialize ImGui D3D12 backend");
		LOG("Initialized ImGui SDL3/D3D12 backends");
		return;
	}

	std::array pool_size{
	    vk::DescriptorPoolSize{
	        vk::DescriptorType::eSampler,
	        100},
	    vk::DescriptorPoolSize{
	        vk::DescriptorType::eSampledImage,
	        100},
	    vk::DescriptorPoolSize{
	        vk::DescriptorType::eCombinedImageSampler,
	        100},
	};

	uint32 max_sets = std::reduce(pool_size.begin(), pool_size.end(), 0u, [](uint32 sum, const vk::DescriptorPoolSize& size) {
		return sum + size.descriptorCount;
	});

	auto& context = dynamic_cast<VulkanContext&>(renderer.getContext());
	auto& vulkan_device = static_cast<VulkanDevice&>(context.getDevice());
	device = vulkan_device.getHandle();

	vk::DescriptorPoolCreateInfo pool_info{};
	pool_info.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
	    .setMaxSets(max_sets)
	    .setPoolSizes(pool_size);
	descriptor_pool = device.createDescriptorPool(pool_info);

	VkFormat color_format =
	    static_cast<VkFormat>(toVkFormat(context.getFormat()));
	VkPipelineRenderingCreateInfo rendering_info{
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
	    .colorAttachmentCount = 1,
	    .pColorAttachmentFormats = &color_format,
	};

	ImGui_ImplVulkan_InitInfo init_info{
	    .Instance = context.getInstance(),
	    .PhysicalDevice = context.getPhysicalDevice(),
	    .Device = device,
	    .QueueFamily = context.getQueueIndices().graphics_family.value(),
	    .Queue = vulkan_device.getQueue().getHandle(),
	    .DescriptorPool = descriptor_pool,
	    .MinImageCount = 3,
	    .ImageCount = 3,
	    .PipelineInfoMain = {
	        .PipelineRenderingCreateInfo = rendering_info,
	    },
	    .UseDynamicRendering = true,
	};

	CHECK(ImGui_ImplVulkan_Init(&init_info),
	    "Failed to initialize ImGui Vulkan backend");
	LOG("Initialized ImGui SDL3/Vulkan backends");
}

Widget::~Widget()
{
	try {
		renderer->waitIdle();
	} catch (const std::exception& error) {
		LOG(Error, "ImGui shutdown: {}", error.what());
	}
	if (d3d12_descriptors)
		ImGui_ImplDX12_Shutdown();
	else
		ImGui_ImplVulkan_Shutdown();
	if (device && descriptor_pool)
		device.destroyDescriptorPool(descriptor_pool);
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
	LOG(Debug, "Shut down ImGui backends");
}

void Widget::newFrame(float dt)
{
	sampleFrameTime(dt);
	if (d3d12_descriptors)
		ImGui_ImplDX12_NewFrame();
	else
		ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	for (const auto& callback : draw_callbacks)
		callback();
}

void Widget::drawFrame(RHICommandList& command)
{
	ImGui::Render();
	if (d3d12_descriptors) {
		auto& context = renderer->getContext();
		auto& native = static_cast<D3D12CommandList&>(command);
		const auto extent = context.getExtent();
		auto target = context.getDevice().createFramebuffer(RHIFramebufferDesc{}.setWidth(extent.width).setHeight(extent.height).addColorAttachment(&context.getBackbufferView()));
		auto& backbuffer = context.getBackbuffer();
		command.transitionTexture(&backbuffer, backbuffer.getState(), RenderTarget);
		command.beginRendering(target.get());
		ID3D12DescriptorHeap* heaps[] = {d3d12_descriptors->getHandle()};
		native.getCurrentCommand()->getHandle()->SetDescriptorHeaps(1, heaps);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), native.getCurrentCommand()->getHandle());
		command.clear();
		return;
	}

	auto* vk_command = dynamic_cast<VulkanCommandList*>(&command);
	if (!vk_command || !vk_command->getCurrentCommand())
		return;

	auto* backbuffer_view = dynamic_cast<VulkanTextureView*>(&renderer->getContext().getBackbufferView());
	if (!backbuffer_view)
		return;

	auto* backbuffer = dynamic_cast<VulkanTexture*>(&backbuffer_view->getTexture());
	if (!backbuffer)
		return;

	vk_command->transitionTexture(backbuffer, backbuffer->getState(), RenderTarget);

	vk::RenderingAttachmentInfo color_attachment{};
	color_attachment.setImageView(backbuffer_view->getHandle())
	    .setImageLayout(vk::ImageLayout::eAttachmentOptimal)
	    .setLoadOp(vk::AttachmentLoadOp::eLoad)
	    .setStoreOp(vk::AttachmentStoreOp::eStore);

	vk::RenderingInfo rendering_info{};
	rendering_info.setRenderArea(vk::Rect2D({0, 0}, {backbuffer->getDesc().width, backbuffer->getDesc().height}))
	    .setLayerCount(1)
	    .setColorAttachments(color_attachment);

	auto command_buffer = vk_command->getCurrentCommand()->getHandle();
	if (!command_buffer)
		return;

	command_buffer.beginRendering(rendering_info);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
	command_buffer.endRendering();
}

bool Widget::pollEvent(const SDL_Event& event)
{
	return ImGui_ImplSDL3_ProcessEvent(&event);
}

void Widget::hook(std::function<void()> callback)
{
	draw_callbacks.push_back(callback);
}

void Widget::sampleFrameTime(float dt)
{
	const auto& rendered = renderer->getStats();
	if (window->isMinimized() || !rendered.frame_rendered) {
		frame_timing = {};
		return;
	}
	if (rendered.frame_index == frame_timing.sampled_frame || !std::isfinite(dt) || dt <= 0.0f)
		return;
	frame_timing.sampled_frame = rendered.frame_index;

	auto& timing = frame_timing;
	timing.min_fps_elapsed += dt;
	timing.max_frame_ms = std::max(timing.max_frame_ms, dt * 1000.0f);
	const bool minimum_refreshed = timing.min_fps_elapsed >= 1.0f;
	if (minimum_refreshed) {
		timing.min_fps = 1000.0f / timing.max_frame_ms;
		timing.min_fps_elapsed = 0.0f;
		timing.max_frame_ms = 0.0f;
	}
	timing.milliseconds[timing.next] = dt * 1000.0f;
	timing.next = (timing.next + 1) % timing.milliseconds.size();
	timing.count = std::min(timing.count + 1, timing.milliseconds.size());
	timing.refresh_elapsed += dt;
	if (!minimum_refreshed && timing.refresh_elapsed < 0.25f && timing.count != 1 && timing.count != 100)
		return;
	timing.refresh_elapsed = 0.0f;

	const auto samples = std::span(timing.milliseconds).first(timing.count);
	const double total = std::accumulate(samples.begin(), samples.end(), 0.0);
	timing.average_fps = static_cast<float>(1000.0 * timing.count / total);
	if (timing.count >= 100) {
		auto sorted = timing.milliseconds;
		const size_t slow_count = (timing.count + 99) / 100;
		std::nth_element(sorted.begin(), sorted.begin() + slow_count,
		    sorted.begin() + timing.count, std::greater<float>{});
		const double slow_total = std::accumulate(sorted.begin(), sorted.begin() + slow_count, 0.0);
		timing.low_fps = static_cast<float>(1000.0 * slow_count / slow_total);
	}
	if (minimum_refreshed) {
		const auto* backend = renderer->getContext().getDevice().getAPI() == RHIAPI::D3D12 ? "d3d12" : "vulkan";
		const auto low_fps = timing.count >= 100 ? std::format("{:.2f}", timing.low_fps) : "collecting";
		LOG(Debug, "{} frame timing: average {:.2f} FPS, 1% low {} FPS, 1s min {:.2f} FPS (max {:.2f} ms)",
		    backend, timing.average_fps, low_fps, timing.min_fps, 1000.0f / timing.min_fps);
	}
}

void Widget::drawPerformance()
{
	if (!ImGui::TreeNodeEx("Performance", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth))
		return;

	const auto& timing = frame_timing;
	const float frame_ms = timing.count ? timing.milliseconds[(timing.next + timing.milliseconds.size() - 1) % timing.milliseconds.size()] : 0.0f;
	if (timing.count) {
		ImGui::Text("FPS: %.2f", 1000.0f / frame_ms);
		ImGui::Text("Average FPS: %.2f", timing.average_fps);
	} else {
		ImGui::TextUnformatted("FPS: collecting samples...");
		ImGui::TextUnformatted("Average FPS: collecting samples...");
	}
	if (timing.min_fps > 0.0f)
		ImGui::Text("Min FPS (1s): %.2f", timing.min_fps);
	else
		ImGui::TextUnformatted("Min FPS (1s): collecting samples...");
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("1000 / the longest frame interval in milliseconds.\nUpdated once per second; includes rendering and CPU/GPU waits.");
	if (timing.count >= 100)
		ImGui::Text("1%% Low FPS: %.2f", timing.low_fps);
	else
		ImGui::TextUnformatted("1% Low FPS: collecting samples...");
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("1000 / mean of the slowest 1%% frame times in milliseconds.\nRolling window: up to 1000 frames; refreshed every 0.25 s.\nMeasures frame intervals including waits, not GPU execution time.");

	if (timing.count)
		ImGui::Text("Frame time: %.2f ms", frame_ms);
	else
		ImGui::TextUnformatted("Frame time: collecting samples...");

	ImGui::TreePop();
}

void Widget::drawSceneGraph(const World* world)
{
	if (!world)
		return;

	const auto* title = renderer->getContext().getDevice().getAPI() == RHIAPI::D3D12 ?
	    "Inspector (d3d12)###Inspector" :
	    "Inspector (vulkan)###Inspector";
	if (ImGui::Begin(title)) {
		drawPerformance();
		if (renderer && ImGui::TreeNodeEx("Renderer", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth)) {
			auto& settings = renderer->getSettings();
			ImGui::TextUnformatted("Render Path: Deferred");
			ImGui::Checkbox("Frustum Culling", &settings.frustum_culling);
			ImGui::Checkbox("Directional Shadows", &settings.directional_shadows);
			const auto& stats = renderer->getStats();
			ImGui::Text("Frame: %llu%s", static_cast<unsigned long long>(stats.frame_index), stats.frame_rendered ? "" : " (skipped)");
			ImGui::Text("RDG Passes: %u", stats.rdg_pass_count);
			ImGui::Text("Draws: %u (opaque %u, masked %u, transparent %u)",
			    stats.draw_count,
			    stats.opaque_draw_count,
			    stats.masked_draw_count,
			    stats.transparent_draw_count);
			ImGui::Text("Shadow Casters: %u", stats.shadow_draw_count);
			ImGui::Text("Frustum Culled: %u", stats.culled_draw_count);
			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Scene", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth)) {
			ImGui::Text("World: %s", world->getName().c_str());
			for (auto* root_actor : world->getRootActors())
				drawSceneActors(root_actor);
			ImGui::TreePop();
		}
		drawAssets();
	}

	ImGui::End();
}

void Widget::drawSceneActors(const Actor* actor)
{
	if (!actor)
		return;

	auto* mutable_actor = const_cast<Actor*>(actor);
	auto actor_title = std::format("[{}] {}", mutable_actor->getType().name(), mutable_actor->getName());

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (ImGui::TreeNodeEx((void*) (intptr) mutable_actor->getUid(), flags, "%s", actor_title.c_str())) {
		ImGui::Indent();

		if (auto* root_component = mutable_actor->getRootComponent()) {
			auto& transform = root_component->getTransform();
			auto translation = transform.getTranslation();
			auto rotation = Math::degrees(Math::eulerAngles(transform.getRotation()));
			auto scale = transform.getScaling();

			if (ImGui::DragFloat3("Position", &translation.x, 0.01f))
				transform.setTranslation(translation);
			const bool rotation_changed = ImGui::DragFloat3("Rotation", &rotation.x, 0.5f);
			if (ImGui::DragFloat3("Scale", &scale.x, 0.01f))
				transform.setScaling(scale);

			auto normalize = [](float angle) {
				angle = std::fmod(angle, 360.0f);
				if (angle > 180.0f)
					angle -= 360.0f;
				else if (angle < -180.0f)
					angle += 360.0f;
				return angle;
			};

			rotation = Vec3(normalize(rotation.x), normalize(rotation.y), normalize(rotation.z));
			if (rotation.x > 89.9f && rotation.x < 90.1f)
				rotation.x = 90.1f;
			else if (rotation.x < -89.9f && rotation.x > -90.1f)
				rotation.x = -90.1f;

			if (rotation_changed)
				transform.setRotation(Math::fromEuler(Math::radians(rotation)));
		}

		if (mutable_actor->hasComponent<CameraComponent>()) {
			auto* camera = &mutable_actor->getComponent<CameraComponent>();
			if (auto* persp_camera = dynamic_cast<PerspectiveCameraComponent*>(camera)) {
				auto aspect_ratio = persp_camera->getAspectRatio();
				auto fov = persp_camera->getFov();
				auto near_plane = persp_camera->getNearPlane();
				auto far_plane = persp_camera->getFarPlane();

				ImGui::DragFloat("Aspect Ratio", &aspect_ratio, 0.01f, 0.1f, 10.0f);
				ImGui::DragFloat("FOV", &fov, 0.01f, 0.1f, 1.57f);
				ImGui::DragFloat("Near Plane", &near_plane, 0.01f, 0.01f, 1000.0f);
				ImGui::DragFloat("Far Plane", &far_plane, 0.1f, 1.0f, 10000.0f);

				persp_camera->setAspectRatio(aspect_ratio);
				persp_camera->setFov(fov);
				persp_camera->setNearPlane(near_plane);
				persp_camera->setFarPlane(far_plane);
			}

			if (auto* ortho_camera = dynamic_cast<OrthographicCameraComponent*>(camera)) {
				auto left = ortho_camera->getLeft();
				auto right = ortho_camera->getRight();
				auto top = ortho_camera->getTop();
				auto bottom = ortho_camera->getBottom();
				auto near_plane = ortho_camera->getNearPlane();
				auto far_plane = ortho_camera->getFarPlane();

				ImGui::DragFloat("Left", &left, 0.1f, -1000.0f, 1000.0f);
				ImGui::DragFloat("Right", &right, 0.1f, -1000.0f, 1000.0f);
				ImGui::DragFloat("Top", &top, 0.1f, -1000.0f, 1000.0f);
				ImGui::DragFloat("Bottom", &bottom, 0.1f, -1000.0f, 1000.0f);
				ImGui::DragFloat("Near Plane", &near_plane, 0.01f, 0.01f, 1000.0f);
				ImGui::DragFloat("Far Plane", &far_plane, 0.1f, 1.0f, 10000.0f);

				ortho_camera->setLeft(left);
				ortho_camera->setRight(right);
				ortho_camera->setTop(top);
				ortho_camera->setBottom(bottom);
				ortho_camera->setNearPlane(near_plane);
				ortho_camera->setFarPlane(far_plane);
			}
		}

		if (mutable_actor->hasComponent<LightComponent>()) {
			auto* light = &mutable_actor->getComponent<LightComponent>();
			auto color = light->getColor();
			auto intensity = light->getIntensity();

			ImGui::ColorEdit3("Color", &color.r);
			ImGui::DragFloat("Intensity", &intensity, 100.0f, 0.0f);

			light->setColor(color);
			light->setIntensity(intensity);

			if (auto* point_light = dynamic_cast<PointLightComponent*>(light)) {
				auto range = point_light->getRange();
				ImGui::DragFloat("Range", &range, 10.0f, 0.0f);
				point_light->setRange(range);
			}

			if (auto* spot_light = dynamic_cast<SpotLightComponent*>(light)) {
				auto range = spot_light->getRange();
				auto inner_cone = Math::degrees(spot_light->getInnerConeAngle());
				auto outer_cone = Math::degrees(spot_light->getOuterConeAngle());

				ImGui::DragFloat("Range", &range, 10.0f, 0.0f);
				ImGui::DragFloat("Inner Cone Angle", &inner_cone, 0.1f, 0.0f, 90.0f);
				ImGui::DragFloat("Outer Cone Angle", &outer_cone, 0.1f, 0.0f, 90.0f);

				spot_light->setRange(range);
				spot_light->setInnerConeAngle(Math::radians(inner_cone));
				spot_light->setOuterConeAngle(Math::radians(outer_cone));
			}
		}

		const auto& children = mutable_actor->getAttachedActors();
		for (const auto& child : children)
			drawSceneActors(child);

		ImGui::Unindent();
		ImGui::TreePop();
	}
}

void Widget::drawAssets()
{
	const auto& assets = AssetManager::instance();

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
	if (ImGui::TreeNodeEx("Assets", flags)) {
		ImGui::Indent();

		auto materials = assets.getLoadedAssets<MaterialAsset>();
		if (!materials.empty() && ImGui::TreeNodeEx("Materials", flags)) {
			ImGui::Indent();

			for (const auto& material : materials) {
				auto material_title = std::format("[{}] {}", material->getType().name(), material->getName());
				ImGui::Text("%s", material_title.c_str());
			}

			ImGui::Unindent();
			ImGui::TreePop();
		}

		auto textures = assets.getLoadedAssets<TextureAsset>();
		if (!textures.empty() && ImGui::TreeNodeEx("Textures", flags)) {
			ImGui::Indent();

			for (const auto& texture : textures) {
				auto texture_title = std::format("[{}] {}", texture->getType().name(), texture->getName());
				ImGui::Text("%s", texture_title.c_str());
			}

			ImGui::Unindent();
			ImGui::TreePop();
		}

		auto meshes = assets.getLoadedAssets<MeshAsset>();
		if (!meshes.empty() && ImGui::TreeNodeEx("Meshes", flags)) {
			ImGui::Indent();

			for (const auto& mesh : meshes) {
				auto mesh_title = std::format("[{}] {}", mesh->getType().name(), mesh->getName());
				ImGui::Text("%s", mesh_title.c_str());
			}

			ImGui::Unindent();
			ImGui::TreePop();
		}

		ImGui::Unindent();
		ImGui::TreePop();
	}
}
}        // namespace Vortex
