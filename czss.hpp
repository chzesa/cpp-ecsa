#ifndef CZSS_HEADERS_H
#define CZSS_HEADERS_H

#include <cstring>
#include <cstdint>
#include <type_traits>
#include <vector>
#include <queue>
#include <unordered_map>
#include "external/c-fiber/czsf.h"
#include <math.h>

namespace czss
{

struct Dummy
{
	using Cont = Dummy;
	template <typename Return, typename Inspector>
	constexpr static Return evaluate() { return 0; }

	template <typename Return, typename Inspector>
	constexpr static Return evaluate(uint64_t value) { return 0; }

	template <typename Inspector, typename A>
	inline static void evaluate(A a) { }

	template <typename Inspector, typename A, typename B>
	inline static void evaluate(A a, B b) { }

	template <typename Inspector, typename A, typename B, typename C>
	inline static void evaluate(A a, B b, C c) { }

	template <typename Return, typename Inspector, typename A>
	static Return evaluate(A a) { return 0; }
};

struct Root
{
	template <typename U>
	constexpr static bool baseContainsType() { return false; }

	template <typename U>
	constexpr static bool containsType() { return false; }
};

template <typename Base, typename Value, typename ...Rest>
struct Rbox : Rbox <Base, Value>, Rbox<Rbox <Base, Value>, Rest...>
{
	using Fwd = Rbox<Rbox <Base, Value>, Rest...>;
	using Cont = Rbox <Base, Value, Rest...>;

	template <typename Return, typename Inspector>
	constexpr static Return evaluate()
	{
		return Inspector::template inspect<Base, Cont, Value, typename Value::Cont, typename Fwd::Cont>();
	}

	template <typename Return, typename Inspector>
	constexpr static Return evaluate(uint64_t value)
	{
		return Inspector::template inspect<Base, Cont, Value, typename Value::Cont, typename Fwd::Cont>(value);
	}

	template <typename Inspector, typename A>
	inline static void evaluate(A* a)
	{
		Inspector::template inspect<Base, Cont, Value, typename Value::Cont, typename Fwd::Cont>(a);
	}

	template <typename Inspector, typename A, typename B>
	inline static void evaluate(A* a, B* b)
	{
		Inspector::template inspect<Base, Cont, Value, typename Value::Cont, typename Fwd::Cont>(a, b);
	}

	template <typename Inspector, typename A, typename B, typename C>
	inline static void evaluate(A* a, B* b, C* c)
	{
		Inspector::template inspect<Base, Cont, Value, typename Value::Cont, typename Fwd::Cont>(a, b, c);
	}

	template <typename Return, typename Inspector, typename A>
	static Return evaluate(A* a)
	{
		return Inspector::template inspect<Base, Cont, Value, typename Value::Cont, typename Fwd::Cont>(a);
	}
};

template <typename Base, typename Value>
struct Rbox <Base, Value>
{
	using Cont = Rbox <Base, Value>;

	template <typename Return, typename Inspector>
	constexpr static Return evaluate()
	{
		return Inspector::template inspect<Base, Cont, Value, typename Value::Cont, Dummy>();
	}

	template <typename Return, typename Inspector>
	constexpr static Return evaluate(uint64_t value)
	{
		return Inspector::template inspect<Base, Cont, Value, typename Value::Cont, Dummy>(value);
	}

	template <typename Inspector, typename A>
	inline static void evaluate(A* a)
	{
		Inspector::template inspect<Base, Cont, Value, typename Value::Cont, Dummy>(a);
	}

	template <typename Inspector, typename A, typename B>
	inline static void evaluate(A* a, B* b)
	{
		Inspector::template inspect<Base, Cont, Value, typename Value::Cont, Dummy>(a, b);
	}

	template <typename Inspector, typename A, typename B, typename C>
	inline static void evaluate(A* a, B* b, C* c)
	{
		Inspector::template inspect<Base, Cont, Value, typename Value::Cont, Dummy>(a, b, c);
	}

