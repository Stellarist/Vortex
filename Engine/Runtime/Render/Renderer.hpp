#pragma once

#include <functional>

#include <vulkan/vulkan.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "Backend/Context.hpp"
#include "Backend/Command.hpp"
#include "Backend/Sync.hpp"
#include "Resources/GpuScene.hpp"
#include "Paths/ForwardPath.hpp"
#include "Paths/DeferredPath.hpp"
#include "Runtime/World/World.hpp"

struct Frame {
	static constexpr int FRAMES_IN_FLIGHT = 2;

	uint32_t image_count{};
	uint32_t image_index{};
	uint32_t current_frame{};

	std::vector<CommandBuffer>              commands{};
	std::vector<std::unique_ptr<Semaphore>> image_available_semaphores{};
	std::vector<std::unique_ptr<Semaphore>> render_finished_semaphores{};
	std::vector<std::unique_ptr<Fence>>     in_flight_fences{};

	CommandBuffer currentCommand() const;
};

class Renderer {
	std::unique_ptr<Context> context;

	std::unique_ptr<ForwardPath>  forward_pipeline;
	std::unique_ptr<DeferredPath> deferred_pipeline;
	std::unique_ptr<GpuScene>     render_scene;

	World* active_world{};

	Frame frame{};

	PathType type{PathType::Deferred};

	std::vector<std::function<void()>> render_callbacks;

public:
	Renderer(Window& window);
	~Renderer();

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	Renderer(Renderer&&) noexcept = default;
	Renderer& operator=(Renderer&&) noexcept = default;

	void begin();
	void end();
	void wait();
	void draw();
	void call();
	void hook(std::function<void()> callback);

	void tick(float dt);

	auto getActiveWorld() const -> World*;
	void setActiveWorld(World& world);

	Context&    getContext() const;
	GpuScene&   getGpuScene() const;
	Frame&      getCurrentFrame() const;
	RenderPass* getUIPass() const;
};
