#ifndef CZSS_HEADERS_H
#define CZSS_HEADERS_H

#include <tuple_utils.hpp>

#ifndef CZSF_HEADERS_H
#include <czsf.h>
#endif

#include <cmath>
#include <cstdint>
#include <cstring>
#include <queue>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <functional>

/* Timing functions
	#define CZSS_TIMING_BEGIN timing_begin_function
	#define CZSS_TIMING_END timing_end_function

	where timing_begin/end_function definition is as follows:

	template <typename Arch, typename System>
	void timing_begin/end_function(Arch* arch);

	Timing functions are called before and after system::run.
*/

namespace czss
{

#if __cplusplus < 201703L
#define CZSS_CONST_IF if
#else
#define CZSS_CONST_IF if constexpr
#endif

#define CZSS_NAME(A, B) template <> const char* czss::name<A>() { const static char name[] = B; return name; }

template <typename Value>
const static char* name()
{
	const static char name[] = "Unknown";
	return name;
}

template <typename Base>
struct BaseTypeFilter
{
	template <typename T>
	static constexpr bool test()
	{
		return std::is_base_of<Base, T>::value;
	}
};

template<typename Tuple, typename Cat>
using Filter = tuple_utils::Subset<Tuple, BaseTypeFilter<Cat>>;

struct Dummy
{
	using Cont = std::tuple<>;
};

template <typename ...A>
using Rbox = tuple_utils::Set<A...>;

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

template <typename Tuple, typename T>
constexpr bool contains()
{
	return tuple_utils::Contains<Tuple, T>::value;
}

template<typename Left, typename Right>
constexpr bool containsAllIn()
{
	return std::tuple_size< tuple_utils::Difference<Right, Left> >::value == 0;
}

} // namespace inspect

template <typename Tuple, typename Callback>
struct OncePerTypeCallback
{
	template <typename Value1>
	struct CB
	{
		template <typename Value2, typename ...Params>
		static void callback(Params&&... params)
		{
			Callback::template callback<Value1, Value2>(std::forward<Params>(params)...);
		}
	};

	template <typename Value, typename ...Params>
	static void callback(Params&&... params)
	{
		for (int i = 0; i < std::tuple_size<Tuple>::value; i++)
			tuple_utils::Switch<Tuple>::template fn<CB<Value>>(i, std::forward<Params>(params)...);
	}
};

template <typename Tuple, typename Callback, typename ...Params>
void oncePerPair(Params&&... params)
{
	for (int i = 0; i < std::tuple_size<Tuple>::value; i++)
		tuple_utils::Switch<Tuple>::template fn<OncePerTypeCallback<Tuple, Callback>>(i, std::forward<Params>(params)...);
}

// #####################
// Tags & Validation
// #####################

struct ThreadSafe {};
struct Virtual {};

struct ComponentBase {};
struct IteratorBase {};
struct EntityBase {};
struct SystemBase {};
struct ResourceBase {};
struct PermissionsBase {};
struct DependencyBase {};
struct ReaderBase {};
struct WriterBase {};
struct OrchestratorBase {};

template <typename T>
constexpr bool isValidType()
{
	return (1
		<< (std::is_base_of<ComponentBase, T>::value ? 1 : 0)
		<< (std::is_base_of<IteratorBase, T>::value ? 1 : 0)
		<< (std::is_base_of<EntityBase, T>::value ? 1 : 0)
		<< (std::is_base_of<SystemBase, T>::value ? 1 : 0)
		<< (std::is_base_of<ResourceBase, T>::value ? 1 : 0)
		<< (std::is_base_of<PermissionsBase, T>::value ? 1 : 0)
		<< (std::is_base_of<DependencyBase, T>::value ? 1 : 0)
		<< (std::is_base_of<ReaderBase, T>::value ? 1 : 0)
		<< (std::is_base_of<WriterBase, T>::value ? 1 : 0)
		<< (std::is_base_of<OrchestratorBase, T>::value ? 1 : 0)
	) == 2;
}

template<typename B, typename T>
constexpr bool isBaseType()
{
	return isValidType<T>() && std::is_base_of<B, T>::value;
}

template<typename T>
constexpr bool isThreadSafe()
{
	return isBaseType<ResourceBase, T>() && std::is_base_of<ThreadSafe, T>::value;
}

