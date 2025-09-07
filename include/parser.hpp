/*
 * Copyright: 2025 nodeluna
 * SPDX-License-Identifier: Apache-2.0
 * repository: https://github.com/nodeluna/luco
 */

#pragma once

#include <algorithm>
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
#include <set>
#include <stack>
#include <fstream>
#include <format>
#include <functional>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#include <cassert>
#include <source_location>
#include <type_traits>
#include "concepts.hpp"
#include "expected.hpp"
#include "error.hpp"
#include "log.hpp"

/**
 * @brief the namespace for luco
 */
namespace luco
{
	class parser {
		private:
			inline static bool			 done_or_not_ok(const expected<bool, error>& ok);
			inline static expected<monostate, error> return_error_if_not_ok(const expected<bool, error>& ok);
			inline static expected<monostate, error> parsing(struct parsing_data& data, struct syntax& syntax);

		public:
			inline static luco::node		  parse(const std::filesystem::path& path);
			inline static luco::node		  parse(const std::string& raw_json);
			inline static luco::node		  parse(const char* raw_json);
			inline static expected<luco::node, error> try_parse(const std::filesystem::path& path) noexcept;
			inline static expected<luco::node, error> try_parse(const std::string& raw_json) noexcept;
			inline static expected<luco::node, error> try_parse(const char* raw_json) noexcept;
	};

	enum class luco_syntax {
		none,
		opening_bracket,
		closing_bracket,
		transient_bracket,
		undetermined_transient_bracket,
		quotes_1,
		quotes_2,
		key,
		equal_sign,
		value,
		string_type,
		boolean,
		null,
		number,
		newline,
		empty,
		escaped_newline,
		object,
		comment,
		nested_comment,
		array,
		maybe_empty_space_after,
		flush_value,
	};

	inline std::string syntax_reflect(enum luco_syntax l)
	{
		switch (l)
		{
			case luco_syntax::none:
				return "none";
			case luco_syntax::opening_bracket:
				return "opening_bracket";
			case luco_syntax::closing_bracket:
				return "closing_bracket";
			case luco_syntax::transient_bracket:
				return "transient_bracket";
			case luco_syntax::undetermined_transient_bracket:
				return "undetermined_transient_bracket";
			case luco_syntax::quotes_1:
				return "quotes_1";
			case luco_syntax::quotes_2:
				return "quotes_2";
			case luco_syntax::key:
				return "key";
			case luco_syntax::equal_sign:
				return "equal_sign";
			case luco_syntax::value:
				return "value";
			case luco_syntax::flush_value:
				return "flush_value";
			case luco_syntax::string_type:
				return "string_type";
			case luco_syntax::boolean:
				return "boolean";
			case luco_syntax::number:
				return "number";
			case luco_syntax::empty:
				return "empty";
			case luco_syntax::object:
				return "object";
			case luco_syntax::array:
				return "array";
			case luco_syntax::null:
				return "null";
			case luco_syntax::newline:
				return "newline";
			case luco_syntax::escaped_newline:
				return "escaped_newline";
			case luco_syntax::comment:
				return "comment";
			case luco_syntax::nested_comment:
				return "nested_comment";
			case luco_syntax::maybe_empty_space_after:
				return "maybe_empty_space_after";
		}

		return "";
	}

	inline std::string reflect_stack_syntax(std::stack<std::pair<luco_syntax, std::pair<size_t, size_t>>> hierarchy)
	{
		if (hierarchy.empty())
		{
			return "!nothing!";
		}

		std::string stack;

		do
		{
			stack += ("\n == " + syntax_reflect(hierarchy.top().first));
			hierarchy.pop();
		} while (not hierarchy.empty());

		return stack;
	}

	class token {
		protected:
			std::set<luco_syntax>		 is	       = {};
			std::set<char>			 _not	       = {};
			std::set<luco_syntax>		 pop	       = {};
			std::multimap<luco_syntax, char> expected      = {};
			std::set<char>			 special_chars = {'{', '=', '}', '"', '\'', '\\'};

			inline virtual bool		 is_end_of_token(struct parsing_data& _);
			inline bool			 handle_is(struct parsing_data&, const std::set<luco_syntax>&);
			inline bool			 handle_not(struct parsing_data&, const std::set<char>&);
			inline bool			 handle_expected(struct parsing_data&, const std::multimap<luco_syntax, char>&);

		public:
			inline token()
			{
			}

			inline virtual luco::expected<bool, luco::error> handle_token(struct parsing_data& _);
			inline virtual bool				 is_token(struct parsing_data& _);
			inline virtual void				 prepare_for_next_token(struct parsing_data& _, luco_syntax __);
			inline static bool				 is_escaped(struct parsing_data& data, char ch);
			inline bool					 is_comment(const struct parsing_data& data);
			inline void					 register_token(struct parsing_data& data, luco_syntax token_type);
			inline bool	   is_registered_token(const struct parsing_data&, luco_syntax token_type);
			inline void	   unregister_token(struct parsing_data& data);
			inline static bool is_empty(const char ch);
			inline static bool is_newline(const char ch);
			inline static bool is_empty_newline(const char ch);
			inline static bool delimiter(struct parsing_data& data, char ch);

			inline virtual ~token()
			{
			}
	};
}

namespace luco
{
	enum class luco_value_type {
		none,
		unqouted_string,
		qouted_string_1,
		qouted_string_2,
		escaped_string_newline_unqouted,
		escaped_string_newline_qouted_1,
		escaped_string_newline_qouted_2,
		end_string_1,
		end_string_2,
		end_string_unqouted,
	};

