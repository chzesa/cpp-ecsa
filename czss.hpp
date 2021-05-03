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
	const SystemBase* system = nullptr;
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
	const SystemBase* system = nullptr;
	std::unordered_set<uint64_t> exclusions;
	std::unordered_set<uint64_t> implicitDependencies;
	std::unordered_set<uint64_t> prunedDependencies;
	uint64_t weight;
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
	bool validateGraph = false;
	std::unordered_map<uint64_t, SystemData> systems;
	std::unordered_map<uint64_t, Node> graph;

	void validate();
	void addExclusion(uint64_t left, uint64_t right);
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
System<S>::System()
{
	contoller = defaultController();
	contoller->registerSystem(this);
}

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
	if (validateGraph)
	{
		validate();
		validateGraph = false;
	}

	std::vector<TaskData> tasks;
	std::unordered_map<uint64_t, uint64_t> deps;
	std::unordered_map<uint64_t, czsf::Barrier> fences;

	auto comp = []( OrderedNode a, OrderedNode b ) { return a.value < b.value; };
	std::priority_queue<OrderedNode, std::vector<OrderedNode>, decltype(comp)> que(comp);

	for (auto& pair : systems)
	{
		if (pair.second.dependees.size() == 0)
		{
			que.push({
				pair.first,
				graph[pair.first].weight
			});
		}

		deps[pair.first] = pair.second.dependees.size();
		fences.insert({pair.first, czsf::Barrier(1)});
	}

	while(que.size() > 0)
	{
		OrderedNode node = que.top();
		que.pop();
		SystemData& data = systems[node.key];
		for (auto dep : data.dependencies)
		{
			deps[dep]--;
			if (deps[dep] == 0)
			{
				que.push({
					dep,
					graph[dep].weight
				});
			}
		}

		if (data.system != nullptr)
		{
			tasks.push_back({
				node.key,
				data.system,
				&graph[node.key].prunedDependencies,
				&fences
			});
		}
	}

	std::reverse(tasks.begin(), tasks.end());
	czsf::Barrier barrier(tasks.size());
	czsf::run(task, tasks.data(), tasks.size(), &barrier);
	barrier.wait();
}

void Controller::addExclusion(uint64_t left, uint64_t right)
{
	Node& l = graph[left];
	Node& r = graph[right];
	l.exclusions.insert(right);
	r.exclusions.insert(left);
}

void Controller::validate()
{
	std::unordered_map<uint64_t, uint64_t> blockers;
	std::deque<uint64_t> queue;

	// Iterate through every node. For each system B that system A reads, if system C writes
	// to system B, and no dependency between A and C exists, mark them exclusive. Similarly
	// for system B that A writes to and C reads.
	for (auto& pair : systems)
	{
		SystemData& data = pair.second;
		for (auto key : data.readsFrom)
		{
			std::unordered_set<uint64_t>& writers = systems[key].writers;
			for (auto conflict : writers)
			{
				if (conflict == pair.first) continue;
				if (data.dependencies.find(conflict) == data.dependencies.end()
					&& data.dependees.find(conflict) == data.dependencies.end())
				{
					addExclusion(pair.first, conflict);
				}
			}
		}

		for (auto key : data.writesTo)
		{
			std::unordered_set<uint64_t>& readers = systems[key].readers;
			for (auto conflict : readers)
			{
				if (conflict == pair.first) continue;
				if (data.dependencies.find(conflict) == data.dependencies.end()
					&& data.dependees.find(conflict) == data.dependencies.end())
				{
					addExclusion(pair.first, conflict);
				}
			}

			std::unordered_set<uint64_t>& writers = systems[key].writers;
			for (auto conflict : writers)
			{
				if (conflict == pair.first) continue;
				if (data.dependencies.find(conflict) == data.dependencies.end()
					&& data.dependees.find(conflict) == data.dependencies.end())
				{
					addExclusion(pair.first, conflict);
				}
			}
		}

		if (pair.second.dependencies.size() == 0)
		{
			queue.push_back(pair.first);
		}

		blockers.insert({pair.first, pair.second.dependencies.size()});
	}

	// BFS through tree to build a list of transitive dependencies
	while(queue.size() > 0)
	{
		uint64_t key = queue.front();
		queue.pop_front();

		SystemData& data = systems[key];
		Node& node = graph[key];

		for (auto dep : data.dependencies)
		{
			SystemData& depData = systems[dep];
			for(auto transitive : depData.dependencies)
			{
				node.implicitDependencies.insert(transitive);
			}

			Node& depNode = graph[dep];
			for (auto transitive : depNode.implicitDependencies)
			{
				node.implicitDependencies.insert(transitive);
			}
		}

		for (auto dee : data.dependees)
		{
			blockers[dee]--;
			if (blockers[dee] == 0)
			{
				queue.push_back(dee);
			}

			if (blockers[dee] < 0)
			{
				std::string errMsg = "[czss] Cyclical reference to " + std::to_string(dee);
				throw std::logic_error(errMsg);
			}
		}
	}

	// Ensure dependencies exist between all resources that read and write to a shared resource
	// Calculate node weights for ordering
	for (auto& pair : graph)
	{
		Node& cur = pair.second;
		for (auto conflict : pair.second.exclusions)
		{
			Node& other = graph[conflict];
			if (cur.implicitDependencies.find(conflict) == cur.implicitDependencies.end()
				&& other.implicitDependencies.find(pair.first) == other.implicitDependencies.end())
			{
				std::string errMsg =
					"[czss] No ordering between elements "
					+ std::to_string(pair.first) + " and " + std::to_string(conflict)
					+ " accessing the same resource";
				throw std::logic_error(errMsg);
			}
		}

		SystemData& data = systems[pair.first];
		for (auto dep : data.dependencies)
		{
			if (cur.implicitDependencies.find(dep) == cur.implicitDependencies.end())
			{
				cur.prunedDependencies.insert(dep);
			}
		}

		cur.weight = cur.implicitDependencies.size() + cur.prunedDependencies.size();
	}
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
	auto node = graph.find(sys->identity());
	if (node == graph.end())
	{
		graph.insert({sys->identity(), {}});
		graph[sys->identity()].system = sys;
	} else
	{
		node->second.system = sys;
	}

	node = graph.find(target);
	if (node == graph.end())
	{
		graph.insert({target, {}});
	}

	validateGraph = true;
	RegistrationHelper ret = {};
	uint64_t src = sys->identity();
	registerSystem(sys);
	reg(target);

	ret.src = &systems.find(src)->second;
	ret.tar = &systems.find(target)->second;
	return ret;
}

void Controller::registerSystem(const SystemBase* sys)
{
	uint64_t key = sys->identity();
	auto data = systems.find(key);
	if (data == systems.end())
	{
		SystemData d = {};
		d.system = sys;
		systems.insert({key, d});
	} else 
	{
		data->second.system = sys;
	}
}

void Controller::reg(uint64_t key)
{
	auto data = systems.find(key);
	if (data == systems.end())
	{
		systems.insert({key, {}});
	}
}

SystemBase* Controller::get(uint64_t identifier)
{
	auto res = systems.find(identifier);
	if (res == systems.end())
	{
		return nullptr;
	}

	return const_cast<SystemBase*>(res->second.system); // todo
}

} // namespace czss

#endif // CZSS_IMPLEMENTATION_GUARD_
#endif // CZSS_IMPLEMENTATION