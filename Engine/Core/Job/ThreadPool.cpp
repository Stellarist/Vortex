#include "ThreadPool.hpp"

ThreadPool::ThreadPool(size_t thread_count) :
    stop(false)
{
	if (thread_count == 0)
		thread_count = 1;

	workers.reserve(thread_count);

	for (size_t i = 0; i < thread_count; ++i)
		workers.emplace_back([this, i] {
			workerThread();
		});
}

ThreadPool::~ThreadPool()
{
	shutdown();
}

void ThreadPool::workerThread()
{
	while (true) {
		std::function<void()> task;

		{
			std::unique_lock<std::mutex> lock(queue_mutex);

			condition.wait(lock, [this]() {
				return stop.load() || !tasks.empty();
			});

			if (stop.load() && tasks.empty())
				return;

			if (!tasks.empty()) {
				task = std::move(tasks.front());
				tasks.pop();
				active_threads.fetch_add(1);
			}

			if (task)
				task();

			{
				std::lock_guard<std::mutex> lock(queue_mutex);
				active_threads.fetch_sub(1);

				if (tasks.empty() && (active_threads.load() == 0))
					finish_condition.notify_all();
			}
		}
	}
}

void ThreadPool::wait()
{
	std::unique_lock<std::mutex> lock(queue_mutex);

	finish_condition.wait(lock, [this]() {
		return tasks.empty() && (active_threads.load(std::memory_order_acquire) == 0);
	});
}

void ThreadPool::shutdown()
{
	stop.store(true, std::memory_order_release);

	condition.notify_all();

	for (std::thread& worker : workers)
		if (worker.joinable())
			worker.join();

	workers.clear();
}

bool ThreadPool::isStopped() const
{
	return stop.load(std::memory_order_acquire);
}

size_t ThreadPool::threadCount() const
{
	return workers.size();
}

size_t ThreadPool::pendingTaskCount()
{
	std::lock_guard<std::mutex> lock(queue_mutex);
	return tasks.size();
}

size_t ThreadPool::activeThreadCount()
{
	return active_threads.load(std::memory_order_acquire);
}
