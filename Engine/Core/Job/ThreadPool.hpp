#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <future>
#include <functional>
#include <condition_variable>

class ThreadPool {
private:
	std::vector<std::thread>          workers;
	std::queue<std::function<void()>> tasks;

	mutable std::mutex      queue_mutex;
	std::condition_variable condition;
	std::condition_variable finish_condition;

	std::atomic<bool>   stop{};
	std::atomic<size_t> active_threads{};

	void workerThread();

public:
	ThreadPool(size_t thread_count = std::thread::hardware_concurrency());
	~ThreadPool();

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

	ThreadPool(ThreadPool&&) = delete;
	ThreadPool& operator=(ThreadPool&&) = delete;

	template <typename F, typename... Args>
	auto submit(F&& func, Args&&... args) -> std::future<decltype(func(args...))>;

	template <typename F>
	void enqueue(F&& func);

	template <typename Rep, typename Period>
	bool waitFor(const std::chrono::duration<Rep, Period>& rel_time);

	void wait();
	void shutdown();

	bool isStopped() const;

	size_t threadCount() const;
	size_t pendingTaskCount();
	size_t activeThreadCount();
};

template <typename F, typename... Args>
auto ThreadPool::submit(F&& func, Args&&... args) -> std::future<decltype(func(args...))>
{
	auto task = std::make_shared<std::packaged_task<decltype(func(args...))>>(
	    std::bind(std::forward<F>(func), std::forward<Args>(args)...));

	auto result = task->get_future();

	{
		std::lock_guard<std::mutex> lock(queue_mutex);
		if (stop.load())
			throw std::runtime_error("Cannot submit task to stopped ThreadPool");

		tasks.emplace([task]() { (*task)(); });
	}

	condition.notify_one();

	return result;
}

template <typename F>
void ThreadPool::enqueue(F&& func)
{
	{
		std::lock_guard<std::mutex> lock(queue_mutex);
		if (stop.load())
			throw std::runtime_error("Cannot enqueue task to stopped ThreadPool");

		tasks.emplace(std::forward<F>(func));
	}

	condition.notify_one();
}

template <typename Rep, typename Period>
bool ThreadPool::waitFor(const std::chrono::duration<Rep, Period>& timeout)
{
	std::lock_guard<std::mutex> lock(queue_mutex);

	return finish_condition.wait_for(lock, timeout, [this]() {
		return tasks.empty() && (active_threads.load() == 0);
	});
}
