module Core.Clock;

namespace Vortex {

Clock* Time::main_clock = nullptr;

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

uint64 Clock::getFrameCount() const
{
	return frame_count;
}

float Clock::getFPS() const
{
	return delta_time > 0.0f ? (1.0f / delta_time) : 0.0f;
}


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

uint64 Time::getFrameCount()
{
	return main_clock ? main_clock->getFrameCount() : 0;
}

float Time::getFPS()
{
	return main_clock ? main_clock->getFPS() : 0.0f;
}


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

	auto now = std::chrono::high_resolution_clock::now();

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

}        // namespace Vortex
