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

#include <iostream>

#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

#include "ingen/Configuration.hpp"
#include "raul/fmt.hpp"
#include "sord/sordmm.hpp"

#define NS_INGEN "http://drobilla.net/ns/ingen#"

namespace Ingen {

Configuration::Configuration()
	: _shortdesc("A realtime modular audio processor.")
	, _desc(
"Ingen is a flexible modular system that be used in various ways.\n"
"The engine can run as a stand-alone server controlled via network protocol,\n"
"or internal to another process (e.g. the GUI).  The GUI, or other\n"
"clients, can communicate with the engine via any supported protocol, or host the\n"
"engine in the same process.  Many clients can connect to an engine at once.\n\n"
"Examples:\n"
"  ingen -e              # Run an engine, listen for connections\n"
"  ingen -g              # Run a GUI, connect to running engine\n"
"  ingen -eg             # Run an engine and a GUI in one process\n"
"  ingen -egl foo.ttl    # Run an engine and a GUI and load a graph\n"
"  ingen -egl foo.ingen  # Run an engine and a GUI and load a graph")
	, _max_name_length(0)
{
	add("clientPort",  "client-port", 'C', "Client port", INT, Value());
	add("connect",     "connect",     'c', "Connect to engine URI", STRING, Value("unix:///tmp/ingen.sock"));
	add("engine",      "engine",      'e', "Run (JACK) engine", BOOL, Value(false));
	add("enginePort",  "engine-port", 'E', "Engine listen port", INT, Value(16180));
	add("socket",      "socket",      'S', "Engine socket path", STRING, Value("/tmp/ingen.sock"));
	add("gui",         "gui",         'g', "Launch the GTK graphical interface", BOOL, Value(false));
	add("",            "help",        'h', "Print this help message", BOOL, Value(false));
	add("jackName",    "jack-name",   'n', "JACK name", STRING, Value("ingen"));
	add("jackServer",  "jack-server", 's', "JACK server name", STRING, Value(""));
	add("uuid",        "uuid",        'u', "JACK session UUID", STRING, Value());
	add("load",        "load",        'l', "Load graph", STRING, Value());
	add("path",        "path",        'L', "Target path for loaded graph", STRING, Value());
	add("queueSize",   "queue-size",  'q', "Event queue size", INT, Value(4096));
	add("run",         "run",         'r', "Run script", STRING, Value());
}

Configuration&
Configuration::add(const std::string& key,
                   const std::string& name,
                   char               letter,
                   const std::string& desc,
                   const OptionType   type,
                   const Value&       value)
{
	assert(value.type() == type || value.type() == 0);
	_max_name_length = std::max(_max_name_length, name.length());
	_options.insert(make_pair(name, Option(name, letter, desc, type, value)));
	if (!key.empty()) {
		_keys.insert(make_pair(key, name));
	}
	if (letter != '\0') {
		_short_names.insert(make_pair(letter, name));
	}
	return *this;
}

void
Configuration::print_usage(const std::string& program, std::ostream& os)
{
	os << "Usage: " << program << " [OPTION]..." << std::endl;
	os << _shortdesc << std::endl << std::endl;
	os << _desc << std::endl << std::endl;
	os << "Options:" << std::endl;
	for (Options::iterator o = _options.begin(); o != _options.end(); ++o) {
		Option& option = o->second;
			os << "  ";
		if (option.letter != '\0')
			os << "-" << option.letter << ", ";
		else
			os << "    ";
		os.width(_max_name_length + 4);
		os << std::left << (std::string("--") + o->first);
		os << option.desc << std::endl;
	}
}

int
Configuration::set_value_from_string(Configuration::Option& option,
                                     const std::string&     value)
		throw (Configuration::CommandLineError)
{
	int   intval = 0;
	char* endptr = NULL;
	switch (option.type) {
	case INT:
		intval = static_cast<int>(strtol(value.c_str(), &endptr, 10));
		if (endptr && *endptr == '\0') {
			option.value = Value(intval);
		} else {
			throw CommandLineError(
				(Raul::fmt("option `%1%' has non-integer value `%2%'")
				 % option.name % value).str());
		}
		break;
	case STRING:
		option.value = Value(value.c_str());
		assert(option.value.type() == STRING);
		break;
	default:
		throw CommandLineError(
			(Raul::fmt("bad option type `%1%'") % option.name).str());
	}
	return EXIT_SUCCESS;
}

/** Parse command line arguments. */
void
Configuration::parse(int argc, char** argv) throw (Configuration::CommandLineError)
{
	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] != '-' || !strcmp(argv[i], "-")) {
			_files.push_back(argv[i]);
		} else if (argv[i][1] == '-') {
			const std::string name = std::string(argv[i]).substr(2);
			Options::iterator o = _options.find(name);
			if (o == _options.end()) {
				throw CommandLineError(
					(Raul::fmt("unrecognized option `%1%'") % name).str());
			}
			if (o->second.type == BOOL) {
				o->second.value = Value(true);
			} else {
				if (++i >= argc)
					throw CommandLineError(
						(Raul::fmt("missing value for `%1'") % name).str());
				set_value_from_string(o->second, argv[i]);
			}
		} else {
			const size_t len = strlen(argv[i]);
			for (size_t j = 1; j < len; ++j) {
				char letter = argv[i][j];
				ShortNames::iterator n = _short_names.find(letter);
				if (n == _short_names.end())
					throw CommandLineError(
						(Raul::fmt("unrecognized option `%1%'") % letter).str());
				Options::iterator o = _options.find(n->second);
				if (j < len - 1) {
					if (o->second.type != BOOL)
						throw CommandLineError(
							(Raul::fmt("missing value for `%1%'") % letter).str());
					o->second.value = Value(true);
				} else {
					if (o->second.type == BOOL) {
						o->second.value = Value(true);
					} else {
						if (++i >= argc)
							throw CommandLineError(
								(Raul::fmt("missing value for `%1%'") % letter).str());
						set_value_from_string(o->second, argv[i]);
					}
				}
			}
		}
	}
}

