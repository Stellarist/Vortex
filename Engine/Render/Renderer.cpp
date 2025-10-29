#include "Renderer.hpp"

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Graphics/Command.hpp"
#include "Graphics/Sync.hpp"

Renderer::Renderer(Window& window)
{
	context = std::make_unique<Context>(window);
	swap_chain = std::make_unique<SwapChain>(window, *context);
	render_pass = std::make_unique<RenderPass>(*context, *swap_chain);

	frame.image_count = swap_chain->getImageCount();
	frame.images_in_flight.resize(frame.image_count, nullptr);
	for (uint32_t i = 0; i < Frame::MAX_FRAMES_IN_FLIGHT; ++i) {
		frame.commands[i] = context->getGraphicsCommandPool().allocate();
		frame.image_available_semaphores[i] = std::make_unique<Semaphore>(*context);
		frame.render_finished_semaphores[i] = std::make_unique<Semaphore>(*context);
		frame.in_flight_fences[i] = std::make_unique<Fence>(*context, true);
	}
}

Renderer::~Renderer()
{
	if (context)
		wait();
}

void Renderer::begin()
{
	frame.in_flight_fences.at(frame.current_frame)->wait();
	frame.image_index = swap_chain->acquireNextImage(
	    frame.image_available_semaphores.at(frame.current_frame)->get(), nullptr);

	if (frame.images_in_flight.at(frame.image_index))
		frame.images_in_flight.at(frame.image_index)->wait();
	frame.images_in_flight.at(frame.image_index) = frame.in_flight_fences.at(frame.current_frame).get();
	frame.in_flight_fences.at(frame.current_frame)->reset();

	auto& command = frame.commands.at(frame.current_frame);
	command.reset();
	CommandPool::begin(command);
	render_pass->begin(command, frame.image_index, swap_chain->getExtent(), {{0.0f, 0.0f, 0.0f, 1.0f}});

	command.setScissor(0, vk::Rect2D{}
	                          .setOffset({0, 0})
	                          .setExtent(swap_chain->getExtent()));
	command.setViewport(0, vk::Viewport{}
	                           .setX(0.0f)
	                           .setY(0.0f)
	                           .setWidth(static_cast<float>(swap_chain->getExtent().width))
	                           .setHeight(static_cast<float>(swap_chain->getExtent().height))
	                           .setMinDepth(0.0f)
	                           .setMaxDepth(1.0f));
}

void Renderer::end()
{
	auto  chain = swap_chain->get();
	auto  stage = vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput);
	auto& command = frame.commands.at(frame.current_frame);

	render_pass->end(command);
	CommandPool::end(command);

	auto wait_sema = frame.image_available_semaphores.at(frame.current_frame)->get();
	auto signal_sema = frame.render_finished_semaphores.at(frame.current_frame)->get();

	context->submit(command, {&wait_sema, 1},
	                {&signal_sema, 1},
	                {&stage, 1},
	                frame.in_flight_fences.at(frame.current_frame)->get());
	context->present({&frame.image_index, 1},
	                 {&chain, 1},
	                 {&signal_sema, 1});

	frame.current_frame = (frame.current_frame + 1) % Frame::MAX_FRAMES_IN_FLIGHT;
}

void Renderer::draw()
{
	auto& command = frame.commands.at(frame.current_frame);
	command.bindPipeline(vk::PipelineBindPoint::eGraphics, graphics_pipeline->get());

	if (render_scene)
		render_scene->draw(command, graphics_pipeline->getLayout());
}

void Renderer::call()
{
	for (auto& callback : render_callbacks)
		callback();
}

void Renderer::wait()
{
	context->getLogicalDevice().waitIdle();
}

void Renderer::hook(std::function<void()> callback)
{
	render_callbacks.push_back(std::move(callback));
}

void Renderer::tick(float dt)
{
	if (!active_world)
		return;

	if (render_scene)
		render_scene->update(dt);

	begin();
	draw();
	call();
	end();
}

World* Renderer::getActiveWorld() const
{
	return active_world;
}

void Renderer::setActiveWorld(World& world)
{
	active_world = &world;

	render_scene = std::make_unique<RenderScene>(*context, *active_world);

	std::vector<vk::DescriptorSetLayout> descriptor_layouts = {
	    render_scene->getSceneLayout()->get(),
	    render_scene->getMaterialLayout()->get(),
	    render_scene->getObjectLayout()->get(),
	};

	graphics_pipeline = std::make_unique<GraphicsPipeline>(
	    *context, *render_pass, descriptor_layouts);
}

Context& Renderer::getContext() const
{
	return *context;
}

SwapChain& Renderer::getSwapChain() const
{
	return *swap_chain;
}

RenderPass& Renderer::getRenderPass() const
{
	return *render_pass;
}

GraphicsPipeline& Renderer::getGraphicsPipeline() const
{
	return *graphics_pipeline;
}

RenderScene& Renderer::getRenderScene() const
{
	return *render_scene;
}

Frame& Renderer::getCurrentFrame() const
{
	return const_cast<Frame&>(frame);
}

vk::CommandBuffer Frame::getCurrentCommandBuffer() const
{
	return commands.at(current_frame);
}
