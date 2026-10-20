module;

#include <SDL3/SDL.h>

export module Editor:Widget;

import vulkan;

import Editor.Window;
import Runtime.World;
import Runtime.Render;
import Runtime.D3D12;

export namespace Vortex {

class Widget {
private:
	struct FrameTiming {
		std::array<float, 1000> milliseconds{};
		size_t count{};
		size_t next{};
		uint64 sampled_frame{};
		float refresh_elapsed{};
		float average_fps{};
		float low_fps{};
		float min_fps_elapsed{};
		float max_frame_ms{};
		float min_fps{};
	};

	FrameTiming frame_timing{};

	vk::DescriptorPool descriptor_pool{};
	vk::Device device{};
	std::unique_ptr<D3D12DescriptorHeap> d3d12_descriptors;

	std::vector<std::function<void()>> draw_callbacks{};

	Renderer* renderer{};
	Window* window{};

	void sampleFrameTime(float dt);
	void drawPerformance();

public:
	Widget(Window& window, Renderer& renderer);
	~Widget();

	void newFrame(float dt);
	void drawFrame(RHICommandList& command);

	bool pollEvent(const SDL_Event& event);
	void hook(std::function<void()> callback);

	void drawSceneGraph(const World* world);
	void drawSceneActors(const Actor* actor);
	void drawAssets();
};

}        // namespace Vortex