	template <typename Return, typename Inspector, typename A>
	static Return evaluate(A* a)
	{
		return Inspector::template inspect<Base, Cont, Value, typename Value::Cont, Dummy>(a);
	}
};

template <typename ...T>
struct Container : Rbox<Root, T...> { };

template <>
struct Container<> : Dummy { };

template <typename T>
constexpr T min(T a, T b)
{
	return a < b ? a : b;
}

template <typename T>
constexpr T max(T a, T b)
{
	return a > b ? a : b;
}

namespace inspect
{

template <typename Ret, typename Cont, typename Inspector>
constexpr Ret cInspect()
{
	return Cont::template evaluate<Ret, Inspector>();
}

template <typename T>
struct TypeCounter
{
	template <typename Base, typename This, typename Value, typename Inner, typename Next>
	constexpr static uint64_t inspect()
	{
		return (std::is_same<T, Value>() ? 1 : 0)
			+ Inner::template evaluate <uint64_t, TypeCounter<T>>()
			+ Next::template evaluate <uint64_t, TypeCounter<T>>();
	}
};

template <typename T>
struct DerivativeCounter
{
	template <typename Base, typename This, typename Value, typename Inner, typename Next>
	constexpr static uint64_t inspect()
	{
		return (std::is_base_of<T, Value>() ? 1 : 0)
			+ Inner::template evaluate <uint64_t, DerivativeCounter<T>>()
			+ Next::template evaluate <uint64_t, DerivativeCounter<T>>();
	}
};

template <typename Cont, typename T>
constexpr uint64_t numInstances()
{
	return cInspect<uint64_t, Cont, TypeCounter<T>>();
}

template <typename Cont>
struct NumContained
{
	template <typename Base, typename This, typename Value, typename Inner, typename Next>
	constexpr static uint64_t inspect()
	{
		return (numInstances<Cont, Value>() > 0 ? 1 : 0)
			+ Next::template evaluate<uint64_t, NumContained<Cont>>();
	}
};

template <typename Cont, typename ...T>
constexpr bool containsAll()
{
	return Container<T...>::template evaluate<uint64_t, NumContained<Cont>>()
		== sizeof...(T);
}

template <typename Cont, typename ...T>
constexpr bool contains()
{
	return Container<T...>::template evaluate<uint64_t, NumContained<Cont>>() > 0;
}

template <typename Cont, typename T, typename Cat>
constexpr uint64_t indexOf();

template <typename A, typename B>
constexpr bool branch()
{
	return !std::is_same<A, Dummy>() && !std::is_same<B, Dummy>();
}

template <typename Cont>
struct ContentsCheckerAll
{
	template <typename Base, typename This, typename Value, typename Inner, typename Next>
	constexpr static uint64_t inspect()
	{
		return contains<Cont, Value>()
			&& ( std::is_same<Next, Dummy>() ? true : Next::template evaluate <bool, ContentsCheckerAll<Cont>>() )
			&& ( std::is_same<Inner, Dummy>() ? true : Inner::template evaluate <bool, ContentsCheckerAll<Cont>>() );
	}
};

template <typename Cont>
struct ContentsCheckerAny
{
	template <typename Base, typename This, typename Value, typename Inner, typename Next>
	constexpr static uint64_t inspect()
	{
		return contains<Cont, Value>()
			|| ( std::is_same<Next, Dummy>() ? true : Next::template evaluate <bool, ContentsCheckerAny<Cont>>() )
			|| ( std::is_same<Inner, Dummy>() ? true : Inner::template evaluate <bool, ContentsCheckerAny<Cont>>() );
	}
};

template<typename Left, typename Right>
constexpr bool containsAllIn()
{
	return Right::template evaluate<bool, ContentsCheckerAll<Left>>();
}

template<typename Left, typename Right>
constexpr bool containsAnyIn()
{
	return Right::template evaluate<bool, ContentsCheckerAny<Left>>();
}

/*
	Algorithm to uniquely number each type within a container.
	Every time an item has both Next (in the "array"), and 
	is itself a container and therefore can be recursed inside,
	the path through the container splits.
	On such a split, the Next type is folded into the current
	root of the node and this becomes the new root of the Inner
	branch.
	This way the recursion in the Inner branch can test whether
	the other branch already has an item. If the item exists in
	the other branch, or if it exists further in the same
	branch, it has already been counted and the counter is not
	incremented.
*/

template <typename Root, typename T, typename Cat>
struct IndexFinder
{
	template <typename Base, typename This, typename Value, typename Inner, typename Next>
	constexpr static uint64_t inspect()
	{
		return (	branch<Inner, Next>()
					? Inner::template evaluate <uint64_t, IndexFinder<Container<Root, Next>, T, Cat>>()
					: Inner::template evaluate <uint64_t, IndexFinder<Root, T, Cat>>()
			)
			+ Next::template evaluate <uint64_t, IndexFinder<Root, T, Cat>>()
			+ (
				std::is_base_of<Cat, Value>()
					? ( ( std::is_same<T, Value>() || contains<Next, Value, T>() || contains<Inner, Value, T>() || contains<Root, Value, T>() )
						? 0 : 1
					) : 0
			);
	}
};

template <typename Cont, typename Cat>
struct UniquesCounter
{
	template <typename Base, typename This, typename Value, typename Inner, typename Next>
	constexpr static uint64_t inspect()
	{
		return 	max(( std::is_base_of<Cat, Value>() ? indexOf<Cont, Value, Cat>() : 0 ),
				max(Inner::template evaluate <uint64_t, UniquesCounter<Cont, Cat>>(),
					Next::template evaluate <uint64_t, UniquesCounter<Cont, Cat>>() ));
	}
};

template <typename Cont, typename Cat>
constexpr uint64_t numUniques()
{
	return cInspect<uint64_t, Cont, UniquesCounter<Cont, Cat>>() +
		(cInspect<uint64_t, Cont, DerivativeCounter<Cat>>() > 0 ? 1 : 0 );
};

template <typename Cont, typename T, typename Cat>
constexpr uint64_t indexOf()
{
	return cInspect<uint64_t, Cont, IndexFinder<Dummy, T, Cat>>();
}

} // namespace inspect

// #####################
// Tags & Validation
// #####################

struct EnableConstructor {};
struct DisableConstructor {};
struct DisableDestructor {};
struct ThreadSafe {};

struct ComponentBase {};
struct IteratorBase {};
struct EntityBase {};
struct SystemBase {};
struct ResourceBase {};
struct PermissionsBase {};

template <typename T>
constexpr bool isValidType();

template<typename B, typename T>
constexpr bool isBaseType();

template<typename T>
constexpr bool isThreadSafe();

template<typename T>
constexpr bool isComponent();

template<typename T>
constexpr bool isIterator();

template<typename T>
constexpr bool isEntity();

template<typename T>
constexpr bool isSystem();

template<typename T>
constexpr bool isResource();

template<typename T>
constexpr bool isPermission();

// #####################
// Data Container
// #####################

struct Guid
{
	Guid();
	Guid(uint64_t id);
	uint64_t get();
private:
	uint64_t id;
};

template <typename T>
struct Padding
{
	Padding() : Padding(true) {  }
	Padding(bool init) { if (init && !std::is_base_of<DisableConstructor, T>()) new(into()) T(); }
	~Padding() { if (!std::is_base_of<DisableDestructor, T>()) into()->~T(); }
	T* into() { return reinterpret_cast<T*>(d); }
private:
	char d[max(sizeof(T), uint64_t(1))];
};

template <typename T>
struct SparseVec
{
	bool init = false;
	std::priority_queue<uint64_t, std::vector<uint64_t>, std::greater<uint64_t>> indices;

	SparseVec();
	uint64_t create();
	void destroy(uint64_t i);

	T* get(uint64_t id);
	T* first();

	Padding<T>* items;
	uint64_t size;
private:
	void expand(uint64_t by);
};

template <typename E>
struct EntityStore
{
	bool init = false;
	std::unordered_map<uint64_t, Padding<E>> map;
	uint64_t nextId;

	E* get(uint64_t id);

	E* create(uint64_t& id);
};

// #####################
// stubs for templates
// #####################

struct TemplateStubs
{
	template <typename T>
	constexpr static bool isCompatible() { return false; }

	template <typename T>
	void setComponent(T* p) {}

	template <typename T>
	T* getComponent() { return nullptr; }

	void setGuid(Guid guid);
	Guid getGuid();
};

// #####################
// Primitives
// #####################

template <typename C>
struct Component : Dummy, ComponentBase, TemplateStubs
{
	using Cont = Dummy;

	friend C;
private:
	Component() {};
};

template<typename R>
struct Resource : Dummy, ResourceBase, TemplateStubs
{
	using Cont = Dummy;

	friend R;
	Resource() {};
};

// #####################
// Component collection types
// #####################

template <typename ...Components>
struct ComponentContainer : Container<Components...>
{
	void* components[max(inspect::numUniques<Container<Components...>, ComponentBase>(), uint64_t(1))];

	template <typename Component>
	constexpr static bool containsComponent();

	template <typename Component>
	constexpr static uint64_t componentIndex();

	template <typename Component>
	Component* getComponent();

	template <typename Component>
	void setComponent(Component* p);

	ComponentContainer() {}
	~ComponentContainer() {}
};

template <typename ...Components>
struct Iterator : ComponentContainer<Components...>, IteratorBase
{
	using Cont = Container<Components...>;

	template <typename Entity>
	constexpr static bool isCompatible();

	void setGuid(Guid guid) { this->id = guid; }
	Guid getGuid() { return id; }

private:
	Guid id;
};

struct VirtualArchitecture;

template <typename ...Components>
struct Entity : ComponentContainer<Components...>, EntityBase
{
	using Cont = Container<Components...>;

