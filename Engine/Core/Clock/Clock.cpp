#include "Clock.hpp"

Clock::Clock() :
    start_time(std::chrono::high_resolution_clock::now()),
    last_frame_time(start_time),
    delta_time(0.0f),
    total_time(0.0f),
    frame_count(0)
{}

void Clock::tick()
{
	auto     current_time = std::chrono::high_resolution_clock::now();
	Duration dt = current_time - last_frame_time;

	delta_time = dt.count();
	total_time += delta_time;
	frame_count++;

	last_frame_time = current_time;
}

void Clock::reset()
{
	start_time = std::chrono::high_resolution_clock::now();
	last_frame_time = start_time;
	delta_time = 0.0f;
	total_time = 0.0f;
	frame_count = 0;
}

float Clock::getDeltaTime() const
{
	return delta_time;
}

float Clock::getTotalTime() const
{
	return total_time;
}

uint64_t Clock::getFrameCount() const
{
	return frame_count;
}

float Clock::getFPS() const
{
	return delta_time > 0.0f ? (1.0f / delta_time) : 0.0f;
}

Clock* Time::main_clock = nullptr;

void Time::setMainClock(Clock* clock)
{
	main_clock = clock;
}

float Time::getDeltaTime()
{
	return main_clock ? main_clock->getDeltaTime() : 0.0f;
}

float Time::getTotalTime()
{
	return main_clock ? main_clock->getTotalTime() : 0.0f;
}

uint64_t Time::getFrameCount()
{
	return main_clock ? main_clock->getFrameCount() : 0;
}

float Time::getFPS()
{
	return main_clock ? main_clock->getFPS() : 0.0f;
}
