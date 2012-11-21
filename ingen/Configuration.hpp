/*
  This file is part of Ingen.
  Copyright 2007-2012 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INGEN_CONFIGURATION_HPP
#define INGEN_CONFIGURATION_HPP

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <list>
#include <map>
#include <string>

#include "raul/Exception.hpp"

namespace Ingen {

/** Ingen configuration (command line options and/or configuration file).
 * @ingroup IngenShared
 */
class Configuration {
public:
	Configuration();

	enum OptionType {
		NOTHING,
		BOOL,
		INT,
		STRING
	};

	class Value {
	public:
		Value()       : _type(NOTHING) { _val._string = NULL; }
		Value(bool v) : _type(BOOL)    { _val._bool = v; }
		Value(int  v) : _type(INT)     { _val._int = v; }

		Value(const char* v) : _type(STRING) {
			const size_t len = strlen(v);
			_val._string = (char*)calloc(len + 1, 1);
			memcpy(_val._string, v, len + 1);
		}

		Value(const std::string& v) : _type(STRING) {
			_val._string = (char*)calloc(v.length() + 1, 1);
			memcpy(_val._string, v.c_str(), v.length() + 1);
		}

		Value(const Value& copy)
			: _type(copy._type)
			, _val(copy._val)
		{
			if (_type == STRING) {
				const size_t len = strlen(copy.get_string());
				_val._string = (char*)malloc(len + 1);
				memcpy(_val._string, copy.get_string(), len + 1);
			}
		}

		Value& operator=(const Value& other)
		{
			if (&other == this) {
				return *this;
			}
			if (_type == STRING) {
				free(_val._string);
			}
			_type = other._type;
			_val  = other._val;
			if (_type == STRING) {
				const size_t len = strlen(other.get_string());
				_val._string = (char*)malloc(len + 1);
				memcpy(_val._string, other.get_string(), len + 1);
			}
			return *this;
		}

		~Value() {
			if (_type == STRING) {
				free(_val._string);
			}
		}

		inline OptionType type()     const { return _type; }
		inline bool       is_valid() const { return _type != NOTHING; }

		inline int32_t     get_int()    const { assert(_type == INT);    return _val._int; }
		inline bool        get_bool()   const { assert(_type == BOOL);   return _val._bool; }
		inline const char* get_string() const { assert(_type == STRING); return _val._string; }

	private:
		OptionType _type;
		union {
			bool    _bool;
			int32_t _int;
			char*   _string;
		} _val;
	};

	/** Add a configuration option.
	 *
	 * @param key URI local name, in camelCase
	 * @param name Long option name (without leading "--")
	 * @param letter Short option name (without leading "-")
	 * @param desc Description
	 * @param type Type
	 * @param value Default value
	 */
	Configuration& add(const std::string& key,
	                   const std::string& name,
	                   char               letter,
	                   const std::string& desc,
	                   const OptionType   type,
	                   const Value&       value);

	void print_usage(const std::string& program, std::ostream& os);

	struct CommandLineError : public Raul::Exception {
		explicit CommandLineError(const std::string& m) : Exception(m) {}
	};

	void parse(int argc, char** argv) throw (CommandLineError);

	/** Load a specific file. */
	bool load(const std::string& path);

	/** Load files from the standard configuration directories for the app.
	 *
	 * The system configuration file(s), e.g. /etc/xdg/appname/filename,
	 * will be loaded before the user's, e.g. ~/.config/appname/filename,
	 * so the user options will override the system options.
	 */
	std::list<std::string> load_default(const std::string& app,
	                                    const std::string& file);

	void print(std::ostream& os, const std::string mime_type="text/plain") const;

	const Value& option(const std::string& long_name) const;
	bool         set(const std::string& long_name, const Value& value);

	const std::list<std::string>& files() const { return _files; }

private:
	struct Option {
	public:
		Option(const std::string& n, char l, const std::string& d,
		       const OptionType type, const Value& def)
			: name(n), letter(l), desc(d), type(type), value(def)
		{}

		std::string name;
		char        letter;
		std::string desc;
		OptionType  type;
		Value       value;
	};

	struct OptionNameOrder {
		inline bool operator()(const Option& a, const Option& b) {
			return a.name < b.name;
		}
	};

	typedef std::map<std::string, Option>      Options;
	typedef std::map<char, std::string>        ShortNames;
	typedef std::map<std::string, std::string> Keys;
	typedef std::list<std::string>             Files;

	int set_value_from_string(Configuration::Option& option, const std::string& value)
			throw (Configuration::CommandLineError);

	const std::string _shortdesc;
	const std::string _desc;
	Options           _options;
	Keys              _keys;
	ShortNames        _short_names;
	Files             _files;
	size_t            _max_name_length;
};

} // namespace Ingen

static inline std::ostream&
operator<<(std::ostream& os, const Ingen::Configuration::Value& value)
{
	switch (value.type()) {
	case Ingen::Configuration::NOTHING: return os << "(nil)";
	case Ingen::Configuration::INT:     return os << value.get_int();
	case Ingen::Configuration::BOOL:    return os << (value.get_bool() ? "true" : "false");
	case Ingen::Configuration::STRING:  return os << value.get_string();
	}
	return os;
}

#endif // INGEN_CONFIGURATION_HPP