	template <typename Iterator>
	constexpr static bool isCompatible();

private:
	void setGuid(Guid guid) { this->id = guid.get(); }
	Guid getGuid() { return {id}; }
	friend VirtualArchitecture;
	uint64_t id;
};

// #####################
// Permissions
// #####################

template <typename ...T>
struct Reader : Container<T...>, TemplateStubs
{
	using Cont = Container<T...>;

	template <typename U>
	constexpr static bool canRead();

	template <typename U>
	constexpr static bool canWrite();

	template <typename Entity>
	constexpr static bool canOrchestrate();
};

template <typename ...T>
struct Writer : Container<T...>, TemplateStubs
{
	using Cont = Container<T...>;

	template <typename U>
	constexpr static bool canWrite();

	template <typename U>
	constexpr static bool canRead();

	template <typename Entity>
	constexpr static bool canOrchestrate();
};

template <typename ...Entities>
struct Orchestrator : Container<Entities...>, TemplateStubs
{
	using Cont = Container<Entities...>;

	template <typename U>
	constexpr static bool canRead();

	template <typename U>
	constexpr static bool canWrite();

	template <typename Entity>
	constexpr static bool canOrchestrate();
};

// #####################
// Graphs
// #####################

template <typename ...N>
struct Dependency : Container<N...>
{
	using Cont = Container<N...>;

	template <typename Node>
	constexpr static bool transitivelyDependsOn();

	template <typename Node>
	constexpr static bool directlyDependsOn();

	template <typename Node>
	constexpr static bool dependsOn();

private:
	template <typename Node>
	struct TransitiveDependencyInspector
	{
		template <typename Base, typename This, typename Value, typename Inner, typename Next>
		constexpr static bool inspect();
	};
};

template <>
struct Dependency<> : Container<>
{
	template <typename Node>
	constexpr static bool transitivelyDependsOn() { return false; }

	template <typename Node>
	constexpr static bool directlyDependsOn() { return false; }

	template <typename Node>
	constexpr static bool dependsOn() { return false; }
};

// #####################
// System
// #####################

template <typename Dependencies, typename ...Permissions>
struct System : Container<Permissions...>, SystemBase, TemplateStubs
{
	using This = System<Dependencies, Permissions...>;
	using Cont = Container<Permissions...>;
	using Dep = Dependencies;

	template <typename Other>
	constexpr static bool exclusiveWith();

	template <typename Other>
	constexpr static bool dependsOn();

	template <typename T>
	constexpr static bool canRead();

	template <typename T>
	constexpr static bool canWrite();

	template <typename Entity>
	constexpr static bool canOrchestrate();

private:
	template <typename Sys>
	struct SysExclCheck
	{
		template <typename Base, typename This, typename Value, typename Inner, typename Next>
		constexpr static bool inspect();
	};

	template <typename T>
	struct ReadCheck
	{
		template <typename Base, typename This, typename Value, typename Inner, typename Next>
		constexpr static bool inspect();
	};

	template <typename T>
	struct WriteCheck
	{
		template <typename Base, typename This, typename Value, typename Inner, typename Next>
		constexpr static bool inspect();
	};

	template <typename E>
	struct OrchestrateCheck
	{
		template <typename Base, typename This, typename Value, typename Inner, typename Next>
		constexpr static bool inspect();
		
	};
};

// #####################
// Architectures
// #####################

template <typename Arch, typename Entity>
struct EntityId
{
private:
	friend VirtualArchitecture;
	uint64_t id;

	EntityId(uint64_t id)
	{
		this->id = id;
	}
};

struct VirtualArchitecture
{
	virtual void systemCallback(uint64_t id, czsf::Barrier* barriers) = 0;

protected:
	template<typename Entity>
	static void setEntityId(Entity* ent, uint64_t id)
	{
		ent->setGuid(Guid(id));
	}

	template<typename Entity>
	static uint64_t getEntityId(Entity* ent)
	{
		return ent->getGuid().get();
	}

	template <typename Arch, typename Entity>
	static EntityId<Arch, Entity> getEntityId(Entity* ent)
	{
		return EntityId<Arch, Entity>(VirtualArchitecture::getEntityId(ent));
	}

	template <typename Arch, typename Entity>
	static uint64_t getEntityId(EntityId<Arch, Entity> key)
	{
		return key.id;
	}
};

struct RunTaskData
{
	VirtualArchitecture* arch;
	czsf::Barrier* barriers;
	uint64_t id;
};

void runSysCallback(RunTaskData* d);

template <typename Desc, typename ...Systems>
struct Architecture : VirtualArchitecture
{
	using Cont = Container<Systems...>;
	using This = Architecture<Desc, Systems...>;

	Architecture()
	{
		static_assert(!isSystem<Desc>(), "The first template parameter for Architecture must be the inheriting object");
		// TODO assert all components are used in at least one entity
		// TODO assert every entity can be instantiated
		static_assert(!Cont::template evaluate<bool, SystemDependencyIterator>(), "Some System A depends on some System B which is not included in Architecture.");
		static_assert(!Cont::template evaluate<bool, SystemDependencyMissingOuter>(), "An explicit dependency is missing between two systems.");

		Cont::template evaluate<Constructor>(this);
	}

	static constexpr uint64_t numSystems() { return inspect::numUniques<Cont, SystemBase>(); }

	template <typename Resource>
	void setResource(Resource* res)
	{
		static_assert(isResource<Resource>(), "Template parameter must be a Resource.");
		static_assert(inspect::contains<Cont, Resource>(), "Architecture doesn't contain Resource.");
		uint64_t index = inspect::indexOf<Container<Systems...>, Resource, ResourceBase>();
		resources[index] = res;
	}

	template <typename Resource>
	Resource* getResource()
	{
		static_assert(isResource<Resource>(), "Template parameter must be a Resource.");
		static_assert(inspect::contains<Cont, Resource>(), "Architecture doesn't contain Resource.");
		uint64_t index = inspect::indexOf<Container<Systems...>, Resource, ResourceBase>();
		return reinterpret_cast<Resource*>(resources[index]);
	}

	template <typename Component>
	SparseVec<Component>* getComponents()
	{
		// static_assert(isComponent<Component>(), "Template parameter must be a Component.");
		uint64_t index = inspect::indexOf<Cont, Component, ComponentBase>();
		return reinterpret_cast<SparseVec<Component>*>(&components[0]) + index;
	}

	template <typename Entity>
	EntityStore<Entity>* getEntities()
	{
		// assert(isEntity<Entity>());
		uint64_t index = inspect::indexOf<Cont, Entity, EntityBase>();
		return reinterpret_cast<EntityStore<Entity>*>(&entities[0]) + index;
	}

	template <typename Entity>
	Entity* getEntity(Guid guid)
	{
		return inspect::indexOf<Cont, Entity, EntityBase>() == typeKey(guid)
			? getEntity<Entity>(guidId(guid))
			: nullptr;
	}

