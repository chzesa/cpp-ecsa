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

#if __cplusplus < 201703L
#define CZSS_CONST_IF if
#else
#define CZSS_CONST_IF if constexpr
#endif

#define CZSS_NAME(A, B) template <> const char* czss::name<A>() { const static char name[] = B; return name; }

template<typename T>
struct Repair
{
	Repair(T* min, T* max, int64_t offset)
	{
		this->min = min;
		this->max = max;
		this->offset = offset;
	}

	void operator ()(T*& ptr) const
	{
		if (min <= ptr && ptr < max)
			ptr += offset;
	}

private:
	T* min;
	T* max;
	int64_t offset;
};

template <typename Component, typename PointerType>
void managePointer(Component& component, const Repair<PointerType>& rep) { }

template <typename Value>
const static char* name()
{
	const static char name[] = "Unknown";
	return name;
}

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

	template <typename Inspector, typename A, typename B>
	inline static void evaluate(A a, B* b)
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

	template <typename Inspector, typename A, typename B>
	inline static void evaluate(A a, B* b)
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

template <typename Base, typename Value>
struct NrBox
{
	using Cont = NrBox <Base, Value>;
	template <typename Return, typename Inspector>
	constexpr static Return evaluate()
	{
		return Inspector::template inspect<Base, Dummy, Value, Dummy, Dummy>();
	}

	template <typename Return, typename Inspector>
	constexpr static Return evaluate(uint64_t value)
	{
		return Inspector::template inspect<Base, Dummy, Value, Dummy, Dummy>(value);
	}

	template <typename Inspector, typename A>
	inline static void evaluate(A* a)
	{
		Inspector::template inspect<Base, Dummy, Value, Dummy, Dummy>(a);
	}

	template <typename Inspector, typename A, typename B>
	inline static void evaluate(A* a, B* b)
	{
		Inspector::template inspect<Base, Dummy, Value, Dummy, Dummy>(a, b);
	}

	template <typename Inspector, typename A, typename B>
	inline static void evaluate(A a, B* b)
	{
		Inspector::template inspect<Base, Dummy, Value, Dummy, Dummy>(a, b);
	}

	template <typename Inspector, typename A, typename B, typename C>
	inline static void evaluate(A* a, B* b, C* c)
	{
		Inspector::template inspect<Base, Dummy, Value, Dummy, Dummy>(a, b, c);
	}

	template <typename Return, typename Inspector, typename A>
	static Return evaluate(A* a)
	{
		return Inspector::template inspect<Base, Dummy, Value, Dummy, Dummy>(a);
	}
};

template <typename T>
struct NrContainer : NrBox<Root, T> { };

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

template <typename T>
constexpr bool isDummy();

template <typename Fold, typename Callback>
struct OncePerType
{
	template <typename Base, typename Box, typename Value, typename Inner, typename Next>
	inline static void inspect()
	{
		CZSS_CONST_IF (!inspect::contains<Fold, Value>())
			Callback::template callback<Value>();

		Inner::template evaluate<OncePerType<Container<Fold, NrContainer<Value>, Next>, Callback>>();
		Next::template evaluate<OncePerType<Container<Fold, NrContainer<Value>>, Callback>>();
	}

	template <typename Base, typename Box, typename Value, typename Inner, typename Next, typename A>
	inline static void inspect(A* a)
	{
		CZSS_CONST_IF (!inspect::contains<Fold, Value>())
			Callback::template callback<Value>(a);

		Inner::template evaluate<OncePerType<Container<Fold, NrContainer<Value>, Next>, Callback>>(a);
		Next::template evaluate<OncePerType<Container<Fold, NrContainer<Value>>, Callback>>(a);
	}

	template <typename Base, typename Box, typename Value, typename Inner, typename Next, typename A, typename B>
	inline static void inspect(A* a, B* b)
	{
		CZSS_CONST_IF (!inspect::contains<Fold, Value>())
			Callback::template callback<Value>(a, b);

		Inner::template evaluate<OncePerType<Container<Fold, NrContainer<Value>, Next>, Callback>>(a, b);
		Next::template evaluate<OncePerType<Container<Fold, NrContainer<Value>>, Callback>>(a, b);
	}

	template <typename Base, typename Box, typename Value, typename Inner, typename Next, typename A, typename B>
	inline static void inspect(A a, B* b)
	{
		CZSS_CONST_IF (!inspect::contains<Fold, Value>())
			Callback::template callback<Value>(a, b);

		Inner::template evaluate<OncePerType<Container<Fold, NrContainer<Value>, Next>, Callback>>(a, b);
		Next::template evaluate<OncePerType<Container<Fold, NrContainer<Value>>, Callback>>(a, b);
	}

	template <typename Base, typename Box, typename Value, typename Inner, typename Next, typename A, typename B, typename C>
	inline static void inspect(A* a, B* b, C* c)
	{
		CZSS_CONST_IF (!inspect::contains<Fold, Value>())
			Callback::template callback<Value>(a, b, c);

		Inner::template evaluate<OncePerType<Container<Fold, NrContainer<Value>, Next>, Callback>>(a, b, c);
		Next::template evaluate<OncePerType<Container<Fold, NrContainer<Value>>, Callback>>(a, b, c);
	}
};

