/*
 * Copyright: 2025 nodeluna
 * SPDX-License-Identifier: Apache-2.0
 * repository: https://github.com/nodeluna/luco
 */

#pragma once

#include <any>
#include <cstddef>
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
#include "concepts.hpp"
#include "error.hpp"

/**
 * @brief the namespace for luco
 */
namespace luco
{
	void __print(const std::string& data)
	{
		std::cout << data;
	}
	enum class value_type {
		none,
		string,
		number,
		integer,
		double_t,
		null,
		boolean,
		temp_escape_type,
		unknown,
	};

	/**
	 * @enum node_type
	 * @brief this enum can be used to explicitly make a node that's either an object, array or value
	 * @detail @cpp
	 * luco::node node(luco::node_type::array);
	 * luco::node node; // default is luco::node_type::object
	 * @ecpp
	 */
	enum class node_type {
		object,
		array,
		value,
	};

	/**
	 * @class null_type
	 * @brief an empty class to represent a luco null value
	 */
	class null_type {
		public:
			null_type() = default;

			bool operator==(const null_type&) const
			{
				return true;
			}

			bool operator!=(const null_type&) const
			{
				return false;
			}
	};

	inline null_type null;

	/**
	 * @class value
	 * @brief holds a luco value such as <std::string, double, int64_t, bool, null_type, monostate>
	 */
	class value {
		private:
			using value_type_variant  = std::variant<std::string, double, int64_t, bool, null_type, monostate>;
			value_type_variant _value = monostate();
			value_type	   _type  = value_type::none;

			template<is_allowed_value_type val_type>
			void set_state(const val_type& val) noexcept
			{
				if constexpr (std::is_same_v<val_type, bool>)
				{
					_type  = value_type::boolean;
					_value = val;
				}
				else if constexpr (std::is_arithmetic_v<val_type>)
				{
					if constexpr (std::is_floating_point_v<val_type>)
					{
						_type = value_type::double_t;
					}
					else
					{
						_type = value_type::integer;
					}

					_value = val;
				}
				else if constexpr (std::is_same_v<val_type, std::string> || std::is_same_v<val_type, const char*> ||
						   std::is_same_v<val_type, char*>)
				{
					_type  = value_type::string;
					_value = val;
				}
				else if constexpr (std::is_same_v<val_type, null_type>)
				{
					_type  = value_type::null;
					_value = val;
				}
				else if constexpr (std::is_same_v<val_type, monostate>)
				{
					_type  = value_type::none;
					_value = monostate();
				}
				else
				{
					static_assert(false && "unsupported value_type in class value");
				}
			}

			expected<monostate, error> set_state(const std::string& val, value_type t)
			{
				_type = t;
				switch (t)
				{
					case value_type::double_t:
					case value_type::number:
						_value = std::stod(val);
						break;
					case value_type::integer:
						_value = std::stoll(val);
						break;
					case value_type::string:
						_value = val;
						break;
					case value_type::boolean:
						_value = (val == "true" ? true : false);
						break;
					case value_type::null:
						_value = null_type();
						break;
					case value_type::none:
						_value = monostate();
						break;
					case value_type::unknown:
					case value_type::temp_escape_type:
					default:
						_type  = value_type::none;
						_value = monostate();
						return unexpected(error(error_type::wrong_type, "unsupported value_type in class value"));
				}

				return monostate();
			}

		public:
			/**
			 * @brief constructor for luco::value
			 * @param val luco value to be set
			 * @tparam is_allowed_value_type the type of the luco value
			 */
			template<is_allowed_value_type val_type>
			value(const val_type& val) noexcept
			{
				this->set_state(val);
			}

			/**
			 * @brief copy constructor for luco::value
			 * @param other luco::value to be copied
			 */
			value(const value& other) : _value(other._value), _type(other._type)
			{
			}

			/**
			 * @brief copy assignment for luco::value
			 * @param other luco::value to be copied
			 * @return the address of the modified luco::value
			 */
			value& operator=(const value& other)
			{
				_value = other._value;
				_type  = other._type;
				return *this;
			}

			/**
			 * @brief move constructor for luco::value
			 * @param other luco::value to be moved
			 */
			value(const value&& other) : _value(std::move(other._value)), _type(other._type)
			{
			}

			/**
			 * @brief move assignment for luco::value
			 * @param other luco::value to be moved
			 * @return the address of the modified luco::value
			 */
			value& operator=(const value&& other)
			{
				_value = std::move(other._value);
				_type  = other._type;
				return *this;
			}

			/**
			 * @brief constructor which sets an empty luco::value
			 */
			value() noexcept : _value(monostate()), _type(value_type::none)
			{
			}

			/**
			 * @brief gets the type of the stored luco value
			 * @return luco::value_type of the stored luco value
			 */
			luco::value_type type() const noexcept
			{
				return _type;
			}

			/**
			 * @brief sets the value and type of luco::value
			 * @param val luco value to be set
			 * @tparam is_allowed_value_type the type of the luco value
			 */
			template<is_allowed_value_type val_type>
			void set_value_type(const val_type& val) noexcept
			{
				this->set_state(val);
			}

			/**
			 * @brief sets the value and type of luco::value
			 * @param val luco value to be set
			 * @param type luco type (luco::value_type) to be set
			 * @return luco::monostate or luco::error if the value wasn't set (wrong type was provided)
			 */
			expected<monostate, error> set_value_type(const std::string& val, value_type type)
			{
				return this->set_state(val, type);
			}

			/**
			 * @brief checks if luco::value is holding luco string (std::string)
			 * @return true if it does
			 */
			bool is_string() const noexcept
			{
				return std::holds_alternative<std::string>(_value);
			}

			/**
			 * @brief checks if luco::value is holding luco number (double or int64_t)
			 * @return true if it does
			 */
			bool is_number() const noexcept
			{
				return std::holds_alternative<double>(_value) || std::holds_alternative<int64_t>(_value);
			}

			/**
			 * @brief checks if luco::value is holding luco number (double)
			 * @return true if it does
			 */
			bool is_double() const noexcept
			{
				return std::holds_alternative<double>(_value);
			}

			/**
			 * @brief checks if luco::value is holding luco number (int64_t)
			 * @return true if it does
			 */
			bool is_integer() const noexcept
			{
				return std::holds_alternative<int64_t>(_value);
			}

			/**
			 * @brief checks if luco::value is holding luco boolean (bool)
			 * @return true if it does
			 */
			bool is_boolean() const noexcept
			{
				return std::holds_alternative<bool>(_value);
			}

			/**
			 * @brief checks if luco::value is holding luco null (luco::null_type)
			 * @return true if it does
			 */
			bool is_null() const noexcept
			{
				return std::holds_alternative<null_type>(_value);
			}

			/**
			 * @brief checks if luco::value is not holding luco value (luco::monostate)
			 * @return true if it does
			 */
			bool is_empty() const noexcept
			{
				return std::holds_alternative<monostate>(_value);
			}