	enum special_char {
		none   = 1 << 0,
		escape = 1 << 1,
		append = 1 << 2,
	};

	struct parsing_data {
			std::string					    line;
			size_t						    i					= 0;
			size_t						    line_number				= 1;
			bool						    shift_index_backward_for_oldnewline = false;
			bool						    eof					= false;
			std::stack<std::pair<std::string, luco_value_type>> keys;
			std::stack<luco::node*>				    luco_objs;
			std::tuple<size_t, bool, char>			    escaped_special_char = std::make_tuple(0, false, '\0');
			std::pair<std::string, luco_value_type>		    raw_value		 = {"", luco_value_type::none};
			class value					    value;
			std::stack<std::pair<luco_syntax, std::pair<size_t, size_t>>> hierarchy;
	};

	inline std::string error_location(const struct parsing_data& data, std::optional<std::pair<size_t, size_t>> location = std::nullopt)
	{
		size_t line_number = 1;
		size_t index	   = 0;
		if (location)
		{
			line_number = location->first;
			index	    = location->second;
		}
		else
		{
			line_number = data.line_number;
			index	    = data.i;
		}

		std::string err_str = "\x1b[1;31m";
		err_str += std::format("{0}:{1}\n", line_number, index);
		err_str += std::format("  {0}\t|\t{1}", line_number, data.line);

		size_t line_number_width = std::to_string(line_number).size();
		size_t index_width	 = std::to_string(index).size();

#if __cpp_lib_format >= 202311L
		err_str += std::format(std::runtime_format("{}"), "  {0:<{1}}\t|\t", ' ', line_number_width);
		err_str += std::format(std::runtime_format("{}"), "{0:<{1}}", ' ', index_width);
		err_str += std::format(std::runtime_format("{}"), "{0:>{1}}\n", '^', (index == 0 ? index : index - 1));
#else
		err_str += std::format("  {0:<{1}}\t|\t", ' ', line_number_width);
		err_str += std::format("{0:<{1}}", ' ', index_width);
		err_str += std::format("{0:>{1}}\n", '^', (index == 0 ? index : index - 1));
#endif
		err_str += "\x1b[0m";
		return err_str;
	}

	inline std::string dump_data(const parsing_data& data)
	{
		std::string dump = "[data dump]\n";

		dump += std::format("(line: {})\n", data.line);
		dump += std::format("(char: {})\n", (data.line[data.i] == '\n'	 ? "[newline]"
						     : data.line[data.i] == '\t' ? "[tab]"
						     : data.line[data.i] == ' '	 ? "[space]"
										 : std::string(1, data.line[data.i])));
		dump += std::format("(location: line: {}, index: {})\n", data.line_number, data.i);
		dump += std::format("(syntax: {})\n", reflect_stack_syntax(data.hierarchy));
		{
			std::stack<std::pair<std::string, luco_value_type>> keys = data.keys;
			dump += std::format("(keys: ");
			while (not keys.empty())
			{
				dump += std::format("\t{}", keys.top().first);
				keys.pop();
			}
			dump += std::format(")\n");
		}

		dump += std::format("(value: {})\n", data.raw_value.first);

		return dump;
	}

	inline bool token::handle_is(struct parsing_data& data, const std::set<luco_syntax>& is_tokens)
	{
		if (data.hierarchy.empty())
		{
			return false;
		}

		for (const auto& element : is_tokens)
		{
			if (data.hierarchy.top().first == element)
			{
				return true;
			}
		}

		return false;
	}

	inline bool token::handle_not(struct parsing_data& data, const std::set<char>& __not)
	{
		for (const auto& ch : __not)
		{
			if (data.line[data.i] != ch)
			{
				continue;
			}

			assert(data.line[data.i] == ch);

			if (auto itr = special_chars.find(ch); itr != special_chars.end())
			{
				if (not this->is_escaped(data, ch))
				{

					return false;
				}
			}
			else
			{
				return false;
			}
		}

		return true;
	}

	inline bool token::handle_expected(struct parsing_data& data, const std::multimap<luco_syntax, char>& expected_tokens)
	{
		for (const auto& expect : expected_tokens)
		{
			if (expect.second != data.line[data.i])
			{
				continue;
			}

			assert(data.line[data.i] == expect.second);

			if (auto itr = special_chars.find(expect.second); itr != special_chars.end())
			{
				if (not this->is_escaped(data, expect.second))
				{
					return true;
				}
			}
			else
			{
				return true;
			}
		}

		return false;
	}

	inline bool token::delimiter(struct parsing_data& data, char ch)
	{
		if (data.line[data.i] == ch && not token::is_escaped(data, ch))
		{
			return true;
		}
		return false;
	}

	inline bool token::is_escaped(struct parsing_data& data, char ch)
	{
		auto& [escape_char_pos, append_escape_char, escape_char] = data.escaped_special_char;

		auto reset_escape_char					 = [&]()
		{
			escape_char_pos	   = 0;
			append_escape_char = false;
			escape_char	   = '\0';
		};

		std::set<char> special_chars = {'{', '=', '}', '"', '\'', '\\'};

		if (data.i + 1 < data.line.size() && data.line[data.i + 1] == ch)
		{
			if (auto itr = special_chars.find(ch); itr == special_chars.end())
			{
				reset_escape_char();
				return false;
			}
			else if (escape_char != ch)
			{
				append_escape_char = false;
			}
			else if (escape_char_pos != (data.line_number * data.i) && escape_char_pos != 0)
			{
				append_escape_char = true;
				escape_char_pos	   = data.line_number * data.i;
			}
			else
			{
				append_escape_char = false;
			}
			escape_char_pos = data.line_number * data.i;
			escape_char	= ch;

			return true;
		}
		else if (escape_char == ch && not append_escape_char)
		{
			append_escape_char = true;
			escape_char_pos	   = data.line_number * data.i;
			return true;
		}
		else
		{
			reset_escape_char();
			return false;
		}
	}

