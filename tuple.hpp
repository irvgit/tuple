#pragma once

#if __cplusplus < 202302L
    #error out of date c++ version, compile with -stdc++=2c
#endif

#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>
#include <tuple>

//change impls std::tuple_element_t to pack indexing when g++15 is out, then std::tuple will not be needed other than to specialize

namespace tpl {
    namespace detail {
        template <std::size_t tp_index, class... tp_types_ts>
        struct tuple_impl : tuple_impl<tp_index + 1, tp_types_ts...> {  [[no_unique_address]] std::tuple_element_t<sizeof...(tp_types_ts) - 1 - tp_index, std::tuple<tp_types_ts...>> m_value; };
        template <class... tp_types_ts>
        struct tuple_impl<sizeof...(tp_types_ts) - 1, tp_types_ts...> { [[no_unique_address]] std::tuple_element_t<0, std::tuple<tp_types_ts...>> m_value; };
        template <std::size_t tp_index>
        struct tuple_impl<tp_index> {};
        template <class... tp_types_ts>
        tuple_impl(tp_types_ts...) -> tuple_impl<0, tp_types_ts...>;
    }
    template <class... tp_types_ts>
    struct tuple : detail::tuple_impl<0, tp_types_ts...> {};
    template <class... tp_types_ts>
    tuple(tp_types_ts...) -> tuple<tp_types_ts...>;
    
    namespace detail {
        template <typename tp_type_t>
        struct is_tuple : std::false_type{};
        template <class... ts>
        struct is_tuple<tuple<ts...>> : std::true_type{};
        
        template <typename tp_type_t>
        concept tuple_non_cvref = is_tuple<std::remove_cvref_t<tp_type_t>>::value && std::same_as<tp_type_t, std::remove_cvref_t<tp_type_t>>;
        template <typename tp_type_t>
        concept tuple_or_cv = is_tuple<std::remove_cvref_t<tp_type_t>>::value && std::same_as<tp_type_t, std::remove_reference_t<tp_type_t>>;
        template <typename tp_type_t>
        concept tuple_or_cvref = is_tuple<std::remove_cvref_t<tp_type_t>>::value;
        
        template <tuple_or_cv tp_tuple_t>
        struct tuple_size;
        template <class... tp_types_ts>
        struct tuple_size<tuple<tp_types_ts...>> : std::integral_constant<std::size_t, sizeof...(tp_types_ts)> {};
        template <tuple_or_cv tp_tuple_t>
        auto constexpr tuple_size_v = tuple_size<std::remove_cv_t<tp_tuple_t>>::value;

        template <std::size_t tp_index, typename tp_tuple_t>
        struct get_tuple_impl;
        template <std::size_t tp_index, class... tp_tuple_ts>
        struct get_tuple_impl<tp_index, tuple<tp_tuple_ts...>> : std::type_identity<tuple_impl<tp_index, tp_tuple_ts...>> {};
        template <std::size_t tp_index, typename tp_tuple_t>
        using get_tuple_impl_t = typename get_tuple_impl<tp_index, std::remove_cvref_t<tp_tuple_t>>::type;

        template <typename tp_tuple_impl_t, typename tp_qualified_tuple_t>
        struct tuple_element {
            using element_t                 = decltype(tp_tuple_impl_t::m_value);
            using with_const_t              = std::conditional_t<std::is_const_v<tp_qualified_tuple_t>, std::add_const_t<element_t>, element_t>;
            using with_const_and_volatile_t = std::conditional_t<std::is_volatile_v<tp_qualified_tuple_t>, std::add_volatile_t<with_const_t>, with_const_t>;
            using type                      = with_const_and_volatile_t;
        };
        template <std::size_t tp_index, tuple_or_cv tp_tuple_t>
        using tuple_element_t = typename tuple_element<get_tuple_impl_t<std::tuple_size_v<std::remove_reference_t<tp_tuple_t>> - 1 - tp_index, tp_tuple_t>, tp_tuple_t>::type;

        inline namespace adl_exposed {
            template <std::size_t tp_index, tuple_or_cvref tp_tuple_t>
            auto constexpr get(tp_tuple_t&& p_tuple) noexcept -> auto&& {
                return std::forward<tp_tuple_t>(p_tuple).get_tuple_impl_t<std::tuple_size_v<std::remove_reference_t<tp_tuple_t>> - 1 - tp_index, tp_tuple_t>::m_value;
            }
        }
    }

