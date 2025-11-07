#pragma once

#include <chrono>

class Clock {
public:
	using TimePoint = std::chrono::high_resolution_clock::time_point;
	using Duration = std::chrono::duration<float>;

private:
	TimePoint start_time;
	TimePoint last_frame_time;

	float delta_time;
	float total_time;

	uint64_t frame_count;

public:
	Clock();

	void tick();
	void reset();

	float    getDeltaTime() const;
	float    getTotalTime() const;
	uint64_t getFrameCount() const;
	float    getFPS() const;
};

class Time {
private:
	static Clock* main_clock;

public:
	static void setMainClock(Clock* clock);

	static float    getDeltaTime();
	static float    getTotalTime();
	static uint64_t getFrameCount();
	static float    getFPS();
};