	inline bool token::is_empty(const char ch)
	{
		if (ch == ' ' || ch == '\t')
		{
			return true;
		}
		return false;
	}

	inline bool token::is_newline(const char ch)
	{
		if (ch == '\n')
		{
			return true;
		}
		return false;
	}

	inline bool token::is_empty_newline(const char ch)
	{
		if (ch == ' ' || ch == '\t' || ch == '\n')
		{
			return true;
		}
		return false;
	}

	inline luco::expected<bool, luco::error> token::handle_token(struct parsing_data&)
	{
		return false;
	}

	inline bool token::is_token(struct parsing_data&)
	{
		return false;
	}

	inline bool token::is_end_of_token(struct parsing_data&)
	{
		return false;
	}

	inline void token::prepare_for_next_token(struct parsing_data&, luco_syntax)
	{
	}

	inline bool token::is_comment(const struct parsing_data& data)
	{
		if (not data.hierarchy.empty() && data.hierarchy.top().first == luco_syntax::comment)
		{
			return true;
		}
		return false;
	}

	inline void token::register_token(struct parsing_data& data, luco_syntax token_type)
	{
		data.hierarchy.push(std::make_pair(token_type, std::make_pair(data.line_number, data.i)));
	}

	inline bool token::is_registered_token(const struct parsing_data& data, luco_syntax token_type)
	{
		if (not data.hierarchy.empty() && data.hierarchy.top().first == token_type)
		{
			return true;
		}
		return false;
	}

	inline void token::unregister_token(struct parsing_data& data)
	{
		assert(not data.hierarchy.empty() && "internal parsing error in token::unregister_token");
		data.hierarchy.pop();
	}

	class luco_simple_types {
		public:
			enum class boolean_types {
				none,
				_true,
				_false,
			};

			enum class number_types {
				none,
				_float,
				_int
			};

			inline static std::variant<std::string, bool, double, int64_t, null_type> get_type(const std::string& raw_value)
			{
				if (luco_simple_types::is_null(raw_value))
				{
					return null_type();
				}
				else if (auto is_number = luco_simple_types::is_number(raw_value); is_number != number_types::none)
				{
					switch (is_number)
					{
						case number_types::_int:
							return std::stoll(raw_value);
						case number_types::_float:
							return std::stod(raw_value);
						case number_types::none:
							assert(false && "oopsie in luco type deduction: check the number_types enum");
							return false;
					}
					return false;
				}
				else if (auto is_bool = luco_simple_types::is_boolean(raw_value); is_bool != boolean_types::none)
				{
					switch (is_bool)
					{
						case boolean_types::_true:
							return true;
						case boolean_types::_false:
							return false;
						case boolean_types::none:
							assert(false && "oopsie in luco type deduction: check the boolean_types enum");
							return false;
					}
					return false;
				}
				else
				{
					return raw_value;
				}
			}

			inline static number_types is_number(const std::string& data)
			{
				if (data.empty())
				{
					return number_types::none;
				}
				bool has_decimal = false;
				bool number	 = std::all_of(data.begin(), data.end(),
							       [&](char c)
							       {
								       if (std::isdigit(c))
								       {
									       return true;
								       }
								       else if (c == '.' && not has_decimal)
								       {
									       has_decimal = true;
									       return true;
								       }
								       else
								       {
									       return false;
								       }
							       });
				if (not number)
				{
					return number_types::none;
				}
				else if (not has_decimal)
				{
					return number_types::_int;
				}
				else
				{
					return number_types::_float;
				}
			}

			inline static boolean_types is_boolean(const std::string& data)
			{
				if (data == "on" || data == "true")
				{
					return boolean_types::_true;
				}
				else if (data == "off" || data == "false")
				{
					return boolean_types::_false;
				}
				else
				{
					return boolean_types::none;
				}
			}

			inline static bool is_null(const std::string& data)
			{
				if (data == "null")
				{
					return true;
				}
				return false;
			}

			inline static bool is_quoted_string(const luco_value_type& key_value_type)
			{
				if (key_value_type == luco_value_type::qouted_string_2)
				{
					return true;
				}
				else if (key_value_type == luco_value_type::qouted_string_1)
				{
					return true;
				}
				return false;
			}

			inline static void strip_if_unqouted_string(std::string& data, const luco_value_type& key_value_type)
			{
				if (key_value_type != luco_value_type::unqouted_string || data.empty() || not token::is_empty(data.back()))
				{
					return;
				}

				size_t i = data.size() - 1;
				for (;; i--)
				{
					if (not token::is_empty(data[i]))
					{
						break;
					}
					if (i == 0)
					{
						break;
					}
				}

				if (i != data.size() - 1)
				{
					data = data.substr(0, i + 1);
				}
			}

			inline static bool append_string(struct parsing_data& data, luco_value_type& key_value_type, char ch)
			{
				// TODO: use delimiter method
				if (token::delimiter(data, '{') || token::delimiter(data, '}'))
				{
					return false;
				}
				else if (not token::is_newline(ch) && key_value_type == luco_value_type::unqouted_string)
				{
					return true;
				}
				else if (not token::delimiter(data, '"') && key_value_type == luco_value_type::qouted_string_2)
				{
					return true;
				}
				else if (not token::delimiter(data, '\'') && key_value_type == luco_value_type::qouted_string_1)
				{
					return true;
				}
				else
				{
					return false;
				}
			}