			/**
			 * @brief cast luco::value into a std::string if it is holding a luco string (std::string)
			 * @detail @cpp
			 * luco::expected<std::string, error> string = value.try_as_string();
			 * if (not string)
			 * {
			 *	// handle error
			 *	std::println("{}", string.error().message());
			 * }
			 * else
			 * {
			 *	// success
			 *	std::string string_value = string.value();
			 * }
			 * @ecpp
			 * @return std::string or luco::error if it doesn't hold a string
			 * @see as_string()
			 */
			expected<std::string, error> try_as_string() noexcept
			{
				if (not this->is_string())
				{
					return unexpected(error(error_type::wrong_type,
								"wrong type: trying to cast the value '{}' which is a '{}' to 'string'",
								this->stringify(), this->type_name()));
				}

				return std::get<std::string>(_value);
			}

			/**
			 * @brief cast luco::value into a double if it is holding a luco number (double or int64_t)
			 * @return double or luco::error if it doesn't hold a number
			 * @see as_number()
			 */
			expected<double, error> try_as_number() noexcept
			{
				if (not this->is_number())
				{
					return unexpected(error(error_type::wrong_type,
								"wrong type: trying to cast the value '{}' which is a '{}' to 'number'",
								this->stringify(), this->type_name()));
				}

				if (this->is_double())
				{
					return std::get<double>(_value);
				}
				else
				{
					return std::get<int64_t>(_value);
				}
			}

			/**
			 * @brief cast luco::value into a int64_t if it is holding a luco number (int64_t)
			 * @return int64_t or luco::error if it doesn't hold a number
			 * @see as_integer()
			 */
			expected<int64_t, error> try_as_integer() noexcept
			{
				if (not this->is_integer())
				{
					return unexpected(error(error_type::wrong_type,
								"wrong type: trying to cast the value '{}' which is a '{}' to 'integer'",
								this->stringify(), this->type_name()));
				}

				return std::get<int64_t>(_value);
			}

			/**
			 * @brief cast luco::value into a double if it is holding a luco number (double)
			 * @return double or luco::error if it doesn't hold a number
			 * @see as_double()
			 */
			expected<double, error> try_as_double() noexcept
			{
				if (not this->is_double())
				{
					return unexpected(error(error_type::wrong_type,
								"wrong type: trying to cast the value '{}' which is a '{}' to 'double'",
								this->stringify(), this->type_name()));
				}

				return std::get<double>(_value);
			}

			/**
			 * @brief cast luco::value into a bool if it is holding a luco boolean (bool)
			 * @return bool or luco::error if it doesn't hold a boolean
			 * @see as_boolean()
			 */
			expected<bool, error> try_as_boolean() noexcept
			{
				if (not this->is_boolean())
				{
					return unexpected(error(error_type::wrong_type,
								"wrong type: trying to cast the value '{}' which is a '{}' to 'boolean'",
								this->stringify(), this->type_name()));
				}

				return std::get<bool>(_value);
			}

			/**
			 * @brief cast luco::value into a luco::null_type if it is holding a luco null (luco::null_type)
			 * @return luco::null_type or luco::error if it doesn't hold a null
			 * @see as_null()
			 */
			expected<null_type, error> try_as_null() noexcept
			{
				if (not this->is_null())
				{
					return unexpected(error(error_type::wrong_type,
								"wrong type: trying to cast the value '{}' which is a '{}' to 'null'",
								this->stringify(), this->type_name()));
				}

				return std::get<null_type>(_value);
			}

			/**
			 * @brief cast luco::value into a string if it is holding a luco string (std::string)
			 * @exception luco::error if it doesn't luco string
			 * @return luco string
			 * @see try_as_string()
			 */
			std::string as_string()
			{
				auto ok = this->try_as_string();
				if (not ok)
				{
					throw ok.error();
				}

				return ok.value();
			}

			/**
			 * @brief cast luco::value into a number if it is holding a luco number (double or int64_t)
			 * @exception luco::error if it doesn't hold luco number
			 * @return luco number
			 * @see try_as_number()
			 */
			double as_number()
			{
				auto ok = this->try_as_number();
				if (not ok)
				{
					throw ok.error();
				}

				return ok.value();
			}

			/**
			 * @brief cast luco::value into a number if it is holding a luco number (int64_t)
			 * @exception luco::error if it doesn't hold luco number
			 * @return luco number
			 * @see try_as_integer()
			 */
			int64_t as_integer()
			{
				auto ok = this->try_as_integer();
				if (not ok)
				{
					throw ok.error();
				}

				return ok.value();
			}

			/**
			 * @brief cast luco::value into a number if it is holding a luco number (double)
			 * @exception luco::error if it doesn't hold luco number
			 * @return luco number
			 * @see try_as_double()
			 */
			double as_double()
			{
				auto ok = this->try_as_double();
				if (not ok)
				{
					throw ok.error();
				}

				return ok.value();
			}

			/**
			 * @brief cast luco::value into a boolean if it is holding a luco boolean (bool)
			 * @exception luco::error if it doesn't hold luco boolean
			 * @return luco boolean
			 * @see try_as_boolean()
			 */
			bool as_boolean()
			{
				auto ok = this->try_as_boolean();
				if (not ok)
				{
					throw ok.error();
				}

				return ok.value();
			}

			/**
			 * @brief cast luco::value into a null if it is holding a luco null (luco::null_type)
			 * @exception luco::error if it doesn't hold luco null
			 * @return luco null
			 * @see try_as_null()
			 */
			null_type as_null()
			{
				auto ok = this->try_as_null();
				if (not ok)
				{
					throw ok.error();
				}

				return ok.value();
			}

			/**
			 * @brief cast the luco value into a std::string
			 * @return string representation of the value
			 */
			std::string stringify() const noexcept
			{
				if (this->is_double())
				{
					std::string str			 = std::to_string(std::get<double>(_value));

					auto	    pop_zeros_at_the_end = [&]()
					{
						bool found_zero = false;
						for (size_t i = str.size() - 1;; i--)
						{
							if (str[i] == '0' && not found_zero)
							{
								found_zero = true;
							}
							else if (str[i] == '0' && found_zero)
							{
								str.pop_back();
							}
							else if (found_zero)
							{
								str.pop_back();
								found_zero = false;
							}
							else
							{
								break;
							}

							if (i == 0)
							{
								break;
							}
						}

						if (not str.empty() && str.back() == '.')
						{
							str += "0";
						}
					};

					pop_zeros_at_the_end();

					return str;
				}
				else if (this->is_integer())
				{
					return std::to_string(std::get<int64_t>(_value));
				}
				else if (this->is_string())
				{
					return std::get<std::string>(_value);
				}
				else if (this->is_boolean())
				{
					return std::get<bool>(_value) == true ? "true" : "false";
				}
				else if (this->is_null())
				{
					return "null";
				}
				else
				{
					return "";
				}
			}

			/**
			 * @brief gets string representation of luco::value_type of the internal value
			 * @return string name of the value_type
			 */
			std::string type_name() const noexcept
			{
				if (this->is_string())
				{
					return "string";
				}
				else if (this->is_boolean())
				{
					return "boolean";
				}
				else if (this->is_null())
				{
					return "null";
				}
				else if (this->is_double())
				{
					return "double";
				}
				else if (this->is_integer())
				{
					return "integer";
				}
				else if (this->is_empty())
				{
					return "none";
				}
				else
				{
					return "unknown";
				}
			}
	};

	class array;
	class object;
	class node;