	template <typename Entity>
	Entity* getEntity(uint64_t id)
	{
		return getEntities<Entity>()->get(id);
	}

	template <typename Entity>
	Entity* createEntity()
	{
		auto ent = initializeEntity<Entity>();
		if (std::is_base_of<EnableConstructor, Entity>())
			new(ent) Entity();

		return ent;
	}

	template <typename Entity, typename A>
	Entity* createEntity(A a)
	{
		auto ent = initializeEntity<Entity>();
		new (ent) Entity(a);
		return ent;
	}

	template <typename Entity, typename A, typename B>
	Entity* createEntity(A a, B b)
	{
		auto ent = initializeEntity<Entity>();
		new (ent) Entity(a, b);
		return ent;
	}

	template <typename Entity, typename A, typename B, typename C>
	Entity* createEntity(A a, B b, C c)
	{
		auto ent = initializeEntity<Entity>();
		new (ent) Entity(a, b, c);
		return ent;
	}

	template <typename Entity, typename A, typename B, typename C, typename D>
	Entity* createEntity(A a, B b, C c, D d)
	{
		auto ent = initializeEntity<Entity>();
		new (ent) Entity(a, b, c, d);
		return ent;
	}

	template <typename Entity, typename A, typename B, typename C, typename D, typename E>
	Entity* createEntity(A a, B b, C c, D d, E e)
	{
		auto ent = initializeEntity<Entity>();
		new (ent) Entity(a, b, c, d, e);
		return ent;
	}

	template <typename Entity>
	static Guid getEntityGuid(Entity* ent)
	{
		return Guid(VirtualArchitecture::getEntityId(ent)
			+ (inspect::indexOf<Cont, Entity, EntityBase>()) << (63 - typeKeyLength()));
	}

	template <typename Entity>
	static EntityId<Desc, Entity> getEntityId(Entity* ent)
	{
		return VirtualArchitecture::getEntityId<Desc, Entity>(ent);
	}

	template <typename Entity>
	void destroyEntity(uint64_t id)
	{
		auto entities = getEntities<Entity>();

		auto ent = entities->map.find(id);
		if (ent == entities->map.end())
			return;

		EntityComponentDestructor<Entity> ds = {this, entities->get(id)};
		Entity::template evaluate(&ds);

		entities->map.erase(ent);
	}

	void destroyEntity(Guid guid)
	{
		EntityDestructor ds {this, typeKey(guid), guidId(guid)};
		Cont::template evaluate(&ds);
	}

	template <typename Entity>
	void destroyEntity(EntityId<Desc, Entity> key)
	{
		destroyEntity<Entity>(This::getEntityId(key));
	}

	void run()
	{
		uint64_t sysCount = inspect::numUniques<Container<Systems...>, SystemBase>();
		czsf::Barrier barriers[sysCount];
		RunTaskData taskData[sysCount];

		for (uint64_t i = 0; i < sysCount; i++)
		{
			taskData[i].arch = this;
			taskData[i].barriers = barriers;
			taskData[i].id = i;
		}

		czsf::Barrier wait(sysCount);
		czsf::run(runSysCallback, taskData, sysCount, &wait);
		wait.wait();
	}

	template <typename Sys>
	void run();

	void systemCallback(uint64_t id, czsf::Barrier* barriers) override
	{
		Cont::template evaluate<SystemRunner>(&id, barriers, this);
	}

	static constexpr uint64_t typeKeyLength()
	{
		return ceil(log2(inspect::numUniques<Cont, EntityBase>()));
	}

	static uint64_t typeKey(Guid guid)
	{
		uint64_t klen = typeKeyLength();
		uint64_t mask = ((uint64_t(1) << (klen + 1)) - 1) << (63 - klen);
		return (guid.get() & mask) >> (63 - klen);
	}
	static uint64_t guidId(Guid guid)
	{
		uint64_t klen = typeKeyLength();
		uint64_t key = typeKey(guid) << (63 - klen);

		return guid.get() - key;
	}

	~Architecture()
	{
		Cont::template evaluate<Destructor>(this);
	}

private:
	void* resources[max(inspect::numUniques<Cont, ResourceBase>(), uint64_t(1))] = {0};
	char components[max(inspect::numUniques<Cont, ComponentBase>(), uint64_t(1)) * sizeof(SparseVec<char>)] = {0};
	char entities[max(inspect::numUniques<Cont, EntityBase>(), uint64_t(1)) * sizeof(EntityStore<uint64_t>)] = {0};

	template <typename Component>
	Component* createComponent()
	{
		static_assert(isComponent<Component>(), "Template parameter must be a Component");

		auto components = this->getComponents<Component>();
		auto first = components->first();
		uint64_t index = components->create();
		auto second = components->first();

		if (first != second)
		{
			Cont::template evaluate<ComponentPointerFixup<Component>>(this, first, second);
		}

		return components->get(index);
	}

	template <typename Entity>
	Entity* initializeEntity()
	{
		static_assert(isEntity<Entity>(), "Template parameter must be an Entity.");

		auto entities = getEntities<Entity>();

		uint64_t id;
		auto ent = entities->create(id);
		setEntityId(ent, id);
		Entity::template evaluate<ComponentCreator<Entity>>(this, ent);

		return ent;
	}

	struct SystemRunner
	{
		template <typename Base, typename Box, typename Value, typename Inner, typename Next>
		inline static void inspect(uint64_t* id, czsf::Barrier* barriers, This* arch);
	};

	template <typename Sys>
	struct SystemBlocker
	{
		template <typename Base, typename Box, typename Value, typename Inner, typename Next>
		inline static void inspect(czsf::Barrier* barriers)
		{
			if (Sys::Dep::template directlyDependsOn<Value>() && !Sys::Dep::template transitivelyDependsOn<Value>())
			{
				barriers[inspect::indexOf<Cont, Value, SystemBase>()].wait();
			}

			Next::template evaluate<SystemBlocker<Sys>>(barriers);
		}
	};

	template <typename E>
	struct ComponentCreator
	{
		template <typename Base, typename Box, typename Value, typename Inner, typename Next>
		inline static void inspect(This* arch, E* entity)
		{
			if (isComponent<Value>())
			{
				Value* p = arch->template createComponent<Value>();
				entity->template setComponent<Value>(p);
			}
			Next::template evaluate<ComponentCreator<E>>(arch, entity);
		}
	};

	template <typename E>
	struct EntityComponentDestructor
	{
		This* arch;
		E* entity;
		template <typename Base, typename Box, typename Value, typename Inner, typename Next>
		inline void inspect()
		{
			uint64_t index = entity->template getComponent<Value>()
				- reinterpret_cast<Value*>(arch->template getComponents<Value>()->items.data());
			arch->template getComponents<Value>()->destroy(index);

			Next::template evaluate(this);
		}
	};