			inline static bool expected_multi_line_string(luco_value_type& key_value_type)
			{
				if (key_value_type == luco_value_type::escaped_string_newline_qouted_1 ||
				    key_value_type == luco_value_type::escaped_string_newline_qouted_2 ||
				    key_value_type == luco_value_type::escaped_string_newline_unqouted)
				{
					return true;
				}
				else
				{
					return false;
				}
			}

			inline static bool end_of_string(luco_value_type& key_value_type)
			{
				if (key_value_type == luco_value_type::end_string_1 || key_value_type == luco_value_type::end_string_2 ||
				    key_value_type == luco_value_type::end_string_unqouted)
				{
					return true;
				}
				return false;
			}

			inline static bool handle_empty_in_string(struct parsing_data& data, luco_value_type& key_value_type, char ch)
			{
				if (token::is_empty_newline(ch) &&
				    (key_value_type == luco_value_type::none || expected_multi_line_string(key_value_type)))
				{
					return false;
				}
				else if (std::get<size_t>(data.escaped_special_char) != 0)
				{
					auto& [escape_char_pos, append_escape_char, escape_char] = data.escaped_special_char;

					if (not append_escape_char)
					{
						return false;
					}

					escape_char_pos	   = 0;
					append_escape_char = false;
					escape_char	   = '\0';
					if (key_value_type == luco_value_type::none)
					{
						key_value_type = luco_value_type::unqouted_string;
					}
					return true;
				}
				else if (token::delimiter(data, '\\'))
				{
					if (key_value_type == luco_value_type::end_string_1)
					{
						key_value_type = luco_value_type::escaped_string_newline_qouted_1;
					}
					else if (key_value_type == luco_value_type::end_string_2)
					{
						key_value_type = luco_value_type::escaped_string_newline_qouted_2;
					}
					else
					{
						key_value_type = luco_value_type::escaped_string_newline_unqouted;
					}
					return false;
				}
				else if (key_value_type == luco_value_type::escaped_string_newline_qouted_1)
				{
					if (token::delimiter(data, '\''))
					{
						key_value_type = luco_value_type::qouted_string_1;
					}
					return false;
				}
				else if (key_value_type == luco_value_type::escaped_string_newline_qouted_2)
				{
					if (token::delimiter(data, '"'))
					{
						key_value_type = luco_value_type::qouted_string_2;
					}
					return false;
				}
				else if (key_value_type == luco_value_type::escaped_string_newline_unqouted)
				{
					key_value_type = luco_value_type::unqouted_string;
					return true;
				}
				else if (append_string(data, key_value_type, ch))
				{
					return true;
				}
				else if (key_value_type == luco_value_type::none)
				{
					if (token::delimiter(data, '\''))
					{
						key_value_type = luco_value_type::qouted_string_1;
					}
					else if (token::delimiter(data, '"'))
					{
						key_value_type = luco_value_type::qouted_string_2;
					}
					else if (not token::delimiter(data, '{') && not token::delimiter(data, '}'))
					{
						key_value_type = luco_value_type::unqouted_string;
						return true;
					}
					return false;
				}
				else if (key_value_type == luco_value_type::end_string_1 || key_value_type == luco_value_type::end_string_2)
				{
					return false;
				}
				else if (token::delimiter(data, '\'') && key_value_type == luco_value_type::qouted_string_1)
				{
					key_value_type = luco_value_type::end_string_1;
					return true;
				}
				else if (token::delimiter(data, '"') && key_value_type == luco_value_type::qouted_string_2)
				{
					key_value_type = luco_value_type::end_string_2;
					return true;
				}
				else if (token::is_newline(data.line[data.i]) && key_value_type == luco_value_type::unqouted_string)
				{
					key_value_type = luco_value_type::end_string_unqouted;
					return true;
				}
				else if (ch == '\n')
				{
					return false;
				}
				else
				{
					return false;
				}
			}
	};

	class comment : public token {
		private:
			unsigned long brackets_count = 0;

			inline bool   is_end_of_token(struct parsing_data& data) override
			{
				if (not data.hierarchy.empty() && data.hierarchy.top().first == luco_syntax::comment &&
				    this->handle_expected(data, this->expected))
				{
					return true;
				}
				else if (not data.hierarchy.empty() && data.hierarchy.top().first == luco_syntax::nested_comment &&
					 this->delimiter(data, '}') && brackets_count == 0)
				{
					return true;
				}

				return false;
			}

		public:
			inline comment()
			{
				this->expected.insert(std::make_pair(luco_syntax::newline, '\n'));
			}

			inline ~comment()
			{
			}

			inline luco::expected<bool, luco::error> handle_token(struct parsing_data& data) override
			{
				if (this->is_token(data))
				{
					this->register_token(data, luco_syntax::comment);
				}
				else if (this->is_end_of_token(data))
				{
					if (data.hierarchy.top().first != luco_syntax::nested_comment)
					{
						data.shift_index_backward_for_oldnewline = true;
					}
					this->unregister_token(data);
					return true;
				}

				if (this->is_registered_token(data, luco_syntax::comment))
				{
					if (this->delimiter(data, '{'))
					{
						this->unregister_token(data);
						this->register_token(data, luco_syntax::nested_comment);
					}
					return true;
				}
				else if (this->is_registered_token(data, luco_syntax::nested_comment))
				{
					if (this->delimiter(data, '{'))
					{
						this->brackets_count++;
					}
					else if (this->delimiter(data, '}'))
					{
						if (this->brackets_count > 0)
						{
							this->brackets_count--;
						}
					}
					return true;
				}
				else
				{
					return false;
				}
			}

