#include "Renderer.hpp"

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Graphics/Device.hpp"
#include "Graphics/SwapChain.hpp"
#include "Paths/ForwardPath.hpp"
#include "Paths/DeferredPath.hpp"
#include "Core/File/PathResolver.hpp"
#include "Core/File/JsonParser.hpp"

Renderer::Renderer(Window& window)
{
	context = std::make_unique<Context>(window);

	frame.image_count = context->getSwapChain().getImageCount();
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
	auto fence = frame.in_flight_fences[frame.current_frame].get();
	fence->wait();

	auto& image = frame.image_index;
	auto  wait = frame.image_available_semaphores[frame.current_frame].get();
	image = context->getSwapChain().acquireNextImage(wait->get(), nullptr);

	auto& image_fence = frame.images_in_flight[image];
	if (image_fence)
		image_fence->wait();
	image_fence = fence;

	fence->reset();

	auto& command = frame.commands[frame.current_frame];
	command.get().reset();
	command.begin();
}

void Renderer::end()
{
	auto& command = frame.commands[frame.current_frame];
	command.end();

	auto image = frame.image_index;
	auto wait = frame.image_available_semaphores[frame.current_frame].get();
	auto signal = frame.render_finished_semaphores[frame.current_frame].get();
	auto fence = frame.in_flight_fences[frame.current_frame].get();
	auto stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	context->submit({command}, fence, {wait}, {signal}, {stage});
	context->present({image}, {signal});

	frame.current_frame = (frame.current_frame + 1) % Frame::MAX_FRAMES_IN_FLIGHT;
}

void Renderer::call()
{
	for (auto& callback : render_callbacks)
		callback();
}

void Renderer::wait()
{
	context->getDevice().logical().waitIdle();
}

void Renderer::draw()
{
	auto command = frame.commands[frame.current_frame].get();

	switch (type) {
	case PathType::Forward:
		forward_pipeline->beginForwardPass(command, frame.image_index, context->getSwapChain().getExtent());
		command.bindPipeline(vk::PipelineBindPoint::eGraphics, forward_pipeline->getForwardPipeline().get());
		render_scene->draw(command, forward_pipeline->getForwardPipeline().getLayout());
		call();
		forward_pipeline->endForwardPass(command);

		break;

	case PathType::Deferred:
		deferred_pipeline->beginGeometryPass(command, context->getSwapChain().getExtent());
		command.bindPipeline(vk::PipelineBindPoint::eGraphics, deferred_pipeline->getGeometryPipeline().get());
		render_scene->draw(command, deferred_pipeline->getGeometryPipeline().getLayout());
		deferred_pipeline->endGeometryPass(command);

		deferred_pipeline->beginLightingPass(command, frame.image_index, context->getSwapChain().getExtent());
		command.bindPipeline(vk::PipelineBindPoint::eGraphics, deferred_pipeline->getLightingPipeline().get());
		deferred_pipeline->bindDescriptor(command);
		command.draw(3, 1, 0, 0);
		call();
		deferred_pipeline->endLightingPass(command);

		break;

	default:
		throw std::runtime_error("Unknown render pipeline type");
	}
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

	auto descriptor_layouts = render_scene->getDescriptorSetLayouts();

	const auto& shaders_config = PathResolver::getConfigsDir() / "config.json";
	const auto& config_data = JsonParser::readJson(shaders_config);

	auto forward_path = PathResolver::getShadersDir() / config_data["forward_shader"].get<std::string>();
	auto geometry_path = PathResolver::getShadersDir() / config_data["deferred_geometry_shader"].get<std::string>();
	auto lighting_path = PathResolver::getShadersDir() / config_data["deferred_lighting_shader"].get<std::string>();

	auto forward_shader = std::make_shared<Shader>(*context, forward_path.string());
	auto geometry_shader = std::make_shared<Shader>(*context, geometry_path.string());
	auto lighting_shader = std::make_shared<Shader>(*context, lighting_path.string());

	forward_pipeline = std::make_unique<ForwardPath>();
	forward_pipeline->initialize(*context);
	forward_pipeline->build(descriptor_layouts, forward_shader->getStages());

	deferred_pipeline = std::make_unique<DeferredPath>();
	deferred_pipeline->initialize(*context);
	deferred_pipeline->build(descriptor_layouts, geometry_shader->getStages(), descriptor_layouts, lighting_shader->getStages());
}

Context& Renderer::getContext() const
{
	return *context;
}

RenderScene& Renderer::getRenderScene() const
{
	return *render_scene;
}

Frame& Renderer::getCurrentFrame() const
{
	return const_cast<Frame&>(frame);
}

RenderPass* Renderer::getUIPass() const
{
	if (type == PathType::Deferred)
		return &deferred_pipeline->getLightingPass().getPass();
	else
		return &forward_pipeline->getForwardPass().getPass();
}

CommandBuffer Frame::currentCommand() const
{
	return commands[current_frame];
}
