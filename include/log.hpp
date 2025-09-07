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
#ifdef VERBOSE_LOGS
	inline std::string log(const std::string& msg, std::source_location location = std::source_location::current())
#else
	inline std::string log(const std::string&, std::source_location __ = std::source_location::current())
#endif
	{
#ifdef VERBOSE_LOGS
		const std::string reset	 = "\x1b[0m";
		const std::string color1 = "\x1b[31m";
		const std::string color2 = "\x1b[32m";
		const std::string color3 = "\x1b[34m";

		std::string	  output = std::format("{}[{}:{}]{}\n", color1, location.file_name(), location.line(), reset);
		output += std::format("{}\t|{}\n", color3, reset);
		output += std::format("{}\t|__function--> {}{}\n", color3, location.function_name(), reset);
		output += std::format("{}\t\t|{}\n", color2, reset);
		output += std::format("{}\t\t|__message--> '{}'{}\n", color2, msg, reset);
		output += std::format("\n");

		return output;
#else
		( void ) __;
		return "";
#endif
	}

#ifdef VERBOSE_LOGS
	inline void print_log(const std::string& msg, std::source_location location = std::source_location::current())
#else
	inline void print_log(const std::string&, std::source_location __ = std::source_location::current())
#endif
	{
#ifdef VERBOSE_LOGS
		std::cout << log(msg, location);
#else
		( void ) __;
#endif
	}
}
