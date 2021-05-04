#ifndef CZSS_HEADERS_H
#define CZSS_HEADERS_H

#include <stdint.h>
#include <unordered_map>
#include <unordered_set>
#include <typeinfo>
#include <string>

namespace czss
{
struct Controller;

struct SystemBase
{
	virtual void run() const;
	virtual uint64_t identity() const = 0;
	virtual Controller* ctrl() const;
	virtual std::string name() const;
};

template <typename S>
struct System : SystemBase
{
	uint64_t identity() const override;
	static uint64_t sidentity();
	Controller* ctrl() const override;

protected:
	System();
	System(Controller* loc);

private:
	Controller* contoller;
};

template <typename T>
struct Reader
{
protected:
	Reader<T>();
	const T* get() const;
};

template <typename T>
struct Writer
{
protected:
	Writer<T>();
	T* get() const;
};

template <typename T>
struct Dependency
{
protected:
	Dependency<T>();
};


// #############
// Controller helper structs
// #############

struct SystemData
{
	std::unordered_set<uint64_t> readsFrom;
	std::unordered_set<uint64_t> writesTo;
	std::unordered_set<uint64_t> readers;
	std::unordered_set<uint64_t> writers;
	std::unordered_set<uint64_t> dependencies;
	std::unordered_set<uint64_t> dependees;
};

struct RegistrationHelper
{
	SystemData* src;
	SystemData* tar;
};

struct Node
{
	std::unordered_set<uint64_t> dependencies;
	std::unordered_set<uint64_t> dependees;
	uint64_t weight = 1;
};

// #############
// Controller
// #############

struct Controller
{
	void run();

	SystemBase* get(uint64_t identifier);
	void registerReader(const SystemBase* sys, uint64_t target);
	void registerWriter(const SystemBase* sys, uint64_t target);
	void registerDependency(const SystemBase* sys, uint64_t target);
	void registerSystem(const SystemBase* sys);
private:
	bool changed = false;
	std::unordered_map<uint64_t, const SystemBase*> systems;
	std::unordered_map<uint64_t, SystemData> systemsData;
	std::unordered_map<uint64_t, Node> graph;
	std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>> implicits;

	void validate();
	void testAcyclic();
	void testComplete();
	void cullDependencies();
	void computeWeights();

	bool implicitlyDependsOn(uint64_t sys, uint64_t other);
	bool directlyDependsOn(uint64_t sys, uint64_t other);
	bool dependencyExists(uint64_t sys, uint64_t other);
	RegistrationHelper registerPair(const SystemBase* sys, uint64_t target);
	void reg(uint64_t key);
};

Controller* defaultController();

// #############
// Template implementations
// #############
template <typename S>
uint64_t System<S>::identity() const
{
	return (uint64_t)&typeid(S);
}

template <typename S>
System<S>::System(Controller* ctrl)
{
	contoller = ctrl;
	contoller->registerSystem(this);
}

template <typename S>
System<S>::System() : System(defaultController()) {}

template <typename S>
Controller* System<S>::ctrl () const
{
	return contoller;
}

template <typename S>
uint64_t System<S>::sidentity()
{
	return (uint64_t)&typeid(S);
}

template <typename T>
Reader<T>::Reader()
{
	const SystemBase* sys = reinterpret_cast<const SystemBase*>(this);
	sys->ctrl()->registerReader(sys, T::sidentity());
}

template <typename T>
const T* Reader<T>::get() const
{
	const SystemBase* sys = reinterpret_cast<const SystemBase*>(this);
	return reinterpret_cast<const T*>(sys->ctrl()->get(T::sidentity()));
}

template <typename T>
Writer<T>::Writer()
{
	const SystemBase* sys = reinterpret_cast<const SystemBase*>(this);
	sys->ctrl()->registerWriter(sys, T::sidentity());
}

template <typename T>
T* Writer<T>::get() const
{
	const SystemBase* sys = reinterpret_cast<const SystemBase*>(this);
	return reinterpret_cast<T*>(sys->ctrl()->get(T::sidentity()));
}

template <typename T>
Dependency<T>::Dependency()
{
	const SystemBase* sys = reinterpret_cast<const SystemBase*>(this);
	sys->ctrl()->registerDependency(sys, T::sidentity());
}

} // namespace czss

