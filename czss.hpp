#ifndef CZSS_HEADERS_H
#define CZSS_HEADERS_H

#include <stdint.h>
#include <unordered_map>
#include <unordered_set>
#include <typeinfo>
#include <string>
#include <vector>
#include <czsf.h>

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

	friend S;
private:
	System();
	System(Controller* loc);
	Controller* contoller;
};

struct ComponentBase
{
	virtual uint64_t identity() const = 0;
	virtual std::string name() const;
};

template <typename C>
struct Component : ComponentBase
{
	uint64_t identity() const override;
	static uint64_t sidentity();

	friend C;
private:
	Component();
	Component(Controller* loc);
};

template <typename T, typename ...U>
struct Reader : Reader<T>, Reader<U...> { };

template <typename T>
struct Reader<T>
{
protected:
	Reader<T>();
	const T* get() const;
};

template <typename T, typename ...U>
struct Writer : Writer<T>, Writer<U...> { };

template <typename T>
struct Writer<T>
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
	std::unordered_set<uint64_t> dependencies;
	std::unordered_set<uint64_t> dependees;
};

struct ComponentData
{
	std::unordered_set<uint64_t> readers;
	std::unordered_set<uint64_t> writers;
};

struct Node
{
	std::unordered_set<uint64_t> dependencies;
	std::unordered_set<uint64_t> dependees;
	uint64_t weight = 1;
};

struct TaskData
{
	uint64_t id;
	uint64_t weight;
	const SystemBase* system = nullptr;
	const std::unordered_set<uint64_t>* dependencies;
	std::unordered_map<uint64_t, czsf::Barrier*>* fences;
	czsf::Barrier* signal;
};

// #############
// Controller
// #############

struct Controller
{
	void run();
	ComponentBase* get(uint64_t identifier);

	template<typename T>
	void updateWeight(uint64_t value);

	void registerReader(const SystemBase* sys, uint64_t target);
	void registerWriter(const SystemBase* sys, uint64_t target);
	void registerDependency(const SystemBase* sys, uint64_t target);
	void registerSystem(const SystemBase* system);
	void registerComponent(ComponentBase* component);
private:
	std::unordered_map<uint64_t, ComponentBase*> components;
	std::unordered_map<uint64_t, ComponentData> componentsData;

	std::unordered_map<uint64_t, const SystemBase*> systems;
	std::unordered_map<uint64_t, SystemData> systemsData;

	std::unordered_map<uint64_t, Node> graph;
	std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>> implicits;

	std::vector<czsf::Barrier> barriers;
	std::unordered_map<uint64_t, czsf::Barrier*> barrierMap;
	std::vector<TaskData> tasks;

	bool changed = false;
	bool weightsChanged = false;

	void validate();
	void testAcyclic();
	void testComplete();
	void cullDependencies();
	void computeWeights();

	void generateTasks();
	void prioritize();

	bool implicitlyDependsOn(uint64_t sys, uint64_t other);
	bool directlyDependsOn(uint64_t sys, uint64_t other);
	bool dependencyExists(uint64_t sys, uint64_t other);
};

Controller* defaultController();

// #############
// Template implementations
// #############

template <typename S>
System<S>::System(Controller* ctrl)
{
	static_assert(!std::is_base_of<ComponentBase, S>::value, "[czss] A System cannot also be a Component");
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
uint64_t System<S>::identity() const
{
	return (uint64_t)&typeid(S);
}

template <typename S>
uint64_t System<S>::sidentity()
{
	return (uint64_t)&typeid(S);
}

template <typename C>
uint64_t Component<C>::identity() const
{
	return (uint64_t)&typeid(C);
}

template <typename C>
uint64_t Component<C>::sidentity()
{
	return (uint64_t)&typeid(C);
}

template <typename C>
Component<C>::Component(Controller* ctrl)
{
	static_assert(!std::is_base_of<SystemBase, C>::value, "[czss] A Component cannot also be a System");
	ctrl->registerComponent(this);
}

template <typename C>
Component<C>::Component() : Component(defaultController()) {}

template <typename T>
Reader<T>::Reader()
{
	static_assert(std::is_base_of<ComponentBase, T>::value, "[czss] Reader<T>: T must be a Component<U>");
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
	static_assert(std::is_base_of<ComponentBase, T>::value, "[czss] Writer<T>: T must be a Component<U>");
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
	static_assert(std::is_base_of<SystemBase, T>::value, "[czss] Dependency<T>: T must be a System<U>");
	const SystemBase* sys = reinterpret_cast<const SystemBase*>(this);
	sys->ctrl()->registerDependency(sys, T::sidentity());
}

template <typename S>
void Controller::updateWeight(uint64_t value)
{
	graph[S::sidentity()].weight = value;
	weightsChanged = true;
}

} // namespace czss