    namespace extra {
        template <typename tp_type_t>
        concept tuple_like = requires (tp_type_t p_value) {
            requires std::tuple_size_v<tp_type_t> != 0;
            typename std::tuple_element_t<0, tp_type_t>;
            get<0>(p_value);
        };
    }

    namespace detail {
        template <std::size_t tp_index, typename tuple_type_t>
        using get_t = decltype(get<tp_index>(std::declval<tuple_type_t>()));

        template <typename tp_type_t>
        concept tuple_like_or_cv = extra::tuple_like<std::remove_cvref_t<tp_type_t>>;
        template <typename tp_type_t>
        concept tuple_like_or_cvref = extra::tuple_like<std::remove_cvref_t<tp_type_t>>;

        template <typename tp_tuple_like1_t, typename tp_tuple_like2_t>
        concept tuples_same_size = std::tuple_size_v<std::remove_reference_t<tp_tuple_like1_t>> == std::tuple_size_v<std::remove_reference_t<tp_tuple_like2_t>>;
        
        template <typename, typename, typename>
        struct tuplewise_constructable_from_impl : std::false_type {};
        template <typename tp_tuple_like1_t, typename tp_tuple_like2_t, std::size_t... tp_is>
        struct tuplewise_constructable_from_impl<std::index_sequence<tp_is...>, tp_tuple_like1_t, tp_tuple_like2_t> : std::bool_constant<(... && std::constructible_from<std::tuple_element_t<tp_is, tp_tuple_like1_t>, get_t<tp_is, tp_tuple_like2_t>>)> {};
        template <typename tp_tuple_like1_t, typename tp_tuple_like2_t>
        concept tuplewise_constructable_from = tuples_same_size<tp_tuple_like1_t, tp_tuple_like2_t> && tuplewise_constructable_from_impl<std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<tp_tuple_like1_t>>>, std::remove_cvref_t<tp_tuple_like1_t>, tp_tuple_like2_t>::value;

        template <typename, typename, typename>
        struct nothrow_tuplewise_constructable_from_impl : std::false_type {};
        template <typename tp_tuple_like1_t, typename tp_tuple_like2_t, std::size_t... tp_is>
        struct nothrow_tuplewise_constructable_from_impl<std::index_sequence<tp_is...>, tp_tuple_like1_t, tp_tuple_like2_t> : std::bool_constant<(... && std::is_nothrow_constructible_v<std::tuple_element_t<tp_is, tp_tuple_like1_t>, get_t<tp_is, tp_tuple_like2_t>>)> {};
        template <typename tp_tuple_like1_t, typename tp_tuple_like2_t>
        concept nothrow_tuplewise_constructable_from = tuples_same_size<tp_tuple_like1_t, tp_tuple_like2_t> && nothrow_tuplewise_constructable_from_impl<std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<tp_tuple_like1_t>>>, std::remove_cvref_t<tp_tuple_like1_t>, tp_tuple_like2_t>::value;
        
        template <typename, typename, typename>
        struct tuplewise_assignable_from_impl : std::false_type {};
        template <typename tp_tuple_like1_t, typename tp_tuple_like2_t, std::size_t... tp_is>
        struct tuplewise_assignable_from_impl<std::index_sequence<tp_is...>, tp_tuple_like1_t, tp_tuple_like2_t> : std::bool_constant<(... && std::assignable_from<get_t<tp_is, tp_tuple_like1_t>, get_t<tp_is, tp_tuple_like2_t>>)> {};
        template <typename tp_tuple_like1_t, typename tp_tuple_like2_t>
        concept tuplewise_assignable_from = tuples_same_size<tp_tuple_like1_t, tp_tuple_like2_t> && tuplewise_assignable_from_impl<std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<tp_tuple_like1_t>>>, tp_tuple_like1_t, tp_tuple_like2_t>::value;