	using object_pairs = std::initializer_list<std::pair<std::string, std::any>>;
	using array_values = std::initializer_list<std::any>;

	using luco_array   = std::vector<class node>;
	using luco_node	   = std::variant<std::shared_ptr<class value>, std::shared_ptr<luco::array>, std::shared_ptr<luco::object>>;

	/**
	 * @class node
	 * @brief the class that holds a luco node which is either luco::object, luco::array or luco::value
	 */
	class node {
		private:
			luco_node _node;

		protected:
			void handle_std_any(const std::any& any_value, std::function<void(std::any)> insert_func);

			template<typename is_allowed_node_type>
			constexpr std::variant<class value, luco::node> static handle_allowed_node_types(
			    const is_allowed_node_type& value) noexcept;

			template<typename container_or_node_type>
			constexpr void setting_allowed_node_type(const container_or_node_type& node_value) noexcept;

			template<is_allowed_value_type T>
			expected<T, error> access_value(std::function<expected<T, error>(std::shared_ptr<class value>)> fun) const;

		public:
			/**
			 * @brief default constructor which creates luco::node with type luco::node_type::object
			 */
			explicit node();
			explicit node(const luco_node& n);

			/**
			 * @brief constructor to allow setting the type of the luco::node
			 * @param type type of node from enum luco::node_type
			 */
			explicit node(enum node_type type);

			template<typename container_or_node_type>
			explicit node(const container_or_node_type& container) noexcept;

			node(const std::initializer_list<std::pair<std::string, std::any>>& pairs);
			node(const std::initializer_list<std::any>& val);

			template<typename container_or_node_type>
			expected<std::reference_wrapper<luco::node>, error> insert(const std::string&		 key,
										   const container_or_node_type& node);

			template<typename container_or_node_type>
			expected<std::reference_wrapper<luco::node>, error>	  push_back(const container_or_node_type& node);

			/**
			 * @brief access the luco::value the luco::node is holding, if it exists
			 * @return luco::value or luco::error if it doesn't hold a luco::value
			 * @see as_value()
			 */
			expected<std::shared_ptr<class value>, error>		  try_as_value() const noexcept;

			/**
			 * @brief access the luco::array the luco::node is holding, if it exists
			 * @return luco::array or luco::error if it doesn't hold a luco::array
			 * @see as_array()
			 */
			expected<std::shared_ptr<luco::array>, error>		  try_as_array() const noexcept;

			/**
			 * @brief access the luco::object the luco::node is holding, if it exists
			 * @return luco::object or luco::error if it doesn't hold a luco::object
			 * @see as_object()
			 */
			expected<std::shared_ptr<luco::object>, error>		  try_as_object() const noexcept;

			/**
			 * @brief access the luco::value the luco::node is holding, if it exists
			 * @throw luco::error if it doesn't hold luco::value
			 * @return std::shared_ptr<luco::value>
			 * @see try_as_value()
			 */
			std::shared_ptr<class value>				  as_value() const;

			/**
			 * @brief access the luco::array the luco::node is holding, if it exists
			 * @throw luco::error if it doesn't hold luco::array
			 * @return std::shared_ptr<luco::array>
			 * @see try_as_array()
			 */
			std::shared_ptr<luco::array>				  as_array() const;

			/**
			 * @brief access the luco::object the luco::node is holding, if it exists
			 * @throw luco::error if it doesn't hold luco::object
			 * @return std::shared_ptr<luco::object>
			 * @see try_as_object()
			 */
			std::shared_ptr<luco::object>				  as_object() const;

			/**
			 * @brief cast a node into a std::string if it is holding luco::value that is a luco string (std::string)
			 * @detail @cpp
			 * luco::expected<std::string, error> string = node.try_as_string();
			 * if (not string)
			 * {
			 *	// handle error
			 *	std::println("{}", string.error().message());
			 * }
			 * else
			 * {
			 *	// success
			 *	std::string string_value = string.value();
			 * }
			 * @ecpp
			 * @return std::string or luco::error if it doesn't hold a string
			 * @see as_string()
			 */
			expected<std::string, error>				  try_as_string() const noexcept;

			/**
			 * @brief cast a node into a int64_t if it is holding luco::value that is a luco number (int64_t)
			 * @return int64_t or luco::error if it doesn't hold a string
			 * @see as_integer()
			 */
			expected<int64_t, error>				  try_as_integer() const noexcept;

			/**
			 * @brief cast a node into a double if it is holding luco::value that is a luco number (double)
			 * @return double or luco::error if it doesn't hold a string
			 * @see as_double()
			 */
			expected<double, error>					  try_as_double() const noexcept;

			/**
			 * @brief cast a node into a double if it is holding luco::value that is a luco number (double or int64_t)
			 * @return double or luco::error if it doesn't hold a string
			 * @see as_number()
			 */
			expected<double, error>					  try_as_number() const noexcept;

			/**
			 * @brief cast a node into a bool if it is holding luco::value that is a luco boolean (bool)
			 * @return bool or luco::error if it doesn't hold a string
			 * @see as_boolean()
			 */
			expected<bool, error>					  try_as_boolean() const noexcept;

			/**
			 * @brief cast a node into a luco::null_type if it is holding luco::value that is a luco null
			 * @detail @cpp
			 * luco::expected<null_type, error> null = node.try_as_null();
			 * if (not null)
			 * {
			 *	// handle error
			 *	std::println("{}", null.error().message());
			 * }
			 * else
			 * {
			 *	// success
			 *	luco::null_type null_value = null.value();
			 * }
			 * @ecpp
			 * @return luco::null_type or luco::error if it doesn't hold a null (luco::null_type)
			 * @see as_null()
			 */
			expected<null_type, error>				  try_as_null() const noexcept;

			/**
			 * @brief cast a node into a string if it is holding luco::value that is a luco string (luco::null_type)
			 * @exception luco::error if it doesn't hold a luco::value and string
			 * @return luco string
			 * @see try_as_string()
			 */
			std::string						  as_string() const;

			/**
			 * @brief cast a node into a int64_t if it is holding luco::value that is a luco number (int64_t)
			 * @exception luco::error if it doesn't hold a luco::value and int64_t
			 * @return luco number
			 * @see try_as_integer()
			 */
			int64_t							  as_integer() const;

			/**
			 * @brief cast a node into a double if it is holding luco::value that is a luco number (double)
			 * @exception luco::error if it doesn't hold a luco::value and double
			 * @return luco number
			 * @see try_as_double()
			 */
			double							  as_double() const;

			/**
			 * @brief cast a node into a double if it is holding luco::value that is a luco number (int64_t or double)
			 * @exception luco::error if it doesn't hold a luco::value and (int64_t or double)
			 * @return luco number
			 * @see try_as_number()
			 */
			double							  as_number() const;

			/**
			 * @brief cast a node into a bool if it is holding luco::value that is a luco boolean (bool)
			 * @exception luco::error if it doesn't hold a luco::value and bool
			 * @return luco boolean
			 * @see try_as_boolean()
			 */
			bool							  as_boolean() const;

			/**
			 * @brief cast a node into a luco::null_type if it is holding luco::value that is a luco null (luco::null_type)
			 * @exception luco::error if it doesn't hold a luco::value and luco::null_type
			 * @return luco null
			 * @see try_as_null()
			 */
			null_type						  as_null() const;

