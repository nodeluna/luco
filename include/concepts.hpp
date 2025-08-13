/*
 * Copyright: 2025 nodeluna
 * SPDX-License-Identifier: Apache-2.0
 * repository: https://github.com/nodeluna/luco
 */

#pragma once

#include <any>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <stack>
#include <fstream>
#include <format>
#include <functional>
#include <unordered_set>
#include <variant>
#include <vector>
#include <cassert>
#include <source_location>
#include <type_traits>
#include "expected.hpp"

/**
 * @brief the namespace for luco
 */
namespace luco
{
	class value;
	class array;
	class object;
	class node;
	class null_type;

	/**
	  @brief puts a constraint on the allowed luco types for luco::value
	 */
	template<typename allowed_value_types>
	concept is_allowed_value_type = std::is_same_v<allowed_value_types, std::string> ||
					std::is_same_v<allowed_value_types, const char*> || std::is_arithmetic_v<allowed_value_types> ||
					std::is_same_v<allowed_value_types, null_type> || std::is_same_v<allowed_value_types, bool>;
	/**
	 * @brief allowed types in luco::node
	 */
	template<typename allowed_node_types>
	concept is_allowed_node_type = std::is_same_v<allowed_node_types, std::string> || std::is_same_v<allowed_node_types, const char*> ||
				       std::is_arithmetic_v<allowed_node_types> || std::is_same_v<allowed_node_types, null_type> ||
				       std::is_same_v<allowed_node_types, bool> || std::is_same_v<allowed_node_types, class value> ||
				       std::is_same_v<allowed_node_types, node>;

	template<typename variant_type>
	concept is_allowed_variant = requires(variant_type variant) {
		{
			std::visit(
			    [](auto&& arg) -> bool
			    {
				    using arg_type = std::decay_t<decltype(arg)>;
				    return is_allowed_node_type<arg_type>;
			    },
			    variant)
		} -> std::same_as<bool>;
	};

	/**
	 * @brief concept for enabling luco::node to accept std key/value containers
	 */
	template<typename container_type>
	concept is_key_value_container = requires(container_type container) {
		typename container_type::key_type;
		typename container_type::mapped_type;
		{
			container.begin()
		} -> std::same_as<typename container_type::iterator>;
		{
			container.end()
		} -> std::same_as<typename container_type::iterator>;
	} && std::is_same_v<typename container_type::key_type, std::string> && is_allowed_node_type<typename container_type::mapped_type>;

	template<typename string_type>
	concept is_string_type = std::is_same_v<string_type, std::string> || std::is_same_v<string_type, std::wstring>;

	/**
	 * @brief concept for enabling luco::node to accept std array-like containers
	 */
	template<typename container_type>
	concept is_value_container =
	    requires(container_type container) {
		    typename container_type::value_type;
		    {
			    container.begin()
		    } -> std::same_as<typename container_type::iterator>;
		    {
			    container.end()
		    } -> std::same_as<typename container_type::iterator>;
	    } && not is_key_value_container<container_type> && is_allowed_node_type<typename container_type::value_type> &&
	    not is_string_type<container_type>;

	/**
	 * @brief puts a constraint on the allowed std container types to be inserted into luco::node
	 */
	template<typename container_type>
	concept container_type_concept = is_key_value_container<container_type> || is_value_container<container_type>;

	/**
	 * @brief puts a constraint on the allowed types to be inserted into luco::node which is container_type_concept or
	 * is_allowed_node_type
	 */
	template<typename value_type>
	concept container_or_node_type =
	    container_type_concept<value_type> || is_allowed_node_type<value_type> || is_allowed_variant<value_type>;
}