        template <typename, typename, typename>
        struct nothrow_tuplewise_assignable_from_impl : std::false_type {};
        template <typename tp_tuple_like1_t, typename tp_tuple_like2_t, std::size_t... tp_is>
        struct nothrow_tuplewise_assignable_from_impl<std::index_sequence<tp_is...>, tp_tuple_like1_t, tp_tuple_like2_t> : std::bool_constant<(... && std::is_nothrow_assignable_v<get_t<tp_is, tp_tuple_like1_t>, get_t<tp_is, tp_tuple_like2_t>>)> {};
        template <typename tp_tuple_like1_t, typename tp_tuple_like2_t>
        concept nothrow_tuplewise_assignable_from = tuples_same_size<tp_tuple_like1_t, tp_tuple_like2_t> && nothrow_tuplewise_assignable_from_impl<std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<tp_tuple_like1_t>>>, tp_tuple_like1_t, tp_tuple_like2_t>::value;

        template <typename, typename, typename, typename = void>
        struct tuplewise_invokable_impl : std::false_type {};
        template <typename tp_invokable_t, typename tp_tuple_like_t, std::size_t... tp_is>
        struct tuplewise_invokable_impl<std::index_sequence<tp_is...>, tp_invokable_t, tp_tuple_like_t, std::enable_if_t<std::invocable<tp_invokable_t, get_t<tp_is, tp_tuple_like_t>...>>> : std::true_type {};
        template <typename tp_invokable_t, typename tp_tuple_like_t>
        concept tuplewise_invokable = tuplewise_invokable_impl<std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<tp_tuple_like_t>>>, tp_invokable_t, tp_tuple_like_t>::value;

        template <typename, typename, typename, typename = void>
        struct nothrow_tuplewise_invokable_impl : std::false_type {};
        template <typename tp_invokable_t, typename tp_tuple_like_t, std::size_t... tp_is>
        struct nothrow_tuplewise_invokable_impl<std::index_sequence<tp_is...>, tp_invokable_t, tp_tuple_like_t, std::enable_if_t<std::is_nothrow_invocable_v<tp_invokable_t, get_t<tp_is, tp_tuple_like_t>...>>> : std::true_type {};
        template <typename tp_invokable_t, typename tp_tuple_like_t>
        concept nothrow_tuplewise_invokable = nothrow_tuplewise_invokable_impl<std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<tp_tuple_like_t>>>, tp_invokable_t, tp_tuple_like_t>::value;
    }

    namespace detail {
        template <std::size_t tp_index>
        struct get_fn {
            template <tuple_like_or_cvref tp_tuple_like_t>
            auto constexpr operator()(tp_tuple_like_t&& p_tuple_like) const noexcept -> auto&& {
                return get<tp_index>(std::forward<tp_tuple_like_t>(p_tuple_like));
            }
        };
    }
    template <std::size_t tp_index>
    auto constexpr get = detail::get_fn<tp_index>{};

    namespace detail {
        struct apply_fn {
            template <detail::tuple_like_or_cvref tp_tuple_like_t, tuplewise_invokable<tp_tuple_like_t> tp_callable_t>
            auto constexpr operator()(tp_callable_t&& p_callable, tp_tuple_like_t&& p_tuple_like) const noexcept(nothrow_tuplewise_invokable<tp_callable_t, tp_tuple_like_t>) -> decltype(auto) {
                return []<std::size_t... tp_is>(std::index_sequence<tp_is...>, auto&& p_callable, auto&& p_tuple_like) {
                    static_assert(std::invocable<tp_callable_t, get_t<tp_is, tp_tuple_like_t>...>, "tp_callable_t not invokable with tuple elements"); //change to c++26 compile time exception
                    return std::invoke(std::forward<tp_callable_t>(p_callable), get<tp_is>(std::forward<tp_tuple_like_t>(p_tuple_like))...);
                }(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<tp_tuple_like_t>>>{}, std::forward<tp_callable_t>(p_callable), std::forward<tp_tuple_like_t>(p_tuple_like));
            }
        };
    }
    auto constexpr apply = detail::apply_fn{};

    namespace detail {
        struct forward_as_tuple_fn {
            template <class... tp_types_ts>
            auto constexpr operator()(tp_types_ts&&... p_arguments) const noexcept -> tuple<tp_types_ts&&...> {
                return tuple<tp_types_ts&&...>{std::forward<tp_types_ts>(p_arguments)...};
            }
        };
    }
    auto constexpr forward_as_tuple = detail::forward_as_tuple_fn{};

