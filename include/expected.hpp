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

/**
 * @brief the namespace for luco
 */
namespace luco
{
	/**
	 * @class monostate
	 * @brief an implementation of std::monostate to allow using luco with C++20. check the cppreference
	 */
	class monostate {};

	/**
	 * @class expected
	 * @brief an implementation of std::expected to allow using luco with C++20. check the cppreference
	 */
	template<class T = monostate, class E = monostate>
	class expected {
		private:
			bool _has_value = false;

			union {
					T _value;
					E _error;
			};

		public:
			constexpr expected(const T& t);

			constexpr expected(const E& e);

			constexpr expected();

			constexpr expected& operator=(const expected& other);

			template<class U>
			constexpr expected(const expected<U, E>& other);

			constexpr expected(const expected& other);

			constexpr expected(const expected&& other) noexcept;

			constexpr expected& operator=(const expected&& other) noexcept;

			constexpr ~expected();

			constexpr bool	    has_value() const noexcept;

			constexpr explicit  operator bool() const noexcept;

			constexpr const T&  value() const&;

			constexpr const E&  error() const&;

			constexpr T&	    value() &;

			constexpr E&	    error() &;

			constexpr const T&& value() const&&;

			constexpr const E&& error() const&&;

			constexpr T&&	    value() &&;

			constexpr E&&	    error() &&;

			template<class U = typename std::remove_cv<T>::type>
			constexpr T value_or(U&& other) const&;

			template<class U = typename std::remove_cv<E>::type>
			constexpr E error_or(U&& other) const&;
	};

	/**
	 * @brief a helper function to construct an unexpected type
	 */
	template<class E>
	constexpr expected<monostate, E>	   unexpected(const E& e);

	/**
	 * @brief a helper function to construct an unexpected type
	 * @detail @cpp
	 * luco::expected<luco::node, std::string> node = luco::unexpected("error");
	 * @ecpp
	 */
	constexpr expected<monostate, std::string> unexpected(const char* e);
}

/**
 * @brief the namespace for luco
 */
namespace luco
{
	template<class T, class E>
	constexpr expected<T, E>::expected(const T& t) : _has_value(true)
	{
		std::construct_at(std::addressof(_value), T(t));
	}

	template<class T, class E>
	constexpr expected<T, E>::expected(const E& e) : _has_value(false)
	{
		std::construct_at(std::addressof(_error), E(e));
	}

	template<class T, class E>
	constexpr expected<T, E>::expected() : _has_value(true)
	{
		static_assert(std::is_default_constructible<T>::value, "");
		std::construct_at(std::addressof(_value), T());
	}

	template<class T, class E>
	constexpr expected<T, E>& expected<T, E>::operator=(const expected<T, E>& other)
	{
		static_assert(std::is_copy_assignable<T>::value && std::is_copy_assignable<E>::value, "");
		if (this != &other)
		{
			this->~expected();
			std::construct_at(this, expected(other));
		}

		return *this;
	}

	template<class T, class E>
	template<class U>
	constexpr expected<T, E>::expected(const expected<U, E>& other) : _has_value(other.has_value())
	{
		static_assert(std::is_same<U, monostate>::value, "no available conversion between the provided value types");
		static_assert(std::is_copy_constructible<T>::value && std::is_copy_constructible<E>::value, "");
		if (_has_value)
		{

			if constexpr (std::is_default_constructible<T>::value)
			{
				std::construct_at(std::addressof(_value), T());
			}
			else
			{
				std::construct_at(std::addressof(_error), E(other.error()));
			}
		}
		else
		{
			std::construct_at(std::addressof(_error), E(other.error()));
		}
	}

	template<class T, class E>
	constexpr expected<T, E>::expected(const expected& other) : _has_value(other.has_value())
	{
		static_assert(std::is_copy_constructible<T>::value && std::is_copy_constructible<E>::value, "");
		if (_has_value)
		{
			std::construct_at(std::addressof(_value), T(other.value()));
		}
		else
		{
			std::construct_at(std::addressof(_error), E(other.error()));
		}
	}