	struct EntityDestructor
	{
		This* arch;
		uint64_t typeKey;
		uint64_t id;
		template <typename Base, typename Box, typename Value, typename Inner, typename Next>
		inline bool inspect()
		{
			if(isEntity<Value>() && inspect::contains<Cont, Value>() && inspect::indexOf<Cont, Value, EntityBase>() == typeKey)
			{
				arch->template destroyEntity<Value>(id);
				return true;
			}
			else
			{
				return Next::template evaluate<bool>(this) || Inner::template evaluate<bool>(this);
			}
		}
	};

	template<typename Component>
	struct ComponentPointerFixup
	{
		template <typename Base, typename Box, typename Value, typename Inner, typename Next>
		inline static void inspect(This* arch, Component* first, Component* second)
		{
			if (isEntity<Value>()
				&& inspect::contains<Value, Component>()
				&& !inspect::contains<Inner, Value>()
				&& !inspect::contains<Next, Value>())
			{
				auto ents = arch->template getEntities<Value>();
				for (auto& ent : ents->map)
				{
					ent.second.into()->template setComponent<Component>(
						ent.second.into()->template getComponent<Component>() - first + second
					);
				}
			}

			Inner::template evaluate<ComponentPointerFixup<Component>>(arch, first, second);
			Next::template evaluate<ComponentPointerFixup<Component>>(arch, first, second);
		}
	};

	// Used to initialize the architecture itself
	struct Constructor
	{
		template <typename Base, typename Box, typename Value, typename Inner, typename Next>
		static inline void inspect(This* arch)
		{
			if (isComponent<Value>())
			{
				auto p = arch->template getComponents<Value>();
				if (!p->init)
				{
					*p = SparseVec<Value>();
					p->init = true;
				}
			}

			if (isEntity<Value>())
			{
				auto p = arch->template getEntities<Value>();
				if(!p->init)
				{
					*p = EntityStore<Value>();
					p->init = true;
				}
			}

			Inner::template evaluate<Constructor>(arch);
			Next::template evaluate<Constructor>(arch);
		}
	};

	// Used to destruct the architecture itself
	struct Destructor
	{
		template <typename Base, typename Box, typename Value, typename Inner, typename Next>
		inline static void inspect(This* arch)
		{
			if (isComponent<Value>())
			{
				auto comp = arch->template getComponents<Value>();
				if (comp->init)
				{
					comp->init = false;
					comp->~SparseVec<Value>();
				}
			}

			if (isEntity<Value>())
			{
				auto ent = arch->template getEntities<Value>();
				if(ent->init)
				{
					ent->init = false;
					ent->~EntityStore<Value>();
				}
			}

			Inner::template evaluate<Destructor>(arch);
			Next::template evaluate<Destructor>(arch);
		}
	};

	struct SystemsCompleteCheck
	{
		template <typename Base, typename Box, typename Value, typename Inner, typename Next>
		static constexpr bool inspect()
		{
			return isSystem<Value>() && !inspect::contains<Cont, Value>()
				|| Next::template evaluate<bool, SystemsCompleteCheck>()
				|| Inner::template evaluate<bool, SystemsCompleteCheck>();
		}
	};

	struct SystemDependencyIterator
	{
		template <typename Base, typename Box, typename Value, typename Inner, typename Next>
		static constexpr bool inspect()
		{
			return (Value::Dep::template evaluate<bool, SystemsCompleteCheck>())
				|| Next::template evaluate<bool, SystemDependencyIterator>();
		}
	};

	template<typename Sys>
	struct SystemsCompare
	{
		template <typename Base, typename Box, typename Value, typename Inner, typename Next>
		static constexpr bool inspect()
		{
			return (isSystem<Value>()
				&& !std::is_same<Sys, Value>()
				&& Sys::template exclusiveWith<Value>()
				&& !Sys::template dependsOn<Value>()
				&& !Value::template dependsOn<Sys>())
				|| Next::template evaluate<bool, SystemsCompare<Sys>>();
		}
	};

	struct SystemDependencyMissingOuter
	{
		template <typename Base, typename Box, typename Value, typename Inner, typename Next>
		static constexpr bool inspect()
		{
			return Cont::template evaluate<bool, SystemsCompare<Value>>()
				|| Next::template evaluate <bool, SystemDependencyMissingOuter>();
		}
	};
};

// #####################
// Accessors
// #####################

template <typename Iter, typename Arch, typename Sys>
struct IteratorAccessor;

template <typename Iter, typename Arch, typename Sys>
struct IterableStub;

template <typename Iter, typename Arch, typename Sys>
struct IteratorIterator;

template<typename Arch, typename Sys>
struct Accessor;

template <typename Iter, typename Arch, typename Sys>
struct IteratorAccessor
{
	IteratorAccessor(const IteratorAccessor<Iter, Arch, Sys>& other);

	template<typename Component>
	const Component* view();

	template<typename Component>
	Component* get();

	Guid guid();

private:
	friend IteratorIterator<Iter, Arch, Sys>;

	IteratorAccessor() {}

	template <typename Entity>
	IteratorAccessor(Entity* ent)
	{
		iter.setGuid(Arch::getEntityGuid(ent));
		Iter::template evaluate<Constructor<Entity>>(&iter, ent);
	}

	Iter iter;

	template<typename Entity>
	struct Constructor
	{
		template <typename Base, typename This, typename Value, typename Inner, typename Next>
		inline static void inspect(Iter* iter, Entity* ent)
		{
			iter->template setComponent<Value>( ent->template getComponent<Value>() );
			Next::template evaluate<Constructor<Entity>>(iter, ent);
		}
	};
};

template <typename Iter, typename Arch, typename Sys>
struct IteratorIterator
{
	using This = IteratorIterator<Iter, Arch, Sys>;
	using U = IteratorAccessor<Iter, Arch, Sys>;

	typedef uint64_t difference_type;
	typedef U value_type;
	typedef U& reference;
	typedef U* pointer;
	typedef std::forward_iterator_tag iterator_category;

	IteratorIterator()
	{
		arch = nullptr;
		iterator = {};
	}

	IteratorIterator(const IteratorIterator& other)
	{
		arch = other.arch;
		typeKey = other.typeKey;
		iterator = other.iterator;
		hasValue = other.hasValue;
		inner = other.inner;
	}

	U& operator* ()
	{
		return inner;
	}

	U* operator-> ()
	{
		return inner;
	}

	friend bool operator== (const This& a, const This& b)
	{
		return a.iterator == b.iterator;
	}

	friend bool operator!= (const This& a, const This& b)
	{
		return a.iterator != b.iterator;
	};

	This operator++(int)
	{
		This ret = *this;
		++(*this);
		return ret;
	}
	
