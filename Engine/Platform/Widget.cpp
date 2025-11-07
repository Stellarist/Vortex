#include "Widget.hpp"

#include <numeric>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include "Render/Graphics/Device.hpp"
#include "Core/Clock/Clock.hpp"
#include "Scene/Core/Node.hpp"
#include "Scene/Components/Camera.hpp"
#include "Scene/Components/Light.hpp"
#include "Scene/Components/Mesh.hpp"
#include "Scene/Resources/Material.hpp"
#include "Scene/Resources/Texture.hpp"
#include "Scene/Resources/SubMesh.hpp"

Widget::Widget(Window& window, Renderer& renderer) :
    window(&window)
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
	uint32_t max_sets = std::reduce(pool_size.begin(), pool_size.end(), 0u,
	    [](uint32_t sum, const vk::DescriptorPoolSize& size) {
		    return sum + size.descriptorCount;
	    });

	auto& context = renderer.getContext();
	descriptor_pool = std::make_unique<DescriptorPool>(
	    context,
	    max_sets,
	    pool_size,
	    vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);

	ImGui_ImplVulkan_InitInfo init_info{
	    .Instance = context.getInstance(),
	    .PhysicalDevice = context.getDevice().physical(),
	    .Device = context.getDevice().logical(),
	    .QueueFamily = context.getDevice().graphicsQueueIndex(),
	    .Queue = context.getDevice().graphicsQueue(),
	    .DescriptorPool = descriptor_pool->get(),
	    .RenderPass = renderer.getUIPass()->get(),
	    .MinImageCount = 3,
	    .ImageCount = 3,
	};

	if (!ImGui_ImplVulkan_Init(&init_info))
		throw std::runtime_error("Failed to initialize ImGui Vulkan backend");
}

void Widget::drawSceneGraph(const World* world, float dt)
{
	if (!world)
		return;

	auto* scene = world->getActiveScene();

	ImGui::Begin("Scene Graph");

	if (ImGui::TreeNodeEx("Scene Graph", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth)) {
		ImGui::Text("FPS: %.2f", Time::getFPS());
		ImGui::Text("Scene: %s", scene->getName().c_str());
		ImGui::Separator();

		drawSceneNodes(scene->getRoot());
		drawSceneComponents(scene);
		drawSceneResources(scene);

		ImGui::TreePop();
	}

	ImGui::End();
}