	template<class T, class E>
	constexpr expected<T, E>::expected(const expected&& other) noexcept : _has_value(other._has_value)
	{
		static_assert(std::is_move_constructible<T>::value && std::is_move_constructible<E>::value, "");
		if (this->has_value())
		{
			std::construct_at(std::addressof(_value), T(std::move(other._value)));
		}
		else
		{
			std::construct_at(std::addressof(_error), E(std::move(other._error)));
		}
	}

	template<class T, class E>
	constexpr expected<T, E>& expected<T, E>::operator=(const expected<T, E>&& other) noexcept
	{
		static_assert(std::is_move_assignable<T>::value && std::is_move_assignable<E>::value, "");
		if (this != &other)
		{
			this->~expected();
			std::construct_at(this, expected(std::move(other)));
		}

		return *this;
	}

	template<class T, class E>
	constexpr expected<T, E>::~expected()
	{
		if (this->has_value())
		{
			_value.~T();
		}
		else
		{
			_error.~E();
		}
	}

	template<class T, class E>
	constexpr bool expected<T, E>::has_value() const noexcept
	{
		return _has_value;
	}

	template<class T, class E>
	constexpr expected<T, E>::operator bool() const noexcept
	{
		return this->has_value();
	}

	template<class T, class E>
	constexpr const T& expected<T, E>::value() const&
	{
		if (not _has_value)
		{
			throw std::runtime_error("Attempted to access the value of a error state");
		}
		return _value;
	}

	template<class T, class E>
	constexpr const E& expected<T, E>::error() const&
	{
		if (_has_value)
		{
			throw std::runtime_error("Attempted to access the error of a value state");
		}
		return _error;
	}

	template<class T, class E>
	constexpr T& expected<T, E>::value() &
	{
		if (not _has_value)
		{
			throw std::runtime_error("Attempted to access the value of a error state");
		}
		return _value;
	}

	template<class T, class E>
	constexpr E& expected<T, E>::error() &
	{
		if (_has_value)
		{
			throw std::runtime_error("Attempted to access the error of a value state");
		}
		return _error;
	}

	template<class T, class E>
	constexpr const T&& expected<T, E>::value() const&&
	{
		if (not _has_value)
		{
			throw std::runtime_error("Attempted to access the value of a error state");
		}
		return std::move(_value);
	}

	template<class T, class E>
	constexpr const E&& expected<T, E>::error() const&&
	{
		if (_has_value)
		{
			throw std::runtime_error("Attempted to access the error of a value state");
		}
		return std::move(_error);
	}

	template<class T, class E>
	constexpr T&& expected<T, E>::value() &&
	{
		if (not _has_value)
		{
			throw std::runtime_error("Attempted to access the value of a error state");
		}
		return std::move(_value);
	}

	template<class T, class E>
	constexpr E&& expected<T, E>::error() &&
	{
		if (_has_value)
		{
			throw std::runtime_error("Attempted to access the error of a value state");
		}
		return std::move(_error);
	}

	template<class T, class E>
	template<class U>
	constexpr T expected<T, E>::value_or(U&& other) const&
	{
		static_assert(std::is_convertible<U, T>::value, "the provided type must be convertible to the value type");
		if (_has_value)
		{
			return _value;
		}
		else
		{
			return static_cast<T>(std::forward<U>(other));
		}
	}

	template<class T, class E>
	template<class U>
	constexpr E expected<T, E>::error_or(U&& other) const&
	{
		static_assert(std::is_convertible<U, E>::value, "the provided type must be convertible to the error type");
		if (not _has_value)
		{
			return _error;
		}
		else
		{
			return static_cast<E>(std::forward<U>(other));
		}
	}

	/**
	 * @brief a helper function to construct an unexpected type
	 */
	template<class E>
	constexpr expected<monostate, E> unexpected(const E& e)
	{
		return expected<monostate, E>(e);
	}

	/**
	 * @brief a helper function to construct an unexpected type
	 * @detail @cpp
	 * luco::expected<luco::node, std::string> node = luco::unexpected("error");
	 * @ecpp
	 */
	constexpr expected<monostate, std::string> unexpected(const char* e)
	{
		return unexpected<std::string>(std::string(e));
	}
}
