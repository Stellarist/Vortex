#include "Job.hpp"

#include <thread>

Job::Job(JobFunc func) :
    function(std::move(func))
{}

void Job::execute()
{
	if (function)
		function();

	completed.store(true);

	complete();
}

void Job::complete()
{
	for (auto& weak_dependent : dependents)
		if (auto dependent = weak_dependent.lock())
			dependent->onDependencyCompleted();
}

void Job::wait()
{
	while (!isCompleted())
		std::this_thread::yield();
}

void Job::dependsOn(std::shared_ptr<Job> dependency)
{
	if (!dependency)
		return;

	unfinished_dependencies.fetch_add(1);
	dependency->addDependent(shared_from_this());
}

void Job::onDependencyCompleted()
{
	int remaining = unfinished_dependencies.fetch_sub(1);
}

bool Job::isReady() const
{
	return unfinished_dependencies.load() == 0;
}

bool Job::isCompleted() const
{
	return completed.load();
}