void Widget::drawSceneNodes(const Node* root)
{
	if (!root)
		return;

	auto* node = const_cast<Node*>(root);
	auto  node_title = std::format("[{}] {}", node->getType().name(), node->getName());

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (ImGui::TreeNodeEx((void*) (intptr_t) node->getUid(), flags, "%s", node_title.c_str())) {
		ImGui::Indent();

		if (node != node->getScene()->getRoot()) {
			auto& transform = node->getTransform();
			auto  translation = transform.getTranslation();
			auto  rotation = glm::degrees(glm::eulerAngles(transform.getRotation()));
			auto  scale = transform.getScaling();

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

			rotation = glm::vec3(normalize(rotation.x), normalize(rotation.y), normalize(rotation.z));
			if (rotation.x > 89.9f && rotation.x < 90.1f)
				rotation.x = 90.1f;
			else if (rotation.x < -89.9f && rotation.x > -90.1f)
				rotation.x = -90.1f;

			transform.setTranslation(translation);
			transform.setRotation(glm::quat(glm::radians(rotation)));
			transform.setScaling(scale);

			node->getTransform().invalidateWorldMatrix();

			if (node->hasComponent<Camera>()) {
				auto* camera = &node->getComponent<Camera>();
				if (auto* persp_camera = dynamic_cast<PerspectiveCamera*>(camera)) {
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

				if (auto* ortho_camera = dynamic_cast<OrthoCamera*>(camera)) {
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

			if (node->hasComponent<Light>()) {
				auto* light = &node->getComponent<Light>();
				auto  color = light->getColor();
				auto  intensity = light->getIntensity();

				ImGui::ColorEdit3("Color", &color.r);
				ImGui::DragFloat("Intensity", &intensity, 100.0f, 0.0f);

				light->setColor(color);
				light->setIntensity(intensity);

				if (auto* directional_light = dynamic_cast<DirectionalLight*>(light)) {
					auto direction = directional_light->getDirection();
					ImGui::DragFloat3("Direction", &direction.x, 0.001f, -1.0f, 1.0f);
					directional_light->setDirection(direction);
				}

				if (auto* point_light = dynamic_cast<PointLight*>(light)) {
					auto range = point_light->getRange();
					ImGui::DragFloat("Range", &range, 10.0f, 0.0f);
					point_light->setRange(range);
				}

				if (auto* spot_light = dynamic_cast<SpotLight*>(light)) {
					auto range = spot_light->getRange();
					auto inner_cone = glm::degrees(spot_light->getInnerConeAngle());
					auto outer_cone = glm::degrees(spot_light->getOuterConeAngle());

					ImGui::DragFloat("Range", &range, 10.0f, 0.0f);
					ImGui::DragFloat("Inner Cone Angle", &inner_cone, 0.1f, 0.0f, 90.0f);
					ImGui::DragFloat("Outer Cone Angle", &outer_cone, 0.1f, 0.0f, 90.0f);

					spot_light->setRange(range);
					spot_light->setInnerConeAngle(glm::radians(inner_cone));
					spot_light->setOuterConeAngle(glm::radians(outer_cone));
				}
			}
		}

		const auto& children = node->getChildren();
		for (const auto& child : children)
			drawSceneNodes(child);

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

		if (scene->hasComponent<Camera>() && ImGui::TreeNodeEx("Cameras", flags)) {
			ImGui::Indent();

			auto cameras = scene->getComponents<Camera>();
			for (const auto& camera : cameras) {
				auto camera_title = std::format("[{}] {}", camera->getType().name(), camera->getName());
				ImGui::Text("%s", camera_title.c_str());
			}

			ImGui::Unindent();
			ImGui::TreePop();
		}

		if (scene->hasComponent<Camera>() && ImGui::TreeNodeEx("Lights", flags)) {
			ImGui::Indent();

			auto lights = scene->getComponents<Light>();
			for (const auto& light : lights) {
				auto light_title = std::format("[{}] {}", light->getType().name(), light->getName());
				ImGui::Text("%s", light_title.c_str());
			}

			ImGui::Unindent();
			ImGui::TreePop();
		}

		if (scene->hasComponent<Camera>() && ImGui::TreeNodeEx("Meshes", flags)) {
			ImGui::Indent();

			auto meshes = scene->getComponents<Mesh>();
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

void Widget::drawSceneResources(const Scene* scene)
{
	if (!scene)
		return;

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
	if (ImGui::TreeNodeEx("Resources", flags)) {
		ImGui::Indent();

		if (scene->hasResource<Material>() && ImGui::TreeNodeEx("Materials", flags)) {
			ImGui::Indent();

			auto materials = scene->getResources<Material>();
			for (const auto& material : materials) {
				auto material_title = std::format("[{}] {}", material->getType().name(), material->getName());
				ImGui::Text("%s", material_title.c_str());
			}

			ImGui::Unindent();
			ImGui::TreePop();
		}

		if (scene->hasResource<Texture>() && ImGui::TreeNodeEx("Textures", flags)) {
			ImGui::Indent();

			auto textures = scene->getResources<Texture>();
			for (const auto& texture : textures) {
				auto texture_title = std::format("[{}] {}", texture->getType().name(), texture->getName());
				ImGui::Text("%s", texture_title.c_str());
			}

			ImGui::Unindent();
			ImGui::TreePop();
		}

		if (scene->hasResource<SubMesh>() && ImGui::TreeNodeEx("SubMeshes", flags)) {
			ImGui::Indent();

			auto submeshes = scene->getResources<SubMesh>();
			for (const auto& submesh : submeshes) {
				auto submesh_title = std::format("[{}] {}", submesh->getType().name(), submesh->getName());
				ImGui::Text("%s", submesh_title.c_str());
			}

			ImGui::Unindent();
			ImGui::TreePop();
		}

		ImGui::Unindent();
		ImGui::TreePop();
	}
}

Widget::~Widget()
{
	ImGui_ImplVulkan_Shutdown();
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

void Widget::drawFrame(CommandBuffer command)
{
	ImGui::Render();

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command.get());
}

bool Widget::pollEvent(const SDL_Event& event)
{
	return ImGui_ImplSDL3_ProcessEvent(&event);
}

void Widget::hook(std::function<void()> callback)
{
	draw_callbacks.push_back(callback);
}
