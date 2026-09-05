module;

#include <SDL3/SDL.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

module Editor;

import vulkan;

namespace Vortex {

Widget::Widget(Window& window, Renderer& renderer) :
    window(&window), renderer(&renderer)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui::GetIO().FontGlobalScale = 1.75f;

	if (!ImGui_ImplSDL3_InitForVulkan(window.get())) {
		ImGui::DestroyContext();
		throw std::runtime_error("Failed to initialize ImGui SDL3 backend");
	}

	std::array pool_size{
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

	if (!ImGui_ImplVulkan_Init(&init_info))
		throw std::runtime_error("Failed to initialize ImGui Vulkan backend");
}

Widget::~Widget()
{
	ImGui_ImplVulkan_Shutdown();
	if (device && descriptor_pool)
		device.destroyDescriptorPool(descriptor_pool);
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
}

void Widget::newFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	for (const auto& callback : draw_callbacks)
		callback();
}

void Widget::drawFrame(RHICommandList& command)
{
	ImGui::Render();

	auto* vk_command = dynamic_cast<VulkanCommandList*>(&command);
	if (!vk_command || !vk_command->getCurrentCommand())
		return;

	auto* backbuffer_view = dynamic_cast<VulkanTextureView*>(&renderer->getContext().getBackbufferView());
	if (!backbuffer_view)
		return;

	auto* backbuffer = dynamic_cast<VulkanTexture*>(&backbuffer_view->getTexture());
	if (!backbuffer)
		return;

	vk_command->transitionTexture(backbuffer, RenderTarget);

	vk::RenderingAttachmentInfo color_attachment{};
	color_attachment.setImageView(backbuffer_view->getHandle())
	    .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
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

void Widget::drawSceneGraph(const World* world, float dt)
{
	if (!world)
		return;

	auto* scene = world->getActiveScene();
	if (!scene)
		return;

	ImGui::Begin("Scene Graph");

	if (ImGui::TreeNodeEx("Scene Graph", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth)) {
		ImGui::Text("FPS: %.2f", Time::getFPS());
		ImGui::Text("Scene: %s", scene->getName().c_str());
		ImGui::Separator();

		for (auto* root_actor : scene->getRootActors())
			drawSceneActors(root_actor);
		drawSceneComponents(scene);
		drawAssets(world->getAssetManager());

		ImGui::TreePop();
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

			ImGui::DragFloat3("Position", &translation.x, 0.01f);
			ImGui::DragFloat3("Rotation", &rotation.x, 0.5f);
			ImGui::DragFloat3("Scale", &scale.x, 0.01f);

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

			transform.setTranslation(translation);
			transform.setRotation(Math::fromEuler(Math::radians(rotation)));
			transform.setScaling(scale);
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

void Widget::drawSceneComponents(const Scene* scene)
{
	if (!scene)
		return;

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
	if (ImGui::TreeNodeEx("Components", flags)) {
		ImGui::Indent();

		if (scene->hasComponent<CameraComponent>() && ImGui::TreeNodeEx("Camera Components", flags)) {
			ImGui::Indent();

			auto cameras = scene->getComponents<CameraComponent>();
			for (const auto& camera : cameras) {
				auto camera_title = std::format("[{}] {}", camera->getType().name(), camera->getName());
				ImGui::Text("%s", camera_title.c_str());
			}

			ImGui::Unindent();
			ImGui::TreePop();
		}

		if (scene->hasComponent<LightComponent>() && ImGui::TreeNodeEx("Light Components", flags)) {
			ImGui::Indent();

			auto lights = scene->getComponents<LightComponent>();
			for (const auto& light : lights) {
				auto light_title = std::format("[{}] {}", light->getType().name(), light->getName());
				ImGui::Text("%s", light_title.c_str());
			}

			ImGui::Unindent();
			ImGui::TreePop();
		}

		if (scene->hasComponent<MeshComponent>() && ImGui::TreeNodeEx("Mesh Components", flags)) {
			ImGui::Indent();

			auto meshes = scene->getComponents<MeshComponent>();
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

void Widget::drawAssets(const AssetManager* assets)
{
	if (!assets)
		return;

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
	if (ImGui::TreeNodeEx("Assets", flags)) {
		ImGui::Indent();

		auto materials = assets->getLoadedAssets<MaterialAsset>();
		if (!materials.empty() && ImGui::TreeNodeEx("Materials", flags)) {
			ImGui::Indent();

			for (const auto& material : materials) {
				auto material_title = std::format("[{}] {}", material->getType().name(), material->getName());
				ImGui::Text("%s", material_title.c_str());
			}

			ImGui::Unindent();
			ImGui::TreePop();
		}

		auto textures = assets->getLoadedAssets<TextureAsset>();
		if (!textures.empty() && ImGui::TreeNodeEx("Textures", flags)) {
			ImGui::Indent();

			for (const auto& texture : textures) {
				auto texture_title = std::format("[{}] {}", texture->getType().name(), texture->getName());
				ImGui::Text("%s", texture_title.c_str());
			}

			ImGui::Unindent();
			ImGui::TreePop();
		}

		auto meshes = assets->getLoadedAssets<MeshAsset>();
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
