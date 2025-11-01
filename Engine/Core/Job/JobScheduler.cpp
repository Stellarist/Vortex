#include "JobScheduler.hpp"

JobScheduler& JobScheduler::instance()
{
	static JobScheduler instance;
	return instance;
}

void JobScheduler::initialize(size_t num_threads)
{
	if (thread_pool)
		throw std::runtime_error("JobScheduler is already initialized");

	if (num_threads == 0)
		num_threads = std::thread::hardware_concurrency() - 1;

	thread_pool = std::make_unique<ThreadPool>(num_threads);
	stop.store(false);
	scheduler = std::thread([this]() { scheduleLoop(); });
}

void JobScheduler::shutdown()
{
	if (!thread_pool)
		return;

	stop.store(true);
	condition.notify_all();

	if (scheduler.joinable())
		scheduler.join();

	thread_pool->wait();
	thread_pool.reset();
}

void JobScheduler::scheduleLoop()
{
	while (true) {
		std::unique_lock<std::mutex> lock(mutex);

		condition.wait(lock, [this]() {
			return stop.load() || !pending.empty();
		});

		if (stop.load())
			break;

		auto it = pending.begin();
		while (it != pending.end()) {
			if ((*it)->isReady()) {
				auto job = *it;
				it = pending.erase(it);

				thread_pool->enqueue([job]() {
					job->execute();
				});
			} else if ((*it)->isCompleted()) {
				it = pending.erase(it);
			} else {
				it++;
			}
		}
	}
}

void JobScheduler::submit(JobHandle job)
{
	if (!job)
		return;

	if (!thread_pool)
		throw std::runtime_error("JobScheduler is not initialized");

	if (job->isReady())
		thread_pool->enqueue([job]() {
			job->execute();
		});
	else {
		{
			std::lock_guard<std::mutex> lock(mutex);
			pending.push_back(job);
		}

		condition.notify_one();
	}
}

void JobScheduler::wait(JobHandle job)
{
	if (job)
		job->wait();
}

void JobScheduler::waitAll()
{
	if (thread_pool)
		thread_pool->wait();
}