template <typename Container, int Num>
struct Switch
{
	template <typename Return, typename Inspector, typename A>
	constexpr static Return evaluate(int i, A a)
	{
		return i == Num - 1
			? Container::template evaluate<Return, Inspector>(a)
			: Switch<Container, Num - 1>::template evaluate<Return, Inspector>(i, a);
	}

	template <typename Inspector, typename A>
	inline static void evaluate(int i, A* a)
	{
		if (i == Num - 1)
			Container::template evaluate<Inspector>(a);
		else
			Switch<Container, Num - 1>::template evaluate<Inspector>(i, a);
	}

	template <typename Inspector, typename A, typename B>
	inline static void evaluate(int i, A* a, B* b)
	{
		if (i == Num - 1)
			Container::template evaluate<Inspector>(a, b);
		else
			Switch<Container, Num - 1>::template evaluate<Inspector>(i, a, b);
	}

	template <typename Inspector, typename A, typename B, typename C>
	inline static void evaluate(int i, A* a, B* b, C* c)
	{
		if (i == Num - 1)
			Container::template evaluate<Inspector>(a, b, c);
		else
			Switch<Container, Num - 1>::template evaluate<Inspector>(i, a, b, c);
	}
};

template <typename Container>
struct Switch <Container, 0>
{
	template <typename Return,  typename Inspector, typename A>
	constexpr static Return evaluate(int i, A a) { return Return(); }

	template <typename Inspector, typename A>
	inline static void evaluate(int i, A* a) { }

	template <typename Inspector, typename A, typename B>
	inline static void evaluate(int i, A* a, B* b) { }

	template <typename Inspector, typename A, typename B, typename C>
	inline static void evaluate(int i, A* a, B* b, C* c) { }
};


// #####################
// Tags & Validation
// #####################

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

template <typename E>
struct EntityStore
{
	EntityStore()
	{
		entities = reinterpret_cast<E*>(malloc(sizeof (E) * 32));

		for (uint64_t i = 0; i < 32; i++)
			free_indices.push(i);
	}

	E* get(uint64_t id)
	{
		auto res = index_map.find(id);
		if (res == index_map.end())
			return nullptr;

		return &entities[res->second];
	}

	E* create(uint64_t& id)
	{
		id = nextId++;
		if(free_indices.size() == 0)
		{
			E* old = reinterpret_cast<E*>(entities);
			expand(size());
			for (auto& p : used_indices)
				p = p - old + reinterpret_cast<E*>(entities);
		}

		uint64_t index = free_indices.top();
		free_indices.pop();

		used_indices.push_back(reinterpret_cast<E*>(entities) + index);
		used_indices_map.insert({id, index});
		used_indices_map_reverse.insert({index, id});

		index_map.insert({id, index});

		return &entities[index];
	}

	void destroy(uint64_t id)
	{
		auto res = index_map.find(id);
		if (res == index_map.end())
			return;

		uint64_t index = res->second;

		entities[index].~E();
		memset(&entities[index], 0, sizeof(E));

		index_map.erase(id);

		uint64_t ui_index = used_indices_map[id];

		uint64_t replace_index = used_indices.size() - 1;
		used_indices[ui_index] = used_indices[replace_index];
		uint64_t replace_id = used_indices_map_reverse[replace_index];

		used_indices_map_reverse[ui_index] = replace_id;
		used_indices_map_reverse.erase(replace_index);
		used_indices_map[replace_id] = ui_index;
		used_indices_map.erase(id);

		used_indices.pop_back();

		free_indices.push(index);
	}

	uint64_t size()
	{
		return used_indices.size();
	}

// private:

	void expand(uint64_t by)
	{
		uint64_t size = used_indices.size();
		E* next = reinterpret_cast<E*>(malloc(sizeof (E) * (size + by)));

		memcpy(next, entities, sizeof (E) * size);
		free(entities);

		entities = next;

		uint64_t new_size = size + by;
		while(size < new_size)
		{
			free_indices.push(size);
			size++;
		}
	}

	uint64_t nextId;
	E* entities;
	std::vector<E*> used_indices;

	std::priority_queue<uint64_t, std::vector<uint64_t>, std::greater<uint64_t>> free_indices;
	std::unordered_map<uint64_t, uint64_t> index_map;

	std::unordered_map<uint64_t, uint64_t> used_indices_map;
	std::unordered_map<uint64_t, uint64_t> used_indices_map_reverse;
};

// #####################
// stubs for templates
// #####################

struct TemplateStubs
{
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

template <typename Value, bool Decision, typename Inherit>
struct ConditionalValue : Inherit { };

template <typename Value, typename Inherit>
struct ConditionalValue <Value, true, Inherit> : Inherit
{
	template <typename T>
	const T* viewComponent()
	{
		return getComponent<T>();
	}

