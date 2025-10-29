#pragma once

#include <functional>

#include <vulkan/vulkan.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "RHI/RenderScene.hpp"
#include "Graphics/Context.hpp"
#include "Graphics/Descriptor.hpp"
#include "Graphics/SwapChain.hpp"
#include "Graphics/RenderPass.hpp"
#include "Graphics/GraphicsPipeline.hpp"
#include "Graphics/Sync.hpp"
#include "Scene/World.hpp"

struct Frame {
	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	uint32_t image_count{};
	uint32_t image_index{};
	uint32_t current_frame{};

	const DescriptorSet*                 set{};
	std::unique_ptr<DescriptorPool>      pool{};
	std::unique_ptr<DescriptorSetLayout> layout{};

	std::array<vk::CommandBuffer, MAX_FRAMES_IN_FLIGHT>          commands{};
	std::array<std::unique_ptr<Semaphore>, MAX_FRAMES_IN_FLIGHT> image_available_semaphores{};
	std::array<std::unique_ptr<Semaphore>, MAX_FRAMES_IN_FLIGHT> render_finished_semaphores{};
	std::array<std::unique_ptr<Fence>, MAX_FRAMES_IN_FLIGHT>     in_flight_fences{};
	std::vector<Fence*>                                          images_in_flight{};

	vk::CommandBuffer getCurrentCommandBuffer() const;
};

class Renderer {
	std::unique_ptr<Context>          context;
	std::unique_ptr<SwapChain>        swap_chain;
	std::unique_ptr<RenderPass>       render_pass;
	std::unique_ptr<GraphicsPipeline> graphics_pipeline;
	std::unique_ptr<RenderScene>      render_scene;

	World* active_world{};

	Frame frame;

	std::vector<std::function<void()>> render_callbacks;

public:
	Renderer(Window& window);

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	Renderer(Renderer&&) noexcept = default;
	Renderer& operator=(Renderer&&) noexcept = default;

	~Renderer();

	void begin();
	void end();
	void wait();
	void draw();
	void call();
	void hook(std::function<void()> callback);

	void tick(float dt);

	auto getActiveWorld() const -> World*;
	void setActiveWorld(World& world);

	Context&          getContext() const;
	SwapChain&        getSwapChain() const;
	RenderPass&       getRenderPass() const;
	GraphicsPipeline& getGraphicsPipeline() const;
	RenderScene&      getRenderScene() const;
	Frame&            getCurrentFrame() const;
};
