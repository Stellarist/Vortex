// test_threadpool.cpp
#include "ThreadPool.hpp"
#include <iostream>
#include <atomic>

int main()
{
	ThreadPool pool(4);

	std::atomic<int> counter{0};

	// 提交1000个任务
	for (int i = 0; i < 1000; ++i) {
		pool.enqueue([&counter]() {
			counter.fetch_add(1);
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		});
	}

	pool.wait();

	std::cout << "Counter: " << counter.load() << std::endl;

	if (counter.load() != 1000) {
		std::cerr << "❌ FAILED: Expected 1000, got " << counter.load() << std::endl;
		return 1;
	}

	std::cout << "✅ PASSED" << std::endl;
	return 0;
}