	template<typename T>
	T* getComponent()
	{
		CZSS_CONST_IF (std::is_same<T, Value>())
			return reinterpret_cast<T*>(&value);

		return Inherit::template getComponent<T>();
	}

private:
	Value value;
};

template <typename Value>
struct ConditionalValue <Value, true, Dummy>
{
	template <typename T>
	const T* viewComponent()
	{
		return getComponent<T>();
	}

	template<typename T>
	T* getComponent()
	{
		CZSS_CONST_IF (std::is_same<T, Value>())
			return reinterpret_cast<T*>(&value);

		return nullptr;
	}

private:
	Value value;
};

template <typename Value>
struct ConditionalValue <Value, false, Dummy>
{
	template <typename T>
	const T* viewComponent()
	{
		return nullptr;
	}

	template<typename T>
	T* getComponent()
	{
		return nullptr;
	}
};

template <typename Base, typename Value, typename ...Rest>
struct ComponentInheritor
	: ConditionalValue <
		Value,
		!inspect::contains<Base, Value>(),
		ComponentInheritor<Container<Base, NrContainer<Value>>, Rest...>
	>
{ };

template <typename Base, typename Value>
struct ComponentInheritor<Base, Value>
	: ConditionalValue <
		Value,
		!inspect::contains<Base, Value>(),
		Dummy
	>
{ };

template <typename ...Components>
struct Iterator : Container<Components...>, IteratorBase, TemplateStubs
{
	using Cont = Container<Components...>;

	void setGuid(Guid guid) { this->id = guid; }
	Guid getGuid() { return id; }

private:
	Guid id;
};

struct VirtualArchitecture;

template <typename ...Components>
struct Entity : ComponentInheritor<Dummy, Components...>, Container<Components...>, EntityBase
{
	using Cont = Container<Components...>;

	Guid getGuid() { return {id}; }
private:
	void setGuid(Guid guid) { this->id = guid.get(); }
	friend VirtualArchitecture;
	uint64_t id;
};

template <typename Iterator, typename Entity>
constexpr static bool isIteratorCompatibleWithEntity()
{
	return isIterator<Iterator>() && isEntity<Entity>()
		? inspect::containsAllIn<typename Entity::Cont, typename Iterator::Cont>()
		: false;
}

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

template <typename Arch>
struct RunTaskData
{
	Arch* arch;
	czsf::Barrier* barriers;
	uint64_t id;
};

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

