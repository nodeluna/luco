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
#include "concepts.hpp"

/**
 * @brief the namespace for luco
 */
namespace luco
{
	/**
	 * @enum error_type
	 * @brief error values used in luco::error
	 */
	enum class error_type {
		none,
		key_not_found,
		filesystem_error,
		parsing_error,
		parsing_error_wrong_type,
		wrong_type,
		wronge_index,
	};

	/**
	 * @class error
	 * @brief a class type inspired by std::error_code to handle errors
	 */
	class error {
		private:
			error_type  err_type;
			std::string msg;

		public:
			/**
			 * @brief constructor
			 * @param err holds the value of luco::error_type
			 * @param message the string message of the error
			 */
			error(error_type err, const std::string& message) noexcept;

			/**
			 * @brief constructor
			 * @param err holds the value of luco::error_type
			 * @param fmt std::format() type
			 * @param args the arguments for std::format()
			 * @tparam args_t the types of arguments for std::format()
			 */
			template<typename... args_t>
			error(error_type err, std::format_string<args_t...> fmt, args_t&&... args) noexcept;

			/**
			 * @brief get the string message of the error
			 * @return get the string message of the error
			 */
			const char*	   what() const noexcept;

			/**
			 * @brief get the string message of the error
			 * @return get the string message of the error
			 */
			const std::string& message() const noexcept;

			/**
			 * @brief get the luco::error_type of the error
			 * @return get the luco::error_type of the error
			 */
			error_type	   value() const noexcept;
	};
}

namespace luco
{
	error::error(error_type err, const std::string& message) noexcept : err_type(err), msg(message)
	{
	}

	template<typename... args_t>
	error::error(error_type err, std::format_string<args_t...> fmt, args_t&&... args) noexcept
	    : err_type(err), msg(std::format(fmt, std::forward<args_t>(args)...))
	{
	}

	const char* error::what() const noexcept
	{
		return msg.c_str();
	}

	const std::string& error::message() const noexcept
	{
		return msg;
	}

	error_type error::value() const noexcept
	{
		return err_type;
	}
}