    namespace extra {
        auto constexpr empty_tuple = tuple{};

        namespace detail {
            struct as_forwarding_tuple_fn {
                template <tpl::detail::tuple_like_or_cvref tp_tuple_like_t>
                auto constexpr operator()(tp_tuple_like_t&& p_tuple_like) const noexcept -> auto {
                    return apply([](auto&&... p_arguments) { return forward_as_tuple(std::forward<decltype(p_arguments)>(p_arguments)...); }, std::forward<tp_tuple_like_t>(p_tuple_like));
                }
            };
        }
        auto constexpr as_forwarding_tuple = detail::as_forwarding_tuple_fn{};
        
        namespace detail {
            struct to_std_tuple_fn {
                template <tpl::detail::tuple_like_or_cvref tp_tuple_like_t>
                requires (tpl::detail::tuplewise_constructable_from<tp_tuple_like_t, tp_tuple_like_t>)
                auto constexpr operator()(tp_tuple_like_t&& p_tuple_like) const noexcept(tpl::detail::nothrow_tuplewise_constructable_from<tp_tuple_like_t, tp_tuple_like_t>) -> auto {
                    return []<std::size_t... tp_is>(std::index_sequence<tp_is...>, auto&& p_tuple_like) {
                        return std::tuple<std::tuple_element_t<tp_is, std::remove_reference_t<tp_tuple_like_t>>...>{get<tp_is>(std::forward<tp_tuple_like_t>(p_tuple_like))...};
                    }(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<tp_tuple_like_t>>>{}, std::forward<tp_tuple_like_t>(p_tuple_like));
                }
            };
        }
        auto constexpr to_std_tuple = detail::to_std_tuple_fn{};

        namespace detail {
            struct assign_fn {
                template <tpl::detail::tuple_like_or_cvref tp_tuple_like1_t, tpl::detail::tuple_like_or_cvref tp_tuple_like2_t>
                requires (tpl::detail::tuplewise_assignable_from<tp_tuple_like1_t, tp_tuple_like2_t>)
                auto constexpr operator()(tp_tuple_like1_t&& p_tuple_like_to, tp_tuple_like2_t&& p_tuple_like_from) const noexcept(tpl::detail::tuplewise_assignable_from<tp_tuple_like1_t, tp_tuple_like2_t>) -> assign_fn {
                    []<std::size_t... tp_is>(std::index_sequence<tp_is...>, auto&& p_tuple_like_to, auto&& p_tuple_like_from) {
                        (... , (get<tp_is>(std::forward<decltype(p_tuple_like_to)>(p_tuple_like_to)) = get<tp_is>(std::forward<decltype(p_tuple_like_from)>(p_tuple_like_from))));
                    }(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<tp_tuple_like1_t>>>{}, std::forward<tp_tuple_like1_t>(p_tuple_like_to), std::forward<tp_tuple_like2_t>(p_tuple_like_from));
                    return *this;
                }
            };
        }
        auto constexpr assign = detail::assign_fn{};

        namespace detail {            
            struct from_tuple_like_fn {
                template <tpl::detail::tuple_like_or_cvref tp_tuple_like_t>
                requires (tpl::detail::tuplewise_constructable_from<tp_tuple_like_t, tp_tuple_like_t>)
                auto constexpr operator()(tp_tuple_like_t&& p_tuple_like) const noexcept(tpl::detail::nothrow_tuplewise_constructable_from<tp_tuple_like_t, tp_tuple_like_t>) -> auto {
                    return []<std::size_t... tp_is>(std::index_sequence<tp_is...>, auto&& p_tuple_like) {
                        return tuple<std::tuple_element_t<tp_is, std::remove_reference_t<tp_tuple_like_t>>...>{get<tp_is>(std::forward<tp_tuple_like_t>(p_tuple_like))...};
                    }(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<tp_tuple_like_t>>>{}, std::forward<tp_tuple_like_t>(p_tuple_like));
                }
            };
        }
        auto constexpr from_tuple_like = detail::from_tuple_like_fn{};
    }