		Cont::template evaluate<OncePerType<Dummy, ConstructorCallback>>(this);
	}

	static constexpr uint64_t numSystems() { return inspect::numUniques<Cont, SystemBase>(); }
	static constexpr uint64_t numEntities() { return inspect::numUniques<Cont, EntityBase>(); }
	static constexpr uint64_t numComponents() { return inspect::numUniques<Cont, ComponentBase>(); }

	template <typename Resource>
	void setResource(Resource* res)
	{
		static_assert(isResource<Resource>(), "Template parameter must be a Resource.");
		static_assert(inspect::contains<Cont, Resource>(), "Architecture doesn't contain Resource.");
		static const uint64_t index = inspect::indexOf<Container<Systems...>, Resource, ResourceBase>();
		resources[index] = res;
	}

	template <typename Resource>
	Resource* getResource()
	{
		// static_assert(isResource<Resource>(), "Template parameter must be a Resource.");
		static_assert(inspect::contains<Cont, Resource>(), "Architecture doesn't contain Resource.");
		static const uint64_t index = inspect::indexOf<Container<Systems...>, Resource, ResourceBase>();
		return reinterpret_cast<Resource*>(resources[index]);
	}

	template <typename Entity>
	EntityStore<Entity>* getEntities()
	{
		// assert(isEntity<Entity>());
		static const uint64_t index = inspect::indexOf<Cont, Entity, EntityBase>();
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
		Guid guid = ent->getGuid();
		new(ent) Entity();
		setEntityId(ent, guid.get());
		return ent;
	}

	template <typename Entity, typename A>
	Entity* createEntity(A a)
	{
		auto ent = initializeEntity<Entity>();
		Guid guid = ent->getGuid();
		new (ent) Entity(a);
		setEntityId(ent, guid.get());
		return ent;
	}

	template <typename Entity, typename A, typename B>
	Entity* createEntity(A a, B b)
	{
		auto ent = initializeEntity<Entity>();
		Guid guid = ent->getGuid();
		new (ent) Entity(a, b);
		setEntityId(ent, guid.get());
		return ent;
	}

	template <typename Entity, typename A, typename B, typename C>
	Entity* createEntity(A a, B b, C c)
	{
		auto ent = initializeEntity<Entity>();
		Guid guid = ent->getGuid();
		new (ent) Entity(a, b, c);
		setEntityId(ent, guid.get());
		return ent;
	}

	template <typename Entity, typename A, typename B, typename C, typename D>
	Entity* createEntity(A a, B b, C c, D d)
	{
		auto ent = initializeEntity<Entity>();
		Guid guid = ent->getGuid();
		new (ent) Entity(a, b, c, d);
		setEntityId(ent, guid.get());
		return ent;
	}

	template <typename Entity, typename A, typename B, typename C, typename D, typename E>
	Entity* createEntity(A a, B b, C c, D d, E e)
	{
		auto ent = initializeEntity<Entity>();
		Guid guid = ent->getGuid();
		new (ent) Entity(a, b, c, d, e);
		setEntityId(ent, guid.get());
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
		entities->destroy(id);
	}

	void destroyEntity(Guid guid)
	{
		uint64_t tk = typeKey(guid);
		uint64_t id = guidId(guid);
		Switch<Cont, numEntities()>::template evaluate<OncePerType<Dummy, EntityDestructorCallback>>(tk, this, &tk, &id);
	}

	template <typename Entity>
	void destroyEntity(EntityId<Desc, Entity> key)
	{
		destroyEntity<Entity>(This::getEntityId(key));
	}

	template <typename Component, typename System>
	bool componentReallocated()
	{
		static const uint64_t index = inspect::indexOf<Cont, Component, ComponentBase>()
			* inspect::numUniques<Cont, SystemBase>()
			+ inspect::indexOf<Cont, System, SystemBase>();
		bool ret = reallocated[index];
		reallocated[index] = false;
		return ret;
	}

	template <typename T>
	void run(T* fls)
	{
		runForSystems(systemCallback, fls);
	}

	template <typename T>
	void initialize(T* fls)
	{
		runForSystems(initializeSystemCallback, fls);
	}

	template <typename T>
	void shutdown(T* fls)
	{
		runForSystems(shutdownSystemCallback, fls);
	}

	void run()
	{
		runForSystems<Dummy>(systemCallback, nullptr);
	}

	void initialize()
	{
		runForSystems<Dummy>(initializeSystemCallback, nullptr);
	}

	void shutdown()
	{
		runForSystems<Dummy>(shutdownSystemCallback, nullptr);
	}

	template <typename Sys>
	void run();

	static void systemCallback(RunTaskData<This>* data)
	{
		Switch<Cont, numSystems()>::template evaluate<SystemRunner>(data->id, &data->id, data->barriers, data->arch);
	}

	static void initializeSystemCallback(RunTaskData<This>* data)
	{
		Switch<Cont, numSystems()>::template evaluate<SystemInitialize>(data->id, &data->id, data->barriers, data->arch);
	}

	static void shutdownSystemCallback(RunTaskData<This>* data)
	{
		Switch<Cont, numSystems()>::template evaluate<SystemShutdown>(data->id, &data->id, data->barriers, data->arch);
	}

	static constexpr uint64_t typeKeyLength()
	{
		return ceil(log2(inspect::numUniques<Cont, EntityBase>()));
	}

	static uint64_t typeKey(Guid guid)
	{
		static const uint64_t klen = typeKeyLength();
		uint64_t mask = ((uint64_t(1) << (klen + 1)) - 1) << (63 - klen);
		return (guid.get() & mask) >> (63 - klen);
	}
	static uint64_t guidId(Guid guid)
	{
		static const uint64_t klen = typeKeyLength();
		uint64_t key = typeKey(guid) << (63 - klen);

		return guid.get() - key;
	}

	~Architecture()
	{
		Cont::template evaluate<OncePerType<Dummy, DestructorCallback>>(this);
	}

