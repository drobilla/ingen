/*
  This file is part of Ingen.
  Copyright 2007-2015 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <iostream>

#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

#include "ingen/Configuration.hpp"
#include "ingen/Log.hpp"
#include "ingen/ingen.h"
#include "sord/sordmm.hpp"
#include "sratom/sratom.h"

namespace Ingen {

Configuration::Configuration(Forge& forge)
	: _forge(forge)
	, _shortdesc("A realtime modular audio processor.")
	, _desc(
		"Ingen is a flexible modular system that be used in various ways.\n"
		"The engine can run as a server controlled via a network protocol,\n"
		"as an LV2 plugin, or in a monolithic process with a GUI.  The GUI\n"
		"may be run separate to control a remote engine, and many clients\n"
		"may connect to an engine at once.\n\n"
		"Examples:\n"
		"  ingen -e             # Run engine, listen for connections\n"
		"  ingen -g             # Run GUI, connect to running engine\n"
		"  ingen -eg            # Run engine and GUI in one process\n"
		"  ingen -eg foo.ingen  # Run engine and GUI and load a graph")
	, _max_name_length(0)
{
	add("clientPort",     "client-port",    'C', "Client port", SESSION, forge.Int, Atom());
	add("connect",        "connect",        'c', "Connect to engine URI", SESSION, forge.String, forge.alloc("unix:///tmp/ingen.sock"));
	add("engine",         "engine",         'e', "Run (JACK) engine", SESSION, forge.Bool, forge.make(false));
	add("enginePort",     "engine-port",    'E', "Engine listen port", SESSION, forge.Int, forge.make(16180));
	add("socket",         "socket",         'S', "Engine socket path", SESSION, forge.String, forge.alloc("/tmp/ingen.sock"));
	add("gui",            "gui",            'g', "Launch the GTK graphical interface", SESSION, forge.Bool, forge.make(false));
	add("",               "help",           'h', "Print this help message", SESSION, forge.Bool, forge.make(false));
	add("",               "version",        'V', "Print version information", SESSION, forge.Bool, forge.make(false));
	add("jackName",       "jack-name",      'n', "JACK name", SESSION, forge.String, forge.alloc("ingen"));
	add("jackServer",     "jack-server",    's', "JACK server name", GLOBAL, forge.String, forge.alloc(""));
	add("uuid",           "uuid",           'u', "JACK session UUID", SESSION, forge.String, Atom());
	add("load",           "load",           'l', "Load graph", SESSION, forge.String, Atom());
	add("path",           "path",           'L', "Target path for loaded graph", SESSION, forge.String, Atom());
	add("queueSize",      "queue-size",     'q', "Event queue size", GLOBAL, forge.Int, forge.make(4096));
	add("humanNames",     "human-names",     0,  "Show human names in GUI", GUI, forge.Bool, forge.make(true));
	add("portLabels",     "port-labels",     0,  "Show port labels in GUI", GUI, forge.Bool, forge.make(true));
	add("graphDirectory", "graph-directory", 0,  "Default directory for opening graphs", GUI, forge.String, Atom());
}

Configuration&
Configuration::add(const std::string& key,
                   const std::string& name,
                   char               letter,
                   const std::string& desc,
                   Scope              scope,
                   const LV2_URID     type,
                   const Atom&        value)
{
	assert(value.type() == type || value.type() == 0);
	_max_name_length = std::max(_max_name_length, name.length());
	_options.insert(make_pair(name, Option(key, name, letter, desc, scope, type, value)));
	if (!key.empty()) {
		_keys.insert(make_pair(key, name));
	}
	if (letter != '\0') {
		_short_names.insert(make_pair(letter, name));
	}
	return *this;
}

std::string
Configuration::variable_string(LV2_URID type) const
{
	if (type == _forge.String) {
		return "=STRING";
	} else if (type == _forge.Int) {
		return "=INT";
	}
	return "";
}

void
Configuration::print_usage(const std::string& program, std::ostream& os)
{
	os << "Usage: " << program << " [OPTION]... [GRAPH]" << std::endl;
	os << _shortdesc << std::endl << std::endl;
	os << _desc << std::endl << std::endl;
	os << "Options:" << std::endl;
	for (const auto& o : _options) {
		const Option& option = o.second;
		os << "  ";
		if (option.letter != '\0')
			os << "-" << option.letter << ", ";
		else
			os << "    ";
		os.width(_max_name_length + 11);
		os << std::left;
		os << (std::string("--") + o.first + variable_string(option.type));
		os << option.desc << std::endl;
	}
}

int
Configuration::set_value_from_string(Configuration::Option& option,
                                     const std::string&     value)
		throw (Configuration::OptionError)
{
	if (option.type == _forge.Int) {
		char* endptr = NULL;
		int   intval = static_cast<int>(strtol(value.c_str(), &endptr, 10));
		if (endptr && *endptr == '\0') {
			option.value = _forge.make(intval);
		} else {
			throw OptionError(
				(fmt("Option `%1%' has non-integer value `%2%'")
				 % option.name % value).str());
		}
	} else if (option.type == _forge.String) {
		option.value = _forge.alloc(value.c_str());
		assert(option.value.type() == _forge.String);
	} else if (option.type == _forge.Bool) {
		option.value = _forge.make(bool(!strcmp(value.c_str(), "true")));
		assert(option.value.type() == _forge.Bool);
	} else {
		throw OptionError(
			(fmt("Bad option type `%1%'") % option.name).str());
	}
	return EXIT_SUCCESS;
}

/** Parse command line arguments. */
void
Configuration::parse(int argc, char** argv) throw (Configuration::OptionError)
{
	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] != '-' || !strcmp(argv[i], "-")) {
			// File argument
			const Options::iterator o = _options.find("load");
			if (!o->second.value.is_valid()) {
				_options.find("load")->second.value = _forge.alloc(argv[i]);
			} else {
				throw OptionError("Multiple graphs specified");
			}
		} else if (argv[i][1] == '-') {
			// Long option
			std::string name   = std::string(argv[i]).substr(2);
			const char* equals = strchr(argv[i], '=');
			if (equals) {
				name = name.substr(0, name.find('='));
			}

			const Options::iterator o = _options.find(name);
			if (o == _options.end()) {
				throw OptionError(
					(fmt("Unrecognized option `%1%'") % name).str());
			} else if (o->second.type == _forge.Bool) {  // --flag
				o->second.value = _forge.make(true);
			} else if (equals) {  // --opt=val
				set_value_from_string(o->second, equals + 1);
			} else if (++i < argc) {  // --opt val
				set_value_from_string(o->second, argv[i]);
			} else {
				throw OptionError(
					(fmt("Missing value for `%1%'") % name).str());
			}
		} else {
			// Short option
			const size_t len = strlen(argv[i]);
			for (size_t j = 1; j < len; ++j) {
				const char                 letter = argv[i][j];
				const ShortNames::iterator n      = _short_names.find(letter);
				if (n == _short_names.end()) {
					throw OptionError(
						(fmt("Unrecognized option `%1%'") % letter).str());
				}

				const Options::iterator o = _options.find(n->second);
				if (j < len - 1) {  // Non-final POSIX style flag
					if (o->second.type != _forge.Bool) {
						throw OptionError(
							(fmt("Missing value for `%1%'") % letter).str());
					}
					o->second.value = _forge.make(true);
				} else if (o->second.type == _forge.Bool) {  // -f
					o->second.value = _forge.make(true);
				} else if (++i < argc) {  // -v val
					set_value_from_string(o->second, argv[i]);
				} else {
					throw OptionError(
						(fmt("Missing value for `%1%'") % letter).str());
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
		if (pred.to_string().substr(0, sizeof(INGEN_NS) - 1) == INGEN_NS) {
			const std::string key = pred.to_string().substr(sizeof(INGEN_NS) - 1);
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

std::string
Configuration::save(URIMap&            uri_map,
                    const std::string& app,
                    const std::string& filename,
                    unsigned           scopes)
		throw (FileError)
{
	// Save to file if it is absolute, otherwise save to user config dir
	std::string path = filename;
	if (!Glib::path_is_absolute(path)) {
		path = Glib::build_filename(
			Glib::build_filename(Glib::get_user_config_dir(), app), filename);
	}

	// Create parent directories if necessary
	const std::string dir = Glib::path_get_dirname(path);
	if (g_mkdir_with_parents(dir.c_str(), 0755) < 0) {
		throw FileError((fmt("Error creating directory %1% (%2%)")
		                 % dir % strerror(errno)).str());
	}

	// Attempt to open file for writing
	FILE* file = fopen(path.c_str(), "w");
	if (!file) {
		throw FileError((fmt("Failed to open file %1% (%2%)")
		                 % path % strerror(errno)).str());
	}

	// Use the file's URI as the base URI
	SerdURI  base_uri;
	SerdNode base = serd_node_new_file_uri(
		(const uint8_t*)path.c_str(), NULL, &base_uri, true);

	// Create environment with ingen prefix
	SerdEnv* env = serd_env_new(&base);
	serd_env_set_prefix_from_strings(
		env, (const uint8_t*)"ingen", (const uint8_t*)INGEN_NS);

	// Create Turtle writer
	SerdWriter* writer = serd_writer_new(
		SERD_TURTLE,
		(SerdStyle)(SERD_STYLE_RESOLVED|SERD_STYLE_ABBREVIATED),
		env,
		&base_uri,
		serd_file_sink,
		file);

	// Write a prefix directive for each prefix in the environment
	serd_env_foreach(env, (SerdPrefixSink)serd_writer_set_prefix, writer);

	// Create an atom serialiser and connect it to the Turtle writer
	Sratom* sratom = sratom_new(&uri_map.urid_map_feature()->urid_map);
	sratom_set_pretty_numbers(sratom, true);
	sratom_set_sink(sratom, (const char*)base.buf,
	                (SerdStatementSink)serd_writer_write_statement, NULL,
	                writer);

	// Write a statement for each valid option
	for (auto o : _options) {
		const Atom& value = o.second.value;
		if (!(o.second.scope & scopes) ||
		    o.second.key.empty() ||
		    !value.is_valid()) {
			continue;
		}

		const std::string key(std::string("ingen:") + o.second.key);
		SerdNode pred = serd_node_from_string(
			SERD_CURIE, (const uint8_t*)key.c_str());
		sratom_write(sratom, &uri_map.urid_unmap_feature()->urid_unmap, 0,
		             &base, &pred, value.type(), value.size(), value.get_body());
	}

	sratom_free(sratom);
	serd_writer_free(writer);
	serd_env_free(env);
	serd_node_free(&base);
	fclose(file);

	return path;
}

std::list<std::string>
Configuration::load_default(const std::string& app,
                            const std::string& filename)
{
	std::list<std::string> loaded;

	const std::vector<std::string> dirs = Glib::get_system_config_dirs();
	for (auto d : dirs) {
		const std::string path = Glib::build_filename(
			Glib::build_filename(d, app), filename);
		if (load(path)) {
			loaded.push_back(path);
		}
	}

	const std::string path = Glib::build_filename(
		Glib::build_filename(Glib::get_user_config_dir(), app), filename);
	if (load(path)) {
		loaded.push_back(path);
	}

	return loaded;
}

const Atom&
Configuration::option(const std::string& long_name) const
{
	static const Atom nil;
	Options::const_iterator o = _options.find(long_name);
	if (o == _options.end()) {
		return nil;
	} else {
		return o->second.value;
	}
}

bool
Configuration::set(const std::string& long_name, const Atom& value)
{
	Options::iterator o = _options.find(long_name);
	if (o != _options.end()) {
		o->second.value = value;
		return true;
	}
	return false;
}

} // namespace Ingen