	This& operator++()
	{
		Arch::Cont::template evaluate<bool, Incrementer>(this);
		while(!hasValue && typeKey<inspect::numUniques<typename Arch::Cont, EntityBase>())
		{
			typeKey++;
			Arch::Cont::template evaluate<bool, Incrementer>(this);
		}

		return *this;
	}

private:
	friend IterableStub<Iter, Arch, Sys>;

	IteratorIterator(Arch* arch)
	{
		this->arch = arch;
		typeKey = 0;
		hasValue = false;
		++(*this);
	}

	Arch* arch;
	uint64_t typeKey;
	typename std::unordered_map<uint64_t, Padding<uint64_t>>::iterator iterator;
	bool hasValue;
	U inner;

	struct Incrementer
	{
		template <typename Base, typename Box, typename Value, typename Inner, typename Next>
		inline static bool inspect(This* iterac)
		{
			if ( isEntity<Value>() && Value::template isCompatible<Iter>()
				&& inspect::indexOf<typename Arch::Cont, Value, EntityBase>() == iterac->typeKey)
			{
				auto p = reinterpret_cast<typename std::unordered_map<uint64_t, Padding<Value>>::iterator*>(&iterac->iterator);

				if (!iterac->hasValue)
				{
					*p = iterac->arch->template getEntities<Value>()->map.begin();
					iterac->hasValue = true;
				}
				else
				{
					(*p)++;
				}

				if (*p == iterac->arch->template getEntities<Value>()->map.end())
				{
					iterac->hasValue = false;
				}

				if (iterac->hasValue)
				{
					iterac->inner = U(reinterpret_cast<Value*>(&(**p).second));
				}

				return true;
			}
			else
			{
				return Next::template evaluate<bool, Incrementer>(iterac) || Inner::template evaluate<bool, Incrementer>(iterac);
			}
		}
	};
};

template <typename Iter, typename Arch, typename Sys>
struct IterableStub
{
	IteratorIterator<Iter, Arch, Sys> begin()
	{
		return IteratorIterator<Iter, Arch, Sys>(arch);
	}

	IteratorIterator<Iter, Arch, Sys> end()
	{
		return IteratorIterator<Iter, Arch, Sys>();
	}

private:
	IterableStub(Arch* arch)
	{
		this ->arch = arch;
	}
	friend Accessor<Arch, Sys>;
	Arch* arch;
};

template<typename Arch, typename Sys>
struct Accessor
{
	template <typename Entity>
	Entity* createEntity()
	{
		entityPermission<Entity>();
		return arch->template createEntity<Entity>();
	}

	template <typename Entity, typename A >
	Entity* createEntity(A a)
	{
		entityPermission<Entity>();
		return arch->template createEntity<Entity>(a);
	}

	template <typename Entity, typename A, typename B>
	Entity* createEntity(A a, B b)
	{
		entityPermission<Entity>();
		return arch->template createEntity<Entity>(a, b);
	}

	template <typename Entity, typename A, typename B, typename C>
	Entity* createEntity(A a, B b, C c)
	{
		entityPermission<Entity>();
		return arch->template createEntity<Entity>(a, b, c);
	}

	template <typename Entity, typename A, typename B, typename C, typename D>
	Entity* createEntity(A a, B b, C c, D d)
	{
		entityPermission<Entity>();
		return arch->template createEntity<Entity>(a, b, c, d);
	}

	template <typename Entity, typename A, typename B, typename C, typename D, typename E>
	Entity* createEntity(A a, B b, C c, D d, E e)
	{
		entityPermission<Entity>();
		return arch->template createEntity<Entity>(a, b, c, d, e);
	}

	template <typename Entity>
	Entity* getEntity(Guid guid)
	{
		static_assert(isEntity<Entity>(), "Attempted to create non-entity.");
		static_assert(inspect::containsAnyIn<Entity, typename Sys::Cont>(), "System cannot access any component of the entity.");
		static_assert(inspect::contains<typename Arch::Cont, Entity>(), "Architecture doesn't contain the Entity.");
		return arch->template getEntity<Entity>(guid);
	}

	void destroyEntity(Guid guid)
	{
		if (Arch::Cont::template evaluate<bool, EntityDestructionPermission>(Arch::typeKey(guid)))
		{
			arch->destroyEntity(guid);
		}
	}

	template <typename Entity>
	void destroyEntity(EntityId<Arch, Entity> key)
	{
		static_assert(Sys::template canOrchestrate<Entity>(), "System does not have the permission to destroy this entity.");
		arch->destroyEntity(key);
	}

	template <typename Entity>
	Guid getEntityGuid(Entity* ent)
	{
		return Arch::getEntityGuid(ent);
	}

	template <typename Entity>
	EntityId<Arch, Entity> getEntityId(Entity* ent)
	{
		return Arch::getEntityId(ent);
	}

	template <typename Resource>
	const Resource* viewResource()
	{
		static_assert(isResource<Resource>(), "Attempted to read non-resource.");
		static_assert(Sys::template canRead<Resource>(), "System lacks permission to read Resource.");
		static_assert(inspect::contains<typename Arch::Cont, Resource>(), "Architecture doesn't contain the Resource.");
		return arch->template getResource<Resource>();
	}

	template <typename Resource>
	Resource* getResource()
	{
		static_assert(isResource<Resource>(), "Attempted to write to non-resource.");
		static_assert(Sys::template canWrite<Resource>(), "System lacks permission to modify Resource.");
		static_assert(inspect::contains<typename Arch::Cont, Resource>(), "Architecture doesn't contain the Resource.");
		return arch->template getResource<Resource>();
	}

	template <typename Iterator>
	IterableStub<Iterator, Arch, Sys> iterate()
	{
		static_assert(isIterator<Iterator>(), "Attempted to iterate non-iterator.");
		static_assert(inspect::containsAllIn<Sys, Iterator>(), "System doesn't have access to all components of Iterator.");
		static_assert(inspect::contains<typename Arch::Cont, Iterator>(), "Architecture doesn't contain the Iterator.");
		return IterableStub<Iterator, Arch, Sys>(arch);
	}

	template <typename Component>
	uint64_t componentIndex(Component* component)
	{
		static_assert(Sys::template canRead<Component>(), "System doesn't have read access to component.");
		return component - arch->template getComponents<Component>()->first();
	}

	template <typename Component>
	const Component* viewComponent(uint64_t index)
	{
		static_assert(Sys::template canRead<Component>(), "System doesn't have read access to component.");
		return arch->template getComponents<Component>()->get(index);
	}

	template <typename Component>
	Component* getComponent(uint64_t index)
	{
		static_assert(Sys::template canWrite<Component>(), "System doesn't have write access to component.");
		return arch->template getComponents<Component>()->get(index);
	}


// private:
	// friend Arch;
	Accessor(Arch* arch) { this->arch = arch; }
	Arch* arch;

private:
	template <typename Entity>
	void entityPermission()
	{
		static_assert(isEntity<Entity>(), "Attempted to create non-entity.");
		static_assert(Sys::template canOrchestrate<Entity>(), "System lacks permission to create the Entity.");
		static_assert(inspect::contains<typename Arch::Cont, Entity>(), "Architecture doesn't contain the Entity.");
	}