			/**
			 * @brief checks if luco::node is holding luco::value
			 * @return true if it does
			 * @detail @cpp
			 * luco::node node(luco::node_type::value);
			 * if (node.is_value())
			 * {
			 *	// do something
			 * }
			 * @ecpp
			 */
			bool							  is_value() const noexcept;

			/**
			 * @brief checks if luco::node is holding luco::array
			 * @return true if it does
			 */
			bool							  is_array() const noexcept;

			/**
			 * @brief checks if luco::node is holding luco::object
			 * @return true if it does
			 */
			bool							  is_object() const noexcept;

			/**
			 * @brief checks if luco::node is holding luco::value that is holding luco string (std::string)
			 * @return true if it does
			 */
			bool							  is_string() const noexcept;

			/**
			 * @brief checks if luco::node is holding luco::value that is holding luco number (int64_t)
			 * @return true if it does
			 */
			bool							  is_integer() const noexcept;

			/**
			 * @brief checks if luco::node is holding luco::value that is holding luco number (double)
			 * @return true if it does
			 */
			bool							  is_double() const noexcept;

			/**
			 * @brief checks if luco::node is holding luco::value that is holding luco number (double or int64_t)
			 * @return true if it does
			 */
			bool							  is_number() const noexcept;

			/**
			 * @brief checks if luco::node is holding luco::value that is holding luco boolean (bool)
			 * @return true if it does
			 */
			bool							  is_boolean() const noexcept;

			/**
			 * @brief checks if luco::node is holding luco::value that is holding luco null (luco::null_type)
			 * @return true if it does
			 */
			bool							  is_null() const noexcept;

			/**
			 * @brief gets the luco::node_type of the internal node
			 * @return node_type
			 */
			node_type						  type() const noexcept;

			/**
			 * @brief gets string representation of luco::node_type of the internal node
			 * @return string name of the node_type
			 */
			std::string						  type_name() const noexcept;

			/**
			 * @brief gets the luco::value_type of the node if it's holding luco::value. otherwise it returns value_type::none
			 * @return value_type
			 */
			value_type						  valuetype() const noexcept;

			/**
			 * @brief gets string representation of luco::value_type of the internal node if it's holding luco::value
			 * @return string name of the value_type
			 */
			std::string						  value_type_name() const noexcept;

			/**
			 * @brief stringify the luco (object, array or value) inside the luco::node
			 * @detail @luco
			 *
			 *   key1 = "val1"
			 *   key2 = "val2"
			 *
			 * @eluco
			 * @detail @luco
			 *
			 *   "val1"
			 *   "val2"
			 *   "val3"
			 *
			 * @eluco
			 * @detail @luco
			 *  "value"
			 * @eluco
			 * @return serialized luco
			 */
			std::string						  stringify() const noexcept;

			/**
			 * @brief checks if a key exists in a luco object
			 * @param key key to lookup
			 * @return true if it does
			 */
			bool							  contains(const std::string& key) const noexcept;

			/**
			 * @brief access the node at the specified object key
			 * @param object_key luco key to access in an object
			 * @return luco::node& at the specified key
			 * @see try_at()
			 */
			class node&						  at(const std::string& object_key) const;

			/**
			 * @brief access the node at the specified array index
			 * @param array_index luco index to access in an array
			 * @return luco::node& at the specified index
			 * @see try_at()
			 */
			class node&						  at(const size_t array_index) const;

			/**
			 * @brief access the node at the specified object key
			 * @param object_key luco key to access in an object
			 * @detail @cpp
			 * luco::expected<std::reference_wrapper<luco::node>, luco::error> maybe_node =
			 * object_node.try_at("key");
			 * if (maybe_node)
			 * {
			 *	// notice the '&' is neccessary if this reference would be used to modify
			 *	// the value inside
			 *	luco::node& node_at_key = maybe_node.value().get();
			 *	// .value() to get the expected type, .get() for the node reference
			 *
			 *	// or you can set it like this to avoid forgetting the '&'
			 *	maybe_node.value().get().set(std::string("new value"));
			 * }
			 * else
			 * {
			 *	std::println("{}", maybe_node.error().message());
			 * }
			 * @ecpp
			 * @return either std::reference_wrapper<luco::node> if the node was found or luco::error if not
			 * @see at()
			 */
			expected<std::reference_wrapper<luco::node>, luco::error> try_at(const std::string& object_key) const noexcept;

			/**
			 * @brief access the node at the specified array index
			 * @param array_index luco index to access in an array
			 * @detail @cpp
			 * luco::expected<std::reference_wrapper<luco::node>, luco::error> maybe_node =
			 * array_node.try_at(0);
			 * if (maybe_node)
			 * {
			 *	// notice the '&' is neccessary if this reference would be used to modify
			 *	// the value inside
			 *	luco::node& node_at_key = maybe_node.value().get();
			 *	// .value() to get the expected type, .get() for the node reference
			 *
			 *	// or you can set it like this to avoid forgetting the '&'
			 *	maybe_node.value().get().set(true);
			 * }
			 * else
			 * {
			 *	std::println("{}", maybe_node.error().message());
			 * }
			 * @ecpp
			 * @return either std::reference_wrapper<luco::node> if the node was found or luco::error if not
			 * @see at()
			 */
			expected<std::reference_wrapper<luco::node>, luco::error> try_at(const size_t array_index) const noexcept;

			/**
			 * @brief set a node with a container_or_node_type
			 * @param node_value value to be set
			 */
			template<typename container_or_node_type>
			void set(const container_or_node_type& node_value) noexcept;

			/**
			 * @brief asign a node with a container_or_node_type
			 * @param node_value value to be set
			 * @return the address of the node which can be used to modify the value
			 */
			template<typename container_or_node_type>
			class node&			  operator=(const container_or_node_type& node_value) noexcept;

			class node&			  operator+=(const std::initializer_list<std::pair<std::string, std::any>>& pairs);
			class node&			  operator+=(const std::initializer_list<std::any>& val);

			/**
			 * @brief add two luco::node together. they must have the same type and be either object, array, string or number
			 * @param other_node the other node to add to the current node
			 * @detail @cpp
			 * luco::node new_node = node1 + node2;
			 * @ecpp
			 * @luco
			 * node1_key = "node1_value"
			 * @eluco
			 * @luco
			 * node2_key = "node2_value"
			 * @eluco
			 * @luco
			 * node1_key = "node1_value"
			 * node2_key = "node2_value"
			 * @eluco
			 * @throw luco::error if different types or not one of the required types
			 * @return new node containing content of both nodes
			 */
			class node			  operator+(const node& other_node);

			void				  dump_to_json(const std::function<void(std::string)> out_func = __print,
								       const std::pair<char, size_t>& indent_conf = {' ', 4}, size_t indent = 0) const;

			void				  dump_to_luco(const std::function<void(std::string)> out_func = __print,
								       const std::pair<char, size_t>& indent_conf = {' ', 4}, size_t indent = 0) const;

			/**
			 * @brief write luco::node to stdout
			 * @param indent_conf indentation config for writing {char, size}
			 */
			void				  dump_to_stdout(const std::pair<char, size_t>& indent_conf = {' ', 4}) const;