    namespace detail {
        struct merge_forwarding_tuples_fn {
            template <tuple_or_cvref tp_tuple1_t, tuple_or_cvref tp_tuple2_t>
            auto constexpr operator()(tp_tuple1_t p_tuple1, tp_tuple2_t p_tuple2) const -> auto {
                return []<std::size_t... tp_is>(std::index_sequence<tp_is...>, auto p_tuple1, auto p_tuple2) {
                    return []<std::size_t... tp_js>(std::index_sequence<tp_js...>, auto p_tuple1, auto p_tuple2) {
                        return tuple<get_t<tp_is, tp_tuple1_t>..., get_t<tp_js, tp_tuple2_t>...>{get<tp_is>(std::move(p_tuple1))..., get<tp_js>(std::move(p_tuple2))...};
                    }(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<tp_tuple2_t>>>{}, std::move(p_tuple1), std::move(p_tuple2));
                }(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<tp_tuple1_t>>>{}, std::move(p_tuple1), std::move(p_tuple2));
            }
        };
        auto constexpr merge_forwarding_tuples = detail::merge_forwarding_tuples_fn{};
        struct tuple_cat_fn {
            template <tuple_like_or_cvref... tp_tuple_like_ts>
            auto constexpr operator()(tp_tuple_like_ts&&... p_tuple_likes) const -> auto {
                if constexpr (sizeof...(p_tuple_likes) == 0) // won't need this branch when using pack indexing approach instead of first,rest...
                    return tuple{};
                else return []<class... tp_tuple_element_ts>(this auto p_self, auto p_delta_references, auto&& p_first_tuple, auto&&... p_rest_tuples) { //can switch this to pack indexing in gcc15, will be a bit more elegant
                        return []<std::size_t... tp_is>(std::index_sequence<tp_is...>, auto p_outer, auto p_delta_references, auto&& p_first_tuple, auto&&... p_rest_tuples) {
                            auto l_new_delta_references = merge_forwarding_tuples(std::move(p_delta_references), extra::as_forwarding_tuple(std::forward<decltype(p_first_tuple)>(p_first_tuple)));
                            if constexpr (sizeof...(p_rest_tuples) != 0)
                                return p_outer.template operator()<tp_tuple_element_ts..., std::tuple_element_t<tp_is, std::remove_reference_t<decltype(p_first_tuple)>>...>(
                                    std::move(l_new_delta_references),
                                    std::forward<decltype(p_rest_tuples)>(p_rest_tuples)...
                                );
                            else {
                                using tuple_to_construct_t = tuple<tp_tuple_element_ts..., std::tuple_element_t<tp_is, std::remove_reference_t<decltype(p_first_tuple)>>...>;
                                static_assert(tuplewise_constructable_from<tuple_to_construct_t, decltype(std::move(l_new_delta_references))>, "all ith element types of tuple concatination must be constructible with all correlating ith forwarding calls to get: (std::constructible_from<std::tuple_element_t<ith, tuple_concat_t>, decltype(std::get<ith>(std::forward<tuple_concat_forwarding_references_t>(tuple_concat_forwarding_references)))"); //change to c++26 compile time exception
                                return apply([](auto&&... p_elements) { return tuple_to_construct_t{std::forward<decltype(p_elements)>(p_elements)...}; }, std::move(l_new_delta_references));
                            }
                        }(std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<decltype(p_first_tuple)>>>{}, p_self, std::move(p_delta_references), std::forward<decltype(p_first_tuple)>(p_first_tuple), std::forward<decltype(p_rest_tuples)>(p_rest_tuples)...);
                }(extra::empty_tuple, std::forward<tp_tuple_like_ts>(p_tuple_likes)...);
            }
        };
    }
    auto constexpr tuple_cat = detail::tuple_cat_fn{};
}

template <tpl::detail::tuple_or_cv tp_tuple_t>
struct std::tuple_size<tp_tuple_t> : std::integral_constant<std::size_t, tpl::detail::tuple_size_v<tp_tuple_t>> {};

template <std::size_t tp_index, tpl::detail::tuple_or_cv tp_tuple_t>
struct std::tuple_element<tp_index, tp_tuple_t> : std::type_identity<tpl::detail::tuple_element_t<tp_index, tp_tuple_t>> {};