private:
	void* resources[max(inspect::numUniques<Cont, ResourceBase>(), uint64_t(1))] = {0};
	char entities[max(inspect::numUniques<Cont, EntityBase>(), uint64_t(1)) * sizeof(EntityStore<uint64_t>)] = {0};
	bool reallocated[max(inspect::numUniques<Cont, ComponentBase>() * inspect::numUniques<Cont, SystemBase>(), uint64_t(1))] = {0};

	template <typename T>
	void runForSystems(void (*fn)(RunTaskData<This>*), T* fls)
	{
		static const uint64_t sysCount = inspect::numUniques<Container<Systems...>, SystemBase>();
		czsf::Barrier barriers[sysCount];
		RunTaskData<This> taskData[sysCount];

		for (uint64_t i = 0; i < sysCount; i++)
		{
			barriers[i] = czsf::Barrier(1);
			taskData[i].arch = this;
			taskData[i].barriers = barriers;
			taskData[i].id = i;
		}

		czsf::Barrier wait(sysCount);
		if (fls == nullptr)
			czsf::run(fn, taskData, sysCount, &wait);
		else
			czsf::run(fls, fn, taskData, sysCount, &wait);
		wait.wait();
	}

	template <typename Entity>
	struct PointerFixupData
	{
		This* arch;
		Entity* oldLoc;
		Entity* oldMaxLoc;
	};

	template <typename Entity>
	Entity* initializeEntity()
	{
		static_assert(isEntity<Entity>(), "Template parameter must be an Entity.");

		auto entities = getEntities<Entity>();
		Entity* loc = entities->entities;
		uint64_t count = entities->size();

		uint64_t id;
		auto ent = entities->create(id);
		setEntityId(ent, id);



		if (loc != entities->entities)
		{
			PointerFixupData<Entity> data = {this, loc, loc + count};
			Cont::template evaluate<OncePerType<Dummy, ManagedPointerCallback<Entity>>>(&data);
		}

		return ent;
	}

	template <typename Entity>
	struct ManagedPointerCallback
	{
		template <typename Value>
		static inline void callback(PointerFixupData<Entity>* data)
		{
			CZSS_CONST_IF (isEntity<Value>())
			{
				auto entities = data->arch->template getEntities<Value>();
				for (auto ptr : entities->used_indices)
					Cont::template evaluate<OncePerType<Dummy, EntityComponentManagedPointerCallback<Entity, Value>>>(data, ptr);
			}

			CZSS_CONST_IF (isResource<Value>())
			{
				auto resource = data->arch->template getResource<Value>();
				if (resource != nullptr)
				{
					Entity* loc = data->arch->template getEntities<Entity>()->entities;
					Entity* min = data->oldLoc;
					Entity* max = data->oldMaxLoc;
					Repair<Entity> repair(min, max, loc - min);
					managePointer(*resource, repair);
					Cont::template evaluate<OncePerType<Dummy, ComponentManagedPointerCallback<Entity, Value>>>(data, resource);
				}
			}
		}
	};

	template <typename Entity, typename IterateEntity>
	struct EntityComponentManagedPointerCallback
	{
		template <typename Value>
		static inline void callback(PointerFixupData<Entity>* data, IterateEntity* ptr)
		{
			CZSS_CONST_IF (isComponent<Value>() && inspect::contains<typename IterateEntity::Cont, Value>())
			{
				Cont::template evaluate<OncePerType<Dummy, ManagedComponentPointerFixupCallback<Entity, Value>>>(data, ptr->template getComponent<Value>());
			}
		}
	};

	template <typename Entity, typename Update>
	struct ComponentManagedPointerCallback
	{
		template <typename Value>
		static inline void callback(PointerFixupData<Entity>* data, Update* ptr)
		{
			CZSS_CONST_IF (isComponent<Value>())
			{
				Cont::template evaluate<OncePerType<Dummy, ManagedComponentPointerFixupCallback<Entity, Update>>>(data, ptr);
			}
		}
	};

	template <typename Entity, typename Update>
	struct ManagedComponentPointerFixupCallback
	{
		template <typename Value>
		static inline void callback(PointerFixupData<Entity>* data, Update* ptr)
		{
			CZSS_CONST_IF (isComponent<Value>() && inspect::contains<typename Entity::Cont, Value>())
			{
				Value* min = data->oldLoc->template getComponent<Value>();
				Value* max = reinterpret_cast<Value*>(data->oldMaxLoc);
				Entity* loc = data->arch->template getEntities<Entity>()->entities;
				Repair<Value> repair(min, max, loc->template getComponent<Value>() - min);
				managePointer(*ptr, repair);
			}
		}
	};

	struct SystemRunner
	{
		template <typename Base, typename Box, typename Value, typename Inner, typename Next>
		inline static void inspect(uint64_t* id, czsf::Barrier* barriers, This* arch);
	};

	struct SystemInitialize
	{
		template <typename Base, typename Box, typename Value, typename Inner, typename Next>
		inline static void inspect(uint64_t* id, czsf::Barrier* barriers, This* arch);
	};

	struct SystemShutdown
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
			CZSS_CONST_IF (Sys::Dep::template directlyDependsOn<Value>() && !Sys::Dep::template transitivelyDependsOn<Value>())
			{
				barriers[inspect::indexOf<Cont, Value, SystemBase>()].wait();
			}

			Next::template evaluate<SystemBlocker<Sys>>(barriers);
		}
	};

	template <typename Sys>
	struct DependeeBlocker
	{
		template <typename Base, typename Box, typename Value, typename Inner, typename Next>
		inline static void inspect(czsf::Barrier* barriers)
		{
			CZSS_CONST_IF (Value::Dep::template directlyDependsOn<Sys>() && !Value::Dep::template transitivelyDependsOn<Sys>())
			{
				barriers[inspect::indexOf<Cont, Value, SystemBase>()].wait();
			}

			Next::template evaluate<DependeeBlocker<Sys>>(barriers);
		}
	};

	struct EntityDestructorCallback
	{
		template <typename Value>
		inline static void callback(This* arch, uint64_t* typeKey, uint64_t* id)
		{
			if(isEntity<Value>() && inspect::contains<Cont, Value>() && inspect::indexOf<Cont, Value, EntityBase>() == *typeKey)
				arch->template destroyEntity<Value>(*id);
		}
	};

	struct ConstructorCallback
	{
		template <typename Value>
		static inline void callback(This* arch)
		{
			CZSS_CONST_IF (isEntity<Value>())
			{
				auto p = arch->template getEntities<Value>();
				*p = EntityStore<Value>();
			}
		}
	};

	struct DestructorCallback
	{
		template <typename Value>
		static inline void callback(This* arch)
		{
			CZSS_CONST_IF (isEntity<Value>())
			{
				auto ent = arch->template getEntities<Value>();
				ent->~EntityStore<Value>();
			}
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
	IteratorAccessor(const IteratorAccessor<Iter, Arch, Sys>& other)
	{
		this->typeKey = other.typeKey;
		this->entity = other.entity;
	}

	template<typename Component>
	const Component* view()
	{
		static_assert(inspect::contains<typename Iter::Cont, Component>(), "Iterator doesn't contain the requested component.");
		static_assert(Sys::template canRead<Component>(), "System lacks read permissions for the Iterator's components.");
		return get_ptr<Component>();
	}

	template<typename Component>
	Component* get()
	{
		static_assert(Iter::template containsComponent<Component>(), "Iterator doesn't contain the requested component.");
		static_assert(Sys::template canWrite<Component>(), "System lacks write permissions for the Iterator's components.");
		return get_ptr<Component>();
	}

	Guid guid()
	{
		// TODO
		// return iter.getGuid();
		return Guid();
	}

private:
	friend IteratorIterator<Iter, Arch, Sys>;
	friend Accessor<Arch, Sys>;

	IteratorAccessor() {}

	template <typename Entity>
	IteratorAccessor(Entity* ent)
	{
		entity = ent;
		typeKey = inspect::indexOf<typename Arch::Cont, Entity, EntityBase>();
	}

	uint64_t typeKey;
	void* entity;

	struct Data
	{
		uint64_t typeKey;
		void* entity;
	};

	template <typename Component>
	Component* get_ptr()
	{
		Data data = {typeKey, entity};
		return Switch<typename Arch::Cont, inspect::numUniques<typename Arch::Cont, EntityBase>()>
			::template evaluate<Component*, Getter<Component>>(typeKey, &data);
	}

	template <typename Component>
	struct Getter
	{
		template <typename Base, typename This, typename Value, typename Inner, typename Next>
		inline static Component* inspect(Data* data)
		{
			if (isEntity<Value>() && inspect::indexOf<typename Arch::Cont, Value, EntityBase>() == data->typeKey)
			{
				return reinterpret_cast<Value*>(data->entity)->template getComponent<Component>();
			}
			Component* res = Inner::template evaluate<Component*, Getter<Component>>(data);
			if (res != nullptr)
				return res;
			return Next:: template evaluate<Component*, Getter<Component>>(data);
		}
	};
};

// template <typename Iter, typename Arch, typename Sys>
// struct IteratorIterator
// {
// 	using This = IteratorIterator<Iter, Arch, Sys>;
// 	using U = IteratorAccessor<Iter, Arch, Sys>;

// 	typedef uint64_t difference_type;
// 	typedef U value_type;
// 	typedef U& reference;
// 	typedef U* pointer;
// 	typedef std::forward_iterator_tag iterator_category;

// 	IteratorIterator()
// 	{
// 		arch = nullptr;
// 		iterator = {};
// 	}

// 	IteratorIterator(const IteratorIterator& other)
// 	{
// 		arch = other.arch;
// 		typeKey = other.typeKey;
// 		iterator = other.iterator;
// 		hasValue = other.hasValue;
// 		inner = other.inner;
// 	}

// 	U& operator* ()
// 	{
// 		return inner;
// 	}

// 	U* operator-> ()
// 	{
// 		return inner;
// 	}

// 	friend bool operator== (const This& a, const This& b)
// 	{
// 		return a.iterator == b.iterator;
// 	}

// 	friend bool operator!= (const This& a, const This& b)
// 	{
// 		return a.iterator != b.iterator;
// 	};

// 	This operator++(int)
// 	{
// 		This ret = *this;
// 		++(*this);
// 		return ret;
// 	}
	
// 	This& operator++()
// 	{
// 		Switch<typename Arch::Cont, Arch::numEntities()>::template evaluate<OncePerType<Dummy, IncrementerCallback>>(typeKey, this);

// 		while (!hasValue && typeKey < Arch::numEntities())
// 		{
// 			typeKey = Switch<typename Arch::Cont, Arch::numEntities()>::template evaluate<uint64_t, NextKey>(typeKey, typeKey);
// 			if (typeKey < Arch::numEntities())
// 			{
// 				Switch<typename Arch::Cont, Arch::numEntities()>::template evaluate<OncePerType<Dummy, IncrementerCallback>>(typeKey, this);
// 			}
// 		}

// 		return *this;
// 	}

// private:
// 	friend IterableStub<Iter, Arch, Sys>;

// 	IteratorIterator(Arch* arch)
// 	{
// 		this->arch = arch;
// 		typeKey = 0;
// 		hasValue = false;
// 		++(*this);
// 	}

// 	Arch* arch;
// 	uint64_t typeKey;
// 	typename std::unordered_map<uint64_t, Padding<uint64_t>>::iterator iterator;
// 	bool hasValue;
// 	U inner;

// 	struct NextKey
// 	{
// 		template <typename Base, typename Box, typename Value, typename Inner, typename Next>
// 		static constexpr uint64_t inspect(uint64_t key)
// 		{
// 			return min(
// 				min((isDummy<Inner>() ? -1 : Inner::template evaluate<uint64_t, NextKey>(key)), (isDummy<Next>() ? -1 : Next::template evaluate<uint64_t, NextKey>(key))),
// 				inspect::indexOf<typename Arch::Cont, Value, EntityBase>() > key ? inspect::indexOf<typename Arch::Cont, Value, EntityBase>() : -1
// 			);
// 		};
// 	};

// 	struct IncrementerCallback
// 	{
// 		template <typename Value>
// 		static inline void callback(This* iterac)
// 		{
// 			if (isEntity<Value>() && Value::template isCompatible<Iter>() && inspect::indexOf<typename Arch::Cont, Value, EntityBase>() == iterac->typeKey)
// 			{
// 				auto p = reinterpret_cast<typename std::unordered_map<uint64_t, Padding<Value>>::iterator*>(&iterac->iterator);

// 				if (!iterac->hasValue)
// 				{
// 					*p = iterac->arch->template getEntities<Value>()->map.begin();
// 					iterac->hasValue = true;
// 				}
// 				else
// 				{
// 					(*p)++;
// 				}

// 				if (*p == iterac->arch->template getEntities<Value>()->map.end())
// 				{
// 					iterac->hasValue = false;
// 				}

// 				if (iterac->hasValue)
// 				{
// 					iterac->inner = U(reinterpret_cast<Value*>(&(**p).second));
// 				}
// 			}
// 		}
// 	};
// };

// template <typename Iter, typename Arch, typename Sys>
// struct IterableStub
// {
// 	IteratorIterator<Iter, Arch, Sys> begin()
// 	{
// 		return IteratorIterator<Iter, Arch, Sys>(arch);
// 	}

// 	IteratorIterator<Iter, Arch, Sys> end()
// 	{
// 		return IteratorIterator<Iter, Arch, Sys>();
// 	}

// private:
// 	IterableStub(Arch* arch)
// 	{
// 		this ->arch = arch;
// 	}
// 	friend Accessor<Arch, Sys>;
// 	Arch* arch;
// };

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

	// template <typename Iterator>
	// IterableStub<Iterator, Arch, Sys> iterate()
	// {
	// 	iteratorPermission<Iterator>();
	// 	return IterableStub<Iterator, Arch, Sys>(arch);
	// }

	template <typename Iterator, typename F>
	void iterate(F f)
	{
		iteratorPermission<Iterator>();
		Arch::Cont::template evaluate<OncePerType<Dummy, IteratorCallback<Iterator, F>>>(f, arch);
	}

	template <typename Iterator, typename F>
	void parallelIterate(uint64_t numTasks, F f)
	{
		iteratorPermission<Iterator>();
		uint64_t counter = countCompatibleEntities<Iterator>();

		ParallelIterateTaskData<F> tasks[numTasks];

		for (uint64_t i = 0; i < numTasks; i++)
		{
			tasks[i].index = i;
			tasks[i].arch = arch;
			tasks[i].func = &f;
			tasks[i].entityCount = min(counter, max(uint64_t(1), counter / (numTasks - i)));
			counter -= tasks[i].entityCount;

			if (i > 0)
			{
				tasks[i].beginIndex = tasks[i - 1].beginIndex + tasks[i - 1].entityCount;
			}

			if (counter == 0)
			{
				numTasks = i + 1;
				break;
			}
		}

		czsf::Barrier barrier(numTasks);
		czsf::run(ParallelIterateTask<Iterator, F>, tasks, numTasks, &barrier);
		barrier.wait();
	}

	template <typename Iterator>
	uint64_t countCompatibleEntities()
	{
		uint64_t entityCount = 0;
		Arch::Cont::template evaluate<OncePerType<Dummy, EntityCountCallback<Iterator>>>(&entityCount, arch);
		return entityCount;
	}

	template <typename Iterator>
	static constexpr uint64_t numCompatibleEntities()
	{
		return Arch::Cont::template evaluate<uint64_t, NumCompatibleEntities<Dummy, Iterator>>();
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

	template <typename Component>
	bool componentReallocated()
	{
		return arch->template componentReallocated<Component, Sys>();
	}


// private:
	// friend Arch;
	Accessor(Arch* arch) { this->arch = arch; }
	Arch* arch;

private:
	template <typename Iterator>
	void iteratorPermission()
	{
		static_assert(isIterator<Iterator>(), "Attempted to iterate non-iterator.");
		static_assert(inspect::containsAllIn<Sys, Iterator>(), "System doesn't have access to all components of Iterator.");
		static_assert(inspect::contains<typename Arch::Cont, Iterator>(), "Architecture doesn't contain the Iterator.");
	}

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

	template <typename Iterator, typename F>
	struct IteratorCallback
	{
		template <typename Value>
		static inline void callback(F f, Arch* arch)
		{
			CZSS_CONST_IF (isEntity<Value>() && isIteratorCompatibleWithEntity<Iterator, Value>())
			{
				auto ents = arch->template getEntities<Value>();
				for (auto& ent : ents->used_indices)
				{
					IteratorAccessor<Iterator, Arch, Sys> accessor(ent);
					f(accessor);
				}
			}
		}
	};

	template <typename Iterator>
	struct EntityCountCallback
	{
		template <typename Value>
		static inline void callback(uint64_t* count, Arch* arch)
		{
			CZSS_CONST_IF (isEntity<Value>() && isIteratorCompatibleWithEntity<Iterator, Value>())
			{
				auto entities = arch->template getEntities<Value>();
				*count += entities->size();
			}
		}
	};

	template <typename Handled, typename Iterator>
	struct NumCompatibleEntities
	{
		template <typename Base, typename This, typename Value, typename Inner, typename Next>
		static constexpr uint64_t inspect()
		{
			return (isEntity<Value>()
				&& isIteratorCompatibleWithEntity<Iterator, Value>()
				&& !inspect::contains<Next, Value>()
				&& !inspect::contains<Handled, Value>()
				? 1 : 0)
				+ Inner::template evaluate<uint64_t, NumCompatibleEntities<Container <Handled, NrContainer<Value>, Next>, Iterator>>()
				+ Next::template evaluate<uint64_t, NumCompatibleEntities<Dummy, Iterator>>();
		}
	};

	template <typename F>
	struct ParallelIterateTaskData
	{
		uint64_t index = 0;
		uint64_t beginIndex = 0;
		uint64_t entityCount = 0;
		Arch* arch;
		F* func;
	};

	template <typename Iterator, typename F>
	struct ParallelIterateTaskCallback
	{
		template <typename Value>
		static inline void callback(ParallelIterateTaskData<F>* data)
		{
			CZSS_CONST_IF (!isEntity<Value>() || !isIteratorCompatibleWithEntity<Iterator, Value>())
				return;

			auto entities = data->arch->template getEntities<Value>();
			if (entities->size() == 0 || entities->size() < data->beginIndex)
			{
				data->beginIndex -= entities->size();
				return;
			}

			auto it = entities->used_indices.begin();
			it += data->beginIndex;

			auto end = entities->used_indices.end();
			uint64_t handled;

			if (data->entityCount < end - it)
			{
				handled = data->entityCount;
				end = it + data->entityCount;
			} else {
				handled = end - it;
			}

			for (; it != end; it++)
			{
				IteratorAccessor<Iterator, Arch, Sys> accessor(*it);
				(*data->func)(data->index, accessor);
			}


			data->entityCount -= handled;
			data->beginIndex = 0;
		}
	};


	template <typename Iterator, typename F>
	static void ParallelIterateTask(ParallelIterateTaskData<F>* data)
	{
		static const uint64_t limit = Accessor<Arch, Sys>::numCompatibleEntities<Iterator>();

		F lambda = *data->func;
		for (uint64_t i = 0; i < limit; i++)
		{
			data->func = &lambda;
			Switch<typename Arch::Cont, limit>::template evaluate<OncePerType<Dummy, ParallelIterateTaskCallback<Iterator, F>>>(i, data);
			if (data->entityCount == 0)
				return;
		}
	}
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

template <typename T>
constexpr bool isDummy()
{
	return std::is_same<T, Dummy>();
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

template <typename Desc, typename ...Systems>
template <typename Base, typename Box, typename Value, typename Inner, typename Next>
void Architecture<Desc, Systems...>::SystemInitialize::inspect(uint64_t* id, czsf::Barrier* barriers, This* arch)
{
	if (inspect::indexOf<Cont, Value, SystemBase>() != *id)
	{
		Next::template evaluate<SystemInitialize>(id, barriers, arch);
		return;
	}

	Cont::template evaluate<SystemBlocker<Value>>(barriers);
	Value::initialize(Accessor<Desc, Value>(reinterpret_cast<Desc*>(arch)));
	barriers[*id].signal();
}

template <typename Desc, typename ...Systems>
template <typename Base, typename Box, typename Value, typename Inner, typename Next>
void Architecture<Desc, Systems...>::SystemShutdown::inspect(uint64_t* id, czsf::Barrier* barriers, This* arch)
{
	if (inspect::indexOf<Cont, Value, SystemBase>() != *id)
	{
		Next::template evaluate<SystemShutdown>(id, barriers, arch);
		return;
	}

	Cont::template evaluate<DependeeBlocker<Value>>(barriers);
	Value::shutdown(Accessor<Desc, Value>(reinterpret_cast<Desc*>(arch)));
	barriers[*id].signal();
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

} // namespace czss

#endif	// CZSS_IMPLEMENTATION_GUARD_

#endif // CZSS_IMPLEMENTATION