			/**
			 * @brief write luco::node to string
			 * @param indent_conf indentation config for writing {char, size}
			 * @return luco serialized
			 */
			std::string			  dump_to_string(const std::pair<char, size_t>& indent_conf = {' ', 4}) const;

			/**
			 * @brief write luco::node to a file
			 * @param path path to write to
			 * @param indent_conf indentation config for writing {char, size}
			 */
			expected<monostate, error>	  dump_to_file(const std::filesystem::path&   path,
								       const std::pair<char, size_t>& indent_conf = {' ', 4}) const;

			expected<class luco::node, error> add_value_to_array(const size_t index, const class value& value);
			expected<class luco::node, error> add_node_to_array(const size_t index, const luco::node& node);
	};
	using luco_object = std::map<std::string, class node>;

	/**
	 * @class object
	 * @brief the class that holds a luco object
	 */
	class object {
		private:
			luco_object _object;

		public:
			/**
			 * @brief constructor for luco::object
			 */
			explicit object() : _object(luco_object{})
			{
			}

			/**
			 * @brief insert luco::node into key
			 * @param key the luco key to insert at
			 * @param element the luco::node to be inserted
			 * @return a reference of the inserted luco::node
			 */
			luco::node& insert(const std::string& key, const class node& element)
			{
				return _object[key] = element;
			}

			/**
			 * @brief remove a key with its associated node from the luco::object
			 * @param key the luco key to be removed
			 * @return number of keys removed
			 */
			luco_object::size_type erase(const std::string& key)
			{
				return _object.erase(key);
			}

			/**
			 * @brief remove an iterator with its associated node from the luco::object
			 * @param pos the iterator to be removed
			 * @return iterator following the last removed iterator
			 */
			luco_object::iterator erase(const luco_object::iterator pos)
			{
				return _object.erase(pos);
			}

			/**
			 * @brief remove a range with its associated luco::node's from the luco::object
			 * @param begin the beginning of the range to be removed
			 * @param end the end of the range to be removed
			 * @return iterator following the last removed iterator
			 */
			luco_object::iterator erase(const luco_object::iterator begin, const luco_object::iterator end)
			{
				return _object.erase(begin, end);
			}

			/**
			 * @brief get the number of keys
			 * @return the number of keys
			 */
			size_t size() const noexcept
			{
				return _object.size();
			}

			/**
			 * @brief check if the luco::object is empty
			 * @return true if it is
			 */
			bool empty() const noexcept
			{
				return _object.empty();
			}

			/**
			 * @brief find a key
			 * @return iterator of the found key found or end()
			 */
			luco_object::iterator find(const std::string& key)
			{
				return _object.find(key);
			}

			/**
			 * @brief returns an the first iterator
			 * @return the first iterator
			 */
			luco_object::iterator begin()
			{
				return _object.begin();
			}

			/**
			 * @brief returns an iterator past the last key
			 * @return iterator past the last key
			 */
			luco_object::iterator end()
			{
				return _object.end();
			}

			/**
			 * @brief access specified key *with* bounds checking
			 * @param key to be accessed
			 * @return a reference to the luco::node associated to the key
			 * @throw std::out_of_range if the container doesn't have the key
			 */
			class luco::node& at(const std::string& key)
			{
				return _object.at(key);
			}

			/**
			 * @brief access specified key *without* bounds checking or insert the key if it doesn't exist
			 * @param key to be accessed
			 * @return a reference to the luco::node associated to the key
			 */
			class luco::node& operator[](const std::string& key)
			{
				return _object[key];
			}
	};

	/**
	 * @class array
	 * @brief the class that holds a luco array
	 */
	class array {
		private:
			luco_array _array;

		public:
			explicit array(const luco_array& arr) noexcept : _array(arr)
			{
			}

			explicit array() noexcept
			{
			}

			void push_back(const class node& element)
			{
				return _array.push_back(element);
			}

			void pop_back()
			{
				return _array.pop_back();
			}

			luco_array::iterator erase(const luco_array::iterator pos)
			{
				return _array.erase(pos);
			}

			luco_array::iterator erase(const luco_array::iterator begin, const luco_array::iterator end)
			{
				return _array.erase(begin, end);
			}

			class node& front()
			{
				return _array.front();
			}

			class node& back()
			{
				return _array.back();
			}

			size_t size() const noexcept
			{
				return _array.size();
			}

			bool empty() const noexcept
			{
				return _array.empty();
			}

			luco_array::iterator begin()
			{
				return _array.begin();
			}

			luco_array::iterator end()
			{
				return _array.end();
			}

			class luco::node& at(size_t i)
			{
				return _array.at(i);
			}

			class luco::node& operator[](size_t i)
			{
				return _array[i];
			}
	};
}

namespace luco
{
	node::node() : _node(std::make_shared<luco::object>())
	{
	}

	node::node(enum node_type type)
	{
		switch (type)
		{
			case node_type::value:
				_node = std::make_shared<class value>();
				break;
			case node_type::array:
				_node = std::make_shared<luco::array>();
				break;
			case luco::node_type::object:
				_node = std::make_shared<luco::object>();
				break;
		}
	}

	template<typename container_or_node_type>
	node::node(const container_or_node_type& node_value) noexcept
	{
		node::setting_allowed_node_type(node_value);
	}

	template<typename is_allowed_node_type>
	constexpr std::variant<class value, luco::node> node::handle_allowed_node_types(const is_allowed_node_type& value) noexcept
	{
		if constexpr (std::is_same<is_allowed_node_type, class value>::value)
		{
			return value;
		}
		else if constexpr (std::is_same<is_allowed_node_type, bool>::value)
		{
			return luco::value(value);
		}
		else if constexpr (std::is_arithmetic<is_allowed_node_type>::value)
		{
			if constexpr (std::is_floating_point_v<is_allowed_node_type>)
			{
				return luco::value(value);
			}
			else
			{
				return luco::value(value);
			}
		}
		else if constexpr (std::is_same<is_allowed_node_type, std::string>::value)
		{
			return luco::value(value);
		}
		else if constexpr (std::is_same<is_allowed_node_type, const char*>::value)
		{
			return luco::value(value);
		}
		else if constexpr (std::is_same<is_allowed_node_type, null_type>::value)
		{
			return luco::value(luco::null);
		}
		else if constexpr (std::is_same<is_allowed_node_type, luco::node>::value)
		{
			return value;
		}
		else
		{
			static_assert(false && "unsupported value_type in function node::handle_allowed_node_types(...)");
		}
	}