			inline bool is_token(struct parsing_data& data) override
			{
				if (this->delimiter(data, '#'))
				{
					return true;
				}
				else
				{
					return false;
				}
			}
	};

	class opening_bracket : public token {
		private:
			inline bool is_end_of_token(struct parsing_data& data) override
			{
				if ((data.raw_value.second == luco_value_type::none && not delimiter(data, '{')) || data.hierarchy.empty())
				{
					return false;
				}
				else if (data.hierarchy.top().first == luco_syntax::transient_bracket &&
					 this->handle_expected(data, this->expected))
				{
					return true;
				}
				return false;
			}

		public:
			inline opening_bracket()
			{
				this->is.insert(luco_syntax::opening_bracket);

				this->expected.insert(std::make_pair(luco_syntax::object, '='));
				this->expected.insert(std::make_pair(luco_syntax::object, '{'));
				this->expected.insert(std::make_pair(luco_syntax::array, '\n'));
			}

			inline ~opening_bracket()
			{
			}

			inline luco::expected<bool, luco::error> handle_token(struct parsing_data& data) override
			{
				if (this->is_token(data))
				{
					this->unregister_token(data);
					this->register_token(data, luco_syntax::transient_bracket);
				}
				else if (this->is_end_of_token(data))
				{
					this->unregister_token(data);
					luco_simple_types::strip_if_unqouted_string(data.keys.top().first, data.keys.top().second);

					if (this->delimiter(data, '='))
					{
						this->prepare_for_next_token(data, luco_syntax::object);
						this->prepare_for_next_token(data, luco_syntax::equal_sign);
					}
					else if (this->delimiter(data, '{') && data.raw_value.second != luco_value_type::none)
					{
						this->prepare_for_next_token(data, luco_syntax::object);
						this->prepare_for_next_token(data, luco_syntax::opening_bracket);
					}
					else if (this->is_newline(data.line[data.i]))
					{
						this->prepare_for_next_token(data, luco_syntax::array);
						this->prepare_for_next_token(data, luco_syntax::flush_value);
					}
					else if (this->delimiter(data, '{') && data.raw_value.second == luco_value_type::none)
					{
						this->prepare_for_next_token(data, luco_syntax::array);
						auto ok = data.luco_objs.top()->insert(data.keys.top().first, luco::node(node_type::array));
						if (not ok)
						{
							return unexpected(ok.error());
						}
						data.luco_objs.push(&ok.value().get());
						this->register_token(data, luco_syntax::transient_bracket);
						return true;
					}
					else
					{
						return unexpected(error(error_type::parsing_error, "expected '{{' or '=' encountered: '{}'",
									data.line[data.i]));
					}
					luco::expected<std::reference_wrapper<luco::node>, error> ok =
					    unexpected(error(error_type::none, "meow"));

					if (data.hierarchy.top().first == luco_syntax::opening_bracket ||
					    data.hierarchy.top().first == luco_syntax::equal_sign)
					{
						if (data.luco_objs.top()->is_object())
						{
							ok = data.luco_objs.top()->insert(data.keys.top().first,
											  luco::node(node_type::object));
						}
						else
						{
							ok = data.luco_objs.top()->push_back(luco::node(node_type::object));
						}
						data.keys.push(std::move(data.raw_value));
						data.raw_value.first.clear();
						data.raw_value.second = luco_value_type::none;
					}
					else if (data.hierarchy.top().first == luco_syntax::flush_value)
					{
						if (data.luco_objs.top()->is_object())
						{
							ok = data.luco_objs.top()->insert(data.keys.top().first,
											  luco::node(node_type::array));
						}
						else
						{
							ok = data.luco_objs.top()->push_back(luco::node(node_type::array));
						}
					}

					if (not ok)
					{
						return unexpected(ok.error());
					}
					data.luco_objs.push(&ok.value().get());
					return true;
				}

				if (this->is_registered_token(data, luco_syntax::transient_bracket))
				{
					assert(not data.keys.empty());

					if (data.raw_value.second == luco_value_type::none && this->is_newline(data.line[data.i]))
					{
						return true;
					}
					else if (luco_simple_types::handle_empty_in_string(data, data.raw_value.second, data.line[data.i]))
					{
						if (not luco_simple_types::end_of_string(data.raw_value.second))
						{
							data.raw_value.first += data.line[data.i];
						}
						return true;
					}
					else
					{
						return false;
					}
				}
				else
				{
					return false;
				}
			}

			inline void prepare_for_next_token(struct parsing_data& data, luco_syntax type) override
			{
				this->register_token(data, type);
			}

			inline bool is_token(struct parsing_data& data) override
			{
				assert(not data.hierarchy.empty());

				if (this->handle_is(data, is))
				{
					return true;
				}
				else
				{
					return false;
				}
			}
	};

	class luco_key : public token {
		private:
			inline bool is_end_of_token(struct parsing_data& data) override
			{
				if (not data.hierarchy.empty() && data.hierarchy.top().first == luco_syntax::key &&
				    this->handle_expected(data, this->expected))
				{
					return true;
				}

				return false;
			}

		public:
			inline luco_key()
			{
				this->is.insert(luco_syntax::object);

				this->_not.insert('\n');
				this->_not.insert('\t');
				this->_not.insert(' ');
				this->_not.insert('}');
				this->_not.insert('{');

				this->expected.insert(std::make_pair(luco_syntax::equal_sign, '='));
				this->expected.insert(std::make_pair(luco_syntax::opening_bracket, '{'));
			}

