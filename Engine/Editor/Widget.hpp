#pragma once

#include <functional>

#include <vulkan/vulkan.hpp>

#include "Window.hpp"
#include "Runtime/World/World.hpp"
#include "Runtime/Render/RHI/RHICommand.hpp"
#include "Runtime/Render/Renderer.hpp"

class Widget {
private:
	vk::DescriptorPool descriptor_pool{};
	vk::Device         device{};

	std::vector<std::function<void()>> draw_callbacks{};

	Renderer* renderer{};
	Window*   window{};

public:
	Widget(Window& window, Renderer& renderer);
	~Widget();

	void newFrame();
	void drawFrame(RHICommandList& command);

	bool pollEvent(const SDL_Event& event);
	void hook(std::function<void()> callback);

	void drawSceneGraph(const World* world, float dt);
	void drawSceneNodes(const Node* root);
	void drawSceneComponents(const Scene* scene);
	void drawSceneResources(const Scene* scene);
};