bool
Configuration::load(const std::string& path)
{
	if (!Glib::file_test(path, Glib::FILE_TEST_EXISTS)) {
		return false;
	}

	SerdNode node = serd_node_new_file_uri(
		(const uint8_t*)path.c_str(), NULL, NULL, true);
	const std::string uri((const char*)node.buf);

	Sord::World world;
	Sord::Model model(world, uri, SORD_SPO, false);
	SerdEnv*    env = serd_env_new(&node);
	model.load_file(env, SERD_TURTLE, uri, uri);

	Sord::Node nodemm(world, Sord::Node::URI, (const char*)node.buf);
	Sord::Node nil;
	for (Sord::Iter i = model.find(nodemm, nil, nil); !i.end(); ++i) {
		const Sord::Node& pred = i.get_predicate();
		const Sord::Node& obj  = i.get_object();
		if (pred.to_string().substr(0, sizeof(NS_INGEN) - 1) == NS_INGEN) {
			const std::string key = pred.to_string().substr(sizeof(NS_INGEN) - 1);
			const Keys::iterator k = _keys.find(key);
			if (k != _keys.end() && obj.type() == Sord::Node::LITERAL) {
				set_value_from_string(_options.find(k->second)->second,
				                      obj.to_string());
			}
		}
	}
	     
	serd_node_free(&node);
	serd_env_free(env);
	return true;
}

std::list<std::string>
Configuration::load_default(const std::string& app, const std::string& file)
{
	std::list<std::string> loaded;

	const std::vector<std::string> dirs = Glib::get_system_config_dirs();
	for (std::vector<std::string>::const_iterator i = dirs.begin();
	     i != dirs.end();
	     ++i) {
		const std::string path = Glib::build_filename(*i, app, file);
		if (load(path)) {
			loaded.push_back(path);
		}
	}

	const std::string path = Glib::build_filename(
		Glib::get_user_config_dir(), app, file);
	if (load(path)) {
		loaded.push_back(path);
	}

	return loaded;
}

void
Configuration::print(std::ostream& os, const std::string mime_type) const
{
	for (Options::const_iterator o = _options.begin(); o != _options.end(); ++o) {
		const Option& option = o->second;
		os << o->first << " = " << option.value << std::endl;
	}
}

const Configuration::Value&
Configuration::option(const std::string& long_name) const
{
	static const Value nil;
	Options::const_iterator o = _options.find(long_name);
	if (o == _options.end()) {
		return nil;
	} else {
		return o->second.value;
	}
}

bool
Configuration::set(const std::string& long_name, const Value& value)
{
	Options::iterator o = _options.find(long_name);
	if (o != _options.end()) {
		o->second.value = value;
		return true;
	}
	return false;
}

} // namespace Ingen