			inline ~luco_key()
			{
			}

			inline luco::expected<bool, luco::error> handle_token(struct parsing_data& data) override
			{
				if (this->is_token(data))
				{
					this->register_token(data, luco_syntax::key);
					data.keys.push(std::make_pair("", luco_value_type::none));
				}
				else if (this->is_end_of_token(data) &&
					 not luco_simple_types::expected_multi_line_string(data.keys.top().second))
				{
					this->unregister_token(data);
					this->prepare_for_next_token(data, this->delimiter(data, '=') ? luco_syntax::equal_sign
												      : luco_syntax::opening_bracket);
					return true;
				}

				if (this->is_registered_token(data, luco_syntax::key))
				{
					assert(not data.keys.empty());
					if (luco_simple_types::handle_empty_in_string(data, data.keys.top().second, data.line[data.i]))
					{
						if (not luco_simple_types::end_of_string(data.keys.top().second))
						{
							data.keys.top().first += data.line[data.i];
						}
						return true;
					}
					else
					{
						return false;
					}
					// else if (luco_simple_types::unexpected_multi_line_string(data.keys.top().second,
					// data.line[data.i]))
					// {
					// 	return unexpected(error(error_type::parsing_error, "unexpected newline: key"));
					// }
				}
				else
				{
					return false;
				}
			}

			inline void prepare_for_next_token(struct parsing_data& data, luco_syntax token_type) override
			{
				this->register_token(data, token_type);
			}

			inline bool is_token(struct parsing_data& data) override
			{
				assert(not data.hierarchy.empty());

				if (this->handle_is(data, is) && this->handle_not(data, _not))
				{
					return true;
				}
				else
				{
					return false;
				}
			}
	};

	class luco_value : public token {
		private:
			inline bool is_end_of_token(struct parsing_data& data) override
			{
				if (not data.hierarchy.empty() && data.hierarchy.top().first == luco_syntax::value &&
				    (this->handle_expected(data, this->expected) && not delimiter(data, '\\')))
				{
					return true;
				}
				else if (data.hierarchy.top().first == luco_syntax::flush_value)
				{
					data.shift_index_backward_for_oldnewline = true;
					return true;
				}

				return false;
			}

			inline luco::expected<bool, luco::error> insert_value(struct parsing_data& data)
			{
				luco_simple_types::strip_if_unqouted_string(data.raw_value.first, data.raw_value.second);

				std::variant<std::string, bool, double, int64_t, null_type> typed_value =
				    luco_simple_types::get_type(data.raw_value.first);

				if (data.luco_objs.top()->type() == node_type::object)
				{
					luco_simple_types::strip_if_unqouted_string(data.keys.top().first, data.keys.top().second);

					auto ok = data.luco_objs.top()->insert(data.keys.top().first, typed_value);
					if (not ok)
					{
						return unexpected(ok.error());
					}

					data.keys.pop();
				}
				else if (data.luco_objs.top()->type() == node_type::array)
				{
					auto ok = data.luco_objs.top()->push_back(typed_value);
					if (not ok)
					{
						return unexpected(ok.error());
					}
				}
				else
				{
					assert(true && "top object isn't an object or array");
				}
				data.raw_value.first.clear();
				data.raw_value.second = luco_value_type::none;

				return true;
			}

		public:
			inline luco_value()
			{
				this->is.insert(luco_syntax::equal_sign);
				this->is.insert(luco_syntax::array);

				this->_not.insert('\n');
				this->_not.insert('\t');
				this->_not.insert(' ');
				this->_not.insert('}');

				this->expected.insert(std::make_pair(luco_syntax::newline, '\n'));
				// this->expected.insert(std::make_pair(luco_syntax::flush_value, '\n'));
			}

			inline ~luco_value()
			{
			}

			inline luco::expected<bool, luco::error> handle_token(struct parsing_data& data) override
			{
				if (this->is_token(data))
				{
					this->prepare_for_next_token(data, luco_syntax::none);
					this->register_token(data, luco_syntax::value);
				}
				else if (this->is_end_of_token(data) &&
					 not luco_simple_types::expected_multi_line_string(data.raw_value.second))
				{
					auto ok = this->insert_value(data);
					this->unregister_token(data);
					return ok;
				}

				if (this->is_registered_token(data, luco_syntax::value))
				{
					assert(not data.keys.empty());
					this->is_escaped(data, data.line[data.i]);

					if (luco_simple_types::handle_empty_in_string(data, data.raw_value.second, data.line[data.i]))
					{
						if (token::delimiter(data, '='))
						{
							data.raw_value.second = luco_value_type::end_string_unqouted;
							return false;
						}
						if (not luco_simple_types::end_of_string(data.raw_value.second))
						{
							data.raw_value.first += data.line[data.i];
						}
						return true;
					}
					else if (data.line[data.i] == '{' && delimiter(data, '{'))
					{
						this->unregister_token(data);
						if (data.raw_value.second != luco_value_type::none)
						{
							auto ok = this->insert_value(data);
							if (not ok)
							{
								return ok;
							}
						}

						this->register_token(data, luco_syntax::transient_bracket);
						return true;
					}
					else
					{
						return false;
					}
				}
				else
				{
					return false;
				}
			}

			inline void prepare_for_next_token(struct parsing_data& data, luco_syntax) override
			{
				if (data.hierarchy.top().first == luco_syntax::equal_sign ||
				    data.hierarchy.top().first == luco_syntax::flush_value)
				{
					this->unregister_token(data);
				}
			}

