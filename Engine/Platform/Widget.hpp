#pragma once

#include <functional>

#include <vulkan/vulkan.hpp>

#include "Window.hpp"
#include "Scene/World.hpp"
#include "Render/Renderer.hpp"

class Widget {
private:
	std::unique_ptr<DescriptorPool> descriptor_pool;

	std::vector<std::function<void()>> draw_callbacks{};

	Window* window{};

public:
	Widget(Window& window, Renderer& renderer);
	~Widget();

	void drawSceneGraph(const World* world, float dt);
	void drawSceneNodes(const Node* root);
	void drawSceneComponents(const Scene* scene);
	void drawSceneResources(const Scene* scene);

	void newFrame();
	void drawFrame(CommandBuffer command_buffer);

	bool pollEvent(const SDL_Event& event);
	void hook(std::function<void()> callback);
};