	template<typename container_or_node_type>
	constexpr void node::setting_allowed_node_type(const container_or_node_type& node_value) noexcept
	{
		if constexpr (is_value_container<container_or_node_type>)
		{
			_node = std::make_shared<luco::array>();

			for (auto& val : node_value)
			{
				std::variant<class value, luco::node> v = this->handle_allowed_node_types(val);

				if (std::holds_alternative<class value>(v))
				{
					this->push_back(node(std::get<class value>(v)));
				}
				else
				{
					this->push_back(std::get<luco::node>(v));
				}
			}
		}
		else if constexpr (is_key_value_container<container_or_node_type>)
		{
			_node = std::make_shared<luco::object>();

			for (auto& [key, val] : node_value)
			{
				std::variant<class value, luco::node> v = this->handle_allowed_node_types(val);

				if (std::holds_alternative<class value>(v))
				{
					this->insert(key, node(std::get<class value>(v)));
				}
				else
				{
					this->insert(key, std::get<luco::node>(v));
				}
			}
		}
		else if constexpr (is_allowed_node_type<container_or_node_type>)
		{
			std::variant<class value, luco::node> n = this->handle_allowed_node_types(node_value);
			if (std::holds_alternative<class value>(n))
			{
				_node = std::make_shared<class value>(std::get<class value>(n));
			}
			else
			{
				_node = std::get<luco::node>(n)._node;
			}
		}
		else if constexpr (is_allowed_variant<container_or_node_type>)
		{
			std::variant<class value, luco::node> n = std::visit(
			    [](auto&& arg)
			    {
				    return node::handle_allowed_node_types(arg);
			    },
			    node_value);
			if (std::holds_alternative<class value>(n))
			{
				_node = std::make_shared<class value>(std::get<class value>(n));
			}
			else
			{
				_node = std::get<luco::node>(n)._node;
			}
		}
		else
		{
			static_assert(false && "unsupported type in the luco::node constructor");
		}
	}

	void node::handle_std_any(const std::any& any_value, std::function<void(std::any)> insert_func)
	{
		if (any_value.type() == typeid(luco::node))
		{
			auto val = std::any_cast<luco::node>(any_value);
			insert_func(val);
		}
		else
		{
			class value value;
			if (any_value.type() == typeid(bool))
			{
				auto val = std::any_cast<bool>(any_value);
				value.set_value_type(val);
				insert_func(value);
			}
			else if (any_value.type() == typeid(double))
			{
				auto val = std::any_cast<double>(any_value);
				value.set_value_type(val);
				insert_func(value);
			}
			else if (any_value.type() == typeid(int))
			{
				auto val = std::any_cast<int>(any_value);
				value.set_value_type(val);
				insert_func(value);
			}
			else if (any_value.type() == typeid(float))
			{
				auto val = std::any_cast<float>(any_value);
				value.set_value_type(val);
				insert_func(value);
			}
			else if (any_value.type() == typeid(const char*))
			{
				auto val = std::any_cast<const char*>(any_value);
				value.set_value_type(val);
				insert_func(value);
			}
			else if (any_value.type() == typeid(std::string))
			{
				auto val = std::any_cast<std::string>(any_value);
				value.set_value_type(val);
				insert_func(value);
			}
			else if (any_value.type() == typeid(luco::null_type))
			{
				value.set_value_type(luco::null);
				insert_func(value);
			}
			else
			{
				throw error(error_type::wrong_type,
					    std::string("unknown type given to luco::node constructor: ") + any_value.type().name());
			}
		}
	}

	node::node(const std::initializer_list<std::pair<std::string, std::any>>& pairs) : _node(std::make_shared<luco::object>())
	{
		std::string key;
		auto	    map		= this->as_object();

		auto	    insert_func = [&](const std::any& value)
		{
			if (value.type() == typeid(luco::node))
			{
				auto val = std::any_cast<luco::node>(value);
				map->insert(key, val);
			}
			else
			{
				auto val = std::any_cast<class value>(value);
				map->insert(key, luco::node(val));
			}
		};

		for (const auto& pair : pairs)
		{
			key = pair.first;
			this->handle_std_any(pair.second, insert_func);
			key.clear();
		}
	}

	node::node(const std::initializer_list<std::any>& val) : _node(std::make_shared<luco::array>())
	{
		auto vector	 = this->as_array();

		auto insert_func = [&](const std::any& value)
		{
			if (value.type() == typeid(luco::node))
			{
				auto val = std::any_cast<luco::node>(value);
				vector->push_back(val);
			}
			else
			{
				auto val = std::any_cast<class value>(value);
				vector->push_back(luco::node(val));
			}
		};

		for (const auto& value : val)
		{
			this->handle_std_any(value, insert_func);
		}
	}

	expected<class luco::node, error> node::add_node_to_array(const size_t index, const luco::node& node)
	{
		if (not this->is_array())
		{
			return unexpected(error(error_type::wrong_type, "wrong type: trying to add node to an array node"));
		}

		auto arr = this->as_array();
		if (index >= arr->size())
		{
			return unexpected(
			    error(error_type::wrong_type, "wrong type: trying to add node to an array node at an out-of-band index"));
		}

		(*arr)[index] = node;

		return (*arr)[index];
	}

	template<typename container_or_node_type>
	expected<std::reference_wrapper<luco::node>, error> node::insert(const std::string& key, const container_or_node_type& value)
	{
		if (not this->is_object())
		{
			return unexpected(error(error_type::wrong_type, "wrong type: trying to add node to an object node"));
		}

		auto obj = this->as_object();
		return std::ref(obj->insert(key, luco::node(value)));
	}

	template<typename container_or_node_type>
	expected<std::reference_wrapper<luco::node>, error> node::push_back(const container_or_node_type& value)
	{
		luco::node n(value);
		if (not this->is_array())
		{
			return unexpected(error(error_type::wrong_type, "wrong type: trying to add node to an array node"));
		}

		auto arr = this->as_array();
		arr->push_back(luco::node(value));
		return std::ref(arr->back());
	}

	expected<class luco::node, error> node::add_value_to_array(const size_t index, const class value& value)
	{
		return this->add_node_to_array(index, luco::node(value));
	}

	expected<std::shared_ptr<class value>, error> node::try_as_value() const noexcept
	{
		if (not this->is_value())
		{
			return unexpected(
			    error(error_type::wrong_type, "wrong type: trying to cast a '{}' node to a value", this->type_name()));
		}

		return std::get<std::shared_ptr<class value>>(_node);
	}

	expected<std::shared_ptr<luco::array>, error> node::try_as_array() const noexcept
	{
		if (not this->is_array())
		{
			return unexpected(
			    error(error_type::wrong_type, "wrong type: trying to cast a '{} node to an array", this->type_name()));
		}

		return std::get<std::shared_ptr<luco::array>>(_node);
	}

	expected<std::shared_ptr<luco::object>, error> node::try_as_object() const noexcept
	{
		if (not this->is_object())
		{
			return unexpected(
			    error(error_type::wrong_type, "wrong type: trying to cast a '{}' node to an object", this->type_name()));
		}

		return std::get<std::shared_ptr<luco::object>>(_node);
	}

	std::shared_ptr<class value> node::as_value() const
	{
		auto ok = this->try_as_value();
		if (not ok)
		{
			throw ok.error();
		}

		return ok.value();
	}

	std::shared_ptr<luco::array> node::as_array() const
	{
		auto ok = this->try_as_array();
		if (not ok)
		{
			throw ok.error();
		}

		return ok.value();
	}

	std::shared_ptr<luco::object> node::as_object() const
	{
		auto ok = this->try_as_object();
		if (not ok)
		{
			throw ok.error();
		}

		return ok.value();
	}