			inline bool is_token(struct parsing_data& data) override
			{
				assert(not data.hierarchy.empty());

				if (this->handle_is(data, is) && this->handle_not(data, _not))
				{
					return true;
				}
				else
				{
					return false;
				}
			}
	};

	class closing_bracket : public token {
		public:
			inline closing_bracket()
			{
				this->expected.insert(std::make_pair(luco_syntax::closing_bracket, '}'));
				this->expected.insert(std::make_pair(luco_syntax::transient_bracket, '}'));

				this->is.insert(luco_syntax::object);
				this->is.insert(luco_syntax::array);
			}

			inline ~closing_bracket()
			{
			}

			inline luco::expected<bool, luco::error> handle_token(struct parsing_data& data) override
			{
				if (this->is_token(data))
				{
					assert(data.hierarchy.top().first == luco_syntax::object ||
					       data.hierarchy.top().first == luco_syntax::array);
					this->register_token(data, luco_syntax::closing_bracket);
				}

				if (this->is_registered_token(data, luco_syntax::closing_bracket))
				{
					this->unregister_token(data);
					assert(not data.keys.empty());
					data.keys.top().first.clear();
					data.keys.top().second = luco_value_type::none;

					if (data.hierarchy.top().first != luco_syntax::object &&
					    data.hierarchy.top().first != luco_syntax::array)
					{
						return unexpected(error(error_type::parsing_error, "encountered '}' without a '{'"));
					}
					this->prepare_for_next_token(data, luco_syntax::none);

					if (data.hierarchy.empty())
					{
						return unexpected(error(error_type::parsing_error,
									"{} encountered more '}}' than there is '{{'",
									error_location(data)));
					}

					return true;
				}
				else if (this->is_registered_token(data, luco_syntax::transient_bracket) && this->delimiter(data, '}'))
				{
					assert(data.raw_value.first.empty() && data.raw_value.second == luco_value_type::none);

					this->unregister_token(data);
					auto ok = data.luco_objs.top()->insert(data.keys.top().first, luco::node(node_type::object));
					if (not ok)
					{
						return unexpected(ok.error());
					}
					return true;
				}
				else
				{
					return false;
				}
			}

			inline void prepare_for_next_token(struct parsing_data& data, luco_syntax) override
			{
				assert(data.hierarchy.top().first == luco_syntax::object ||
				       data.hierarchy.top().first == luco_syntax::array);

				this->unregister_token(data);

				if (data.hierarchy.top().first == luco_syntax::object)
				{
					assert(not data.keys.empty());
					data.keys.pop();
				}

				assert(not data.luco_objs.empty());
				data.luco_objs.pop();
			}

			inline bool is_token(struct parsing_data& data) override
			{
				assert(not data.hierarchy.empty());

				if (this->handle_is(data, this->is) && delimiter(data, '}'))
				{
					return true;
				}
				else
				{
					return false;
				}
			}
	};

	struct syntax {
			class luco_key	      luco_key;
			class luco_value      luco_value;
			class opening_bracket opening_bracket;
			class closing_bracket closing_bracket;
			class comment	      comment;
	};

	inline luco::expected<bool, luco::error> syntax_error(struct parsing_data& data, const struct syntax&)
	{
		if (token::is_empty_newline(data.line[data.i]))
		{
			return false;
		}
		else if (data.hierarchy.empty())
		{
			return unexpected(error(luco::error_type::parsing_error, "{} the number of '}}' is more than the number of '{{'",
						error_location(data)));
		}
		else if (data.raw_value.second == luco_value_type::escaped_string_newline_qouted_1 && not token::delimiter(data, '\\'))
		{
			return unexpected(error(luco::error_type::parsing_error, "{} expected ''' on the new line string but found '{}'",
						error_location(data), data.line[data.i]));
		}
		else if (data.raw_value.second == luco_value_type::escaped_string_newline_qouted_2 && not token::delimiter(data, '\\'))
		{
			return unexpected(error(luco::error_type::parsing_error, "{} expected '\"' on the new line string but found '{}'",
						error_location(data), data.line[data.i]));
		}
		else if (data.raw_value.second == luco_value_type::escaped_string_newline_unqouted && data.eof)
		{
			return unexpected(error(luco::error_type::parsing_error,
						"{} expected a string on the new line but reached end of file", error_location(data)));
		}
		else if (data.raw_value.second == luco_value_type::end_string_1 || data.raw_value.second == luco_value_type::end_string_2)
		{
			return unexpected(error(luco::error_type::parsing_error,
						"{} expected a new line after [value] reaching end-of-string but found '{}'",
						error_location(data), data.line[data.i]));
		}
		else if (not data.keys.empty() && luco_simple_types::end_of_string(data.keys.top().second) && not data.hierarchy.empty() &&
			 data.hierarchy.top().first == luco_syntax::key)
		{
			return unexpected(error(luco::error_type::parsing_error,
						"{} expected '=' or '{{' after [key] reaching end-of-string but found '{}'",
						error_location(data), data.line[data.i]));
		}
		else if (luco_simple_types::end_of_string(data.raw_value.second) && not data.hierarchy.empty() &&
			 data.hierarchy.top().first == luco_syntax::value)
		{
			return unexpected(error(luco::error_type::parsing_error,
						"{} expected 'newline' after [value] reaching end-of-string but found '{}'",
						error_location(data), data.line[data.i]));
		}
		else if (not data.hierarchy.empty() && data.hierarchy.top().first == luco_syntax::object && token::delimiter(data, '{'))
		{
			return unexpected(error(luco::error_type::parsing_error, "{} expected 'key' in the [global object] but found '{}'",
						error_location(data), data.line[data.i]));
		}
		else if (token::delimiter(data, '}') && not data.hierarchy.empty() &&
			 (data.hierarchy.top().first != luco_syntax::object || data.hierarchy.top().first != luco_syntax::array))
		{
			return unexpected(error(luco::error_type::parsing_error, "{} found '}}' without being in an [object] or [array]",
						error_location(data), data.line[data.i]));
		}
		else
		{
			return false;
		}
	}

