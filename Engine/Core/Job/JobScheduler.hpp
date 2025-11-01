#pragma once

#include "Job.hpp"
#include "ThreadPool.hpp"

#include <memory>

using JobHandle = std::shared_ptr<Job>;

class JobScheduler {
private:
	std::unique_ptr<ThreadPool> thread_pool;

	std::thread             scheduler;
	std::mutex              mutex;
	std::condition_variable condition;

	std::vector<JobHandle> pending;

	std::atomic<bool> stop{};

	JobScheduler() = default;

	void scheduleLoop();

public:
	JobScheduler(const JobScheduler&) = delete;
	JobScheduler& operator=(const JobScheduler&) = delete;

	JobScheduler(JobScheduler&&) = delete;
	JobScheduler& operator=(JobScheduler&&) = delete;

	static JobScheduler& instance();

	void initialize(size_t num_threads = 0);
	void shutdown();

	template <typename F, typename... Args>
	JobHandle schedule(F&& func, Args&&... args);

	template <typename F>
	JobHandle schedule(F&& func);

	void submit(JobHandle job);
	void wait(JobHandle job);
	void waitAll();
};

template <typename F, typename... Args>
JobHandle JobScheduler::schedule(F&& func, Args&&... args)
{
	if (!thread_pool)
		throw std::runtime_error("JobScheduler is not initialized");

	auto bound_task = std::bind(std::forward<F>(func), std::forward<Args>(args)...);
	auto job = std::make_shared<Job>(bound_task);

	thread_pool->enqueue([job]() {
		job->execute();
	});

	return job;
}

template <typename F>
JobHandle JobScheduler::schedule(F&& func)
{
	if (!thread_pool)
		throw std::runtime_error("JobScheduler is not initialized");

	auto job = std::make_shared<Job>(std::forward<F>(func));

	thread_pool->enqueue([job]() {
		job->execute();
	});

	return job;
}
