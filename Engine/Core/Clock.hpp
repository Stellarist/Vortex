export module Core:Clock;

import std;
import :Types;

export namespace Vortex {

class Clock {
public:
	using TimePoint = std::chrono::steady_clock::time_point;
	using Duration = std::chrono::duration<float>;

private:
	TimePoint last_frame_time{std::chrono::steady_clock::now()};

	float delta_time{};
	float total_time{};

	uint64 frame_count{};

public:
	void tick()
	{
		const auto current_time = std::chrono::steady_clock::now();
		delta_time = Duration(current_time - last_frame_time).count();
		total_time += delta_time;
		++frame_count;
		last_frame_time = current_time;
	}

	void reset()
	{
		last_frame_time = std::chrono::steady_clock::now();
		delta_time = 0.0f;
		total_time = 0.0f;
		frame_count = 0;
	}

	float getDeltaTime() const { return delta_time; }
	float getTotalTime() const { return total_time; }
	uint64 getFrameCount() const { return frame_count; }
	float getFPS() const { return delta_time > 0.0f ? 1.0f / delta_time : 0.0f; }
};

}        // namespace Vortex