#endif // CZSS_HEADERS_H

#ifdef CZSS_IMPLEMENTATION
#ifndef CZSS_IMPLEMENTATION_GUARD_
#define CZSS_IMPLEMENTATION_GUARD_

#include <deque>
#include <algorithm>
#include <stdexcept>

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

std::string ComponentBase::name() const
{
	return "Unnamed Component";
}

void task(TaskData* data)
{
	for (auto dep : *data->dependencies)
	{
		data->fences->find(dep)->second->wait();
	}

	data->system->run();
	data->fences->find(data->id)->second->signal();
}

void Controller::generateTasks()
{
	barrierMap.clear();
	barriers.clear();
	barriers.reserve(systems.size());
	tasks.clear();
	tasks.reserve(systems.size());

	uint64_t counter = 0;

	for (auto& pair : systems)
	{
		barriers.push_back(czsf::Barrier(1));

		barrierMap.insert({pair.first, &barriers[counter]});
		auto& node = graph[pair.first];
		tasks.push_back({
			pair.first,
			node.weight,
			systems[pair.first],
			&node.dependencies,
			&barrierMap,
			&barriers[counter]
		});

		counter++;
	}
}

void Controller::prioritize()
{
	for (auto& task : tasks)
	{
		task.weight = graph[task.id].weight;
	}

	sort(tasks.begin(), tasks.end(), [] (const TaskData & a, const TaskData & b) -> bool
	{
		return a.weight > b.weight;
	});
}

void Controller::run()
{
	if (changed)
	{
		validate();
		generateTasks();
		changed = false;
	}

	if (weightsChanged)
	{
		prioritize();
		weightsChanged = false;
	}

	for (uint64_t i = 0; i < barriers.size(); i++)
	{
		barriers[i] = czsf::Barrier(1);
	}

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
					+ systems[dependee]->name()
					+ " (" +std::to_string(dependee) + ")";
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
			std::unordered_set<uint64_t>& readers = componentsData[component].readers;
			for (auto reader : readers)
			{
				if (!dependencyExists(pair.first, reader))
				{
					std::string errMsg = "[czss] No ordering between writer "
					+ systems[pair.first]->name() + " (" + std::to_string(pair.first)
					+ ") and reader "
					+ systems[reader]->name() + " ("+ std::to_string(reader)
					+ ") accessing the same resource";
					throw std::logic_error(errMsg);
				}
			}

			std::unordered_set<uint64_t>& writers = componentsData[component].writers;
			for (auto writer : writers)
			{
				if (!dependencyExists(pair.first, writer))
				{
					std::string errMsg = "[czss] No ordering between writer "
					+ systems[pair.first]->name() + " (" + std::to_string(pair.first)
					+ ") and writer "
					+ systems[writer]->name() + " ("+ std::to_string(writer)
					+ ") accessing the same resource";
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

		graph[system].dependees.clear();
	}
}

void Controller::validate()
{
	testAcyclic();
	testComplete();
	cullDependencies();
	computeWeights();

	implicits.clear();
}

void Controller::registerReader(const SystemBase* sys, uint64_t target)
{
	changed = true;
	weightsChanged = true;
	systemsData[sys->identity()].readsFrom.insert(target);
	componentsData[target].readers.insert(sys->identity());
}

void Controller::registerWriter(const SystemBase* sys, uint64_t target)
{
	changed = true;
	weightsChanged = true;
	systemsData[sys->identity()].writesTo.insert(target);
	componentsData[target].writers.insert(sys->identity());
}

void Controller::registerDependency(const SystemBase* sys, uint64_t target)
{
	changed = true;
	weightsChanged = true;
	systemsData[sys->identity()].dependencies.insert(target);
	systemsData[target].dependees.insert(sys->identity());
}

void Controller::registerSystem(const SystemBase* sys)
{
	systems[sys->identity()] = sys;
}

void Controller::registerComponent(ComponentBase* component)
{
	components[component->identity()] = component;
}

ComponentBase* Controller::get(uint64_t identifier)
{
	return components[identifier];
}

} // namespace czss

#endif // CZSS_IMPLEMENTATION_GUARD_
#endif // CZSS_IMPLEMENTATION