	inline bool parser::done_or_not_ok(const expected<bool, error>& ok)
	{
		if ((ok && ok.value()) || not ok)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	inline expected<monostate, error> parser::return_error_if_not_ok(const expected<bool, error>& ok)
	{
		if (not ok)
		{
			return unexpected(ok.error());
		}
		return monostate();
	}

	inline expected<monostate, error> parser::parsing(struct parsing_data& data, struct syntax& syntax)
	{
		expected<bool, error> ok;

		if (ok = syntax.comment.handle_token(data); done_or_not_ok(ok))
		{
			return return_error_if_not_ok(ok);
		}
		else if (ok = syntax.luco_key.handle_token(data); done_or_not_ok(ok))
		{
			return return_error_if_not_ok(ok);
		}
		else if (ok = syntax.luco_value.handle_token(data); done_or_not_ok(ok))
		{
			return return_error_if_not_ok(ok);
		}
		else if (ok = syntax.opening_bracket.handle_token(data); done_or_not_ok(ok))
		{
			return return_error_if_not_ok(ok);
		}
		else if (ok = syntax.closing_bracket.handle_token(data); done_or_not_ok(ok))
		{
			return return_error_if_not_ok(ok);
		}
		else if (ok = syntax_error(data, syntax); done_or_not_ok(ok))
		{
			return return_error_if_not_ok(ok);
		}
		else
		{
			return monostate();
		}
	}

	inline luco::node parser::parse(const std::filesystem::path& path)
	{
		expected<luco::node, error> ok = luco::parser::try_parse(path);
		if (not ok)
		{
			throw ok.error();
		}

		return ok.value();
	}

	inline luco::node parser::parse(const char* raw_json)
	{
		expected<luco::node, error> ok = luco::parser::try_parse(raw_json);
		if (not ok)
		{
			throw ok.error();
		}

		return ok.value();
	}

	inline luco::node parser::parse(const std::string& raw_json)
	{
		expected<luco::node, error> ok = luco::parser::try_parse(raw_json);
		if (not ok)
		{
			throw ok.error();
		}

		return ok.value();
	}

	inline expected<luco::node, error> parser::try_parse(const std::filesystem::path& path) noexcept
	{
		luco::node		       luco_data = luco::node(node_type::object);

		std::unique_ptr<std::ifstream> file	 = std::make_unique<std::ifstream>(path);
		if (not file->is_open())
		{
			return unexpected(luco::error(error_type::filesystem_error,
						      std::format("couldn't open '{}', {}", path.string(), std::strerror(errno))));
		}
		struct parsing_data data;
		struct syntax	    syntax;

		data.hierarchy.push(std::make_pair(luco_syntax::object, std::make_pair(1, 0)));
		data.luco_objs.push(&luco_data);
		data.keys.push({"", luco_value_type::none});

		expected<monostate, error> ok;

		while (std::getline(*file, data.line))
		{
			data.line += "\n";
			for (data.i = 0; data.i < data.line.size(); data.i++)
			{
				ok = luco::parser::parsing(data, syntax);
				if (not ok)
				{
					return unexpected(ok.error());
				}
				else if (data.shift_index_backward_for_oldnewline)
				{
					data.shift_index_backward_for_oldnewline = false;
					data.i--;
				}
			}

			if (not file->eof())
			{
				data.line.clear();
			}

			data.line_number++;
		}

		if (data.hierarchy.top().first == luco_syntax::nested_comment)
		{
			return unexpected(error(luco::error_type::parsing_error, "{} non-ending nested comment was encountered at",
						error_location(data, data.hierarchy.top().second)));
		}

		return luco_data;
	}

	inline expected<luco::node, error> parser::try_parse(const std::string& raw_json) noexcept
	{
		luco::node	    luco_data = luco::node(node_type::object);

		struct parsing_data data;
		struct syntax	    syntax;

		data.hierarchy.push(std::make_pair(luco_syntax::object, std::make_pair(1, 0)));
		data.luco_objs.push(&luco_data);
		data.keys.push({"", luco_value_type::none});

		expected<monostate, error> ok;

		for (size_t i = 0; i < raw_json.size(); i++)
		{
			data.line += raw_json[i];
			if (not data.line.empty() && (data.line.back() == '\n' || data.line.back() == ',' || data.line.back() == '}'))
			{

				for (data.i = 0; data.i < data.line.size(); data.i++)
				{
					ok = luco::parser::parsing(data, syntax);
					if (not ok)
					{
						return unexpected(ok.error());
					}
					else if (data.shift_index_backward_for_oldnewline)
					{
						data.shift_index_backward_for_oldnewline = false;
						data.i--;
					}
				}
				data.line.clear();

				data.line_number++;
			}
		}

		if (data.hierarchy.top().first == luco_syntax::nested_comment)
		{
			return unexpected(error(luco::error_type::parsing_error, "{} non-ending nested comment was encountered at",
						error_location(data, data.hierarchy.top().second)));
		}

		return luco_data;
	}

	inline expected<luco::node, error> parser::try_parse(const char* raw_json) noexcept
	{
		assert(raw_json != NULL);
		std::string string_json(raw_json);
		return luco::parser::try_parse(string_json);
	}
}