#endif // CZSS_HEADERS_H

#ifdef CZSS_IMPLEMENTATION
#ifndef CZSS_IMPLEMENTATION_GUARD_
#define CZSS_IMPLEMENTATION_GUARD_

#include <deque>
#include <queue>
#include <algorithm>
#include <vector>
#include <stdexcept>
#include <czsf.h>

namespace czss
{
static Controller DEFAULT_CONTROLLER = {};
Controller* defaultController() { return &DEFAULT_CONTROLLER; }

void SystemBase::run() const { }

Controller* SystemBase::ctrl () const
{
	return defaultController();
}

std::string SystemBase::name() const
{
	return "Unnamed System";
}

struct TaskData
{
	uint64_t id;
	uint64_t weight;
	const SystemBase* system = nullptr;
	const std::unordered_set<uint64_t>* dependencies;
	std::unordered_map<uint64_t, czsf::Barrier>* fences;
};

struct OrderedNode
{
	uint64_t key;
	uint64_t value;
};

void task(TaskData* data)
{
	for (auto dep : *data->dependencies)
	{
		data->fences->find(dep)->second.wait();
	}

	data->system->run();
	data->fences->find(data->id)->second.signal();
}

void Controller::run()
{
	if (changed)
	{
		validate();
		changed = false;
	}

	std::unordered_map<uint64_t, czsf::Barrier> fences;
	std::vector<TaskData> tasks;

	for (auto& pair : systemsData)
	{
		if (systems.find(pair.first) == systems.end()) continue;
		fences.insert({pair.first, czsf::Barrier(1)});
		auto& node = graph[pair.first];
		tasks.push_back({
			pair.first,
			node.weight,
			systems[pair.first],
			&node.dependencies,
			&fences
		});
	}

	sort(tasks.begin(), tasks.end(), [] (const TaskData & a, const TaskData & b) -> bool
	{ 
		return a.weight > b.weight; 
	});

	czsf::Barrier barrier(tasks.size());
	czsf::run(task, tasks.data(), tasks.size(), &barrier);
	barrier.wait();
}

bool Controller::implicitlyDependsOn(uint64_t sys, uint64_t other)
{
	auto map = implicits[sys];
	if (map.find(other) != map.end())
		return map[other];

	for (auto dep : systemsData[sys].dependencies)
	{
		if (directlyDependsOn(dep, other) || implicitlyDependsOn(dep, other))
		{
			map[other] = true;
			return true;
		}
	}

	map[other] = false;

	return false;
}

bool Controller::directlyDependsOn(uint64_t sys, uint64_t other)
{
	return systemsData[sys].dependencies.find(other)
		!= systemsData[sys].dependencies.end();
}

bool Controller::dependencyExists(uint64_t sys, uint64_t other)
{
	return sys == other
		|| directlyDependsOn(sys, other)
		|| directlyDependsOn(other, sys)
		|| implicitlyDependsOn(sys, other)
		|| implicitlyDependsOn(other, sys);
}

void Controller::testAcyclic()
{
	std::unordered_map<uint64_t, uint64_t> blockers;
	std::deque<uint64_t> queue;

	for (auto& pair : systemsData)
	{
		if (pair.second.dependencies.size() == 0)
		{
			queue.push_back(pair.first);
		}

		blockers.insert({pair.first, pair.second.dependencies.size()});
	}

	while(queue.size() > 0)
	{
		uint64_t key = queue.front();
		queue.pop_front();

		for (auto dependee : systemsData[key].dependees)
		{
			blockers[dependee]--;
			if (blockers[dependee] == 0)
			{
				queue.push_back(dependee);
			}

			if (blockers[dependee] < 0)
			{
				std::string errMsg = "[czss] Cyclical reference to "
					+ std::to_string(dependee);
				throw std::logic_error(errMsg);
			}
		}
	}
}

void Controller::testComplete()
{
	implicits.clear();
	for (auto& pair : systems)
		implicits.insert({pair.first, {}});

	for (auto& pair : systemsData)
	{
		for (auto component : pair.second.writesTo)
		{
			std::unordered_set<uint64_t>& readers = systemsData[component].readers;
			for (auto reader : readers)
			{
				if (!dependencyExists(pair.first, reader))
				{
					std::string errMsg = "[czss] No ordering between writer "
					+ std::to_string(pair.first) + " and reader " + std::to_string(reader)
					+ " accessing the same resource";
					throw std::logic_error(errMsg);
				}
			}

			std::unordered_set<uint64_t>& writers = systemsData[component].writers;
			for (auto writer : writers)
			{
				if (!dependencyExists(pair.first, writer))
				{
					std::string errMsg = "[czss] No ordering between writer "
					+ std::to_string(pair.first) + " and writer " + std::to_string(writer)
					+ " accessing the same resource";
					throw std::logic_error(errMsg);
				}
			}
		}
	}
}

void Controller::cullDependencies()
{
	graph.clear();
	for (auto& pair : systems)
		graph[pair.first] = {};

	for (auto& pair : systems)
	{
		auto& node = graph[pair.first];
		for (auto dependency : systemsData[pair.first].dependencies)
		{
			if (!implicitlyDependsOn(pair.first, dependency))
			{
				node.dependencies.insert(dependency);
				graph[dependency].dependees.insert(pair.first);
			}
		}
	}
}

void Controller::computeWeights()
{
	std::unordered_map<uint64_t, uint64_t> blockers;
	std::deque<uint64_t> queue;

	for (auto& pair : systems)
	{
		uint64_t numDeps = graph[pair.first].dependencies.size();
		blockers.insert({pair.first, numDeps});
		if (numDeps == 0)
			queue.push_back(pair.first);
	}

	while(queue.size() > 0)
	{
		uint64_t system = queue.front();
		queue.pop_front();

		for (auto dependency : graph[system].dependencies)
		{
			blockers[dependency]--;
			if (blockers[dependency] == 0)
			{
				queue.push_back(dependency);
			}
		}

		for (auto dependee : graph[system].dependees)
		{
			graph[system].weight += graph[dependee].weight;
		}
	}
}

void Controller::validate()
{
	testAcyclic();
	testComplete();
	cullDependencies();
	computeWeights();
}

void Controller::registerReader(const SystemBase* sys, uint64_t target)
{
	RegistrationHelper helper = registerPair(sys, target);
	helper.src->readsFrom.insert(target);
	helper.tar->readers.insert(sys->identity());
}

void Controller::registerWriter(const SystemBase* sys, uint64_t target)
{
	RegistrationHelper helper = registerPair(sys, target);
	helper.src->writesTo.insert(target);
	helper.tar->writers.insert(sys->identity());
}

void Controller::registerDependency(const SystemBase* sys, uint64_t target)
{
	RegistrationHelper helper = registerPair(sys, target);
	helper.src->dependencies.insert(target);
	helper.tar->dependees.insert(sys->identity());
}

RegistrationHelper Controller::registerPair(const SystemBase* sys, uint64_t target)
{
	changed = true;
	RegistrationHelper ret = {};
	reg(target);

	ret.src = &systemsData[sys->identity()];
	ret.tar = &systemsData[target];
	return ret;
}

void Controller::registerSystem(const SystemBase* sys)
{
	systems[sys->identity()] = sys;
}

void Controller::reg(uint64_t key)
{
	auto data = systemsData.find(key);
	if (data == systemsData.end())
	{
		systemsData.insert({key, {}});
	}
}

SystemBase* Controller::get(uint64_t identifier)
{
	auto res = systems.find(identifier);
	if (res == systems.end())
	{
		return nullptr;
	}

	return const_cast<SystemBase*>(res->second); // todo
}

} // namespace czss

#endif // CZSS_IMPLEMENTATION_GUARD_
#endif // CZSS_IMPLEMENTATION