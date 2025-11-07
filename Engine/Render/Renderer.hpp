#pragma once

#include <functional>

#include <vulkan/vulkan.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "RHI/RenderScene.hpp"
#include "Graphics/Context.hpp"
#include "Graphics/Command.hpp"
#include "Graphics/Sync.hpp"
#include "Paths/ForwardPath.hpp"
#include "Paths/DeferredPath.hpp"
#include "Scene/World.hpp"

struct Frame {
	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	uint32_t image_count{};
	uint32_t image_index{};
	uint32_t current_frame{};

	std::array<CommandBuffer, MAX_FRAMES_IN_FLIGHT>              commands{};
	std::array<std::unique_ptr<Semaphore>, MAX_FRAMES_IN_FLIGHT> image_available_semaphores{};
	std::array<std::unique_ptr<Semaphore>, MAX_FRAMES_IN_FLIGHT> render_finished_semaphores{};
	std::array<std::unique_ptr<Fence>, MAX_FRAMES_IN_FLIGHT>     in_flight_fences{};
	std::vector<Fence*>                                          images_in_flight{};

	CommandBuffer currentCommand() const;
};

class Renderer {
	std::unique_ptr<Context> context;

	std::unique_ptr<ForwardPath>  forward_pipeline;
	std::unique_ptr<DeferredPath> deferred_pipeline;
	std::unique_ptr<RenderScene>  render_scene;

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

	Context&     getContext() const;
	RenderScene& getRenderScene() const;
	Frame&       getCurrentFrame() const;
	RenderPass*  getUIPass() const;
};
