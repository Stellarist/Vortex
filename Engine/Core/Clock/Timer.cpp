#include "Timer.hpp"

Timer::Timer() :
    start_time(std::chrono::high_resolution_clock::now()),
    running(false)
{}

void Timer::start()
{
	start_time = std::chrono::high_resolution_clock::now();
	running = true;
}

void Timer::stop()
{
	running = false;
}

void Timer::reset()
{
	start_time = std::chrono::high_resolution_clock::now();
	running = false;
}

float Timer::getElapsedSeconds() const
{
	if (!running)
		return 0.0f;

	auto                         now = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> elapsed = now - start_time;

	return elapsed.count();
}

float Timer::getElapsedMilliseconds() const
{
	return getElapsedSeconds() * 1000.0f;
}

bool Timer::isRunning() const
{
	return running;
}
