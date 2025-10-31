#include "Widget.hpp"

#include <numeric>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

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
	    .PhysicalDevice = context.getPhysicalDevice(),
	    .Device = context.getLogicalDevice(),
	    .QueueFamily = context.getGraphicsQueueIndex(),
	    .Queue = context.getGraphicsQueue(),
	    .DescriptorPool = descriptor_pool->get(),
	    .RenderPass = renderer.getRenderPass().get(),
	    .MinImageCount = 3,
	    .ImageCount = 3,
	};

	if (!ImGui_ImplVulkan_Init(&init_info))
		throw std::runtime_error("Failed to initialize ImGui Vulkan backend");
}

void Widget::drawSceneGraph(const World* world)
{
	if (!world)
		return;

	auto* scene = world->getActiveScene();

	ImGui::Begin("Scene Graph");

	if (ImGui::TreeNodeEx("Scene Graph", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth)) {
		ImGui::Text("Scene: %s", scene->getName().c_str());
		ImGui::Separator();

		drawSceneNodes(&scene->getRoot());
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
	auto  node_title = std::format("[{}] {} (ID: {})", node->getType().name(), node->getName(), node->getId());

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (ImGui::TreeNodeEx((void*) (intptr_t) node->getId(), flags, "%s", node_title.c_str())) {
		ImGui::Indent();

		if (node != &node->getScene()->getRoot()) {
			auto& transform = node->getTransform();
			auto* pos = const_cast<glm::vec3*>(&transform.getTranslation());
			auto* rot = const_cast<glm::quat*>(&transform.getRotation());
			auto* scale = const_cast<glm::vec3*>(&transform.getScaling());

			if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth)) {
				ImGui::DragFloat3("Position", &pos->x, 0.01f);
				ImGui::DragFloat4("Rotation", &rot->x, 0.01f);
				ImGui::DragFloat3("Scale", &scale->x, 0.01f);
				ImGui::TreePop();
			}

			node->getTransform().invalidateWorldMatrix();
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

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (ImGui::TreeNodeEx("Components", flags)) {
		ImGui::Indent();

		if (scene->hasComponent<Camera>()) {
			auto cameras = scene->getComponents<Camera>();
			for (const auto& camera : cameras) {
				auto camera_title = std::format("[{}] {}:{}", camera->getType().name(), camera->getName(), camera->getNode()->getId());
				ImGui::Text("%s", camera_title.c_str());
			}

			auto lights = scene->getComponents<Light>();
			for (const auto& light : lights) {
				auto light_title = std::format("[{}] {}:{}", light->getType().name(), light->getName(), light->getNode()->getId());
				ImGui::Text("%s", light_title.c_str());
			}

			auto meshes = scene->getComponents<Mesh>();
			for (const auto& mesh : meshes) {
				auto mesh_title = std::format("[{}] {}:{}", mesh->getType().name(), mesh->getName(), mesh->getNode()->getId());
				ImGui::Text("%s", mesh_title.c_str());
			}
		}

		ImGui::Unindent();
		ImGui::TreePop();
	}
}

void Widget::drawSceneResources(const Scene* scene)
{
	if (!scene)
		return;

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (ImGui::TreeNodeEx("Resources", flags)) {
		ImGui::Indent();

		if (scene->hasResource<Material>()) {
			auto materials = scene->getResources<Material>();
			for (const auto& material : materials) {
				auto material_title = std::format("[{}] {}", material->getType().name(), material->getName());
				ImGui::Text("%s", material_title.c_str());
			}
		}

		if (scene->hasResource<Texture>()) {
			auto textures = scene->getResources<Texture>();
			for (const auto& texture : textures) {
				auto texture_title = std::format("[{}] {}", texture->getType().name(), texture->getName());
				ImGui::Text("%s", texture_title.c_str());
			}
		}

		if (scene->hasResource<SubMesh>()) {
			auto submeshes = scene->getResources<SubMesh>();
			for (const auto& submesh : submeshes) {
				auto submesh_title = std::format("[{}] {}", submesh->getType().name(), submesh->getName());
				ImGui::Text("%s", submesh_title.c_str());
			}
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

void Widget::drawFrame(vk::CommandBuffer command_buffer)
{
	ImGui::Render();

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
}

bool Widget::pollEvent(const SDL_Event& event)
{
	return ImGui_ImplSDL3_ProcessEvent(&event);
}

void Widget::hook(std::function<void()> callback)
{
	draw_callbacks.push_back(callback);
}
