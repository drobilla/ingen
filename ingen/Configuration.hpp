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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <list>
#include <map>
#include <string>

#include "ingen/Forge.hpp"
#include "ingen/URIMap.hpp"

#include "raul/Atom.hpp"
#include "raul/Exception.hpp"

namespace Ingen {

/** Ingen configuration (command line options and/or configuration file).
 * @ingroup IngenShared
 */
class Configuration {
public:
	explicit Configuration(Forge& forge);

	/** The scope of a configuration option.
	 *
	 * This controls when and where an option will be saved or restored.
	 */
	enum Scope {
		GLOBAL  = 1,     ///< Applies to any Ingen instance
		SESSION = 1<<1,  ///< Applies to this Ingen instance only
		GUI     = 1<<2   ///< Persistent GUI settings saved at exit
	};

	/** Add a configuration option.
	 *
	 * @param key URI local name, in camelCase
	 * @param name Long option name (without leading "--")
	 * @param letter Short option name (without leading "-")
	 * @param desc Description
	 * @param scope Scope of option
	 * @param type Type
	 * @param value Default value
	 */
	Configuration& add(const std::string&       key,
	                   const std::string&       name,
	                   char                     letter,
	                   const std::string&       desc,
	                   Scope                    scope,
	                   const Raul::Atom::TypeID type,
	                   const Raul::Atom&        value);

	void print_usage(const std::string& program, std::ostream& os);

	struct OptionError : public Raul::Exception {
		explicit OptionError(const std::string& m) : Exception(m) {}
	};

	struct FileError : public Raul::Exception {
		explicit FileError(const std::string& m) : Exception(m) {}
	};

	void parse(int argc, char** argv) throw (OptionError);

	/** Load a specific file. */
	bool load(const std::string& path);

	/** Save configuration to a file.
	 *
	 * @param filename If absolute, the configuration will be saved to this
	 * path.  Otherwise the configuration will be saved to the user
	 * configuration directory (e.g. ~/.config/ingen/filename).
	 *
	 * @param scopes Bitwise OR of Scope values.  Only options which match the
	 * given scopes will be saved.
	 *
	 * @return The absolute path of the saved configuration file.
	 */
	std::string save(URIMap&            uri_map,
	                 const std::string& app,
	                 const std::string& filename,
	                 unsigned           scopes) throw (FileError);

	/** Load files from the standard configuration directories for the app.
	 *
	 * The system configuration file(s), e.g. /etc/xdg/appname/filename,
	 * will be loaded before the user's, e.g. ~/.config/appname/filename,
	 * so the user options will override the system options.
	 */
	std::list<std::string> load_default(const std::string& app,
	                                    const std::string& filename);

	const Raul::Atom& option(const std::string& long_name) const;
	bool              set(const std::string& long_name, const Raul::Atom& value);

	const std::list<std::string>& files() const { return _files; }

private:
	struct Option {
	public:
		Option(const std::string& k, const std::string& n, char l,
		       const std::string& d, Scope s,
		       const Raul::Atom::TypeID type, const Raul::Atom& def)
			: key(k), name(n), letter(l)
			, desc(d), scope(s)
			, type(type), value(def)
		{}

		std::string        key;
		std::string        name;
		char               letter;
		std::string        desc;
		Scope              scope;
		Raul::Atom::TypeID type;
		Raul::Atom         value;
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

	int set_value_from_string(Configuration::Option& option,
	                          const std::string&     value)
	throw (Configuration::OptionError);

	Forge&            _forge;
	const std::string _shortdesc;
	const std::string _desc;
	Options           _options;
	Keys              _keys;
	ShortNames        _short_names;
	Files             _files;
	size_t            _max_name_length;
};

} // namespace Ingen

#endif // INGEN_CONFIGURATION_HPP

