#pragma once

#include "external/unique_tuple/unique_tuple.hpp"

#include <tuple>
#include <type_traits>

template <typename Tuple, typename ...R>
struct tuple_union_impl;

template <typename ...A, typename ...B, typename ...R>
struct tuple_union_impl<std::tuple<A...>, std::tuple<B...>, R...>
{
	using type = typename tuple_union_impl<std::tuple<A..., B...>, R...>::type;
};

template <typename ...A, typename ...B>
struct tuple_union_impl<std::tuple<A...>, std::tuple<B...>>
{
	using type = unique_tuple::unique_tuple<A..., B...>;
};

template <typename ...V>
using tuple_union = typename tuple_union_impl<V...>::type;

template <typename V, typename T>
struct tuple_contains_impl;

template <typename V>
struct tuple_contains_impl<V, std::tuple<>> : std::false_type {};

template <typename V, typename A, typename ...R>
struct tuple_contains_impl<V, std::tuple<A, R...>> : tuple_contains_impl<V, std::tuple<R...>> {};

template <typename V, typename ...R>
struct tuple_contains_impl<V, std::tuple<V, R...>> : std::true_type {};

template <typename Tuple, typename V>
struct tuple_contains
{
	static constexpr bool value = tuple_contains_impl<V, Tuple>::value;
};

template <typename V, typename Tuple>
struct tuple_type_index_impl;

template <typename V, typename ...R>
struct tuple_type_index_impl<V, std::tuple<V, R...>>
{
	static const std::size_t index = 0;
};

template <typename V, typename A, typename ...R>
struct tuple_type_index_impl<V, std::tuple<A, R...>>
{
	static const std::size_t index = 1 + tuple_type_index_impl<V, std::tuple<R...>>::index;
};

template <typename V>
struct tuple_type_index_impl<V, std::tuple<>>
{
	static const std::size_t index = 0;
};

template <typename V, typename Tuple>
struct tuple_type_index
{
	static constexpr std::size_t value = tuple_type_index_impl<V, Tuple>::index;
};

template <typename A, typename B, typename ...R>
struct tuple_difference_impl;

template <typename ...A, typename B, typename V, typename ...R>
struct tuple_difference_impl<std::tuple<A...>, B, V, R...>
{
	using type = typename std::conditional<
		tuple_contains<B, V>::value
		, tuple_difference_impl<std::tuple<A...>, B, R...>
		, tuple_difference_impl<std::tuple<V, A...>, B, R...>
	>::type::type;
};

template <typename ...A, typename B, typename V>
struct tuple_difference_impl<std::tuple<A...>, B, V>
{
	using type = typename std::conditional<
		tuple_contains<B, V>::value
		, unique_tuple::unique_tuple<A...>
		, unique_tuple::unique_tuple<V, A...>
	>::type;
};

template <typename ...A, typename B>
struct tuple_difference_impl<std::tuple<A...>, B>
{
	using type = std::tuple<>;
};

template <typename A, typename B>
struct tuple_difference_impl_2;

template <typename A, typename ...B>
struct tuple_difference_impl_2<A, std::tuple<B...>>
{
	using type = typename tuple_difference_impl<std::tuple<>, A, B...>::type;
};

template <typename A, typename B>
using tuple_difference = typename tuple_difference_impl_2<B, A>::type;

template <typename A, typename B>
using tuple_symmetric_difference = tuple_union<tuple_difference<tuple_union<A, B>, A>, tuple_difference<tuple_union<A, B>, B>>;

template <typename A, typename B>
using tuple_intersect = tuple_difference<tuple_union<A, B>, tuple_symmetric_difference<A, B>>;

template <typename Tuple, typename B, typename ...R>
struct cartesian_product_impl;

template <typename ...A, typename B, typename V, typename ...R>
struct cartesian_product_impl<std::tuple<A...>, B, V, R...>
{
	using type = typename cartesian_product_impl<std::tuple<std::tuple<B, V>, A...>, B, R...>::type;
};

template <typename ...A, typename B, typename V>
struct cartesian_product_impl<std::tuple<A...>, B, V>
{
	using type = std::tuple<std::tuple<B, V>, A...>;
};

template <typename A, typename B, typename ...R>
struct cartesian_product_impl_2;

template <typename ...A, typename ...B, typename V, typename ...R>
struct cartesian_product_impl_2<std::tuple<A...>, std::tuple<B...>, V, R...>
{
	using type = typename cartesian_product_impl_2<typename cartesian_product_impl<std::tuple<A...>, V, B...>::type, std::tuple<B...>, R... >::type;
};

template <typename ...A, typename ...B, typename V>
struct cartesian_product_impl_2<std::tuple<A...>, std::tuple<B...>, V>
{
	using type = typename cartesian_product_impl<std::tuple<A...>, V, B...>::type;
};

template <typename A, typename B>
struct cartesian_product_impl_3;

template <typename ...A, typename ...B>
struct cartesian_product_impl_3<std::tuple<A...>, std::tuple<B...>>
{
	using type = typename cartesian_product_impl_2<std::tuple<>, std::tuple<B...>, A...>::type;
};

template <typename A, typename B>
using cartesian_product = typename cartesian_product_impl_3<A, B>::type;

template <typename F, typename A, typename ...R>
struct subset_impl;

template <typename F, typename ...A, typename V, typename ...R>
struct subset_impl<F, std::tuple<A...>, V, R...>
{
	using type = typename std::conditional<
		F::template test<V>()
		, subset_impl<F, std::tuple<V, A...>, R...>
		, subset_impl<F, std::tuple<A...>, R...>
	>::type::type;
};

template <typename F, typename ...A, typename V>
struct subset_impl<F, std::tuple<A...>, V>
{
	using type = typename std::conditional<
		F::template test<V>()
		, unique_tuple::unique_tuple<V, A...>
		, unique_tuple::unique_tuple<A...>
	>::type;
};

template <typename T, typename F>
struct subset_impl_2;

template <typename ...A, typename F>
struct subset_impl_2<std::tuple<A...>, F>
{
	using type = typename subset_impl<F, std::tuple<>, A...>::type;
};

template<typename Tuple, typename F>
using subset = typename subset_impl_2<Tuple, F>::type;
