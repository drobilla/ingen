/* This file is part of Ingen.
 * Copyright (C) 2007-2009 Dave Robillard <http://drobilla.net>
 *
 * Ingen is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "ingen-config.h"
#include <iostream>
#include <string>
#include <stdlib.h>
#include <signal.h>
#include <boost/optional.hpp>
#include <glibmm/convert.h>
#include <glibmm/miscutils.h>
#include <glibmm/thread.h>
#include "raul/log.hpp"
#include "raul/Configuration.hpp"
#include "raul/Path.hpp"
#include "raul/SharedPtr.hpp"
#include "redlandmm/World.hpp"
#include "interface/EngineInterface.hpp"
#include "shared/runtime_paths.hpp"
#include "module/ingen_module.hpp"
#include "module/Module.hpp"
#include "module/World.hpp"
#include "engine/Engine.hpp"
#include "serialisation/Parser.hpp"
#ifdef WITH_BINDINGS
#include "bindings/ingen_bindings.hpp"
#endif

using namespace std;
using namespace Raul;
using namespace Ingen;

SharedPtr<Ingen::Engine> engine;

void
ingen_interrupt(int)
{
	cout << "ingen: Interrupted" << endl;
	engine->quit();
}

void
ingen_abort(const char* msg)
{
	cerr << "ingen: Error: " << msg << endl;
	ingen_destroy_world();
	exit(1);
}

int
main(int argc, char** argv)
{
	Raul::Configuration conf("A realtime modular audio processor.",
	"Ingen is a flexible modular system that be used in various ways.\n"
	"The engine can run as a stand-alone server controlled via a network protocol\n"
	"(e.g. OSC), or internal to another process (e.g. the GUI).  The GUI, or other\n"
	"clients, can communicate with the engine via any supported protocol, or host the\n"
	"engine in the same process.  Many clients can connect to an engine at once.\n\n"
	"Examples:\n"
	"  ingen -e                     # Run an engine, listen for OSC\n"
	"  ingen -g                     # Run a GUI, connect via OSC\n"
	"  ingen -eg                    # Run an engine and a GUI in one process\n"
	"  ingen -egl patch.ingen.ttl   # Run an engine and a GUI and load a patch");

	conf.add("client-port", 'C', "Client OSC port", Atom::INT, Atom())
		.add("connect",     'c', "Connect to engine URI", Atom::STRING, "osc.udp://localhost:16180")
		.add("engine",      'e', "Run (JACK) engine", Atom::BOOL, false)
		.add("engine-port", 'E', "Engine listen port", Atom::INT,  16180)
		.add("gui",         'g', "Launch the GTK graphical interface", Atom::BOOL, false)
		.add("help",        'h', "Print this help message", Atom::BOOL, false)
		.add("jack-client", 'n', "JACK client name", Atom::STRING, "ingen")
		.add("jack-server", 's', "JACK server name", Atom::STRING, "default")
		.add("load",        'l', "Load patch", Atom::STRING, Atom())
		.add("parallelism", 'p', "Number of concurrent process threads", Atom::INT, 1)
		.add("path",        'L', "Target path for loaded patch", Atom::STRING, Atom())
		.add("run",         'r', "Run script", Atom::STRING, Atom());

	// Parse command line options
	try {
		conf.parse(argc, argv);
	} catch (std::exception& e) {
		cout << "ingen: " << e.what() << endl;
		return EXIT_FAILURE;
	}

	// Verify option sanity
	if (argc <= 1) {
		conf.print_usage("ingen", cout);
		return EXIT_FAILURE;
	} else if (conf.option("help").get_bool()) {
		conf.print_usage("ingen", cout);
		return EXIT_SUCCESS;
	}

	// Set bundle path from executable location so resources/modules can be found
	Shared::set_bundle_path_from_code((void*)&main);

	SharedPtr<Shared::EngineInterface> engine_interface;

	Glib::thread_init();
#if HAVE_SOUP
	g_type_init();
#endif

	Ingen::Shared::World* world = ingen_get_world();

	world->argc = argc;
	world->argv = argv;
	world->conf = &conf;

	// Set up RDF namespaces
	world->rdf_world->add_prefix("dc",      "http://purl.org/dc/elements/1.1/");
	world->rdf_world->add_prefix("doap",    "http://usefulinc.com/ns/doap#");
	world->rdf_world->add_prefix("ingen",   "http://drobilla.net/ns/ingen#");
	world->rdf_world->add_prefix("ingenui", "http://drobilla.net/ns/ingenuity#");
	world->rdf_world->add_prefix("lv2",     "http://lv2plug.in/ns/lv2core#");
	world->rdf_world->add_prefix("lv2ev",   "http://lv2plug.in/ns/ext/event#");
	world->rdf_world->add_prefix("lv2midi", "http://lv2plug.in/ns/ext/midi#");
	world->rdf_world->add_prefix("owl",     "http://www.w3.org/2002/07/owl#");
	world->rdf_world->add_prefix("rdfs",    "http://www.w3.org/2000/01/rdf-schema#");
	world->rdf_world->add_prefix("sp",      "http://lv2plug.in/ns/dev/string-port#");
	world->rdf_world->add_prefix("xsd",     "http://www.w3.org/2001/XMLSchema#");

	// Run engine
	if (conf.option("engine").get_bool()) {
		if (!world->load("ingen_engine"))
			ingen_abort("Unable to load engine module");

		if (!world->local_engine)
			ingen_abort("Unable to create engine");

		engine = world->local_engine;
		engine_interface = world->engine;

		// Not loading a GUI, load network engine interfaces
		if (!conf.option("gui").get_bool()) {
			#ifdef HAVE_LIBLO
			if (!world->load("ingen_osc"))
				ingen_abort("Unable to load OSC module");
			#endif
			#ifdef HAVE_SOUP
			if (!world->load("ingen_http"))
				ingen_abort("Unable to load HTTP module");
			#endif
		}
	}

	// Load client library (required for patch loading and/or GUI)
	if (conf.option("load").is_valid() || conf.option("gui").get_bool())
		if (!world->load("ingen_client"))
			ingen_abort("Unable to load client module");

	// If we don't have a local engine interface (for GUI), use network
	if (!engine_interface) {
		const char* const uri = conf.option("connect").get_string();
		if (!(engine_interface = world->interface(uri)))
			ingen_abort((string("Unable to create interface to `") + uri + "'").c_str());
	}

	// Activate the engine, if we have one
	if (engine) {
		if (!world->load("ingen_jack"))
			ingen_abort("Unable to load jack module");

		engine->activate();
	}

	world->engine = engine_interface;

	// Load a patch
	if (conf.option("load").is_valid() && engine_interface) {
		boost::optional<Path>   data_path = Path("/");
		boost::optional<Path>   parent;
		boost::optional<Symbol> symbol;
		const Raul::Atom&       path_option = conf.option("path");

		if (path_option.is_valid()) {
			if (Path::is_valid(path_option.get_string())) {
				const Path p(path_option.get_string());
				if (!p.is_root()) {
					parent = p.parent();
					symbol = p.symbol();
				}
			} else {
				cerr << "Invalid path given: '" << path_option << endl;
			}
		}

		if (!world->load("ingen_serialisation"))
			ingen_abort("Unable to load serialisation module");

		if (world->parser) {
			// Assumption:  Containing ':' means URI, otherwise filename
			string uri = conf.option("load").get_string();
			if (uri.find(':') == string::npos) {
				if (Glib::path_is_absolute(uri))
					uri = Glib::filename_to_uri(uri);
				else
					uri = Glib::filename_to_uri(Glib::build_filename(
						Glib::get_current_dir(), uri));
			}

			engine_interface->load_plugins();
			world->parser->parse_document(
					world, engine_interface.get(), uri, data_path, parent, symbol);

		} else {
			ingen_abort("Unable to create parser");
		}
	}

	// Load GUI
	if (conf.option("gui").get_bool())
		if (!world->load("ingen_gui"))
			ingen_abort("Unable to load GUI module");

	// Run a script
	if (conf.option("run").is_valid()) {
#ifdef WITH_BINDINGS
		if (!world->load("ingen_bindings"))
			ingen_abort("Unable to load bindings module");

		world->run("application/x-python", conf.option("run").get_string());
#else
		cerr << "This build of ingen does not support scripting." << endl;
#endif

	// Listen to OSC and run main loop
	} else if (engine && !conf.option("gui").get_bool()) {
		signal(SIGINT, ingen_interrupt);
		signal(SIGTERM, ingen_interrupt);
		engine->main(); // Block here
	}

	// Shut down
	if (engine) {
		engine->deactivate();
		engine.reset();
	}

	ingen_destroy_world();

	return 0;
}

