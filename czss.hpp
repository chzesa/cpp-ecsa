#ifndef CZSS_HEADERS_H
#define CZSS_HEADERS_H

#include "tuple_utils.hpp"

#ifndef CZSF_HEADERS_H
#include "external/c-fiber/czsf.h"
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

struct Dummy
{
	using Cont = std::tuple<>;
};

template <typename ...A>
using Rbox = unique_tuple::unique_tuple<A...>;

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
	return tuple_contains<Tuple, T>::value;
}

template<typename Left, typename Right>
constexpr bool containsAllIn()
{
	return std::tuple_size< tuple_difference<Right, Left> >::value == 0;
}

} // namespace inspect

template <size_t I, typename T>
struct ForEach
{
	inline static void fn()
	{
		T::template callback<I>();
		ForEach<I - 1, T>::fn();
	}

	template<typename ...Params>
	inline static void fn(Params&&... params)
	{
		T::template callback<I>(std::forward<Params>(params)...);
		ForEach<I - 1, T>::fn(std::forward<Params>(params)...);
	}

	constexpr static bool constFn()
	{
		return T::template callback<I>() || ForEach<I - 1, T>::constFn();
	}

	template<typename ...Params>
	constexpr static bool constFn(Params&&... params)
	{
		return T::template callback<I>(std::forward<Params>(params)...) || ForEach<I - 1, T>::constFn(std::forward<Params>(params)...);
	}

	constexpr static size_t constSum()
	{
		return T::template callback<I>() + ForEach<I - 1, T>::constFn();
	}
};

template <typename T>
struct ForEach<0, T>
{
	inline static void fn()
	{
		T::template callback<0>();
	}

	template<typename ...Params>
	inline static void fn(Params&&... params)
	{
		T::template callback<0>(std::forward<Params>(params)...);
	}

	constexpr static bool constFn()
	{
		return T::template callback<0>() || false;
	}

	template<typename ...Params>
	constexpr static bool constFn(Params&&... params)
	{
		return T::template callback<0>(std::forward<Params>(params)...) || false;
	}

	constexpr static size_t constSum()
	{
		return T::template callback<0>();
	}
};

template <typename T>
struct ForEach<uint64_t(-1), T>
{
	inline static void fn() { }

	template<typename ...Params>
	inline static void fn(Params&&... params) { }
};

template <typename Tuple, typename Callback>
struct OncePerType
{
	inline static void fn()
	{
		ForEach<std::tuple_size<Tuple>::value - 1, CB>::fn();
	}

	template <typename ...Params>
	inline static void fn(Params&&... params)
	{
		ForEach<std::tuple_size<Tuple>::value - 1, CB>::fn(std::forward<Params>(params)...);
	}

	constexpr static bool constFn()
	{
		return ForEach<std::tuple_size<Tuple>::value - 1, CB2<bool>>::constFn();
	}

	template <typename ...Params>
	constexpr static bool constFn(Params&&... params)
	{
		return ForEach<std::tuple_size<Tuple>::value - 1, CB2<bool>>::constFn(std::forward<Params>(params)...);
	}

	constexpr static size_t constSum()
	{
		return ForEach<std::tuple_size<Tuple>::value - 1, CB2<size_t>>::constFn();
	}

private:
	struct CB
	{
		template <size_t I>
		inline static void callback()
		{
			Callback::template callback<typename std::tuple_element<I, Tuple>::type>();
		}

		template <size_t I, typename ...Params>
		inline static void callback(Params&&... params)
		{
			Callback::template callback<typename std::tuple_element<I, Tuple>::type>(std::forward<Params>(params)...);
		}
	};

	template <typename Ret>
	struct CB2
	{
		template <size_t I>
		constexpr static Ret callback()
		{
			return Callback::template callback<typename std::tuple_element<I, Tuple>::type>();
		}

		template <size_t I, typename ...Params>
		constexpr static Ret callback(Params&&... params)
		{
			return Callback::template callback<typename std::tuple_element<I, Tuple>::type>(std::forward<Params>(params)...);
		}
	};
};

template <typename Tuple, int Num>
struct SwitchImpl
{
	template <typename Return, typename Inspector, typename ...Params>
	constexpr static Return fn(int i, Params&&... params)
	{
		return i == Num - 1
			? Inspector::template callback<typename std::tuple_element<Num - 1, Tuple>::type>(std::forward<Params>(params)...)
			: SwitchImpl<Tuple, Num - 1>::template fn<Return, Inspector>(i, std::forward<Params>(params)...);
	}

	template <typename Inspector, typename ...Params>
	inline static void fn(int i, Params&&... params)
	{
		if (i == Num - 1)
			Inspector::template callback<typename std::tuple_element<Num - 1, Tuple>::type>(std::forward<Params>(params)...);
		else
			SwitchImpl<Tuple, Num - 1>::template fn<Inspector>(i, std::forward<Params>(params)...);
	}
};