	struct EntityDestructionPermission
	{
		template <typename Base, typename This, typename Value, typename Inner, typename Next>
		static constexpr bool inspect(uint64_t key)
		{
			return isEntity<Value>()
				&& inspect::indexOf<typename Arch::Cont, Value, EntityBase>() == key
				&& Sys::template canOrchestrate<Value>()
				|| Inner::template evaluate<bool, Accessor<Arch, Sys>::EntityDestructionPermission>(key)
				|| Next::template evaluate<bool, Accessor<Arch, Sys>::EntityDestructionPermission>(key);
		}
	};
};


// #####################################################################
// Implementation
// #####################################################################

// #####################
// Type Validation
// #####################

template <typename T>
constexpr bool isValidType()
{
	return (1
		<< std::is_base_of<ComponentBase, T>()
		<< std::is_base_of<IteratorBase, T>()
		<< std::is_base_of<EntityBase, T>()
		<< std::is_base_of<SystemBase, T>()
		<< std::is_base_of<ResourceBase, T>()
		<< std::is_base_of<PermissionsBase, T>()
	) == 2;
}

template<typename B, typename T>
constexpr bool isBaseType()
{
	return isValidType<T>() && std::is_base_of<B, T>();
}

template<typename T>
constexpr bool isThreadSafe()
{
	return isBaseType<ResourceBase, T>() && std::is_base_of<ThreadSafe, T>();
}

template<typename T>
constexpr bool isComponent()
{
	return isBaseType<ComponentBase, T>();
}

template<typename T>
constexpr bool isIterator()
{
	return isBaseType<IteratorBase, T>();
}

template<typename T>
constexpr bool isEntity()
{
	return isBaseType<EntityBase, T>();
}

template<typename T>
constexpr bool isSystem()
{
	return isBaseType<SystemBase, T>();
}

template<typename T>
constexpr bool isResource()
{
	return isBaseType<ResourceBase, T>();
}

template<typename T>
constexpr bool isPermission()
{
	return isBaseType<PermissionsBase, T>();
}

// #####################
// Linked Vector
// #####################
template <typename T>
SparseVec<T>::SparseVec()
{
	static const uint64_t n = 64;
	items = (Padding<T>*) malloc(sizeof (Padding<T>) * n);

	for (int i = n; i > 0; i--)
		indices.push(i - 1);

	size = n;
}

template <typename T>
uint64_t SparseVec<T>::create()
{
	if (indices.empty())
		expand(size);

	uint64_t i = indices.top();
	indices.pop();
	new(&items[i]) Padding<T>();

	return i;
}

template <typename T>
T* SparseVec<T>::get(uint64_t i)
{
	if (i >= size)
		return nullptr;

	return items[i].into();
}

template <typename T>
T* SparseVec<T>::first()
{
	return items[0].into();
}

template <typename T>
void SparseVec<T>::destroy(uint64_t i)
{
	if (i >= size)
		return;

	items[i].~Padding<T>();

	indices.push(i);
}

template <typename T>
void SparseVec<T>::expand(uint64_t by)
{
	Padding<T>* next = (Padding<T>*) malloc(sizeof (Padding<T>) * (size + by));
	memcpy(next, items, sizeof (Padding<T>) * size);
	free(items);
	items = next;

	for (int i = size + by; i > size; i--)
		indices.push(i - 1);

	size += by;
}

template <typename E>
E* EntityStore<E>::get(uint64_t id)
{
	if (map.find(id) == map.end())
		return nullptr;

	return map[id].into();
}

template <typename E>
E* EntityStore<E>::create(uint64_t& id)
{
	id = nextId++;
	map.emplace(id, false);
	return map[id].into();
}

// #####################
// ComponentContainer
// #####################
template <typename ...Components>
template <typename Component>
constexpr bool ComponentContainer<Components...>::containsComponent()
{
	// assert(isComponent<Component>());
	return inspect::contains <ComponentContainer<Components...>, Component>();
}

template <typename ...Components>
template <typename Component>
constexpr uint64_t ComponentContainer<Components...>::componentIndex()
{
	// assert(containsComponent<Component>());
	return inspect::indexOf<ComponentContainer<Components...>, Component, ComponentBase>();
}

template <typename ...Components>
template <typename Component>
Component* ComponentContainer<Components...>::getComponent()
{
	// assert(containsComponent<Component>());
	if (!containsComponent<Component>())
	{
		return nullptr;
	}
	return reinterpret_cast<Component*>(components[componentIndex<Component>()]);
}

template <typename ...Components>
template <typename Component>
void ComponentContainer<Components...>::setComponent(Component* p)
{
	if (containsComponent<Component>())
	{
		components[componentIndex<Component>()] = p;
	}
}

// #####################
// Iterator
// #####################

template <typename ...Components>
template <typename Entity>
constexpr bool Iterator<Components...>::isCompatible()
{
	// assert(isEntity<Entity>());
	return inspect::containsAll<Entity, Components...>();
};

// #####################
// Entity
// #####################

template <typename ...Components>
template <typename Iterator>
constexpr bool Entity<Components...>::isCompatible()
{
	// assert(isIterator<Iterator>());
	return Iterator::template isCompatible <Entity<Components...>>();
}

// #####################
// Permission Objects
// #####################

// Reader

template <typename ...T> 
template <typename U>
constexpr bool Reader<T...>::canRead()
{
	return inspect::contains<Reader<T...>, U>();
}

template <typename ...T> 
template <typename U>
constexpr bool Reader<T...>::canWrite()
{
	return false;
}

template <typename ...T> 
template <typename Entity>
constexpr bool Reader<T...>::canOrchestrate()
{
	return false;
}

// Writer

template <typename ...T> 
template <typename U>
constexpr bool Writer<T...>::canRead()
{
	return canWrite<U>();
}

template <typename ...T> 
template <typename U>
constexpr bool Writer<T...>::canWrite()
{
	return inspect::contains<Writer<T...>, U>();
}

template <typename ...T> 
template <typename Entity>
constexpr bool Writer<T...>::canOrchestrate()
{
	return false;
}

// Orhcestrator

template <typename ...Entities> 
template <typename U>
constexpr bool Orchestrator<Entities...>::canRead()
{
	return canWrite<U>();
}

template <typename ...Entities> 
template <typename U>
constexpr bool Orchestrator<Entities...>::canWrite()
{
	return inspect::contains<Orchestrator<Entities...>, U>();
}

template <typename ...Entities>
template <typename Entity>
constexpr bool Orchestrator<Entities...>::canOrchestrate()
{
	return inspect::contains<Orchestrator<Entities...>, Entity>();
}

// #####################
// Graph
// #####################

template<typename ...N>
template <typename Node>
template <typename Base, typename This, typename Value, typename Inner, typename Next>
constexpr bool Dependency<N...>::TransitiveDependencyInspector<Node>::inspect()
{
	return Value::template dependsOn<Node>()
		|| Next::template evaluate<bool, TransitiveDependencyInspector<Node>>();
}

template <typename ...N>
template <typename Node>
constexpr bool Dependency<N...>::transitivelyDependsOn()
{
	return inspect::cInspect<bool, Dependency<N...>, TransitiveDependencyInspector<Node>>();
}

template <typename ...N>
template <typename Node>
constexpr bool Dependency<N...>::directlyDependsOn()
{
	return inspect::contains<Dependency<N...>, Node>();
}

template <typename ...N>
template <typename Node>
constexpr bool Dependency<N...>::dependsOn()
{
	return directlyDependsOn<Node>() || transitivelyDependsOn<Node>();
}

// #####################
// System
// #####################

template <typename Dependencies, typename ...Permissions>
template <typename Sys>
template <typename Base, typename This, typename Value, typename Inner, typename Next>
constexpr bool System<Dependencies, Permissions...>::SysExclCheck<Sys>::inspect()
{
	// return (isComponent<Value>() && Sys::template canWrite<Value>())
	return (!isResource<Value>() && Sys::template canWrite<Value>())
		|| (isResource<Value>() && !isThreadSafe<Value>() && Sys::template canWrite<Value>())
		|| Next::template evaluate<bool, SysExclCheck<Sys>>()
		|| Inner::template evaluate<bool, SysExclCheck<Sys>>();
}

template <typename Dependencies, typename ...Permissions>
template <typename Other>
constexpr bool System<Dependencies, Permissions...>::exclusiveWith()
{
	return Cont::template evaluate<bool, SysExclCheck<Other>>()
		|| Other::Cont::template evaluate<bool, SysExclCheck<This>>() ;
}

template <typename Dependencies, typename ...Permissions>
template <typename Other>
constexpr bool System<Dependencies, Permissions...>::dependsOn()
{
	return Dependencies::template dependsOn<Other>();
}

template <typename Dependencies, typename ...Permissions>
template <typename T>
constexpr bool System<Dependencies, Permissions...>::canRead()
{
	return Container<Permissions... >::template evaluate<bool, ReadCheck<T>>();
}

template <typename Dependencies, typename ...Permissions>
template <typename T>
template <typename Base, typename This, typename Value, typename Inner, typename Next>
constexpr bool System<Dependencies, Permissions...>::ReadCheck<T>::inspect()
{
	return Value::template canRead<T>() || Next::template evaluate<bool, ReadCheck<T>>();
}

template <typename Dependencies, typename ...Permissions>
template <typename T>
constexpr bool System<Dependencies, Permissions...>::canWrite()
{
	return Container<Permissions...>::template evaluate <bool, WriteCheck<T>>();
}

template <typename Dependencies, typename ...Permissions>
template <typename T>
template <typename Base, typename This, typename Value, typename Inner, typename Next>
constexpr bool System<Dependencies, Permissions...>::WriteCheck<T>::inspect()
{
	return Value::template canWrite<T>() || Next::template evaluate<bool, WriteCheck<T>>();
}

template <typename Dependencies, typename ...Permissions>
template <typename Entity>
constexpr bool System<Dependencies, Permissions...>::canOrchestrate()
{
	return Container<Permissions...>::template evaluate<bool, OrchestrateCheck<Entity>>();
}

template <typename Dependencies, typename ...Permissions>
template <typename E>
template <typename Base, typename This, typename Value, typename Inner, typename Next>
constexpr bool System<Dependencies, Permissions...>::OrchestrateCheck<E>::inspect()
{
	return Value::template canOrchestrate<E>() || Next::template evaluate<bool, OrchestrateCheck<E>>();
}

// #####################
// Architecture
// #####################

template <typename Desc, typename ...Systems>
template <typename Sys>
void Architecture<Desc, Systems...>::run()
{
	Sys::run(Accessor<Desc, Sys>(reinterpret_cast<Desc*>(this)));
}

template <typename Desc, typename ...Systems>
template <typename Base, typename Box, typename Value, typename Inner, typename Next>
void Architecture<Desc, Systems...>::SystemRunner::inspect(uint64_t* id, czsf::Barrier* barriers, This* arch)
{
	if (inspect::indexOf<Cont, Value, SystemBase>() != *id)
	{
		Next::template evaluate<SystemRunner>(id, barriers, arch);
		return;
	}

	Cont::template evaluate<SystemBlocker<Value>>(barriers);
	Value::run(Accessor<Desc, Value>(reinterpret_cast<Desc*>(arch)));
	barriers[*id].signal();
}


// #####################
// Accessors
// #####################

template <typename Iter,typename Arch, typename Sys>
IteratorAccessor<Iter,Arch, Sys>::IteratorAccessor(const IteratorAccessor<Iter, Arch, Sys>& other)
{
	this->iter = other.iter;
}

template <typename Iter,typename Arch, typename Sys>
template <typename Component>
const Component* IteratorAccessor<Iter,Arch, Sys>::view()
{
	static_assert(Iter::template containsComponent<Component>(), "Iterator doesn't contain the requested component.");
	static_assert(Sys::template canRead<Component>(), "System lacks read permissions for the Iterator's components.");
	return iter.template getComponent<Component>();
}

template <typename Iter,typename Arch, typename Sys>
template <typename Component>
Component* IteratorAccessor<Iter,Arch, Sys>::get()
{
	static_assert(Iter::template containsComponent<Component>(), "Iterator doesn't contain the requested component.");
	static_assert(Sys::template canWrite<Component>(), "System lacks write permissions for the Iterator's components.");
	return iter.template getComponent<Component>();
}

template <typename Iter,typename Arch, typename Sys>
Guid IteratorAccessor<Iter,Arch, Sys>::guid()
{
	return iter.getGuid();
}

} // namespace czss

#endif // CZSS_HEADERS_H

#ifdef CZSS_IMPLEMENTATION

#ifndef CZSS_IMPLEMENTATION_GUARD_
#define CZSS_IMPLEMENTATION_GUARD_

namespace czss
{

Guid::Guid() { }

Guid::Guid(uint64_t id)
{
	this->id = id;
}

uint64_t Guid::get()
{
	return id;
}

void TemplateStubs::setGuid(Guid guid) { }
Guid TemplateStubs::getGuid() { return Guid(0); }

void runSysCallback(RunTaskData* d)
{
	d->arch->systemCallback(d->id, d->barriers);
}

} // namespace czss

#endif	// CZSS_IMPLEMENTATION_GUARD_

#endif // CZSS_IMPLEMENTATION