template<typename T>
constexpr bool isVirtual()
{
	return (isBaseType<EntityBase, T>() || isBaseType<ComponentBase, T>())
		&& std::is_base_of<Virtual, T>::value;
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

template<typename T>
constexpr bool isDependency()
{
	return isBaseType<DependencyBase, T>();
}

template<typename T>
constexpr bool isReader()
{
	return isBaseType<ReaderBase, T>();
}


template<typename T>
constexpr bool isWriter()
{
	return isBaseType<WriterBase, T>();
}


template<typename T>
constexpr bool isOrchestrator()
{
	return isBaseType<OrchestratorBase, T>();
}


template <typename T>
constexpr bool isDummy()
{
	return std::is_same<T, Dummy>();
}

template <typename Tuple, typename T, typename Cat>
constexpr uint64_t indexOf()
{
	return inspect::contains<Filter<Tuple, Cat>, T>() ? tuple_utils::Index<T, Filter<Tuple, Cat>>::value : -1;
}

template <typename T>
struct FlattenImpl2;

template <typename Tuple>
using Flatten = typename FlattenImpl2<Tuple>::type;

template <typename A, typename ...R>
struct FlattenImpl;

template <typename ...A, typename V, typename ...R>
struct FlattenImpl<std::tuple<A...>, V, R...>
{
	using type = typename FlattenImpl<tuple_utils::Union<std::tuple<A...>, std::tuple<V>, Flatten<typename V::Cont>>, R...>::type;
};

template <typename ...A, typename V>
struct FlattenImpl<std::tuple<A...>, V>
{
	using type = tuple_utils::Union<std::tuple<A...>, std::tuple<V>, Flatten<typename V::Cont>>;
};

template <typename ...A>
struct FlattenImpl2<std::tuple<A...>>
{
	using type = typename FlattenImpl<std::tuple<>, A...>::type;
};

template <>
struct FlattenImpl2<std::tuple<>>
{
	using type = std::tuple<>;
};

template <typename A, typename ...R>
struct ExpandImpl;

template <typename ...A, typename V, typename ...R>
struct ExpandImpl<std::tuple<A...>, V, R...>
{
	using type = typename ExpandImpl<tuple_utils::Union<typename V::Cont, std::tuple<A...>>, R...>::type;
};

template <typename ...A, typename V>
struct ExpandImpl<std::tuple<A...>, V>
{
	using type = tuple_utils::Union<typename V::Cont, std::tuple<A...>>;
};

template <typename A>
struct ExpandImpl2;

template <typename ...A>
struct ExpandImpl2<std::tuple<A...>>
{
	using type = typename ExpandImpl<std::tuple<>, A...>::type;
};

template <>
struct ExpandImpl2<std::tuple<>>
{
	using type = std::tuple<>;
};

template <typename Tuple>
using Expand = typename ExpandImpl2<Tuple>::type;

template <template<typename> typename W, typename A, typename ...R>
struct RewrapElementsImpl;

template <template<typename> typename W, typename ...A, typename V, typename ...R>
struct RewrapElementsImpl<W, std::tuple<A...>, V, R...>
{
	using type = typename RewrapElementsImpl<W, std::tuple<typename W<V>::type, A...>, R...>::type;
};

template <template<typename> typename W, typename ...A, typename V>
struct RewrapElementsImpl<W, std::tuple<A...>, V>
{
	using type = tuple_utils::Set<typename W<V>::type, A...>;
};

template <template<typename> typename W, typename A>
struct RewrapElementsImpl2;

template <template<typename> typename W, typename ...A>
struct RewrapElementsImpl2<W, std::tuple<A...>>
{
	using type = typename RewrapElementsImpl<W, std::tuple<>, A...>::type;
};

template <template<typename> typename W>
struct RewrapElementsImpl2<W, std::tuple<>>
{
	using type = std::tuple<>;
};

template <template<typename> typename W, typename Tuple>
using RewrapElements = typename RewrapElementsImpl2<W, Tuple>::type;


template <typename Tuple, typename Cat>
constexpr size_t numUniques()
{
	return std::tuple_size<Filter<Tuple, Cat>>::value;
}

// #####################
// Data Rbox
// #####################

struct Guid
{
	Guid(const Guid& guid);
	Guid();
	Guid(uint64_t id);
	uint64_t get() const;
	bool operator ==(const Guid& other) const;
	bool operator !=(const Guid& other) const;
	bool operator <(const Guid& other) const;
private:
	uint64_t id;
};

template <typename E>
struct EntityStore
{
	using type = EntityStore<E>;

	EntityStore()
	{
		CZSS_CONST_IF (!isVirtual<E>())
			expand();
	}

	E* get(uint64_t id)
	{
		auto res = used_indices_map.find(id);
		if (res == used_indices_map.end())
			return nullptr;
		return used_indices[res->second];
	}

private:
	static constexpr size_t BASE_POWER = 5;

	struct Index
	{
		bool operator< (const Index& other) const
		{
			return tier == other.tier ? index < other.index : tier < other.tier;
		}

		size_t tier;
		size_t index;
	};

	E* get(const Index& index)
	{
		return &entities[index.tier][index.index];
	}

	inline size_t indexToActiveI(const Index& index) const
	{
		// The arrays are powers of two in size so offset is calculated
		// by setting all bits preceding the power of the current tier
		// to 1 and masking off the section that is below base power
		// i.e. arrays that don't exist.
		// e.g. BASE_POWER = 2, index.tier = 1
		// 00000010 2
		// 00010000 bit shift
		// 00001111 subtract
		// 00001000 and
		static constexpr size_t maskL = ~size_t(0) << (BASE_POWER + 1);
		size_t offset = ((2 << (index.tier + BASE_POWER)) - 1) & maskL;
		return offset + index.index;
	}

public:
	inline bool isActive(const size_t& i) const
	{
		size_t mod = i % (sizeof(size_t) * 8);
		return active[i / (sizeof(size_t) * 8)] & (1 << mod);
	}

	inline void setActive(const size_t& i, const bool& value)
	{
		size_t mod = i % (sizeof(size_t) * 8);
		if (value)
			active[i / (sizeof(size_t) * 8)] |= (1 << mod);
		else
			active[i / (sizeof(size_t) * 8)] &= ~(1 << mod);
	}

	inline bool isActive(const Index& index) const
	{
		return isActive(indexToActiveI(index));
	}

	inline void setActive(const Index& index, const bool& value)
	{
		setActive(indexToActiveI(index), value);
	}

	template <typename T, typename ...Params>
	T* create(uint64_t& id, Params&&... params)
	{
		id = nextId++;

		T* res;
		auto used_indices_index = used_indices.size();

		CZSS_CONST_IF (isVirtual<T>())
		{
			res = new T(std::forward<Params>(params)...);
		}
		else
		{
			if(free_indices.size() == 0)
				expand();

			auto index = free_indices.top();
			free_indices.pop();
			res = get(index);
			new(res) T(std::forward<Params>(params)...);
			index_map.insert({id, index});
		}

		used_indices.push_back(res);
		used_indices_map.insert({id, used_indices_index});
		used_indices_map_reverse.insert({used_indices_index, id});

		return res;
	}

	void destroy(uint64_t id)
	{
		CZSS_CONST_IF (isVirtual<E>())
		{
			auto res = used_indices_map.find(id);
			if (res == used_indices_map.end())
				return;

			delete(used_indices[res->second]);
		}
		else
		{
			auto res = index_map.find(id);
			if (res == index_map.end())
				return;

			auto index = res->second;
			auto p = get(index);
			p->~E();
			memset(reinterpret_cast<void*>(p), 0, sizeof(E));
			free_indices.push(index);
			index_map.erase(id);
		}

		auto ui_index = used_indices_map[id];

		auto replace_index = used_indices.size() - 1;
		used_indices[ui_index] = used_indices[replace_index];
		auto replace_id = used_indices_map_reverse[replace_index];

		used_indices_map_reverse[ui_index] = replace_id;
		used_indices_map_reverse.erase(replace_index);
		used_indices_map[replace_id] = ui_index;
		used_indices_map.erase(id);

		used_indices.pop_back();
	}

	uint64_t size() const
	{
		return used_indices.size();
	}

	void clear()
	{
		index_map.clear();
		used_indices_map.clear();
		used_indices_map_reverse.clear();
		free_indices.clear();

		callDestructors();

		for (size_t k = 0; k < tierCount; k++)
		{
			addArrayKeys(k);
			size_t n = 2 << (k + BASE_POWER);
		}
		CZSS_CONST_IF (!isVirtual<E>())
		{
			memset(entities, 0, sizeof(E) * free_indices.size());
		}

		used_indices.clear();
	}

	~EntityStore()
	{
		callDestructors();

		for (size_t i = 0; i < tierCount; i++)
			free(entities[i]);
	}

	std::vector<E*> used_indices;
	std::vector<size_t> active;

private:
	E* entities[26];

	// empty slots in entities Index
	std::priority_queue<Index, std::vector<Index>> free_indices;

	// maps entity id to entities array
	std::unordered_map<uint64_t, Index> index_map;

	// maps entity id to index in used_indices
	std::unordered_map<uint64_t, uint64_t> used_indices_map;

	// reverse of the above
	std::unordered_map<uint64_t, uint64_t> used_indices_map_reverse;

	uint64_t nextId = 1;
	size_t tierCount = 0;

	void expand()
	{
		uint64_t n = 2 << (tierCount + BASE_POWER);
		addArrayKeys(tierCount);
		E* p = reinterpret_cast<E*>(malloc(sizeof (E) * n));
		entities[tierCount] = p;
		tierCount++;
	}

	void callDestructors()
	{
		for (E* ptr : used_indices)
		{
			CZSS_CONST_IF (isVirtual<E>())
				delete(ptr);
			else
				ptr->~E();
		}
	}

	void addArrayKeys(size_t tier)
	{
		size_t n = 2 << (tier + BASE_POWER);
		for (size_t i = 0; i < n; i++)
			free_indices.push({tier, i});
	}
};

// #####################
// stubs for templates
// #####################

struct TemplateStubs
{
	template <typename T>
	T* getComponent() { return nullptr; }

	void setGuid(Guid guid);
	Guid getGuid() const;
};

// #####################
// Primitives
// #####################

template <typename C>
struct Component : Dummy, ComponentBase, TemplateStubs
{
	using Cont = std::tuple<>;

	friend C;
private:
	Component() {};
};

template<typename R>
struct Resource : Dummy, ResourceBase, TemplateStubs
{
	using Cont = std::tuple<>;

	friend R;
private:
	Resource() {};
};

// #####################
// Component collection types
// #####################

template <typename ...Components>
struct Iterator : IteratorBase, TemplateStubs
{
	using Cont = tuple_utils::Set<Components...>;

	void setGuid(Guid guid) { this->id = guid; }
	Guid getGuid() const { return id; }

private:
	Guid id;
};

struct VirtualArchitecture;

template <typename ...Components>
struct Entity : EntityBase
{
	using Cont = tuple_utils::Set<Components...>;

private:
	struct AbstractFilter
	{
		template <typename Component>
		static constexpr bool test()
		{
			return std::is_abstract<Component>::value;
		}
	};

	template <typename T>
	struct alignas(alignof(T)) AbstractPlaceholder
	{
		using type = AbstractPlaceholder<T>;
		AbstractPlaceholder()
		{
			static_assert(isVirtual<T>());
		}
		char data[sizeof (T)];
	};

	using Abstracts = tuple_utils::Subset<Cont, AbstractFilter>;
	using Concrete = tuple_utils::Difference<Cont, Abstracts>;
public:

	template <typename Component>
	static constexpr bool hasComponent()
	{
		return inspect::contains<Cont, Component>();
	}

	template <typename T>
	const T* viewComponent() const
	{
		CZSS_CONST_IF(tuple_utils::Contains<Cont, T>::value)
		{
			CZSS_CONST_IF(tuple_utils::Contains<Abstracts, T>::value)
			{
				return reinterpret_cast<T*>(&std::get<min(tuple_utils::Index<T, Abstracts>::value, std::tuple_size<Abstracts>::value - 1)>(abstracts).data);
			}
			else
			{
				return &std::get<min(tuple_utils::Index<T, Concrete>::value, std::tuple_size<Concrete>::value - 1)>(concrete);
			}
		}

		return nullptr;
	}

	template<typename T>
	T* getComponent()
	{
		CZSS_CONST_IF(tuple_utils::Contains<Cont, T>::value)
		{
			CZSS_CONST_IF(tuple_utils::Contains<Abstracts, T>::value)
			{
				return reinterpret_cast<T*>(&std::get<min(tuple_utils::Index<T, Abstracts>::value, std::tuple_size<Abstracts>::value - 1)>(abstracts).data);
			}
			else
			{
				return &std::get<min(tuple_utils::Index<T, Concrete>::value, std::tuple_size<Concrete>::value - 1)>(concrete);
			}
		}

		return nullptr;
	}

	template <typename B, typename D>
	B* setComponent(D&& d)
	{
		static_assert(isVirtual<B>());
		static_assert(tuple_utils::Contains<Cont, B>::value);
		static_assert(std::is_base_of<B, D>::value);
		static_assert(alignof(B) == alignof(D));
		static_assert(sizeof(B) == sizeof(D));
		auto ret = getComponent<B>();
		new(ret) D(std::move(d));
		return ret;
	}

	template <typename B, typename D>
	B* setComponent()
	{
		static_assert(isVirtual<B>());
		static_assert(tuple_utils::Contains<Cont, B>::value);
		static_assert(std::is_base_of<B, D>::value);
		static_assert(alignof(B) == alignof(D));
		static_assert(sizeof(B) == sizeof(D));
		auto ret = getComponent<B>();
		new(ret) D();
		return ret;
	}

	template <typename B, typename D, typename ...Params>
	B* setComponent(Params&&... params)
	{
		static_assert(isVirtual<B>());
		static_assert(tuple_utils::Contains<Cont, B>::value);
		static_assert(std::is_base_of<B, D>::value);
		static_assert(alignof(B) == alignof(D));
		static_assert(sizeof(B) == sizeof(D));
		auto ret = getComponent<B>();
		new(ret) D(std::forward(params)...);
		return ret;
	}


	Guid getGuid() const { return {id}; }
private:
	void setGuid(Guid guid) { this->id = guid.get(); }
	friend VirtualArchitecture;
	uint64_t id;

	RewrapElements<AbstractPlaceholder, Abstracts> abstracts;
	Concrete concrete;
};

template<typename Sys, typename Entity>
struct TypedEntityAccessor;

template <typename Iterator, typename Entity>
constexpr static bool isIteratorCompatibleWithEntity()
{
	using _iteratorComponents = Filter<Flatten<typename Iterator::Cont>, ComponentBase>;

	return isIterator<Iterator>()
		&& isEntity<Entity>()
		&& inspect::containsAllIn<Flatten<typename Entity::Cont>, _iteratorComponents>();
}

// #####################
// Permissions
// #####################

template <typename ...T>
struct Reader : TemplateStubs, ReaderBase, PermissionsBase
{
	using Cont = tuple_utils::Set<T...>;
};

template <typename ...T>
struct Writer : TemplateStubs, WriterBase, PermissionsBase
{
	using Cont = tuple_utils::Set<T...>;
};

template <typename ...Entities>
struct Orchestrator : TemplateStubs, OrchestratorBase, PermissionsBase
{
	using Cont = tuple_utils::Set<Entities...>;
};

template <typename Derived>
struct BaseTypeOfFilter
{
	template <typename T>
	static constexpr bool test()
	{
		return std::is_base_of<T, Derived>();
	}
};

template <typename Sys, typename T>
constexpr bool canOrchestrate()
{
	return isVirtual<T>()
		? std::tuple_size<
			tuple_utils::Subset<Flatten<Filter<typename Sys::Cont, OrchestratorBase>>, BaseTypeOfFilter<T>>
		>::value > 0
		: inspect::contains<Flatten<Filter<typename Sys::Cont, OrchestratorBase>>, T>();
}

template <typename Sys, typename T>
constexpr bool canWrite()
{
	return inspect::contains<Flatten<Filter<typename Sys::Cont, WriterBase>>, T>()
		|| canOrchestrate<Sys, T>();
}


template <typename Sys, typename T>
constexpr bool canRead()
{
	return inspect::contains<Flatten<Filter<typename Sys::Cont, ReaderBase>>, T>()
		|| canWrite<Sys, T>()
		|| canOrchestrate<Sys, T>();
}

// #####################
// Graphs
// #####################

template <typename ...N>
struct Dependency : DependencyBase, TemplateStubs
{
	using Cont = tuple_utils::Set<N...>;
};

template <>
struct Dependency<> : DependencyBase, TemplateStubs
{
	using Cont = std::tuple<>;
};

template <typename Left, typename Right>
constexpr static bool directlyDependsOn()
{
	return isSystem<Left>() && isSystem<Right>() && inspect::contains<Expand<Filter<typename Left::Cont, DependencyBase>>, Right>();
}

template <typename Left, typename Right>
constexpr static bool dependsOn()
{
	return isSystem<Left>() && isSystem<Right>() && inspect::contains<Flatten<typename Left::Cont>, Right>();
}

template <typename Left, typename Right>
constexpr static bool redundantDependency()
{
	return directlyDependsOn<Left, Right> () && inspect::contains<Flatten<Expand<Expand<Filter<typename Left::Cont, DependencyBase>>>>, Right>();
}

template <typename Left, typename Right>
constexpr static bool transitivelyDependsOn()
{
	return isSystem<Left>() && isSystem<Right>() && dependsOn<Left, Right>() && !directlyDependsOn<Left, Right>() && !redundantDependency<Left, Right>();
}

// #####################
// System
// #####################

template<typename Arch, typename Sys>
struct Accessor;

template <typename ...Permissions>
struct System : SystemBase, TemplateStubs
{
	using Cont = tuple_utils::Set<Permissions...>;
};

template <typename Sys>
using SystemAccesses = Flatten<tuple_utils::Difference<typename Sys::Cont, Filter<typename Sys::Cont, DependencyBase>>>;
// using SystemAccesses = Flatten<tuple_union<Filter<typename Sys::Cont, ReaderBase>, Filter<typename Sys::Cont, WriterBase>, Filter<typename Sys::Cont, OrchestratorBase>>>;

template <typename A, typename B>
constexpr bool exclusiveWith();

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
	static uint64_t getEntityId(const Entity* ent)
	{
		return ent->getGuid().get();
	}

	template <typename Arch, typename Entity>
	static EntityId<Arch, Entity> getEntityId(const Entity* ent)
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

template <typename Arch, typename Subset>
struct Runner
{

	template <typename Sys>
	struct SystemBlocker
	{
		template <typename Value>
		inline static void callback(czsf::Barrier* barriers)
		{
			CZSS_CONST_IF (directlyDependsOn<Sys, Value>() && !transitivelyDependsOn<Sys, Value>())
			{
				barriers[indexOf<Subset, Value, SystemBase>()].wait();
			}
		}
	};

	template <typename Sys>
	struct DependeeBlocker
	{
		template <typename Value>
		inline static void callback(czsf::Barrier* barriers)
		{
			CZSS_CONST_IF (directlyDependsOn<Value, Sys>() && !transitivelyDependsOn<Value, Sys>())
			{
				barriers[indexOf<Subset, Value, SystemBase>()].wait();
			}
		}
	};

	struct SystemRunner
	{
		template <typename Value>
		inline static void callback(uint64_t* id, czsf::Barrier* barriers, Arch* arch)
		{
			tuple_utils::OncePerType<Subset, SystemBlocker<Value>>::fn(barriers);

#ifdef CZSS_TIMING_BEGIN
			CZSS_TIMING_BEGIN<Arch, Value>(arch);
#endif
			Accessor<Arch, Value> accessor(arch);
			Value::run(accessor);

#ifdef CZSS_TIMING_END
			CZSS_TIMING_END<Arch, Value>(arch);
#endif
			barriers[*id].signal();
		}
	};

	struct SystemInitialize
	{
		template <typename Value>
		inline static void callback(const uint64_t* id, czsf::Barrier* barriers, Arch* arch)
		{
			tuple_utils::OncePerType<Subset, SystemBlocker<Value>>::fn(barriers);
			Accessor<Arch, Value> accessor(arch);
			Value::initialize(accessor);
			barriers[*id].signal();
		}
	};

	struct SystemShutdown
	{
		template <typename Value>
		inline static void callback(uint64_t* id, czsf::Barrier* barriers, Arch* arch)
		{
			tuple_utils::OncePerType<Subset, DependeeBlocker<Value>>::fn(barriers);
			Accessor<Arch, Value> accessor(arch);
			Value::shutdown(accessor);
			barriers[*id].signal();
		}
	};

	static void systemCallback(RunTaskData<Arch>* data)
	{
		tuple_utils::Switch<Subset>::template fn<SystemRunner>(data->id, &data->id, data->barriers, data->arch);
	}

	static void initializeSystemCallback(RunTaskData<Arch>* data)
	{
		tuple_utils::Switch<Subset>::template fn<SystemInitialize>(data->id, &data->id, data->barriers, data->arch);
	}

	static void shutdownSystemCallback(RunTaskData<Arch>* data)
	{ 
		tuple_utils::Switch<Subset>::template fn<SystemShutdown>(data->id, &data->id, data->barriers, data->arch);
	}

	template <typename T>
	static void runForSystems(Arch* arch, void (*fn)(RunTaskData<Arch>*), T* fls)
	{
		static const uint64_t sysCount = numUniques<Subset, SystemBase>();
		czsf::Barrier barriers[sysCount];
		RunTaskData<Arch> taskData[sysCount];

		for (uint64_t i = 0; i < sysCount; i++)
		{
			new (&barriers[i]) czsf::Barrier(1);
			taskData[i].arch = arch;
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
};

template <template<typename...> class T, typename ...Values>
struct RewrapImpl;

template <template<typename...> class T, typename ...Values>
struct RewrapImpl<T, std::tuple<Values...>>
{
	using type = T<Values...>;
};

template <template<typename...> class T, typename ...Values>
using Rewrap = typename RewrapImpl<T, Values...>::type;

template <typename Entity, typename Accessor>
void onCreate(Entity& entity, Accessor& accessor) {};

template <typename Component, typename Entity, typename Accessor>
void onCreate(Component& component, Entity& entity, Accessor& accessor) {};

template <typename Entity, typename Accessor, typename Context>
void onCreate(Entity& entity, Accessor& accessor, const Context& context) {};

template <typename Component, typename Entity, typename Accessor, typename Context>
void onCreate(Component& component, Entity& entity, Accessor& accessor, const Context& context) {};

template <typename Entity, typename Accessor>
void onDestroy(Entity& entity, Accessor& accessor) {};

template <typename Component, typename Entity, typename Accessor>
void onDestroy(Component& component, Entity& entity, Accessor& accessor) {};

template <typename Desc, typename ...Contents>
struct Architecture : VirtualArchitecture
{
	using Cont = tuple_utils::Set<Contents...>;
	using This = Architecture<Desc, Contents...>;
	using OmniSystem = System <
		Rewrap<Orchestrator, Filter<Cont, EntityBase>>,
		Rewrap<Writer, Filter<Cont, ResourceBase>>
	>;

	Accessor<Desc, OmniSystem> accessor()
	{
		return Accessor<Desc, OmniSystem>(reinterpret_cast<Desc*>(this));
	}

	static constexpr uint64_t numEntities() { return numUniques<Cont, EntityBase>(); }

	template<typename Entity>
	static constexpr uint64_t entityIndex()
	{
		return isEntity<Entity>() ? indexOf<Cont, Entity, EntityBase>() : -1;
	}

	template <typename Resource>
	void setResource(Resource* res)
	{
		static_assert(isResource<Resource>(), "Template parameter must be a Resource.");
		static_assert(inspect::contains<Cont, Resource>(), "Architecture doesn't contain Resource.");
		static constexpr uint64_t INDEX = indexOf<Cont, Resource, ResourceBase>();
		std::get<tuple_utils::Index<Resource*, RewrapElements<std::add_pointer, Filter<Cont, ResourceBase>>>::value>(resources) = res;
	}

	template <typename Alloc>
	struct ResourceInitializer
	{
		template <typename R>
		static void callback(This* arch, Alloc& alloc)
		{
			typedef typename std::allocator_traits<Alloc>::template rebind_alloc<R> ralloc;
			using traits = std::allocator_traits<ralloc>;
			auto a = ralloc(alloc);
			auto p = traits::allocate(a, 1);
			traits::construct(a, p);
			arch->setResource(p);
		}
	};

	template <typename Alloc>
	struct ResourceDestructor
	{
		template <typename R>
		static void callback(This* arch, Alloc& alloc)
		{
			typedef typename std::allocator_traits<Alloc>::template rebind_alloc<R> ralloc;
			using traits = std::allocator_traits<ralloc>;
			auto a = ralloc(alloc);
			auto p = arch->template getResource<R>();
			traits::destroy(a, p);
			traits::deallocate(a, p, 1);
		}
	};

	template <typename T, typename Alloc>
	void initializeResources(Alloc& alloc)
	{
		tuple_utils::OncePerType<T, ResourceInitializer<Alloc>>::fn(this, alloc);
	}

	template <typename Alloc>
	void initializeResources(Alloc& alloc)
	{
		using _r = Filter<Cont, ResourceBase>;
		this->initializeResources<_r, Alloc>(alloc);
	}

	template <typename T, typename Alloc>
	void freeResources(Alloc& alloc)
	{
		tuple_utils::OncePerType<T, ResourceDestructor<Alloc>>::fn(this, alloc);
	}

	template <typename Alloc>
	void freeResources(Alloc& alloc)
	{
		using _r = Filter<Cont, ResourceBase>;
		this->freeResources<_r, Alloc>(alloc);
	}

	template <typename Resource>
	Resource* getResource()
	{
		static_assert(isResource<Resource>(), "Template parameter must be a Resource.");
		static_assert(inspect::contains<Cont, Resource>(), "Architecture doesn't contain Resource.");
		return std::get<tuple_utils::Index<Resource*, RewrapElements<std::add_pointer, Filter<Cont, ResourceBase>>>::value>(resources);
	}

	template <typename Entity>
	EntityStore<Entity>* getEntities()
	{
		static_assert(inspect::contains<Cont, Entity>(), "Architecture doesn't contain the entity.");
		// assert(isEntity<Entity>());
		return &std::get<tuple_utils::Index<EntityStore<Entity>, RewrapElements<EntityStore, Filter<Cont, EntityBase>>>::value>(entities);
	}

	template <typename Category>
	static const char* name(uint64_t id)
	{
		static const char* d = "Unknown";
		const char** result = &d;
		tuple_utils::OncePerType<Cont, NameFinder<Category>>::fn(id, result);
		return *result;
	}

	void* getEntity(Guid guid)
	{
		void* ret = nullptr;
		uint64_t tk = typeKey(guid);
		tuple_utils::Switch<Filter<Cont, EntityBase>>::template fn<GetEntityVoidPtr>(tk, this, tk, guid, &ret);
		return ret;
	}

	template <typename Entity>
	Entity* getEntity(Guid guid)
	{
		return indexOf<Cont, Entity, EntityBase>() == typeKey(guid)
			? getEntity<Entity>(guidId(guid))
			: nullptr;
	}

	template <typename Entity>
	Entity* getEntity(uint64_t id)
	{
		return getEntities<Entity>()->get(id);
	}

private:
	template <typename System>
	struct ResolveGuid
	{
		template <typename T, typename F>
		static void callback(This* arch, uint64_t key, uint64_t id, F f, bool& result)
		{
			if (entityIndex<T>() == key)
			{
				auto entities = arch->template getEntities<T>();
				auto entity = entities->get(id);
				if (entity != nullptr)
				{
					auto accessor = TypedEntityAccessor<System, T>(entity);
					f(accessor);
					result = true;
				}
			}
		}
	};

	template <typename System, typename Component>
	struct AccessComponent
	{
		template <typename T, typename F>
		static void callback(This* arch, uint64_t key, uint64_t id, F f, bool& result)
		{
			if (entityIndex<T>() == key)
			{
				auto entities = arch->template getEntities<T>();
				auto entity = entities->get(id);
				if (entity != nullptr)
				{
					auto accessor = TypedEntityAccessor<System, T>(entity);
					f(accessor.template getComponent<Component>());
					result = true;
				}
			}
		}
	};

	template <typename ...Components>
	struct ContainsAllComponentsFilter
	{
		template <typename Entity>
		static constexpr bool test()
		{
			return inspect::containsAllIn<typename Entity::Cont, Filter<Flatten<std::tuple<Components...>>, ComponentBase>>();
		}
	};

public:
	template <typename Sys, typename F>
	bool accessEntity(Guid guid, F f)
	{
		bool result = false;
		tuple_utils::OncePerType<Filter<Cont, EntityBase>, ResolveGuid<Sys>>::fn(this, typeKey(guid), guidId(guid), f, result);
		return result;
	}

	template <typename Sys, typename Component, typename F>
	bool accessComponent(Guid guid,  F f)
	{
		using _entities = Filter<Cont, EntityBase>;
		using _set = tuple_utils::Subset<_entities, ContainsAllComponentsFilter<Component>>;
		bool result = false;
		tuple_utils::OncePerType<_set, AccessComponent<Sys, Component>>::fn(this, typeKey(guid), guidId(guid), f, result);
		return result;
	}

	template <typename Sys, typename F, typename ...Components>
	bool accessEntityFiltered(Guid guid, F f)
	{
		using _entities = Filter<Cont, EntityBase>;
		using _set = tuple_utils::Subset<_entities, ContainsAllComponentsFilter<Components...>>;
		bool result = false;
		tuple_utils::OncePerType<_set, ResolveGuid<Sys>>::fn(this, typeKey(guid), guidId(guid), f, result);
		return result;
	}

private:
	template <typename System, typename Entity>
	void postInitializeEntity(Guid guid, Entity* ent)
	{
		setEntityId(ent, guid.get());
		auto accessor = Accessor<Desc, System>(reinterpret_cast<Desc*>(this));
		tuple_utils::OncePerType<typename Entity::Cont, OnCreateCallback>::fn(*ent, accessor);
		onCreate(*ent, accessor);
	}

	template <typename System, typename Entity, typename Context>
	void postInitializeEntity(Guid guid, Entity* ent, const Context& context)
	{
		setEntityId(ent, guid.get());
		auto accessor = Accessor<Desc, System>(reinterpret_cast<Desc*>(this));
		tuple_utils::OncePerType<typename Entity::Cont, OnCreateContextCallback>::fn(*ent, accessor, context);
		onCreate(*ent, accessor, context);
	}

public:
	template <typename Entity, typename System, typename ...Params>
	Entity* createEntity(Params&&... params)
	{
		CZSS_CONST_IF (isEntity<Entity>())
		{
			auto ent = initializeEntity<Entity>(std::forward<Params>(params)...);
			Guid guid = ent->getGuid();
			postInitializeEntity<System>(guid, ent);
			return ent;
		}

		return nullptr;
	}

	template <typename Entity, typename ...Params>
	Entity* createEntity(Params&&... params)
	{
		return createEntity<Entity, OmniSystem>(std::forward(params)...);
	}

	template <typename Entity, typename System, typename Context, typename ...Params>
	Entity* createEntityWithContext(const Context& context, Params&&... params)
	{
		CZSS_CONST_IF (isEntity<Entity>())
		{
			auto ent = initializeEntity<Entity>(std::forward<Params>(params)...);
			Guid guid = ent->getGuid();
			postInitializeEntity<System>(guid, ent, context);
			return ent;
		}

		return nullptr;
	}

	template <typename Entity, typename Context, typename ...Params>
	Entity* createEntityWithContext(const Context& context, Params&&... params)
	{
		return createEntityWithContext<Entity, OmniSystem>(context, std::forward(params)...);
	}

	template <typename Entity>
	static Guid getEntityGuid(Entity* ent)
	{
		return Guid(VirtualArchitecture::getEntityId(ent)
			+ (indexOf<Cont, Entity, EntityBase>()) << (63 - typeKeyLength()));
	}

	template <typename Entity>
	static EntityId<Desc, Entity> getEntityId(Entity* ent)
	{
		return VirtualArchitecture::getEntityId<Desc, Entity>(ent);
	}

	template <typename System, typename Entity>
	void destroyEntity(Entity& entity)
	{
		auto entities = getEntities<Entity>();
		auto id = guidId(entity.getGuid());
		auto accessor = Accessor<Desc, System>(reinterpret_cast<Desc*>(this));
		tuple_utils::OncePerType<typename Entity::Cont, OnDestroyCallback>::fn(entity, accessor);
		onDestroy(entity, accessor);
		entities->destroy(id);
	}

	template <typename Entity>
	void destroyEntity(Entity& entity)
	{
		destroyEntity<OmniSystem, Entity>(entity);
	}

	template <typename System, typename Entity>
	void destroyEntity(uint64_t id)
	{
		auto entities = getEntities<Entity>();
		auto res = entities->get(id);
		if (res != nullptr)
			destroyEntity<System>(*res);
	}

	// todo system parameter, this is used in callback
	template <typename Entity>
	void destroyEntity(uint64_t id)
	{
		destroyEntity<OmniSystem, Entity>(id);
	}

	template <typename System>
	void destroyEntity(Guid guid)
	{
		uint64_t tk = typeKey(guid);
		uint64_t id = guidId(guid);
		tuple_utils::Switch<Filter<Cont, EntityBase>>::template fn<EntityDestructorCallback<System>>(tk, this, id);
	}

	void destroyEntity(Guid guid)
	{
		destroyEntity<OmniSystem>(guid);
	}

	template <typename Entity>
	void destroyEntity(EntityId<Desc, Entity> key)
	{
		destroyEntity<Entity>(This::getEntityId(key));
	}

	template <typename ...Entities>
	void destroyEntities()
	{
		tuple_utils::OncePerType<tuple_utils::Set<Entities...>, DestroyEntitiesCallback>(this);
	}

	static constexpr uint64_t typeKeyLength()
	{
		return ceil(log2(numUniques<Cont, EntityBase>()));
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

	template <typename V>
	static constexpr uint64_t absoluteIndex()
	{
		return tuple_utils::Index<V, Cont>::value;
	}

	static std::string dotGraphString()
	{
		std::string ret("digraph {\n");

		tuple_utils::OncePerType<Cont, DotGenerateNodes>::fn(&ret);
		using Filtered = tuple_utils::Union< Filter<Cont, EntityBase>, Filter<Cont, SystemBase>, Filter<Cont, ComponentBase>, Filter<Cont, IteratorBase>, Filter<Cont, ResourceBase>>;
		oncePerPair<Filtered, DotGenerateEdges>(&ret);
		ret += "}\n";

		return ret;
	}

	struct DotGenerateNodes
	{
		template<typename V>
		static void callback(std::string* str)
		{
			CZSS_CONST_IF (absoluteIndex<V>() == absoluteIndex<Dummy>())
				return;

			std::string a;
			std::string shape = "box";

			CZSS_CONST_IF(isSystem<V>())
				shape = "ellipse";
			CZSS_CONST_IF(isEntity<V>())
				shape = "cylinder";
			CZSS_CONST_IF(isComponent<V>())
				shape = "rect";
			CZSS_CONST_IF(isResource<V>())
				shape = "diamond";

			a += std::to_string(absoluteIndex<V>()) + " [label = " + czss::name<V>() + ", shape = " + shape + " ]\n";
			str->append(a);
		}
	};

	struct DotGenerateEdges
	{
		template<typename A, typename B>
		static void callback(std::string* str)
		{
			CZSS_CONST_IF (absoluteIndex<A>() == absoluteIndex<Dummy>() || absoluteIndex<B>() == absoluteIndex<Dummy>())
				return;

			std::string an = std::to_string(absoluteIndex<A>());
			std::string bn = std::to_string(absoluteIndex<B>());

			std::string s;

			CZSS_CONST_IF (isSystem<A>() && isSystem<B>() && !std::is_same<A, B>())
			{
				CZSS_CONST_IF (directlyDependsOn<A, B>())
				{
					CZSS_CONST_IF (!exclusiveWith<A, B>())
						s += an + " -> " + bn + " [ style = dotted, color = grey ]\n";
					else if (redundantDependency<A, B>())
						s += an + " -> " + bn + " [ style = dotted, color = grey ]\n";
					else
						s += an + " -> " + bn + "\n";
				}

				CZSS_CONST_IF (exclusiveWith<A, B>() && !dependsOn<A, B>() && !dependsOn<B, A>())
						s += an + " -> " + bn + " [ style = dotted ]\n";
			}

			CZSS_CONST_IF (isSystem<A>() && isComponent<B>())
			{
				CZSS_CONST_IF (canWrite<A, B>())
					s += an + " -> " + bn + " [ color = yellow ]\n";

				else CZSS_CONST_IF (canRead<A, B>())
					s += an + " -> " + bn + " [ color = green ]\n";
			}

			CZSS_CONST_IF (isSystem<A>() && isEntity<B>())
			{
				CZSS_CONST_IF (canOrchestrate<A, B>())
					s += an + " -> " + bn + " [ color = red ]\n";
			}

			CZSS_CONST_IF (isSystem<A>() && isResource<B>())
			{
				CZSS_CONST_IF (isThreadSafe<B>() && canWrite<A, B>())
					s += an + " -> " + bn + " [ color = green ]\n";
				else CZSS_CONST_IF (canWrite<A, B>())
					s += an + " -> " + bn + " [ color = red ]\n";
				else CZSS_CONST_IF (canRead<A, B>())
					s += an + " -> " + bn + " [ color = green ]\n";
			}

			CZSS_CONST_IF((isEntity<A>() || isIterator<A>()) && isComponent<B>() && inspect::contains<Flatten<typename A::Cont>, B>())
				s += an + " -> " + bn + "\n";

			str->append(s);
		}
	};

private:
	RewrapElements<std::add_pointer, Filter<Cont, ResourceBase>> resources;
	RewrapElements<EntityStore, Filter<Cont, EntityBase>> entities;

	template <typename Entity, typename ...Params>
	Entity* initializeEntity(Params&&... params)
	{
		static_assert(isEntity<Entity>(), "Template parameter must be an Entity.");

		Entity* ent;
		uint64_t tk = entityIndex<Entity>() << 63 - typeKeyLength();
		uint64_t id;
		auto entities = getEntities<Entity>();
		ent = entities->template create<Entity>(id, std::forward<Params>(params)...);

		id += tk;
		setEntityId(ent, id);

		return ent;
	}

	struct DestroyEntitiesCallback
	{
		template <typename Value>
		static void callback(This* arch)
		{
			EntityStore<Value>* entities = arch->template getEntities<Value>();
			entities->clear();
		}
	};

	template <typename System>
	struct EntityDestructorCallback
	{
		template <typename Value>
		inline static void callback(This* arch, const uint64_t& id)
		{
			arch->template destroyEntity<System, Value>(id);
		}
	};

	struct GetEntityVoidPtr
	{
		template <typename Value>
		inline static void callback(This* arch, const uint64_t& typeKey, const Guid& guid, void** ret)
		{
			if(isEntity<Value>() && inspect::contains<Cont, Value>() && indexOf<Cont, Value, EntityBase>() == typeKey)
				*ret = arch->template getEntity<Value>(guid);
		}
	};

	template <typename Cat>
	struct NameFinder
	{
		template <typename Value>
		inline static void callback(uint64_t id, const char** res)
		{
			if (std::is_base_of<Cat, Value>() && indexOf<Cont, Value, Cat>() == id)
			{
				*res = czss::name<Value>();
			}
		}
	};

	struct OnCreateCallback
	{
		template <typename Component, typename Entity, typename Accessor>
		inline static void callback(Entity& value, Accessor& accessor)
		{
			onCreate(*value.template getComponent<Component>(), value, accessor);
		}
	};

	struct OnCreateContextCallback
	{
		template <typename Component, typename Entity, typename Accessor, typename Context>
		inline static void callback(Entity& value, Accessor& accessor, const Context& context)
		{
			onCreate(*value.template getComponent<Component>(), value, accessor, context);
		}
	};

	struct OnDestroyCallback
	{
		template <typename Component, typename Entity, typename Accessor>
		inline static void callback(Entity& value, Accessor& accessor)
		{
			onDestroy(*value.template getComponent<Component>(), value, accessor);
		}
	};
};

// #####################
// Accessors
// #####################

template<typename Sys>
struct HasEntityReadPermissionCallback
{
	template <typename Value>
	static void callback()
	{
		static_assert(canRead<Sys, Value>(), "System cannot read component.");
	}
};

template<typename Sys>
struct HasEntityWritePermissionCallback
{
	template <typename Value>
	static void callback()
	{
		static_assert(canWrite<Sys, Value>(), "System cannot write to component.");
	}
};

template <typename Iter, typename Arch, typename Sys>
struct IteratorAccessor;

template <typename Iter, typename Arch, typename Sys>
struct IterableStub;

template <typename Iter, typename Arch, typename Sys>
struct IteratorIterator;

template<typename Arch, typename Sys>
struct Accessor;

template <typename Base, typename Derived>
struct PermissionCompare
{
	template <typename V>
	static constexpr void callback()
	{
		static_assert(isEntity<V>() && canOrchestrate<Derived, V>() ? canOrchestrate<Base, V>()
				: ((isComponent<V>() || isResource<V>()) && canWrite<Derived, V>() ? canWrite<Base, V>()
					: ((isComponent<V>() || isResource<V>()) && canRead<Derived, V>() ? canRead<Base, V>()
						: true)),
			"Derived accessor must have same or lesser permissions than the original.");
	}
};

template <typename Base, typename Derived>
static constexpr void canDeriveAssert()
{
	tuple_utils::OncePerType<typename Derived::Cont, PermissionCompare<Base, Derived>>::fn();
}

template<typename Sys, typename Entity>
struct TypedEntityAccessor
{
	template <typename P>
	operator TypedEntityAccessor<P, Entity>& ()
	{
		canDeriveAssert<Sys, P>();
		return *reinterpret_cast<TypedEntityAccessor<P, Entity>*>(this);
	}

	TypedEntityAccessor(const Entity* ent)
	{
		_entity = const_cast<Entity*>(ent);
	}

	template<typename Component>
	const Component* viewComponent() const
	{
		static_assert(canRead<Sys, Component>(), "System lacks read permissions for the Iterator's components.");
		return _entity->template viewComponent<Component>();
	}

	template<typename Component>
	Component* getComponent()
	{
		static_assert(canWrite<Sys, Component>(), "System lacks write permissions for the Iterator's components.");
		return _entity->template getComponent<Component>();
	}

	Guid getGuid() const
	{
		return _entity->getGuid();
	}

	const Entity* viewEntity() const
	{
		tuple_utils::OncePerType<Flatten<typename Entity::Cont>, HasEntityReadPermissionCallback<Sys>>::fn();
		return _entity;
	}

	Entity* getEntity()
	{
		tuple_utils::OncePerType<Flatten<typename Entity::Cont>, HasEntityWritePermissionCallback<Sys>>::fn();
		return _entity;
	}

private:
	Entity* _entity;
};

template <typename Arch, typename Sys>
struct EntityAccessor
{
	template <typename P>
	operator EntityAccessor<Arch, P>& ()
	{
		canDeriveAssert<Sys, P>();
		return *reinterpret_cast<EntityAccessor<Arch, P>*>(this);
	}

	template <typename Entity>
	EntityAccessor(const Entity* ent)
	{
		entity = const_cast<Entity*>(ent);
		typeKey = indexOf<typename Arch::Cont, Entity, EntityBase>();
	}

	template <typename Component>
	bool hasComponent() const
	{
		return viewComponent<Component>() != nullptr;
	}

	template<typename Component>
	const Component* viewComponent() const
	{
		static_assert(canRead<Sys, Component>(), "System lacks read permissions for the Iterator's components.");
		return get_ptr<Component>();
	}

	template<typename Component>
	Component* getComponent()
	{
		static_assert(canWrite<Sys, Component>(), "System lacks write permissions for the Iterator's components.");
		return get_ptr<Component>();
	}

	Guid getGuid() const
	{
		Guid guid;

		tuple_utils::Switch<Filter<typename Arch::Cont, EntityBase>>::template fn<GuidGetter>(typeKey, &guid, entity);
		return guid;
	}

	bool null() const
	{
		return entity == nullptr;
	}

protected:
	EntityAccessor()
	{
		entity = nullptr;
	}

	EntityAccessor(const EntityAccessor<Arch, Sys>& other)
	{
		this->typeKey = other.typeKey;
		this->entity = other.entity;
	}

	EntityAccessor(uint64_t typeKey, void* entity)
	{
		this->typeKey = typeKey;
		this->entity = entity;
	}

private:
	friend Accessor<Arch, Sys>;
	uint64_t typeKey;
	void* entity;

	template <typename Component>
	Component* get_ptr() const
	{
		void* result = nullptr;
		tuple_utils::Switch<Filter<typename Arch::Cont, EntityBase>>::template fn<ComponentGetter<Component>>(typeKey, entity, &result);
		return reinterpret_cast<Component*>(result);
	}

	template <typename Component>
	struct Callback
	{
		template <typename Value, typename Entity>
		static void callback(Entity* entity, void** result)
		{
			CZSS_CONST_IF (std::is_same<Component, Value>())
				*result = entity->template getComponent<Value>();
		}
	};

	template <typename Component>
	struct ComponentGetter
	{
		template <typename Value>
		inline static void callback(void* entity, void** result)
		{
			tuple_utils::OncePerType<typename Value::Cont, Callback<Component>>::fn(reinterpret_cast<Value*>(entity), result);
		}
	};

	struct GuidGetter
	{
		template <typename Value>
		inline static void callback(Guid* guid, void* entity)
		{
			*guid = reinterpret_cast<Value*>(entity)->getGuid();
		}
	};
};

template <typename Iter, typename Arch, typename Sys>
struct IteratorAccessor : EntityAccessor<Arch, Sys>
{
	IteratorAccessor(const IteratorAccessor<Iter, Arch, Sys>& other) : EntityAccessor<Arch, Sys>(other) { }

	template <typename Component>
	bool hasComponent() const
	{
		return EntityAccessor<Arch, Sys>::template hasComponent<Component>();
	}

	template<typename Component>
	const Component* viewComponent() const
	{
		return EntityAccessor<Arch, Sys>::template viewComponent<Component>();
	}

	template<typename Component>
	Component* getComponent()
	{
		return EntityAccessor<Arch, Sys>::template getComponent<Component>();
	}

private:
	friend IteratorIterator<Iter, Arch, Sys>;
	friend Accessor<Arch, Sys>;

	IteratorAccessor() {}
	template <typename Entity>
	IteratorAccessor(Entity* ent) : EntityAccessor<Arch, Sys>(ent) { }
	IteratorAccessor(uint64_t key, void* entity) : EntityAccessor<Arch, Sys>(key, entity) { }
};

template <typename Iter>
struct IteratorCompatabilityFilter
{
	template <typename Entity>
	static constexpr bool test()
	{
		return isEntity<Entity>() && isIteratorCompatibleWithEntity<Iter, Entity>();
	}
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
		typeKey = limit();
	}

	IteratorIterator(const IteratorIterator& other)
	{
		arch = other.arch;
		typeKey = other.typeKey;
		index = other.index;
		maxIndex = other.maxIndex;
		accessor = other.accessor;
	}

	U& operator* ()
	{
		return accessor;
	}

	U* operator-> ()
	{
		return accessor;
	}

	friend bool operator== (const This& a, const This& b)
	{
		if (a.typeKey >= limit() && b.typeKey >= limit())
			return true;

		return a.typeKey == b.typeKey
			&& a.index == b.index;
	}

	friend bool operator!= (const This& a, const This& b)
	{
		return !(a == b);
	};

	This operator++(int)
	{
		This ret = *this;
		++(*this);
		return ret;
	}

	This& operator++()
	{
		using CompatibleEntities = tuple_utils::Subset<typename Arch::Cont, IteratorCompatabilityFilter<Iter>>;
		tuple_utils::Switch<CompatibleEntities>::template fn<IncrementerCallback>(typeKey, this);

		while (index >= maxIndex && typeKey < limit())
		{
			typeKey++;
			if (typeKey < limit())
				tuple_utils::Switch<CompatibleEntities>::template fn<IncrementerCallback>(typeKey, this);
		}

		return *this;
	}

private:
	friend IterableStub<Iter, Arch, Sys>;

	IteratorIterator(Arch* arch)
	{
		this->arch = arch;
		index = 0;
		maxIndex = 0;
		typeKey = 0;
		++(*this);
	}

	Arch* arch;
	U accessor;
	uint64_t index;
	uint64_t maxIndex;
	uint64_t typeKey;

	static constexpr size_t limit()
	{
		using CompatibleEntities = tuple_utils::Subset<typename Arch::Cont, IteratorCompatabilityFilter<Iter>>;
		return std::tuple_size<CompatibleEntities>::value;
	}

	struct IncrementerCallback
	{
		template <typename Value>
		static inline void callback(This* iterac)
		{
			auto entities = iterac->arch->template getEntities<Value>();

			if (iterac->index < iterac->maxIndex)
			{
				iterac->index++;
			}
			else
			{
				iterac->index = 0;
				iterac->maxIndex = entities->size();
			}

			if (iterac->index < iterac->maxIndex)
				iterac->accessor = U(entities->used_indices[iterac->index]);
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

template <typename Arch, typename Sys, typename ...Components>
struct FilterAccessor
{
	template <typename F>
	bool accessEntity(Guid guid, F f)
	{
		return arch->template accessEntityFiltered<Sys, F, Components...>(guid, f);
	}

private:
	Arch* arch;
	FilterAccessor(Arch* arch) : arch(arch) {}
	friend Accessor<Arch, Sys>;
};

template <typename Arch, typename Sys, typename ...Components>
struct ConstFilterAccessor
{
	template <typename F>
	bool accessEntity(Guid guid, F f) const
	{
		return arch->template accessEntityFiltered<Sys, F, Components...>(guid, f);
	}

private:
	Arch* arch;
	ConstFilterAccessor(Arch* arch) : arch(arch) {}
	friend Accessor<Arch, Sys>;
};

template<typename Arch, typename Sys>
struct Accessor
{
	Accessor(const Accessor&) = delete;
	Accessor(Accessor&&) = delete;
	Accessor& operator=(const Accessor&) = delete;
	Accessor& operator=(Accessor&&) = delete;

public:
	template <typename P>
	operator Accessor<Arch, P>& ()
	{
		canDeriveAssert<Sys, P>();
		return *reinterpret_cast<Accessor<Arch, P>*>(this);
	}

	template <typename Entity, typename ...Params>
	Entity* createEntity(Params&&... params)
	{
		entityPermission<Entity>();
		return arch->template createEntity<Entity>(std::forward<Params>(params)...);
	}

	template <typename Entity, typename Context, typename ...Params>
	Entity* createEntityWithContext(Context&& context, Params&&... params)
	{
		entityPermission<Entity>();
		return arch->template createEntityWithContext<Entity>(context, std::forward(params)...);
	}

	template <typename Entity>
	EntityAccessor<Arch, Sys> entityAccessor(const Entity* entity)
	{
		return EntityAccessor<Arch, Sys>(entity);
	}

	EntityAccessor<Arch, Sys> entityAccessor(Guid guid)
	{
		void* ptr = arch->getEntity(guid);
		uint64_t typeKey = Arch::typeKey(guid);
		return EntityAccessor<Arch, Sys>(typeKey, ptr);
	}

	template <typename F>
	bool accessEntity(Guid guid, F f)
	{
		return arch->template accessEntity<Sys>(guid, f);
	}

	template <typename Component, typename F>
	bool accessComponent(Guid guid, F f)
	{
		return arch->template accessComponent<Sys, Component>(guid, f);
	}

	template <typename ...Components>
	FilterAccessor<Arch, Sys, Components...> filterEntities()
	{
		return FilterAccessor<Arch, Sys, Components...>(arch);
	}

	template <typename ...Components>
	ConstFilterAccessor<Arch, Sys, Components...> filterEntitiesConst() const
	{
		return ConstFilterAccessor<Arch, Sys, Components...>(arch);
	}

	template <typename Entity>
	const Entity* viewEntity(Guid guid) const
	{
		static_assert(isEntity<Entity>(), "Attempted to create non-entity.");
		tuple_utils::OncePerType<Flatten<typename Entity::Cont>, HasEntityReadPermissionCallback<Sys>>::fn();
		static_assert(inspect::contains<typename Arch::Cont, Entity>(), "Architecture doesn't contain the Entity.");
		return arch->template getEntity<Entity>(guid);
	}

	template <typename Entity>
	const Entity* viewEntity(EntityId<Arch, Entity> id) const
	{
		static_assert(isEntity<Entity>(), "Attempted to create non-entity.");
		tuple_utils::OncePerType<Flatten<typename Entity::Cont>, HasEntityReadPermissionCallback<Sys>>::fn();
		static_assert(inspect::contains<typename Arch::Cont, Entity>(), "Architecture doesn't contain the Entity.");
		return arch->template getEntity<Entity>(id);
	}

	template <typename Entity>
	Entity* getEntity(Guid guid)
	{
		static_assert(isEntity<Entity>(), "Attempted to create non-entity.");
		tuple_utils::OncePerType<Flatten<typename Entity::Cont>, HasEntityWritePermissionCallback<Sys>>::fn();
		static_assert(inspect::contains<typename Arch::Cont, Entity>(), "Architecture doesn't contain the Entity.");
		return arch->template getEntity<Entity>(guid);
	}

	template <typename Entity>
	Entity* getEntity(EntityId<Arch, Entity> id)
	{
		static_assert(isEntity<Entity>(), "Attempted to create non-entity.");
		tuple_utils::OncePerType<Flatten<typename Entity::Cont>, HasEntityWritePermissionCallback<Sys>>::fn();
		static_assert(inspect::contains<typename Arch::Cont, Entity>(), "Architecture doesn't contain the Entity.");
		return arch->template getEntity<Entity>(id);
	}

	template <typename Entity>
	EntityId<Arch, Entity> getEntityId(const Entity* ent) const
	{
		return Arch::getEntityId(ent);
	}

	void destroyEntity(Guid guid)
	{
		if (tuple_utils::OncePerType<typename Arch::Cont, EntityDestructionPermission>::constFn(Arch::typeKey(guid)))
		{
			arch->template destroyEntity<Sys>(guid);
		}
	}

	template <typename Entity>
	void destroyEntity(EntityId<Arch, Entity> key)
	{
		static_assert(Sys::template canOrchestrate<Entity>(), "System does not have the permission to destroy this entity.");
		arch->template destroyEntity<Sys>(key);
	}

	template <typename ...Entities>
	void destroyEntities()
	{
		tuple_utils::OncePerType<Rbox<Entities...>, EntityOrchestrationPermissionTest>::fn();
		arch->template destroyEntities<Entities...>();
	}

	struct MiniRunMapper
	{
		template <typename V>
		struct MrmImpl
		{
			using type = V;
			using fn = V;
		};

		template <typename A, typename S>
		struct MrmImpl<void(*)(Accessor<A, S>)>
		{
			using type = S;
			using fn = void(*)(Accessor<A, S>);
		};

		template <typename A, typename S>
		struct MrmImpl<void(*)(Accessor<A, S>&)>
		{
			using type = S;
			using fn = void(*)(Accessor<A, S>&);
		};

		template <typename A, typename S>
		struct MrmImpl<void(*)(const Accessor<A, S>&)>
		{
			using type = S;
			using fn = void(*)(const Accessor<A, S>&);
		};

		template <typename V>
		using type = typename MrmImpl<V>::type;

		template <typename V>
		using fn = typename MrmImpl<V>::fn;
	};

	template <std::size_t I, typename A>
	struct MiniRunFilter
	{
		template <typename V>
		struct FilterImpl;

		template <std::size_t J, typename B>
		struct FilterImpl<std::tuple< std::integral_constant<std::size_t, J>, B >>
		{
			static constexpr bool value = (J < I) && exclusiveWith<A, B>();
		};

		template <typename V>
		static constexpr bool test()
		{
			return FilterImpl<V>::value;
		}
	};

	struct MinirunTaskData
	{
		czsf::Barrier* barriers;
		void (*fn) (void*);
		Accessor<Arch, Sys>* arch;
		size_t i;
	};

	template <size_t I, typename Others, typename P>
	static void _minirun(MinirunTaskData* task)
	{
		using _s = typename MiniRunMapper::template type<P>;
		using _filt = tuple_utils::Subset<Others, MiniRunFilter<I, _s>>;

		// Redundant waits here
		tuple_utils::oncePerType2<_filt>([&] <typename O> () {
			auto i = std::tuple_element<0, O>::type::value;
			task->barriers[i].wait();
		});

		auto fn = reinterpret_cast<typename MiniRunMapper::template fn<P>>(task->fn);
		fn(*task->arch);
	}

	template <typename ...S>
	void minirun(S... fn)
	{
		using _systems = tuple_utils::Map<std::tuple<S...>, MiniRunMapper>;
		using _numbered = tuple_utils::Numbered<_systems>;

		static constexpr size_t N = sizeof...(S);
		auto pointers = std::tuple(fn...);

		std::vector<czsf::Barrier> barriers(N);
		std::vector<MinirunTaskData> taskData(N);

		for (size_t i = 0; i < N; i++)
			new(&barriers[i]) czsf::Barrier(1);

		tuple_utils::oncePerType2<_numbered>([&] <typename P> () {
			static constexpr size_t I = std::tuple_element<0, P>::type::value;

			taskData[I].barriers = barriers.data();
			taskData[I].fn = reinterpret_cast<void(*)(void*)> (std::get<I>(pointers));
			taskData[I].arch = this;
			taskData[I].i = I;
			czsf::run(_minirun<I, _numbered, typename std::tuple_element<I, decltype(pointers)>::type>, &taskData[I], 1, &barriers[I]);
		});

		for (size_t i = 0; i < N; i++)
			barriers[i].wait();
	}


	template <typename Resource>
	const Resource* viewResource() const
	{
		static_assert(isResource<Resource>(), "Attempted to read non-resource.");
		static_assert(canRead<Sys, Resource>(), "System lacks permission to read Resource.");
		static_assert(inspect::contains<typename Arch::Cont, Resource>(), "Architecture doesn't contain the Resource.");
		return arch->template getResource<Resource>();
	}

	template <typename Resource>
	Resource* getResource()
	{
		static_assert(isResource<Resource>(), "Attempted to write to non-resource.");
		static_assert(canWrite<Sys, Resource>(), "System lacks permission to modify Resource.");
		static_assert(inspect::contains<typename Arch::Cont, Resource>(), "Architecture doesn't contain the Resource.");
		return arch->template getResource<Resource>();
	}

	// TODO add special permission?
	template <typename Resource>
	void setResource(Resource* res)
	{
		static_assert(isResource<Resource>(), "Attempted to write to non-resource.");
		static_assert(canWrite<Sys, Resource>(), "System lacks permission to modify Resource.");
		static_assert(inspect::contains<typename Arch::Cont, Resource>(), "Architecture doesn't contain the Resource.");
		arch->setResource(res);
	}

	template <typename Iterator>
	IterableStub<Iterator, Arch, Sys> iterate()
	{
		iteratorPermission<Iterator>();
		return IterableStub<Iterator, Arch, Sys>(arch);
	}

	template <typename ...Values>
	IterableStub<Iterator<Values...>, Arch, Sys> iterate2()
	{
		return iterate<Iterator<Values...>>();
	}

	template <typename Iterator, typename F>
	void iterate(F f)
	{
		using _compat = tuple_utils::Subset<typename Arch::Cont, IteratorCompatabilityFilter<Iterator>>;
		iteratorPermission<Iterator>();

		tuple_utils::OncePerType<_compat, TypedIteratorCallback<Iterator>>::fn(f, arch);
	}

	template <typename Iterator, typename F>
	void parallelIterate(uint64_t numTasks, F f)
	{
		parallelIterateImpl<Iterator>(numTasks, f, TypedParallelIterateTask<Iterator, F>);
	}

	template <typename Iterator>
	uint64_t countCompatibleEntities()
	{
		uint64_t entityCount = 0;
		using _compat = tuple_utils::Subset<typename Arch::Cont, IteratorCompatabilityFilter<Iterator>>;
		tuple_utils::OncePerType<_compat, EntityCountCallback>::fn(&entityCount, arch);
		return entityCount;
	}

// private:
	// friend Arch;
	Accessor(Arch* arch) { this->arch = arch; }
	Arch* arch;

private:
	template <typename Iterator, typename F, typename CB>
	void parallelIterateImpl(uint64_t numTasks, F f, CB* cb)
	{
		iteratorPermission<Iterator>();
		uint64_t counter = countCompatibleEntities<Iterator>();

		std::vector<ParallelIterateTaskData<F>> taskvec(numTasks);
		ParallelIterateTaskData<F>* tasks = taskvec.data();

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
		czsf::run(cb, tasks, numTasks, &barrier);
		barrier.wait();
	}

	template <typename Iterator>
	static void iteratorPermission()
	{
		static_assert(isIterator<Iterator>(), "Attempted to iterate non-iterator.");
		static_assert(inspect::containsAllIn<SystemAccesses<Sys>, typename Iterator::Cont>(), "System doesn't have access to all components of Iterator.");
	}

	template <typename Entity>
	static void entityPermission()
	{
		static_assert(isEntity<Entity>(), "Attempted to orchestrate non-entity.");
		static_assert(canOrchestrate<Sys, Entity>(), "System lacks permission to orchestrate the Entity.");
		// static_assert(inspect::contains<typename Arch::Cont, Entity>(), "Architecture doesn't contain the Entity.");
	}

	struct EntityDestructionPermission
	{
		template <typename Value>
		static constexpr bool callback(const uint64_t& key)
		{
			return isEntity<Value>()
				&& indexOf<typename Arch::Cont, Value, EntityBase>() == key
				&& canOrchestrate<Sys, Value>();
		}
	};

	struct EntityOrchestrationPermissionTest
	{
		template <typename V>
		static void callback()
		{
			entityPermission<V>();
		}
	};

	template <typename Other, typename Systems>
	struct SystemAccessValidationCallback
	{
		template <typename Value>
		static void callback()
		{
			#define ErrorMessage "System running other systems via accessor must have all permissions of the systems being run."

			CZSS_CONST_IF (isComponent<Value>())
			{
				CZSS_CONST_IF (canRead<Other, Value>())
					static_assert(canRead<Sys, Value>(), ErrorMessage);

				CZSS_CONST_IF (canWrite<Other, Value>())
					static_assert(canWrite<Sys, Value>(), ErrorMessage);
			}

			CZSS_CONST_IF (isEntity<Value>())
			{
				CZSS_CONST_IF (canOrchestrate<Other, Value>())
					static_assert(canOrchestrate<Sys, Value>(), ErrorMessage);
			}

			#undef ErrorMessage

			CZSS_CONST_IF (isSystem<Value>())
			{
				static_assert(!inspect::contains<Arch::Cont, Value>(), "Subsystems must not refer any types in the architecture");
				static_assert(inspect::contains<Systems, Value>(), "Subsystem run must include all dependencies.");
			}
		}
	};

	template <typename Systems>
	struct SystemsRunPermissionTest
	{
		template <typename V>
		static void callback()
		{
			static_assert(isSystem<V>(), "Only systems allowed as template parameters.");
			static_assert(!inspect::contains<Arch::Cont, V>(), "Architecture must not refer to system.");
			typename V::Cont::template evaluate<tuple_utils::OncePerType<Dummy, SystemAccessValidationCallback<V, Systems>>>();
		}
	};

	template <typename Iterator>
	struct TypedIteratorCallback
	{
		template <typename Value, typename F>
		static inline void callback(F& f, Arch* arch)
		{
			CZSS_CONST_IF (isEntity<Value>() && isIteratorCompatibleWithEntity<Iterator, Value>())
			{
				auto ents = arch->template getEntities<Value>();
				for (auto& ent : ents->used_indices)
				{
					auto accessor = TypedEntityAccessor<Sys, Value>(ent);
					f(accessor);
				}
			}
		}
	};

	struct EntityCountCallback
	{
		template <typename Entity>
		static inline void callback(uint64_t* count, Arch* arch)
		{
			auto entities = arch->template getEntities<Entity>();
			*count += entities->size();
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
	struct TypedParallelIterateTaskCallback
	{
		template <typename Value>
		static inline void callback(ParallelIterateTaskData<F>* data)
		{
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
				auto accessor = TypedEntityAccessor<Sys, Value>(*it);
				F lambda = *data->func;
				lambda(data->index, accessor);
			}

			data->entityCount -= handled;
			data->beginIndex = 0;
		}
	};

	template <typename Iterator, typename F>
	static void TypedParallelIterateTask(ParallelIterateTaskData<F>* data)
	{
		using _compat = tuple_utils::Subset<typename Arch::Cont, IteratorCompatabilityFilter<Iterator>>;
		static const uint64_t limit = std::tuple_size<_compat>::value;

		for (uint64_t i = 0; i < limit; i++)
		{
			tuple_utils::Switch<_compat>::template fn<TypedParallelIterateTaskCallback<Iterator, F>>(i, data);
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



// #####################
// System
// #####################

template <typename Sys>
struct SystemExclusiveCheck
{
	template <typename Value>
	static constexpr bool callback()
	{
		return (!isResource<Value>() && canWrite<Sys, Value>())
			|| (isResource<Value>() && !isThreadSafe<Value>() && canWrite<Sys, Value>());
	}
};

template <typename A, typename B>
constexpr bool exclusiveWith()
{
	return tuple_utils::OncePerType<SystemAccesses<A>, SystemExclusiveCheck<B>>::constFn()
		|| tuple_utils::OncePerType<SystemAccesses<B>, SystemExclusiveCheck<A>>::constFn();
}

template <typename Arch>
void synchronize(Accessor<Arch, typename Arch::OmniSystem>& arch) {}

} // namespace czss

namespace std {
template<>
struct hash<czss::Guid>
{
	size_t operator()(const czss::Guid& guid) const noexcept
	{
		return std::hash<uint64_t>()(guid.get());
	}
};

} // namespace std

#endif // CZSS_HEADERS_H

#ifdef CZSS_IMPLEMENTATION

#ifndef CZSS_IMPLEMENTATION_GUARD_
#define CZSS_IMPLEMENTATION_GUARD_

namespace czss
{

Guid::Guid() { }

Guid::Guid(uint64_t v) : id(v) { }

Guid::Guid(const Guid& other) : id(other.id) { }

uint64_t Guid::get() const
{
	return id;
}

bool Guid::operator ==(const Guid& other) const
{
	return this->id == other.id;
}

bool Guid::operator !=(const Guid& other) const
{
	return this->id != other.id;
}

bool Guid::operator <(const Guid& other) const
{
	return this->id < other.id;
}

void TemplateStubs::setGuid(Guid guid) { }
Guid TemplateStubs::getGuid() const { return Guid(0); }

} // namespace czss

#endif	// CZSS_IMPLEMENTATION_GUARD_

#endif // CZSS_IMPLEMENTATION