	bool node::is_value() const noexcept
	{
		if (std::holds_alternative<std::shared_ptr<class value>>(_node))
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	bool node::is_array() const noexcept
	{
		if (std::holds_alternative<std::shared_ptr<luco::array>>(_node))
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	bool node::is_object() const noexcept
	{
		if (std::holds_alternative<std::shared_ptr<luco::object>>(_node))
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	bool node::is_string() const noexcept
	{
		return not this->is_value() ? false : this->as_value()->is_string();
	}

	bool node::is_integer() const noexcept
	{
		return not this->is_value() ? false : this->as_value()->is_integer();
	}

	bool node::is_double() const noexcept
	{
		return not this->is_value() ? false : this->as_value()->is_double();
	}

	bool node::is_number() const noexcept
	{
		return not this->is_value() ? false : this->as_value()->is_number();
	}

	bool node::is_boolean() const noexcept
	{
		return not this->is_value() ? false : this->as_value()->is_boolean();
	}

	bool node::is_null() const noexcept
	{
		return not this->is_value() ? false : this->as_value()->is_null();
	}

	node_type node::type() const noexcept
	{
		if (this->is_value())
		{
			return node_type::value;
		}
		else if (this->is_array())
		{
			return node_type::array;
		}
		else
		{
			return node_type::object;
		}
	}

	std::string node::type_name() const noexcept
	{
		if (this->is_value())
		{
			return "node value";
		}
		else if (this->is_array())
		{
			return "node array";
		}
		else
		{
			return "node object";
		}
	}

	template<is_allowed_value_type T>
	expected<T, error> node::access_value(std::function<expected<T, error>(std::shared_ptr<class value>)> fun) const
	{
		auto val = this->try_as_value();
		if (not val)
		{
			return val.error();
		}

		return fun(val.value());
	}

	expected<std::string, error> node::try_as_string() const noexcept
	{
		auto cast_fn = [](std::shared_ptr<class luco::value> val) -> expected<std::string, error>
		{
			return val->try_as_string();
		};
		return this->access_value<std::string>(cast_fn);
	}

	expected<int64_t, error> node::try_as_integer() const noexcept
	{
		auto cast_func = [](std::shared_ptr<class luco::value> val) -> expected<int64_t, error>
		{
			return val->try_as_integer();
		};
		return this->access_value<int64_t>(cast_func);
	}

	expected<double, error> node::try_as_double() const noexcept
	{
		auto cast_func = [](std::shared_ptr<class luco::value> val) -> expected<double, error>
		{
			return val->try_as_double();
		};
		return this->access_value<double>(cast_func);
	}

	expected<double, error> node::try_as_number() const noexcept
	{
		auto cast_func = [](std::shared_ptr<class luco::value> val) -> expected<double, error>
		{
			return val->try_as_number();
		};
		return this->access_value<double>(cast_func);
	}

	expected<bool, error> node::try_as_boolean() const noexcept
	{
		auto cast_func = [](std::shared_ptr<class luco::value> val) -> expected<bool, error>
		{
			return val->try_as_boolean();
		};
		return this->access_value<bool>(cast_func);
	}

	expected<null_type, error> node::try_as_null() const noexcept
	{
		auto cast_func = [](std::shared_ptr<class luco::value> val) -> expected<null_type, error>
		{
			return val->try_as_null();
		};
		return this->access_value<null_type>(cast_func);
	}

	std::string node::as_string() const
	{
		auto ok = this->try_as_string();
		if (not ok)
		{
			throw ok.error();
		}
		return ok.value();
	}

	int64_t node::as_integer() const
	{
		auto ok = this->try_as_integer();
		if (not ok)
		{
			throw ok.error();
		}
		return ok.value();
	}

	double node::as_double() const
	{
		auto ok = this->try_as_double();
		if (not ok)
		{
			throw ok.error();
		}
		return ok.value();
	}

	double node::as_number() const
	{
		auto ok = this->try_as_number();
		if (not ok)
		{
			throw ok.error();
		}
		return ok.value();
	}

	bool node::as_boolean() const
	{
		auto ok = this->try_as_boolean();
		if (not ok)
		{
			throw ok.error();
		}
		return ok.value();
	}

	null_type node::as_null() const
	{
		auto ok = this->try_as_null();
		if (not ok)
		{
			throw ok.error();
		}
		return ok.value();
	}

	value_type node::valuetype() const noexcept
	{
		if (not this->is_value())
		{
			return value_type::none;
		}
		return this->as_value()->type();
	}

	std::string node::value_type_name() const noexcept
	{
		if (not this->is_value())
		{
			return "none";
		}
		return this->as_value()->type_name();
	}

	std::string node::stringify() const noexcept
	{
		if (this->is_value())
		{
			return this->as_value()->stringify();
		}
		else
		{
			return this->dump_to_string();
		}
	}

	bool node::contains(const std::string& key) const noexcept
	{
		auto obj = this->try_as_object();
		if (not obj)
		{
			return false;
		}
		auto itr = obj.value()->find(key);

		if (itr != obj.value()->end())
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	class node& node::at(const std::string& object_key) const
	{
		auto obj = this->as_object();
		auto itr = obj->find(object_key);
		if (itr == obj->end())
		{
			throw error(error_type::key_not_found, std::format("key: '{}' not found", object_key));
		}
		return itr->second;
	}

	class node& node::at(const size_t array_index) const
	{
		auto arr = this->as_array();
		if (array_index >= arr->size())
		{
			throw error(error_type::key_not_found, "index: '{}' not found", array_index);
		}

		return arr->at(array_index);
	}

	expected<std::reference_wrapper<luco::node>, luco::error> node::try_at(const std::string& object_key) const noexcept
	{
		auto obj = this->try_as_object();
		if (not obj)
		{
			return unexpected(obj.error());
		}
		auto itr = obj.value()->find(object_key);
		if (itr == obj.value()->end())
		{
			return unexpected(error(error_type::key_not_found, std::format("key: '{}' not found", object_key)));
		}

		return std::ref(itr->second);
	}

	expected<std::reference_wrapper<luco::node>, luco::error> node::try_at(const size_t array_index) const noexcept
	{
		auto arr = this->try_as_array();
		if (not arr)
		{
			return unexpected(error(error_type::key_not_found, "index: '{}' not found", array_index));
		}

		return std::ref(arr.value()->at(array_index));
	}

	template<typename container_or_node_type>
	class node& node::operator=(const container_or_node_type& node_value) noexcept
	{
		if constexpr (std::is_same_v<container_or_node_type, luco::node>)
		{
			if (this != &node_value)
			{
				this->setting_allowed_node_type(node_value);
			}
		}
		else
		{
			this->setting_allowed_node_type(node_value);
		}
		return *this;
	}

	class node& node::operator+=(const std::initializer_list<std::pair<std::string, std::any>>& pairs)
	{
		if (not this->is_object())
		{
			throw error(error_type::wrong_type, "wrong type: trying to insert pairs to a non-object");
		}

		std::string key;
		auto	    map		= this->as_object();

		auto	    insert_func = [&](const std::any& value)
		{
			if (value.type() == typeid(luco::node))
			{
				auto val = std::any_cast<luco::node>(value);
				map->insert(key, val);
			}
			else
			{
				auto val = std::any_cast<class value>(value);
				map->insert(key, luco::node(val));
			}
		};

		for (const auto& pair : pairs)
		{
			key = pair.first;
			this->handle_std_any(pair.second, insert_func);
			key.clear();
		}
		return *this;
	}

	class node& node::operator+=(const std::initializer_list<std::any>& val)
	{
		if (not this->is_array())
		{
			throw error(error_type::wrong_type, "wrong type: trying to insert pairs to a non-array");
		}

		auto vector	 = this->as_array();

		auto insert_func = [&](const std::any& value)
		{
			if (value.type() == typeid(luco::node))
			{
				auto val = std::any_cast<luco::node>(value);
				vector->push_back(val);
			}
			else
			{
				auto val = std::any_cast<class value>(value);
				vector->push_back(luco::node(val));
			}
		};

		for (const auto& value : val)
		{
			this->handle_std_any(value, insert_func);
		}
		return *this;
	}

	class node node::operator+(const node& other_node)
	{
		if (this->type() != other_node.type())
		{
			throw error(error_type::wrong_type, "trying to + two nodes with different types");
		}

		if (this->is_object())
		{
			luco::node new_node(luco::node_type::object);
			for (const auto& [key, node] : *this->as_object())
			{
				new_node.insert(key, node);
			}
			for (const auto& [key, node] : *other_node.as_object())
			{
				new_node.insert(key, node);
			}

			return new_node;
		}
		else if (this->is_array())
		{
			luco::node new_node(luco::node_type::array);
			for (const auto& node : *this->as_array())
			{
				new_node.push_back(node);
			}
			for (const auto& node : *other_node.as_array())
			{
				new_node.push_back(node);
			}

			return new_node;
		}
		else
		{
			luco::node new_node(this->type());
			auto	   val = this->as_value();

			if (val->type() == value_type::string)
			{
				new_node = this->as_value()->as_string() + other_node.as_value()->as_string();
			}
			else if (val->type() == value_type::double_t || val->type() == value_type::integer)
			{
				new_node = this->as_value()->as_number() + other_node.as_value()->as_number();
			}
			else
			{
				throw error(error_type::wrong_type,
					    "trying to + two nodes with values that are neither a number nor string");
			}

			return new_node;
		}
	}

	template<typename container_or_node_type>
	void node::set(const container_or_node_type& node_value) noexcept
	{
		this->setting_allowed_node_type(node_value);
	}

	void node::dump_to_json(const std::function<void(std::string)> out_func, const std::pair<char, size_t>& indent_conf,
				size_t indent) const
	{
		using node_or_value = std::variant<std::shared_ptr<class value>, luco::node>;

		auto dump_val	    = [&indent, &out_func, &indent_conf](const node_or_value& value_or_nclass)
		{
			if (std::holds_alternative<std::shared_ptr<class value>>(value_or_nclass))
			{
				auto val = std::get<std::shared_ptr<class value>>(value_or_nclass);
				assert(val != nullptr);
				if (val->type() == luco::value_type::string)
				{
					out_func(std::format("\"{}\"", val->stringify()));
				}
				else
				{
					out_func(std::format("{}", val->stringify()));
				}
			}
			else
			{
				std::get<luco::node>(value_or_nclass).dump_to_json(out_func, indent_conf, indent + indent_conf.second);
			}
		};

		if (this->is_object())
		{
			out_func(std::format("{{\n"));
			auto   map   = this->as_object();
			size_t count = 0;
			for (const auto& pair : *map)
			{
				out_func(
				    std::format("{}\"{}\": ", std::string(indent + indent_conf.second, indent_conf.first), pair.first));
				dump_val(pair.second);

				if (++count != map->size())
				{
					out_func(std::format(","));
				}

				out_func(std::format("\n"));
			}
			out_func(std::format("{}}}", std::string(indent, indent_conf.first)));
		}
		else if (this->is_array())
		{
			out_func(std::format("[\n"));
			auto   vector = this->as_array();
			size_t count  = 0;
			for (const auto& array_value : *vector)
			{
				out_func(std::format("{}", std::string(indent + indent_conf.second, indent_conf.first)));
				dump_val(array_value);

				if (++count != vector->size())
				{
					out_func(std::format(","));
				}

				out_func(std::format("\n"));
			}
			out_func(std::format("{}]", std::string(indent, indent_conf.first)));
		}
		else if (this->is_value())
		{
			auto val = this->as_value();
			dump_val(val);
		}
	}

	void node::dump_to_luco(const std::function<void(std::string)> out_func, const std::pair<char, size_t>& indent_conf,
				size_t indent) const
	{
		using node_or_value = std::variant<std::shared_ptr<class value>, luco::node>;

		auto dump_val	    = [&indent, &out_func, &indent_conf](const node_or_value& value_or_nclass)
		{
			if (std::holds_alternative<std::shared_ptr<class value>>(value_or_nclass))
			{
				auto val = std::get<std::shared_ptr<class value>>(value_or_nclass);
				assert(val != nullptr);
				if (val->type() == luco::value_type::string)
				{
					out_func(std::format("\"{}\"", val->stringify()));
				}
				else
				{
					out_func(std::format("{}", val->stringify()));
				}
			}
			else
			{
				std::get<luco::node>(value_or_nclass).dump_to_luco(out_func, indent_conf, indent + indent_conf.second);
			}
		};

		if (this->is_object())
		{
			size_t indent_width = 0;
			if (indent != 0)
			{
				indent_width = indent + indent_conf.second;
				out_func(std::format("{{\n"));
			}
			else
			{
				indent_width = 0;
			}

			auto map = this->as_object();
			for (const auto& pair : *map)
			{
				if (pair.second.is_array() || pair.second.is_object())
				{
					out_func(std::format("{}{} ", std::string(indent_width, indent_conf.first), pair.first));
				}
				else
				{
					out_func(std::format("{}{} = ", std::string(indent_width, indent_conf.first), pair.first));
				}
				dump_val(pair.second);
				out_func(std::format("\n"));
			}
			if (indent != 0)
			{
				out_func(std::format("{}}}", std::string(indent, indent_conf.first)));
			}
		}
		else if (this->is_array())
		{
			out_func(std::format("{{\n"));
			auto vector = this->as_array();
			for (const auto& array_value : *vector)
			{
				out_func(std::format("{}", std::string(indent + indent_conf.second, indent_conf.first)));
				dump_val(array_value);
				out_func(std::format("\n"));
			}
			out_func(std::format("{}}}", std::string(indent, indent_conf.first)));
		}
		else if (this->is_value())
		{
			auto val = this->as_value();
			dump_val(val);
		}
	}

	void node::dump_to_stdout(const std::pair<char, size_t>& indent_conf) const
	{
		auto func = [](const std::string& output)
		{
			std::cout << output;
		};
		this->dump_to_luco(func, indent_conf);
	}

	std::string node::dump_to_string(const std::pair<char, size_t>& indent_conf) const
	{
		std::string data;
		auto	    func = [&data](const std::string& output)
		{
			data += output;
		};
		this->dump_to_luco(func, indent_conf);

		return data;
	}

	expected<monostate, error> node::dump_to_file(const std::filesystem::path& path, const std::pair<char, size_t>& indent_conf) const
	{
		std::ofstream file(path);
		if (not file.is_open())
		{
			return unexpected(error(error_type::filesystem_error, std::strerror(errno)));
		}

		auto func = [&file](const std::string& output)
		{
			file << output;
		};
		this->dump_to_luco(func, indent_conf);
		file.close();

		return monostate();
	}
}
