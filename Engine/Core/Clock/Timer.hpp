#pragma once

#include <chrono>

class Timer {
private:
	using TimePoint = std::chrono::high_resolution_clock::time_point;

	TimePoint start_time;
	bool      running;

public:
	Timer();

	void start();
	void stop();
	void reset();

	float getElapsedSeconds() const;
	float getElapsedMilliseconds() const;

	bool isRunning() const;
};