template <typename Tuple>
struct SwitchImpl <Tuple, 0>
{
	template <typename Return,  typename Inspector, typename ...Params>
	constexpr static Return fn(int i, Params&&... params) { return Return(); }

	template <typename Inspector, typename ...Params>
	inline static void fn(int i, Params&&... params) { }
};

template <typename Tuple>
struct Switch
{
	template <typename Return, typename Inspector, typename ...Params>
	constexpr static Return fn(int i, Params&&... params)
	{
		return SwitchImpl<Tuple, std::tuple_size<Tuple>::value>::template fn<Return, Inspector>(i, std::forward<Params>(params)...);
	}

	template <typename Inspector, typename ...Params>
	inline static void fn(int i, Params&&... params)
	{
		SwitchImpl<Tuple, std::tuple_size<Tuple>::value>::template fn<Inspector>(i, std::forward<Params>(params)...);
	}
};

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
			Switch<Tuple>::template fn<CB<Value>>(i, std::forward<Params>(params)...);
	}
};

template <typename Tuple, typename Callback, typename ...Params>
void oncePerPair(Params&&... params)
{
	for (int i = 0; i < std::tuple_size<Tuple>::value; i++)
		Switch<Tuple>::template fn<OncePerTypeCallback<Tuple, Callback>>(i, std::forward<Params>(params)...);
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
		<< std::is_base_of<ComponentBase, T>()
		<< std::is_base_of<IteratorBase, T>()
		<< std::is_base_of<EntityBase, T>()
		<< std::is_base_of<SystemBase, T>()
		<< std::is_base_of<ResourceBase, T>()
		<< std::is_base_of<PermissionsBase, T>()
		<< std::is_base_of<DependencyBase, T>()
		<< std::is_base_of<ReaderBase, T>()
		<< std::is_base_of<WriterBase, T>()
		<< std::is_base_of<OrchestratorBase, T>()
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
constexpr bool isVirtual()
{
	return (isBaseType<EntityBase, T>() || isBaseType<ComponentBase, T>())
		&& std::is_base_of<Virtual, T>();
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

template <typename F, typename A, typename ...R>
struct Filter2Impl;

template <typename F, typename ...A, typename V, typename ...R>
struct Filter2Impl<F, std::tuple<A...>, V, R...>
{
	using type = typename std::conditional<
		F::template test<V>()
		, Filter2Impl<F, std::tuple<V, A...>, R...>
		, Filter2Impl<F, std::tuple<A...>, R...>
	>::type::type;
};

template <typename F, typename ...A, typename V>
struct Filter2Impl<F, std::tuple<A...>, V>
{
	using type = typename std::conditional<
		F::template test<V>()
		, unique_tuple::unique_tuple<V, A...>
		, unique_tuple::unique_tuple<A...>
	>::type;
};

template <typename T, typename F>
struct Filter2Impl2;

template <typename ...A, typename F>
struct Filter2Impl2<std::tuple<A...>, F>
{
	using type = typename Filter2Impl<F, std::tuple<>, A...>::type;
};

template <typename F>
struct Filter2Impl2<std::tuple<>, F>
{
	using type = std::tuple<>;
};

template<typename Tuple, typename F>
using Filter2 = typename Filter2Impl2<Tuple, F>::type;

template <typename Cat, typename A, typename ...R>
struct FilterImpl;

template <typename Cat, typename ...A, typename V, typename ...R>
struct FilterImpl<Cat, std::tuple<A...>, V, R...>
{
	using type = typename std::conditional<
		std::is_base_of<Cat, V>() ? true : false
		, FilterImpl<Cat, std::tuple<V, A...>, R...>
		, FilterImpl<Cat, std::tuple<A...>, R...>
	>::type::type;
};

template <typename Cat, typename ...A, typename V>
struct FilterImpl<Cat, std::tuple<A...>, V>
{
	using type = typename std::conditional<
		std::is_base_of<Cat, V>() ? true : false
		, unique_tuple::unique_tuple<V, A...>
		, unique_tuple::unique_tuple<A...>
	>::type;
};

template <typename T, typename Cat>
struct FilterImpl2;

template <typename ...A, typename Cat>
struct FilterImpl2<std::tuple<A...>, Cat>
{
	using type = typename FilterImpl<Cat, std::tuple<>, A...>::type;
};

template<typename Tuple, typename Cat>
using Filter = typename FilterImpl2<Tuple, Cat>::type;

template <typename Tuple, typename T, typename Cat>
constexpr uint64_t indexOf()
{
	return inspect::contains<Filter<Tuple, Cat>, T>() ? tuple_type_index<T, Filter<Tuple, Cat>>::value : -1;
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
	using type = typename FlattenImpl<tuple_union<std::tuple<A...>, std::tuple<V>, Flatten<typename V::Cont>>, R...>::type;
};

template <typename ...A, typename V>
struct FlattenImpl<std::tuple<A...>, V>
{
	using type = tuple_union<std::tuple<A...>, std::tuple<V>, Flatten<typename V::Cont>>;
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
	using type = typename ExpandImpl<tuple_union<typename V::Cont, std::tuple<A...>>, R...>::type;
};

template <typename ...A, typename V>
struct ExpandImpl<std::tuple<A...>, V>
{
	using type = tuple_union<typename V::Cont, std::tuple<A...>>;
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
	bool operator <(const Guid& other) const;
private:
	uint64_t id;
};

template <typename E>
struct EntityStore
{
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

public:

	template <typename T>
	T* create(uint64_t& id)
	{
		id = nextId++;

		T* res;
		auto used_indices_index = used_indices.size();

		CZSS_CONST_IF (isVirtual<T>())
		{
			res = new T();
		}
		else
		{
			if(free_indices.size() == 0)
				expand();

			auto index = free_indices.top();
			free_indices.pop();
			res = get(index);
			new(res) T();
			index_map.insert({id, index});
		}

		used_indices.push_back(res);
		used_indices_map.insert({id, used_indices_index});
		used_indices_map_reverse.insert({used_indices_index, id});

		return res;
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
			memset(p, 0, sizeof(E));
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

	uint64_t size()
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
			size_t n = 2 << (k + 5);
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
		uint64_t n = 2 << (tierCount + 5);
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
		size_t n = 2 << (tier + 5);
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
	using Cont = unique_tuple::unique_tuple<Components...>;

	void setGuid(Guid guid) { this->id = guid; }
	Guid getGuid() const { return id; }

private:
	Guid id;
};

struct VirtualArchitecture;

template <typename ...Components>
struct Entity : EntityBase
{
	using Cont = unique_tuple::unique_tuple<Components...>;

	template <typename T>
	const T* viewComponent() const
	{
		CZSS_CONST_IF(tuple_contains<Cont, T>::value)
		{
			return &std::get<min(tuple_type_index<T, Cont>::value, std::tuple_size<Cont>::value - 1)>(storage);
		}

		return nullptr;
	}

	template<typename T>
	T* getComponent()
	{
		CZSS_CONST_IF(tuple_contains<Cont, T>::value)
		{
			return &std::get<min(tuple_type_index<T, Cont>::value, std::tuple_size<Cont>::value - 1)>(storage);
		}

		return nullptr;
	}

	template <typename B, typename D>
	B* setComponent(D d)
	{
		static_assert(isVirtual<B>());
		static_assert(tuple_contains<Cont, B>::value);
		static_assert(std::is_base_of<B, D>());
		static_assert(alignof(B) == alignof(D));
		static_assert(sizeof(B) == sizeof(D));
		auto ret = getComponent<B>();
		*reinterpret_cast<D*>(ret) = std::move(d);
		return ret;
	}


	Guid getGuid() const { return {id}; }
private:
	void setGuid(Guid guid) { this->id = guid.get(); }
	friend VirtualArchitecture;
	uint64_t id;

	Cont storage;
};

template<typename Sys, typename Entity>
struct TypedEntityAccessor;

template <typename Iterator, typename Entity>
constexpr static bool isIteratorCompatibleWithEntity()
{
	return isIterator<Iterator>()
		&& isEntity<Entity>()
		&& inspect::containsAllIn<Flatten<typename Entity::Cont>, Flatten<typename Iterator::Cont>>();
}

// #####################
// Permissions
// #####################

template <typename ...T>
struct Reader : TemplateStubs, ReaderBase
{
	using Cont = unique_tuple::unique_tuple<T...>;
};

template <typename ...T>
struct Writer : TemplateStubs, WriterBase
{
	using Cont = unique_tuple::unique_tuple<T...>;
};

template <typename ...Entities>
struct Orchestrator : TemplateStubs, OrchestratorBase
{
	using Cont = unique_tuple::unique_tuple<Entities...>;
};

template <typename Base>
struct BaseTypeOfFilter
{
	template <typename Entity>
	static constexpr bool test()
	{
		return isEntity<Entity>() && std::is_base_of<Entity, Base>();
	}
};

template <typename Sys, typename T>
constexpr bool canOrchestrate()
{
	return isVirtual<T>()
		? std::tuple_size<
			Filter2<Flatten<Filter<typename Sys::Cont, OrchestratorBase>>, BaseTypeOfFilter<T>>
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
	using Cont = unique_tuple::unique_tuple<N...>;
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
	using Cont = unique_tuple::unique_tuple<Permissions...>;
};

template <typename Sys>
using SystemAccesses = Flatten<tuple_difference<typename Sys::Cont, Filter<typename Sys::Cont, DependencyBase>>>;
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
			OncePerType<Subset, SystemBlocker<Value>>::fn(barriers);

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
			OncePerType<Subset, SystemBlocker<Value>>::fn(barriers);
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
			OncePerType<Subset, DependeeBlocker<Value>>::fn(barriers);
			Accessor<Arch, Value> accessor(arch);
			Value::shutdown(accessor);
			barriers[*id].signal();
		}
	};

	static void systemCallback(RunTaskData<Arch>* data)
	{
		Switch<Subset>::template fn<SystemRunner>(data->id, &data->id, data->barriers, data->arch);
	}

	static void initializeSystemCallback(RunTaskData<Arch>* data)
	{
		Switch<Subset>::template fn<SystemInitialize>(data->id, &data->id, data->barriers, data->arch);
	}

	static void shutdownSystemCallback(RunTaskData<Arch>* data)
	{ 
		Switch<Subset>::template fn<SystemShutdown>(data->id, &data->id, data->barriers, data->arch);
	}

	template <typename T>
	static void runForSystems(Arch* arch, void (*fn)(RunTaskData<Arch>*), T* fls)
	{
		static const uint64_t sysCount = numUniques<Subset, SystemBase>();
		czsf::Barrier barriers[sysCount];
		RunTaskData<Arch> taskData[sysCount];

		for (uint64_t i = 0; i < sysCount; i++)
		{
			barriers[i] = czsf::Barrier(1);
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

template <typename Desc, typename ...Systems>
struct Architecture : VirtualArchitecture
{
	using Cont = Flatten<Rbox<Systems...>>;
	using This = Architecture<Desc, Systems...>;
	using OmniSystem = System <
		Rewrap<Orchestrator, Filter<Cont, EntityBase>>,
		Rewrap<Writer, Filter<Cont, ResourceBase>>
	>;

	Architecture()
	{
		static_assert(!isSystem<Desc>(), "The first template parameter for Architecture must be the inheriting object");
		// TODO assert all components are used in at least one entity
		// TODO assert every entity can be instantiated
		oncePerPair<Filter<Cont, SystemBase>, SystemDependencyCheck>(0);
		OncePerType<Cont, ConstructorCallback>::fn(this);
	}

	static constexpr uint64_t numSystems() { return numUniques<Cont, SystemBase>(); }
	static constexpr uint64_t numEntities() { return numUniques<Cont, EntityBase>(); }

	template <typename System>
	static constexpr uint64_t systemIndex()
	{
		return isSystem<System>() ? indexOf<Cont, System, SystemBase>() : -1;
	}

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
		static const uint64_t index = indexOf<Cont, Resource, ResourceBase>();
		resources[index] = res;
	}

	template <typename Resource>
	Resource* getResource()
	{
		// static_assert(isResource<Resource>(), "Template parameter must be a Resource.");
		static_assert(inspect::contains<Cont, Resource>(), "Architecture doesn't contain Resource.");
		static const uint64_t index = indexOf<Cont, Resource, ResourceBase>();
		return reinterpret_cast<Resource*>(resources[index]);
	}

	template <typename Entity>
	EntityStore<Entity>* getEntities()
	{
		// assert(isEntity<Entity>());
		return reinterpret_cast<EntityStore<Entity>*>(&entities[0]) + entityIndex<Entity>();
	}

	template <typename Category>
	static const char* name(uint64_t id)
	{
		static const char* d = "Unknown";
		const char** result = &d;
		OncePerType<Cont, NameFinder<Category>>::fn(id, result);
		return *result;
	}

	void* getEntity(Guid guid)
	{
		void* ret = nullptr;
		uint64_t tk = typeKey(guid);
		Switch<Filter<Cont, EntityBase>>::template fn<GetEntityVoidPtr>(tk, this, tk, guid, &ret);
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
				auto entities = arch->getEntities<T>();
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
		OncePerType<Filter<Cont, EntityBase>, ResolveGuid<Sys>>::fn(this, typeKey(guid), guidId(guid), f, result);
		return result;
	}

	template <typename Sys, typename F, typename ...Components>
	bool accessEntityFiltered(Guid guid, F f)
	{
		bool result = false;
		OncePerType<Filter2<Cont, ContainsAllComponentsFilter<Components...>>, ResolveGuid<Sys>>::fn(this, typeKey(guid), guidId(guid), f, result);
		return result;
	}

private:
	template <typename System, typename Entity>
	void postInitializeEntity(Guid guid, Entity* ent)
	{
		setEntityId(ent, guid.get());
		auto accessor = Accessor<Desc, System>(reinterpret_cast<Desc*>(this));
		OncePerType<typename Entity::Cont, OnCreateCallback>::fn(*ent, accessor);
		onCreate(*ent, accessor);
	}

	template <typename System, typename Entity, typename Context>
	void postInitializeEntity(Guid guid, Entity* ent, const Context& context)
	{
		setEntityId(ent, guid.get());
		auto accessor = Accessor<Desc, System>(reinterpret_cast<Desc*>(this));
		OncePerType<typename Entity::Cont, OnCreateContextCallback>::fn(*ent, accessor, context);
		onCreate(*ent, accessor, context);
	}

public:
	template <typename Entity, typename System>
	Entity* createEntity()
	{
		CZSS_CONST_IF (isEntity<Entity>())
		{
			auto ent = initializeEntity<Entity>();
			Guid guid = ent->getGuid();
			postInitializeEntity<System>(guid, ent);
			return ent;
		}

		return nullptr;
	}

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

	template <typename Entity>
	Entity* createEntity()
	{
		return createEntity<Entity, OmniSystem>();
	}

	template <typename Entity, typename ...Params>
	Entity* createEntity(Params&&... params)
	{
		return createEntity<Entity, OmniSystem>(std::forward(params)...);
	}

	template <typename Entity, typename System, typename Context>
	Entity* createEntityWithContext(const Context& context)
	{
		CZSS_CONST_IF (isEntity<Entity>())
		{
			auto ent = initializeEntity<Entity>();
			Guid guid = ent->getGuid();
			postInitializeEntity<System>(guid, ent, context);
			return ent;
		}

		return nullptr;
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

	template <typename Entity, typename Context>
	Entity* createEntityWithContext(const Context& context)
	{
		return createEntityWithContext<Entity, OmniSystem>(context);
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
		OncePerType<typename Entity::Cont, OnDestroyCallback>::fn(entity, accessor);
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
		Switch<Filter<Cont, EntityBase>>::template fn<EntityDestructorCallback<System>>(tk, this, id);
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
		OncePerType<unique_tuple::unique_tuple<Entities...>, DestroyEntitiesCallback>(this);
	}

	template <typename T>
	void run(T* fls)
	{
		Runner<Desc, Filter<Cont, SystemBase>>::template runForSystems(reinterpret_cast<Desc*>(this), Runner<Desc, Filter<Cont, SystemBase>>::systemCallback, fls);
	}

	template <typename T>
	void initialize(T* fls)
	{
		Runner<Desc, Filter<Cont, SystemBase>>::template runForSystems(reinterpret_cast<Desc*>(this), Runner<Desc, Filter<Cont, SystemBase>>::initializeSystemCallback, fls);
	}

	template <typename T>
	void shutdown(T* fls)
	{
		Runner<Desc, Filter<Cont, SystemBase>>::template runForSystems(reinterpret_cast<Desc*>(this), Runner<Desc, Filter<Cont, SystemBase>>::shutdownSystemCallback, fls);
	}

	void run()
	{
		Runner<Desc, Filter<Cont, SystemBase>>::template runForSystems<Dummy>(reinterpret_cast<Desc*>(this), Runner<Desc, Filter<Cont, SystemBase>>::systemCallback, nullptr);
	}

	void initialize()
	{
		Runner<Desc, Filter<Cont, SystemBase>>::template runForSystems<Dummy>(reinterpret_cast<Desc*>(this), Runner<Desc, Filter<Cont, SystemBase>>::initializeSystemCallback, nullptr);
	}

	void shutdown()
	{
		Runner<Desc, Filter<Cont, SystemBase>>::template runForSystems<Dummy>(reinterpret_cast<Desc*>(this), Runner<Desc, Filter<Cont, SystemBase>>::shutdownSystemCallback, nullptr);
	}

	template <typename Sys>
	void run()
	{
		Sys::run(Accessor<Desc, Sys>(reinterpret_cast<Desc*>(this)));
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

	~Architecture()
	{
		OncePerType<Cont, DestructorCallback>::fn(this);
	}

	template <typename V>
	static constexpr uint64_t absoluteIndex()
	{
		return tuple_type_index<V, Cont>::value;
	}

	static std::string dotGraphString()
	{
		std::string ret("digraph {\n");

		OncePerType<Cont, DotGenerateNodes>::fn(&ret);
		using Filtered = tuple_union< Filter<Cont, EntityBase>, Filter<Cont, SystemBase>, Filter<Cont, ComponentBase>, Filter<Cont, IteratorBase>, Filter<Cont, ResourceBase>>;
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
	void* resources[max(numUniques<Cont, ResourceBase>(), uint64_t(1))] = {0};
	char entities[max(numUniques<Cont, EntityBase>(), uint64_t(1)) * sizeof(EntityStore<uint64_t>)] = {0};

	template <typename Entity>
	struct InitializeEntityCallback
	{
		template <typename T>
		static void callback(This* arch, uint64_t& id, uint64_t& tk, Entity*& ent)
		{
			tk = indexOf<Cont, T, EntityBase>() << 63 - typeKeyLength();
			auto entities = arch->getEntities<T>();
			ent = entities->template create<Entity>(id);
		}

		template <typename T, typename ...Params>
		static void callback(This* arch, uint64_t& id, uint64_t& tk, Entity*& ent, Params&&... params)
		{
			tk = indexOf<Cont, T, EntityBase>() << 63 - typeKeyLength();
			auto entities = arch->getEntities<T>();
			ent = entities->template create<Entity>(id, std::forward<Params>(params)...);
		}
	};

	template <typename Entity>
	Entity* initializeEntity()
	{
		static_assert(isEntity<Entity>(), "Template parameter must be an Entity.");
		Entity* ent;
		uint64_t id, tk;
		OncePerType<Filter2<Cont, BaseTypeOfFilter<Entity>>, InitializeEntityCallback<Entity>>::fn(this, id, tk, ent);

		id += tk;
		setEntityId(ent, id);

		return ent;
	}

	template <typename Entity, typename ...Params>
	Entity* initializeEntity(Params&&... params)
	{
		static_assert(isEntity<Entity>(), "Template parameter must be an Entity.");

		Entity* ent;
		uint64_t id, tk;
		OncePerType<Filter2<Cont, BaseTypeOfFilter<Entity>>, InitializeEntityCallback<Entity>>::fn(this, id, tk, ent, std::forward<Params>(params)...);

		id += tk;
		setEntityId(ent, id);

		return ent;
	}

	struct DestroyEntitiesCallback
	{
		template <typename Value>
		static void callback(This* arch)
		{
			EntityStore<Value>* entities = arch->getEntities<Value>();
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

	struct ConstructorCallback
	{
		template <typename Value>
		static inline void callback(This* arch)
		{
			CZSS_CONST_IF (isEntity<Value>())
			{
				auto p = arch->template getEntities<Value>();
				new(p) EntityStore<Value>();
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
		template <typename Value, typename Inner, typename Next>
		static constexpr bool inspect()
		{
			return isSystem<Value>() && !inspect::contains<Cont, Value>()
				|| Next::template evaluate<bool, SystemsCompleteCheck>()
				|| Inner::template evaluate<bool, SystemsCompleteCheck>();
		}
	};

	struct SystemDependencyIterator
	{
		template <typename Value, typename Inner, typename Next>
		static constexpr bool inspect()
		{
			return (Value::Dep::template evaluate<bool, SystemsCompleteCheck>())
				|| Next::template evaluate<bool, SystemDependencyIterator>();
		}
	};

	struct SystemDependencyCheck
	{
		template <typename A, typename B>
		static void callback(int i)
		{
			static_assert(!std::is_same<A, B>() && (exclusiveWith<A, B>() || exclusiveWith<B, A>())
				? (dependsOn<A, B>() || dependsOn<B, A>())
				: true,
				"Explicit dependency missing between systems."
			);
		}
	};

	struct GetEntityVoidPtr
	{
		template <typename Value>
		inline static void callback(This* arch, const uint64_t& typeKey, const Guid& guid, void** ret)
		{
			if(isEntity<Value>() && inspect::contains<Cont, Value>() && indexOf<Cont, Value, EntityBase>() == typeKey)
				*ret = arch->getEntity<Value>(guid);
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

template<typename Sys, typename Entity>
struct TypedEntityAccessor
{
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

	Guid guid() const
	{
		return _entity->getGuid();
	}

	const Entity* viewEntity() const
	{
		OncePerType<Flatten<typename Entity::Cont>, HasEntityReadPermissionCallback<Sys>>::fn();
		return _entity;
	}

	Entity* getEntity()
	{
		OncePerType<Flatten<typename Entity::Cont>, HasEntityWritePermissionCallback<Sys>>::fn();
		return _entity;
	}

private:
	Entity* _entity;
};

template <typename Arch, typename Sys>
struct EntityAccessor
{
	template <typename Entity>
	EntityAccessor(const Entity* ent)
	{
		entity = const_cast<Entity*>(ent);
		typeKey = indexOf<typename Arch::Cont, Entity, EntityBase>();
	}

	template <typename Component>
	bool hasComponent() const
	{
		return view<Component>() != nullptr;
	}

	template<typename Component>
	const Component* view() const
	{
		static_assert(canRead<Sys, Component>(), "System lacks read permissions for the Iterator's components.");
		return get_ptr<Component>();
	}

	template<typename Component>
	Component* get()
	{
		static_assert(canWrite<Sys, Component>(), "System lacks write permissions for the Iterator's components.");
		return get_ptr<Component>();
	}

	Guid guid() const
	{
		Guid guid;

		Switch<Filter<typename Arch::Cont, EntityBase>>::template fn<GuidGetter>(typeKey, &guid, entity);
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
		Switch<Filter<typename Arch::Cont, EntityBase>>::template fn<ComponentGetter<Component>>(typeKey, entity, &result);
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
			OncePerType<typename Value::Cont, Callback<Component>>::fn(reinterpret_cast<Value*>(entity), result);
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
	const Component* view() const
	{
		return EntityAccessor<Arch, Sys>::template view<Component>();
	}

	template<typename Component>
	Component* get()
	{
		return EntityAccessor<Arch, Sys>::template get<Component>();
	}

private:
	friend IteratorIterator<Iter, Arch, Sys>;
	friend Accessor<Arch, Sys>;

	IteratorAccessor() {}
	template <typename Entity>
	IteratorAccessor(Entity* ent) : EntityAccessor<Arch, Sys>(ent) { }
	IteratorAccessor(uint64_t key, void* entity) : EntityAccessor<Arch, Sys>(key, entity) { }
};

template <typename Iter, typename Arch, typename Sys>
struct IteratorIterator
{
	struct CompatabilityFilter
	{
		template <typename Entity>
		static constexpr bool test()
		{
			return isEntity<Entity>() && isIteratorCompatibleWithEntity<Iter, Entity>();
		}
	};

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
		using CompatibleEntities = Filter2<typename Arch::Cont, CompatabilityFilter>;
		Switch<CompatibleEntities>::template fn<IncrementerCallback>(typeKey, this);

		while (index >= maxIndex && typeKey < limit())
		{
			typeKey++;
			if (typeKey < limit())
				Switch<CompatibleEntities>::template fn<IncrementerCallback>(typeKey, this);
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
		using CompatibleEntities = Filter2<typename Arch::Cont, CompatabilityFilter>;
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

	template <typename Entity>
	Entity* createEntity()
	{
		entityPermission<Entity>();
		return arch->template createEntity<Entity>();
	}

	template <typename Entity, typename ...Params>
	Entity* createEntity(Params&&... params)
	{
		entityPermission<Entity>();
		return arch->template createEntity<Entity>(std::forward<Params>(params)...);
	}

	template <typename Entity, typename Context>
	Entity* createEntityWithContext(Context&& context)
	{
		entityPermission<Entity>();
		return arch->template createEntityWithContext<Entity>(context);
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
		OncePerType<Flatten<typename Entity::Cont>, HasEntityReadPermissionCallback<Sys>>::fn();
		static_assert(inspect::contains<typename Arch::Cont, Entity>(), "Architecture doesn't contain the Entity.");
		return arch->template getEntity<Entity>(guid);
	}

	template <typename Entity>
	const Entity* viewEntity(EntityId<Arch, Entity> id) const
	{
		static_assert(isEntity<Entity>(), "Attempted to create non-entity.");
		OncePerType<Flatten<typename Entity::Cont>, HasEntityReadPermissionCallback<Sys>>::fn();
		static_assert(inspect::contains<typename Arch::Cont, Entity>(), "Architecture doesn't contain the Entity.");
		return arch->template getEntity(id);
	}

	template <typename Entity>
	Entity* getEntity(Guid guid)
	{
		static_assert(isEntity<Entity>(), "Attempted to create non-entity.");
		OncePerType<Flatten<typename Entity::Cont>, HasEntityWritePermissionCallback<Sys>>::fn();
		static_assert(inspect::contains<typename Arch::Cont, Entity>(), "Architecture doesn't contain the Entity.");
		return arch->template getEntity<Entity>(guid);
	}

	template <typename Entity>
	Entity* getEntity(EntityId<Arch, Entity> id)
	{
		static_assert(isEntity<Entity>(), "Attempted to create non-entity.");
		OncePerType<Flatten<typename Entity::Cont>, HasEntityWritePermissionCallback<Sys>>::fn();
		static_assert(inspect::contains<typename Arch::Cont, Entity>(), "Architecture doesn't contain the Entity.");
		return arch->template getEntity(id);
	}

	template <typename Entity>
	EntityId<Arch, Entity> getEntityId(const Entity* ent) const
	{
		return Arch::getEntityId(ent);
	}

	void destroyEntity(Guid guid)
	{
		if (OncePerType<typename Arch::Cont, EntityDestructionPermission>::constFn(Arch::typeKey(guid)))
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
		OncePerType<Rbox<Entities...>, EntityOrchestrationPermissionTest>::fn();
		arch->template destroyEntities<Entities...>();
	}

	template <typename T, typename ...Systems>
	void run(T* fls, bool init)
	{
		// Rbox<Systems...>::template evaluate<Filter<TestAlwaysTrue, TestIfRbox, SystemsRunPermissionTest<Rbox<Systems...>>>>();

		if (init)
			Runner<Arch, Rbox<Systems...>>::runForSystems(arch, Runner<Arch, Rbox<Systems...>>::initializeSystemCallback, fls);

		Runner<Arch, Rbox<Systems...>>::runForSystems(arch, Runner<Arch, Rbox<Systems...>>::systemCallback, fls);

		if (init)
			Runner<Arch, Rbox<Systems...>>::runForSystems(arch, Runner<Arch, Rbox<Systems...>>::shutdownSystemCallback, fls);
	}

	template <typename ...Systems>
	void run(bool init)
	{
		run<Dummy, Systems...>(nullptr, init);
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
		iteratorPermission<Iterator>();
		OncePerType<typename Arch::Cont, IteratorCallback<Iterator>>::fn(f, arch);
	}

	template <typename Iterator, typename F>
	void iterate2(F f)
	{
		iteratorPermission<Iterator>();
		OncePerType<typename Arch::Cont, TypedIteratorCallback<Iterator>>::fn(f, arch);
	}

	template <typename Iterator, typename F>
	void parallelIterate(uint64_t numTasks, F f)
	{
		parallelIterateImpl<Iterator>(numTasks, f, ParallelIterateTask<Iterator, F>);
	}

	template <typename Iterator, typename F>
	void parallelIterate2(uint64_t numTasks, F f)
	{
		parallelIterateImpl<Iterator>(numTasks, f, TypedParallelIterateTask<Iterator, F>);
	}

	template <typename Iterator>
	uint64_t countCompatibleEntities()
	{
		uint64_t entityCount = 0;
		OncePerType<Filter<typename Arch::Cont, EntityBase>, EntityCountCallback<Iterator>>::fn(&entityCount, arch);
		return entityCount;
	}

	template <typename Iterator>
	static constexpr uint64_t numCompatibleEntities()
	{
		return OncePerType<typename Arch::Cont, NumCompatibleEntities<Iterator>>::constSum();
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
			typename V::Cont::template evaluate<OncePerType<Dummy, SystemAccessValidationCallback<V, Systems>>>();
		}
	};

	template <typename Iterator>
	struct IteratorCallback
	{
		template <typename Value, typename F>
		static inline void callback(F& f, Arch* arch)
		{
			CZSS_CONST_IF (isEntity<Value>() && isIteratorCompatibleWithEntity<Iterator, Value>())
			{
				auto ents = arch->template getEntities<Value>();
				for (auto& ent : ents->used_indices)
				{
					IteratorAccessor<Iterator, Arch, Sys> accessor(ent);
					auto lambda = f;
					lambda(accessor);
				}
			}
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
					auto lambda = f;
					lambda(accessor);
				}
			}
		}
	};

	template <typename Iterator>
	struct EntityCountCallback
	{
		template <typename Entity>
		static inline void callback(uint64_t* count, Arch* arch)
		{
			CZSS_CONST_IF (isIteratorCompatibleWithEntity<Iterator, Entity>())
			{
				auto entities = arch->template getEntities<Entity>();
				*count += entities->size();
			}
		}
	};

	template <typename Iterator>
	struct NumCompatibleEntities
	{
		template <typename Value>
		static constexpr uint64_t callback()
		{
			return isEntity<Value>() && isIteratorCompatibleWithEntity<Iterator, Value>() ? 1 : 0;
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
				F lambda = *data->func;
				lambda(data->index, accessor);
			}

			data->entityCount -= handled;
			data->beginIndex = 0;
		}
	};

	template <typename Iterator, typename F>
	static void ParallelIterateTask(ParallelIterateTaskData<F>* data)
	{
		static const uint64_t limit = Accessor<Arch, Sys>::numCompatibleEntities<Iterator>();

		for (uint64_t i = 0; i < limit; i++)
		{
			Switch<Filter<typename Arch::Cont, EntityBase>>::template fn<ParallelIterateTaskCallback<Iterator, F>>(i, data);
			if (data->entityCount == 0)
				return;
		}
	}

	template <typename Iterator, typename F>
	struct TypedParallelIterateTaskCallback
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
		static const uint64_t limit = Accessor<Arch, Sys>::numCompatibleEntities<Iterator>();

		for (uint64_t i = 0; i < limit; i++)
		{
			Switch<Filter<typename Arch::Cont, EntityBase>>::template fn<TypedParallelIterateTaskCallback<Iterator, F>>(i, data);
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
	return OncePerType<SystemAccesses<A>, SystemExclusiveCheck<B>>::constFn()
		|| OncePerType<SystemAccesses<B>, SystemExclusiveCheck<A>>::constFn();
}

} // namespace czss

template<>
struct std::hash<czss::Guid>
{
	std::size_t operator()(czss::Guid const& guid) const noexcept
	{
		return guid.get();
	}
};


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

Guid::Guid(const Guid& other)
{
	this->id = other.id;
}

uint64_t Guid::get() const
{
	return id;
}

bool Guid::operator ==(const Guid& other) const
{
	return this->id == other.id;
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