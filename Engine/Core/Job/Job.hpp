#pragma once

#include <functional>
#include <atomic>
#include <memory>

using JobFunc = std::function<void()>;

class Job : public std::enable_shared_from_this<Job> {
private:
	JobFunc function;

	std::atomic<bool>    completed{};
	std::atomic<int32_t> unfinished_dependencies{};

	std::vector<std::weak_ptr<Job>> dependents;

	void addDependent(std::shared_ptr<Job> dependent);

public:
	Job(JobFunc func);

	void execute();
	void complete();
	void wait();

	void dependsOn(std::shared_ptr<Job> other);
	void onDependencyCompleted();

	bool isReady() const;
	bool isCompleted() const;
};
